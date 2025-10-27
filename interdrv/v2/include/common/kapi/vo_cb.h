#ifndef __VO_CB_H__
#define __VO_CB_H__

#ifdef __cplusplus
	extern "C" {
#endif
#include <ldc_cb.h>

enum vo_cb_cmd {
	VO_CB_IRQ_HANDLER,
	VO_CB_GET_RGN_HDLS,
	VO_CB_SET_RGN_HDLS,
	VO_CB_SET_RGN_CFG,
	VO_CB_SET_RGN_COVEREX_CFG,
	VO_CB_GET_CHN_SIZE,
	VO_CB_QBUF_TRIGGER,
	VO_CB_QBUF_VO_GET_CHN_ROTATION,
	VO_CB_GET_RETRAIN_INFO,
	VO_CB_GDC_OP_DONE = LDC_CB_GDC_OP_DONE,
	VO_CB_MAX
};

struct vo_get_chnrotation_cfg {
	u8 layer;
	u8 chn;
	u8 rotation;
};

struct vo_retrain_info {
	bool is_vo_en;
	u64 frame_end;
	u64 margin;
};

#ifdef __cplusplus
}
#endif

#endif /* __VO_CB_H__ */
