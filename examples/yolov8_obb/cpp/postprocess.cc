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

#include "yolov8-obb.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <iostream>
#include <cmath>
#include <algorithm>

#include <set>
#include <vector>
#define LABEL_NALE_TXT_PATH "./model/yolov8_obb_labels_list.txt"

static char *labels[OBJ_CLASS_NUM];

const int anchor[3][6] = {{10, 13, 16, 30, 33, 23},
    {30, 61, 62, 45, 59, 119},
    {116, 90, 156, 198, 373, 326}
};

inline static int clamp(float val, int min, int max) {
    return val > min ? (val < max ? val : max) : min;
}

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

static inline void GetCovarianceMatrix(float Box_w,float Box_h, float Box_angle, float &A, float &B, float &C)
{
    float a = Box_w;
    float b = Box_h;
    float c = Box_angle;

    float cos1 = cos(c);
    float sin1 = sin(c);
    float cos2 = pow(cos1, 2);
    float sin2 = pow(sin1, 2);

    A = a * cos2 + b * sin2;
    B = a * sin2 + b * cos2;
    C = (a - b) * cos1 * sin1;
}

static inline float probiou(float obb1_x,float obb1_y,float obb1_w,float obb1_h,float obb1_angle,
    float obb2_x,float obb2_y,float obb2_w,float obb2_h,float obb2_angle)
{
    float eps = 1e-7;

    float x1 = obb1_x;
    float y1 = obb1_y;
    float x2 = obb2_x;
    float y2 = obb2_y;
    float a1 = 0, b1 = 0, c1 = 0;
    GetCovarianceMatrix(obb1_w, obb1_h, obb1_angle, a1, b1, c1);

    float a2 = 0, b2 = 0, c2 = 0;
    GetCovarianceMatrix(obb2_w, obb2_h, obb2_angle, a2, b2, c2);

    float t1 = (((a1 + a2) * pow((y1 - y2), 2) + (b1 + b2) * pow((x1 - x2), 2)) / ((a1 + a2) * (b1 + b2) - pow((c1 + c2), 2) + eps)) * 0.25;
    float t2 = (((c1 + c2) * (x2 - x1) * (y1 - y2)) / ((a1 + a2) * (b1 + b2) - pow((c1 + c2), 2) + eps)) * 0.5;

    float temp1 = (a1 * b1 - pow(c1, 2));
    temp1 = temp1 > 0 ? temp1 : 0;

    float temp2 = (a2 * b2 - pow(c2, 2));
    temp2 = temp2 > 0 ? temp2 : 0;

    float t3 = log((((a1 + a2) * (b1 + b2) - pow((c1 + c2), 2)) / (4 * sqrt((temp1 * temp2)) + eps) + eps)) * 0.5;

    float bd = 0;
    if ((t1 + t2 + t3) > 100)
    {
        bd = 100;
    }
    else if ((t1 + t2 + t3) < eps)
    {
        bd = eps;
    }
    else
    {
        bd = t1 + t2 + t3;
    }

    float hd = sqrt((1.0 - exp(-bd) + eps));
    return 1 - hd;
}

static int nms(int validCount, std::vector<float> &outputLocations, std::vector<int> classIds, std::vector<int> &order,
               int filterId, float threshold) {
    for (int i = 0; i < validCount; ++i) {
        if (order[i] == -1 || classIds[i] != filterId) {
            continue;
        }
        int n = order[i];
        for (int j = i + 1; j < validCount; ++j) {
            int m = order[j];
            if (m == -1 || classIds[i] != filterId) {
                continue;
            }
            float xmin0 = outputLocations[n * 5 + 0];
            float ymin0 = outputLocations[n * 5 + 1];
            float w0 = outputLocations[n * 5 + 2];
            float h0 = outputLocations[n * 5 + 3];
            float angle0 = outputLocations[n * 5 + 4];

            float xmin1 = outputLocations[m * 5 + 0];
            float ymin1 = outputLocations[m * 5 + 1];
            float w1 = outputLocations[m * 5 + 2];
            float h1 = outputLocations[m * 5 + 3];
            float angle1 = outputLocations[n * 5 + 4];
            
            float iou = probiou(xmin0, ymin0, w0, h0, angle0, xmin1, ymin1, w1, h1, angle1);
            if (iou > threshold) {
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

static int process_i8(int8_t *input, int8_t *angle_feature, int grid_h, int grid_w, int stride,
                      std::vector<float> &boxes, std::vector<float> &boxScores, std::vector<int> &classId, float threshold,
                      int32_t zp, float scale, int32_t angle_feature_zp, float angle_feature_scale, int index) {
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

                    float xywh_add[2], xywh_sub[2];
                    xywh_add[0] = xywh_[0] + xywh_[2];
                    xywh_add[1] = xywh_[1] + xywh_[3];
                    xywh_sub[0] = (xywh_[2] - xywh_[0]) / 2;
                    xywh_sub[1] = (xywh_[3] - xywh_[1]) / 2;
                    float angle_feature_ = deqnt_affine_to_f32(angle_feature[index + (h * grid_w) + w], angle_feature_zp, angle_feature_scale);
                    angle_feature_ = (angle_feature_ - 0.25) * 3.1415927410125732;
                    float angle_feature_cos = cos(angle_feature_);
                    float angle_feature_sin = sin(angle_feature_);
                    float xy_mul1 = xywh_sub[0] * angle_feature_cos;
                    float xy_mul2 = xywh_sub[1] * angle_feature_sin;
                    float xy_mul3 = xywh_sub[0] * angle_feature_sin;
                    float xy_mul4 = xywh_sub[1] * angle_feature_cos;
                    xywh_[0] = ((xy_mul1 - xy_mul2) + w + 0.5) * stride;
                    xywh_[1] = ((xy_mul3 + xy_mul4) + h + 0.5) * stride;
                    xywh_[2] = xywh_add[0] * stride;
                    xywh_[3] = xywh_add[1] * stride;
                    xywh[0] = (xywh_[0] - xywh_[2] / 2);
                    xywh[1] = (xywh_[1] - xywh_[3] / 2);
                    xywh[2] = xywh_[2];
                    xywh[3] = xywh_[3];
                    boxes.push_back(xywh[0]);//x
                    boxes.push_back(xywh[1]);//y
                    boxes.push_back(xywh[2]);//w
                    boxes.push_back(xywh[3]);//h
                    boxes.push_back(angle_feature_);//angle
                    boxScores.push_back(box_conf_f32);
                    classId.push_back(a);
                    validCount++;
                }
            }
        }
    }
    return validCount;
}


static int process_u8(uint8_t *input, uint8_t *angle_feature, int grid_h, int grid_w, int stride,
                      std::vector<float> &boxes, std::vector<float> &boxScores, std::vector<int> &classId, float threshold,
                      int32_t zp, float scale, int32_t angle_feature_zp, float angle_feature_scale, int index) {
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

                    float xywh_add[2], xywh_sub[2];
                    xywh_add[0] = xywh_[0] + xywh_[2];
                    xywh_add[1] = xywh_[1] + xywh_[3];
                    xywh_sub[0] = (xywh_[2] - xywh_[0]) / 2;
                    xywh_sub[1] = (xywh_[3] - xywh_[1]) / 2;
                    float angle_feature_ = deqnt_affine_u8_to_f32(angle_feature[index + (h * grid_w) + w], angle_feature_zp, angle_feature_scale);
                    angle_feature_ = (angle_feature_ - 0.25) * 3.1415927410125732;
                    float angle_feature_cos = cos(angle_feature_);
                    float angle_feature_sin = sin(angle_feature_);
                    float xy_mul1 = xywh_sub[0] * angle_feature_cos;
                    float xy_mul2 = xywh_sub[1] * angle_feature_sin;
                    float xy_mul3 = xywh_sub[0] * angle_feature_sin;
                    float xy_mul4 = xywh_sub[1] * angle_feature_cos;
                    xywh_[0] = ((xy_mul1 - xy_mul2) + w + 0.5) * stride;
                    xywh_[1] = ((xy_mul3 + xy_mul4) + h + 0.5) * stride;
                    xywh_[2] = xywh_add[0] * stride;
                    xywh_[3] = xywh_add[1] * stride;
                    xywh[0] = (xywh_[0] - xywh_[2] / 2);
                    xywh[1] = (xywh_[1] - xywh_[3] / 2);
                    xywh[2] = xywh_[2];
                    xywh[3] = xywh_[3];
                    boxes.push_back(xywh[0]);//x
                    boxes.push_back(xywh[1]);//y
                    boxes.push_back(xywh[2]);//w
                    boxes.push_back(xywh[3]);//h
                    boxes.push_back(angle_feature_);//angle
                    boxScores.push_back(box_conf_f32);
                    classId.push_back(a);
                    validCount++;
                }
            }
        }
    }
    return validCount;
}

static int process_fp32(float *input, float *angle_feature, int grid_h, int grid_w, int stride,
                      std::vector<float> &boxes, std::vector<float> &boxScores, std::vector<int> &classId, float threshold,
                      int32_t zp, float scale, int32_t angle_feature_zp, float angle_feature_scale, int index) {
    int input_loc_len = 64;
    int tensor_len = input_loc_len + OBJ_CLASS_NUM;
    int validCount = 0;

    float thres_fp32 = unsigmoid(threshold);
    for (int h = 0; h < grid_h; h++) {
        for (int w = 0; w < grid_w; w++) {
            for (int a = 0; a < OBJ_CLASS_NUM; a++) {
                if(input[(input_loc_len + a)*grid_w * grid_h + h * grid_w + w ] >= thres_fp32) { //[1,tensor_len,grid_h,grid_w]
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

                    float xywh_add[2], xywh_sub[2];
                    xywh_add[0] = xywh_[0] + xywh_[2];
                    xywh_add[1] = xywh_[1] + xywh_[3];
                    xywh_sub[0] = (xywh_[2] - xywh_[0]) / 2;
                    xywh_sub[1] = (xywh_[3] - xywh_[1]) / 2;
                    float angle_feature_ = angle_feature[index + (h * grid_w) + w];
                    angle_feature_ = (angle_feature_ - 0.25) * 3.1415927410125732;
                    float angle_feature_cos = cos(angle_feature_);
                    float angle_feature_sin = sin(angle_feature_);
                    float xy_mul1 = xywh_sub[0] * angle_feature_cos;
                    float xy_mul2 = xywh_sub[1] * angle_feature_sin;
                    float xy_mul3 = xywh_sub[0] * angle_feature_sin;
                    float xy_mul4 = xywh_sub[1] * angle_feature_cos;
                    xywh_[0] = ((xy_mul1 - xy_mul2) + w + 0.5) * stride;
                    xywh_[1] = ((xy_mul3 + xy_mul4) + h + 0.5) * stride;
                    xywh_[2] = xywh_add[0] * stride;
                    xywh_[3] = xywh_add[1] * stride;
                    xywh[0] = (xywh_[0] - xywh_[2] / 2);
                    xywh[1] = (xywh_[1] - xywh_[3] / 2);
                    xywh[2] = xywh_[2];
                    xywh[3] = xywh_[3];
                    boxes.push_back(xywh[0]);//x
                    boxes.push_back(xywh[1]);//y
                    boxes.push_back(xywh[2]);//w
                    boxes.push_back(xywh[3]);//h
                    boxes.push_back(angle_feature_);//angle
                    boxScores.push_back(box_conf_f32);
                    classId.push_back(a);
                    validCount++;
                }
            }
        }
    }
    return validCount;
}

int post_process(rknn_app_context_t *app_ctx, void *outputs, letterbox_t *letter_box, float conf_threshold, float nms_threshold, object_detect_result_list *od_results) {
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
            validCount += process_u8((uint8_t *)_outputs[i].buf, (uint8_t *)_outputs[3].buf, grid_h, grid_w, stride, filterBoxes, objProbs,
                                     classId, conf_threshold, app_ctx->output_attrs[i].zp, app_ctx->output_attrs[i].scale,
                                     app_ctx->output_attrs[3].zp, app_ctx->output_attrs[3].scale, index);
        } else {
            validCount += process_fp32((float *)_outputs[i].buf, (float *)_outputs[3].buf, grid_h, grid_w, stride, filterBoxes, objProbs,
                                     classId, conf_threshold, app_ctx->output_attrs[i].zp, app_ctx->output_attrs[i].scale,
                                     app_ctx->output_attrs[3].zp, app_ctx->output_attrs[3].scale, index);
        }
        index += grid_h * grid_w;
    }
#else
    for (int i = 0; i < 3; i++) {
        grid_h = app_ctx->output_attrs[i].dims[2];
        grid_w = app_ctx->output_attrs[i].dims[3];
        stride = model_in_h / grid_h;
        if (app_ctx->is_quant) {
            validCount += process_i8((int8_t *)_outputs[i].buf, (int8_t *)_outputs[3].buf, grid_h, grid_w, stride, filterBoxes, objProbs,
                                     classId, conf_threshold, app_ctx->output_attrs[i].zp, app_ctx->output_attrs[i].scale,
                                     app_ctx->output_attrs[3].zp, app_ctx->output_attrs[3].scale, index);
        } else {
            validCount += process_fp32((float *)_outputs[i].buf, (float *)_outputs[3].buf, grid_h, grid_w, stride, filterBoxes, objProbs,
                                     classId, conf_threshold, app_ctx->output_attrs[i].zp, app_ctx->output_attrs[i].scale,
                                     app_ctx->output_attrs[3].zp, app_ctx->output_attrs[3].scale, index);
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
        float angle = filterBoxes[n * 5 + 4];

        // std::vector<float> rbbox_to_corners(const std::vector<float> &rbbox)

        int id = classId[n];
        float obj_conf = objProbs[i];

        od_results->results[last_count].box.x = (int)(clamp(x1, 0, model_in_w) / letter_box->scale);
        od_results->results[last_count].box.y = (int)(clamp(y1, 0, model_in_h) / letter_box->scale);
        od_results->results[last_count].box.w = (int)(clamp(w, 0, model_in_w) / letter_box->scale);
        od_results->results[last_count].box.h = (int)(clamp(h, 0, model_in_h) / letter_box->scale);
        od_results->results[last_count].box.angle = angle;
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
