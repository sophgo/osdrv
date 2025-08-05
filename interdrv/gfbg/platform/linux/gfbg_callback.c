#include "gfbg_init.h"
#include "gfbg_main.h"
#include "gfbg_cb.h"
#include "gfbg_callback.h"
#include "gfbg_debug.h"
#include "gfbg_ctrl.h"

void gfbg_vo_int_cb(vo_dev dev_id, vo_layer layer_id, union disp_intr intr_status)
{
	gfbg_par *par = NULL;
	unsigned int bind_dev_id = 0;

	par = (gfbg_par *)(g_layer[layer_id].info->par);
	// vo_gfbg_get_bind_dev_id(layer_id, &bind_dev_id);
	if (par->layer_open && (dev_id == bind_dev_id)) {
		if (intr_status.b.disp_frame_end) {
			par->refresh_info.do_refresh_job = false;
			gfbg_interrupt_process(layer_id);
		}
	}
}

int gfbg_cb(void *dev, cb_modules_id caller, unsigned int cmd, void *arg)
{
	int ret = -1;
	UNUSED(dev);

	switch (cmd) {
	case GFBG_CB_IRQ_HANDLER:
	{
		struct gfbg_int_status disp_intr_status = *(struct gfbg_int_status *)arg;

		gfbg_vo_int_cb(disp_intr_status.dev_id, disp_intr_status.layer_id,
			       disp_intr_status.intr_status);

		ret = 0;
		break;
	}

	default:
		break;
	}

	return ret;
}
