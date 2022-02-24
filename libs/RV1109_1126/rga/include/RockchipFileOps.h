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

#ifndef _rk_file_ops_h_
#define _rk_file_ops_h_

// -------------------------------------------------------------------------------
int get_buf_from_file(void *buf, int f, int sw, int sh, int index);
int output_buf_data_to_file(void *buf, int f, int sw, int sh, int index);
#endif

