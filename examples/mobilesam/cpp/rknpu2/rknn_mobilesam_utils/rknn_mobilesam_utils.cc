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

#include "rknn_mobilesam_utils.h"
#include "common.h"
#include "file_utils.h"
#include "image_utils.h"
#include "preprocess.h"


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


int init_mobilesam_model_utils(rknn_mobilesam_context* mobilesam_ctx, const char* model_path)
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
    mobilesam_ctx->rknn_ctx = ctx;
    mobilesam_ctx->io_num = io_num;
    mobilesam_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
    memcpy(mobilesam_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
    mobilesam_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
    memcpy(mobilesam_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

    if (input_attrs[0].fmt == RKNN_TENSOR_NCHW)
    {
        printf("model is NCHW input fmt\n");
        mobilesam_ctx->model_channel = input_attrs[0].dims[1];
        mobilesam_ctx->model_height = input_attrs[0].dims[2];
        mobilesam_ctx->model_width = input_attrs[0].dims[3];
    }
    else
    {
        printf("model is NHWC input fmt\n");
        mobilesam_ctx->model_height = input_attrs[0].dims[1];
        mobilesam_ctx->model_width = input_attrs[0].dims[2];
        mobilesam_ctx->model_channel = input_attrs[0].dims[3];
    }
    printf("input image height=%d, input image width=%d, input image channel=%d\n",
        mobilesam_ctx->model_height, mobilesam_ctx->model_width, mobilesam_ctx->model_channel);

    return 0;
}

int release_mobilesam_model_utils(rknn_mobilesam_context* mobilesam_ctx)
{
    if (mobilesam_ctx->input_attrs != NULL)
    {
        free(mobilesam_ctx->input_attrs);
        mobilesam_ctx->input_attrs = NULL;
    }
    if (mobilesam_ctx->output_attrs != NULL)
    {
        free(mobilesam_ctx->output_attrs);
        mobilesam_ctx->output_attrs = NULL;
    }
    if (mobilesam_ctx->rknn_ctx != 0)
    {
        rknn_destroy(mobilesam_ctx->rknn_ctx);
        mobilesam_ctx->rknn_ctx = 0;
    }
    return 0;
}

int inference_mobilesam_encoder_utils(rknn_mobilesam_context* mobilesam_ctx, image_buffer_t* img, float* img_embeds)
{
    int ret;
    image_buffer_t dst_img;
    rknn_input inputs[mobilesam_ctx->io_num.n_input];
    rknn_output outputs[mobilesam_ctx->io_num.n_output];

    if ((!mobilesam_ctx) || (!img))
    {
        printf("mobilesam_ctx or img is NULL");
        return -1;
    }

    memset(&dst_img, 0, sizeof(image_buffer_t));
    memset(inputs, 0, sizeof(inputs));
    memset(outputs, 0, sizeof(outputs));

    // Pre Process
    dst_img.width = mobilesam_ctx->model_width;
    dst_img.height = mobilesam_ctx->model_height;
    dst_img.format = IMAGE_FORMAT_RGB888;
    dst_img.size = get_image_size(&dst_img);
    dst_img.virt_addr = (unsigned char *)malloc(dst_img.size);
    if (dst_img.virt_addr == NULL)
    {
        printf("malloc buffer size:%d fail!\n", dst_img.size);
        return -1;
    }

    pre_process(img, &dst_img);

    // Set Input Data
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].size = mobilesam_ctx->model_width * mobilesam_ctx->model_height * mobilesam_ctx->model_channel;
    inputs[0].buf = dst_img.virt_addr;

    ret = rknn_inputs_set(mobilesam_ctx->rknn_ctx, mobilesam_ctx->io_num.n_input, inputs);
    if (ret < 0)
    {
        printf("rknn_input_set fail! ret=%d\n", ret);
        goto out;
    }

    // Run
    printf("rknn_run\n");
    ret = rknn_run(mobilesam_ctx->rknn_ctx, nullptr);
    if (ret < 0)
    {
        printf("rknn_run fail! ret=%d\n", ret);
        goto out;
    }

    // Get Output
    outputs[0].want_float = 1;
    ret = rknn_outputs_get(mobilesam_ctx->rknn_ctx, 1, outputs, NULL);
    if (ret < 0)
    {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        goto out;
    }

    memcpy(img_embeds, (float*)outputs[0].buf, outputs[0].size);

    // Remeber to release rknn output
    rknn_outputs_release(mobilesam_ctx->rknn_ctx, 1, outputs);

out:
    if (dst_img.virt_addr != NULL)
    {
        free(dst_img.virt_addr);
    }

    return ret;

}

int inference_mobilesam_decoder_utils(rknn_mobilesam_context* mobilesam_ctx, float* img_embeds, float* point_coords, float* point_labels, float* scores, float* masks)
{
    int ret;
    rknn_input inputs[mobilesam_ctx->io_num.n_input];
    rknn_output outputs[mobilesam_ctx->io_num.n_output];
    float mask_input[mobilesam_ctx->input_attrs[3].n_elems];
    float has_mask_input[mobilesam_ctx->input_attrs[4].n_elems];

    if ((!mobilesam_ctx) || (!img_embeds) || (!point_coords) || (!point_labels))
    {
        printf("mobilesam_ctx or mobilesam decoder input is NULL");
        return -1;
    }

    // To-Do:支持mask_input
    memset(inputs, 0, sizeof(inputs));
    memset(outputs, 0, sizeof(outputs));
    memset(mask_input, 0, sizeof(mask_input));
    memset(has_mask_input, 0, sizeof(has_mask_input));

    // Set Input Data
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_FLOAT32;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].size = mobilesam_ctx->input_attrs[0].n_elems * sizeof(uint32_t);
    inputs[0].buf = img_embeds;

    inputs[1].index = 1;
    inputs[1].type = RKNN_TENSOR_FLOAT32;
    inputs[1].fmt = RKNN_TENSOR_UNDEFINED;
    inputs[1].size = mobilesam_ctx->input_attrs[1].n_elems * sizeof(uint32_t);
    inputs[1].buf = point_coords;

    inputs[2].index = 2;
    inputs[2].type = RKNN_TENSOR_FLOAT32;
    inputs[2].fmt = RKNN_TENSOR_UNDEFINED;
    inputs[2].size = mobilesam_ctx->input_attrs[2].n_elems * sizeof(uint32_t);
    inputs[2].buf = point_labels;

    inputs[3].index = 3;
    inputs[3].type = RKNN_TENSOR_FLOAT32;
    inputs[3].fmt = RKNN_TENSOR_NHWC;
    inputs[3].size = mobilesam_ctx->input_attrs[3].n_elems * sizeof(uint32_t);
    inputs[3].buf = mask_input;

    inputs[4].index = 4;
    inputs[4].type = RKNN_TENSOR_FLOAT32;
    inputs[4].fmt = RKNN_TENSOR_UNDEFINED;
    inputs[4].size = mobilesam_ctx->input_attrs[4].n_elems * sizeof(uint32_t);
    inputs[4].buf = has_mask_input;

    ret = rknn_inputs_set(mobilesam_ctx->rknn_ctx, mobilesam_ctx->io_num.n_input, inputs);
    if (ret < 0)
    {
        printf("rknn_input_set fail! ret=%d\n", ret);
        return -1;
    }

    // Run
    printf("rknn_run\n");
    ret = rknn_run(mobilesam_ctx->rknn_ctx, NULL);
    if (ret < 0)
    {
        printf("rknn_run fail! ret=%d\n", ret);
        return -1;
    }

    // Get Output
    for (int i = 0; i < mobilesam_ctx->io_num.n_output; i++)
    {
        outputs[i].index = i;
        outputs[i].want_float = 1;
    }
    ret = rknn_outputs_get(mobilesam_ctx->rknn_ctx, mobilesam_ctx->io_num.n_output, outputs, NULL);
    if (ret < 0)
    {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        return -1;
    }

    memcpy(scores, (float*)outputs[0].buf, outputs[0].size);
    memcpy(masks, (float*)outputs[1].buf, outputs[1].size);

    // Remeber to release rknn output
    rknn_outputs_release(mobilesam_ctx->rknn_ctx, 1, outputs);

    return ret;
}
