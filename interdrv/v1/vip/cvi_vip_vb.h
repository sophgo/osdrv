/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_vip_vb.h
 * Description:
 */

#ifndef __CVI_VIP_VB_H__
#define __CVI_VIP_VB_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/kernel.h>
#include <linux/version.h>

#define VIP_MAX_PLANES 3

struct cvi_size {
	u16 width;
	u16 height;
};

struct cvi_rect {
	s16 left;
	s16 top;
	u16 width;
	u16 height;
};

enum cvi_vb_state {
	VB_STATE_DEQUEUED,
	VB_STATE_PREPARING,
	VB_STATE_PREPARED,
	VB_STATE_QUEUED,
	VB_STATE_REQUEUEING,
	VB_STATE_ACTIVE,
	VB_STATE_DONE,
	VB_STATE_ERROR,
};

struct cvi_vb_plane {
	void			*mem_priv;
	u32			length;
	u64			phy_addr;
};

/**
 * struct vb2_buffer - represents a video buffer
 * @num_planes:		number of planes in the buffer
 *			on an internal driver queue
 * @planes:		private per-plane information; do not change
 */
struct cvi_vb {
	unsigned int		num_planes;
	struct cvi_vb_plane	planes[VIP_MAX_PLANES];
	enum cvi_vb_state	state;
};

#endif  // __CVI_VIP_VB_H__
