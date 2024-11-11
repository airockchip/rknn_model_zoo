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

#ifndef _RKNN_DEMO_MMS_TTS_H_
#define _RKNN_DEMO_MMS_TTS_H_

#include "rknn_api.h"
#include <iostream>
#include <vector>
#include <string>
#include "process.h"

typedef struct
{
    rknn_context rknn_ctx;
    rknn_input_output_num io_num;
    rknn_tensor_attr *input_attrs;
    rknn_tensor_attr *output_attrs;
} rknn_app_context_t;

typedef struct
{
    rknn_app_context_t encoder_context;
    rknn_app_context_t decoder_context;
} rknn_mms_tts_context_t;

int init_mms_tts_model(const char *model_path, rknn_app_context_t *app_ctx);
int release_mms_tts_model(rknn_app_context_t *app_ctx);
int inference_mms_tts_model(rknn_mms_tts_context_t *app_ctx, std::vector<int64_t> &input_ids, std::vector<int64_t> &attention_mask, int &predicted_lengths_max_real, const char *audio_save_path);

#endif //_RKNN_DEMO_MMS_TTS_H_