/*
 * Copyright (C) 2016 Rockchip Electronics Co.Ltd
 * Authors:
 *  Zhiqin Wei <wzq@rock-chips.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifndef __ROCKCHIP_RGA_MACRO_H__
#define __ROCKCHIP_RGA_MACRO_H__

/* flip source image horizontally (around the vertical axis) */                                                                                               
#define HAL_TRANSFORM_FLIP_H     0x01
/* flip source image vertically (around the horizontal axis)*/                                                                                                
#define HAL_TRANSFORM_FLIP_V     0x02
/* rotate source image 90 degrees clockwise */                                                                                                                
#define HAL_TRANSFORM_ROT_90     0x04
/* rotate source image 180 degrees */                                                                                                                         
#define HAL_TRANSFORM_ROT_180    0x03
/* rotate source image 270 degrees clockwise */                                                                                                               
#define HAL_TRANSFORM_ROT_270    0x07

#endif
