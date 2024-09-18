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

#include "mobilesam.h"
#include "common.h"
#include "file_utils.h"
#include "image_utils.h"


int init_mobilesam_model(const char* encoder_model_path, const char* decoder_model_path, rknn_app_context_t* app_ctx)
{
    int ret;

    printf("--> init mobilesam encoder model\n");
    ret = init_mobilesam_model_utils(&(app_ctx->encoder), encoder_model_path);
    if (ret < 0)
    {
        printf("rknn_init mobilesam encoder model fail! ret=%d\n", ret);
        return -1;
    }

    printf("--> init mobilesam decoder model\n");
    ret = init_mobilesam_model_utils(&(app_ctx->decoder), decoder_model_path);
    if (ret < 0)
    {
        printf("rknn_init mobilesam decoder model fail! ret=%d\n", ret);
        return -1;
    };

    return 0;
}

int release_mobilesam_model(rknn_app_context_t* app_ctx)
{
    release_mobilesam_model_utils(&(app_ctx->encoder));
    release_mobilesam_model_utils(&(app_ctx->decoder));

    return 0;

}

int inference_mobilesam_model(rknn_app_context_t* app_ctx, image_buffer_t* img, float* point_coords, float* point_labels, mobilesam_res* res)
{
    int ret;
    int* tokens;

    if ((!app_ctx) || (!img))
    {
        printf("app_ctx or img is NULL");
        return -1;
    }

    int img_embeds_size = app_ctx->encoder.output_attrs[0].n_elems;
    float* img_embeds_nchw = (float*)malloc(img_embeds_size * sizeof(float));
    float* img_embeds_nhwc = (float*)malloc(img_embeds_size * sizeof(float));
    float iou_predictions[app_ctx->decoder.output_attrs[0].n_elems];
    float low_res_masks[app_ctx->decoder.output_attrs[1].n_elems];

    memset(img_embeds_nchw, 0, img_embeds_size * sizeof(float));
    memset(img_embeds_nhwc, 0, img_embeds_size * sizeof(float));
    memset(iou_predictions, 0, sizeof(iou_predictions));
    memset(low_res_masks, 0, sizeof(low_res_masks));
    memset(res, 0, sizeof(mobilesam_res));

    printf("--> inference mobilesam encoder model\n");
    ret = inference_mobilesam_encoder_utils(&(app_ctx->encoder), img, img_embeds_nchw);
    if (ret != 0)
    {
        printf("inference mobilesam encoder model fail! ret=%d\n", ret);
        return -1;
    }

    rknn_nchw_2_nhwc(img_embeds_nchw, img_embeds_nhwc, app_ctx->encoder.output_attrs[0].dims[0], app_ctx->encoder.output_attrs[0].dims[1],
                                                       app_ctx->encoder.output_attrs[0].dims[2], app_ctx->encoder.output_attrs[0].dims[3]);

    printf("--> inference mobilesam decoder model\n");
    ret = inference_mobilesam_decoder_utils(&(app_ctx->decoder), img_embeds_nhwc, point_coords, point_labels, iou_predictions, low_res_masks);
    if (ret != 0)
    {
        printf("inference mobilesam decoder model fail! ret=%d\n", ret);
       return -1;
    }

    // Post Process
    post_process(app_ctx, iou_predictions, low_res_masks, res, img->height, img->width);

    if (img_embeds_nchw != NULL)
    {
        free(img_embeds_nchw);
    }

    if (img_embeds_nhwc != NULL)
    {
        free(img_embeds_nhwc);
    }

    return ret;
}
