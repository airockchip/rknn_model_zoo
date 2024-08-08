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

#ifndef _RKNN_DEMO_YAMNET_H_
#define _RKNN_DEMO_YAMNET_H_

#include "rknn_api.h"
#include "audio_utils.h"
#include <iostream>
#include <vector>
#include "process.h"

typedef struct
{
    rknn_context rknn_ctx;
    rknn_input_output_num io_num;
    rknn_tensor_attr *input_attrs;
    rknn_tensor_attr *output_attrs;
} rknn_app_context_t;

int init_yamnet_model(const char *model_path, rknn_app_context_t *app_ctx);
int release_yamnet_model(rknn_app_context_t *app_ctx);
int inference_yamnet_model(rknn_app_context_t *app_ctx, audio_buffer_t *audio, LabelEntry *label, ResultEntry *result);

#endif //_RKNN_DEMO_YAMNET_H_