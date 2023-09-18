#ifndef _RKNN_ZERO_COPY_DEMO_POSTPROCESS_H_
#define _RKNN_ZERO_COPY_DEMO_POSTPROCESS_H_

#include <stdint.h>
#include "rknn_api.h"
#include "rknn_demo_utils.h"
// #include "rga_func.h"

#define OBJ_NAME_MAX_SIZE 64
#define OBJ_NUMB_MAX_SIZE 200
#define OBJ_CLASS_NUM     80
#define PROP_BOX_SIZE     (5+OBJ_CLASS_NUM)
#define NMS_THRESHOLD     0.45
#define CONF_THRESHOLD    0.25

typedef enum
{
    MODEL_TYPE_ERROR = -1,
    YOLOX = 0,
    YOLOV5,
    YOLOV6,
    YOLOV7,
    YOLOV8,
    PPYOLOE_PLUS
} MODEL_TYPE;
MODEL_TYPE string_to_model_type(char* model_type_str);

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

typedef struct _YOLO_INFO{
    MODEL_TYPE m_type;
    POST_PROCESS_TYPE post_type;
    char* in_path;
    INPUT_SOURCE in_source;

    int channel;
    int height; 
    int width;

    int anchors[18];
    int anchor_per_branch;
    int strides[3];

    int dfl_len;
    bool score_sum_available;
} YOLO_INFO;



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

// int post_process(rknn_output* rk_outputs, MODEL_INFO* m, LETTER_BOX* lb, detect_result_group_t* group);
int post_process(void** rk_outputs, MODEL_INFO* m, YOLO_INFO* y, detect_result_group_t* group);

int readFloats(const char *fileName, float *result, int max_line, int* valid_number);

double __get_us(struct timeval t);


#endif //_RKNN_ZERO_COPY_DEMO_POSTPROCESS_H_
