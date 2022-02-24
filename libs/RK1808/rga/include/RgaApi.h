/*
 * Copyright (C) 2016 Rockchip Electronics Co.Ltd
 * Authors:
 *	Zhiqin Wei <wzq@rock-chips.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
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

#include "RockchipRgaMacro.h"

#ifdef __cplusplus
extern "C"{
#endif


int c_RkRgaInit();
void c_RkRgaDeInit();
int c_RkRgaGetAllocBuffer(bo_t *bo_info, int width, int height, int bpp);
int c_RkRgaGetMmap(bo_t *bo_info);
int c_RkRgaUnmap(bo_t *bo_info);
int c_RkRgaGetBufferFd(bo_t *bo_info, int *fd);
int c_RkRgaBlit(rga_info_t *src, rga_info_t *dst, rga_info_t *src1);
int c_RkRgaColorFill(rga_info_t *dst);
#ifdef __cplusplus
}
#endif
