
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/sys.h>
#include <linux/of_gpio.h>

#include <linux/cvi_common.h>
#include <linux/cvi_vip.h>
#include <linux/cvi_defines.h>
#include <linux/cvi_buffer.h>

#include <vo.h>
#include <vb.h>
//#include <linux/cvi_base_ctx.h>
#include <vo_common.h>
#include <vo_core.h>
#include <vo_defines.h>
#include <vo_interfaces.h>
#include "vo_mipi_tx.h"
#include <proc/vo_proc.h>
#include <proc/vo_disp_proc.h>

#include "pinctrl-cv1822.h"
//#include "mw/vpu_base.h"
#include <base_cb.h>
#include <vpss_cb.h>
#include <vo_cb.h>
#include <ldc_cb.h>
#include <rgn_cb.h>
#include "vo_rgn_ctrl.h"

#include "base_common.h"
#include "vbq.h"
//include hearder from vpss
#include "dsi_phy.h"

/*******************************************************
 *  MACRO defines
 ******************************************************/
#define SEM_WAIT_TIMEOUT_MS  200
#define VO_PROFILE
/*******************************************************
 *  Global variables
 ******************************************************/
s32 vo_log_lv = CVI_DBG_WARN;
s32 smooth;
s32 debug;
s32 job_init;
static bool hide_vo;
struct platform_device *g_pdev;

module_param(vo_log_lv, int, 0644);
module_param(hide_vo, bool, 0444);

struct _vo_gdc_cb_param {
	MMF_CHN_S chn;
	enum GDC_USAGE usage;
};

struct cvi_vo_ctx *gVoCtx;
struct cvi_vo_dev *gVdev;
static u8 i80_ctrl[I80_CTRL_MAX] = { 0x31, 0x75, 0xff };
static struct mutex vo_gdc_lock;
static atomic_t  dev_open_cnt;
struct vb_jobs_t stVOjobs[VO_MAX_LAYER_NUM];

static void _disp_sel_pinmux(enum cvi_disp_intf intf_type, void *param)
{
	if (intf_type == CVI_VIP_DISP_INTF_I80) {
		PINMUX_CONFIG(PAD_MIPIRX2N, VO_D_10);
		PINMUX_CONFIG(PAD_MIPIRX2P, VO_D_9);
		PINMUX_CONFIG(PAD_MIPIRX1N, VO_D_8);
		PINMUX_CONFIG(PAD_MIPIRX1P, VO_D_7);
		PINMUX_CONFIG(PAD_MIPIRX0N, VO_D_6);
		PINMUX_CONFIG(PAD_MIPIRX0P, VO_D_5);
		PINMUX_CONFIG(PAD_MIPI_TXM2, VO_D_0);
		PINMUX_CONFIG(PAD_MIPI_TXP2, VO_CLK0);
		PINMUX_CONFIG(PAD_MIPI_TXM1, VO_D_2);
		PINMUX_CONFIG(PAD_MIPI_TXP1, VO_D_1);
		PINMUX_CONFIG(PAD_MIPI_TXM0, VO_D_4);
		PINMUX_CONFIG(PAD_MIPI_TXP0, VO_D_3);
	} else if (intf_type == CVI_VIP_DISP_INTF_LVDS) {
		PINMUX_CONFIG(PAD_MIPI_TXM0, XGPIOC_12);
		PINMUX_CONFIG(PAD_MIPI_TXP0, XGPIOC_13);
		PINMUX_CONFIG(PAD_MIPI_TXM1, XGPIOC_14);
		PINMUX_CONFIG(PAD_MIPI_TXP1, XGPIOC_15);
		PINMUX_CONFIG(PAD_MIPI_TXM2, XGPIOC_16);
		PINMUX_CONFIG(PAD_MIPI_TXP2, XGPIOC_17);
		PINMUX_CONFIG(PAD_MIPI_TXM3, XGPIOC_20);
		PINMUX_CONFIG(PAD_MIPI_TXP3, XGPIOC_21);
		PINMUX_CONFIG(PAD_MIPI_TXM4, XGPIOC_18);
		PINMUX_CONFIG(PAD_MIPI_TXP4, XGPIOC_19);

	} else if (intf_type == CVI_VIP_DISP_INTF_BT) {
		struct cvi_bt_intf_cfg *cfg = param;

		if (cfg->mode == BT_MODE_601) {
			PINMUX_CONFIG(PAD_MIPIRX2N, VO_D_10);
			PINMUX_CONFIG(PAD_MIPIRX2P, VO_D_9);
			PINMUX_CONFIG(PAD_MIPIRX1N, VO_D_8);
		}
		PINMUX_CONFIG(PAD_MIPIRX1P, VO_D_7);
		PINMUX_CONFIG(PAD_MIPIRX0N, VO_D_6);
		PINMUX_CONFIG(PAD_MIPIRX0P, VO_D_5);
		PINMUX_CONFIG(PAD_MIPI_TXM2, VO_D_0);
		PINMUX_CONFIG(PAD_MIPI_TXP2, VO_CLK0);
		PINMUX_CONFIG(PAD_MIPI_TXM1, VO_D_2);
		PINMUX_CONFIG(PAD_MIPI_TXP1, VO_D_1);
		PINMUX_CONFIG(PAD_MIPI_TXM0, VO_D_4);
		PINMUX_CONFIG(PAD_MIPI_TXP0, VO_D_3);
	}
}

static void _disp_ctrlpin_set(u32 gpio_num, enum GPIO_ACTIVE_E active)
{
	enum of_gpio_flags flags;
	static s32 count;
	s8 name[16] = "";
	s32 rc = 0;

	if (gpio_is_valid(gpio_num)) {
		flags = GPIOF_DIR_OUT | (active ? GPIOF_INIT_HIGH : GPIOF_INIT_LOW);
		snprintf(name, sizeof(name), "disp_ctrl_pin_%d", count++);
		rc = devm_gpio_request_one(&g_pdev->dev, gpio_num, flags, name);
		if (rc) {
			CVI_TRACE_VO(CVI_DBG_ERR, "gpio_num(%d) failed\n",  gpio_num);
			return;
		}
		gpio_set_value(gpio_num, active);
	}
}

const struct vo_disp_pattern patterns[CVI_VIP_PAT_MAX] = {
	{.type = SCL_PAT_TYPE_OFF,	.color = SCL_PAT_COLOR_MAX},
	{.type = SCL_PAT_TYPE_SNOW, .color = SCL_PAT_COLOR_MAX},
	{.type = SCL_PAT_TYPE_AUTO, .color = SCL_PAT_COLOR_MAX},
	{.type = SCL_PAT_TYPE_FULL, .color = SCL_PAT_COLOR_RED},
	{.type = SCL_PAT_TYPE_FULL, .color = SCL_PAT_COLOR_GREEN},
	{.type = SCL_PAT_TYPE_FULL, .color = SCL_PAT_COLOR_BLUE},
	{.type = SCL_PAT_TYPE_FULL, .color = SCL_PAT_COLOR_BAR},
	{.type = SCL_PAT_TYPE_H_GRAD, .color = SCL_PAT_COLOR_WHITE},
	{.type = SCL_PAT_TYPE_V_GRAD, .color = SCL_PAT_COLOR_WHITE},
	{.type = SCL_PAT_TYPE_FULL, .color = SCL_PAT_COLOR_USR,
	.rgb = {0, 0, 0} },
};

/*******************************************************
 *  Internal APIs
 ******************************************************/

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))

#endif //(LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
extern bool __clk_is_enabled(struct clk *clk);

static s32 _vo_create_proc(struct cvi_vo_dev *vdev)
{
	s32 ret = 0;

	/* vo proc setup */
	vdev->shared_mem = kzalloc(VO_SHARE_MEM_SIZE, GFP_ATOMIC);
	if (!vdev->shared_mem) {
		//pr_err("shared_mem alloc size(%d) failed\n", VO_SHARE_MEM_SIZE);
		return -ENOMEM;
	}

	if (vo_proc_init(vdev, vdev->shared_mem) < 0) {
		pr_err("vo proc init failed\n");
		return -EAGAIN;
	}

	if (vo_disp_proc_init(vdev) < 0) {
		pr_err("vo_dbg proc init failed\n");
		return -EAGAIN;
	}
	return ret;
}

static s32 _vo_destroy_proc(struct cvi_vo_dev *vdev)
{
	s32 ret = 0;

	vo_proc_remove();
	vo_disp_proc_remove();

	kfree(vdev->shared_mem);
	vdev->shared_mem = NULL;

	return ret;
}
u8 _gop_get_bpp(enum sclr_gop_format fmt)
{
	return (fmt == SCL_GOP_FMT_ARGB8888) ? 4 :
		(fmt == SCL_GOP_FMT_256LUT) ? 1 : 2;
}

s32 vo_set_interface(struct cvi_vo_dev *vdev, struct cvi_disp_intf_cfg *cfg)
{
	s32 rc = -1;

	if (smooth) {
		CVI_TRACE_VO(CVI_DBG_DEBUG, "V4L2_CID_DV_VIP_DISP_INTF won't apply if smooth.\n");
		sclr_disp_reg_force_up();
		vdev->disp_interface = cfg->intf_type;

		rc = 0;
		return rc;
	}

	if (atomic_read(&vdev->disp_streamon) == 1) {
		CVI_TRACE_VO(CVI_DBG_INFO, "V4L2_CID_DV_VIP_DISP_ONLINE can't be control if streaming.\n");
		rc = 0;

		return rc;
	}

	if (cfg->intf_type == CVI_VIP_DISP_INTF_DSI) {
		CVI_TRACE_VO(CVI_DBG_INFO, "MIPI use mipi_tx to control.\n");

		rc = 0;
		return rc;
	} else if (cfg->intf_type == CVI_VIP_DISP_INTF_LVDS) {
		s32 i = 0;
		union sclr_lvdstx lvds_reg;
		bool data_en[LANE_MAX_NUM] = {false, false, false, false, false};
		struct clk *clk_disp, *clk_dsi;

		clk_disp = devm_clk_get(&g_pdev->dev, clk_disp_name);
		if (IS_ERR(clk_disp)) {
			CVI_TRACE_VO(CVI_DBG_ERR, "devm_clk_get clk_disp failed.\n");
			return rc;
		}
		if (clk_disp)
			clk_prepare_enable(clk_disp);

		clk_dsi = devm_clk_get(&g_pdev->dev, clk_dsi_name);
		if (IS_ERR(clk_dsi)) {
			CVI_TRACE_VO(CVI_DBG_ERR, "devm_clk_get clk_dsi failed.\n");
			return rc;
		}
		if (clk_dsi)
			clk_prepare_enable(clk_dsi);

		for (i = 0; i < LANE_MAX_NUM; i++) {
			if ((cfg->lvds_cfg.lane_id[i] < 0) ||
				(cfg->lvds_cfg.lane_id[i] >= LANE_MAX_NUM)) {
				dphy_dsi_set_lane(i, DSI_LANE_MAX, false, false);
				continue;
			}
			dphy_dsi_set_lane(i, cfg->lvds_cfg.lane_id[i],
					  cfg->lvds_cfg.lane_pn_swap[i], false);
			if (cfg->lvds_cfg.lane_id[i] != MIPI_TX_LANE_CLK) {
				data_en[cfg->lvds_cfg.lane_id[i] - 1] = true;
			}
		}
		dphy_dsi_lane_en(true, data_en, false);
		_disp_sel_pinmux(cfg->intf_type, &cfg->lvds_cfg);
		_disp_ctrlpin_set(cfg->lvds_cfg.backlight_gpio_num, cfg->lvds_cfg.backlight_avtive);

		sclr_disp_set_intf(SCLR_VO_INTF_LVDS);

		if (cfg->lvds_cfg.pixelclock == 0) {
			CVI_TRACE_VO(CVI_DBG_ERR, "lvds pixelclock 0 invalid\n");
			return rc;
		}
		lvds_reg.b.out_bit = cfg->lvds_cfg.out_bits;
		lvds_reg.b.vesa_mode = cfg->lvds_cfg.mode;
		if (cfg->lvds_cfg.chn_num == 1)
			lvds_reg.b.dual_ch = 0;
		else if (cfg->lvds_cfg.chn_num == 2)
			lvds_reg.b.dual_ch = 1;
		else {
			lvds_reg.b.dual_ch = 0;
			CVI_TRACE_VO(CVI_DBG_ERR, "invalid lvds chn_num(%d). Use 1 instead."
				, cfg->lvds_cfg.chn_num);
		}
		lvds_reg.b.vs_out_en = cfg->lvds_cfg.vs_out_en;
		lvds_reg.b.hs_out_en = cfg->lvds_cfg.hs_out_en;
		lvds_reg.b.hs_blk_en = cfg->lvds_cfg.hs_blk_en;
		lvds_reg.b.ml_swap = cfg->lvds_cfg.msb_lsb_data_swap;
		lvds_reg.b.ctrl_rev = cfg->lvds_cfg.serial_msb_first;
		lvds_reg.b.oe_swap = cfg->lvds_cfg.even_odd_link_swap;
		lvds_reg.b.en = cfg->lvds_cfg.enable;
		dphy_lvds_set_pll(cfg->lvds_cfg.pixelclock, cfg->lvds_cfg.chn_num);
		sclr_lvdstx_set(lvds_reg);
	} else if (cfg->intf_type == CVI_VIP_DISP_INTF_I80) {
		union sclr_bt_enc enc;
		union sclr_bt_sync_code sync;

		_disp_sel_pinmux(cfg->intf_type, &cfg->bt_cfg);
		sclr_disp_set_intf(SCLR_VO_INTF_I80);
		enc.raw = 0;
		enc.b.fmt_sel = 2;
		enc.b.clk_inv = 1;
		sync.raw = 0;
		sync.b.sav_vld = 0x80;
		sync.b.sav_blk = 0xab;
		sync.b.eav_vld = 0x9d;
		sync.b.eav_blk = 0xb6;
		sclr_bt_set(enc, sync);
	} else if (cfg->intf_type == CVI_VIP_DISP_INTF_BT) {
		union sclr_bt_enc enc;
		union sclr_bt_sync_code sync;

		if (cfg->bt_cfg.mode == BT_MODE_1120)
			sclr_disp_set_intf(SCLR_VO_INTF_BT1120);
		else if (cfg->bt_cfg.mode == BT_MODE_656)
			sclr_disp_set_intf(SCLR_VO_INTF_BT656);
		else if (cfg->bt_cfg.mode == BT_MODE_601)
			sclr_disp_set_intf(SCLR_VO_INTF_BT601);
		else {
			CVI_TRACE_VO(CVI_DBG_ERR, "invalid bt-mode(%d)\n", cfg->bt_cfg.mode);
			//return rc;
		}

		//enable clk_bt
		clk_prepare_enable(vdev->clk_bt);

		//set clock to 54Mhz
		dphy_dsi_set_bt656_54mhz(0, 0);

		//set csc value
		sclr_disp_set_out_csc(SCL_CSC_601_FULL_RGB2YUV);
		_disp_sel_pinmux(cfg->intf_type, &cfg->bt_cfg);

		enc.raw = 0;
		enc.b.fmt_sel = cfg->bt_cfg.mode;
		sync.b.sav_vld = 0x80;
		sync.b.sav_blk = 0xab;
		sync.b.eav_vld = 0x9d;
		sync.b.eav_blk = 0xb6;
		sclr_bt_set(enc, sync);
	} else {
		CVI_TRACE_VO(CVI_DBG_ERR, "invalid disp-intf(%d)\n", cfg->intf_type);
		return rc;
	}
	sclr_disp_reg_force_up();

	vdev->disp_interface = cfg->intf_type;

	rc = 0;
	return rc;
}

s32 vo_set_rgn_cfg(const u8 inst, const struct cvi_rgn_cfg *cfg, const struct sclr_size *size)
{
	u8 i, layer = 0;
	struct sclr_gop_cfg *gop_cfg = sclr_gop_get_cfg(inst, layer);
	struct sclr_gop_ow_cfg *ow_cfg;

	gop_cfg->gop_ctrl.raw &= ~0xfff;
	gop_cfg->gop_ctrl.b.hscl_en = cfg->hscale_x2;
	gop_cfg->gop_ctrl.b.vscl_en = cfg->vscale_x2;
	gop_cfg->gop_ctrl.b.colorkey_en = cfg->colorkey_en;
	gop_cfg->colorkey = cfg->colorkey;

	for (i = 0; i < cfg->num_of_rgn; ++i) {
		u8 bpp = _gop_get_bpp((enum sclr_gop_format)cfg->param[i].fmt);

		ow_cfg = &gop_cfg->ow_cfg[i];
		gop_cfg->gop_ctrl.raw |= BIT(i);

		ow_cfg->fmt = (enum sclr_gop_format)cfg->param[i].fmt;
		ow_cfg->addr = cfg->param[i].phy_addr;
		ow_cfg->pitch = cfg->param[i].stride;
		if (cfg->param[i].rect.left < 0) {
			ow_cfg->start.x = 0;
			ow_cfg->addr -= bpp * cfg->param[i].rect.left;
			ow_cfg->img_size.w = cfg->param[i].rect.width + cfg->param[i].rect.left;
		} else if ((cfg->param[i].rect.left + cfg->param[i].rect.width) > size->w) {
			ow_cfg->start.x = cfg->param[i].rect.left;
			ow_cfg->img_size.w = size->w - cfg->param[i].rect.left;
			ow_cfg->mem_size.w = cfg->param[i].stride;
		} else {
			ow_cfg->start.x = cfg->param[i].rect.left;
			ow_cfg->img_size.w = cfg->param[i].rect.width;
			ow_cfg->mem_size.w = cfg->param[i].stride;
		}

		if (cfg->param[i].rect.top < 0) {
			ow_cfg->start.y = 0;
			ow_cfg->addr -= ow_cfg->pitch * cfg->param[i].rect.top;
			ow_cfg->img_size.h = cfg->param[i].rect.height + cfg->param[i].rect.top;
		} else if ((cfg->param[i].rect.top + cfg->param[i].rect.height) > size->h) {
			ow_cfg->start.y = cfg->param[i].rect.top;
			ow_cfg->img_size.h = size->h - cfg->param[i].rect.top;
		} else {
			ow_cfg->start.y = cfg->param[i].rect.top;
			ow_cfg->img_size.h = cfg->param[i].rect.height;
		}

		ow_cfg->end.x = ow_cfg->start.x + (ow_cfg->img_size.w << gop_cfg->gop_ctrl.b.hscl_en)
						- gop_cfg->gop_ctrl.b.hscl_en;
		ow_cfg->end.y = ow_cfg->start.y + (ow_cfg->img_size.h << gop_cfg->gop_ctrl.b.vscl_en)
						- gop_cfg->gop_ctrl.b.vscl_en;
		ow_cfg->mem_size.w = ALIGN(ow_cfg->img_size.w * bpp, GOP_ALIGNMENT);
		ow_cfg->mem_size.h = ow_cfg->img_size.h;
#if 0
		CVI_TRACE_VO(CVI_DBG_INFO, "gop(%d) fmt(%d) rect(%d %d %d %d) addr(%llx) pitch(%d).\n", inst
			, ow_cfg->fmt, ow_cfg->start.x, ow_cfg->start.y, ow_cfg->img_size.w, ow_cfg->img_size.h
			, ow_cfg->addr, ow_cfg->pitch);
#endif
		sclr_gop_ow_set_cfg(inst, layer, i, ow_cfg, true);
	}

	sclr_gop_set_cfg(inst, layer, gop_cfg, true);

	return 0;
}

static void vo_set_rgn_coverex_cfg(struct cvi_rgn_coverex_cfg *cfg)
{
	s32 i;
	struct sclr_cover_cfg sc_cover_cfg;

	for (i = 0; i < RGN_COVEREX_MAX_NUM; i++) {
		if (cfg->rgn_coverex_param[i].enable) {
			sc_cover_cfg.start.raw = 0;
			sc_cover_cfg.color.raw = 0;
			sc_cover_cfg.start.b.enable = 1;
			sc_cover_cfg.start.b.x = cfg->rgn_coverex_param[i].rect.left;
			sc_cover_cfg.start.b.y = cfg->rgn_coverex_param[i].rect.top;
			sc_cover_cfg.img_size.w = cfg->rgn_coverex_param[i].rect.width;
			sc_cover_cfg.img_size.h = cfg->rgn_coverex_param[i].rect.height;
			sc_cover_cfg.color.b.cover_color_r = (cfg->rgn_coverex_param[i].color >> 16) & 0xff;
			sc_cover_cfg.color.b.cover_color_g = (cfg->rgn_coverex_param[i].color >> 8) & 0xff;
			sc_cover_cfg.color.b.cover_color_b = cfg->rgn_coverex_param[i].color & 0xff;
			sclr_cover_set_cfg(SCL_MAX_INST, i, &sc_cover_cfg);
		}
	}
}

/*******************************************************
 *  File operations for core
 ******************************************************/
void vo_fill_disp_timing(struct sclr_disp_timing *timing,
		struct vo_bt_timings *bt_timing)
{

	timing->vtotal = VO_DV_BT_FRAME_HEIGHT(bt_timing) - 1;
	timing->htotal = VO_DV_BT_FRAME_WIDTH(bt_timing) - 1;
	timing->vsync_start = 1;
	timing->vsync_end = timing->vsync_start + bt_timing->vsync - 1;
	timing->vfde_start = timing->vmde_start =
		timing->vsync_start + bt_timing->vsync + bt_timing->vbackporch;
	timing->vfde_end = timing->vmde_end =
		timing->vfde_start + bt_timing->height - 1;
	timing->hsync_start = 1;
	timing->hsync_end = timing->hsync_start + bt_timing->hsync - 1;
	timing->hfde_start = timing->hmde_start =
		timing->hsync_start + bt_timing->hsync + bt_timing->hbackporch;
	timing->hfde_end = timing->hmde_end =
		timing->hfde_start + bt_timing->width - 1;
	timing->vsync_pol = bt_timing->polarities & VO_DV_VSYNC_POS_POL;
	timing->hsync_pol = bt_timing->polarities & VO_DV_HSYNC_POS_POL;
}

struct cvi_disp_buffer *vo_next_buf(struct cvi_vo_dev *vdev)
{
	unsigned long flags;
	struct cvi_disp_buffer *b = NULL;
	s32 i = 0;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	if (!list_empty(&vdev->rdy_queue))
		b = list_first_entry(&vdev->rdy_queue,
			struct cvi_disp_buffer, list);
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);

	for (i = 0; i < 3; i++) {
		CVI_TRACE_VO(CVI_DBG_DEBUG, "qbuf->buf.planes[%d].addr=%llx\n", i, b->buf.planes[i].addr);
	}

	return b;
}

struct cvi_disp_buffer *vo_buf_remove(struct cvi_vo_dev *vdev)
{
	unsigned long flags;
	struct cvi_disp_buffer *b = NULL;

	if (vdev->num_rdy == 0)
		return b;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	if (!list_empty(&vdev->rdy_queue)) {
		b = list_first_entry(&vdev->rdy_queue,
			struct cvi_disp_buffer, list);
		list_del_init(&b->list);
		vfree(b);
		--vdev->num_rdy;
	}
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);

	return b;
}

static void _vo_hw_enque(struct cvi_vo_dev *vdev)
{
	struct vo_buffer *vb2_buf;
	struct cvi_disp_buffer *b = NULL;
	s32 i = 0;

	struct sclr_disp_cfg *cfg;

	if (vdev->disp_online)
		return;
	b = vo_next_buf(vdev);
	if (!b) {
		CVI_TRACE_VO(CVI_DBG_ERR, "Ready queue buffer empty\n");
		return;
	}
	vb2_buf = &b->buf;

	for (i = 0; i < 3; i++) {
		CVI_TRACE_VO(CVI_DBG_DEBUG, "b->buf.planes[%d].addr=%llx\n", i, b->buf.planes[i].addr);
	}

	cfg = sclr_disp_get_cfg();

	cfg->mem.addr0 = b->buf.planes[0].addr;
	cfg->mem.addr1 = b->buf.planes[1].addr;
	cfg->mem.addr2 = b->buf.planes[2].addr;
	cfg->mem.pitch_y = (b->buf.planes[0].bytesused > vdev->bytesperline[0])
			 ? b->buf.planes[0].bytesused
			 : vdev->bytesperline[0];
	cfg->mem.pitch_c = (b->buf.planes[1].bytesused > vdev->bytesperline[1])
			 ? b->buf.planes[1].bytesused
			 : vdev->bytesperline[1];

	sclr_disp_set_mem(&cfg->mem);

	if (vdev->disp_interface == CVI_VIP_DISP_INTF_I80) {
		sclr_disp_reg_force_up();
		sclr_i80_run();
	}
}

void vo_wake_up_th(struct cvi_vo_dev *vdev)
{
	CVI_TRACE_VO(CVI_DBG_INFO, "wake up th when vb buffer done\n");
	vdev->vo_th[E_VO_TH_DISP].flag = 1;
	wake_up(&vdev->vo_th[E_VO_TH_DISP].wq);
}

void vo_buf_queue(struct cvi_vo_dev *vdev, struct cvi_disp_buffer *b)
{
	unsigned long flags;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	list_add_tail(&b->list, &vdev->rdy_queue);
	++vdev->num_rdy;
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);
}

static void vo_disp_buf_queue(struct cvi_vo_dev *vdev, struct cvi_disp_buffer *b)
{
	vo_buf_queue(vdev, b);
	if (vdev->num_rdy == 1) {
		if (gVoCtx->is_layer_enable[0]) {

			CVI_TRACE_VO(CVI_DBG_INFO, "vo_disp_buf_queue set bgcolor\n");
			sclr_disp_enable_window_bgcolor(false);
		}
		_vo_hw_enque(vdev);
		if (vdev->disp_interface == CVI_VIP_DISP_INTF_I80) {
			vo_buf_remove((struct cvi_vo_dev *)vdev);

			vo_wake_up_th((struct cvi_vo_dev *)vdev);
		}
	}
}

static void _i80_package_eol(u8 buffer[3])
{
	// pull high i80-lane
	buffer[0] = 0xff;
	buffer[1] = i80_ctrl[I80_CTRL_EOF];
	buffer[2] = I80_OP_GO;
}

static void _i80_package_eof(u8 buffer[3])
{
	buffer[0] = 0x00;
	buffer[1] = i80_ctrl[I80_CTRL_EOF];
	buffer[2] = I80_OP_DONE;
}

static void _get_frame_rgb(PIXEL_FORMAT_E fmt, u8 **buf, u32 *stride, u16 x, u16 y,
	u8 *r, u8 *g, u8 *b)
{
	if (fmt == PIXEL_FORMAT_RGB_888) {
		u32 offset = 3 * x + stride[0] * y;

		*r = *(buf[0] + offset);
		*g = *(buf[0] + offset + 1);
		*b = *(buf[0] + offset + 2);
	} else if (fmt == PIXEL_FORMAT_BGR_888) {
		u32 offset = 3 * x + stride[0] * y;

		*b = *(buf[0] + offset);
		*g = *(buf[0] + offset + 1);
		*r = *(buf[0] + offset + 2);
	} else if (fmt == PIXEL_FORMAT_RGB_888_PLANAR) {
		u32 offset = x + stride[0] * y;

		*r = *(buf[0] + offset);
		*g = *(buf[1] + offset);
		*b = *(buf[2] + offset);
	} else if (fmt == PIXEL_FORMAT_BGR_888_PLANAR) {
		u32 offset = x + stride[0] * y;

		*b = *(buf[0] + offset);
		*g = *(buf[1] + offset);
		*r = *(buf[2] + offset);
	} else {
		*b = *g = *r = 0;
	}
}

u32 _MAKECOLOR(u8 r, u8 g, u8 b, VO_I80_FORMAT fmt)
{
	u8 r1, g1, b1;
	u8 r_len, g_len, b_len;

	switch (fmt) {
	case VO_I80_FORMAT_RGB444:
		r_len = 4;
		g_len = 4;
		b_len = 4;
		break;

	default:
	case VO_I80_FORMAT_RGB565:
		r_len = 5;
		g_len = 6;
		b_len = 5;
		break;

	case VO_I80_FORMAT_RGB666:
		r_len = 6;
		g_len = 6;
		b_len = 6;
		break;
	}

	r1 = r >> (8 - r_len);
	g1 = g >> (8 - g_len);
	b1 = b >> (8 - b_len);
	return (b1 | (g1 << b_len) | (r1 << (b_len + g_len)));
}


static void _i80_package_rgb(u8 r, u8 g, u8 b, u8 *buffer, u8 byte_cnt)
{
	u32 pixel, i, offset;

	pixel = _MAKECOLOR(r, g, b, gVoCtx->stPubAttr.sti80Cfg.fmt);

	for (i = 0, offset = 0; i < byte_cnt; ++i) {
		*(buffer + offset++) = pixel >> ((byte_cnt - i - 1) << 3);
		*(buffer + offset++) = i80_ctrl[I80_CTRL_DATA];
		*(buffer + offset++) = I80_OP_GO;
	}
}

static void _i80_package_frame(struct vb_s *in, u8 *buffer, u8 byte_cnt)
{
	u32 out_offset = 0;
	u16 line_data = (1 + in->buf.size.u32Width * byte_cnt) * 3;
	u16 padding = ALIGN(line_data, 32) - line_data;
	u8 *in_buf_vir[3] = { CVI_NULL, CVI_NULL, CVI_NULL };
	u8 r, g, b, i, y, x;
#if 0
	struct timespec time[2];

	clock_gettime(CLOCK_MONOTONIC, &time[0]);
#endif
	for (i = 0; i < 3; ++i) {
		if (in->buf.phy_addr[i] == 0 || in->buf.length[i] == 0)
			continue;
		//in_buf_vir[i] = CVI_SYS_MmapCache(in->buf.phy_addr[i], in->buf.length[i]);
		if (in_buf_vir[i] == CVI_NULL) {
			CVI_TRACE_VO(CVI_DBG_INFO, "mmap for i80 transform failed.\n");
			goto ERR_I80_MMAP;
		}
	}

	for (y = 0; y < in->buf.size.u32Height; ++y) {
		for (x = 0; x < in->buf.size.u32Width; ++x) {
			_get_frame_rgb(gVoCtx->stLayerAttr.enPixFormat, in_buf_vir, in->buf.stride, x, y, &r, &g, &b);
			_i80_package_rgb(r, g, b, buffer + out_offset, byte_cnt);
			out_offset += byte_cnt * 3;
		}
		_i80_package_eol(buffer + out_offset);
		out_offset += 3;
		out_offset += padding;
	}
	// replace last eol with eof
	_i80_package_eof(buffer + out_offset - 3 - padding);

#if 0
	clock_gettime(CLOCK_MONOTONIC, &time[1]);
	CVI_TRACE_VO(CVI_DBG_INFO, "consumed %f ms\n", (float)get_diff_in_us(time[0], time[1])/1000);
#endif
ERR_I80_MMAP:
	for (i = 0; i < 3; ++i)
		if (in_buf_vir[i])
			CVI_TRACE_VO(CVI_DBG_INFO, "CVI_SYS_Munmap\n");
			//CVI_SYS_Munmap(in_buf_vir[i], in->buf.length[i]);
}

static s32 _i80_transform_frame(VB_BLK blk_in, VB_BLK *blk_out)
{
	struct vb_s *vb_in, *vb_i80;
	u32 buf_size;
	u8 byte_cnt = (gVoCtx->stPubAttr.sti80Cfg.fmt == VO_I80_FORMAT_RGB666) ? 3 : 2;

	vb_in = (struct vb_s *)blk_in;

	buf_size = ALIGN((vb_in->buf.size.u32Width * byte_cnt + 1) * 3, 32) * vb_in->buf.size.u32Height;
	*blk_out = vb_get_block_with_id(VB_INVALID_POOLID, buf_size, CVI_ID_VO);
	if (*blk_out == VB_INVALID_HANDLE) {
		vb_release_block(blk_in);
		CVI_TRACE_VO(CVI_DBG_INFO, "No more vb for i80 transform.\n");
		return CVI_FAILURE;
	}
	vb_i80 = (struct vb_s *)*blk_out;

	//vb_i80->vir_addr = CVI_SYS_MmapCache(vb_i80->phy_addr, buf_size);
	if (vb_i80->vir_addr == CVI_NULL) {
		vb_release_block(blk_in);
		vb_release_block(*blk_out);
		CVI_TRACE_VO(CVI_DBG_INFO, "mmap for i80 transform failed.\n");
		return CVI_FAILURE;
	}
	_i80_package_frame(vb_in, vb_i80->vir_addr, byte_cnt);
	vb_release_block(blk_in);
	//CVI_SYS_IonFlushCache(vb_i80->phy_addr, vb_i80->vir_addr, buf_size);
	//CVI_SYS_Munmap(vb_i80->vir_addr, buf_size);
	vb_i80->buf.enPixelFormat = PIXEL_FORMAT_RGB_888;
	vb_i80->buf.phy_addr[0] = vb_i80->phy_addr;
	vb_i80->buf.length[0] = buf_size;
	vb_i80->buf.stride[0] = ALIGN((vb_in->buf.size.u32Width * byte_cnt + 1) * 3, 32);
	return CVI_SUCCESS;
}

void vo_post_job(u8 vo_dev)
{
	struct vb_jobs_t *jobs;

	jobs = &stVOjobs;
	if (!jobs) {
		CVI_TRACE_VO(CVI_DBG_ERR, "get in jobs failed\n");
	}

	up(&jobs->sem);

	CVI_TRACE_VO(CVI_DBG_INFO, "vo post job sem.count[%d]\n", jobs->sem.count);
}

static void _vo_qbuf(VB_BLK blk)
{
	//struct vdev *d = get_dev_info(VDEV_TYPE_DISP, 0);
	s32 i = 0;
	struct vb_s *vb = (struct vb_s *)blk;
	struct cvi_disp_buffer *qbuf;
	struct vb_jobs_t *jobs = &stVOjobs;;

	atomic_long_fetch_or(BIT(CVI_ID_VO), &vb->mod_ids);
	if (gVoCtx->stPubAttr.enIntfType == VO_INTF_I80) {
		VB_BLK blk_i80;

		if (_i80_transform_frame(blk, &blk_i80) != CVI_SUCCESS) {
			CVI_TRACE_VO(CVI_DBG_INFO, "i80 transform NG.\n");
			return;
		}
		vb = (struct vb_s *)blk_i80;
	}

	mutex_lock(&jobs->lock);
	if (FIFO_FULL(&jobs->workq)) {
		mutex_unlock(&jobs->lock);
		vb_release_block(blk);
		CVI_TRACE_VO(CVI_DBG_INFO, "vo workq is full. drop new one.\n");
		return;
	}

	if (gVoCtx->clearchnbuf) {
		//pthread_mutex_unlock(&gVoCtx->vb_jobs.lock);
		mutex_unlock(&jobs->lock);
		vb_release_block(blk);
		CVI_TRACE_VO(CVI_DBG_INFO, "clearchnbuf is set. drop new one.\n");
		return;
	}

	for (i = 0; i < 3; i++) {
		CVI_TRACE_VO(CVI_DBG_DEBUG, "vb->buf.phy_add[%d].addr=%llx\n", i, vb->buf.phy_addr[i]);
	}

	FIFO_PUSH(&jobs->workq, vb);
	mutex_unlock(&jobs->lock);

	qbuf = vzalloc(sizeof(struct cvi_disp_buffer));
	if (qbuf == NULL) {
		CVI_TRACE_VO(CVI_DBG_ERR, "QBUF vzalloc size(%zu) fail\n", sizeof(struct cvi_disp_buffer));
		return;
	}
	qbuf->buf.length = 3;
	qbuf->buf.index  = 0;

	for (i = 0; i < qbuf->buf.length; i++) {
		qbuf->buf.planes[i].addr = ((struct vb_s *)blk)->buf.phy_addr[i];
	}

	vo_disp_buf_queue(gVdev, qbuf);
}

void _vo_gdc_callback(void *pParam, VB_BLK blk)
{
	if (!pParam)
		return;

	mutex_unlock(&vo_gdc_lock);

	_vo_qbuf(blk);
	vfree(pParam);
}

static s32 _mesh_gdc_do_op_cb(enum GDC_USAGE usage, const void *pUsageParam,
				struct vb_s *vb_in, PIXEL_FORMAT_E enPixFormat, u64 mesh_addr,
				u8 sync_io, void *pcbParam, u32 cbParamSize,
				MOD_ID_E enModId, ROTATION_E enRotation)
{
	struct mesh_gdc_cfg cfg;
	struct base_exe_m_cb exe_cb;

	memset(&cfg, 0, sizeof(cfg));
	cfg.usage = usage;
	cfg.pUsageParam = pUsageParam;
	cfg.vb_in = vb_in;
	cfg.enPixFormat = enPixFormat;
	cfg.mesh_addr = mesh_addr;
	cfg.sync_io = sync_io;
	cfg.pcbParam = pcbParam;
	cfg.cbParamSize = cbParamSize;
	cfg.enRotation = enRotation;

	exe_cb.callee = E_MODULE_LDC;
	exe_cb.caller = E_MODULE_VO;
	exe_cb.cmd_id = LDC_CB_MESH_GDC_OP;
	exe_cb.data   = &cfg;
	return base_exe_module_cb(&exe_cb);
}

#ifdef VO_PROFILE
static void _vo_update_chnRealFrameRate(VO_CHN_STATUS_S *pstVoChnStatus)
{
	u64 duration, curTimeUs;
	struct timespec64 curTime;

	curTime = ktime_to_timespec64(ktime_get());
	curTimeUs = curTime.tv_sec * 1000000L + curTime.tv_nsec / 1000L;
	duration = curTimeUs - pstVoChnStatus->u64PrevTime;

	if (duration >= 1000000) {
		pstVoChnStatus->u32RealFrameRate = pstVoChnStatus->u32frameCnt;
		pstVoChnStatus->u32frameCnt = 0;
		pstVoChnStatus->u64PrevTime = curTimeUs;
	}
}
#endif

static s32 _vo_disp_thread(void *arg)
{
	struct cvi_vo_dev *vdev = (struct cvi_vo_dev *)arg;
	u32 waittime = 500;//ms
	s32 ret = 0;
	enum E_VO_TH th_id = E_VO_TH_DISP;
	VB_BLK blk;
	struct vb_s *vb;
	MMF_CHN_S chn = {.enModId = CVI_ID_VO, .s32DevId = 0, .s32ChnId = 0};
	struct vb_jobs_t *jobs = &stVOjobs;;
	u32 i = 0;
	SIZE_S size;
	VB_CAL_CONFIG_S stVbCalConfig;
	struct _vo_gdc_cb_param cb_param = { .chn = chn, .usage = GDC_USAGE_LDC};
	unsigned long timeout;
#ifdef VO_PROFILE
	struct timespec64 time[2];
	u32 sum = 0, duration, duration_max = 0, duration_min = 1000 * 1000;
	u8 count = 0;
#endif

	if (!jobs) {
		CVI_TRACE_VO(CVI_DBG_ERR, "get in jobs failed\n");
		return CVI_FAILURE;
	}

	while (1) {
		if (kthread_should_stop()) {
			pr_info("%s exit\n", vdev->vo_th[th_id].th_name);
			atomic_set(&vdev->vo_th[th_id].thread_exit, 1);
			do_exit(1);
		}
#ifdef VO_PROFILE
		_vo_update_chnRealFrameRate(&gVoCtx->chnStatus[chn.s32DevId][chn.s32ChnId]);
		time[0] = ktime_to_timespec64(ktime_get());
#endif

		timeout = msecs_to_jiffies(SEM_WAIT_TIMEOUT_MS);
		ret = down_timeout(&jobs->sem, timeout);
		if (ret == -ETIME) {
			CVI_TRACE_VO(CVI_DBG_DEBUG, "Disp thread expired time, loop\n");
			continue;
		}

		if (gVoCtx->pause) {
			CVI_TRACE_VO(CVI_DBG_INFO, "pause and skip update.\n");
			continue;
		}

		blk = base_mod_jobs_waitq_pop(&stVOjobs);
		if (blk == VB_INVALID_HANDLE) {
			CVI_TRACE_VO(CVI_DBG_INFO, "No more vb for dequeue.\n");
			continue;
		}
		vb = (struct vb_s *)blk;
#ifdef __LP64__
		CVI_TRACE_VO(CVI_DBG_INFO, "wait q pop blk[0x%llx]\n", blk);
#else
		CVI_TRACE_VO(CVI_DBG_INFO, "wait q pop blk[0x%x]\n", blk);
#endif

		for (i = 0; i < 3; i++) {
			CVI_TRACE_VO(CVI_DBG_DEBUG, "vb->buf.phy_add[%d].addr=%llx\n", i, vb->buf.phy_addr[i]);
		}

		gVoCtx->u64DisplayPts[chn.s32DevId][chn.s32ChnId] = vb->buf.u64PTS;
		if (gVoCtx->enRotation == ROTATION_0) {
			if ((vb->buf.s16OffsetLeft != gVoCtx->rect_crop.left)
				|| (vb->buf.s16OffsetTop != gVoCtx->rect_crop.top)) {

				struct sclr_disp_cfg *cfg;
				struct vo_rect area;

				area.width = gVoCtx->rect_crop.width = gVoCtx->stChnAttr.stRect.u32Width;
				area.height = gVoCtx->rect_crop.height = gVoCtx->stChnAttr.stRect.u32Height;
				area.left = gVoCtx->rect_crop.left = vb->buf.s16OffsetLeft;
				area.top = gVoCtx->rect_crop.top = vb->buf.s16OffsetTop;

				cfg = sclr_disp_get_cfg();
				cfg->mem.start_x = area.left;
				cfg->mem.start_y = area.top;
				cfg->mem.width	 = area.width;
				cfg->mem.height  = area.height;

				CVI_TRACE_VO(CVI_DBG_INFO, "Crop Area (%d,%d,%d,%d)\n", cfg->mem.start_x,
				cfg->mem.start_y, cfg->mem.width, cfg->mem.height);

				sclr_disp_set_mem(&cfg->mem);
			}
			_vo_qbuf(blk);
		} else {
			mutex_lock(&vo_gdc_lock);

			size.u32Width = ALIGN(vb->buf.size.u32Width, LDC_ALIGN);
			size.u32Height = ALIGN(vb->buf.size.u32Height, LDC_ALIGN);
			((struct vb_s *)blk)->buf.size = size;

			COMMON_GetPicBufferConfig(size.u32Width, size.u32Height, gVoCtx->stLayerAttr.enPixFormat
				, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, LDC_ALIGN, &stVbCalConfig);
			for (i = 0; i < stVbCalConfig.plane_num; ++i) {
				((struct vb_s *)blk)->buf.length[i] = ALIGN((i == 0)
					? stVbCalConfig.u32MainYSize
					: stVbCalConfig.u32MainCSize,
					stVbCalConfig.u16AddrAlign);
			}
			if (_mesh_gdc_do_op_cb(GDC_USAGE_ROTATION
				, NULL
				, vb
				, gVoCtx->stLayerAttr.enPixFormat
				, gVoCtx->mesh.paddr
				, CVI_TRUE, &cb_param
				, sizeof(cb_param)
				, CVI_ID_VO
				, gVoCtx->enRotation) != CVI_SUCCESS) {
				mutex_unlock(&vo_gdc_lock);
				CVI_TRACE_VO(CVI_DBG_ERR, "gdc rotation failed.\n");
				continue;
			}
#if 0
			CVI_TRACE_VO(CVI_DBG_INFO, "dqbuf chn_id=%d, frm_num=%d, time=%dms\n",
					b.chnId, b.sequence, b.timestamp.tv_sec * 1000 + b.timestamp.tv_usec / 1000);
#endif
		}
		// except for i80, vo needs to keep at least one buf for display.
		// Thus, no buf done if there is only one buffer.
		if (FIFO_SIZE(&jobs->workq) == 1 && (gVoCtx->stPubAttr.enIntfType != VO_INTF_I80)) {
			CVI_TRACE_VO(CVI_DBG_INFO, "vo keep one buf for display.\n");
			continue;
		}

		//check vb buffer done, wait for vo_wake_up_th()
		ret = wait_event_timeout(vdev->vo_th[th_id].wq,
					vdev->vo_th[th_id].flag != 0 || kthread_should_stop(),
					msecs_to_jiffies(waittime) - 1);
		vdev->vo_th[th_id].flag = 0;

		if (kthread_should_stop()) {
			pr_info("%s exit\n", vdev->vo_th[th_id].th_name);
			atomic_set(&vdev->vo_th[th_id].thread_exit, 1);
			do_exit(1);
		}

		vb_dqbuf(chn, &stVOjobs, &blk);

		if (blk == VB_INVALID_HANDLE) {
		//	CVI_TRACE_VO(CVI_DBG_INFO, "%s can't get vb-blk.\n", CVI_SYS_GetModName(chn.enModId));
		} else {
			CVI_TRACE_VO(CVI_DBG_INFO, "vb_done_handler\n");
			gVoCtx->u64PreDonePts[chn.s32DevId][chn.s32ChnId] = ((struct vb_s *)blk)->buf.u64PTS;
			gVoCtx->chnStatus[chn.s32DevId][chn.s32ChnId].u32frameCnt++;

			vb_done_handler(chn, CHN_TYPE_IN, &stVOjobs, blk);
		}

#ifdef VO_PROFILE
		time[1] = ktime_to_timespec64(ktime_get());
		duration = get_diff_in_us(time[0], time[1]);
		duration_max = MAX(duration, duration_max);
		duration_min = MIN(duration, duration_min);
		sum += duration;
		if (++count == 100) {
			CVI_TRACE_VO(CVI_DBG_INFO, "VO duration(ms): average(%d), max(%d) min(%d)\n"
				, sum / count / 1000, duration_max / 1000, duration_min / 1000);
			count = 0;
			sum = duration_max = 0;
			duration_min = 1000 * 1000;
		}
#endif
	}

	return 0;

}

s32 vo_destroy_thread(struct cvi_vo_dev *vdev, enum E_VO_TH th_id)
{
	s32 rc = 0;

	if (th_id < 0 || th_id >= E_VO_TH_MAX) {
		pr_err("No such thread_id(%d)\n", th_id);
		return -1;
	}

	if (vdev->vo_th[th_id].w_thread != NULL) {
		kthread_stop(vdev->vo_th[th_id].w_thread);
		wake_up(&vdev->vo_th[th_id].wq);
		while (atomic_read(&vdev->vo_th[th_id].thread_exit) == 0) {
			pr_info("wait for %s exit\n", vdev->vo_th[th_id].th_name);
			usleep_range(5 * 1000, 10 * 1000);
		}
		vdev->vo_th[th_id].w_thread = NULL;
	}
	return rc;
}

s32 vo_create_thread(struct cvi_vo_dev *vdev, enum E_VO_TH th_id)
{
	struct sched_param param;
	s32 rc = 0;

	if (th_id < 0 || th_id >= E_VO_TH_MAX) {
		CVI_TRACE_VO(CVI_DBG_ERR, "_vo_create_thread fail\n");
		return -1;
	}
	param.sched_priority = MAX_USER_RT_PRIO - 10;

	if (vdev->vo_th[th_id].w_thread == NULL) {
		switch (th_id) {
		case E_VO_TH_DISP:
			memcpy(vdev->vo_th[th_id].th_name, "cvitask_disp", sizeof(vdev->vo_th[th_id].th_name));
			vdev->vo_th[th_id].th_handler = _vo_disp_thread;
			break;

		default:
			CVI_TRACE_VO(CVI_DBG_ERR, "No such thread(%d)\n", th_id);
			return -1;
		}

		vdev->vo_th[th_id].w_thread = kthread_create(vdev->vo_th[th_id].th_handler,
								(void *)vdev,
								vdev->vo_th[th_id].th_name);
		if (IS_ERR(vdev->vo_th[th_id].w_thread)) {
			CVI_TRACE_VO(CVI_DBG_ERR, "Unable to start %s.\n", vdev->vo_th[th_id].th_name);
			return -1;
		}
		sched_setscheduler(vdev->vo_th[th_id].w_thread, SCHED_FIFO, &param);
		vdev->vo_th[th_id].flag = 0;
		atomic_set(&vdev->vo_th[th_id].thread_exit, 0);
		init_waitqueue_head(&vdev->vo_th[th_id].wq);
		wake_up_process(vdev->vo_th[th_id].w_thread);
	}

	return rc;
}

s32 vo_start_streaming(struct cvi_vo_dev *vdev)
{
	s32 rc = 0;

	struct sclr_top_cfg *cfg = sclr_top_get_cfg();

	//dprintk(VIP_VB2, "+\n");

	//_vo_jobs_qbuf();

	cfg->disp_enable = true;
	sclr_top_set_cfg(cfg);
	sclr_disp_enable_window_bgcolor(true);

	vdev->align = VIP_ALIGNMENT;
	vdev->seq_count = 0;
	vdev->frame_number = 0;

	if (vdev->disp_interface != CVI_VIP_DISP_INTF_I80)
		sclr_disp_tgen_enable(true);

	atomic_set(&vdev->disp_streamon, 1);

	//ToDo need to remove this code after enable/disable chn is ok
	//vo_create_thread(vdev, E_VO_TH_DISP);

	return rc;

}
s32 vo_stop_streaming(struct cvi_vo_dev *vdev)
{
	s32 rc = 0;

	//struct cvi_disp_vdev *ddev = vb2_get_drv_priv(vq);
	struct cvi_disp_buffer *cvi_vb2, *tmp;
	unsigned long flags;
	struct vo_buffer *vb2_buf;
	struct sclr_top_cfg *cfg = sclr_top_get_cfg();

	cfg->disp_enable = false;
	sclr_top_set_cfg(cfg);

	sclr_disp_enable_window_bgcolor(true);
	//dprintk(VIP_VB2, "+\n");

	if (!smooth && (vdev->disp_interface != CVI_VIP_DISP_INTF_LVDS))
		sclr_disp_tgen_enable(false);
	/*
	 * Release all the buffers enqueued to driver
	 * when streamoff is issued
	 */
	spin_lock_irqsave(&vdev->rdy_lock, flags);
	list_for_each_entry_safe(cvi_vb2, tmp, &(vdev->rdy_queue), list) {
		vb2_buf = &(cvi_vb2->buf);
	}
	vdev->num_rdy = 0;
	INIT_LIST_HEAD(&vdev->rdy_queue);
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);

	atomic_set(&vdev->disp_streamon, 0);
	memset(&vdev->compose_out, 0, sizeof(vdev->compose_out));

	return rc;

}

static long _vo_s_ctrl(struct cvi_vo_dev *vdev, struct vo_ext_control *p)
{
	u32 id = p->id;
	long rc = -EINVAL;

	switch (id) {
	case VO_IOCTL_SDK_CTRL:
	{
		rc = vo_sdk_ctrl(vdev, p);
		break;
	}

	case VO_IOCTL_START_STREAMING:
	{
		if (vo_start_streaming(vdev)) {
			CVI_TRACE_VO(CVI_DBG_ERR, "Failed to vo start streaming\n");
			break;
		}
		rc = 0;
		break;
	}

	case VO_IOCTL_STOP_STREAMING:
	{
		if (vo_stop_streaming(vdev)) {
			CVI_TRACE_VO(CVI_DBG_ERR, "Failed to vo stop streaming\n");
			break;
		}

		rc = 0;
		break;
	}
	case VO_IOCTL_ENQ_WAITQ:
	{
		s32 ret;
		const uint32_t buf_len = 0x100000;
		MMF_CHN_S chn = {.enModId = CVI_ID_VO, .s32DevId = 0, .s32ChnId = 0};
		uint32_t i, j;
		VB_BLK blk[3] = {VB_INVALID_HANDLE, VB_INVALID_HANDLE, VB_INVALID_HANDLE};
		struct vb_s *vb;

		gVoCtx->u32DisBufLen = 3;

		if (0 == job_init) {
			base_mod_jobs_init(&stVOjobs, gVoCtx->u32DisBufLen - 1, 2, 0);
			job_init = 1;
		}
		// get blk and qbuf to workq
		// ------------------------------------------
		for (i = 0; i < 3; i++) {
			blk[i] = vb_get_block_with_id(VB_INVALID_POOLID, buf_len, CVI_ID_VO);
			vb = (struct vb_s *)blk[i];

			if (blk[i] == VB_INVALID_HANDLE) {
				pr_err("vb_get_block_with_id fail, blk idx(%d)\n", i);

				break;
			}
#ifdef __LP64__
			CVI_TRACE_VO(CVI_DBG_DEBUG, "blk[%d]=(0x%llx)\n", i, blk[i]);
#else
			CVI_TRACE_VO(CVI_DBG_DEBUG, "blk[%d]=(0x%x)\n", i, blk[i]);
#endif

			for (j = 0; j < 3; j++) {
				CVI_TRACE_VO(CVI_DBG_INFO, "vb->buf.phy_addr[%d].addr=%llx\n", j, vb->buf.phy_addr[j]);
			}

			ret = vb_qbuf(chn, CHN_TYPE_IN, &stVOjobs, blk[i]);
			if (ret != CVI_SUCCESS) {
				pr_err("vb_qbuf fail\n");

				break;
			}
		}
		//vb_release_block(blk[i]);

		rc = 0;
		break;
	}

	case VO_IOCTL_SET_DV_TIMINGS:
	{
		struct vo_dv_timings *timings, _timings_;
		struct sclr_disp_timing timing;

		timings = &_timings_;
		if (copy_from_user(timings, (void *)p->ptr, sizeof(struct vo_dv_timings))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "Set DV timing copy_from_user failed.\n");
			break;
		}
#if 0//TODO
		if (!list_empty(&vdev->rdy_queue))
			return -EBUSY;
#endif
		vdev->dv_timings = *timings;
		vdev->sink_rect.width = timings->bt.width;
		vdev->sink_rect.height = timings->bt.height;
		vdev->compose_out = vdev->sink_rect;
		CVI_TRACE_VO(CVI_DBG_INFO, "timing %d-%d\n", timings->bt.width, timings->bt.height);

		vo_fill_disp_timing(&timing, &timings->bt);
		sclr_disp_set_timing(&timing);

		rc = 0;
		break;
	}
	case VO_IOCTL_SEL_TGT_COMPOSE:
	{
		struct vo_rect area;

		if (copy_from_user(&area, p->ptr, sizeof(area)) != 0) {
			CVI_TRACE_VO(CVI_DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", p->id);
			break;
		}

		if (memcmp(&vdev->compose_out, &area, sizeof(area))) {
			struct sclr_rect rect;

			rect.x = area.left;
			rect.y = area.top;
			rect.w = area.width;
			rect.h = area.height;

			CVI_TRACE_VO(CVI_DBG_INFO, "Compose Area (%d,%d,%d,%d)\n", rect.x, rect.y, rect.w, rect.h);
			if (sclr_disp_set_rect(rect) == 0)
				vdev->compose_out = area;
		}
		rc = 0;
		break;
	}
	case VO_IOCTL_SEL_TGT_CROP:
	{
		struct sclr_disp_cfg *cfg;
		struct vo_rect area;

		if (copy_from_user(&area, (void __user *)p->ptr, sizeof(struct vo_rect))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", p->id);
			break;
		}

		cfg = sclr_disp_get_cfg();
		cfg->mem.start_x = area.left;
		cfg->mem.start_y = area.top;
		cfg->mem.width	 = area.width;
		cfg->mem.height  = area.height;

		CVI_TRACE_VO(CVI_DBG_INFO, "Crop Area (%d,%d,%d,%d)\n", cfg->mem.start_x,
												cfg->mem.start_y,
												cfg->mem.width,
												cfg->mem.height);
		sclr_disp_set_mem(&cfg->mem);
		vdev->crop_rect = area;

		rc = 0;
		break;
	}
	case VO_IOCTL_SET_ALIGN:
	{
		if (p->value >= VIP_ALIGNMENT) {
			vdev->align = p->value;
			CVI_TRACE_VO(CVI_DBG_INFO, "Set Align(%d)\n", vdev->align);
		}
		rc = 0;
		break;
	}
	case VO_IOCTL_SET_RGN:
	{
		struct sclr_disp_timing *timing = sclr_disp_get_timing();
		struct sclr_size size;
		struct cvi_rgn_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct cvi_rgn_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", p->id);
			break;
		}
		size.w = timing->hfde_end - timing->hfde_start + 1;
		size.h = timing->vfde_end - timing->vfde_start + 1;
		vo_set_rgn_cfg(SCL_GOP_DISP, &cfg, &size);

		rc = 0;
		break;
	}
	case VO_IOCTL_I80_SW_MODE:
	{
		sclr_i80_sw_mode(p->value);

		rc = 0;
		break;
	}
	case VO_IOCTL_I80_CMD:
	{
		sclr_i80_packet(p->value);

		rc = 0;
		break;
	}
	case VO_IOCTL_SET_CUSTOM_CSC:
	{
		struct sclr_csc_matrix cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct sclr_csc_matrix))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", p->id);
			break;
		}
		sclr_disp_set_csc(&cfg);

		rc = 0;
		break;
	}
	case VO_IOCTL_SET_CLK:
	{
		if (p->value < 8000) {
			CVI_TRACE_VO(CVI_DBG_ERR, "V4L2_CID_DV_VIP_DISP_SET_CLK clk(%d) less than 8000 kHz.\n",
				p->value);
			break;
		}
		dphy_dsi_set_pll(p->value, 4, 24);

		rc = 0;
		break;
	}
	case VO_IOCTL_INTR:
	{
		static bool service_isr = true;
		union sclr_intr intr_mask;

		service_isr = !service_isr;
		intr_mask = sclr_get_intr_mask();
		intr_mask.b.disp_frame_end = (service_isr) ? 1 : 0;
		sclr_set_intr_mask(intr_mask);

		rc = 0;
		break;
	}
	case VO_IOCTL_OUT_CSC:
	{
		if (p->value >= SCL_CSC_601_LIMIT_YUV2RGB &&
			p->value <= SCL_CSC_709_FULL_YUV2RGB) {
			CVI_TRACE_VO(CVI_DBG_ERR, "invalid disp-out-csc(%d)\n", p->value);
			break;
		}
		sclr_disp_set_out_csc(p->value);

		rc = 0;
		break;
	}
	case VO_IOCTL_PATTERN:
	{
		if (p->value >= CVI_VIP_PAT_MAX) {
			CVI_TRACE_VO(CVI_DBG_ERR, "invalid disp-pattern(%d)\n",
					p->value);
			break;
		}
		sclr_disp_set_pattern(patterns[p->value].type, patterns[p->value].color, patterns[p->value].rgb);

		rc = 0;
		break;
	}
	case VO_IOCTL_FRAME_BGCOLOR:
	{
		u16 u16_rgb[3], r, g, b;

		if (copy_from_user(&u16_rgb[0], p->ptr, sizeof(u16_rgb))) {
			//CVI_TRACE_VO(CVI_DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", p->id);
			break;
		}

		r = u16_rgb[0];
		g = u16_rgb[1];
		b = u16_rgb[2];
		CVI_TRACE_VO(CVI_DBG_INFO, "Set Frame BG color (R,G,B) = (%x,%x,%x)\n", r, g, b);

		sclr_disp_set_frame_bgcolor(r, g, b);

		rc = 0;
		break;

	}
	case VO_IOCTL_WINDOW_BGCOLOR:
	{
		u16 u16_rgb[3], r, g, b;

		if (copy_from_user(&u16_rgb[0], p->ptr, sizeof(u16_rgb))) {
			//CVI_TRACE_VO(CVI_DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", p->id);
			break;
		}

		r = u16_rgb[0];
		g = u16_rgb[1];
		b = u16_rgb[2];
		CVI_TRACE_VO(CVI_DBG_INFO, "Set window BG color 2(R,G,B) = (%d,%d,%d)\n", r, g, b);

		sclr_disp_set_window_bgcolor(r, g, b);

		rc = 0;
		break;
	}
	case VO_IOCTL_ONLINE:
	{
		if (atomic_read(&vdev->disp_streamon) == 1) {
			CVI_TRACE_VO(CVI_DBG_ERR, "V4L2_CID_DV_VIP_DISP_ONLINE can't be control if streaming.\n");

			rc = 0;
			break;
		}
		//copy_from_user(&vdev->disp_online, (p->value), sizeof(bool));
		vdev->disp_online = p->value;

		sclr_ctrl_set_disp_src(vdev->disp_online);

		rc = 0;
		break;
	}
	case VO_IOCTL_INTF:
	{
		struct cvi_disp_intf_cfg *cfg, _cfg_;

		cfg = &_cfg_;
		if (copy_from_user(cfg, p->ptr, sizeof(struct cvi_disp_intf_cfg))) {
			//CVI_TRACE_VO(CVI_DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", p->id);
			break;
		}

		if (smooth) {
			CVI_TRACE_VO(CVI_DBG_DEBUG, "V4L2_CID_DV_VIP_DISP_INTF won't apply if smooth.\n");
			sclr_disp_reg_force_up();
			vdev->disp_interface = cfg->intf_type;
			rc = 0;
			break;
		}

#if 0//TODO
		if (vb2_is_streaming(&ddev->vb_q)) {
			dprintk(VIP_ERR, "V4L2_CID_DV_VIP_DISP_INTF can't be control if streaming.\n");
			break;
		}
#endif
		if (atomic_read(&vdev->disp_streamon) == 1) {
			CVI_TRACE_VO(CVI_DBG_ERR, "V4L2_CID_DV_VIP_DISP_ONLINE can't be control if streaming.\n");
			break;
		}

		if (cfg->intf_type == CVI_VIP_DISP_INTF_DSI) {
			CVI_TRACE_VO(CVI_DBG_INFO, "MIPI use mipi_tx to control.\n");
			//return rc;
		} else if (cfg->intf_type == CVI_VIP_DISP_INTF_LVDS) {
			s32 i = 0;
			union sclr_lvdstx lvds_reg;
			bool data_en[LANE_MAX_NUM] = {false, false, false, false, false};

			for (i = 0; i < LANE_MAX_NUM; i++) {
				if ((cfg->lvds_cfg.lane_id[i] < 0) ||
					(cfg->lvds_cfg.lane_id[i] >= LANE_MAX_NUM)) {
					dphy_dsi_set_lane(i, DSI_LANE_MAX, false, false);
					continue;
				}
				dphy_dsi_set_lane(i, cfg->lvds_cfg.lane_id[i],
						  cfg->lvds_cfg.lane_pn_swap[i], false);
				if (cfg->lvds_cfg.lane_id[i] != MIPI_TX_LANE_CLK) {
					data_en[cfg->lvds_cfg.lane_id[i] - 1] = true;
				}
			}
			dphy_dsi_lane_en(true, data_en, false);
			sclr_disp_set_intf(SCLR_VO_INTF_LVDS);

			if (cfg->lvds_cfg.pixelclock == 0) {
				CVI_TRACE_VO(CVI_DBG_ERR, "lvds pixelclock 0 invalid\n");
				//return rc;
			}
			lvds_reg.b.out_bit = cfg->lvds_cfg.out_bits;
			lvds_reg.b.vesa_mode = cfg->lvds_cfg.mode;
			if (cfg->lvds_cfg.chn_num == 1)
				lvds_reg.b.dual_ch = 0;
			else if (cfg->lvds_cfg.chn_num == 2)
				lvds_reg.b.dual_ch = 1;
			else {
				lvds_reg.b.dual_ch = 0;
				CVI_TRACE_VO(CVI_DBG_ERR, "invalid lvds chn_num(%d). Use 1 instead."
					, cfg->lvds_cfg.chn_num);
			}
			lvds_reg.b.vs_out_en = cfg->lvds_cfg.vs_out_en;
			lvds_reg.b.hs_out_en = cfg->lvds_cfg.hs_out_en;
			lvds_reg.b.hs_blk_en = cfg->lvds_cfg.hs_blk_en;
			lvds_reg.b.ml_swap = cfg->lvds_cfg.msb_lsb_data_swap;
			lvds_reg.b.ctrl_rev = cfg->lvds_cfg.serial_msb_first;
			lvds_reg.b.oe_swap = cfg->lvds_cfg.even_odd_link_swap;
			lvds_reg.b.en = cfg->lvds_cfg.enable;
			dphy_lvds_set_pll(cfg->lvds_cfg.pixelclock, cfg->lvds_cfg.chn_num);
			sclr_lvdstx_set(lvds_reg);
		} else if (cfg->intf_type == CVI_VIP_DISP_INTF_I80) {
			union sclr_bt_enc enc;
			union sclr_bt_sync_code sync;

			_disp_sel_pinmux(cfg->intf_type, &cfg->bt_cfg);
			sclr_disp_set_intf(SCLR_VO_INTF_I80);
			enc.raw = 0;
			enc.b.fmt_sel = 2;
			enc.b.clk_inv = 1;
			sync.raw = 0;
			sync.b.sav_vld = 0x80;
			sync.b.sav_blk = 0xab;
			sync.b.eav_vld = 0x9d;
			sync.b.eav_blk = 0xb6;
			sclr_bt_set(enc, sync);
		} else if (cfg->intf_type == CVI_VIP_DISP_INTF_BT) {
			union sclr_bt_enc enc;
			union sclr_bt_sync_code sync;

			if (cfg->bt_cfg.mode == BT_MODE_1120)
				sclr_disp_set_intf(SCLR_VO_INTF_BT1120);
			else if (cfg->bt_cfg.mode == BT_MODE_656)
				sclr_disp_set_intf(SCLR_VO_INTF_BT656);
			else if (cfg->bt_cfg.mode == BT_MODE_601)
				sclr_disp_set_intf(SCLR_VO_INTF_BT601);
			else {
				CVI_TRACE_VO(CVI_DBG_ERR, "invalid bt-mode(%d)\n", cfg->bt_cfg.mode);
				//return rc;
			}

			//enable clk_bt
			clk_prepare_enable(vdev->clk_bt);

			//set clock to 54Mhz
			dphy_dsi_set_bt656_54mhz(0, 0);

			//set csc value
			sclr_disp_set_out_csc(SCL_CSC_601_FULL_RGB2YUV);
			_disp_sel_pinmux(cfg->intf_type, &cfg->bt_cfg);

			enc.raw = 0;
			enc.b.fmt_sel = cfg->bt_cfg.mode;
			sync.b.sav_vld = 0x80;
			sync.b.sav_blk = 0xab;
			sync.b.eav_vld = 0x9d;
			sync.b.eav_blk = 0xb6;
			sclr_bt_set(enc, sync);
		} else {
			CVI_TRACE_VO(CVI_DBG_ERR, "invalid disp-intf(%d)\n", cfg->intf_type);
			//return rc;
		}
		sclr_disp_reg_force_up();

		vdev->disp_interface = cfg->intf_type;

		rc = 0;
		break;
	}
	case VO_IOCTL_ENABLE_WIN_BGCOLOR:
	{
		vdev->bgcolor_enable = p->value;
		sclr_disp_enable_window_bgcolor(p->value);

		rc = 0;
		break;
	}
	case VO_IOCTL_GAMMA_LUT_UPDATE:
	{
		s32 i = 0;
		struct sclr_disp_gamma_attr gamma_attr_sclr;
		VO_GAMMA_INFO_S gamma_attr;

		if (copy_from_user(&gamma_attr, (void *)p->ptr, sizeof(gamma_attr))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "gamma lut update copy_from_user failed.\n");
			break;
		}

		gamma_attr_sclr.enable = gamma_attr.enable;
		gamma_attr_sclr.pre_osd = gamma_attr.osd_apply;

		for (i = 0; i < SCL_DISP_GAMMA_NODE; ++i) {
			gamma_attr_sclr.table[i] = gamma_attr.value[i];
		}

		sclr_disp_gamma_ctrl(gamma_attr_sclr.enable, gamma_attr_sclr.pre_osd);
		sclr_disp_gamma_lut_update(gamma_attr_sclr.table, SCL_DISP_GAMMA_NODE);

		rc = 0;
		break;
	}

	default:
		break;
	}

	return rc;
}

static long _vo_g_ctrl(struct cvi_vo_dev *vdev, struct vo_ext_control *p)
{
	u32 id = p->id;
	long rc = -EINVAL;

	switch (id) {
	case VO_IOCTL_GET_DV_TIMINGS:{
		if (copy_to_user(p->ptr, &vdev->dv_timings, sizeof(struct vo_dv_timings)))
			break;

		return 0;
	}
	break;

	case VO_IOCTL_GET_VLAYER_SIZE:{

		struct sclr_disp_timing *timing = sclr_disp_get_timing();
		struct dsize {
			u32 width;
			u32 height;
		} vsize;

		vsize.width = timing->hfde_end - timing->hfde_start + 1;
		vsize.height = timing->vfde_end - timing->vfde_start + 1;

		if (copy_to_user(p->ptr, &vsize, sizeof(struct dsize)))
			break;
		rc = 0;
	}
	break;

	case VO_IOCTL_GET_INTF_TYPE:{
		enum sclr_vo_sel vo_sel;

		vo_sel = sclr_disp_mux_get();
		if (copy_to_user(p->ptr, &vo_sel, sizeof(vo_sel)))
			break;
		rc = 0;
	}
	break;

	case VO_IOCTL_GET_PANEL_STATUS:{
		s32 is_init = 0;

		if (sclr_disp_mux_get() == SCLR_VO_SEL_I80) {
			is_init = sclr_disp_check_i80_enable();
		} else {
			is_init = sclr_disp_check_tgen_enable();
		}
		if (copy_to_user(p->ptr, &is_init, sizeof(is_init)))
			break;
		rc = 0;
	}
	break;

	case VO_IOCTL_GAMMA_LUT_READ:{
		s32 i = 0;
		VO_GAMMA_INFO_S gamma_attr;
		struct sclr_disp_gamma_attr gamma_attr_sclr;

		sclr_disp_gamma_lut_read(&gamma_attr_sclr);

		gamma_attr.enable = gamma_attr_sclr.enable;
		gamma_attr.osd_apply = gamma_attr_sclr.pre_osd;

		for (i = 0; i < SCL_DISP_GAMMA_NODE; ++i) {
			gamma_attr.value[i] = gamma_attr_sclr.table[i];
		}

		if (copy_to_user((void *)p->ptr, &gamma_attr, sizeof(VO_GAMMA_INFO_S))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "gamma lut read copy_to_user failed.\n");
			break;
		}

		rc = 0;
	}
	break;

	default:
		break;
	}

	return rc;
}

long vo_ioctl(struct file *file, u_int cmd, u_long arg)
{
	struct cvi_vo_dev *vdev = file->private_data;
	s32 ret = 0;
	struct vo_ext_control p;

	if (copy_from_user(&p, (void __user *)arg, sizeof(struct vo_ext_control)))
		return -EINVAL;

	switch (cmd) {
	case VO_IOC_G_CTRL:
	{
		ret = _vo_g_ctrl(vdev, &p);
		break;
	}
	case VO_IOC_S_CTRL:
	{
		ret = _vo_s_ctrl(vdev, &p);
		break;
	}
	default:
		ret = -ENOTTY;
		break;
	}

	if (copy_to_user((void __user *)arg, &p, sizeof(struct vo_ext_control)))
		return -EINVAL;

	return ret;

}

static void _vo_sw_init(struct cvi_vo_dev *vdev)
{
	spin_lock_init(&vdev->rdy_lock);
	INIT_LIST_HEAD(&vdev->rdy_queue);

	vdev->num_rdy = 0;
	atomic_set(&vdev->disp_streamon, 0);
}

s32 vo_open(struct inode *inode, struct file *file)
{
	s32 ret = 0;

	if (!atomic_read(&dev_open_cnt)) {
		struct cvi_vo_dev *vdev;

		vdev = container_of(inode->i_cdev, struct cvi_vo_dev, cdev);

		file->private_data = vdev;

		if (vdev->clk_sc_top)
			clk_prepare_enable(vdev->clk_sc_top);
		if (vdev->clk_disp)
			clk_prepare_enable(vdev->clk_disp);

		_vo_sw_init(vdev);

		sclr_disp_reg_shadow_sel(false);
		if (!smooth)
			sclr_disp_set_cfg(sclr_disp_get_cfg());
	}

	atomic_inc(&dev_open_cnt);

	return ret;

}

void _vo_sdk_release(struct cvi_vo_dev *vdev)
{
	vo_disable_chn(0, 0);
	vo_disablevideolayer(0);
	vo_disable(0);

	memset(gVoCtx, 0, sizeof(*gVoCtx));
}

s32 vo_release(struct inode *inode, struct file *file)
{
	s32 ret = 0;

	atomic_dec(&dev_open_cnt);

	if (!atomic_read(&dev_open_cnt)) {
		struct cvi_vo_dev *vdev;

		vdev = container_of(inode->i_cdev, struct cvi_vo_dev, cdev);

		_vo_sdk_release(vdev);

		if (!(debug & BIT(2)) && vdev->clk_disp && __clk_is_enabled(vdev->clk_disp))
			clk_disable_unprepare(vdev->clk_sc_top);
		if (!(debug & BIT(2)) && vdev->clk_sc_top && __clk_is_enabled(vdev->clk_sc_top))
			clk_disable_unprepare(vdev->clk_disp);
	}

    //TODO
	//vb2_fop_release(file);
	return ret;
}

s32 vo_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct cvi_vo_dev *vdev = file->private_data;

	unsigned long vm_start = vma->vm_start;
	u32 vm_size = vma->vm_end - vma->vm_start;
	u32 offset = vma->vm_pgoff << PAGE_SHIFT;
	void *pos = vdev->shared_mem;

	if ((vm_size + offset) > VO_SHARE_MEM_SIZE)
		return -EINVAL;

	while (vm_size > 0) {
		if (remap_pfn_range(vma, vm_start, virt_to_pfn(pos), PAGE_SIZE, vma->vm_page_prot))
			return -EAGAIN;
		//CVI_TRACE_VO(CVI_DBG_DEBUG, "vo proc mmap vir(%p) phys(%#llx)\n", pos, virt_to_phys((void *) pos));
		vm_start += PAGE_SIZE;
		pos += PAGE_SIZE;
		vm_size -= PAGE_SIZE;
	}

	return 0;
}

u32 vo_poll(struct file *file, struct poll_table_struct *wait)
{
	return 0;
}

s32 vo_cb(void *dev, enum ENUM_MODULES_ID caller, u32 cmd, void *arg)
{
	struct cvi_vo_dev *vdev = (struct cvi_vo_dev *)dev;
	s32 rc = -1;

	switch (cmd) {
	case VO_CB_IRQ_HANDLER:
	{
		union sclr_intr intr_status = *(union sclr_intr *)arg;

		vo_irq_handler(vdev, intr_status);

		rc = 0;
		break;
	}

	case VO_CB_GET_RGN_HDLS:
	{
		struct _rgn_hdls_cb_param *attr = (struct _rgn_hdls_cb_param *)arg;
		VO_LAYER VoLayer = attr->stChn.s32DevId;
		VO_CHN VoChn = attr->stChn.s32ChnId;
		RGN_HANDLE *pstHandle = attr->hdls;
		RGN_TYPE_E enType = attr->enType;

		CVI_TRACE_VO(CVI_DBG_INFO, "VO_CB_GET_RGN_HDLS\n");

		if (vo_cb_get_rgn_hdls(VoLayer, VoChn, enType, pstHandle)) {
			CVI_TRACE_VO(CVI_DBG_INFO, "VO_CB_GET_RGN_HDLS failed.\n");
		}

		rc = 0;
		break;
	}

	case VO_CB_SET_RGN_HDLS:
	{
		struct _rgn_hdls_cb_param *attr = (struct _rgn_hdls_cb_param *)arg;
		VO_LAYER VoLayer = attr->stChn.s32DevId;
		VO_CHN VoChn = attr->stChn.s32ChnId;
		RGN_HANDLE *pstHandle = attr->hdls;
		RGN_TYPE_E enType = attr->enType;

		CVI_TRACE_VO(CVI_DBG_INFO, "VO_CB_SET_RGN_HDLS\n");

		if (vo_cb_set_rgn_hdls(VoLayer, VoChn, enType, pstHandle)) {
			CVI_TRACE_VO(CVI_DBG_INFO, "VO_CB_SET_RGN_HDLS failed.\n");
		}

		rc = 0;
		break;
	}

	case VO_CB_SET_RGN_CFG:
	{
		struct sclr_disp_timing *timing = sclr_disp_get_timing();
		struct sclr_size size;
		struct _rgn_cfg_cb_param *attr = (struct _rgn_cfg_cb_param *)arg;
		struct cvi_rgn_cfg *pstRgnCfg = &attr->rgn_cfg;
		VO_LAYER VoLayer = attr->stChn.s32DevId;
		VO_CHN VoChn = attr->stChn.s32ChnId;

		CVI_TRACE_VO(CVI_DBG_INFO, "VO_CB_SET_RGN_CFG\n");

		if (vo_cb_set_rgn_cfg(VoLayer, VoChn, pstRgnCfg)) {
			CVI_TRACE_VO(CVI_DBG_INFO, "VO_CB_SET_RGN_CFG is failed.\n");
		}

		size.w = timing->hfde_end - timing->hfde_start + 1;
		size.h = timing->vfde_end - timing->vfde_start + 1;
		vo_set_rgn_cfg(SCL_GOP_DISP, pstRgnCfg, &size);
		rc = CVI_SUCCESS;
		break;
	}

	case VO_CB_SET_RGN_COVEREX_CFG:
	{
		struct _rgn_coverex_cfg_cb_param *attr = (struct _rgn_coverex_cfg_cb_param *)arg;
		struct cvi_rgn_coverex_cfg *pstRgnCoverExCfg = &attr->rgn_coverex_cfg;
		VO_LAYER VoLayer = attr->stChn.s32DevId;
		VO_CHN VoChn = attr->stChn.s32ChnId;

		CVI_TRACE_VO(CVI_DBG_INFO, "VO_CB_SET_RGN_COVER_CFG\n");

		if (vo_cb_set_rgn_coverex_cfg(VoLayer, VoChn, pstRgnCoverExCfg)) {
			CVI_TRACE_VO(CVI_DBG_INFO, "VO_CB_SET_RGN_COVER_CFG is failed.\n");
		}

		vo_set_rgn_coverex_cfg(pstRgnCoverExCfg);
		rc = CVI_SUCCESS;
		break;
	}

	case VO_CB_GET_CHN_SIZE:
	{
		struct _rgn_chn_size_cb_param *param = (struct _rgn_chn_size_cb_param *)arg;
		VO_LAYER VoLayer = param->stChn.s32DevId;
		VO_CHN VoChn = param->stChn.s32ChnId;

		CVI_TRACE_VO(CVI_DBG_INFO, "VO_CB_GET_CHN_SIZE\n");

		rc = vo_cb_get_chn_size(VoLayer, VoChn, &param->rect);
		if (rc != CVI_SUCCESS) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_CB_GET_CHN_SIZE failed\n");
		}
		break;
	}

	case VO_CB_GDC_OP_DONE:
	{
		struct ldc_op_done_cfg *cfg =
			(struct ldc_op_done_cfg *)arg;
		_vo_gdc_callback(cfg->pParam, cfg->blk);

		rc = 0;
		break;
	}

	case VO_CB_QBUF_VO_GET_CHN_ROTATION:
	{
		struct vo_get_chnrotation_cfg *cfg =
			(struct vo_get_chnrotation_cfg *)arg;

		vo_get_chnrotation(cfg->VoLayer, cfg->VoChn, (ROTATION_E *)&cfg->enRotation);

		rc = 0;
		break;
	}

	case VO_CB_QBUF_TRIGGER:
	{
		u8 vpss_dev;

		vpss_dev = *((u8 *)arg);
		CVI_TRACE_VO(CVI_DBG_ERR, "VO_CB_QBUF_TRIGGER\n");

		vo_post_job(vpss_dev);
		rc = 0;
		break;
	}

	default:
		break;
	}

	return rc;
}
/*******************************************************
 *  Irq handlers
 ******************************************************/

void vo_irq_handler(struct cvi_vo_dev *vdev, union sclr_intr intr_status)
{
	if (atomic_read(&vdev->disp_streamon) == 0)
		return;

	if (intr_status.b.disp_frame_end) {
		union sclr_disp_dbg_status status = sclr_disp_get_dbg_status(true);

		++vdev->frame_number;

		if (status.b.bw_fail)
			CVI_TRACE_VO(CVI_DBG_ERR, " disp bw failed at frame#%d\n", vdev->frame_number);
		if (status.b.osd_bw_fail)
			CVI_TRACE_VO(CVI_DBG_ERR, " osd bw failed at frame#%d\n", vdev->frame_number);

		//CVI_TRACE_VO(CVI_DBG_ERR, " vo_irq_handler entry 1 ,vdev->num_rdy[%d]\n",vdev->num_rdy);

		// i80 won't need to keep one frame for read, but others need.
		if ((vdev->num_rdy > 1) || (vdev->disp_interface == CVI_VIP_DISP_INTF_I80)) {

			vo_buf_remove((struct cvi_vo_dev *)vdev);
			CVI_TRACE_VO(CVI_DBG_INFO, "vo_irq_handler entry\n");

			// muted until frame available.
			if (gVoCtx->is_layer_enable[0]) {

				CVI_TRACE_VO(CVI_DBG_INFO, "vo_irq_handler set bgcolor\n");
				sclr_disp_enable_window_bgcolor(false);
			}

			vo_wake_up_th((struct cvi_vo_dev *)vdev);

			_vo_hw_enque(vdev);
		}
	}
}
//EXPORT_SYMBOL_GPL(vo_irq_handler);

static s32 _vo_init_param(struct cvi_vo_dev *vdev)
{
	s32 ret = 0, i;

	gVoCtx = (struct cvi_vo_ctx *)vdev->shared_mem;

	memset(gVoCtx, 0, sizeof(*gVoCtx));
	for (i = 0; i < VO_MAX_LAYER_NUM; ++i) {
		gVoCtx->stLayerBind[i].s32VoDev = VO_DEV_INVALID;
		gVoCtx->stLayerBind[i].s32VoLayer = VO_VIDEOLAYER_INVALID;
	}
	mutex_init(gVoCtx->stMutexBindLock);
	//sema_init(&gVoCtx->sem, 0);

err:
	return ret;

}

s32 vo_recv_frame(MMF_CHN_S chn, VB_BLK blk)
{
	s32 ret;

	ret = vb_qbuf(chn, CHN_TYPE_IN, &stVOjobs, blk);
	CVI_TRACE_VO(CVI_DBG_INFO, "vb_qbuf ret[%d]\n", ret);

	return ret;
}

/*******************************************************
 *  Common interface for core
 ******************************************************/
s32 vo_create_instance(struct platform_device *pdev)
{
	s32 ret = 0;
	struct cvi_vo_dev *vdev;
	u16 rgb[3] = {0, 0, 0};

	job_init = 0;//tmp test
	vdev = dev_get_drvdata(&pdev->dev);
	if (!vdev) {
		CVI_TRACE_VO(CVI_DBG_ERR, "invalid data\n");
		return -EINVAL;
	}

	gVdev = vdev;
	g_pdev = pdev;

	if (ret) {
		CVI_TRACE_VO(CVI_DBG_ERR, "Failed to create err_handler thread\n");
		goto err;
	}

#if 0//set base addr by vpss
	sclr_set_base_addr(vdev->reg_base[0]);
	vip_set_base_addr(vdev->reg_base[1]);
	dphy_set_base_addr(vdev->reg_base[2]);
#endif

	if (sclr_disp_mux_get() == SCLR_VO_SEL_I80) {
		smooth = sclr_disp_check_i80_enable();
	} else {
		smooth = sclr_disp_check_tgen_enable();
	}

	if (!smooth) {
		if (vdev->clk_disp && __clk_is_enabled(vdev->clk_disp))
			clk_disable_unprepare(vdev->clk_disp);
		if (vdev->clk_bt && __clk_is_enabled(vdev->clk_bt))
			clk_disable_unprepare(vdev->clk_bt);
		if (vdev->clk_dsi && __clk_is_enabled(vdev->clk_dsi))
			clk_disable_unprepare(vdev->clk_dsi);
	}

	ret = _vo_create_proc(vdev);
	if (ret) {
		CVI_TRACE_VO(CVI_DBG_ERR, "Failed to create proc\n");
		goto err;
	}
	ret = _vo_init_param(vdev);

	if (hide_vo) {
		sclr_disp_set_pattern(SCL_PAT_TYPE_FULL, SCL_PAT_COLOR_USR, rgb);
		sclr_disp_set_frame_bgcolor(0, 0, 0);
	}
	base_register_recv_cb(CVI_ID_VO, vo_recv_frame);

err:
	return ret;

}

s32 vo_destroy_instance(struct platform_device *pdev)
{
	s32 ret = 0;
	struct cvi_vo_dev *vdev;

	vdev = dev_get_drvdata(&pdev->dev);
	if (!vdev) {
		CVI_TRACE_VO(CVI_DBG_ERR, "invalid data\n");
		return -EINVAL;
	}
	base_unregister_recv_cb(CVI_ID_VO);

	vo_destroy_thread(vdev, E_VO_TH_DISP);

	ret = _vo_destroy_proc(vdev);

	return ret;
}
