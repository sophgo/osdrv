#ifndef __MESH_H__
#define __MESH_H__

#include <base_cb.h>

#include "cvi_vip_dwa.h"
#include "dwa_common.h"

struct _dwa_cb_param {
	MMF_CHN_S chn;
	enum dwa_usage usage;
};

s32 mesh_dwa_do_op(struct cvi_dwa_vdev *wdev, enum dwa_usage usage
	, const void *pUsageParam, struct vb_s *vb_in, PIXEL_FORMAT_E enPixFormat
	, u64 mesh_addr, u8 sync_io, void *pcbParam, u32 cbParamSize
	, MOD_ID_E enModId, ROTATION_E enRotation);

#endif /* __MESH_H__ */
