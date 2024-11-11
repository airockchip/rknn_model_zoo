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
#include "mms_tts.h"
#include "file_utils.h"
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

int init_mms_tts_model(const char *model_path, rknn_app_context_t *app_ctx)
{
    int ret;
    int model_len = 0;
    rknn_context ctx = 0;

    ret = rknn_init(&ctx, (char *)model_path, model_len, 0, NULL);
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

int release_mms_tts_model(rknn_app_context_t *app_ctx)
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

int inference_encoder_model(rknn_app_context_t *app_ctx, std::vector<int64_t> &input_ids, std::vector<int64_t> &attention_mask,
                            std::vector<float> &log_duration, std::vector<float> &input_padding_mask, std::vector<float> &prior_means, std::vector<float> &prior_log_variances)
{
    int ret;
    int n_input = 2;
    int n_output = 4;
    rknn_input inputs[n_input];
    rknn_output outputs[n_output];

    memset(inputs, 0, sizeof(inputs));
    memset(outputs, 0, sizeof(outputs));

    // Set Input Data
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_INT64;
    inputs[0].size = INPUT_IDS_SIZE * sizeof(int64_t);
    inputs[0].buf = (int64_t *)malloc(inputs[0].size);
    memcpy(inputs[0].buf, input_ids.data(), inputs[0].size);

    inputs[1].index = 1;
    inputs[1].type = RKNN_TENSOR_INT64;
    inputs[1].size = ATTENTION_MASK_SIZE * sizeof(int64_t);
    inputs[1].buf = (int64_t *)malloc(inputs[1].size);
    memcpy(inputs[1].buf, attention_mask.data(), inputs[1].size);

    ret = rknn_inputs_set(app_ctx->rknn_ctx, n_input, inputs);
    if (ret < 0)
    {
        printf("rknn_input_set fail! ret=%d\n", ret);
        goto out;
    }

    // Run
    ret = rknn_run(app_ctx->rknn_ctx, NULL);
    if (ret < 0)
    {
        printf("rknn_run fail! ret=%d\n", ret);
        goto out;
    }

    // Get Output
    for (int i = 0; i < n_output; i++)
    {
        outputs[i].want_float = 1;
    }
    ret = rknn_outputs_get(app_ctx->rknn_ctx, n_output, outputs, NULL);
    if (ret < 0)
    {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        goto out;
    }

    memcpy(log_duration.data(), (float *)outputs[0].buf, LOG_DURATION_SIZE * sizeof(float));
    memcpy(input_padding_mask.data(), (float *)outputs[1].buf, INPUT_PADDING_MASK_SIZE * sizeof(float));
    memcpy(prior_means.data(), (float *)outputs[2].buf, PRIOR_MEANS_SIZE * sizeof(float));
    memcpy(prior_log_variances.data(), (float *)outputs[3].buf, PRIOR_LOG_VARIANCES_SIZE * sizeof(float));

out:

    // Remeber to release rknn output
    rknn_outputs_release(app_ctx->rknn_ctx, n_output, outputs);
    for (int i = 0; i < n_input; i++)
    {
        if (inputs[i].buf != NULL)
        {
            free(inputs[i].buf);
        }
    }

    return ret;
}

int inference_decoder_model(rknn_app_context_t *app_ctx, std::vector<float> attn, std::vector<float> output_padding_mask,
                            std::vector<float> prior_means, std::vector<float> prior_log_variances, std::vector<float> &output_wav_data)
{
    int ret;
    int n_input = 4;
    int n_output = 1;
    rknn_input inputs[n_input];
    rknn_output outputs[n_output];

    memset(inputs, 0, sizeof(inputs));
    memset(outputs, 0, sizeof(outputs));

    // Set Input Data
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_FLOAT32;
    inputs[0].size = ATTN_SIZE * sizeof(float);
    inputs[0].buf = (float *)malloc(inputs[0].size);
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    memcpy(inputs[0].buf, attn.data(), inputs[0].size);

    inputs[1].index = 1;
    inputs[1].type = RKNN_TENSOR_FLOAT32;
    inputs[1].size = OUTPUT_PADDING_MASK_SIZE * sizeof(float);
    inputs[1].buf = (float *)malloc(inputs[1].size);
    memcpy(inputs[1].buf, output_padding_mask.data(), inputs[1].size);

    inputs[2].index = 2;
    inputs[2].type = RKNN_TENSOR_FLOAT32;
    inputs[2].size = PRIOR_MEANS_SIZE * sizeof(float);
    inputs[2].buf = (float *)malloc(inputs[2].size);
    memcpy(inputs[2].buf, prior_means.data(), inputs[2].size);

    inputs[3].index = 3;
    inputs[3].type = RKNN_TENSOR_FLOAT32;
    inputs[3].size = PRIOR_LOG_VARIANCES_SIZE * sizeof(float);
    inputs[3].buf = (float *)malloc(inputs[3].size);
    memcpy(inputs[3].buf, prior_log_variances.data(), inputs[3].size);

    ret = rknn_inputs_set(app_ctx->rknn_ctx, n_input, inputs);
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
    ret = rknn_outputs_get(app_ctx->rknn_ctx, n_output, outputs, NULL);
    if (ret < 0)
    {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        goto out;
    }

    memcpy(output_wav_data.data(), (float *)outputs[0].buf, output_wav_data.size() * sizeof(float));

out:

    // Remeber to release rknn output
    rknn_outputs_release(app_ctx->rknn_ctx, n_output, outputs);
    for (int i = 0; i < n_input; i++)
    {
        if (inputs[i].buf != NULL)
        {
            free(inputs[i].buf);
        }
    }

    return ret;
}

int inference_mms_tts_model(rknn_mms_tts_context_t *app_ctx, std::vector<int64_t> &input_ids, std::vector<int64_t> &attention_mask, int &predicted_lengths_max_real, const char *audio_save_path)
{
    int ret;
    TIMER timer;
    std::vector<float> log_duration(LOG_DURATION_SIZE);
    std::vector<float> input_padding_mask(INPUT_PADDING_MASK_SIZE);
    std::vector<float> prior_means(PRIOR_MEANS_SIZE);
    std::vector<float> prior_log_variances(PRIOR_LOG_VARIANCES_SIZE);
    std::vector<float> attn(ATTN_SIZE);
    std::vector<float> output_padding_mask(OUTPUT_PADDING_MASK_SIZE);
    std::vector<float> output_wav_data;

    // timer.tik();
    ret = inference_encoder_model(&app_ctx->encoder_context, input_ids, attention_mask, log_duration, input_padding_mask, prior_means, prior_log_variances);
    if (ret != 0)
    {
        printf("inference_encoder_model fail! ret=%d\n", ret);
        goto out;
    }
    // timer.tok();
    // timer.print_time("inference_encoder_model");

    // timer.tik();
    middle_process(log_duration, input_padding_mask, attn, output_padding_mask, predicted_lengths_max_real);
    // timer.tok();
    // timer.print_time("middle_process");

    // timer.tik();
    output_wav_data.resize(predicted_lengths_max_real * PREDICTED_BATCH);
    ret = inference_decoder_model(&app_ctx->decoder_context, attn, output_padding_mask, prior_means, prior_log_variances, output_wav_data);
    if (ret != 0)
    {
        printf("inference_decoder_model fail! ret=%d\n", ret);
        goto out;
    }
    // timer.tok();
    // timer.print_time("inference_decoder_model");

    // timer.tik();
    ret = save_audio(audio_save_path, output_wav_data.data(), output_wav_data.size(), SAMPLE_RATE, 1);
    if (ret != 0)
    {
        printf("save_audio fail! ret=%d\n", ret);
        goto out;
    }
    // timer.tok();
    // timer.print_time("save_audio");

out:

    return ret;
}