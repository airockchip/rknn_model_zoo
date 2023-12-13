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

#include <getopt.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>
#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/eventfd.h>

#include <sched.h>
#include <pthread.h>

#include <stdint.h>
#include <math.h>
#include <memory.h>
#include <sys/time.h>

#include "dma_alloc.h"
#include "RgaUtils.h"

typedef unsigned long long __u64;
typedef  unsigned int __u32;

struct dma_heap_allocation_data {
	__u64 len;
	__u32 fd;
	__u32 fd_flags;
	__u64 heap_flags;
};

struct dma_heap_import_data {
	__u64 len;
	__u32 fd;
	__u32 fd_flags;
	__u64 heap_flags;
};

#define DMA_HEAP_IOC_MAGIC		'H'
#define DMA_HEAP_IOCTL_ALLOC	_IOWR(DMA_HEAP_IOC_MAGIC, 0x0,\
				      struct dma_heap_allocation_data)
#define DMA_HEAP_IOCTL_IMPORT	_IOWR(DMA_HEAP_IOC_MAGIC, 0x1,\
				      struct dma_heap_import_data)

#define DMA_BUF_SYNC_READ      (1 << 0)
#define DMA_BUF_SYNC_WRITE     (2 << 0)
#define DMA_BUF_SYNC_RW        (DMA_BUF_SYNC_READ | DMA_BUF_SYNC_WRITE)
#define DMA_BUF_SYNC_START     (0 << 2)
#define DMA_BUF_SYNC_END       (1 << 2)

struct dma_buf_sync {
	__u64 flags;
};

#define DMA_BUF_BASE		'b'
#define DMA_BUF_IOCTL_SYNC	_IOW(DMA_BUF_BASE, 0, struct dma_buf_sync)

#define CMA_HEAP_SIZE	1024 * 1024

int dma_sync_device_to_cpu(int fd) {
    struct dma_buf_sync sync = {0};

    sync.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW;
    return ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
}

int dma_sync_cpu_to_device(int fd) {
    struct dma_buf_sync sync = {0};

    sync.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW;
    return ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
}

int dma_buf_alloc(const char *path, size_t size, int *fd, void **va) {
    int ret;
    int prot;
    void *mmap_va;
    int dma_heap_fd = -1;
    struct dma_heap_allocation_data buf_data;

    /* open dma_heap fd */
    if (dma_heap_fd < 0) {
        dma_heap_fd = open(path, O_RDWR);
        if (dma_heap_fd < 0) {
            printf("open %s fail!\n", path);
            return dma_heap_fd;
        }
    }

    /* alloc buffer */
    memset(&buf_data, 0x0, sizeof(struct dma_heap_allocation_data));

    buf_data.len = size;
    buf_data.fd_flags = O_CLOEXEC | O_RDWR;
    ret = ioctl(dma_heap_fd, DMA_HEAP_IOCTL_ALLOC, &buf_data);
    if (ret < 0) {
        printf("RK_DMA_HEAP_ALLOC_BUFFER failed\n");
        return ret;
    }

    /* mmap va */
    if (fcntl(buf_data.fd, F_GETFL) & O_RDWR)
        prot = PROT_READ | PROT_WRITE;
    else
        prot = PROT_READ;

    /* mmap contiguors buffer to user */
    mmap_va = (void *)mmap(NULL, buf_data.len, prot, MAP_SHARED, buf_data.fd, 0);
    if (mmap_va == MAP_FAILED) {
        printf("mmap failed: %s\n", strerror(errno));
        return -errno;
    }

    *va = mmap_va;
    *fd = buf_data.fd;

    close(dma_heap_fd);

    return 0;
}

void dma_buf_free(size_t size, int *fd, void *va) {
    int len;

    len =  size;
    munmap(va, len);

    close(*fd);
    *fd = -1;
}



