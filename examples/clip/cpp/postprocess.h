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

#ifndef _RKNN_DEMO_CLIP_POSTPROCESS_H_
#define _RKNN_DEMO_CLIP_POSTPROCESS_H_


typedef struct {
    int img_index;
    int text_index;
    float score;
} clip_res;

int post_process(rknn_app_context_t* app_ctx, float* img_output, float* text_output, clip_res* out_res);

#endif // _RKNN_DEMO_CLIP_POSTPROCESS_H_