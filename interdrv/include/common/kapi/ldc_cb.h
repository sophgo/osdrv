#ifndef __LDC_CB_H__
#define __LDC_CB_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "comm_sys.h"
#include "comm_vb.h"
#include "base_ctx.h"

enum gdc_usage {
	GDC_USAGE_ROTATION,
	GDC_USAGE_FISHEYE,
	GDC_USAGE_LDC,
	GDC_USAGE_MAX
};

typedef void (*gdc_cb)(void *, vb_blk);

struct mesh_gdc_cfg {
	enum gdc_usage usage;
	const void *usage_param;
	struct vb_s *vb_in;
	pixel_format_e pix_format;
	unsigned long long mesh_addr;
	unsigned char sync_io;
	void *cb_param;
	unsigned int cb_param_size;
	rotation_e rotation;
};

struct ldc_op_done_cfg {
	void *param;
	vb_blk blk;
};

enum ldc_cb_cmd {
	LDC_CB_MESH_GDC_OP,
	LDC_CB_VPSS_SBM_DONE,
	LDC_CB_GDC_OP_DONE = 100,	/* Skip VI/VPSS/VO self cmd */
	LDC_CB_GDC_GET_CHN_ATTR,
	LDC_CB_GDC_SET_CHN_CFG,
	LDC_CB_MAX
};

#ifdef __cplusplus
}
#endif

#endif /* __LDC_CB_H__ */
