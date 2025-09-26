#if ((CONFIG_VO_SUPPORT_PROC) && (CONFIG_SUPPORT_VO))
#include <aos/cli.h>
#include <string.h>
#include <stdio.h>
#include "disp.h"

#define VO_DISP_PRC_NAME "soph/vo_disp"
static const char * const str_disp_fmt[] = {"YUV420P", "YUV422P", "RGB888_PLANAR", "RGB888_PACKED", "BGR888_PACKED"
	, "Y_ONLY", "BF16", "", "NV12", "NV21", "NV16", "NV61", "YVYU", "YUYV", "VYUY", "UYVY"};
static const char * const str_disp_csc[] = {"Disable", "2RGB_601_Limit", "2RGB_601_Full", "2RGB_709_Limit"
	, "2RGB_709_Full", "2YUV_601_Limit", "2YUV_601_Full", "2YUV_709_Limit", "2YUV_709_Full"};

/*************************************************************************
 *	VO proc functions
 *************************************************************************/
static void show_mem(struct disp_mem *mem)
{
	printf("start_x(%3d)\t\tstart_y(%3d)\t\twidth(%4d)\t\theight(%4d)\n",
		mem->start_x, mem->start_y, mem->width, mem->height);
	printf("pitch_y(%3d)\t\tpitch_c(%3d)\n", mem->pitch_y, mem->pitch_c);
}

static void vo_disp_show_disp_status()
{
	int i;

	for (i = 0 ; i < VO_MAX_DEV_NUM; ++i) {
		struct disp_cfg *cfg = disp_get_cfg(i);
		union disp_dbg_status status = disp_get_dbg_status(i, true);
		struct disp_timing *timing = disp_get_timing(i);

		printf("--------------DISP(%d)-------------------------\n", i);
		printf("disp_from_sc(%d)\t\tsync_ext(%d)\t\ttgen_en(%d)\t\tfmt(%s)\n",
			cfg->disp_from_sc, cfg->sync_ext, cfg->tgen_en, str_disp_fmt[cfg->fmt]);
		printf("in_csc(%15s)\tout_csc(%15s)\tburst(%d)\ty_thresh(%d)\tc_thresh(%d)\n",
			str_disp_csc[cfg->in_csc], str_disp_csc[cfg->out_csc],
			cfg->burst, cfg->y_thresh, cfg->c_thresh);
		show_mem(&cfg->mem);
		printf("err_fwr_yuv(%d%d%d)\terr_erd_yuv(%d%d%d)\tlb_full_yuv(%d%d%d)\tlb_empty_yuv(%d%d%d)\n",
			status.b.err_fwr_y, status.b.err_fwr_u,  status.b.err_fwr_v, status.b.err_erd_y,
			status.b.err_erd_u, status.b.err_erd_v, status.b.lb_full_y, status.b.lb_full_u,
			status.b.lb_full_v, status.b.lb_empty_y, status.b.lb_empty_u, status.b.lb_empty_v);
		printf("bw fail(%d)\n", status.b.bw_fail);
		printf("--------------DISP-TIMING(%d))------------------\n", i);
		printf("total(%4d * %4d)\thsync_pol(%4d)\tvsync_pol(%4d)\n",
			timing->htotal, timing->vtotal, timing->hsync_pol, timing->vsync_pol);
		printf("hsync_start(%4d)\thsync_end(%4d)\tvsync_start(%4d)\tvsync_end(%4d)\n",
			timing->htotal, timing->vtotal, timing->vsync_start, timing->vsync_end);
		printf("hde-start(%4d)\t\thde-end(%4d)\tvde-start(%4d)\t\tvde-end(%4d)\t\n",
			timing->hfde_start, timing->hfde_end, timing->vfde_start, timing->vfde_end);
	}
}

static void vo_disp_proc_show(int32_t argc, char **argv)
{
	vo_disp_show_disp_status();
}

ALIOS_CLI_CMD_REGISTER(vo_disp_proc_show, proc_vo_disp, vo_disp info);
#endif