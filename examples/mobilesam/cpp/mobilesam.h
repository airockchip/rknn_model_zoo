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


#ifndef _RKNN_DEMO_MOBILESAM_H_
#define _RKNN_DEMO_MOBILESAM_H_

#include "rknn_api.h"
#include "common.h"
#include "rknn_mobilesam_utils.h"
#include "preprocess.h"

typedef struct {
    rknn_mobilesam_context encoder;
    rknn_mobilesam_context decoder;
} rknn_app_context_t;


#include "postprocess.h"

int init_mobilesam_model(const char* encoder_model_path, 
                         const char* decoder_model_path,
                         rknn_app_context_t* app_ctx);

int release_mobilesam_model(rknn_app_context_t* app_ctx);

int inference_mobilesam_model(rknn_app_context_t* app_ctx, image_buffer_t* img, float* point_coords, float* point_labels, mobilesam_res* res);

#endif //_RKNN_DEMO_MOBILESAM_H_
