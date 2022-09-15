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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <vector>
#include "postprocess.h"
#include <stdint.h>
#define LABEL_NALE_TXT_PATH "./model/coco_80_labels_list.txt"

static char *labels[OBJ_CLASS_NUM];

const int anchor0[6] = {10, 13, 16, 30, 33, 23};
const int anchor1[6] = {30, 61, 62, 45, 59, 119};
const int anchor2[6] = {116, 90, 156, 198, 373, 326};

inline static int clamp(float val, int min, int max)
{
    return val > min ? (val < max ? val : max) : min;
}

char *readLine(FILE *fp, char *buffer, int *len)
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

int readLines(const char *fileName, char *lines[], int max_line)
{
    FILE *file = fopen(fileName, "r");
    char *s;
    int i = 0;
    int n = 0;
    while ((s = readLine(file, s, &n)) != NULL)
    {
        lines[i++] = s;
        if (i >= max_line)
            break;
    }
    return i;
}

int loadLabelName(const char *locationFilename, char *label[])
{
    printf("loadLabelName %s\n", locationFilename);
    readLines(locationFilename, label, OBJ_CLASS_NUM);
    return 0;
}

static float CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1, float ymax1)
{
    float w = fmax(0.f, fmin(xmax0, xmax1) - fmax(xmin0, xmin1) + 1.0);
    float h = fmax(0.f, fmin(ymax0, ymax1) - fmax(ymin0, ymin1) + 1.0);
    float i = w * h;
    float u = (xmax0 - xmin0 + 1.0) * (ymax0 - ymin0 + 1.0) + (xmax1 - xmin1 + 1.0) * (ymax1 - ymin1 + 1.0) - i;
    return u <= 0.f ? 0.f : (i / u);
}

static int nms(int validCount, std::vector<float> &outputLocations, std::vector<int> &order, float threshold)
{
    for (int i = 0; i < validCount; ++i)
    {
        if (order[i] == -1)
        {
            continue;
        }
        int n = order[i];
        for (int j = i + 1; j < validCount; ++j)
        {
            int m = order[j];
            if (m == -1)
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

static int quick_sort_indice_inverse(
    std::vector<float> &input,
    int left,
    int right,
    std::vector<int> &indices)
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

static float sigmoid(float x)
{
    return 1.0 / (1.0 + expf(-x));
}

static float unsigmoid(float y)
{
    return -1.0 * logf((1.0 / y) - 1.0);
}

inline static int32_t __clip(float val, float min, float max)
{
    float f = val <= min ? min : (val >= max ? max : val);
    return f;
}

static uint8_t qnt_f32_to_affine(float f32, uint8_t zp, float scale)
{
    float dst_val = (f32 / scale) + zp;
    uint8_t res = (uint8_t)__clip(dst_val, 0, 255);
    return res;
}

static float deqnt_affine_to_f32(uint8_t qnt, uint8_t zp, float scale)
{
    return ((float)qnt - (float)zp) * scale;
}

static int process_u8(uint8_t *input, int *anchor, int grid_h, int grid_w, int height, int width, int stride,
                   std::vector<float> &boxes, std::vector<float> &boxScores, std::vector<int> &classId,
                   float threshold, uint8_t zp, float scale)
{

    int validCount = 0;
    int grid_len = grid_h * grid_w;
    float thres = unsigmoid(threshold);
    uint8_t thres_u8 = qnt_f32_to_affine(thres, zp, scale);
    for (int a = 0; a < 3; a++)
    {
        for (int i = 0; i < grid_h; i++)
        {
            for (int j = 0; j < grid_w; j++)
            {
                uint8_t box_confidence = input[(PROP_BOX_SIZE * a + 4) * grid_len + i * grid_w + j];
                if (box_confidence >= thres_u8)
                {
                    int offset = (PROP_BOX_SIZE * a) * grid_len + i * grid_w + j;
                    uint8_t *in_ptr = input + offset;
                    float box_x = sigmoid(deqnt_affine_to_f32(*in_ptr, zp, scale)) * 2.0 - 0.5;
                    float box_y = sigmoid(deqnt_affine_to_f32(in_ptr[grid_len], zp, scale)) * 2.0 - 0.5;
                    float box_w = sigmoid(deqnt_affine_to_f32(in_ptr[2 * grid_len], zp, scale)) * 2.0;
                    float box_h = sigmoid(deqnt_affine_to_f32(in_ptr[3 * grid_len], zp, scale)) * 2.0;
                    box_x = (box_x + j) * (float)stride;
                    box_y = (box_y + i) * (float)stride;
                    box_w = box_w * box_w * (float)anchor[a * 2];
                    box_h = box_h * box_h * (float)anchor[a * 2 + 1];
                    box_x -= (box_w / 2.0);
                    box_y -= (box_h / 2.0);
                    boxes.push_back(box_x);
                    boxes.push_back(box_y);
                    boxes.push_back(box_w);
                    boxes.push_back(box_h);

                    uint8_t maxClassProbs = in_ptr[5 * grid_len];
                    int maxClassId = 0;
                    for (int k = 1; k < OBJ_CLASS_NUM; ++k)
                    {
                        uint8_t prob = in_ptr[(5 + k) * grid_len];
                        if (prob > maxClassProbs)
                        {
                            maxClassId = k;
                            maxClassProbs = prob;
                        }
                    }
                    float box_conf_f32 = sigmoid(deqnt_affine_to_f32(box_confidence, zp, scale));
                    float class_prob_f32 = sigmoid(deqnt_affine_to_f32(maxClassProbs, zp, scale));
                    boxScores.push_back(box_conf_f32* class_prob_f32);
                    classId.push_back(maxClassId);
                    validCount++;
                }
            }
        }
    }
    return validCount;
}

static int process_fp(float *input, int *anchor, int grid_h, int grid_w, int height, int width, int stride,
                   std::vector<float> &boxes, std::vector<float> &boxScores, std::vector<int> &classId,
                   float threshold)
{

    int validCount = 0;
    int grid_len = grid_h * grid_w;
    float thres_sigmoid = unsigmoid(threshold);
    for (int a = 0; a < 3; a++)
    {
        for (int i = 0; i < grid_h; i++)
        {
            for (int j = 0; j < grid_w; j++)
            {
                float box_confidence = input[(PROP_BOX_SIZE * a + 4) * grid_len + i * grid_w + j];
                if (box_confidence >= thres_sigmoid)
                {
                    int offset = (PROP_BOX_SIZE * a) * grid_len + i * grid_w + j;
                    float *in_ptr = input + offset;
                    float box_x = sigmoid(*in_ptr) * 2.0 - 0.5;
                    float box_y = sigmoid(in_ptr[grid_len]) * 2.0 - 0.5;
                    float box_w = sigmoid(in_ptr[2 * grid_len]) * 2.0;
                    float box_h = sigmoid(in_ptr[3 * grid_len]) * 2.0;
                    box_x = (box_x + j) * (float)stride;
                    box_y = (box_y + i) * (float)stride;
                    box_w = box_w * box_w * (float)anchor[a * 2];
                    box_h = box_h * box_h * (float)anchor[a * 2 + 1];
                    box_x -= (box_w / 2.0);
                    box_y -= (box_h / 2.0);
                    boxes.push_back(box_x);
                    boxes.push_back(box_y);
                    boxes.push_back(box_w);
                    boxes.push_back(box_h);

                    float maxClassProbs = in_ptr[5 * grid_len];
                    int maxClassId = 0;
                    for (int k = 1; k < OBJ_CLASS_NUM; ++k)
                    {
                        float prob = in_ptr[(5 + k) * grid_len];
                        if (prob > maxClassProbs)
                        {
                            maxClassId = k;
                            maxClassProbs = prob;
                        }
                    }
                    float box_conf_f32 = sigmoid(box_confidence);
                    float class_prob_f32 = sigmoid(maxClassProbs);
                    boxScores.push_back(box_conf_f32* class_prob_f32);
                    classId.push_back(maxClassId);
                    validCount++;
                }
            }
        }
    }
    return validCount;
}

int post_process_u8(uint8_t *input0, uint8_t *input1, uint8_t *input2, int model_in_h, int model_in_w,
                 int h_offset, int w_offset, float resize_scale, float conf_threshold, float nms_threshold,
                 std::vector<uint8_t> &qnt_zps, std::vector<float> &qnt_scales,
                 detect_result_group_t *group)
{
    static int init = -1;
    if (init == -1)
    {
        int ret = 0;
        ret = loadLabelName(LABEL_NALE_TXT_PATH, labels);
        if (ret < 0)
        {
            return -1;
        }

        init = 0;
    }
    memset(group, 0, sizeof(detect_result_group_t));

    std::vector<float> filterBoxes;
    std::vector<float> boxesScore;
    std::vector<int> classId;
    int stride0 = 8;
    int grid_h0 = model_in_h / stride0;
    int grid_w0 = model_in_w / stride0;
    int validCount0 = 0;
    validCount0 = process_u8(input0, (int *)anchor0, grid_h0, grid_w0, model_in_h, model_in_w,
                          stride0, filterBoxes, boxesScore, classId, conf_threshold, qnt_zps[0], qnt_scales[0]);

    int stride1 = 16;
    int grid_h1 = model_in_h / stride1;
    int grid_w1 = model_in_w / stride1;
    int validCount1 = 0;
    validCount1 = process_u8(input1, (int *)anchor1, grid_h1, grid_w1, model_in_h, model_in_w,
                          stride1, filterBoxes, boxesScore, classId, conf_threshold, qnt_zps[1], qnt_scales[1]);

    int stride2 = 32;
    int grid_h2 = model_in_h / stride2;
    int grid_w2 = model_in_w / stride2;
    int validCount2 = 0;
    validCount2 = process_u8(input2, (int *)anchor2, grid_h2, grid_w2, model_in_h, model_in_w,
                          stride2, filterBoxes, boxesScore, classId, conf_threshold, qnt_zps[2], qnt_scales[2]);

    int validCount = validCount0 + validCount1 + validCount2;
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

    quick_sort_indice_inverse(boxesScore, 0, validCount - 1, indexArray);

    nms(validCount, filterBoxes, indexArray, nms_threshold);

    int last_count = 0;
    group->count = 0;
    /* box valid detect target */
    for (int i = 0; i < validCount; ++i)
    {

        if (indexArray[i] == -1 || boxesScore[i] < conf_threshold || last_count >= OBJ_NUMB_MAX_SIZE)
        {
            continue;
        }
        int n = indexArray[i];

        float x1 = filterBoxes[n * 4 + 0];
        float y1 = filterBoxes[n * 4 + 1];
        float x2 = x1 + filterBoxes[n * 4 + 2];
        float y2 = y1 + filterBoxes[n * 4 + 3];
        int id = classId[n];

        group->results[last_count].box.left = (int)((clamp(x1, 0, model_in_w) - w_offset) / resize_scale);
        group->results[last_count].box.top = (int)((clamp(y1, 0, model_in_h) - h_offset) / resize_scale);
        group->results[last_count].box.right = (int)((clamp(x2, 0, model_in_w) - w_offset) / resize_scale);
        group->results[last_count].box.bottom = (int)((clamp(y2, 0, model_in_h)  - h_offset) / resize_scale);
        group->results[last_count].prop = boxesScore[i];
        group->results[last_count].class_index = id;
        char *label = labels[id];
        strncpy(group->results[last_count].name, label, OBJ_NAME_MAX_SIZE);

        // printf("result %2d: (%4d, %4d, %4d, %4d), %s\n", i, group->results[last_count].box.left, group->results[last_count].box.top,
        //        group->results[last_count].box.right, group->results[last_count].box.bottom, label);
        last_count++;
    }
    group->count = last_count;

    return 0;
}


int post_process_fp(float *input0, float *input1, float *input2, int model_in_h, int model_in_w,
                 int h_offset, int w_offset, float resize_scale, float conf_threshold, float nms_threshold,
                 detect_result_group_t *group)
{
    static int init = -1;
    if (init == -1)
    {
        int ret = 0;
        ret = loadLabelName(LABEL_NALE_TXT_PATH, labels);
        if (ret < 0)
        {
            return -1;
        }

        init = 0;
    }
    memset(group, 0, sizeof(detect_result_group_t));

    std::vector<float> filterBoxes;
    std::vector<float> boxesScore;
    std::vector<int> classId;
    int stride0 = 8;
    int grid_h0 = model_in_h / stride0;
    int grid_w0 = model_in_w / stride0;
    int validCount0 = 0;
    validCount0 = process_fp(input0, (int *)anchor0, grid_h0, grid_w0, model_in_h, model_in_w,
                          stride0, filterBoxes, boxesScore, classId, conf_threshold);

    int stride1 = 16;
    int grid_h1 = model_in_h / stride1;
    int grid_w1 = model_in_w / stride1;
    int validCount1 = 0;
    validCount1 = process_fp(input1, (int *)anchor1, grid_h1, grid_w1, model_in_h, model_in_w,
                          stride1, filterBoxes, boxesScore, classId, conf_threshold);

    int stride2 = 32;
    int grid_h2 = model_in_h / stride2;
    int grid_w2 = model_in_w / stride2;
    int validCount2 = 0;
    validCount2 = process_fp(input2, (int *)anchor2, grid_h2, grid_w2, model_in_h, model_in_w,
                          stride2, filterBoxes, boxesScore, classId, conf_threshold);

    int validCount = validCount0 + validCount1 + validCount2;
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

    quick_sort_indice_inverse(boxesScore, 0, validCount - 1, indexArray);

    nms(validCount, filterBoxes, indexArray, nms_threshold);

    int last_count = 0;
    group->count = 0;
    /* box valid detect target */
    for (int i = 0; i < validCount; ++i)
    {

        if (indexArray[i] == -1 || boxesScore[i] < conf_threshold || last_count >= OBJ_NUMB_MAX_SIZE)
        {
            continue;
        }
        int n = indexArray[i];

        float x1 = filterBoxes[n * 4 + 0];
        float y1 = filterBoxes[n * 4 + 1];
        float x2 = x1 + filterBoxes[n * 4 + 2];
        float y2 = y1 + filterBoxes[n * 4 + 3];
        int id = classId[n];

        group->results[last_count].box.left = (int)((clamp(x1, 0, model_in_w) - w_offset) / resize_scale);
        group->results[last_count].box.top = (int)((clamp(y1, 0, model_in_h) - h_offset) / resize_scale);
        group->results[last_count].box.right = (int)((clamp(x2, 0, model_in_w) - w_offset) / resize_scale);
        group->results[last_count].box.bottom = (int)((clamp(y2, 0, model_in_h)  - h_offset) / resize_scale);
        group->results[last_count].prop = boxesScore[i];
        group->results[last_count].class_index = id;
        char *label = labels[id];
        strncpy(group->results[last_count].name, label, OBJ_NAME_MAX_SIZE);

        // printf("result %2d: (%4d, %4d, %4d, %4d), %s\n", i, group->results[last_count].box.left, group->results[last_count].box.top,
        //        group->results[last_count].box.right, group->results[last_count].box.bottom, label);
        last_count++;
    }
    group->count = last_count;

    return 0;
}