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

#ifndef _rockchip_rga_h_
#define _rockchip_rga_h_

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
#include "GrallocOps.h"
#include "RgaUtils.h"
#include "rga.h"

//////////////////////////////////////////////////////////////////////////////////
#ifndef ANDROID
#include "RgaSingleton.h"
#endif

#ifdef ANDROID
#include <utils/Singleton.h>
#include <utils/Thread.h>
#include <hardware/hardware.h>

namespace android {
#endif

    class RockchipRga :public Singleton<RockchipRga> {
      public:

        static inline RockchipRga& get() {
            return getInstance();
        }

        int         RkRgaInit();
        void        RkRgaDeInit();
        void        RkRgaGetContext(void **ctx);
#ifndef ANDROID /* LINUX */
        int         RkRgaAllocBuffer(int drm_fd /* input */, bo_t *bo_info,
                                     int width, int height, int bpp, int flags);
        int         RkRgaFreeBuffer(int drm_fd /* input */, bo_t *bo_info);
        int         RkRgaGetAllocBuffer(bo_t *bo_info, int width, int height, int bpp);
        int         RkRgaGetAllocBufferExt(bo_t *bo_info, int width, int height, int bpp, int flags);
        int         RkRgaGetAllocBufferCache(bo_t *bo_info, int width, int height, int bpp);
        int         RkRgaGetMmap(bo_t *bo_info);
        int         RkRgaUnmap(bo_t *bo_info);
        int         RkRgaFree(bo_t *bo_info);
        int         RkRgaGetBufferFd(bo_t *bo_info, int *fd);
#else
        int         RkRgaGetBufferFd(buffer_handle_t handle, int *fd);
        int         RkRgaGetHandleMapCpuAddress(buffer_handle_t handle, void **buf);
#endif
        int         RkRgaBlit(rga_info *src, rga_info *dst, rga_info *src1);
        int         RkRgaCollorFill(rga_info *dst);
        int         RkRgaCollorPalette(rga_info *src, rga_info *dst, rga_info *lut);
        int         RkRgaFlush();


        void        RkRgaSetLogOnceFlag(int log) {
            mLogOnce = log;
        }
        void        RkRgaSetAlwaysLogFlag(bool log) {
            mLogAlways = log;
        }
        void        RkRgaLogOutRgaReq(struct rga_req rgaReg);
        int         RkRgaLogOutUserPara(rga_info *rgaInfo);
        inline bool RkRgaIsReady() {
            return mSupportRga;
        }

        RockchipRga();
        ~RockchipRga();
      private:
        bool                            mSupportRga;
        int                             mLogOnce;
        int                             mLogAlways;
        void *                          mContext;

        friend class Singleton<RockchipRga>;
    };

#ifdef ANDROID
}; // namespace android
#endif

#endif

