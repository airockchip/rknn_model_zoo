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
#include "whisper.h"
#include "file_utils.h"
#include "audio_utils.h"
#include <vector>
#include "process.h"

static void dump_tensor_attr(rknn_tensor_attr *attr)
{
    char dims_str[100];
    char temp_str[100];
    memset(dims_str, 0, sizeof(dims_str));
    for (int i = 0; i < attr->n_dims; i++)
    {
        strcpy(temp_str, dims_str);
        if (i == attr->n_dims - 1)
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

int init_whisper_model(const char *model_path, rknn_app_context_t *app_ctx)
{
    int ret;
    int model_len = 0;
    rknn_context ctx = 0;

    // Load RKNN Model
    ret = rknn_init(&ctx, (void *)model_path, model_len, 0, NULL);
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

int release_whisper_model(rknn_app_context_t *app_ctx)
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

int inference_encoder_model(rknn_app_context_t *app_ctx, std::vector<float> audio_data, float *mel_filters, float *encoder_output)
{
    int ret;

    rknn_input inputs[1];
    rknn_output outputs[1];

    memset(inputs, 0, sizeof(inputs));
    memset(outputs, 0, sizeof(outputs));

    // Set Input Data
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_FLOAT32;
    inputs[0].size = N_MELS * ENCODER_INPUT_SIZE * sizeof(float);
    inputs[0].buf = (float *)malloc(inputs[0].size);
    memcpy(inputs[0].buf, audio_data.data(), inputs[0].size);

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
    outputs[0].want_float = 1;
    ret = rknn_outputs_get(app_ctx->rknn_ctx, 1, outputs, NULL);
    if (ret < 0)
    {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        goto out;
    }

    memcpy(encoder_output, (float *)outputs[0].buf, ENCODER_OUTPUT_SIZE * sizeof(float));

out:

    // Remeber to release rknn output
    rknn_outputs_release(app_ctx->rknn_ctx, 1, outputs);
    if (inputs[0].buf != NULL)
    {
        free(inputs[0].buf);
    }

    return ret;
}

int inference_decoder_model(rknn_app_context_t *app_ctx, float *encoder_output, VocabEntry *vocab, int task_code, std::vector<std::string> &recognized_text)
{
    int ret;
    rknn_input inputs[2];
    rknn_output outputs[1];

    memset(inputs, 0, sizeof(inputs));
    memset(outputs, 0, sizeof(outputs));

    // Set Input Data
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_INT64;
    inputs[0].size = MAX_TOKENS * sizeof(int64_t);
    inputs[0].buf = (int64_t *)malloc(inputs[0].size);

    inputs[1].index = 1;
    inputs[1].type = RKNN_TENSOR_FLOAT32;
    inputs[1].size = DECODER_INPUT_SIZE * sizeof(float);
    inputs[1].buf = (float *)malloc(inputs[1].size);
    memcpy(inputs[1].buf, encoder_output, inputs[1].size);

    int64_t tokens[MAX_TOKENS + 1] = {50258, task_code, 50359, 50363}; // tokenizer.sot_sequence_including_notimestamps
    int timestamp_begin = 50364;                                       // tokenizer.timestamp_begin
    int next_token = 50258;                                            // tokenizer.sot
    int end_token = 50257;                                             // tokenizer.eot
    int pop_id = MAX_TOKENS;

    int count = 0; // Avoid getting stuck in a decode loop
    std::string all_token_str = "";

    for (int i = 0; i < MAX_TOKENS / 4; i++)
    {
        memcpy(&tokens[i * 4], tokens, 4 * sizeof(int64_t));
    }

    while (next_token != end_token && count < 1000)
    {
        count++;

        memcpy(inputs[0].buf, tokens, inputs[0].size);

        ret = rknn_inputs_set(app_ctx->rknn_ctx, 2, inputs);
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
        outputs[0].want_float = 1;
        ret = rknn_outputs_get(app_ctx->rknn_ctx, 1, outputs, NULL);
        if (ret < 0)
        {
            printf("rknn_outputs_get fail! ret=%d\n", ret);
            goto out;
        }

        next_token = argmax((float *)outputs[0].buf);

        std::string next_token_str = vocab[next_token].token;
        all_token_str += next_token_str;

        if (next_token > timestamp_begin)
        {
            continue;
        }
        if (pop_id > 4)
        {
            pop_id--;
        }

        tokens[MAX_TOKENS] = next_token;

        for (int j = pop_id; j < MAX_TOKENS; j++)
        {
            tokens[j] = tokens[j + 1];
        }

        // Remeber to release rknn output
        rknn_outputs_release(app_ctx->rknn_ctx, 1, outputs);
    }

    replace_substr(all_token_str, "\u0120", " ");
    replace_substr(all_token_str, "<|endoftext|>", "");
    replace_substr(all_token_str, "\n", "");

    if (all_token_str.size())
    {
        if (task_code == 50260) // TASK_FOR_ZH
        {
            all_token_str = base64_decode(all_token_str);
        }

        recognized_text.push_back(all_token_str);
    }

out:
    for (int i = 0; i < 2; i++)
    {
        if (inputs[i].buf != NULL)
        {
            free(inputs[i].buf);
        }
    }

    return ret;
}

int inference_whisper_model(rknn_whisper_context_t *app_ctx, std::vector<float> audio_data, float *mel_filters, VocabEntry *vocab, int task_code, std::vector<std::string> &recognized_text)
{
    int ret;
    // TIMER timer;
    float *encoder_output = (float *)malloc(ENCODER_OUTPUT_SIZE * sizeof(float));
    recognized_text.clear();

    // timer.tik();
    ret = inference_encoder_model(&app_ctx->encoder_context, audio_data, mel_filters, encoder_output);
    if (ret != 0)
    {
        printf("inference_encoder_model fail! ret=%d\n", ret);
        goto out;
    }
    // timer.tok();
    // timer.print_time("inference_encoder_model");

    // timer.tik();
    ret = inference_decoder_model(&app_ctx->decoder_context, encoder_output, vocab, task_code, recognized_text);
    if (ret != 0)
    {
        printf("inference_decoder_model fail! ret=%d\n", ret);
        goto out;
    }
    // timer.tok();
    // timer.print_time("inference_decoder_model");

out:
    if (encoder_output != NULL)
    {
        free(encoder_output);
    }

    return ret;
}