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

#ifndef _rk_graphic_buffer_h_
#define _rk_graphic_buffer_h_

#ifdef ANDROID

#include <stdint.h>
#include <vector>
#include <sys/types.h>

#include <system/graphics.h>

#include <utils/Thread.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include <sys/mman.h>
#include <linux/stddef.h>

#include <utils/Atomic.h>
#include <utils/Errors.h>
#include <android/log.h>
#include <utils/Log.h>
#include <log/log_main.h>

#include "drmrga.h"
#include "rga.h"

// -------------------------------------------------------------------------------
int         RkRgaGetHandleFd(buffer_handle_t handle, int *fd);
int         RkRgaGetHandleAttributes(buffer_handle_t handle,
                                     std::vector<int> *attrs);
int         RkRgaGetHandleMapAddress(buffer_handle_t handle,
                                     void **buf);
#endif  //Android

#endif  //_rk_graphic_buffer_h_
