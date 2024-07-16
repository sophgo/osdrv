#ifndef __MESH_H__
#define __MESH_H__

#include <base_cb.h>

#include "dwa_core.h"
#include "dwa_common.h"

struct _dwa_cb_param {
	mmf_chn_s chn;
	enum dwa_usage usage;
};

int mesh_dwa_do_op(struct dwa_vdev *wdev, enum dwa_usage usage
	, const void *usage_param, struct vb_s *vb_in, pixel_format_e pix_format
	, unsigned long long mesh_addr, unsigned char sync_io, void *cb_param, unsigned int cb_param_size
	, mod_id_e mod_id, rotation_e rotation);

#endif /* __MESH_H__ */
