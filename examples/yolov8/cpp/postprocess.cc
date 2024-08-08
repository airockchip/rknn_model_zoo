// Copyright (c) 2021 by Rockchip Electronics Co., Ltd. All Rights Reserved.
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

#include "yolov8.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <set>
#include <vector>
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

static float CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1,
                              float ymax1)
{
    float w = fmax(0.f, fmin(xmax0, xmax1) - fmax(xmin0, xmin1) + 1.0);
    float h = fmax(0.f, fmin(ymax0, ymax1) - fmax(ymin0, ymin1) + 1.0);
    float i = w * h;
    float u = (xmax0 - xmin0 + 1.0) * (ymax0 - ymin0 + 1.0) + (xmax1 - xmin1 + 1.0) * (ymax1 - ymin1 + 1.0) - i;
    return u <= 0.f ? 0.f : (i / u);
}

static int nms(int validCount, std::vector<float> &outputLocations, std::vector<int> classIds, std::vector<int> &order,
               int filterId, float threshold)
{
    for (int i = 0; i < validCount; ++i)
    {
        int n = order[i];
        if (n == -1 || classIds[n] != filterId)
        {
            continue;
        }
        for (int j = i + 1; j < validCount; ++j)
        {
            int m = order[j];
            if (m == -1 || classIds[m] != filterId)
            {
                continue;
            }
            float xmin0 = outputLocations[n * 4 + 0];
            float ymin0 = outputLocations[n * 4 + 1];
            float xmax0 = outputLocations[n * 4 + 0] + outputLocations[n * 4 + 2];
            float ymax0 = outputLocations[n * 4 + 1] + outputLocations[n * 4 + 3];

            float xmin1 = outputLocations[m * 4 + 0];
            float ymin1 = outputLocations[m * 4 + 1];
            float xmax1 = outputLocations[m * 4 + 0] + outputLocations[m * 4 + 2];
            float ymax1 = outputLocations[m * 4 + 1] + outputLocations[m * 4 + 3];

            float iou = CalculateOverlap(xmin0, ymin0, xmax0, ymax0, xmin1, ymin1, xmax1, ymax1);

            if (iou > threshold)
            {
                order[j] = -1;
            }
        }
    }
    return 0;
}

static int quick_sort_indice_inverse(std::vector<float> &input, int left, int right, std::vector<int> &indices)
{
    float key;
    int key_index;
    int low = left;
    int high = right;
    if (left < right)
    {
        key_index = indices[left];
        key = input[left];
        while (low < high)
        {
            while (low < high && input[high] <= key)
            {
                high--;
            }
            input[low] = input[high];
            indices[low] = indices[high];
            while (low < high && input[low] >= key)
            {
                low++;
            }
            input[high] = input[low];
            indices[high] = indices[low];
        }
        input[low] = key;
        indices[low] = key_index;
        quick_sort_indice_inverse(input, left, low - 1, indices);
        quick_sort_indice_inverse(input, low + 1, right, indices);
    }
    return low;
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

static float deqnt_affine_to_f32(int8_t qnt, int32_t zp, float scale) { return ((float)qnt - (float)zp) * scale; }

static float deqnt_affine_u8_to_f32(uint8_t qnt, int32_t zp, float scale) { return ((float)qnt - (float)zp) * scale; }

static void compute_dfl(float* tensor, int dfl_len, float* box){
    for (int b=0; b<4; b++){
        float exp_t[dfl_len];
        float exp_sum=0;
        float acc_sum=0;
        for (int i=0; i< dfl_len; i++){
            exp_t[i] = exp(tensor[i+b*dfl_len]);
            exp_sum += exp_t[i];
        }
        
        for (int i=0; i< dfl_len; i++){
            acc_sum += exp_t[i]/exp_sum *i;
        }
        box[b] = acc_sum;
    }
}

static int process_u8(uint8_t *box_tensor, int32_t box_zp, float box_scale,
                      uint8_t *score_tensor, int32_t score_zp, float score_scale,
                      uint8_t *score_sum_tensor, int32_t score_sum_zp, float score_sum_scale,
                      int grid_h, int grid_w, int stride, int dfl_len,
                      std::vector<float> &boxes,
                      std::vector<float> &objProbs,
                      std::vector<int> &classId,
                      float threshold)
{
    int validCount = 0;
    int grid_len = grid_h * grid_w;
    uint8_t score_thres_u8 = qnt_f32_to_affine_u8(threshold, score_zp, score_scale);
    uint8_t score_sum_thres_u8 = qnt_f32_to_affine_u8(threshold, score_sum_zp, score_sum_scale);

    for (int i = 0; i < grid_h; i++)
    {
        for (int j = 0; j < grid_w; j++)
        {
            int offset = i * grid_w + j;
            int max_class_id = -1;

            // Use score sum to quickly filter
            if (score_sum_tensor != nullptr)
            {
                if (score_sum_tensor[offset] < score_sum_thres_u8)
                {
                    continue;
                }
            }

            uint8_t max_score = -score_zp;
            for (int c = 0; c < OBJ_CLASS_NUM; c++)
            {
                if ((score_tensor[offset] > score_thres_u8) && (score_tensor[offset] > max_score))
                {
                    max_score = score_tensor[offset];
                    max_class_id = c;
                }
                offset += grid_len;
            }

            // compute box
            if (max_score > score_thres_u8)
            {
                offset = i * grid_w + j;
                float box[4];
                float before_dfl[dfl_len * 4];
                for (int k = 0; k < dfl_len * 4; k++)
                {
                    before_dfl[k] = deqnt_affine_u8_to_f32(box_tensor[offset], box_zp, box_scale);
                    offset += grid_len;
                }
                compute_dfl(before_dfl, dfl_len, box);

                float x1, y1, x2, y2, w, h;
                x1 = (-box[0] + j + 0.5) * stride;
                y1 = (-box[1] + i + 0.5) * stride;
                x2 = (box[2] + j + 0.5) * stride;
                y2 = (box[3] + i + 0.5) * stride;
                w = x2 - x1;
                h = y2 - y1;
                boxes.push_back(x1);
                boxes.push_back(y1);
                boxes.push_back(w);
                boxes.push_back(h);

                objProbs.push_back(deqnt_affine_u8_to_f32(max_score, score_zp, score_scale));
                classId.push_back(max_class_id);
                validCount++;
            }
        }
    }
    return validCount;
}

static int process_i8(int8_t *box_tensor, int32_t box_zp, float box_scale,
                      int8_t *score_tensor, int32_t score_zp, float score_scale,
                      int8_t *score_sum_tensor, int32_t score_sum_zp, float score_sum_scale,
                      int grid_h, int grid_w, int stride, int dfl_len,
                      std::vector<float> &boxes, 
                      std::vector<float> &objProbs, 
                      std::vector<int> &classId, 
                      float threshold)
{
    int validCount = 0;
    int grid_len = grid_h * grid_w;
    int8_t score_thres_i8 = qnt_f32_to_affine(threshold, score_zp, score_scale);
    int8_t score_sum_thres_i8 = qnt_f32_to_affine(threshold, score_sum_zp, score_sum_scale);

    for (int i = 0; i < grid_h; i++)
    {
        for (int j = 0; j < grid_w; j++)
        {
            int offset = i* grid_w + j;
            int max_class_id = -1;

            // 通过 score sum 起到快速过滤的作用
            if (score_sum_tensor != nullptr){
                if (score_sum_tensor[offset] < score_sum_thres_i8){
                    continue;
                }
            }

            int8_t max_score = -score_zp;
            for (int c= 0; c< OBJ_CLASS_NUM; c++){
                if ((score_tensor[offset] > score_thres_i8) && (score_tensor[offset] > max_score))
                {
                    max_score = score_tensor[offset];
                    max_class_id = c;
                }
                offset += grid_len;
            }

            // compute box
            if (max_score> score_thres_i8){
                offset = i* grid_w + j;
                float box[4];
                float before_dfl[dfl_len*4];
                for (int k=0; k< dfl_len*4; k++){
                    before_dfl[k] = deqnt_affine_to_f32(box_tensor[offset], box_zp, box_scale);
                    offset += grid_len;
                }
                compute_dfl(before_dfl, dfl_len, box);

                float x1,y1,x2,y2,w,h;
                x1 = (-box[0] + j + 0.5)*stride;
                y1 = (-box[1] + i + 0.5)*stride;
                x2 = (box[2] + j + 0.5)*stride;
                y2 = (box[3] + i + 0.5)*stride;
                w = x2 - x1;
                h = y2 - y1;
                boxes.push_back(x1);
                boxes.push_back(y1);
                boxes.push_back(w);
                boxes.push_back(h);

                objProbs.push_back(deqnt_affine_to_f32(max_score, score_zp, score_scale));
                classId.push_back(max_class_id);
                validCount ++;
            }
        }
    }
    return validCount;
}

static int process_fp32(float *box_tensor, float *score_tensor, float *score_sum_tensor, 
                        int grid_h, int grid_w, int stride, int dfl_len,
                        std::vector<float> &boxes, 
                        std::vector<float> &objProbs, 
                        std::vector<int> &classId, 
                        float threshold)
{
    int validCount = 0;
    int grid_len = grid_h * grid_w;
    for (int i = 0; i < grid_h; i++)
    {
        for (int j = 0; j < grid_w; j++)
        {
            int offset = i* grid_w + j;
            int max_class_id = -1;

            // 通过 score sum 起到快速过滤的作用
            if (score_sum_tensor != nullptr){
                if (score_sum_tensor[offset] < threshold){
                    continue;
                }
            }

            float max_score = 0;
            for (int c= 0; c< OBJ_CLASS_NUM; c++){
                if ((score_tensor[offset] > threshold) && (score_tensor[offset] > max_score))
                {
                    max_score = score_tensor[offset];
                    max_class_id = c;
                }
                offset += grid_len;
            }

            // compute box
            if (max_score> threshold){
                offset = i* grid_w + j;
                float box[4];
                float before_dfl[dfl_len*4];
                for (int k=0; k< dfl_len*4; k++){
                    before_dfl[k] = box_tensor[offset];
                    offset += grid_len;
                }
                compute_dfl(before_dfl, dfl_len, box);

                float x1,y1,x2,y2,w,h;
                x1 = (-box[0] + j + 0.5)*stride;
                y1 = (-box[1] + i + 0.5)*stride;
                x2 = (box[2] + j + 0.5)*stride;
                y2 = (box[3] + i + 0.5)*stride;
                w = x2 - x1;
                h = y2 - y1;
                boxes.push_back(x1);
                boxes.push_back(y1);
                boxes.push_back(w);
                boxes.push_back(h);

                objProbs.push_back(max_score);
                classId.push_back(max_class_id);
                validCount ++;
            }
        }
    }
    return validCount;
}


#if defined(RV1106_1103)
static int process_i8_rv1106(int8_t *box_tensor, int32_t box_zp, float box_scale,
                             int8_t *score_tensor, int32_t score_zp, float score_scale,
                             int8_t *score_sum_tensor, int32_t score_sum_zp, float score_sum_scale,
                             int grid_h, int grid_w, int stride, int dfl_len,
                             std::vector<float> &boxes,
                             std::vector<float> &objProbs,
                             std::vector<int> &classId,
                             float threshold) {
    int validCount = 0;
    int grid_len = grid_h * grid_w;
    int8_t score_thres_i8 = qnt_f32_to_affine(threshold, score_zp, score_scale);
    int8_t score_sum_thres_i8 = qnt_f32_to_affine(threshold, score_sum_zp, score_sum_scale);

    for (int i = 0; i < grid_h; i++) {
        for (int j = 0; j < grid_w; j++) {
            int offset = i * grid_w + j;
            int max_class_id = -1;

            // 通过 score sum 起到快速过滤的作用
            if (score_sum_tensor != nullptr) {
                //score_sum_tensor [1, 1, 80, 80]
                if (score_sum_tensor[offset] < score_sum_thres_i8) {
                    continue;
                }
            }

            int8_t max_score = -score_zp;
            offset = offset * OBJ_CLASS_NUM;
            for (int c = 0; c < OBJ_CLASS_NUM; c++) {
                if ((score_tensor[offset + c] > score_thres_i8) && (score_tensor[offset + c] > max_score)) {
                    max_score = score_tensor[offset + c]; //80类 [1, 80, 80, 80] 3588NCHW 1106NHWC
                    max_class_id = c;
                }
            }

            // compute box
            if (max_score > score_thres_i8) {
                offset = (i * grid_w + j) * 4 * dfl_len;
                float box[4];
                float before_dfl[dfl_len*4];
                for (int k=0; k< dfl_len*4; k++){
                    before_dfl[k] = deqnt_affine_to_f32(box_tensor[offset + k], box_zp, box_scale);
                }
                compute_dfl(before_dfl, dfl_len, box);

                float x1, y1, x2, y2, w, h;
                x1 = (-box[0] + j + 0.5) * stride;
                y1 = (-box[1] + i + 0.5) * stride;
                x2 = (box[2] + j + 0.5) * stride;
                y2 = (box[3] + i + 0.5) * stride;
                w = x2 - x1;
                h = y2 - y1;
                boxes.push_back(x1);
                boxes.push_back(y1);
                boxes.push_back(w);
                boxes.push_back(h);

                objProbs.push_back(deqnt_affine_to_f32(max_score, score_zp, score_scale));
                classId.push_back(max_class_id);
                validCount ++;
            }
        }
    }
    printf("validCount=%d\n", validCount);
    printf("grid h-%d, w-%d, stride %d\n", grid_h, grid_w, stride);
    return validCount;
}
#endif

int post_process(rknn_app_context_t *app_ctx, void *outputs, letterbox_t *letter_box, float conf_threshold, float nms_threshold, object_detect_result_list *od_results)
{
#if defined(RV1106_1103) 
    rknn_tensor_mem **_outputs = (rknn_tensor_mem **)outputs;
#else
    rknn_output *_outputs = (rknn_output *)outputs;
#endif
    std::vector<float> filterBoxes;
    std::vector<float> objProbs;
    std::vector<int> classId;
    int validCount = 0;
    int stride = 0;
    int grid_h = 0;
    int grid_w = 0;
    int model_in_w = app_ctx->model_width;
    int model_in_h = app_ctx->model_height;

    memset(od_results, 0, sizeof(object_detect_result_list));

    // default 3 branch
#ifdef RKNPU1
    int dfl_len = app_ctx->output_attrs[0].dims[2] / 4;
#else
    int dfl_len = app_ctx->output_attrs[0].dims[1] /4;
#endif
    int output_per_branch = app_ctx->io_num.n_output / 3;
    for (int i = 0; i < 3; i++)
    {
#if defined(RV1106_1103)
        dfl_len = app_ctx->output_attrs[0].dims[3] /4;
        void *score_sum = nullptr;
        int32_t score_sum_zp = 0;
        float score_sum_scale = 1.0;
        if (output_per_branch == 3) {
            score_sum = _outputs[i * output_per_branch + 2]->virt_addr;
            score_sum_zp = app_ctx->output_attrs[i * output_per_branch + 2].zp;
            score_sum_scale = app_ctx->output_attrs[i * output_per_branch + 2].scale;
        }
        int box_idx = i * output_per_branch;
        int score_idx = i * output_per_branch + 1;
        grid_h = app_ctx->output_attrs[box_idx].dims[1];
        grid_w = app_ctx->output_attrs[box_idx].dims[2];
        stride = model_in_h / grid_h;
        
        if (app_ctx->is_quant) {
            validCount += process_i8_rv1106((int8_t *)_outputs[box_idx]->virt_addr, app_ctx->output_attrs[box_idx].zp, app_ctx->output_attrs[box_idx].scale,
                                (int8_t *)_outputs[score_idx]->virt_addr, app_ctx->output_attrs[score_idx].zp,
                                app_ctx->output_attrs[score_idx].scale, (int8_t *)score_sum, score_sum_zp, score_sum_scale,
                                grid_h, grid_w, stride, dfl_len, filterBoxes, objProbs, classId, conf_threshold);
        }
        else
        {
            printf("RV1106/1103 only support quantization mode\n", LABEL_NALE_TXT_PATH);
            return -1;
        }

#else
        void *score_sum = nullptr;
        int32_t score_sum_zp = 0;
        float score_sum_scale = 1.0;
        if (output_per_branch == 3){
            score_sum = _outputs[i*output_per_branch + 2].buf;
            score_sum_zp = app_ctx->output_attrs[i*output_per_branch + 2].zp;
            score_sum_scale = app_ctx->output_attrs[i*output_per_branch + 2].scale;
        }
        int box_idx = i*output_per_branch;
        int score_idx = i*output_per_branch + 1;

#ifdef RKNPU1
        grid_h = app_ctx->output_attrs[box_idx].dims[1];
        grid_w = app_ctx->output_attrs[box_idx].dims[0];
#else
        grid_h = app_ctx->output_attrs[box_idx].dims[2];
        grid_w = app_ctx->output_attrs[box_idx].dims[3];
#endif
        stride = model_in_h / grid_h;

        if (app_ctx->is_quant)
        {
#ifdef RKNPU1
            validCount += process_u8((uint8_t *)_outputs[box_idx].buf, app_ctx->output_attrs[box_idx].zp, app_ctx->output_attrs[box_idx].scale,
                                     (uint8_t *)_outputs[score_idx].buf, app_ctx->output_attrs[score_idx].zp, app_ctx->output_attrs[score_idx].scale,
                                     (uint8_t *)score_sum, score_sum_zp, score_sum_scale,
                                     grid_h, grid_w, stride, dfl_len,
                                     filterBoxes, objProbs, classId, conf_threshold);
#else
            validCount += process_i8((int8_t *)_outputs[box_idx].buf, app_ctx->output_attrs[box_idx].zp, app_ctx->output_attrs[box_idx].scale,
                                     (int8_t *)_outputs[score_idx].buf, app_ctx->output_attrs[score_idx].zp, app_ctx->output_attrs[score_idx].scale,
                                     (int8_t *)score_sum, score_sum_zp, score_sum_scale,
                                     grid_h, grid_w, stride, dfl_len, 
                                     filterBoxes, objProbs, classId, conf_threshold);
#endif
        }
        else
        {
            validCount += process_fp32((float *)_outputs[box_idx].buf, (float *)_outputs[score_idx].buf, (float *)score_sum,
                                       grid_h, grid_w, stride, dfl_len, 
                                       filterBoxes, objProbs, classId, conf_threshold);
        }
#endif
    }

    // no object detect
    if (validCount <= 0)
    {
        return 0;
    }
    std::vector<int> indexArray;
    for (int i = 0; i < validCount; ++i)
    {
        indexArray.push_back(i);
    }
    quick_sort_indice_inverse(objProbs, 0, validCount - 1, indexArray);

    std::set<int> class_set(std::begin(classId), std::end(classId));

    for (auto c : class_set)
    {
        nms(validCount, filterBoxes, classId, indexArray, c, nms_threshold);
    }

    int last_count = 0;
    od_results->count = 0;

    /* box valid detect target */
    for (int i = 0; i < validCount; ++i)
    {
        if (indexArray[i] == -1 || last_count >= OBJ_NUMB_MAX_SIZE)
        {
            continue;
        }
        int n = indexArray[i];

        float x1 = filterBoxes[n * 4 + 0] - letter_box->x_pad;
        float y1 = filterBoxes[n * 4 + 1] - letter_box->y_pad;
        float x2 = x1 + filterBoxes[n * 4 + 2];
        float y2 = y1 + filterBoxes[n * 4 + 3];
        int id = classId[n];
        float obj_conf = objProbs[i];

        od_results->results[last_count].box.left = (int)(clamp(x1, 0, model_in_w) / letter_box->scale);
        od_results->results[last_count].box.top = (int)(clamp(y1, 0, model_in_h) / letter_box->scale);
        od_results->results[last_count].box.right = (int)(clamp(x2, 0, model_in_w) / letter_box->scale);
        od_results->results[last_count].box.bottom = (int)(clamp(y2, 0, model_in_h) / letter_box->scale);
        od_results->results[last_count].prop = obj_conf;
        od_results->results[last_count].cls_id = id;
        last_count++;
    }
    od_results->count = last_count;
    return 0;
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
        return "null";
    }

    if (labels[cls_id])
    {
        return labels[cls_id];
    }

    return "null";
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
