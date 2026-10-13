// Copyright (c) 2026 by Rockchip Electronics Co., Ltd. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "yolo26.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <algorithm>
#include <vector>
#include <stdint.h>

#define LABEL_NALE_TXT_PATH "./model/coco_80_labels_list.txt"

static char *labels[OBJ_CLASS_NUM];

inline static int clamp(float val, int min, int max) { return val > min ? (val < max ? val : max) : min; }

static char *readLine(FILE *fp, char *buffer, int *len)
{
    int ch;
    int i = 0;
    size_t buff_len = 0;

    buffer = (char *)malloc(buff_len + 1);
    if (!buffer)
        return NULL; // Out of memory

    while ((ch = fgetc(fp)) != '\n' && ch != EOF)
    {
        buff_len++;
        void *tmp = realloc(buffer, buff_len + 1);
        if (tmp == NULL)
        {
            free(buffer);
            return NULL; // Out of memory
        }
        buffer = (char *)tmp;

        buffer[i] = (char)ch;
        i++;
    }
    buffer[i] = '\0';

    *len = buff_len;

    // Detect end
    if (ch == EOF && (i == 0 || ferror(fp)))
    {
        free(buffer);
        return NULL;
    }
    return buffer;
}

static int readLines(const char *fileName, char *lines[], int max_line)
{
    FILE *file = fopen(fileName, "r");
    char *s;
    int i = 0;
    int n = 0;

    if (file == NULL)
    {
        printf("Open %s fail!\n", fileName);
        return -1;
    }

    while ((s = readLine(file, s, &n)) != NULL)
    {
        lines[i++] = s;
        if (i >= max_line)
            break;
    }
    fclose(file);
    return i;
}

static int loadLabelName(const char *locationFilename, char *label[])
{
    printf("load lable %s\n", locationFilename);
    readLines(locationFilename, label, OBJ_CLASS_NUM);
    return 0;
}

static float sigmoid(float x) { return 1.0 / (1.0 + expf(-x)); }

static float unsigmoid(float y) { return -1.0 * logf((1.0 / y) - 1.0); }

inline static int32_t __clip(float val, float min, float max)
{
    float f = val <= min ? min : (val >= max ? max : val);
    return f;
}

static int8_t qnt_f32_to_affine(float f32, int32_t zp, float scale)
{
    float dst_val = (f32 / scale) + zp;
    int8_t res = (int8_t)__clip(dst_val, -128, 127);
    return res;
}

static uint8_t qnt_f32_to_affine_u8(float f32, int32_t zp, float scale)
{
    float dst_val = (f32 / scale) + zp;
    uint8_t res = (uint8_t)__clip(dst_val, 0, 255);
    return res;
}

// Dequantize INT8 to FP32
static float deqnt_affine_to_f32(int8_t qnt, int32_t zp, float scale)
{
    return ((float)qnt - (float)zp) * scale;
}

static float deqnt_affine_u8_to_f32(uint8_t qnt, int32_t zp, float scale)
{
    return ((float)qnt - (float)zp) * scale;
}

/**
 * @brief YOLO26 post-process (one2one head, no NMS needed)
 * 
 * YOLO26 truncated model outputs:
 * - scores: (1, 80, 8400) - class scores after sigmoid
 * - boxes: (1, 4, 8400) - bounding box coordinates (x1, y1, x2, y2)
 */
int post_process(rknn_app_context_t *app_ctx, void *outputs, letterbox_t *letter_box,
                 float conf_threshold, object_detect_result_list *od_results)
{
#if defined(RV1106_1103)
    rknn_tensor_mem **_outputs = (rknn_tensor_mem **)outputs;
    int model_in_w = app_ctx->model_width;
    int model_in_h = app_ctx->model_height;

    memset(od_results, 0, sizeof(object_detect_result_list));

    // Find scores and boxes tensors by shape
    int8_t *scores_i8 = nullptr;
    int8_t *boxes_i8 = nullptr;
    int scores_idx = -1;
    int boxes_idx = -1;
    int scores_zp = 0, boxes_zp = 0;
    float scores_scale = 1.0f, boxes_scale = 1.0f;

    for (int i = 0; i < app_ctx->io_num.n_output; i++)
    {
        rknn_tensor_attr *attr = &app_ctx->output_attrs[i];
        // scores shape: (1, 80, 8400) or (1, 8400, 80) depending on format
        if (attr->n_dims == 3 && ((attr->dims[1] == OBJ_CLASS_NUM) || (attr->dims[2] == OBJ_CLASS_NUM)))
        {
            scores_idx = i;
            scores_zp = attr->zp;
            scores_scale = attr->scale;
        }
        // boxes shape: (1, 4, 8400) or (1, 8400, 4)
        else if (attr->n_dims == 3 && ((attr->dims[1] == 4) || (attr->dims[2] == 4)))
        {
            boxes_idx = i;
            boxes_zp = attr->zp;
            boxes_scale = attr->scale;
        }
    }

    if (scores_idx < 0 || boxes_idx < 0)
    {
        printf("Error: Could not find scores or boxes tensors!\n");
        printf("Expected: scores (1, 80, 8400) or (1, 8400, 80) and boxes (1, 4, 8400) or (1, 8400, 4)\n");
        for (int i = 0; i < app_ctx->io_num.n_output; i++)
        {
            rknn_tensor_attr *attr = &app_ctx->output_attrs[i];
            printf("  output[%d]: dims=[%d, %d, %d, %d]\n", i,
                   attr->dims[0], attr->dims[1], attr->dims[2], attr->dims[3]);
        }
        return -1;
    }

    scores_i8 = (int8_t *)_outputs[scores_idx]->virt_addr;
    boxes_i8 = (int8_t *)_outputs[boxes_idx]->virt_addr;

    if (scores_i8 == nullptr || boxes_i8 == nullptr)
    {
        printf("Error: Output buffer is NULL!\n");
        return -1;
    }

    // Check data format: NCHW vs NHWC for RV1106
    bool is_nchw = (app_ctx->output_attrs[scores_idx].dims[1] == OBJ_CLASS_NUM);

    std::vector<std::pair<float, int>> candidates; // (score, index)

    int8_t score_thres_i8 = qnt_f32_to_affine(conf_threshold, scores_zp, scores_scale);

    // Iterate through all anchors
    for (int i = 0; i < PROP_BOX_SIZE; i++)
    {
        float max_score = -1.0f;
        int cls_id = -1;

        // Find max class score for this anchor
        if (is_nchw)
        {
            // NCHW format: (1, 80, 8400)
            for (int j = 0; j < OBJ_CLASS_NUM; j++)
            {
                int8_t score_i8 = scores_i8[j * PROP_BOX_SIZE + i];
                float score = deqnt_affine_to_f32(score_i8, scores_zp, scores_scale);
                if (score > max_score)
                {
                    max_score = score;
                    cls_id = j;
                }
            }
        }
        else
        {
            // NHWC format: (1, 8400, 80)
            for (int j = 0; j < OBJ_CLASS_NUM; j++)
            {
                int8_t score_i8 = scores_i8[i * OBJ_CLASS_NUM + j];
                float score = deqnt_affine_to_f32(score_i8, scores_zp, scores_scale);
                if (score > max_score)
                {
                    max_score = score;
                    cls_id = j;
                }
            }
        }

        // Threshold filter
        if (max_score > conf_threshold)
        {
            candidates.push_back({max_score, i});
        }
    }

    // Sort by score and take TopK
    std::sort(candidates.begin(), candidates.end(),
              [](const std::pair<float, int> &a, const std::pair<float, int> &b)
              {
                  return a.first > b.first;
              });

    int count = std::min((int)candidates.size(), OBJ_NUMB_MAX_SIZE);
    od_results->count = count;

    for (int k = 0; k < count; k++)
    {
        int i = candidates[k].second;
        float max_score = candidates[k].first;

        // Find class id again
        int cls_id = -1;
        float score_check = -1.0f;
        if (is_nchw)
        {
            for (int j = 0; j < OBJ_CLASS_NUM; j++)
            {
                int8_t score_i8 = scores_i8[j * PROP_BOX_SIZE + i];
                float score = deqnt_affine_to_f32(score_i8, scores_zp, scores_scale);
                if (score > score_check)
                {
                    score_check = score;
                    cls_id = j;
                }
            }
        }
        else
        {
            for (int j = 0; j < OBJ_CLASS_NUM; j++)
            {
                int8_t score_i8 = scores_i8[i * OBJ_CLASS_NUM + j];
                float score = deqnt_affine_to_f32(score_i8, scores_zp, scores_scale);
                if (score > score_check)
                {
                    score_check = score;
                    cls_id = j;
                }
            }
        }

        // Get box coordinates (x1, y1, x2, y2) and dequantize
        float x1, y1, x2, y2;
        if (is_nchw)
        {
            x1 = deqnt_affine_to_f32(boxes_i8[0 * PROP_BOX_SIZE + i], boxes_zp, boxes_scale);
            y1 = deqnt_affine_to_f32(boxes_i8[1 * PROP_BOX_SIZE + i], boxes_zp, boxes_scale);
            x2 = deqnt_affine_to_f32(boxes_i8[2 * PROP_BOX_SIZE + i], boxes_zp, boxes_scale);
            y2 = deqnt_affine_to_f32(boxes_i8[3 * PROP_BOX_SIZE + i], boxes_zp, boxes_scale);
        }
        else
        {
            x1 = deqnt_affine_to_f32(boxes_i8[i * 4 + 0], boxes_zp, boxes_scale);
            y1 = deqnt_affine_to_f32(boxes_i8[i * 4 + 1], boxes_zp, boxes_scale);
            x2 = deqnt_affine_to_f32(boxes_i8[i * 4 + 2], boxes_zp, boxes_scale);
            y2 = deqnt_affine_to_f32(boxes_i8[i * 4 + 3], boxes_zp, boxes_scale);
        }

        x1 -= letter_box->x_pad;
        y1 -= letter_box->y_pad;
        x2 -= letter_box->x_pad;
        y2 -= letter_box->y_pad;

        // Transform to original image coordinates
        od_results->results[k].box.left = (int)(clamp(x1, 0, model_in_w) / letter_box->scale);
        od_results->results[k].box.top = (int)(clamp(y1, 0, model_in_h) / letter_box->scale);
        od_results->results[k].box.right = (int)(clamp(x2, 0, model_in_w) / letter_box->scale);
        od_results->results[k].box.bottom = (int)(clamp(y2, 0, model_in_h) / letter_box->scale);
        od_results->results[k].prop = max_score;
        od_results->results[k].cls_id = cls_id;
    }

    return 0;
#else
    rknn_output *_outputs = (rknn_output *)outputs;
    int model_in_w = app_ctx->model_width;
    int model_in_h = app_ctx->model_height;

    memset(od_results, 0, sizeof(object_detect_result_list));

    // Find scores and boxes tensors by shape
    float *scores_fp32 = nullptr;
    float *boxes_fp32 = nullptr;
    int scores_idx = -1;
    int boxes_idx = -1;

    for (int i = 0; i < app_ctx->io_num.n_output; i++)
    {
        rknn_tensor_attr *attr = &app_ctx->output_attrs[i];
        // scores shape: (1, 80, 8400) - NCHW format, dims[1] = 80
        if (attr->dims[1] == OBJ_CLASS_NUM)
        {
            scores_idx = i;
        }
        // boxes shape: (1, 4, 8400) - NCHW format, dims[1] = 4
        else if (attr->dims[1] == 4)
        {
            boxes_idx = i;
        }
    }

    if (scores_idx < 0 || boxes_idx < 0)
    {
        printf("Error: Could not find scores or boxes tensors!\n");
        printf("Expected: scores (1, 80, 8400) and boxes (1, 4, 8400)\n");
        for (int i = 0; i < app_ctx->io_num.n_output; i++)
        {
            rknn_tensor_attr *attr = &app_ctx->output_attrs[i];
            printf("  output[%d]: dims=[%d, %d, %d, %d]\n", i,
                   attr->dims[0], attr->dims[1], attr->dims[2], attr->dims[3]);
        }
        return -1;
    }

    // Get float data (want_float=true was set in inference)
    scores_fp32 = (float *)_outputs[scores_idx].buf;
    boxes_fp32 = (float *)_outputs[boxes_idx].buf;

    if (scores_fp32 == nullptr || boxes_fp32 == nullptr)
    {
        printf("Error: Output buffer is NULL!\n");
        return -1;
    }

    // scores shape: (1, 80, 8400), boxes shape: (1, 4, 8400)
    std::vector<std::pair<float, int>> candidates; // (score, index)

    // Iterate through all anchors
    for (int i = 0; i < PROP_BOX_SIZE; i++)
    {
        float max_score = -1.0f;
        int cls_id = -1;

        // Find max class score for this anchor
        for (int j = 0; j < OBJ_CLASS_NUM; j++)
        {
            float score = scores_fp32[j * PROP_BOX_SIZE + i];
            if (score > max_score)
            {
                max_score = score;
                cls_id = j;
            }
        }

        // Threshold filter
        if (max_score > conf_threshold)
        {
            candidates.push_back({max_score, i});
        }
    }

    // Sort by score and take TopK
    std::sort(candidates.begin(), candidates.end(),
              [](const std::pair<float, int> &a, const std::pair<float, int> &b)
              {
                  return a.first > b.first;
              });

    int count = std::min((int)candidates.size(), OBJ_NUMB_MAX_SIZE);
    od_results->count = count;

    for (int k = 0; k < count; k++)
    {
        int i = candidates[k].second;
        float max_score = candidates[k].first;

        // Find class id again
        int cls_id = -1;
        float score_check = -1.0f;
        for (int j = 0; j < OBJ_CLASS_NUM; j++)
        {
            float score = scores_fp32[j * PROP_BOX_SIZE + i];
            if (score > score_check)
            {
                score_check = score;
                cls_id = j;
            }
        }

        // Get box coordinates (x1, y1, x2, y2)
        float x1 = boxes_fp32[0 * PROP_BOX_SIZE + i] - letter_box->x_pad;
        float y1 = boxes_fp32[1 * PROP_BOX_SIZE + i] - letter_box->y_pad;
        float x2 = boxes_fp32[2 * PROP_BOX_SIZE + i] - letter_box->x_pad;
        float y2 = boxes_fp32[3 * PROP_BOX_SIZE + i] - letter_box->y_pad;

        // Transform to original image coordinates
        od_results->results[k].box.left = (int)(clamp(x1, 0, model_in_w) / letter_box->scale);
        od_results->results[k].box.top = (int)(clamp(y1, 0, model_in_h) / letter_box->scale);
        od_results->results[k].box.right = (int)(clamp(x2, 0, model_in_w) / letter_box->scale);
        od_results->results[k].box.bottom = (int)(clamp(y2, 0, model_in_h) / letter_box->scale);
        od_results->results[k].prop = max_score;
        od_results->results[k].cls_id = cls_id;
    }

    return 0;
#endif
}

int init_post_process()
{
    int ret = 0;
    ret = loadLabelName(LABEL_NALE_TXT_PATH, labels);
    if (ret < 0)
    {
        printf("Load %s failed!\n", LABEL_NALE_TXT_PATH);
        return -1;
    }
    return 0;
}

char *coco_cls_to_name(int cls_id)
{
    if (cls_id >= OBJ_CLASS_NUM)
    {
        return (char *)"null";
    }

    if (labels[cls_id])
    {
        return labels[cls_id];
    }

    return (char *)"null";
}

void deinit_post_process()
{
    for (int i = 0; i < OBJ_CLASS_NUM; i++)
    {
        if (labels[i] != nullptr)
        {
            free(labels[i]);
            labels[i] = nullptr;
        }
    }
}
