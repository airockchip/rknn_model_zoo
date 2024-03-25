/*
 * Copyright (C) 2022  Rockchip Electronics Co., Ltd.
 * Authors:
 *     Randall zhuo <randall.zhuo@rock-chips.com>
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

#if defined (__arm__) || defined (__aarch64__)
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h> // open function
#include <unistd.h>    // close function
#include <errno.h>
#include <sys/mman.h>
#include <dlfcn.h>

#include <linux/input.h>
#include "drm_fourcc.h"
#include "xf86drm.h"

typedef int(* DRM_IOCTL)(int fd, unsigned long request, void *arg);
static DRM_IOCTL drmIoctl_func = NULL;
static void *drm_handle = NULL;
static int drm_fd = -1;

struct drm_rockchip_gem_phys {
    uint32_t handle;
    uint32_t phy_addr;
};

#define DRM_ROCKCHIP_GEM_GET_PHYS   0x04
#define DRM_IOCTL_ROCKCHIP_GEM_GET_PHYS     DRM_IOWR(DRM_COMMAND_BASE + \
        DRM_ROCKCHIP_GEM_GET_PHYS, struct drm_rockchip_gem_phys)


static int drm_init()
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
    return drm_fd;
}

static void drm_deinit(int drm_fd)
{
    if (drm_fd > 0)
    {
        close(drm_fd);
    }
}

void *drm_buf_alloc(int TexWidth, int TexHeight, int bpp, int *fd,  int *handle, size_t *actual_size, int flags=0)
{
    int ret;
    char *map = NULL;

    void *vir_addr = NULL;
    struct drm_prime_handle fd_args;
    struct drm_mode_map_dumb mmap_arg;
    struct drm_mode_destroy_dumb destory_arg;

    struct drm_mode_create_dumb alloc_arg;

    if ((drm_fd < 0) || (drmIoctl_func == NULL)) {
        return NULL;
    }

    memset(&alloc_arg, 0, sizeof(alloc_arg));
    alloc_arg.bpp = bpp;
    alloc_arg.width = TexWidth;
    alloc_arg.height = TexHeight;
    alloc_arg.flags = flags;

    //获取handle和size
    ret = drmIoctl_func(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &alloc_arg);
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

    int pagesize = sysconf(_SC_PAGESIZE);
#if 1
    printf("pagesize is %d\n", pagesize);
    printf("create width=%u, height=%u, bpp=%u, size=%lu dumb buffer\n", alloc_arg.width, alloc_arg.height, alloc_arg.bpp, (unsigned long)alloc_arg.size);
    printf("out handle= %d\n", alloc_arg.handle);
#endif
    //获取fd
    memset(&fd_args, 0, sizeof(fd_args));
    fd_args.fd = -1;
    fd_args.handle = alloc_arg.handle;
    fd_args.flags = 0;

    ret = drmIoctl_func(drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &fd_args);
    if (ret)
    {
        printf("rk-debug handle_to_fd failed ret=%d,err=%s, handle=%x \n", ret, strerror(errno), fd_args.handle);
        return NULL;
    }

    if (fd != NULL)
    {
        *fd = fd_args.fd;
    }

    //获取虚拟地址
    memset(&mmap_arg, 0, sizeof(mmap_arg));
    mmap_arg.handle = alloc_arg.handle;

    ret = drmIoctl_func(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &mmap_arg);
    if (ret)
    {
        printf("failed to create map dumb: %s\n", strerror(errno));
        vir_addr = NULL;
        goto destory_dumb;
    }

    vir_addr = map = (char *)mmap(0, alloc_arg.size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_arg.offset);
    if (map == MAP_FAILED)
    {
        printf("failed to mmap buffer: %s\n", strerror(errno));
        vir_addr = NULL;
        goto destory_dumb;
    }
    return vir_addr;

destory_dumb:
    memset(&destory_arg, 0, sizeof(destory_arg));
    destory_arg.handle = alloc_arg.handle;
    ret = drmIoctl_func(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destory_arg);
    return vir_addr;
}

int drm_buf_destroy(int buf_fd, int handle, void *drm_buf, size_t size)
{
    int ret = -1;

    if ((drm_fd < 0) || (drmIoctl_func == NULL)) {
        return -1;
    }

    if (drm_buf == NULL)
    {
        return -1;
    }

    munmap(drm_buf, size);

    struct drm_mode_destroy_dumb destory_arg;
    memset(&destory_arg, 0, sizeof(destory_arg));
    destory_arg.handle = handle;

    ret = drmIoctl_func(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destory_arg);
    if (ret)
    {
        printf("failed to destory dumb %d, error=%s\n", ret, strerror(errno));
    }

    if (buf_fd > 0)
    {
        close(buf_fd);
    }

    return ret;
}

uint32_t drm_buf_get_phy(int handle) {
    struct drm_rockchip_gem_phys phys_arg;
    phys_arg.handle = handle;
    phys_arg.phy_addr = 0;

    int ret = drmIoctl_func(drm_fd, DRM_IOCTL_ROCKCHIP_GEM_GET_PHYS, &phys_arg);
    if (ret)
        printf("failed to get phy address: %s\n", strerror(errno));

    printf("get phys 0x%x\n", phys_arg.phy_addr);

    return phys_arg.phy_addr;
}

__attribute__((constructor)) static int load_drm() {
    drm_fd =  drm_init();

    if (drm_fd < 0) {
        return -1;
    }

    drm_handle = dlopen("libdrm.so", RTLD_LAZY);

    if (!drm_handle) {
        printf("[RKNN] Can not find libdrm.so\n");
        drm_deinit(drm_fd);
        return -1;
    }

    drmIoctl_func = (DRM_IOCTL)dlsym(drm_handle, "drmIoctl");

    if (drmIoctl_func == NULL) {
        dlclose(drm_handle);
        drm_handle = NULL;
        drm_deinit(drm_fd);
        return -1;
    }

    return 0;
}

__attribute__((destructor)) static void unload_drm() {
    if (drm_handle) {
        dlclose(drm_handle);
        drm_handle = NULL;
    }

    drm_deinit(drm_fd);
    drm_fd = -1;
}

#if 0
int main_(){
  void *drm_buf = NULL;
  int drm_fd = -1;
  int out_fd;
  unsigned int handle;
  int width = 224;
  int height = 224;
  int channel = 3;
  int size = width*height*channel;
  int actual_size=0;
  // DRM alloc buffer
  while(1){
    drm_fd = drm_init();

    drm_buf = drm_buf_alloc(drm_fd,width,height,channel*8,&out_fd,&handle,&actual_size);
    // unsigned char * buf = (unsigned char *) drm_buf;
    // for(int i = 0;i<width*height;++i) {
    //   printf("[%d] %d\n",i,(int)buf[i]);
    // }
    // printf("\n");


    //free drm buffer
    drm_buf_destroy(drm_fd,out_fd,handle,drm_buf,actual_size);
  }
  return 0;

}
#endif
#endif

