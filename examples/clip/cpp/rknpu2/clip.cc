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

#include "clip.h"
#include "common.h"
#include "file_utils.h"
#include "image_utils.h"


int init_clip_model(const char* img_model_path, const char* text_model_path, rknn_app_context_t* app_ctx)
{
    int ret;

    printf("--> init clip image model\n");
    ret = init_clip_model_utils(&(app_ctx->img), img_model_path);
    if (ret < 0)
    {
        printf("rknn_init clip image model fail! ret=%d\n", ret);
        return -1;
    }

    printf("--> init clip text model\n");
    ret = init_clip_model_utils(&(app_ctx->text), text_model_path);
    if (ret < 0)
    {
        printf("rknn_init clip text model fail! ret=%d\n", ret);
        return -1;
    }

    app_ctx->clip_tokenize = new CLIPTokenizer();

    return 0;
}

int release_clip_model(rknn_app_context_t* app_ctx)
{
    release_clip_model_utils(&(app_ctx->img));
    release_clip_model_utils(&(app_ctx->text));
    delete app_ctx->clip_tokenize;

    return 0;

}

int inference_clip_model(rknn_app_context_t* app_ctx, image_buffer_t* img, char** input_texts, int text_num, clip_res* out_res)
{
    int ret;
    int* tokens;

    if ((!app_ctx) || (!img))
    {
        printf("app_ctx or img is NULL");
        return -1;
    }

    if (text_num > MAX_TEXT_NUM)
    {
        text_num = MAX_TEXT_NUM;
        printf("Input text num overlimit, modify text num == %d", MAX_TEXT_NUM);
    }
    int sequence_len = app_ctx->text.input_attrs[0].dims[1];
    int tokens_num = text_num * sequence_len;
    tokens = (int*)malloc(tokens_num * sizeof(int));

    float img_output[app_ctx->img.output_attrs[0].dims[0] * app_ctx->img.output_attrs[0].dims[1]];
    float text_output[text_num * app_ctx->text.output_attrs[0].dims[1]];
    if (app_ctx->img.output_attrs[0].dims[1] != app_ctx->text.output_attrs[0].dims[1])
    {   
        printf("The dimensions of the img and text model output are not the same! Please confirm that are consistent");
        exit(-1);
    }
    memset(img_output, 0, sizeof(img_output));
    memset(text_output, 0, sizeof(text_output));

    app_ctx->input_img_num = 1;
    app_ctx->input_text_num = text_num;

    for (int i = 0; i < text_num; i++)
    {
        std::vector<int> token = app_ctx->clip_tokenize->tokenize(input_texts[i], sequence_len, true);
        for (int j = 0; j < token.size(); j++)
        {
            tokens[i*sequence_len+j] = token[j];
        }
    }

    printf("--> inference clip image model\n");
    ret = inference_clip_image_model_utils(&(app_ctx->img), img, img_output);
    if (ret != 0)
    {
        printf("inference clip image model fail! ret=%d\n", ret);
        goto out;
    }

    printf("--> inference clip text model\n");
    for (int i = 0; i < text_num; i++)
    {   
        ret = inference_clip_text_model_utils(&(app_ctx->text), tokens + (i*sequence_len), text_output + (i*app_ctx->text.output_attrs[0].dims[1]));
        if (ret != 0)
        {
            printf("inference clip text model fail! ret=%d\n", ret);
            goto out;
        }
    }

    // Post Process
    post_process(app_ctx, img_output, text_output, out_res);

out:
    if (tokens != NULL)
    {
        free(tokens);
    }

    return ret;
}