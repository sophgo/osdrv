#ifndef _LDC_COMMON_MESH_H
#define _LDC_COMMON_MESH_H

#include <ldc_cb.h>

#define CVI_GDC_MAGIC 0xbabeface
#define CVI_GDC_MESH_SIZE_ROT 0x60000

struct _gdc_cb_param {
	MMF_CHN_S chn;
	enum GDC_USAGE usage;
};

s32 mesh_gdc_do_op(struct cvi_ldc_vdev *wdev, enum GDC_USAGE usage,
		   const void *pUsageParam, struct vb_s *vb_in,
		   PIXEL_FORMAT_E enPixFormat, u64 mesh_addr, u8 sync_io,
		   void *pcbParam, u32 cbParamSize, MOD_ID_E enModId,
		   ROTATION_E enRotation);

#endif /* _LDC_COMMON_MESH_H */
