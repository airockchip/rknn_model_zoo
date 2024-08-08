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
#include <string>

#include "clip_text.h"
#include "common.h"
#include "file_utils.h"
#include "clip_tokenizer.h"


static void dump_tensor_attr(rknn_tensor_attr* attr)
{
    std::string shape_str = attr->n_dims < 1 ? "" : std::to_string(attr->dims[0]);
    for (int i = 1; i < attr->n_dims; ++i) {
        shape_str += ", " + std::to_string(attr->dims[i]);
    }

    printf("  index=%d, name=%s, n_dims=%d, dims=[%s], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, "
           "zp=%d, scale=%f\n",
           attr->index, attr->name, attr->n_dims, shape_str.c_str(), attr->n_elems, attr->size,get_format_string(attr->fmt),
           get_type_string(attr->type), get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}


int init_clip_text_model(rknn_clip_context* clip_ctx, const char* model_path)
{
    int ret;
    rknn_context ctx = 0;

    // Load RKNN Model
    ret = rknn_init(&ctx, (char*)model_path, 0, 0, NULL);
    if (ret < 0)
    {
        printf("rknn_init fail ret=%d\n", ret);
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
    clip_ctx->rknn_ctx = ctx;
    clip_ctx->io_num = io_num;
    clip_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
    memcpy(clip_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
    clip_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
    memcpy(clip_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

    return 0;
}

int release_clip_text_model(rknn_clip_context* clip_ctx)
{
    if (clip_ctx->input_attrs != NULL)
    {
        free(clip_ctx->input_attrs);
        clip_ctx->input_attrs = NULL;
    }
    if (clip_ctx->output_attrs != NULL)
    {
        free(clip_ctx->output_attrs);
        clip_ctx->output_attrs = NULL;
    }
    if (clip_ctx->rknn_ctx != 0)
    {
        rknn_destroy(clip_ctx->rknn_ctx);
        clip_ctx->rknn_ctx = 0;
    }
    return 0;
}

int inference_clip_text_model(rknn_clip_context* clip_ctx, char** input_texts, int text_num, float text_output[])
{
    int ret;
    int* tokens;
    rknn_input inputs[1];
    rknn_output outputs[1];
    CLIPTokenizer* clip_tokenize = new CLIPTokenizer();

    int sequence_len = clip_ctx->input_attrs[0].dims[1];
    int tokens_num = text_num * sequence_len;
    tokens = (int*)malloc(tokens_num * sizeof(int));

    for (int i = 0; i < text_num; i++)
    {
        std::vector<int> token = clip_tokenize->tokenize(input_texts[i], sequence_len, true);
        for (int j = 0; j < token.size(); j++)
        {
            tokens[i*sequence_len+j] = token[j];
        }
    }

    delete clip_tokenize;

    for (int i = 0; i < text_num; i++)
    {
        memset(inputs, 0, sizeof(inputs));
        memset(outputs, 0, sizeof(outputs));

        // Set Input Data
        inputs[0].index = 0;
        inputs[0].type = RKNN_TENSOR_INT32;
        inputs[0].fmt = RKNN_TENSOR_UNDEFINED;
        inputs[0].size = clip_ctx->input_attrs[0].dims[0] * clip_ctx->input_attrs[0].dims[1] * sizeof(int32_t);
        inputs[0].buf = tokens + (i*clip_ctx->input_attrs[0].dims[1]);

        ret = rknn_inputs_set(clip_ctx->rknn_ctx, clip_ctx->io_num.n_input, inputs);
        if (ret < 0)
        {
            printf("rknn_input_set fail! ret=%d\n", ret);
            return -1;
        }

        // Run
        printf("rknn_run_%d\n", i+1);
        ret = rknn_run(clip_ctx->rknn_ctx, NULL);
        if (ret < 0)
        {
            printf("rknn_run fail! ret=%d\n", ret);
            return -1;
        }

        // Get Output
        outputs[0].want_float = 1;
        ret = rknn_outputs_get(clip_ctx->rknn_ctx, 1, outputs, NULL);
        if (ret < 0)
        {
            printf("rknn_outputs_get fail! ret=%d\n", ret);
            goto out;
        }

        memcpy(text_output + (i*clip_ctx->output_attrs[0].dims[1]), (float*)outputs[0].buf, outputs[0].size);

    }

    // Remeber to release rknn output
    rknn_outputs_release(clip_ctx->rknn_ctx, 1, outputs);

out:
    if (tokens != NULL)
    {
        free(tokens);
    }

    return ret;
}
