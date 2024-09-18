// Copyright (c) 2023 by Rockchip Electronics Co., Ltd. All Rights Reserved.
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

#include "yolov8.h"
#include "common.h"
#include "file_utils.h"
#include "image_utils.h"

static void dump_tensor_attr(rknn_tensor_attr *attr) {
    char dims[128] = {0};
    for (int i = 0; i < attr->n_dims; ++i) {
        int idx = strlen(dims);
        sprintf(&dims[idx], "%d%s", attr->dims[i], (i == attr->n_dims - 1) ? "" : ", ");
    }
    printf("  index=%d, name=%s, n_dims=%d, dims=[%s], n_elems=%d, size=%d, w_stride = %d, size_with_stride = %d, "
           "fmt=%s, type=%s, qnt_type=%s, "
           "zp=%d, scale=%f\n",
           attr->index, attr->name, attr->n_dims, dims, attr->n_elems, attr->size, attr->w_stride, attr->size_with_stride,
           get_format_string(attr->fmt), get_type_string(attr->type), get_qnt_type_string(attr->qnt_type), attr->zp,
           attr->scale);
}

int init_yolov8_model(const char *model_path, rknn_app_context_t *app_ctx) {
    int ret;
    int model_len = 0;
    char *model;
    rknn_context ctx = 0;

    // Load RKNN Model
    model_len = read_data_from_file(model_path, &model);
    if (model == NULL) {
        printf("load_model fail!\n");
        return -1;
    }

    ret = rknn_init(&ctx, model, model_len, 0, NULL);
    free(model);
    if (ret < 0) {
        printf("rknn_init fail! ret=%d\n", ret);
        return -1;
    }

    // Get Model Input Output Number
    rknn_input_output_num io_num;
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC) {
        printf("rknn_query fail! ret=%d\n", ret);
        return -1;
    }
    printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

    // Get Model Input Info
    printf("input tensors:\n");
    rknn_tensor_attr input_native_attrs[io_num.n_input];
    memset(input_native_attrs, 0, sizeof(input_native_attrs));
    for (int i = 0; i < io_num.n_input; i++) {
        input_native_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_NATIVE_INPUT_ATTR, &(input_native_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(input_native_attrs[i]));
    }

    // default input type is int8 (normalize and quantize need compute in outside)
    // if set uint8, will fuse normalize and quantize to npu
    input_native_attrs[0].type = RKNN_TENSOR_UINT8;
    app_ctx->input_mems[0] = rknn_create_mem(ctx, input_native_attrs[0].size_with_stride);

    // Set input tensor memory
    ret = rknn_set_io_mem(ctx, app_ctx->input_mems[0], &input_native_attrs[0]);
    if (ret < 0) {
        printf("input_mems rknn_set_io_mem fail! ret=%d\n", ret);
        return -1;
    }

    // Get Model Output Info
    printf("output tensors:\n");
    rknn_tensor_attr output_native_attrs[io_num.n_output];
    memset(output_native_attrs, 0, sizeof(output_native_attrs));
    for (int i = 0; i < io_num.n_output; i++) {
        output_native_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_NATIVE_OUTPUT_ATTR, &(output_native_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(output_native_attrs[i]));
    }

    // Set output tensor memory
    for (uint32_t i = 0; i < io_num.n_output; ++i) {
        app_ctx->output_mems[i] = rknn_create_mem(ctx, output_native_attrs[i].size_with_stride);
        ret = rknn_set_io_mem(ctx, app_ctx->output_mems[i], &output_native_attrs[i]);
        if (ret < 0) {
            printf("output_mems rknn_set_io_mem fail! ret=%d\n", ret);
            return -1;
        }
    }

    // Set to context
    app_ctx->rknn_ctx = ctx;

    // TODO
    if (output_native_attrs[0].qnt_type == RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC && output_native_attrs[0].type == RKNN_TENSOR_INT8) {
        app_ctx->is_quant = true;
    } else {
        app_ctx->is_quant = false;
    }

    rknn_tensor_attr input_attrs[io_num.n_input];
    memset(input_attrs, 0, sizeof(input_attrs));
    for (int i = 0; i < io_num.n_input; i++) {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
    }

    rknn_tensor_attr output_attrs[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs));
    for (int i = 0; i < io_num.n_output; i++) {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
    }

    app_ctx->io_num = io_num;
    app_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
    app_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

    app_ctx->input_native_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->input_native_attrs, input_native_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
    app_ctx->output_native_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->output_native_attrs, output_native_attrs, io_num.n_output * sizeof(rknn_tensor_attr));


    if (input_attrs[0].fmt == RKNN_TENSOR_NCHW) {
        printf("model is NCHW input fmt\n");
        app_ctx->model_channel = input_attrs[0].dims[1];
        app_ctx->model_height = input_attrs[0].dims[2];
        app_ctx->model_width = input_attrs[0].dims[3];
    } else {
        printf("model is NHWC input fmt\n");
        app_ctx->model_height = input_attrs[0].dims[1];
        app_ctx->model_width = input_attrs[0].dims[2];
        app_ctx->model_channel = input_attrs[0].dims[3];
    }
    printf("model input height=%d, width=%d, channel=%d\n",
           app_ctx->model_height, app_ctx->model_width, app_ctx->model_channel);

    return 0;
}

int NC1HWC2_i8_to_NCHW_i8(const int8_t *src, int8_t *dst, int *dims, int channel, int h, int w, int zp, float scale) {
    int batch  = dims[0];
    int C1     = dims[1];
    int C2     = dims[4];
    int hw_src = dims[2] * dims[3];
    int hw_dst = h * w;
    for (int i = 0; i < batch; i++) {
        const int8_t *src_b = src + i * C1 * hw_src * C2;
        int8_t        *dst_b = dst + i * channel * hw_dst;
        for (int c = 0; c < channel; ++c) {
            int           plane  = c / C2;
            const int8_t *src_bc = plane * hw_src * C2 + src_b;
            int           offset = c % C2;
            for (int cur_h = 0; cur_h < h; ++cur_h)
                for (int cur_w = 0; cur_w < w; ++cur_w) {
                    int cur_hw                 = cur_h * w + cur_w;
                    dst_b[c * hw_dst + cur_hw] = src_bc[C2 * cur_hw + offset] ; // int8-->int8
                }
        }
    }

    return 0;
}

int release_yolov8_model(rknn_app_context_t *app_ctx) {
    int ret;
    if (app_ctx->input_attrs != NULL) {
        free(app_ctx->input_attrs);
        app_ctx->input_attrs = NULL;
    }
    if (app_ctx->output_attrs != NULL) {
        free(app_ctx->output_attrs);
        app_ctx->output_attrs = NULL;
    }
    if (app_ctx->input_native_attrs != NULL) {
        free(app_ctx->input_native_attrs);
        app_ctx->input_native_attrs = NULL;
    }
    if (app_ctx->output_native_attrs != NULL) {
        free(app_ctx->output_native_attrs);
        app_ctx->output_native_attrs = NULL;
    }

    for (int i = 0; i < app_ctx->io_num.n_input; i++) {
        if (app_ctx->input_mems[i] != NULL) {
            ret = rknn_destroy_mem(app_ctx->rknn_ctx, app_ctx->input_mems[i]);
            if (ret != RKNN_SUCC) {
                printf("rknn_destroy_mem fail! ret=%d\n", ret);
                return -1;
            }
        }
    }
    for (int i = 0; i < app_ctx->io_num.n_output; i++) {
        if (app_ctx->output_mems[i] != NULL) {
            ret = rknn_destroy_mem(app_ctx->rknn_ctx, app_ctx->output_mems[i]);
            if (ret != RKNN_SUCC) {
                printf("rknn_destroy_mem fail! ret=%d\n", ret);
                return -1;
            }
        }
    }
    if (app_ctx->rknn_ctx != 0) {
        ret = rknn_destroy(app_ctx->rknn_ctx);
        if (ret != RKNN_SUCC) {
            printf("rknn_destroy fail! ret=%d\n", ret);
            return -1;
        }
        app_ctx->rknn_ctx = 0;

    }
    return 0;
}

int inference_yolov8_model(rknn_app_context_t *app_ctx, image_buffer_t *img, object_detect_result_list *od_results) {
    int ret;
    image_buffer_t dst_img;
    letterbox_t letter_box;
    const float nms_threshold = NMS_THRESH;      // 默认的NMS阈值
    const float box_conf_threshold = BOX_THRESH; // 默认的置信度阈值
    int bg_color = 114;

    if ((!app_ctx) || !(img) || (!od_results)) {
        return -1;
    }

    memset(od_results, 0x00, sizeof(*od_results));
    memset(&letter_box, 0, sizeof(letterbox_t));
    memset(&dst_img, 0, sizeof(image_buffer_t));

    // Pre Process
    dst_img.width = app_ctx->model_width;
    dst_img.height = app_ctx->model_height;
    dst_img.format = IMAGE_FORMAT_RGB888;
    dst_img.size = get_image_size(&dst_img);
    dst_img.fd = app_ctx->input_mems[0]->fd;
    dst_img.virt_addr = (unsigned char*)app_ctx->input_mems[0]->virt_addr;

    if (dst_img.virt_addr == NULL && dst_img.fd == 0) {
        printf("malloc buffer size:%d fail!\n", dst_img.size);
        return -1;
    }

    // letterbox
    ret = convert_image_with_letterbox(img, &dst_img, &letter_box, bg_color);
    if (ret < 0) {
        printf("convert_image_with_letterbox fail! ret=%d\n", ret);
        return -1;
    }

    // Run
    printf("rknn_run\n");
    ret = rknn_run(app_ctx->rknn_ctx, nullptr);
    if (ret < 0) {
        printf("rknn_run fail! ret=%d\n", ret);
        return -1;
    }

    //NC1HWC2 to NCHW
    rknn_output outputs[app_ctx->io_num.n_output];
    memset(outputs, 0, sizeof(outputs));
    for (uint32_t i = 0; i < app_ctx->io_num.n_output; i++) {
        int   channel = app_ctx->output_attrs[i].dims[1];
        int   h       = app_ctx->output_attrs[i].n_dims > 2 ? app_ctx->output_attrs[i].dims[2] : 1;
        int   w       = app_ctx->output_attrs[i].n_dims > 3 ? app_ctx->output_attrs[i].dims[3] : 1;
        int   hw      = h * w;
        int   zp      = app_ctx->output_native_attrs[i].zp;
        float scale   = app_ctx->output_native_attrs[i].scale;
        if (app_ctx->is_quant) {
            outputs[i].size = app_ctx->output_native_attrs[i].n_elems * sizeof(int8_t);
            outputs[i].buf = (int8_t *)malloc(outputs[i].size);
            if (app_ctx->output_native_attrs[i].fmt == RKNN_TENSOR_NC1HWC2) {
                NC1HWC2_i8_to_NCHW_i8((int8_t *)app_ctx->output_mems[i]->virt_addr, (int8_t *)outputs[i].buf,
                                      (int *)app_ctx->output_native_attrs[i].dims, channel, h, w, zp, scale);
            } else {
                memcpy(outputs[i].buf, app_ctx->output_mems[i]->virt_addr, outputs[i].size);
            }
        } else {
            printf("Currently zero copy does not support fp16!\n");
            goto out;
        }
    }

    // Post Process
    post_process(app_ctx, outputs, &letter_box, box_conf_threshold, nms_threshold, od_results);

    for (int i = 0; i < app_ctx->io_num.n_output; i++) {
        free(outputs[i].buf);
    }

out:
    return ret;
}