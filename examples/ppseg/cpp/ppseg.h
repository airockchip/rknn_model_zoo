#ifndef _RKNN_DEMO_ppseg_H_
#define _RKNN_DEMO_ppseg_H_

#include "rknn_api.h"
#include "common.h"
#include <tuple>

typedef struct {
    rknn_context rknn_ctx;
    rknn_input_output_num io_num;
    rknn_tensor_attr* input_attrs;
    rknn_tensor_attr* output_attrs;
    int model_channel;
    int model_width;
    int model_height;
} rknn_app_context_t;

int init_ppseg_model(const char* model_path, rknn_app_context_t* app_ctx);

int release_ppseg_model(rknn_app_context_t* app_ctx);

int inference_ppseg_model(rknn_app_context_t* app_ctx, image_buffer_t* img, image_buffer_t*result_image);

#endif //_RKNN_DEMO_ppseg_H_