/*
 * Copyright (C) 2016 Rockchip Electronics Co., Ltd.
 * Authors:
 *  Zhiqin Wei <wzq@rock-chips.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _rga_utils_h_
#define _rga_utils_h_

// -------------------------------------------------------------------------------
float get_bpp_from_format(int format);
int get_perPixel_stride_from_format(int format);
int get_buf_from_file(void *buf, int f, int sw, int sh, int index);
int output_buf_data_to_file(void *buf, int f, int sw, int sh, int index);
const char *translate_format_str(int format);
int get_buf_from_file_FBC(void *buf, int f, int sw, int sh, int index);
int output_buf_data_to_file_FBC(void *buf, int f, int sw, int sh, int index);
#endif

