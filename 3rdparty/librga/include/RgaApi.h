/*
 * Copyright (C) 2016 Rockchip Electronics Co., Ltd.
 * Authors:
 *    Zhiqin Wei <wzq@rock-chips.com>
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
#ifndef _rockchip_rga_c_h_
#define _rockchip_rga_c_h_

#include <stdint.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include <sys/mman.h>
#include <linux/stddef.h>

#include "drmrga.h"
#include "rga.h"

#ifdef __cplusplus
extern "C"{
#endif

/*
 * Compatible with the old version of C interface.The new
 * version of the C interface no longer requires users to
 * initialize rga, so RgaInit and RgaDeInit are just for
 * compatibility with the old C interface, so please do
 * not use ctx, because it is usually a NULL.
 */
#define RgaInit(ctx) ({ \
    int ret = 0; \
    ret = c_RkRgaInit(); \
    c_RkRgaGetContext(ctx); \
    ret;\
})
#define RgaDeInit(ctx) { \
    (void)ctx;        /* unused */ \
    c_RkRgaDeInit(); \
}
#define RgaBlit(...) c_RkRgaBlit(__VA_ARGS__)
#define RgaCollorFill(...) c_RkRgaColorFill(__VA_ARGS__)
#define RgaFlush() c_RkRgaFlush()

int  c_RkRgaInit();
void c_RkRgaDeInit();
void c_RkRgaGetContext(void **ctx);
int  c_RkRgaBlit(rga_info_t *src, rga_info_t *dst, rga_info_t *src1);
int  c_RkRgaColorFill(rga_info_t *dst);
int  c_RkRgaFlush();

#ifndef ANDROID /* linux */
int c_RkRgaGetAllocBuffer(bo_t *bo_info, int width, int height, int bpp);
int c_RkRgaGetAllocBufferCache(bo_t *bo_info, int width, int height, int bpp);
int c_RkRgaGetMmap(bo_t *bo_info);
int c_RkRgaUnmap(bo_t *bo_info);
int c_RkRgaFree(bo_t *bo_info);
int c_RkRgaGetBufferFd(bo_t *bo_info, int *fd);
#endif /* #ifndef ANDROID */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _rockchip_rga_c_h_ */
