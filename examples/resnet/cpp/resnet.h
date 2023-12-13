#ifndef _RKNN_DEMO_RESNET_H_
#define _RKNN_DEMO_RESNET_H_

#include "rknn_api.h"
#include "common.h"

typedef struct {
    rknn_context rknn_ctx;
    rknn_input_output_num io_num;
    rknn_tensor_attr* input_attrs;
    rknn_tensor_attr* output_attrs;
    int model_channel;
    int model_width;
    int model_height;
} rknn_app_context_t;

typedef struct {
    int cls;
    float score;
} resnet_result;

int init_resnet_model(const char* model_path, rknn_app_context_t* app_ctx);

int release_resnet_model(rknn_app_context_t* app_ctx);

int inference_resnet_model(rknn_app_context_t* app_ctx, image_buffer_t* img, resnet_result* out_result, int topK);

#endif //_RKNN_DEMO_RESNET_H_