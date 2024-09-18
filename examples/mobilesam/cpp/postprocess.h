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

#ifndef _RKNN_DEMO_MOBILESAM_POSTPROCESS_H_
#define _RKNN_DEMO_MOBILESAM_POSTPROCESS_H_

typedef struct {
    uint8_t* mask;
    float score;
} mobilesam_res;

typedef struct {
    int x1;
    int y1;
    int x2;
    int y2;
} mobilesam_box;

int post_process(rknn_app_context_t* app_ctx, float* iou_predictions, float* low_res_masks, mobilesam_res* res, int ori_height, int ori_width);

void draw_mask(image_buffer_t* src_imag, uint8_t* mask);

void rknn_nchw_2_nhwc(float* nchw, float* nhwc, int N, int C, int H, int W);

#endif // _RKNN_DEMO_MOBILESAM_POSTPROCESS_H_
