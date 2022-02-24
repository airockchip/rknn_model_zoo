#ifndef __DRM_FUNC_H__
#define __DRM_FUNC_H__
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>// open function
#include <unistd.h> // close function
#include <errno.h>
#include <sys/mman.h>


#include <linux/input.h>
#include "libdrm/drm_fourcc.h"
#include "xf86drm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (* FUNC_DRM_IOCTL)(int fd, unsigned long request, void *arg);

typedef struct _drm_context{
    void *drm_handle;
    FUNC_DRM_IOCTL io_func;
} drm_context;

/* memory type definitions. */
enum drm_rockchip_gem_mem_type
{
    /* Physically Continuous memory and used as default. */
    ROCKCHIP_BO_CONTIG = 1 << 0,
    /* cachable mapping. */
    ROCKCHIP_BO_CACHABLE = 1 << 1,
    /* write-combine mapping. */
    ROCKCHIP_BO_WC = 1 << 2,
    ROCKCHIP_BO_SECURE = 1 << 3,
    ROCKCHIP_BO_MASK = ROCKCHIP_BO_CONTIG | ROCKCHIP_BO_CACHABLE |
                       ROCKCHIP_BO_WC | ROCKCHIP_BO_SECURE
};

int drm_init(drm_context *drm_ctx);

void* drm_buf_alloc(drm_context *drm_ctx,int drm_fd, int TexWidth, int TexHeight,int bpp,int *fd,unsigned int *handle,size_t *actual_size);

int drm_buf_destroy(drm_context *drm_ctx,int drm_fd,int buf_fd, int handle,void *drm_buf,size_t size);

void drm_deinit(drm_context *drm_ctx, int drm_fd);

#ifdef __cplusplus
}
#endif
#endif /*__DRM_FUNC_H__*/