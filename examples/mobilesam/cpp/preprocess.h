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

#ifndef _RKNN_DEMO_MOBILESAM_PREPROCESS_H_
#define _RKNN_DEMO_MOBILESAM_PREPROCESS_H_

#include "image_utils.h"
#include <opencv2/opencv.hpp>

#define IMG_SIZE 448

float* read_coords_from_file(const char* path, int* line_count);

int get_preprocess_shape(int ori_heigth, int ori_width, int* new_shape);
int point_coords_preprocess(float* ori_point_coords, int coords_size, int ori_height, int ori_width, float* cvt_point_coords);
int pre_process(image_buffer_t* src_img, image_buffer_t* dst_img);

#endif // _RKNN_DEMO_MOBILESAM_PREPROCESS_H_
