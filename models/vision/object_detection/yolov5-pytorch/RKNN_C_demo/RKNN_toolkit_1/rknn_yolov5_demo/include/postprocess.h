#ifndef _RKNN_ZERO_COPY_DEMO_POSTPROCESS_H_
#define _RKNN_ZERO_COPY_DEMO_POSTPROCESS_H_

#include <stdint.h>

#define OBJ_NAME_MAX_SIZE 16
#define OBJ_NUMB_MAX_SIZE 200
#define OBJ_CLASS_NUM     80
#define PROP_BOX_SIZE     (5+OBJ_CLASS_NUM)

typedef struct _BOX_RECT
{
    int left;
    int right;
    int top;
    int bottom;
} BOX_RECT;

typedef struct __detect_result_t
{
    char name[OBJ_NAME_MAX_SIZE];
    int class_index;
    BOX_RECT box;
    float prop;
} detect_result_t;

typedef struct _detect_result_group_t
{
    int id;
    int count;
    detect_result_t results[OBJ_NUMB_MAX_SIZE];
} detect_result_group_t;

int post_process_u8(uint8_t *input0, uint8_t *input1, uint8_t *input2, int model_in_h, int model_in_w,
                 int h_offset, int w_offset, float resize_scale, float conf_threshold, float nms_threshold, 
                 std::vector<uint8_t> &qnt_zps, std::vector<float> &qnt_scales,
                 detect_result_group_t *group);

int post_process_fp(float *input0, float *input1, float *input2, int model_in_h, int model_in_w,
                 int h_offset, int w_offset, float resize_scale, float conf_threshold, float nms_threshold, 
                 detect_result_group_t *group);

int readLines(const char *fileName, char *lines[], int max_line);

#endif //_RKNN_ZERO_COPY_DEMO_POSTPROCESS_H_
