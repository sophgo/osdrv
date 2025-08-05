#ifndef __VENC_CB_H__
#define __VENC_CB_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "base_ctx.h"
#include "base_cb.h"

enum VENC_CB_CMD {
	VENC_CB_SET_SBM_INFO,
	VENC_CB_SWITCH_CHN,
	VENC_CB_SKIP_FRM,
	VENC_CB_SNAP_JPG_FRM,
	VENC_CB_OVERFLOW_CHECK,
	VENC_CB_MAX
};

struct venc_sbm_info {
	s32 venc_chn;
	struct sbm_cfg st_sbm_cfg;
};

struct venc_skip_frm_info {
	s32 venc_chn;
};

struct venc_switch_chn {
	s32 venc_chn;
	u8 snr_num;
	u64 frm_num;
};

struct venc_snap_frm_info {
	s32 venc_chn;
	u32 frm_cnt;
};

#ifdef __cplusplus
}
#endif

#endif /* __VENC_CB_H__ */

