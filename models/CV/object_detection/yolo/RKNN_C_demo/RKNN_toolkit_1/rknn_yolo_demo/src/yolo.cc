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
#include "yolo.h"
#include <stdint.h>
#include "rknn_demo_utils.h"

#define LABEL_NALE_TXT_PATH "./model/coco_80_labels_list.txt"

static char *labels[OBJ_CLASS_NUM];

double __get_us(struct timeval t) { return (t.tv_sec * 1000000 + t.tv_usec); }

inline static int clamp(float val, int min, int max)
{
    return val > min ? (val < max ? val : max) : min;
}


int model_type_cmp(MODEL_TYPE *t, char* input_str, const char* type_str, MODEL_TYPE asign_type){
    if (strcmp(input_str, type_str)==0){
        *t = asign_type;
        return 1;
    }
    return 0;
}


MODEL_TYPE string_to_model_type(char* model_type_str){
    int ret = 0;
    MODEL_TYPE _t = MODEL_TYPE_ERROR;
    ret = model_type_cmp(&_t, model_type_str, "yolov5", YOLOV5);
    ret = model_type_cmp(&_t, model_type_str, "v5", YOLOV5);
    ret = model_type_cmp(&_t, model_type_str, "yolov6", YOLOV6);
    ret = model_type_cmp(&_t, model_type_str, "v6", YOLOV6);
    ret = model_type_cmp(&_t, model_type_str, "yolov7", YOLOV7);
    ret = model_type_cmp(&_t, model_type_str, "v7", YOLOV7);
    ret = model_type_cmp(&_t, model_type_str, "yolov8", YOLOV8);
    ret = model_type_cmp(&_t, model_type_str, "v8", YOLOV8);
    ret = model_type_cmp(&_t, model_type_str, "yolox", YOLOX);
    ret = model_type_cmp(&_t, model_type_str, "ppyoloe_plus", PPYOLOE_PLUS);
    ret = model_type_cmp(&_t, model_type_str, "ppyoloe", PPYOLOE_PLUS);
    
    if (_t == MODEL_TYPE_ERROR){
        printf("ERROR: Only support yolov5/yolov6/yolov7/yolov8/yolox/ppyoloe_plus model, but got %s\n", model_type_str);
    }
    return _t;
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


int readFloats(const char *fileName, float* result, int max_line, int* valid_number)
{
    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        printf("failed to open file\n");
        return 1;
    }

    int n = 0;
    while ((n<=max_line) &&(fscanf(file, "%f", &result[n++]) != EOF))
        ;

    /* n-1 float values were successfully read */
    // for (int i=0; i<n-1; i++)
    //     printf("fval[%d]=%f\n", i, result[i]);

    fclose(file);
    *valid_number = n-1;
    return 0;
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

static int nms(int validCount, std::vector<float> &outputLocations, std::vector<int> &class_id, std::vector<int> &order, float threshold, bool class_agnostic)
{
    // printf("class_agnostic: %d\n", class_agnostic);
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

            if (class_agnostic == false && class_id[n] != class_id[m]){
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

/* 
    Post process v1 for yolov5, yolov7, yolox
*/
static int process_v1_u8(uint8_t *input, int *anchor, int anchor_per_branch, int grid_h, int grid_w, int stride,
                   std::vector<float> &boxes, std::vector<float> &boxScores, std::vector<int> &classId,
                   float threshold, uint8_t zp, float scale, MODEL_TYPE yolo)
{
    int validCount = 0;
    int grid_len = grid_h * grid_w;
    float thres = threshold;
    uint8_t thres_u8 = qnt_f32_to_affine(thres, zp, scale);

    for (int a = 0; a < anchor_per_branch; a++)
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

                    float box_conf_f32 = deqnt_affine_to_f32(box_confidence, zp, scale);
                    float class_prob_f32 = deqnt_affine_to_f32(maxClassProbs, zp, scale);
                    float limit_score = 0;
                    limit_score = box_conf_f32* class_prob_f32;

                    // printf("limit score: %f\n", limit_score);
                    if (limit_score > threshold){
                        float box_x, box_y, box_w, box_h;
                        if(yolo == YOLOX){
                            box_x = deqnt_affine_to_f32(*in_ptr, zp, scale);
                            box_y = deqnt_affine_to_f32(in_ptr[grid_len], zp, scale);
                            box_w = deqnt_affine_to_f32(in_ptr[2 * grid_len], zp, scale);
                            box_h = deqnt_affine_to_f32(in_ptr[3 * grid_len], zp, scale);
                            box_w = exp(box_w)* stride;
                            box_h = exp(box_h)* stride;
                        }   
                        else{
                            box_x = deqnt_affine_to_f32(*in_ptr, zp, scale) * 2.0 - 0.5;
                            box_y = deqnt_affine_to_f32(in_ptr[grid_len], zp, scale) * 2.0 - 0.5;
                            box_w = deqnt_affine_to_f32(in_ptr[2 * grid_len], zp, scale) * 2.0;
                            box_h = deqnt_affine_to_f32(in_ptr[3 * grid_len], zp, scale) * 2.0;
                            box_w = box_w * box_w;
                            box_h = box_h * box_h;
                        }

                        box_x = (box_x + j) * (float)stride;
                        box_y = (box_y + i) * (float)stride;
                        if (yolo != YOLOX){
                            box_w *= (float)anchor[a * 2];
                            box_h *= (float)anchor[a * 2 + 1];
                        }

                        box_x -= (box_w / 2.0);
                        box_y -= (box_h / 2.0);

                        boxes.push_back(box_x);
                        boxes.push_back(box_y);
                        boxes.push_back(box_w);
                        boxes.push_back(box_h);
                        boxScores.push_back(box_conf_f32* class_prob_f32);
                        classId.push_back(maxClassId);
                        validCount++;
                    }
                }
            }
        }
    }
    return validCount;
}


static int process_v1_fp(float *input, int *anchor, int anchor_per_branch,int grid_h, int grid_w, int stride,
                   std::vector<float> &boxes, std::vector<float> &boxScores, std::vector<int> &classId,
                   float threshold, MODEL_TYPE yolo)
{
    int validCount = 0;
    int grid_len = grid_h * grid_w;
    for (int a = 0; a < anchor_per_branch; a++)
    {
        for (int i = 0; i < grid_h; i++)
        {
            for (int j = 0; j < grid_w; j++)
            {
                float box_confidence = input[(PROP_BOX_SIZE * a + 4) * grid_len + i * grid_w + j];
                if (box_confidence >= threshold)
                {
                    int offset = (PROP_BOX_SIZE * a) * grid_len + i * grid_w + j;
                    float *in_ptr = input + offset;

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
                    float box_conf_f32 = (box_confidence);
                    float class_prob_f32 = (maxClassProbs);
                    float limit_score = 0;
                    if (yolo == YOLOX){
                        limit_score = class_prob_f32;
                    }
                    else{
                        limit_score = box_conf_f32* class_prob_f32;
                    }
                    // printf("limit score: %f", limit_score);
                    if (limit_score > CONF_THRESHOLD){
                        float box_x, box_y, box_w, box_h;
                        if (yolo == YOLOX){
                            box_x = *in_ptr;
                            box_y = (in_ptr[grid_len]);
                            box_w = exp(in_ptr[2* grid_len])* stride;
                            box_h = exp(in_ptr[3* grid_len])* stride;
                        }
                        else{
                            box_x = *in_ptr * 2.0 - 0.5;
                            box_y = (in_ptr[grid_len]) * 2.0 - 0.5;
                            box_w = (in_ptr[2 * grid_len]) * 2.0;
                            box_h = (in_ptr[3 * grid_len]) * 2.0;
                            box_w *= box_w;
                            box_h *= box_h;
                        }

                        box_x = (box_x + j) * (float)stride;
                        box_y = (box_y + i) * (float)stride;
                        if (yolo != YOLOX){
                            box_w *= (float)anchor[a * 2];
                            box_h *= (float)anchor[a * 2 + 1];
                        }
                        box_x -= (box_w / 2.0);
                        box_y -= (box_h / 2.0);
                        
                        boxes.push_back(box_x);
                        boxes.push_back(box_y);
                        boxes.push_back(box_w);
                        boxes.push_back(box_h);
                        boxScores.push_back(box_conf_f32* class_prob_f32);
                        classId.push_back(maxClassId);
                        validCount++;
                    }
                }
            }
        }
    }
    return validCount;
}

/*
    Post process v2 for yolov6, yolov8, ppyoloe_plus
    Feature:
        score,box in diffenrent tensor
        Anchor free
*/
void compute_dfl(float* tensor, int dfl_len, float* box){
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

static int process_v2_u8(
                   uint8_t *t_box, uint8_t box_zp, float box_scale,
                   uint8_t *t_score, uint8_t score_zp, float score_scale,
                   uint8_t* t_score_sum, uint8_t sum_zp, float sum_scale,
                   int dfl_len, int grid_h, int grid_w, int stride,
                   std::vector<float> &boxes, std::vector<float> &boxScores, std::vector<int> &classId,
                   float threshold, MODEL_TYPE yolo, bool score_sum_available)
{
    int validCount = 0;
    int grid_len = grid_h * grid_w;
    uint8_t score_thres_u8 = qnt_f32_to_affine(threshold, score_zp, score_scale);

    float sum_noise = OBJ_CLASS_NUM* 1./256;
    uint8_t sum_noise_u8   = qnt_f32_to_affine(sum_noise, sum_zp, sum_scale);
    // struct timeval start_time, stop_time;
    // float cc_time=0, box_time=0;

    for (int i= 0; i< grid_h; i++)
    {
        for (int j= 0; j< grid_w; j++)
        {
            float sum = 0;
            int max_class_id = -1;
            int offset = i* grid_w + j;

            if (score_sum_available){
                if (t_score_sum[offset] < sum_noise_u8){
                    continue;
                }
            }

            uint8_t max_score = 0;
            // gettimeofday(&start_time, NULL);
            for (int c= 0; c< OBJ_CLASS_NUM; c++){
                if ((t_score[offset] > score_thres_u8) && (t_score[offset] > max_score))
                {
                    max_score = t_score[offset];
                    max_class_id = c;
                }
                offset += grid_len;
            }
            // gettimeofday(&stop_time, NULL);
            // cc_time += (__get_us(stop_time) - __get_us(start_time)) / 1000;

            // gettimeofday(&start_time, NULL);
            if (max_score> score_thres_u8){
                float box[4];
                offset = i* grid_w + j;
                if ((yolo == YOLOV8 || yolo == PPYOLOE_PLUS || yolo == YOLOV6) && (dfl_len>1)){
                    /// dfl
                    float before_dfl[dfl_len*4];
                    for (int k=0; k< dfl_len*4; k++){
                        before_dfl[k] = deqnt_affine_to_f32(t_box[offset], box_zp, box_scale);
                        offset += grid_len;
                    }
                    compute_dfl(before_dfl, dfl_len, box);
                }
                else if ((yolo == YOLOV6) && (dfl_len < 1)){
                    for (int k=0; k< 4; k++){
                        box[k] = deqnt_affine_to_f32(t_box[offset], box_zp, box_scale);
                        offset += grid_len;
                    }
                }

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
                boxScores.push_back(deqnt_affine_to_f32(max_score, score_zp, score_scale));
                classId.push_back(max_class_id);
                validCount ++;

            }
            // gettimeofday(&stop_time, NULL);
            // box_time += (__get_us(stop_time) - __get_us(start_time)) / 1000;
        }
    }
    // printf("CC_time: %f,    BOX_time: %f\n", cc_time, box_time);
    return validCount;
}


static int process_v2_fp(
                   float *t_box, float *t_score, float* t_score_sum,
                   int dfl_len, int grid_h, int grid_w, int stride,
                   std::vector<float> &boxes, std::vector<float> &boxScores, std::vector<int> &classId,
                   float threshold, MODEL_TYPE yolo, bool score_sum_available)
{
    int validCount = 0;
    int grid_len = grid_h * grid_w;
    float sum_noise = OBJ_CLASS_NUM* 1./256;

    // struct timeval start_time, stop_time;
    // float cc_time=0, box_time=0;

    for (int i= 0; i< grid_h; i++)
    {
        for (int j= 0; j< grid_w; j++)
        {
            float sum = 0;
            int max_class_id = -1;
            int offset = i* grid_w + j;

            if (score_sum_available){
                if (t_score_sum[offset] < sum_noise){
                    continue;
                }
            }

            float max_score = 0;
            // gettimeofday(&start_time, NULL);
            for (int c= 0; c< OBJ_CLASS_NUM; c++){
                if ((t_score[offset] > threshold) && (t_score[offset] > max_score))
                {
                    max_score = t_score[offset];
                    max_class_id = c;
                }
                offset += grid_len;
            }
            // gettimeofday(&stop_time, NULL);
            // cc_time += (__get_us(stop_time) - __get_us(start_time)) / 1000;


            // gettimeofday(&start_time, NULL);
            if (max_score> threshold){
                float box[4];
                offset = i* grid_w + j;
                if ((yolo == YOLOV8 || yolo == PPYOLOE_PLUS || yolo == YOLOV6) && (dfl_len>1)){
                    /// dfl
                    float before_dfl[dfl_len*4];
                    for (int k=0; k< dfl_len*4; k++){
                        before_dfl[k] = t_box[offset];
                        offset += grid_len;
                    }
                    compute_dfl(before_dfl, dfl_len, box);
                }
                else if ((yolo == YOLOV6) && (dfl_len < 1)){
                    for (int k=0; k< 4; k++){
                        box[k] = t_box[offset];
                        offset += grid_len;
                    }
                }

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
                boxScores.push_back(max_score);
                classId.push_back(max_class_id);
                validCount ++;

            }
            // gettimeofday(&stop_time, NULL);
            // box_time += (__get_us(stop_time) - __get_us(start_time)) / 1000;
        }
    }
    // printf("CC_time: %f,    BOX_time: %f\n", cc_time, box_time);
    return validCount;
}


int post_process(void** rk_outputs, MODEL_INFO* m_info, YOLO_INFO* y_info, detect_result_group_t* group)
{
    static int init = -1;
    if (init == -1)
    {
        int ret = 0;
        ret = loadLabelName(LABEL_NALE_TXT_PATH, labels);
        if (ret < 0)
        {
            printf("Failed in loading label\n");
            return -1;
        }

        init = 0;
        printf("post_process load lable finish\n");
    }
    memset(group, 0, sizeof(detect_result_group_t));

    std::vector<float> filterBoxes;
    std::vector<float> boxesScore;
    std::vector<int> classId;
    int validCount = 0;
    int stride = 0;
    int grid_h = 0;
    int grid_w = 0;
    int* anchors;

    for (int i=0; i< 3; i++){
        
        if ((y_info->m_type == YOLOV5) || (y_info->m_type == YOLOV7) || (y_info->m_type == YOLOX)){

            grid_h = m_info->out_attr[i].dims[1];
            grid_w = m_info->out_attr[i].dims[0];
            stride = m_info->in_attr[0].dims[1] / grid_h;

            anchors = &(y_info->anchors[i*2*y_info->anchor_per_branch]);
            if (y_info->post_type == Q8){
                validCount = validCount + process_v1_u8((uint8_t*)rk_outputs[i], anchors, y_info->anchor_per_branch, grid_h, grid_w, stride, 
                                                    filterBoxes, boxesScore, classId, CONF_THRESHOLD, m_info->out_attr[i].zp, m_info->out_attr[i].scale, y_info->m_type);
            }
            else {
                validCount = validCount + process_v1_fp((float*)rk_outputs[i], anchors, y_info->anchor_per_branch, grid_h, grid_w, stride, 
                                                    filterBoxes, boxesScore, classId, CONF_THRESHOLD, y_info->m_type);
            }
        }
        else if ((y_info->m_type == YOLOV6) || (y_info->m_type == YOLOV8) || (y_info->m_type == PPYOLOE_PLUS)){
            void* score_sum = nullptr;
            uint8_t sum_zp = 0;
            float sum_scale = 0.;

            int nt_per_pair = 2;
            if (y_info->score_sum_available){
                nt_per_pair = 3;
                score_sum = (void*)rk_outputs[i*nt_per_pair+2];
                sum_zp = m_info->out_attr[i*nt_per_pair+2].zp;
                sum_scale = m_info->out_attr[i*nt_per_pair+2].scale;
            }

            grid_h = m_info->out_attr[i*nt_per_pair].dims[1];
            grid_w = m_info->out_attr[i*nt_per_pair].dims[0];
            stride = m_info->in_attr[0].dims[1] / grid_h;

            if (y_info->post_type == Q8){
                validCount = validCount + process_v2_u8(
                                                (uint8_t*)rk_outputs[i*nt_per_pair], m_info->out_attr[i*nt_per_pair].zp, m_info->out_attr[i*nt_per_pair].scale,
                                                (uint8_t*)rk_outputs[i*nt_per_pair+1], m_info->out_attr[i*nt_per_pair+1].zp, m_info->out_attr[i*nt_per_pair+1].scale,
                                                (uint8_t*)score_sum, sum_zp, sum_scale,
                                                y_info->dfl_len, grid_h, grid_w, stride, 
                                                filterBoxes, boxesScore, classId, CONF_THRESHOLD, y_info->m_type, y_info->score_sum_available);
            }
            else {
                validCount = validCount + process_v2_fp(
                                                (float*)rk_outputs[i*nt_per_pair], 
                                                (float*)rk_outputs[i*nt_per_pair+1], 
                                                (float*)score_sum, 
                                                y_info->dfl_len, grid_h, grid_w, stride, 
                                                filterBoxes, boxesScore, classId, CONF_THRESHOLD, y_info->m_type, y_info->score_sum_available);
            }
        }
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

    quick_sort_indice_inverse(boxesScore, 0, validCount - 1, indexArray);

    if (y_info->m_type == YOLOX){
        nms(validCount, filterBoxes, classId, indexArray, NMS_THRESHOLD, true);
    }
    else
    {
        nms(validCount, filterBoxes, classId, indexArray, NMS_THRESHOLD, false);
    }
    
    int last_count = 0;
    group->count = 0;
    /* box valid detect target */
    for (int i = 0; i < validCount; ++i)
    {
        if (indexArray[i] == -1 || boxesScore[i] < CONF_THRESHOLD || last_count >= OBJ_NUMB_MAX_SIZE)
        {
            continue;
        }
        int n = indexArray[i];

        float x1 = filterBoxes[n * 4 + 0];
        float y1 = filterBoxes[n * 4 + 1];
        float x2 = x1 + filterBoxes[n * 4 + 2];
        float y2 = y1 + filterBoxes[n * 4 + 3];
        int id = classId[n];

        group->results[last_count].box.left = x1;
        group->results[last_count].box.top = y1;
        group->results[last_count].box.right = x2;
        group->results[last_count].box.bottom = y2;
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
