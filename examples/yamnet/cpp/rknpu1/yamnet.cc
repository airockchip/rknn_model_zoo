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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "yamnet.h"
#include "file_utils.h"
#include "audio_utils.h"
#include "process.h"

static void dump_tensor_attr(rknn_tensor_attr *attr)
{
    char dims_str[100];
    char temp_str[100];
    memset(dims_str, 0, sizeof(dims_str));
    for (int i = attr->n_dims - 1; i >= 0; --i)
    {
        strcpy(temp_str, dims_str);
        if (i == 0)
        {
            sprintf(dims_str, "%s%d", temp_str, attr->dims[i]);
        }
        else
        {
            sprintf(dims_str, "%s%d, ", temp_str, attr->dims[i]);
        }
    }

    printf("  index=%d, name=%s, n_dims=%d, dims=[%s], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, zp=%d, scale=%f\n",
           attr->index, attr->name, attr->n_dims, dims_str, attr->n_elems, attr->size, get_format_string(attr->fmt),
           get_type_string(attr->type), get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}

int init_yamnet_model(const char *model_path, rknn_app_context_t *app_ctx)
{
    int ret;
    int model_len = 0;
    char *model;
    rknn_context ctx = 0;

    // Load RKNN Model
    model_len = read_data_from_file(model_path, &model);
    if (model == NULL)
    {
        printf("load_model fail!\n");
        return -1;
    }

    ret = rknn_init(&ctx, model, model_len, 0);
    free(model);
    if (ret < 0)
    {
        printf("rknn_init fail! ret=%d\n", ret);
        return -1;
    }

    // Get Model Input Output Number
    rknn_input_output_num io_num;
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC)
    {
        printf("rknn_query fail! ret=%d\n", ret);
        return -1;
    }
    printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

    // Get Model Input Info
    printf("input tensors:\n");
    rknn_tensor_attr input_attrs[io_num.n_input];
    memset(input_attrs, 0, sizeof(input_attrs));
    for (int i = 0; i < io_num.n_input; i++)
    {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(input_attrs[i]));
    }

    // Get Model Output Info
    printf("output tensors:\n");
    rknn_tensor_attr output_attrs[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs));
    for (int i = 0; i < io_num.n_output; i++)
    {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(output_attrs[i]));
    }

    // Set to context
    app_ctx->rknn_ctx = ctx;
    app_ctx->io_num = io_num;
    app_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
    app_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

    return 0;
}

int release_yamnet_model(rknn_app_context_t *app_ctx)
{
    if (app_ctx->input_attrs != NULL)
    {
        free(app_ctx->input_attrs);
        app_ctx->input_attrs = NULL;
    }
    if (app_ctx->output_attrs != NULL)
    {
        free(app_ctx->output_attrs);
        app_ctx->output_attrs = NULL;
    }
    if (app_ctx->rknn_ctx != 0)
    {
        rknn_destroy(app_ctx->rknn_ctx);
        app_ctx->rknn_ctx = 0;
    }
    return 0;
}

int inference_yamnet_model(rknn_app_context_t *app_ctx, audio_buffer_t *audio, LabelEntry *label, ResultEntry *result)
{
    int ret;

    rknn_input inputs[1];
    rknn_output outputs[3];

    memset(inputs, 0, sizeof(inputs));
    memset(outputs, 0, sizeof(outputs));

    // Set Input Data
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_FLOAT32;
    inputs[0].size = N_SAMPLES * sizeof(float);
    inputs[0].buf = (float *)malloc(inputs[0].size);

    // Audio Pre Process
    ret = audio_preprocess(audio, (float *)inputs[0].buf);
    if (ret < 0)
    {
        printf("audio_preprocess fail! ret=%d\n", ret);
        goto out;
    }

    ret = rknn_inputs_set(app_ctx->rknn_ctx, 1, inputs);
    if (ret < 0)
    {
        printf("rknn_input_set fail! ret=%d\n", ret);
        goto out;
    }

    // Run
    ret = rknn_run(app_ctx->rknn_ctx, nullptr);
    if (ret < 0)
    {
        printf("rknn_run fail! ret=%d\n", ret);
        goto out;
    }

    // Get Output
    outputs[2].want_float = 1;
    ret = rknn_outputs_get(app_ctx->rknn_ctx, 3, outputs, NULL);
    if (ret < 0)
    {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        goto out;
    }

    // post process
    post_process((float *)outputs[2].buf, label, result);

out:

    // Remeber to release rknn output
    rknn_outputs_release(app_ctx->rknn_ctx, 3, outputs);
    if (inputs[0].buf != NULL)
    {
        free(inputs[0].buf);
    }

    return ret;
}
