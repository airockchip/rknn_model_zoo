// Copyright (c) 2024 by Rockchip Electronics Co., Ltd. All Rights Reserved.
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

#include "yolov8-pose.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#ifndef RKNPU1
#include <Float16.h>
#endif

#include <iostream>
#include <cmath>
#include <algorithm>

#include <set>
#include <vector>
#define LABEL_NALE_TXT_PATH "./model/yolov8_pose_labels_list.txt"

static char *labels[OBJ_CLASS_NUM];

inline static int clamp(float val, int min, int max) { return val > min ? (val < max ? val : max) : min; }

static char *readLine(FILE *fp, char *buffer, int *len) {
    int ch;
    int i = 0;
    size_t buff_len = 0;

    buffer = (char *)malloc(buff_len + 1);
    if (!buffer)
        return NULL; // Out of memory

    while ((ch = fgetc(fp)) != '\n' && ch != EOF) {
        buff_len++;
        void *tmp = realloc(buffer, buff_len + 1);
        if (tmp == NULL) {
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
    if (ch == EOF && (i == 0 || ferror(fp))) {
        free(buffer);
        return NULL;
    }
    return buffer;
}

static int readLines(const char *fileName, char *lines[], int max_line) {
    FILE *file = fopen(fileName, "r");
    char *s;
    int i = 0;
    int n = 0;

    if (file == NULL) {
        printf("Open %s fail!\n", fileName);
        return -1;
    }

    while ((s = readLine(file, s, &n)) != NULL) {
        lines[i++] = s;
        if (i >= max_line)
            break;
    }
    fclose(file);
    return i;
}

static int loadLabelName(const char *locationFilename, char *label[]) {
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
            float xmin0 = outputLocations[n * 5 + 0];
            float ymin0 = outputLocations[n * 5 + 1];
            float xmax0 = outputLocations[n * 5 + 0] + outputLocations[n * 5 + 2];
            float ymax0 = outputLocations[n * 5 + 1] + outputLocations[n * 5 + 3];

            float xmin1 = outputLocations[m * 5 + 0];
            float ymin1 = outputLocations[m * 5 + 1];
            float xmax1 = outputLocations[m * 5 + 0] + outputLocations[m * 5 + 2];
            float ymax1 = outputLocations[m * 5 + 1] + outputLocations[m * 5 + 3];

            float iou = CalculateOverlap(xmin0, ymin0, xmax0, ymax0, xmin1, ymin1, xmax1, ymax1);

            if (iou > threshold)
            {
                order[j] = -1;
            }
        }
    }
    return 0;
}

static int quick_sort_indice_inverse(std::vector<float> &input, int left, int right, std::vector<int> &indices) {
    float key;
    int key_index;
    int low = left;
    int high = right;
    if (left < right) {
        key_index = indices[left];
        key = input[left];
        while (low < high) {
            while (low < high && input[high] <= key) {
                high--;
            }
            input[low] = input[high];
            indices[low] = indices[high];
            while (low < high && input[low] >= key) {
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

static float sigmoid(float x) {
    return 1.0 / (1.0 + expf(-x));
}

static float unsigmoid(float y) {
    return -1.0 * logf((1.0 / y) - 1.0);
}

inline static int32_t __clip(float val, float min, float max) {
    float f = val <= min ? min : (val >= max ? max : val);
    return f;
}

static int8_t qnt_f32_to_affine(float f32, int32_t zp, float scale) {
    float dst_val = (f32 / scale) + zp;
    int8_t res = (int8_t)__clip(dst_val, -128, 127);
    return res;
}

static uint8_t qnt_f32_to_affine_u8(float f32, int32_t zp, float scale) {
    float dst_val = (f32 / scale) + zp;
    uint8_t res = (uint8_t)__clip(dst_val, 0, 255);
    return res;
}

static float deqnt_affine_to_f32(int8_t qnt, int32_t zp, float scale) {
    return ((float)qnt - (float)zp) * scale;
}
static float deqnt_affine_u8_to_f32(uint8_t qnt, int32_t zp, float scale) {
    return ((float)qnt - (float)zp) * scale;
}



void softmax(float *input, int size) {
    float max_val = input[0];
    for (int i = 1; i < size; ++i) {
        if (input[i] > max_val) {
            max_val = input[i];
        }
    }

    float sum_exp = 0.0;
    for (int i = 0; i < size; ++i) {
        sum_exp += expf(input[i] - max_val);
    }

    for (int i = 0; i < size; ++i) {
        input[i] = expf(input[i] - max_val) / sum_exp;
    }
}

static int process_i8(int8_t *input, int grid_h, int grid_w, int stride,
                      std::vector<float> &boxes, std::vector<float> &boxScores, std::vector<int> &classId, float threshold,
                      int32_t zp, float scale, int index) {
    int input_loc_len = 64;
    int tensor_len = input_loc_len + OBJ_CLASS_NUM;
    int validCount = 0;

    int8_t thres_i8 = qnt_f32_to_affine(unsigmoid(threshold), zp, scale);
    for (int h = 0; h < grid_h; h++) {
        for (int w = 0; w < grid_w; w++) {
            for (int a = 0; a < OBJ_CLASS_NUM; a++) {
                if(input[(input_loc_len + a)*grid_w * grid_h + h * grid_w + w ] >= thres_i8) { //[1,tensor_len,grid_h,grid_w]
                    float box_conf_f32 = sigmoid(deqnt_affine_to_f32(input[(input_loc_len + a) * grid_w * grid_h + h * grid_w + w ],
                                                 zp, scale));
                    float loc[input_loc_len];
                    for (int i = 0; i < input_loc_len; ++i) {
                        loc[i] = deqnt_affine_to_f32(input[i * grid_w * grid_h + h * grid_w + w], zp, scale);
                    }

                    for (int i = 0; i < input_loc_len / 16; ++i) {
                        softmax(&loc[i * 16], 16);
                    }
                    float xywh_[4] = {0, 0, 0, 0};
                    float xywh[4] = {0, 0, 0, 0};
                    for (int dfl = 0; dfl < 16; ++dfl) {
                        xywh_[0] += loc[dfl] * dfl;
                        xywh_[1] += loc[1 * 16 + dfl] * dfl;
                        xywh_[2] += loc[2 * 16 + dfl] * dfl;
                        xywh_[3] += loc[3 * 16 + dfl] * dfl;
                    }
                    xywh_[0]=(w+0.5)-xywh_[0];
                    xywh_[1]=(h+0.5)-xywh_[1];
                    xywh_[2]=(w+0.5)+xywh_[2];
                    xywh_[3]=(h+0.5)+xywh_[3];
                    xywh[0]=((xywh_[0]+xywh_[2])/2)*stride;
                    xywh[1]=((xywh_[1]+xywh_[3])/2)*stride;
                    xywh[2]=(xywh_[2]-xywh_[0])*stride;
                    xywh[3]=(xywh_[3]-xywh_[1])*stride;
                    xywh[0]=xywh[0]-xywh[2]/2;
                    xywh[1]=xywh[1]-xywh[3]/2;
                    boxes.push_back(xywh[0]);//x
                    boxes.push_back(xywh[1]);//y
                    boxes.push_back(xywh[2]);//w
                    boxes.push_back(xywh[3]);//h
                    boxes.push_back(float(index + (h * grid_w) + w));//keypoints index
                    boxScores.push_back(box_conf_f32);
                    classId.push_back(a);
                    validCount++;
                }
            }
        }
    }
    return validCount;
}


static int process_u8(uint8_t *input, int grid_h, int grid_w, int stride,
                      std::vector<float> &boxes, std::vector<float> &boxScores, std::vector<int> &classId, float threshold,
                      int32_t zp, float scale, int index) {
    int input_loc_len = 64;
    int tensor_len = input_loc_len + OBJ_CLASS_NUM;
    int validCount = 0;

    uint8_t thres_i8 = qnt_f32_to_affine_u8(unsigmoid(threshold), zp, scale);
    for (int h = 0; h < grid_h; h++) {
        for (int w = 0; w < grid_w; w++) {
            for (int a = 0; a < OBJ_CLASS_NUM; a++) {
                if(input[(input_loc_len + a)*grid_w * grid_h + h * grid_w + w ] >= thres_i8) { //[1,tensor_len,grid_h,grid_w]
                    float box_conf_f32 = sigmoid(deqnt_affine_u8_to_f32(input[(input_loc_len + a) * grid_w * grid_h + h * grid_w + w ],
                                                 zp, scale));
                    float loc[input_loc_len];
                    for (int i = 0; i < input_loc_len; ++i) {
                        loc[i] = deqnt_affine_u8_to_f32(input[i * grid_w * grid_h + h * grid_w + w], zp, scale);
                    }

                    for (int i = 0; i < input_loc_len / 16; ++i) {
                        softmax(&loc[i * 16], 16);
                    }
                    float xywh_[4] = {0, 0, 0, 0};
                    float xywh[4] = {0, 0, 0, 0};
                    for (int dfl = 0; dfl < 16; ++dfl) {
                        xywh_[0] += loc[dfl] * dfl;
                        xywh_[1] += loc[1 * 16 + dfl] * dfl;
                        xywh_[2] += loc[2 * 16 + dfl] * dfl;
                        xywh_[3] += loc[3 * 16 + dfl] * dfl;
                    }
                    xywh_[0]=(w+0.5)-xywh_[0];
                    xywh_[1]=(h+0.5)-xywh_[1];
                    xywh_[2]=(w+0.5)+xywh_[2];
                    xywh_[3]=(h+0.5)+xywh_[3];
                    xywh[0]=((xywh_[0]+xywh_[2])/2)*stride;
                    xywh[1]=((xywh_[1]+xywh_[3])/2)*stride;
                    xywh[2]=(xywh_[2]-xywh_[0])*stride;
                    xywh[3]=(xywh_[3]-xywh_[1])*stride;
                    xywh[0]=xywh[0]-xywh[2]/2;
                    xywh[1]=xywh[1]-xywh[3]/2;
                    boxes.push_back(xywh[0]);//x
                    boxes.push_back(xywh[1]);//y
                    boxes.push_back(xywh[2]);//w
                    boxes.push_back(xywh[3]);//h
                    boxes.push_back(float(index + (h * grid_w) + w));//keypoints index
                    boxScores.push_back(box_conf_f32);
                    classId.push_back(a);
                    validCount++;
                }
            }
        }
    }
    return validCount;
}

static int process_fp32(float *input, int grid_h, int grid_w, int stride,
                      std::vector<float> &boxes, std::vector<float> &boxScores, std::vector<int> &classId, float threshold,
                      int32_t zp, float scale, int index) {
    int input_loc_len = 64;
    int tensor_len = input_loc_len + OBJ_CLASS_NUM;
    int validCount = 0;
    float thres_fp = unsigmoid(threshold);
    for (int h = 0; h < grid_h; h++) {
        for (int w = 0; w < grid_w; w++) {
            for (int a = 0; a < OBJ_CLASS_NUM; a++) {
                if(input[(input_loc_len + a)*grid_w * grid_h + h * grid_w + w ] >= thres_fp) { //[1,tensor_len,grid_h,grid_w]
                    float box_conf_f32 = sigmoid(input[(input_loc_len + a) * grid_w * grid_h + h * grid_w + w ]);
                    float loc[input_loc_len];
                    for (int i = 0; i < input_loc_len; ++i) {
                        loc[i] = input[i * grid_w * grid_h + h * grid_w + w];
                    }

                    for (int i = 0; i < input_loc_len / 16; ++i) {
                        softmax(&loc[i * 16], 16);
                    }
                    float xywh_[4] = {0, 0, 0, 0};
                    float xywh[4] = {0, 0, 0, 0};
                    for (int dfl = 0; dfl < 16; ++dfl) {
                        xywh_[0] += loc[dfl] * dfl;
                        xywh_[1] += loc[1 * 16 + dfl] * dfl;
                        xywh_[2] += loc[2 * 16 + dfl] * dfl;
                        xywh_[3] += loc[3 * 16 + dfl] * dfl;
                    }
                    xywh_[0]=(w+0.5)-xywh_[0];
                    xywh_[1]=(h+0.5)-xywh_[1];
                    xywh_[2]=(w+0.5)+xywh_[2];
                    xywh_[3]=(h+0.5)+xywh_[3];
                    xywh[0]=((xywh_[0]+xywh_[2])/2)*stride;
                    xywh[1]=((xywh_[1]+xywh_[3])/2)*stride;
                    xywh[2]=(xywh_[2]-xywh_[0])*stride;
                    xywh[3]=(xywh_[3]-xywh_[1])*stride;
                    xywh[0]=xywh[0]-xywh[2]/2;
                    xywh[1]=xywh[1]-xywh[3]/2;
                    boxes.push_back(xywh[0]);//x
                    boxes.push_back(xywh[1]);//y
                    boxes.push_back(xywh[2]);//w
                    boxes.push_back(xywh[3]);//h
                    boxes.push_back(float(index + (h * grid_w) + w));//keypoints index
                    boxScores.push_back(box_conf_f32);
                    classId.push_back(a);
                    validCount++;
                }
            }
        }
    }
    return validCount;
}

int post_process(rknn_app_context_t *app_ctx, void *outputs, letterbox_t *letter_box, float conf_threshold, float nms_threshold,
                 object_detect_result_list *od_results) {
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
    int index = 0;
#ifdef RKNPU1
    for (int i = 0; i < 3; i++) {
        grid_h = app_ctx->output_attrs[i].dims[1];
        grid_w = app_ctx->output_attrs[i].dims[0];
        stride = model_in_h / grid_h;
        if (app_ctx->is_quant) {
            validCount += process_u8((uint8_t *)_outputs[i].buf, grid_h, grid_w, stride, filterBoxes, objProbs,
                                     classId, conf_threshold, app_ctx->output_attrs[i].zp, app_ctx->output_attrs[i].scale, index);
        }
        else
        {
            validCount += process_fp32((float *)_outputs[i].buf, grid_h, grid_w, stride, filterBoxes, objProbs,
                                     classId, conf_threshold, app_ctx->output_attrs[i].zp, app_ctx->output_attrs[i].scale, index);
        }
        index += grid_h * grid_w;
    }
#else
    for (int i = 0; i < 3; i++) {
        grid_h = app_ctx->output_attrs[i].dims[2];
        grid_w = app_ctx->output_attrs[i].dims[3];
        stride = model_in_h / grid_h;
        if (app_ctx->is_quant) {
            validCount += process_i8((int8_t *)_outputs[i].buf, grid_h, grid_w, stride, filterBoxes, objProbs,
                                     classId, conf_threshold, app_ctx->output_attrs[i].zp, app_ctx->output_attrs[i].scale,index);
        }
        else
        {
            validCount += process_fp32((float *)_outputs[i].buf, grid_h, grid_w, stride, filterBoxes, objProbs,
                                     classId, conf_threshold, app_ctx->output_attrs[i].zp, app_ctx->output_attrs[i].scale, index);
        }
        index += grid_h * grid_w;
    }
#endif
    // no object detect
    if (validCount <= 0) {
        return 0;
    }
    std::vector<int> indexArray;
    for (int i = 0; i < validCount; ++i) {
        indexArray.push_back(i);
    }
    quick_sort_indice_inverse(objProbs, 0, validCount - 1, indexArray);

    std::set<int> class_set(std::begin(classId), std::end(classId));

    for (auto c : class_set) {
        nms(validCount, filterBoxes, classId, indexArray, c, nms_threshold);
    }

    int last_count = 0;
    od_results->count = 0;

    /* box valid detect target */
    for (int i = 0; i < validCount; ++i) {
        if (indexArray[i] == -1 || last_count >= OBJ_NUMB_MAX_SIZE) {
            continue;
        }
        int n = indexArray[i];
        float x1 = filterBoxes[n * 5 + 0] - letter_box->x_pad;
        float y1 = filterBoxes[n * 5 + 1] - letter_box->y_pad;
        float w = filterBoxes[n * 5 + 2];
        float h = filterBoxes[n * 5 + 3];
        int keypoints_index = (int)filterBoxes[n * 5 + 4];

        for (int j = 0; j < 17; ++j) {
            if (app_ctx->is_quant) {
                #ifdef RKNPU1
                        od_results->results[last_count].keypoints[j][0] = (deqnt_affine_u8_to_f32(((uint8_t *)_outputs[3].buf)[j * 3 * 8400 + 0 * 8400 + keypoints_index],
                                app_ctx->output_attrs[3].zp, app_ctx->output_attrs[3].scale)- letter_box->x_pad)/ letter_box->scale;
                        od_results->results[last_count].keypoints[j][1] = (deqnt_affine_u8_to_f32(((uint8_t *)_outputs[3].buf)[j * 3 * 8400 + 1 * 8400 + keypoints_index],
                                app_ctx->output_attrs[3].zp, app_ctx->output_attrs[3].scale)- letter_box->y_pad)/ letter_box->scale;
                        od_results->results[last_count].keypoints[j][2] = deqnt_affine_u8_to_f32(((uint8_t *)_outputs[3].buf)[j * 3 * 8400 + 2 * 8400 + keypoints_index],
                                app_ctx->output_attrs[3].zp, app_ctx->output_attrs[3].scale);       
                #else
                        od_results->results[last_count].keypoints[j][0] = ((float)((rknpu2::float16 *)_outputs[3].buf)[j*3*8400+0*8400+keypoints_index] 
                                                                        - letter_box->x_pad)/ letter_box->scale;
                        od_results->results[last_count].keypoints[j][1] = ((float)((rknpu2::float16 *)_outputs[3].buf)[j*3*8400+1*8400+keypoints_index] 
                                                                            - letter_box->y_pad)/ letter_box->scale;
                        od_results->results[last_count].keypoints[j][2] = (float)((rknpu2::float16 *)_outputs[3].buf)[j*3*8400+2*8400+keypoints_index];
                #endif
            }
            else
            {
                od_results->results[last_count].keypoints[j][0] = (((float *)_outputs[3].buf)[j*3*8400+0*8400+keypoints_index] 
                                                                - letter_box->x_pad)/ letter_box->scale;
                od_results->results[last_count].keypoints[j][1] = (((float *)_outputs[3].buf)[j*3*8400+1*8400+keypoints_index] 
                                                                    - letter_box->y_pad)/ letter_box->scale;
                od_results->results[last_count].keypoints[j][2] = ((float *)_outputs[3].buf)[j*3*8400+2*8400+keypoints_index];
            }
        }

        int id = classId[n];
        float obj_conf = objProbs[i];
        od_results->results[last_count].box.left = (int)(clamp(x1, 0, model_in_w) / letter_box->scale);
        od_results->results[last_count].box.top = (int)(clamp(y1, 0, model_in_h) / letter_box->scale);
        od_results->results[last_count].box.right = (int)(clamp(x1+w, 0, model_in_w) / letter_box->scale);
        od_results->results[last_count].box.bottom = (int)(clamp(y1+h, 0, model_in_h) / letter_box->scale);
        // od_results->results[last_count].box.angle = angle;
        od_results->results[last_count].prop = obj_conf;
        od_results->results[last_count].cls_id = id;
        last_count++;
    }
    od_results->count = last_count;
    return 0;
}

int init_post_process() {
    int ret = 0;
    ret = loadLabelName(LABEL_NALE_TXT_PATH, labels);
    if (ret < 0) {
        printf("Load %s failed!\n", LABEL_NALE_TXT_PATH);
        return -1;
    }
    return 0;
}

char *coco_cls_to_name(int cls_id) {

    if (cls_id >= OBJ_CLASS_NUM) {
        return "null";
    }

    if (labels[cls_id]) {
        return labels[cls_id];
    }

    return "null";
}

void deinit_post_process() {
    for (int i = 0; i < OBJ_CLASS_NUM; i++) {
        if (labels[i] != nullptr) {
            free(labels[i]);
            labels[i] = nullptr;
        }
    }
}
