#ifndef _RKNN_DEMO_MOBILENET_H_
#define _RKNN_DEMO_MOBILENET_H_

#include "rknn_api.h"
#include "common.h"

typedef struct {
    char *dma_buf_virt_addr;
    int dma_buf_fd;
    size_t size;
}rknn_dma_buf;

typedef struct {
    rknn_context rknn_ctx;
    rknn_input_output_num io_num;
    rknn_tensor_attr* input_attrs;
    rknn_tensor_attr* output_attrs;
#if defined(RV1106_1103) 
    rknn_tensor_mem* input_mems[1];
    rknn_tensor_mem* output_mems[1];
    rknn_dma_buf img_dma_buf;
#endif
    int model_channel;
    int model_width;
    int model_height;
} rknn_app_context_t;

typedef struct {
    int cls;
    float score;
} mobilenet_result;

int init_mobilenet_model(const char* model_path, rknn_app_context_t* app_ctx);

int release_mobilenet_model(rknn_app_context_t* app_ctx);

int inference_mobilenet_model(rknn_app_context_t* app_ctx, image_buffer_t* img, mobilenet_result* out_result, int topK);

#endif //_RKNN_DEMO_MOBILENET_H_