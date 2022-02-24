// Copyright (c) 2021 by Rockchip Electronics Co., Ltd. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "drm_func.h"
#include <dlfcn.h>

int drm_init(drm_context *drm_ctx)
{
    static const char *card = "/dev/dri/card0";
    int flag = O_RDWR;
    int drm_fd = -1;

    drm_fd = open(card, flag);
    if (drm_fd < 0)
    {
        printf("failed to open %s\n", card);
        return -1;
    }

    drm_ctx->drm_handle = dlopen("/usr/lib/libdrm.so", RTLD_LAZY);
    if (!drm_ctx->drm_handle)
    {
        printf("failed to dlopen /usr/lib/libdrm.so\n");
        drm_deinit(drm_ctx, drm_fd);
        return -1;
    }

    drm_ctx->io_func = (FUNC_DRM_IOCTL)dlsym(drm_ctx->drm_handle, "drmIoctl");
    if (drm_ctx->io_func == NULL)
    {
        dlclose(drm_ctx->drm_handle);
        drm_ctx->drm_handle = NULL;
        drm_deinit(drm_ctx, drm_fd);
        printf("failed to dlsym drmIoctl\n");
        return -1;
    }
    return drm_fd;
}

void drm_deinit(drm_context *drm_ctx, int drm_fd)
{
    if (drm_ctx->drm_handle)
    {
        dlclose(drm_ctx->drm_handle);
        drm_ctx->drm_handle = NULL;
    }
    if (drm_fd > 0)
    {
        close(drm_fd);
    }
}

void *drm_buf_alloc(drm_context *drm_ctx, int drm_fd, int TexWidth, int TexHeight, int bpp, int *fd, unsigned int *handle, size_t *actual_size)
{
    int ret;
    if (drm_ctx == NULL)
    {
        printf("drm context is unvalid\n");
        return NULL;
    }
    char *map = NULL;

    void *vir_addr = NULL;
    struct drm_prime_handle fd_args;
    struct drm_mode_map_dumb mmap_arg;
    struct drm_mode_destroy_dumb destory_arg;

    struct drm_mode_create_dumb alloc_arg;

    memset(&alloc_arg, 0, sizeof(alloc_arg));
    alloc_arg.bpp = bpp;
    alloc_arg.width = TexWidth;
    alloc_arg.height = TexHeight;
    // alloc_arg.flags = ROCKCHIP_BO_CONTIG;

    //获取handle和size
    ret = drm_ctx->io_func(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &alloc_arg);
    if (ret)
    {
        printf("failed to create dumb buffer: %s\n", strerror(errno));
        return NULL;
    }
    if (handle != NULL)
    {
        *handle = alloc_arg.handle;
    }
    if (actual_size != NULL)
    {
        *actual_size = alloc_arg.size;
    }
    // printf("create width=%u, height=%u, bpp=%u, size=%lu dumb buffer\n",alloc_arg.width,alloc_arg.height,alloc_arg.bpp,alloc_arg.size);
    // printf("out handle= %d\n",alloc_arg.handle);

    //获取fd
    memset(&fd_args, 0, sizeof(fd_args));
    fd_args.fd = -1;
    fd_args.handle = alloc_arg.handle;
    ;
    fd_args.flags = 0;
    ret = drm_ctx->io_func(drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &fd_args);
    if (ret)
    {
        printf("rk-debug handle_to_fd failed ret=%d,err=%s, handle=%x \n", ret, strerror(errno), fd_args.handle);
        return NULL;
    }
    // printf("out fd = %d, drm fd: %d\n",fd_args.fd,drm_fd);
    if (fd != NULL)
    {
        *fd = fd_args.fd;
    }

    //获取虚拟地址
    memset(&mmap_arg, 0, sizeof(mmap_arg));
    mmap_arg.handle = alloc_arg.handle;

    ret = drm_ctx->io_func(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &mmap_arg);
    if (ret)
    {
        printf("failed to create map dumb: %s\n", strerror(errno));
        vir_addr = NULL;
        goto destory_dumb;
    }
    vir_addr = map = mmap(0, alloc_arg.size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_arg.offset);
    if (map == MAP_FAILED)
    {
        printf("failed to mmap buffer: %s\n", strerror(errno));
        vir_addr = NULL;
        goto destory_dumb;
    }
    // printf("alloc map=%x \n",map);
    return vir_addr;
destory_dumb:
    memset(&destory_arg, 0, sizeof(destory_arg));
    destory_arg.handle = alloc_arg.handle;
    ret = drm_ctx->io_func(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destory_arg);
    if (ret)
        printf("failed to destory dumb %d\n", ret);
    return vir_addr;
}

int drm_buf_destroy(drm_context *drm_ctx, int drm_fd, int buf_fd, int handle, void *drm_buf, size_t size)
{
    int ret = -1;
    if (drm_buf == NULL)
    {
        printf("drm buffer is NULL\n");
        return -1;
    }

    munmap(drm_buf, size);

    struct drm_mode_destroy_dumb destory_arg;
    memset(&destory_arg, 0, sizeof(destory_arg));
    destory_arg.handle = handle;
    ret = drm_ctx->io_func(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destory_arg);
    if (ret)
        printf("failed to destory dumb %d, error=%s\n", ret, strerror(errno));
    if (buf_fd > 0)
    {
        close(buf_fd);
    }

    return ret;
}
