/*
 * Copyright (C) 2022 Rockchip Electronics Co., Ltd.
 * Authors:
 *  Cerf Yu <cerf.yu@rock-chips.com>
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
#ifndef _im2d_single_h_
#define _im2d_single_h_

#include "im2d_type.h"

#ifdef __cplusplus

/**
 * copy
 *
 * @param src
 *      The input source image.
 * @param dst
 *      The output destination image.
 * @param sync
 *      When 'sync == 1', wait for the operation to complete and return, otherwise return directly.
 * @param release_fence_fd
 *      When 'sync == 0', the fence_fd used to identify the current job state
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS imcopy(const rga_buffer_t src, rga_buffer_t dst, int sync = 1, int *release_fence_fd = NULL);

/**
 * Resize
 *
 * @param src
 *      The input source image.
 * @param dst
 *      The output destination image.
 * @param fx
 *      X-direction resize factor.
 * @param fy
 *      X-direction resize factor.
 * @param interpolation
 *      Interpolation formula(Only RGA1 support).
 * @param sync
 *      When 'sync == 1', wait for the operation to complete and return, otherwise return directly.
 * @param release_fence_fd
 *      When 'sync == 0', the fence_fd used to identify the current job state
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS imresize(const rga_buffer_t src, rga_buffer_t dst, double fx = 0, double fy = 0, int interpolation = 0, int sync = 1, int *release_fence_fd = NULL);

/**
 * Crop
 *
 * @param src
 *      The input source image.
 * @param dst
 *      The output destination image.
 * @param rect
 *      The rectangle on the source image that needs to be cropped.
 * @param sync
 *      When 'sync == 1', wait for the operation to complete and return, otherwise return directly.
 * @param release_fence_fd
 *      When 'sync == 0', the fence_fd used to identify the current job state
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS imcrop(const rga_buffer_t src, rga_buffer_t dst, im_rect rect, int sync = 1, int *release_fence_fd = NULL);

/**
 * translate
 *
 * @param src
 *      The input source image.
 * @param dst
 *      The output destination image.
 * @param x
 *      Output the coordinates of the starting point in the X-direction of the destination image.
 * @param y
 *      Output the coordinates of the starting point in the Y-direction of the destination image.
 * @param sync
 *      When 'sync == 1', wait for the operation to complete and return, otherwise return directly.
 * @param release_fence_fd
 *      When 'sync == 0', the fence_fd used to identify the current job state
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS imtranslate(const rga_buffer_t src, rga_buffer_t dst, int x, int y, int sync = 1, int *release_fence_fd = NULL);

/**
 * format convert
 *
 * @param src
 *      The input source image.
 * @param dst
 *      The output destination image.
 * @param sfmt
 *      The source image format.
 * @param dfmt
 *      The destination image format.
 * @param mode
 *      color space mode:
 *          IM_YUV_TO_RGB_BT601_LIMIT
 *          IM_YUV_TO_RGB_BT601_FULL
 *          IM_YUV_TO_RGB_BT709_LIMIT
 *          IM_RGB_TO_YUV_BT601_FULL
 *          IM_RGB_TO_YUV_BT601_LIMIT
 *          IM_RGB_TO_YUV_BT709_LIMIT
 * @param sync
 *      When 'sync == 1', wait for the operation to complete and return, otherwise return directly.
 * @param release_fence_fd
 *      When 'sync == 0', the fence_fd used to identify the current job state
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS imcvtcolor(rga_buffer_t src, rga_buffer_t dst, int sfmt, int dfmt, int mode = IM_COLOR_SPACE_DEFAULT, int sync = 1, int *release_fence_fd = NULL);

/**
 * rotation
 *
 * @param src
 *      The input source image.
 * @param dst
 *      The output destination image.
 * @param rotation
 *      IM_HAL_TRANSFORM_ROT_90
 *      IM_HAL_TRANSFORM_ROT_180
 *      IM_HAL_TRANSFORM_ROT_270
 * @param sync
 *      When 'sync == 1', wait for the operation to complete and return, otherwise return directly.
 * @param release_fence_fd
 *      When 'sync == 0', the fence_fd used to identify the current job state
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS imrotate(const rga_buffer_t src, rga_buffer_t dst, int rotation, int sync = 1, int *release_fence_fd = NULL);

/**
 * flip
 *
 * @param src
 *      The input source image.
 * @param dst
 *      The output destination image.
 * @param mode
 *      IM_HAL_TRANSFORM_FLIP_H
 *      IM_HAL_TRANSFORM_FLIP_V
 * @param sync
 *      When 'sync == 1', wait for the operation to complete and return, otherwise return directly.
 * @param release_fence_fd
 *      When 'sync == 0', the fence_fd used to identify the current job state
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS imflip(const rga_buffer_t src, rga_buffer_t dst, int mode, int sync = 1, int *release_fence_fd = NULL);

/**
 * 2-channel blend (SRC + DST -> DST or SRCA + SRCB -> DST)
 *
 * @param fg_image
 *      The foreground image.
 * @param bg_image
 *      The background image, which is also the output destination image.
 * @param mode
 *      Port-Duff mode:
 *          IM_ALPHA_BLEND_SRC
 *          IM_ALPHA_BLEND_DST
 *          IM_ALPHA_BLEND_SRC_OVER
 *          IM_ALPHA_BLEND_DST_OVER
 * @param sync
 *      When 'sync == 1', wait for the operation to complete and return, otherwise return directly.
 * @param release_fence_fd
 *      When 'sync == 0', the fence_fd used to identify the current job state
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS imblend(const rga_buffer_t fd_image, rga_buffer_t bg_image, int mode = IM_ALPHA_BLEND_SRC_OVER, int sync = 1, int *release_fence_fd = NULL);

/**
 * 3-channel blend (SRC + DST -> DST or SRCA + SRCB -> DST)
 *
 * @param fg_image
 *      The foreground image.
 * @param bg_image
 *      The background image.
 * @param output_image
 *      The output destination image.
 * @param mode
 *      Port-Duff mode:
 *          IM_ALPHA_BLEND_SRC
 *          IM_ALPHA_BLEND_DST
 *          IM_ALPHA_BLEND_SRC_OVER
 *          IM_ALPHA_BLEND_DST_OVER
 * @param sync
 *      When 'sync == 1', wait for the operation to complete and return, otherwise return directly.
 * @param release_fence_fd
 *      When 'sync == 0', the fence_fd used to identify the current job state
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS imcomposite(const rga_buffer_t srcA, const rga_buffer_t srcB, rga_buffer_t dst, int mode = IM_ALPHA_BLEND_SRC_OVER, int sync = 1, int *release_fence_fd = NULL);

/**
 * color key
 *
 * @param fg_image
 *      The foreground image.
 * @param bg_image
 *      The background image, which is also the output destination image.
 * @param colorkey_range
 *      The range of color key.
 * @param sync
 *      When 'sync == 1', wait for the operation to complete and return, otherwise return directly.
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS imcolorkey(const rga_buffer_t src, rga_buffer_t dst, im_colorkey_range range, int mode = IM_ALPHA_COLORKEY_NORMAL, int sync = 1, int *release_fence_fd = NULL);

/**
 * OSD
 *
 * @param osd
 *      The osd text block.
 * @param dst
 *      The background image.
 * @param osd_rect
 *      The rectangle on the source image that needs to be OSD.
 * @param osd_config
 *      osd mode configuration.
 * @param sync
 *      When 'sync == 1', wait for the operation to complete and return, otherwise return directly.
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS imosd(const rga_buffer_t osd,const rga_buffer_t dst,
                       const im_rect osd_rect, im_osd_t *osd_config,
                       int sync = 1, int *release_fence_fd = NULL);

/**
 * nn quantize
 *
 * @param src
 *      The input source image.
 * @param dst
 *      The output destination image.
 * @param nninfo
 *      nn configuration
 * @param sync
 *      When 'sync == 1', wait for the operation to complete and return, otherwise return directly.
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS imquantize(const rga_buffer_t src, rga_buffer_t dst, im_nn_t nn_info, int sync = 1, int *release_fence_fd = NULL);

/**
 * ROP
 *
 * @param src
 *      The input source image.
 * @param dst
 *      The output destination image.
 * @param rop_code
 *      The ROP opcode.
 * @param sync
 *      When 'sync == 1', wait for the operation to complete and return, otherwise return directly.
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS imrop(const rga_buffer_t src, rga_buffer_t dst, int rop_code, int sync = 1, int *release_fence_fd = NULL);

/**
 * fill/reset/draw
 *
 * @param dst
 *      The output destination image.
 * @param rect
 *      The rectangle on the source image that needs to be filled with color.
 * @param color
 *      The fill color value.
 * @param sync
 *      When 'sync == 1', wait for the operation to complete and return, otherwise return directly.
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS imfill(rga_buffer_t dst, im_rect rect, int color, int sync = 1, int *release_fence_fd = NULL);

/**
 * fill array
 *
 * @param dst
 *      The output destination image.
 * @param rect_array
 *      The rectangle arrays on the source image that needs to be filled with color.
 * @param array_size
 *      The size of rectangular area arrays.
 * @param color
 *      The fill color value.
 * @param sync
 *      When 'sync == 1', wait for the operation to complete and return, otherwise return directly.
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS imfillArray(rga_buffer_t dst, im_rect *rect_array, int array_size, uint32_t color, int sync = 1, int *release_fence_fd = NULL);

/**
 * fill rectangle
 *
 * @param dst
 *      The output destination image.
 * @param rect
 *      The rectangle on the source image that needs to be filled with color.
 * @param color
 *      The fill color value.
 * @param thickness
 *      Thickness of lines that make up the rectangle. Negative values, like -1,
 *      mean that the function has to draw a filled rectangle.
 * @param sync
 *      When 'sync == 1', wait for the operation to complete and return, otherwise return directly.
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS imrectangle(rga_buffer_t dst, im_rect rect,
                             uint32_t color, int thickness,
                             int sync = 1, int *release_fence_fd = NULL);

/**
 * fill rectangle array
 *
 * @param dst
 *      The output destination image.
 * @param rect_array
 *      The rectangle arrays on the source image that needs to be filled with color.
 * @param array_size
 *      The size of rectangular area arrays.
 * @param color
 *      The fill color value.
 * @param thickness
 *      Thickness of lines that make up the rectangle. Negative values, like -1,
 *      mean that the function has to draw a filled rectangle.
 * @param sync
 *      When 'sync == 1', wait for the operation to complete and return, otherwise return directly.
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS imrectangleArray(rga_buffer_t dst, im_rect *rect_array, int array_size,
                                   uint32_t color, int thickness,
                                   int sync = 1, int *release_fence_fd = NULL);

/**
 * MOSAIC
 *
 * @param image
 *      The output destination image.
 * @param rect
 *      The rectangle on the source image that needs to be mosaicked.
 * @param mosaic_mode
 *      mosaic block width configuration:
 *          IM_MOSAIC_8
 *          IM_MOSAIC_16
 *          IM_MOSAIC_32
 *          IM_MOSAIC_64
 *          IM_MOSAIC_128
 * @param sync
 *      When 'sync == 1', wait for the operation to complete and return, otherwise return directly.
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS immosaic(const rga_buffer_t image, im_rect rect, int mosaic_mode, int sync = 1, int *release_fence_fd = NULL);

/**
 * MOSAIC array
 *
 * @param image
 *      The output destination image.
 * @param rect_array
 *      The rectangle arrays on the source image that needs to be filled with color.
 * @param array_size
 *      The size of rectangular area arrays.
 * @param mosaic_mode
 *      mosaic block width configuration:
 *          IM_MOSAIC_8
 *          IM_MOSAIC_16
 *          IM_MOSAIC_32
 *          IM_MOSAIC_64
 *          IM_MOSAIC_128
 * @param sync
 *      When 'sync == 1', wait for the operation to complete and return, otherwise return directly.
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS immosaicArray(const rga_buffer_t image, im_rect *rect_array, int array_size, int mosaic_mode, int sync = 1, int *release_fence_fd = NULL);

/**
 * palette
 *
 * @param src
 *      The input source image.
 * @param dst
 *      The output destination image.
 * @param lut
 *      The LUT table.
 * @param sync
 *      When 'sync == 1', wait for the operation to complete and return, otherwise return directly.
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS impalette(rga_buffer_t src, rga_buffer_t dst, rga_buffer_t lut, int sync = 1, int *release_fence_fd = NULL);

/**
 * process for single task mode
 *
 * @param src
 *      The input source image and is also the foreground image in blend.
 * @param dst
 *      The output destination image and is also the foreground image in blend.
 * @param pat
 *      The foreground image, or a LUT table.
 * @param srect
 *      The rectangle on the src channel image that needs to be processed.
 * @param drect
 *      The rectangle on the dst channel image that needs to be processed.
 * @param prect
 *      The rectangle on the pat channel image that needs to be processed.
 * @param opt
 *      The image processing options configuration.
 * @param usage
 *      The image processing usage.
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS improcess(rga_buffer_t src, rga_buffer_t dst, rga_buffer_t pat,
                           im_rect srect, im_rect drect, im_rect prect,
                           int acquire_fence_fd, int *release_fence_fd,
                           im_opt_t *opt_ptr, int usage);

/**
 * make border
 *
 * @param src
 *      The input source image.
 * @param dst
 *      The output destination image.
 * @param top
 *      the top pixels
 * @param bottom
 *      the bottom pixels
 * @param left
 *      the left pixels
 * @param right
 *      the right pixels
 * @param border_type
 *      Border type.
 * @param value
 *      The pixel value at which the border is filled.
 *
 * @returns success or else negative error code.
 */
IM_API IM_STATUS immakeBorder(rga_buffer_t src, rga_buffer_t dst,
                              int top, int bottom, int left, int right,
                              int border_type, int value = 0,
                              int sync = 1, int acquir_fence_fd = -1, int *release_fence_fd = NULL);

#endif /* #ifdef __cplusplus */

IM_C_API IM_STATUS immosaic(const rga_buffer_t image, im_rect rect, int mosaic_mode, int sync);
IM_C_API IM_STATUS imosd(const rga_buffer_t osd,const rga_buffer_t dst,
                         const im_rect osd_rect, im_osd_t *osd_config, int sync);
IM_C_API IM_STATUS improcess(rga_buffer_t src, rga_buffer_t dst, rga_buffer_t pat,
                             im_rect srect, im_rect drect, im_rect prect, int usage);

/* Start: Symbols reserved for compatibility with macro functions */
IM_C_API IM_STATUS imcopy_t(const rga_buffer_t src, rga_buffer_t dst, int sync);
IM_C_API IM_STATUS imresize_t(const rga_buffer_t src, rga_buffer_t dst, double fx, double fy, int interpolation, int sync);
IM_C_API IM_STATUS imcrop_t(const rga_buffer_t src, rga_buffer_t dst, im_rect rect, int sync);
IM_C_API IM_STATUS imtranslate_t(const rga_buffer_t src, rga_buffer_t dst, int x, int y, int sync);
IM_C_API IM_STATUS imcvtcolor_t(rga_buffer_t src, rga_buffer_t dst, int sfmt, int dfmt, int mode, int sync);
IM_C_API IM_STATUS imrotate_t(const rga_buffer_t src, rga_buffer_t dst, int rotation, int sync);
IM_C_API IM_STATUS imflip_t (const rga_buffer_t src, rga_buffer_t dst, int mode, int sync);
IM_C_API IM_STATUS imblend_t(const rga_buffer_t srcA, const rga_buffer_t srcB, rga_buffer_t dst, int mode, int sync);
IM_C_API IM_STATUS imcolorkey_t(const rga_buffer_t src, rga_buffer_t dst, im_colorkey_range range, int mode, int sync);
IM_C_API IM_STATUS imquantize_t(const rga_buffer_t src, rga_buffer_t dst, im_nn_t nn_info, int sync);
IM_C_API IM_STATUS imrop_t(const rga_buffer_t src, rga_buffer_t dst, int rop_code, int sync);
IM_C_API IM_STATUS imfill_t(rga_buffer_t dst, im_rect rect, int color, int sync);
IM_C_API IM_STATUS impalette_t(rga_buffer_t src, rga_buffer_t dst, rga_buffer_t lut, int sync);
/* End: Symbols reserved for compatibility with macro functions */

#ifndef __cplusplus

#define RGA_GET_MIN(n1, n2) ((n1) < (n2) ? (n1) : (n2))

/**
 * copy
 *
 * @param src
 * @param dst
 * @param sync
 *      wait until operation complete
 *
 * @returns success or else negative error code.
 */
#define imcopy(src, dst, ...) \
    ({ \
        IM_STATUS __ret = IM_STATUS_SUCCESS; \
        int __args[] = {__VA_ARGS__}; \
        int __argc = sizeof(__args)/sizeof(int); \
        if (__argc == 0) { \
            __ret = imcopy_t(src, dst, 1); \
        } else if (__argc == 1){ \
            __ret = imcopy_t(src, dst, (int)__args[RGA_GET_MIN(__argc, 0)]); \
        } else { \
            __ret = IM_STATUS_INVALID_PARAM; \
            printf("invalid parameter\n"); \
        } \
        __ret; \
    })

/**
 * Resize
 *
 * @param src
 * @param dst
 * @param fx
 * @param fy
 * @param interpolation
 * @param sync
 *      wait until operation complete
 *
 * @returns success or else negative error code.
 */
#define imresize(src, dst, ...) \
    ({ \
        IM_STATUS __ret = IM_STATUS_SUCCESS; \
        double __args[] = {__VA_ARGS__}; \
        int __argc = sizeof(__args)/sizeof(double); \
        if (__argc == 0) { \
            __ret = imresize_t(src, dst, 0, 0, INTER_LINEAR, 1); \
        } else if (__argc == 2){ \
            __ret = imresize_t(src, dst, __args[RGA_GET_MIN(__argc, 0)], __args[RGA_GET_MIN(__argc, 1)], INTER_LINEAR, 1); \
        } else if (__argc == 3){ \
            __ret = imresize_t(src, dst, __args[RGA_GET_MIN(__argc, 0)], __args[RGA_GET_MIN(__argc, 1)], (int)__args[RGA_GET_MIN(__argc, 2)], 1); \
        } else if (__argc == 4){ \
            __ret = imresize_t(src, dst, __args[RGA_GET_MIN(__argc, 0)], __args[RGA_GET_MIN(__argc, 1)], (int)__args[RGA_GET_MIN(__argc, 2)], (int)__args[RGA_GET_MIN(__argc, 3)]); \
        } else { \
            __ret = IM_STATUS_INVALID_PARAM; \
            printf("invalid parameter\n"); \
        } \
        __ret; \
    })

#define impyramid(src, dst, direction) \
        imresize_t(src, \
                   dst, \
                   direction == IM_UP_SCALE ? 0.5 : 2, \
                   direction == IM_UP_SCALE ? 0.5 : 2, \
                   INTER_LINEAR, 1)

/**
 * format convert
 *
 * @param src
 * @param dst
 * @param sfmt
 * @param dfmt
 * @param mode
 *      color space mode: IM_COLOR_SPACE_MODE
 * @param sync
 *      wait until operation complete
 *
 * @returns success or else negative error code.
 */
#define imcvtcolor(src, dst, sfmt, dfmt, ...) \
    ({ \
        IM_STATUS __ret = IM_STATUS_SUCCESS; \
        int __args[] = {__VA_ARGS__}; \
        int __argc = sizeof(__args)/sizeof(int); \
        if (__argc == 0) { \
            __ret = imcvtcolor_t(src, dst, sfmt, dfmt, IM_COLOR_SPACE_DEFAULT, 1); \
        } else if (__argc == 1){ \
            __ret = imcvtcolor_t(src, dst, sfmt, dfmt, (int)__args[RGA_GET_MIN(__argc, 0)], 1); \
        } else if (__argc == 2){ \
            __ret = imcvtcolor_t(src, dst, sfmt, dfmt, (int)__args[RGA_GET_MIN(__argc, 0)], (int)__args[RGA_GET_MIN(__argc, 1)]); \
        } else { \
            __ret = IM_STATUS_INVALID_PARAM; \
            printf("invalid parameter\n"); \
        } \
        __ret; \
    })

/**
 * Crop
 *
 * @param src
 * @param dst
 * @param rect
 * @param sync
 *      wait until operation complete
 *
 * @returns success or else negative error code.
 */
#define imcrop(src, dst, rect, ...) \
    ({ \
        IM_STATUS __ret = IM_STATUS_SUCCESS; \
        int __args[] = {__VA_ARGS__}; \
        int __argc = sizeof(__args)/sizeof(int); \
        if (__argc == 0) { \
            __ret = imcrop_t(src, dst, rect, 1); \
        } else if (__argc == 1){ \
            __ret = imcrop_t(src, dst, rect, (int)__args[RGA_GET_MIN(__argc, 0)]); \
        } else { \
            __ret = IM_STATUS_INVALID_PARAM; \
            printf("invalid parameter\n"); \
        } \
        __ret; \
    })

/**
 * translate
 *
 * @param src
 * @param dst
 * @param x
 * @param y
 * @param sync
 *      wait until operation complete
 *
 * @returns success or else negative error code.
 */
#define imtranslate(src, dst, x, y, ...) \
    ({ \
        IM_STATUS __ret = IM_STATUS_SUCCESS; \
        int __args[] = {__VA_ARGS__}; \
        int __argc = sizeof(__args)/sizeof(int); \
        if (__argc == 0) { \
            __ret = imtranslate_t(src, dst, x, y, 1); \
        } else if (__argc == 1){ \
            __ret = imtranslate_t(src, dst, x, y, (int)__args[RGA_GET_MIN(__argc, 0)]); \
        } else { \
            __ret = IM_STATUS_INVALID_PARAM; \
            printf("invalid parameter\n"); \
        } \
        __ret; \
    })

/**
 * rotation
 *
 * @param src
 * @param dst
 * @param rotation
 *      IM_HAL_TRANSFORM_ROT_90
 *      IM_HAL_TRANSFORM_ROT_180
 *      IM_HAL_TRANSFORM_ROT_270
 * @param sync
 *      wait until operation complete
 *
 * @returns success or else negative error code.
 */
#define imrotate(src, dst, rotation, ...) \
    ({ \
        IM_STATUS __ret = IM_STATUS_SUCCESS; \
        int __args[] = {__VA_ARGS__}; \
        int __argc = sizeof(__args)/sizeof(int); \
        if (__argc == 0) { \
            __ret = imrotate_t(src, dst, rotation, 1); \
        } else if (__argc == 1){ \
            __ret = imrotate_t(src, dst, rotation, (int)__args[RGA_GET_MIN(__argc, 0)]); \
        } else { \
            __ret = IM_STATUS_INVALID_PARAM; \
            printf("invalid parameter\n"); \
        } \
        __ret; \
    })


/**
 * flip
 *
 * @param src
 * @param dst
 * @param mode
 *      IM_HAL_TRANSFORM_FLIP_H
 *      IM_HAL_TRANSFORM_FLIP_V
 * @param sync
 *      wait until operation complete
 *
 * @returns success or else negative error code.
 */
#define imflip(src, dst, mode, ...) \
    ({ \
        IM_STATUS __ret = IM_STATUS_SUCCESS; \
        int __args[] = {__VA_ARGS__}; \
        int __argc = sizeof(__args)/sizeof(int); \
        if (__argc == 0) { \
            __ret = imflip_t(src, dst, mode, 1); \
        } else if (__argc == 1){ \
            __ret = imflip_t(src, dst, mode, (int)__args[RGA_GET_MIN(__argc, 0)]); \
        } else { \
            __ret = IM_STATUS_INVALID_PARAM; \
            printf("invalid parameter\n"); \
        } \
        __ret; \
    })

/**
 * blend (SRC + DST -> DST or SRCA + SRCB -> DST)
 *
 * @param srcA
 * @param srcB can be NULL.
 * @param dst
 * @param mode
 *      IM_ALPHA_BLEND_MODE
 * @param sync
 *      wait until operation complete
 *
 * @returns success or else negative error code.
 */
#define imblend(srcA, dst, ...) \
    ({ \
        IM_STATUS __ret = IM_STATUS_SUCCESS; \
        rga_buffer_t srcB; \
        memset(&srcB, 0x00, sizeof(rga_buffer_t)); \
        int __args[] = {__VA_ARGS__}; \
        int __argc = sizeof(__args)/sizeof(int); \
        if (__argc == 0) { \
            __ret = imblend_t(srcA, srcB, dst, IM_ALPHA_BLEND_SRC_OVER, 1); \
        } else if (__argc == 1){ \
            __ret = imblend_t(srcA, srcB, dst, (int)__args[RGA_GET_MIN(__argc, 0)], 1); \
        } else if (__argc == 2){ \
            __ret = imblend_t(srcA, srcB, dst, (int)__args[RGA_GET_MIN(__argc, 0)], (int)__args[RGA_GET_MIN(__argc, 1)]); \
        } else { \
            __ret = IM_STATUS_INVALID_PARAM; \
            printf("invalid parameter\n"); \
        } \
        __ret; \
    })
#define imcomposite(srcA, srcB, dst, ...) \
    ({ \
        IM_STATUS __ret = IM_STATUS_SUCCESS; \
        int __args[] = {__VA_ARGS__}; \
        int __argc = sizeof(__args)/sizeof(int); \
        if (__argc == 0) { \
            __ret = imblend_t(srcA, srcB, dst, IM_ALPHA_BLEND_SRC_OVER, 1); \
        } else if (__argc == 1){ \
            __ret = imblend_t(srcA, srcB, dst, (int)__args[RGA_GET_MIN(__argc, 0)], 1); \
        } else if (__argc == 2){ \
            __ret = imblend_t(srcA, srcB, dst, (int)__args[RGA_GET_MIN(__argc, 0)], (int)__args[RGA_GET_MIN(__argc, 1)]); \
        } else { \
            __ret = IM_STATUS_INVALID_PARAM; \
            printf("invalid parameter\n"); \
        } \
        __ret; \
    })

/**
 * color key
 *
 * @param src
 * @param dst
 * @param colorkey_range
 *      max color
 *      min color
 * @param sync
 *      wait until operation complete
 *
 * @returns success or else negative error code.
 */
#define imcolorkey(src, dst, range, ...) \
    ({ \
        IM_STATUS __ret = IM_STATUS_SUCCESS; \
        int __args[] = {__VA_ARGS__}; \
        int __argc = sizeof(__args)/sizeof(int); \
        if (__argc == 0) { \
            __ret = imcolorkey_t(src, dst, range, IM_ALPHA_COLORKEY_NORMAL, 1); \
        } else if (__argc == 1){ \
            __ret = imcolorkey_t(src, dst, range, (int)__args[RGA_GET_MIN(__argc, 0)], 1); \
        } else if (__argc == 2){ \
            __ret = imcolorkey_t(src, dst, range, (int)__args[RGA_GET_MIN(__argc, 0)], (int)__args[RGA_GET_MIN(__argc, 1)]); \
        } else { \
            __ret = IM_STATUS_INVALID_PARAM; \
            printf("invalid parameter\n"); \
        } \
        __ret; \
    })

/**
 * nn quantize
 *
 * @param src
 * @param dst
 * @param nninfo
 * @param sync
 *      wait until operation complete
 *
 * @returns success or else negative error code.
 */
#define imquantize(src, dst, nn_info, ...) \
    ({ \
        IM_STATUS __ret = IM_STATUS_SUCCESS; \
        int __args[] = {__VA_ARGS__}; \
        int __argc = sizeof(__args)/sizeof(int); \
        if (__argc == 0) { \
            __ret = imquantize_t(src, dst, nn_info, 1); \
        } else if (__argc == 1){ \
            __ret = imquantize_t(src, dst, nn_info, (int)__args[RGA_GET_MIN(__argc, 0)]); \
        } else { \
            __ret = IM_STATUS_INVALID_PARAM; \
            printf("invalid parameter\n"); \
        } \
        __ret; \
    })


/**
 * ROP
 *
 * @param src
 * @param dst
 * @param rop_code
 * @param sync
 *      wait until operation complete
 *
 * @returns success or else negative error code.
 */
#define imrop(src, dst, rop_code, ...) \
    ({ \
        IM_STATUS __ret = IM_STATUS_SUCCESS; \
        int __args[] = {__VA_ARGS__}; \
        int __argc = sizeof(__args)/sizeof(int); \
        if (__argc == 0) { \
            __ret = imrop_t(src, dst, rop_code, 1); \
        } else if (__argc == 1){ \
            __ret = imrop_t(src, dst, rop_code, (int)__args[RGA_GET_MIN(__argc, 0)]); \
        } else { \
            __ret = IM_STATUS_INVALID_PARAM; \
            printf("invalid parameter\n"); \
        } \
        __ret; \
    })

/**
 * fill/reset/draw
 *
 * @param src
 * @param dst
 * @param rect
 * @param color
 * @param sync
 *      wait until operation complete
 *
 * @returns success or else negative error code.
 */
#define imfill(buf, rect, color, ...) \
    ({ \
        IM_STATUS __ret = IM_STATUS_SUCCESS; \
        int __args[] = {__VA_ARGS__}; \
        int __argc = sizeof(__args)/sizeof(int); \
        if (__argc == 0) { \
            __ret = imfill_t(buf, rect, color, 1); \
        } else if (__argc == 1){ \
            __ret = imfill_t(buf, rect, color, (int)__args[RGA_GET_MIN(__argc, 0)]); \
        } else { \
            __ret = IM_STATUS_INVALID_PARAM; \
            printf("invalid parameter\n"); \
        } \
        __ret; \
    })

#define imreset(buf, rect, color, ...) \
    ({ \
        IM_STATUS __ret = IM_STATUS_SUCCESS; \
        int __args[] = {__VA_ARGS__}; \
        int __argc = sizeof(__args)/sizeof(int); \
        if (__argc == 0) { \
            __ret = imfill_t(buf, rect, color, 1); \
        } else if (__argc == 1){ \
            __ret = imfill_t(buf, rect, color, (int)__args[RGA_GET_MIN(__argc, 0)]); \
        } else { \
            __ret = IM_STATUS_INVALID_PARAM; \
            printf("invalid parameter\n"); \
        } \
        __ret; \
    })

#define imdraw(buf, rect, color, ...) \
    ({ \
        IM_STATUS __ret = IM_STATUS_SUCCESS; \
        int __args[] = {__VA_ARGS__}; \
        int __argc = sizeof(__args)/sizeof(int); \
        if (__argc == 0) { \
            __ret = imfill_t(buf, rect, color, 1); \
        } else if (__argc == 1){ \
            __ret = imfill_t(buf, rect, color, (int)__args[RGA_GET_MIN(__argc, 0)]); \
        } else { \
            __ret = IM_STATUS_INVALID_PARAM; \
            printf("invalid parameter\n"); \
        } \
        __ret; \
    })

/**
 * palette
 *
 * @param src
 * @param dst
 * @param lut
 * @param sync
 *      wait until operation complete
 *
 * @returns success or else negative error code.
 */
#define impalette(src, dst, lut,  ...) \
    ({ \
        IM_STATUS __ret = IM_STATUS_SUCCESS; \
        int __args[] = {__VA_ARGS__}; \
        int __argc = sizeof(__args)/sizeof(int); \
        if (__argc == 0) { \
            __ret = impalette_t(src, dst, lut, 1); \
        } else if (__argc == 1){ \
            __ret = impalette_t(src, dst, lut, (int)__args[RGA_GET_MIN(__argc, 0)]); \
        } else { \
            __ret = IM_STATUS_INVALID_PARAM; \
            printf("invalid parameter\n"); \
        } \
        __ret; \
    })
/* End define IM2D macro API */
#endif

#endif /* #ifndef _im2d_single_h_ */