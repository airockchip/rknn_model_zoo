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
#ifndef _im2d_common_h_
#define _im2d_common_h_

#include "im2d_type.h"

/**
 * Query RGA basic information, supported resolution, supported format, etc.
 *
 * @param name
 *      RGA_VENDOR
 *      RGA_VERSION
 *      RGA_MAX_INPUT
 *      RGA_MAX_OUTPUT
 *      RGA_INPUT_FORMAT
 *      RGA_OUTPUT_FORMAT
 *      RGA_EXPECTED
 *      RGA_ALL
 *
 * @returns a string describing properties of RGA.
 */
IM_EXPORT_API const char* querystring(int name);

/**
 * String to output the error message
 *
 * @param status
 *      process result value.
 *
 * @returns error message.
 */
#define imStrError(...) \
    ({ \
        const char* im2d_api_err; \
        int __args[] = {__VA_ARGS__}; \
        int __argc = sizeof(__args)/sizeof(int); \
        if (__argc == 0) { \
            im2d_api_err = imStrError_t(IM_STATUS_INVALID_PARAM); \
        } else if (__argc == 1){ \
            im2d_api_err = imStrError_t((IM_STATUS)__args[0]); \
        } else { \
            im2d_api_err = ("Fatal error, imStrError() too many parameters\n"); \
            printf("Fatal error, imStrError() too many parameters\n"); \
        } \
        im2d_api_err; \
    })
IM_C_API const char* imStrError_t(IM_STATUS status);

/**
 * check im2d api header file
 *
 * @param header_version
 *      Default is RGA_CURRENT_API_HEADER_VERSION, no need to change if there are no special cases.
 *
 * @returns no error or else negative error code.
 */
#ifdef __cplusplus
IM_API IM_STATUS imcheckHeader(im_api_version_t header_version = RGA_CURRENT_API_HEADER_VERSION);
#endif

/**
 * check RGA basic information, supported resolution, supported format, etc.
 *
 * @param src
 * @param dst
 * @param pat
 * @param src_rect
 * @param dst_rect
 * @param pat_rect
 * @param mode_usage
 *
 * @returns no error or else negative error code.
 */
#define imcheck(src, dst, src_rect, dst_rect, ...) \
    ({ \
        IM_STATUS __ret = IM_STATUS_NOERROR; \
        rga_buffer_t __pat; \
        im_rect __pat_rect; \
        memset(&__pat, 0, sizeof(rga_buffer_t)); \
        memset(&__pat_rect, 0, sizeof(im_rect)); \
        int __args[] = {__VA_ARGS__}; \
        int __argc = sizeof(__args)/sizeof(int); \
        if (__argc == 0) { \
            __ret = imcheck_t(src, dst, __pat, src_rect, dst_rect, __pat_rect, 0); \
        } else if (__argc == 1){ \
            __ret = imcheck_t(src, dst, __pat, src_rect, dst_rect, __pat_rect, __args[0]); \
        } else { \
            __ret = IM_STATUS_FAILED; \
            printf("check failed\n"); \
        } \
        __ret; \
    })
#define imcheck_composite(src, dst, pat, src_rect, dst_rect, pat_rect, ...) \
    ({ \
        IM_STATUS __ret = IM_STATUS_NOERROR; \
        int __args[] = {__VA_ARGS__}; \
        int __argc = sizeof(__args)/sizeof(int); \
        if (__argc == 0) { \
            __ret = imcheck_t(src, dst, pat, src_rect, dst_rect, pat_rect, 0); \
        } else if (__argc == 1){ \
            __ret = imcheck_t(src, dst, pat, src_rect, dst_rect, pat_rect, __args[0]); \
        } else { \
            __ret = IM_STATUS_FAILED; \
            printf("check failed\n"); \
        } \
        __ret; \
    })
IM_C_API IM_STATUS imcheck_t(const rga_buffer_t src, const rga_buffer_t dst, const rga_buffer_t pat,
                             const im_rect src_rect, const im_rect dst_rect, const im_rect pat_rect, const int mode_usage);
/* Compatible with the legacy symbol */
IM_C_API void rga_check_perpare(rga_buffer_t *src, rga_buffer_t *dst, rga_buffer_t *pat,
                                im_rect *src_rect, im_rect *dst_rect, im_rect *pat_rect, int mode_usage);

/**
 * block until all execution is complete
 *
 * @param release_fence_fd
 *      RGA job release fence fd
 *
 * @returns success or else negative error code.
 */
IM_EXPORT_API IM_STATUS imsync(int release_fence_fd);

/**
 * config
 *
 * @param name
 *      enum IM_CONFIG_NAME
 * @param value
 *
 * @returns success or else negative error code.
 */
IM_EXPORT_API IM_STATUS imconfig(IM_CONFIG_NAME name, uint64_t value);

#endif /* #ifndef _im2d_common_h_ */
