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
#ifndef _im2d_mpi_hpp_
#define _im2d_mpi_hpp_

#include "im2d_type.h"

/**
 * Create and config an rga ctx for rockit-ko
 *
 * @param flags
 *      Some configuration flags for this job
 *
 * @returns job id.
 */
IM_EXPORT_API im_ctx_id_t imbegin(uint32_t flags);

/**
 * Cancel and delete an rga ctx for rockit-ko
 *
 * @param flags
 *      Some configuration flags for this job
 *
 * @returns success or else negative error code.
 */
IM_EXPORT_API IM_STATUS imcancel(im_ctx_id_t id);

/**
 * process for rockit-ko
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
 * @param acquire_fence_fd
 * @param release_fence_fd
 * @param opt
 *      The image processing options configuration.
 * @param usage
 *      The image processing usage.
 * @param ctx_id
 *      ctx id
 *
 * @returns success or else negative error code.
 */
#ifdef __cplusplus
IM_API IM_STATUS improcess(rga_buffer_t src, rga_buffer_t dst, rga_buffer_t pat,
                           im_rect srect, im_rect drect, im_rect prect,
                           int acquire_fence_fd, int *release_fence_fd,
                           im_opt_t *opt, int usage, im_ctx_id_t ctx_id);
#endif
IM_EXPORT_API IM_STATUS improcess_ctx(rga_buffer_t src, rga_buffer_t dst, rga_buffer_t pat,
                                      im_rect srect, im_rect drect, im_rect prect,
                                      int acquire_fence_fd, int *release_fence_fd,
                                      im_opt_t *opt, int usage, im_ctx_id_t ctx_id);

#endif /* #ifndef _im2d_mpi_hpp_ */