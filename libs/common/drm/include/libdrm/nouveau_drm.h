/*
 * Copyright 2005 Stephane Marchesin.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __NOUVEAU_DRM_H__
#define __NOUVEAU_DRM_H__

#define NOUVEAU_DRM_HEADER_PATCHLEVEL 16

#include "drm.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct drm_nouveau_channel_alloc {
	uint32_t     fb_ctxdma_handle;
	uint32_t     tt_ctxdma_handle;

	int          channel;
	uint32_t     pushbuf_domains;

	/* Notifier memory */
	uint32_t     notifier_handle;

	/* DRM-enforced subchannel assignments */
	struct {
		uint32_t handle;
		uint32_t grclass;
	} subchan[8];
	uint32_t nr_subchan;
};

struct drm_nouveau_channel_free {
	int channel;
};

struct drm_nouveau_grobj_alloc {
	int      channel;
	uint32_t handle;
	int      class;
};

struct drm_nouveau_notifierobj_alloc {
	uint32_t channel;
	uint32_t handle;
	uint32_t size;
	uint32_t offset;
};

struct drm_nouveau_gpuobj_free {
	int      channel;
	uint32_t handle;
};

/* FIXME : maybe unify {GET,SET}PARAMs */
#define NOUVEAU_GETPARAM_PCI_VENDOR      3
#define NOUVEAU_GETPARAM_PCI_DEVICE      4
#define NOUVEAU_GETPARAM_BUS_TYPE        5
#define NOUVEAU_GETPARAM_FB_PHYSICAL     6
#define NOUVEAU_GETPARAM_AGP_PHYSICAL    7
#define NOUVEAU_GETPARAM_FB_SIZE         8
#define NOUVEAU_GETPARAM_AGP_SIZE        9
#define NOUVEAU_GETPARAM_PCI_PHYSICAL    10
#define NOUVEAU_GETPARAM_CHIPSET_ID      11
#define NOUVEAU_GETPARAM_VM_VRAM_BASE    12
#define NOUVEAU_GETPARAM_GRAPH_UNITS     13
#define NOUVEAU_GETPARAM_PTIMER_TIME     14
#define NOUVEAU_GETPARAM_HAS_BO_USAGE    15
#define NOUVEAU_GETPARAM_HAS_PAGEFLIP    16
struct drm_nouveau_getparam {
	uint64_t param;
	uint64_t value;
};

struct drm_nouveau_setparam {
	uint64_t param;
	uint64_t value;
};

#define NOUVEAU_GEM_DOMAIN_CPU       (1 << 0)
#define NOUVEAU_GEM_DOMAIN_VRAM      (1 << 1)
#define NOUVEAU_GEM_DOMAIN_GART      (1 << 2)
#define NOUVEAU_GEM_DOMAIN_MAPPABLE  (1 << 3)
#define NOUVEAU_GEM_DOMAIN_COHERENT  (1 << 4)

#define NOUVEAU_GEM_TILE_COMP        0x00030000 /* nv50-only */
#define NOUVEAU_GEM_TILE_LAYOUT_MASK 0x0000ff00
#define NOUVEAU_GEM_TILE_16BPP       0x00000001
#define NOUVEAU_GEM_TILE_32BPP       0x00000002
#define NOUVEAU_GEM_TILE_ZETA        0x00000004
#define NOUVEAU_GEM_TILE_NONCONTIG   0x00000008

struct drm_nouveau_gem_info {
	__u32 handle;
	__u32 domain;
	__u64 size;
	__u64 offset;
	__u64 map_handle;
	__u32 tile_mode;
	__u32 tile_flags;
};

struct drm_nouveau_gem_new {
	struct drm_nouveau_gem_info info;
	__u32 channel_hint;
	__u32 align;
};

#define NOUVEAU_GEM_MAX_BUFFERS 1024
struct drm_nouveau_gem_pushbuf_bo_presumed {
	__u32 valid;
	__u32 domain;
	__u64 offset;
};

struct drm_nouveau_gem_pushbuf_bo {
	__u64 user_priv;
	__u32 handle;
	__u32 read_domains;
	__u32 write_domains;
	__u32 valid_domains;
	struct drm_nouveau_gem_pushbuf_bo_presumed presumed;
};

#define NOUVEAU_GEM_RELOC_LOW  (1 << 0)
#define NOUVEAU_GEM_RELOC_HIGH (1 << 1)
#define NOUVEAU_GEM_RELOC_OR   (1 << 2)
#define NOUVEAU_GEM_MAX_RELOCS 1024
struct drm_nouveau_gem_pushbuf_reloc {
	__u32 reloc_bo_index;
	__u32 reloc_bo_offset;
	__u32 bo_index;
	__u32 flags;
	__u32 data;
	__u32 vor;
	__u32 tor;
};

#define NOUVEAU_GEM_MAX_PUSH 512
struct drm_nouveau_gem_pushbuf_push {
	__u32 bo_index;
	__u32 pad;
	__u64 offset;
	__u64 length;
};

struct drm_nouveau_gem_pushbuf {
	__u32 channel;
	__u32 nr_buffers;
	__u64 buffers;
	__u32 nr_relocs;
	__u32 nr_push;
	__u64 relocs;
	__u64 push;
	__u32 suffix0;
	__u32 suffix1;
	__u64 vram_available;
	__u64 gart_available;
};

#define NOUVEAU_GEM_CPU_PREP_NOWAIT                                  0x00000001
#define NOUVEAU_GEM_CPU_PREP_NOBLOCK                                 0x00000002
#define NOUVEAU_GEM_CPU_PREP_WRITE                                   0x00000004
struct drm_nouveau_gem_cpu_prep {
	__u32 handle;
	__u32 flags;
};

struct drm_nouveau_gem_cpu_fini {
	__u32 handle;
};

enum nouveau_bus_type {
	NV_AGP     = 0,
	NV_PCI     = 1,
	NV_PCIE    = 2,
};

struct drm_nouveau_sarea {
};

#define DRM_NOUVEAU_GETPARAM           0x00
#define DRM_NOUVEAU_SETPARAM           0x01
#define DRM_NOUVEAU_CHANNEL_ALLOC      0x02
#define DRM_NOUVEAU_CHANNEL_FREE       0x03
#define DRM_NOUVEAU_GROBJ_ALLOC        0x04
#define DRM_NOUVEAU_NOTIFIEROBJ_ALLOC  0x05
#define DRM_NOUVEAU_GPUOBJ_FREE        0x06
#define DRM_NOUVEAU_NVIF               0x07
#define DRM_NOUVEAU_GEM_NEW            0x40
#define DRM_NOUVEAU_GEM_PUSHBUF        0x41
#define DRM_NOUVEAU_GEM_CPU_PREP       0x42
#define DRM_NOUVEAU_GEM_CPU_FINI       0x43
#define DRM_NOUVEAU_GEM_INFO           0x44

#if defined(__cplusplus)
}
#endif

#endif /* __NOUVEAU_DRM_H__ */
