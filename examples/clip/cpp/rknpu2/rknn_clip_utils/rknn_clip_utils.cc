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

#include "rknn_clip_utils.h"
#include "common.h"
#include "file_utils.h"
#include "image_utils.h"


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


int init_clip_model_utils(rknn_clip_context* clip_ctx, const char* model_path)
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

    if (input_attrs[0].fmt == RKNN_TENSOR_NCHW)
    {
        printf("model is NCHW input fmt\n");
        clip_ctx->model_channel = input_attrs[0].dims[1];
        clip_ctx->model_height = input_attrs[0].dims[2];
        clip_ctx->model_width = input_attrs[0].dims[3];
        printf("input image height=%d, input image width=%d, input image channel=%d\n",
                clip_ctx->model_height, clip_ctx->model_width, clip_ctx->model_channel);
    }
    else if (input_attrs[0].fmt == RKNN_TENSOR_NHWC)
    {
        printf("model is NHWC input fmt\n");
        clip_ctx->model_height = input_attrs[0].dims[1];
        clip_ctx->model_width = input_attrs[0].dims[2];
        clip_ctx->model_channel = input_attrs[0].dims[3];
        printf("input image height=%d, input image width=%d, input image channel=%d\n",
                clip_ctx->model_height, clip_ctx->model_width, clip_ctx->model_channel);
    }
    else
    {
        printf("model is UNDEFINED input fmt\n");
        clip_ctx->model_height = input_attrs[0].dims[0];
        clip_ctx->model_width = input_attrs[0].dims[1];
        printf("input text batch size=%d, input sequence length=%d\n", 
                clip_ctx->model_height, clip_ctx->model_width);
    }

    return 0;
}

int release_clip_model_utils(rknn_clip_context* clip_ctx)
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

int inference_clip_image_model_utils(rknn_clip_context* clip_ctx, image_buffer_t* img, float img_output[])
{
    int ret;
    image_buffer_t dst_img;
    rknn_input inputs[1];
    rknn_output outputs[1];

    if ((!clip_ctx) || (!img))
    {
        printf("clip_ctx or img is NULL");
        return -1;
    }

    memset(&dst_img, 0, sizeof(image_buffer_t));
    memset(inputs, 0, sizeof(inputs));
    memset(outputs, 0, sizeof(outputs));

    // Pre Process
    dst_img.width = clip_ctx->model_width;
    dst_img.height = clip_ctx->model_height;
    dst_img.format = IMAGE_FORMAT_RGB888;
    dst_img.size = get_image_size(&dst_img);
    dst_img.virt_addr = (unsigned char *)malloc(dst_img.size);
    if (dst_img.virt_addr == NULL)
    {
        printf("malloc buffer size:%d fail!\n", dst_img.size);
        return -1;
    }

    // center crop
    if (img->width < CROP_SIZE || img->height < CROP_SIZE)
    {
        ret = convert_image(img, &dst_img, NULL, NULL, 0);
    }
    else
    {
        image_rect_t src_box;
        memset(&src_box, 0, sizeof(image_rect_t));
        src_box.left = (img->width - CROP_SIZE) / 2;
        src_box.top = (img->height - CROP_SIZE) / 2;
        src_box.right = src_box.left + CROP_SIZE - 1;
        src_box.bottom = src_box.top + CROP_SIZE - 1;
        ret = convert_image(img, &dst_img, &src_box, NULL, 0);
    }
    if (ret < 0)
    {
        printf("convert_image fail! ret=%d\n", ret);
        goto out;
    }

    // Set Input Data
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].size = clip_ctx->model_width * clip_ctx->model_height * clip_ctx->model_channel;
    inputs[0].buf = dst_img.virt_addr;

    ret = rknn_inputs_set(clip_ctx->rknn_ctx, clip_ctx->io_num.n_input, inputs);
    if (ret < 0)
    {
        printf("rknn_input_set fail! ret=%d\n", ret);
        goto out;
    }

    // Run
    printf("rknn_run\n");
    ret = rknn_run(clip_ctx->rknn_ctx, nullptr);
    if (ret < 0)
    {
        printf("rknn_run fail! ret=%d\n", ret);
        goto out;
    }

    // Get Output
    outputs[0].want_float = 1;
    ret = rknn_outputs_get(clip_ctx->rknn_ctx, 1, outputs, NULL);
    if (ret < 0)
    {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        goto out;
    }

    memcpy(img_output, (float*)outputs[0].buf, outputs[0].size);

    // Remeber to release rknn output
    rknn_outputs_release(clip_ctx->rknn_ctx, 1, outputs);

out:
    if (dst_img.virt_addr != NULL)
    {
        free(dst_img.virt_addr);
    }

    return ret;

}

int inference_clip_text_model_utils(rknn_clip_context* clip_ctx, int* tokens, float text_output[])
{
    int ret;
    rknn_input inputs[1];
    rknn_output outputs[1];

    if ((!clip_ctx) || (!tokens))
    {
        printf("clip_ctx or tokens is NULL");
        return -1;
    }

    memset(inputs, 0, sizeof(inputs));
    memset(outputs, 0, sizeof(outputs));

    // Set Input Data
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_INT32;
    inputs[0].fmt = RKNN_TENSOR_UNDEFINED;
    inputs[0].size = clip_ctx->model_width * clip_ctx->model_height * sizeof(int32_t);
    inputs[0].buf = tokens;

    ret = rknn_inputs_set(clip_ctx->rknn_ctx, clip_ctx->io_num.n_input, inputs);
    if (ret < 0)
    {
        printf("rknn_input_set fail! ret=%d\n", ret);
        return -1;
    }

    // Run
    printf("rknn_run\n");
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
        return -1;
    }

    memcpy(text_output, (float*)outputs[0].buf, outputs[0].size);

    // Remeber to release rknn output
    rknn_outputs_release(clip_ctx->rknn_ctx, 1, outputs);

    return ret;
}