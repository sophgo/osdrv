#ifndef __RGN_CB_H__
#define __RGN_CB_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "comm_region.h"
#include "base_uapi.h"
#include "base_ctx.h"

enum rgn_cb_cmd {
	RGN_CB_MAX
};

struct _rgn_chn_size_cb_param {
	mmf_chn_s chn;
	rect_s rect;
};

struct _rgn_hdls_cb_param {
	mmf_chn_s chn;
	rgn_handle *hdls;
	rgn_type_e type;
	__u32 layer;
};

struct _rgn_cfg_cb_param {
	mmf_chn_s chn;
	struct rgn_cfg rgn_cfg;
	__u32 layer;
};

struct _rgn_ex_cfg_cb_param {
	mmf_chn_s chn;
	struct rgn_ex_cfg rgn_ex_cfg;
};

struct _rgn_coverex_cfg_cb_param {
	mmf_chn_s chn;
	struct rgn_coverex_cfg rgn_coverex_cfg;
};

struct _rgn_mosaic_cfg_cb_param {
	mmf_chn_s chn;
	struct rgn_mosaic_cfg rgn_mosaic_cfg;
};


struct _rgn_lut_cb_param {
	mmf_chn_s chn;
	struct rgn_lut_cfg lut_cfg;
};

struct _rgn_get_ow_addr_cb_param {
	mmf_chn_s chn;
	rgn_handle handle;
	__u32 layer;
	__u64 addr;
};
struct _rgn_clr_ow_addr_cb_param {
	mmf_chn_s chn;
	rgn_handle handle;
	__u32 layer;
	__u8 ow_inst;
};

struct _rgn_is_addr_in_use_cb_param {
	mmf_chn_s chn;
	rgn_handle handle;
	__u32 layer;
	__u64 addr;     // [in] physical address to check
	__u8 in_use;    // [out] whether the address is currently in use
};

#ifdef __cplusplus
}
#endif

#endif /* __RGN_CB_H__ */
