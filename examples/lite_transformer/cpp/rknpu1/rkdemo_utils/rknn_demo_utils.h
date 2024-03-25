#ifndef _RKNN_DEMO_UTILS_H
#define _RKNN_DEMO_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <memory.h>
#include <assert.h>

#include "rknn_api.h"

typedef struct _MODEL_INFO
{
    rknn_context ctx; // rknn context
    bool use_zp;      // whether use zero copy api, default is true

    uint32_t n_input;                     // input number
    rknn_tensor_attr *in_attrs = nullptr; // input tensors` attribute
    rknn_input *inputs = nullptr;         // rknn inputs, used for normal api
    rknn_tensor_mem **in_mems = nullptr;  // inputs` memory, used for zero-copy api

    uint32_t n_output;                     // output number
    rknn_tensor_attr *out_attrs = nullptr; // output tensors` attribute
    rknn_output *outputs = nullptr;        // rknn outputs, used for normal api
    rknn_tensor_mem **out_mems = nullptr;  // outputs` memory, used for zero-copy api
} MODEL_INFO;

int rkdemo_model_init(bool is_encoder, const char *model_path, MODEL_INFO *model_info);
int rkdemo_model_release(MODEL_INFO *model_info);

#endif