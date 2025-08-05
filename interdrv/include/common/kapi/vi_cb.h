#ifndef __VI_CB_H__
#define __VI_CB_H__

#ifdef __cplusplus
	extern "C" {
#endif
#include "ldc_cb.h"
#include "comm_vi.h"

enum VI_CB_CMD {
	VI_CB_QBUF_TRIGGER,
	VI_CB_SC_FRM_DONE,
	VI_CB_SET_VIVPSSMODE,
	VI_CB_GDC_OP_DONE = LDC_CB_GDC_OP_DONE,
	VI_CB_GDC_GET_CHN_ATTR,
	VI_CB_GDC_SET_CHN_CFG,
	VI_CB_MSP_CFG,
	VI_CB_MSP_GET_INFO,
	VI_CB_RESET_ISP,
	VI_CB_MAX
};

struct vi_vpss_online {
	__u8   raw_num;
	__u8   is_vpss_online;
};

struct vi_ldc_chn_attr {
	vi_pipe vi_pipe;
	vi_chn vi_chn;
	size_s size;
};

struct vi_ldc_internal_chn_attr {
	mod_id_e mod;
	struct vi_ldc_chn_attr vi_chn_attr;
};

struct vi_cb_chn_ldc_cfg {
	__s32 vi_pipe;
	__s32 vi_chn;
	rotation_e rotation;
	vi_ldc_attr_s ldc_attr;
	__u64 mesh_handle;
};

struct vi_ldc_internal_chn_ldc_cfg {
	mod_id_e mod;
	struct vi_cb_chn_ldc_cfg vi_cfg;
};

struct vi_cb_msp_cfg {
	__s32 vi_pipe;
	__s32 vi_chn;
	__u32 width;
	__u32 height;
	bool is_resize;
};

struct vi_cb_msp_info {
	__s32 vi_pipe;
	__s32 vi_chn;
	__u64 frm_num;
	struct mlv_i_s mlv_i;
};

#ifdef __cplusplus
}
#endif

#endif /* __VI_CB_H__ */
