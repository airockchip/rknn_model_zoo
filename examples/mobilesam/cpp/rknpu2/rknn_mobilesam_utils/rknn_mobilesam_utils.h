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


#ifndef _RKNN_DEMO_MOBILESAM_UTILS_H_
#define _RKNN_DEMO_MOBILESAM_UTILS_H_

#include "rknn_api.h"
#include "common.h"

typedef struct {
    rknn_context rknn_ctx;
    rknn_input_output_num io_num;
    rknn_tensor_attr* input_attrs;
    rknn_tensor_attr* output_attrs;

    int model_channel;
    int model_width;
    int model_height;
} rknn_mobilesam_context;

int init_mobilesam_model_utils(rknn_mobilesam_context* mobilesam_ctx, const char* model_path);

int release_mobilesam_model_utils(rknn_mobilesam_context* mobilesam_ctx);

int inference_mobilesam_encoder_utils(rknn_mobilesam_context* mobilesam_ctx, image_buffer_t* img, float* img_embeds);

int inference_mobilesam_decoder_utils(rknn_mobilesam_context* mobilesam_ctx, float* img_embeds, float* point_coords, float* point_labels, float* scores, float* masks);

#endif //_RKNN_DEMO_MOBILESAM_UTILS_H_
