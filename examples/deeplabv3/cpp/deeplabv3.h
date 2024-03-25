#ifndef _RKNN_DEMO_DEEPLABV3_H_
#define _RKNN_DEMO_DEEPLABV3_H_

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

int init_deeplabv3_model(const char* model_path, rknn_app_context_t* app_ctx);

int release_deeplabv3_model(rknn_app_context_t* app_ctx);

int inference_deeplabv3_model(rknn_app_context_t* app_ctx, image_buffer_t* img);

#endif //_RKNN_DEMO_DEEPLABV3_H_