#ifndef __GFBG_CB_H__
#define __GFBG_CB_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "disp.h"

enum gfbg_cb_cmd {
	GFBG_CB_IRQ_HANDLER,
	GFBG_CB_MAX
};

struct gfbg_int_status {
	int dev_id;
	u32 layer_id;
	union disp_intr intr_status;
};

#ifdef __cplusplus
}
#endif

#endif /* __GFBG_CB_H__ */