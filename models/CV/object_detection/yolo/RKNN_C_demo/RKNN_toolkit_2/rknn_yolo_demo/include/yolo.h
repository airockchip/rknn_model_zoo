#ifndef _RKNN_ZERO_COPY_DEMO_POSTPROCESS_H_
#define _RKNN_ZERO_COPY_DEMO_POSTPROCESS_H_

#include <stdint.h>
#include "rknn_api.h"
#include "rga_func.h"

#define OBJ_NAME_MAX_SIZE 16
#define OBJ_NUMB_MAX_SIZE 200
#define OBJ_CLASS_NUM     80
#define PROP_BOX_SIZE     (5+OBJ_CLASS_NUM)
#define NMS_THRESHOLD     0.45
#define CONF_THRESHOLD    0.25
#define MAX_OUTPUTS 3

typedef enum
{
    YOLOX = 0,
    YOLOV5,
    YOLOV7
} MODEL_TYPE;


typedef enum
{
    Q8 = 0,
    FP = 1,
} POST_PROCESS_TYPE;

typedef enum
{
    SINGLE_IMG = 0,
    MULTI_IMG
} INPUT_SOURCE;


typedef struct _MODEL_INFO{
    MODEL_TYPE m_type;
    POST_PROCESS_TYPE post_type;
    INPUT_SOURCE in_source;

    char* m_path = nullptr;
    char* in_path = nullptr;

    int channel;
    int height; 
    int width;
    RgaSURF_FORMAT color_expect;

    int anchors[18];
    int anchor_per_branch;

    int in_nodes;
    rknn_tensor_attr* in_attr = nullptr;

    int out_nodes = 3;
    rknn_tensor_attr* out_attr = nullptr;

    int strides[3] = {8,16,32};

} MODEL_INFO;

typedef struct _LETTER_BOX{
    int in_width, in_height;
    int target_width, target_height;

    float img_wh_ratio, target_wh_ratio, resize_scale;
    int resize_width, resize_height;
    int h_pad, w_pad;
    bool add_extra_sz_h_pad = false;
    bool add_extra_sz_w_pad = false;
} LETTER_BOX;


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

int readLines(const char *fileName, char *lines[], int max_line);

int compute_letter_box(LETTER_BOX* lb);

//int post_process(rknn_output* rk_outputs, MODEL_INFO* m, LETTER_BOX* lb, detect_result_group_t* group);
int post_process(void** rk_outputs, MODEL_INFO* m, LETTER_BOX* lb, detect_result_group_t* group);

int readFloats(const char *fileName, float *result, int max_line, int* valid_number);

#endif //_RKNN_ZERO_COPY_DEMO_POSTPROCESS_H_
