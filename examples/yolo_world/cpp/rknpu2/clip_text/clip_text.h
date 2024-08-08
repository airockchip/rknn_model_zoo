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


#ifndef _RKNN_DEMO_CLIP_TEXT_H_
#define _RKNN_DEMO_CLIP_TEXT_H_

#include "rknn_api.h"

typedef struct {
    rknn_context rknn_ctx;
    rknn_input_output_num io_num;
    rknn_tensor_attr* input_attrs;
    rknn_tensor_attr* output_attrs;
} rknn_clip_context;

int init_clip_text_model(rknn_clip_context* clip_ctx, const char* model_path);

int release_clip_text_model(rknn_clip_context* clip_ctx);

int inference_clip_text_model(rknn_clip_context* clip_ctx, char** input_texts, int text_num, float text_output[]);

#endif //_RKNN_DEMO_CLIP_TEXT_H_
