
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/sys.h>
#include <linux/of_gpio.h>
#include <linux/timer.h>

#include <linux/cvi_common.h>
#include <linux/cvi_vip.h>
#include <linux/cvi_defines.h>
#include <linux/cvi_buffer.h>
#include <linux/cvi_mipi_tx.h>

#include <vo.h>
#include <vb.h>
#include <vo_common.h>
#include <vo_core.h>
#include <vo_defines.h>
#include <vo_interfaces.h>
#include "disp.h"
#include <proc/vo_proc.h>
#include <proc/vo_disp_proc.h>

#include <base_cb.h>
#include <vpss_cb.h>
#include <vo_cb.h>
#include <ldc_cb.h>
#include <rgn_cb.h>
#include "vo_rgn_ctrl.h"

#include "base_common.h"
#include "vbq.h"
#include "dsi_phy.h"

/*******************************************************
 *  MACRO defines
 ******************************************************/
#define WAIT_TIMEOUT_MS  200
#define VO_PROFILE
/*******************************************************
 *  Global variables
 ******************************************************/
s32 vo_log_lv = CVI_DBG_WARN;
s32 smooth[VO_MAX_DEV_NUM];
s32 debug;
static bool hide_vo;

module_param(vo_log_lv, int, 0644);
module_param(hide_vo, bool, 0444);

struct _vo_gdc_cb_param {
	MMF_CHN_S chn;
	enum GDC_USAGE usage;
};

struct vo_stitch_cb_data {
	wait_queue_head_t wait;
	u8 u8Flag;
};

#define IS_VB_OFFSET_INVALID(buf) \
	((buf).s16OffsetLeft < 0 || (buf).s16OffsetRight < 0 || \
	 (buf).s16OffsetTop < 0 || (buf).s16OffsetBottom < 0 || \
	 ((u32)((buf).s16OffsetLeft + (buf).s16OffsetRight) > (buf).size.u32Width) || \
	 ((u32)((buf).s16OffsetTop + (buf).s16OffsetBottom) > (buf).size.u32Height))

#define FRC_INVALID(stFrameRate)	\
	(stFrameRate.s32DstFrameRate <= 0 || stFrameRate.s32SrcFrameRate <= 0 ||	\
		stFrameRate.s32DstFrameRate >= stFrameRate.s32SrcFrameRate)

struct cvi_vo_ctx *gVoCtx;
struct cvi_vo_dev *gVdev;
static u8 i80_ctrl[I80_CTRL_MAX] = {0x31, 0x75, 0xff};
static atomic_t  dev_open_cnt;


//update proc info
static void _update_vo_real_frame_rate(struct timer_list *timer);
DEFINE_TIMER(vo_timer_proc, _update_vo_real_frame_rate);

#if 0 // TODO: to be removed, should be done in uboot
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
	s32 rc = CVI_SUCCESS;

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
#endif

const struct vo_disp_pattern patterns[CVI_VIP_PAT_MAX] = {
	{.type = PAT_TYPE_OFF,	.color = PAT_COLOR_MAX},
	{.type = PAT_TYPE_SNOW, .color = PAT_COLOR_MAX},
	{.type = PAT_TYPE_AUTO, .color = PAT_COLOR_MAX},
	{.type = PAT_TYPE_FULL, .color = PAT_COLOR_RED},
	{.type = PAT_TYPE_FULL, .color = PAT_COLOR_GREEN},
	{.type = PAT_TYPE_FULL, .color = PAT_COLOR_BLUE},
	{.type = PAT_TYPE_FULL, .color = PAT_COLOR_BAR},
	{.type = PAT_TYPE_H_GRAD, .color = PAT_COLOR_WHITE},
	{.type = PAT_TYPE_V_GRAD, .color = PAT_COLOR_WHITE},
	{.type = PAT_TYPE_FULL, .color = PAT_COLOR_USR,
	.rgb = {0, 0, 0} },
};

/*******************************************************
 *  Internal APIs
 ******************************************************/
static int _vo_stitch_call_vpss(struct vpss_stitch_cfg *stitch_cfg)
{
	struct base_exe_m_cb exe_cb;

	exe_cb.callee = E_MODULE_VPSS;
	exe_cb.caller = E_MODULE_VO;
	exe_cb.cmd_id = VPSS_CB_STITCH;
	exe_cb.data   = (void *)stitch_cfg;

	return base_exe_module_cb(&exe_cb);
}

s32 _vo_create_proc(struct cvi_vo_ctx *ctx)
{
	s32 ret = 0;

	if (vo_proc_init(ctx) < 0) {
		CVI_TRACE_VO(CVI_DBG_ERR, "vo proc init failed\n");
		return -EAGAIN;
	}

	if (vo_disp_proc_init() < 0) {
		CVI_TRACE_VO(CVI_DBG_ERR, "proc init failed\n");
		return -EAGAIN;
	}
	return ret;
}

void _vo_destroy_proc(void)
{
	vo_disp_proc_remove();
	vo_proc_remove();
}

s32 vo_set_interface(VO_DEV VoDev, struct cvi_disp_intf_cfg *cfg)
{
	struct cvi_vo_dev *vdev = gVdev;

	if (smooth[VoDev]) {
		CVI_TRACE_VO(CVI_DBG_DEBUG, "set_interface won't apply if smooth.\n");
		disp_reg_force_up(VoDev);
		vdev->vo_core[VoDev].disp_interface = cfg->intf_type;
		return CVI_SUCCESS;
	}

	if (atomic_read(&vdev->vo_core[VoDev].disp_streamon) == 1) {
		CVI_TRACE_VO(CVI_DBG_INFO, "set_interface can't be control if streaming.\n");
		return CVI_SUCCESS;
	}

	if (cfg->intf_type == CVI_VIP_DISP_INTF_DSI) {
		CVI_TRACE_VO(CVI_DBG_INFO, "MIPI use mipi_tx to control.\n");
		return CVI_SUCCESS;
	} else if (cfg->intf_type == CVI_VIP_DISP_INTF_HDMI) {
		CVI_TRACE_VO(CVI_DBG_INFO, "HDMI use hdmi_tx to control.\n");
		return CVI_SUCCESS;
	} else if (cfg->intf_type == CVI_VIP_DISP_INTF_LVDS) {
		s32 i = 0;
		union disp_lvdstx lvds_cfg;
		bool data_en[LANE_MAX_NUM] = {false, false, false, false, false};

		for (i = 0; i < LANE_MAX_NUM; i++) {
			if ((cfg->lvds_cfg.lane_id[i] < 0) ||
				(cfg->lvds_cfg.lane_id[i] >= LANE_MAX_NUM)) {
				dphy_dsi_set_lane(VoDev, i, DSI_LANE_MAX, false, false);
				continue;
			}
			dphy_dsi_set_lane(VoDev, i, cfg->lvds_cfg.lane_id[i],
					  cfg->lvds_cfg.lane_pn_swap[i], false);
			if (cfg->lvds_cfg.lane_id[i] != MIPI_TX_LANE_CLK) {
				data_en[cfg->lvds_cfg.lane_id[i] - 1] = true;
			}
		}
		dphy_dsi_lane_en(VoDev, true, data_en, false);

		disp_set_intf(VoDev, DISP_VO_INTF_LVDS);

		if (cfg->lvds_cfg.pixelclock == 0) {
			CVI_TRACE_VO(CVI_DBG_ERR, "lvds pixelclock 0 invalid\n");
			return -1;
		}
		lvds_cfg.b.out_bit = cfg->lvds_cfg.out_bits;
		lvds_cfg.b.vesa_mode = cfg->lvds_cfg.mode;
		if (cfg->lvds_cfg.chn_num == 1)
			lvds_cfg.b.dual_ch = 0;
		else if (cfg->lvds_cfg.chn_num == 2)
			lvds_cfg.b.dual_ch = 1;
		else {
			lvds_cfg.b.dual_ch = 0;
			CVI_TRACE_VO(CVI_DBG_ERR, "invalid lvds chn_num(%d). Use 1 instead."
				, cfg->lvds_cfg.chn_num);
		}
		lvds_cfg.b.vs_out_en = cfg->lvds_cfg.vs_out_en;
		lvds_cfg.b.hs_out_en = cfg->lvds_cfg.hs_out_en;
		lvds_cfg.b.hs_blk_en = cfg->lvds_cfg.hs_blk_en;
		lvds_cfg.b.ml_swap = cfg->lvds_cfg.msb_lsb_data_swap;
		lvds_cfg.b.ctrl_rev = cfg->lvds_cfg.serial_msb_first;
		lvds_cfg.b.oe_swap = cfg->lvds_cfg.even_odd_link_swap;
		lvds_cfg.b.en = cfg->lvds_cfg.enable;
		dphy_lvds_set_pll(VoDev, cfg->lvds_cfg.pixelclock, cfg->lvds_cfg.chn_num);
		disp_lvdstx_set(VoDev, lvds_cfg);
	} else if (cfg->intf_type == CVI_VIP_DISP_INTF_I80) {
		union disp_bt_enc enc;
		union disp_bt_sync_code sync;

#if 0 // TODO: to be removed, should be done in uboot
		_disp_sel_pinmux(cfg->intf_type, &cfg->bt_cfg);
#endif
		disp_set_intf(VoDev, DISP_VO_INTF_I80);
		enc.raw = 0;
		enc.b.fmt_sel = 2;
		enc.b.clk_inv = 1;
		sync.raw = 0;
		sync.b.sav_vld = 0x80;
		sync.b.sav_blk = 0xab;
		sync.b.eav_vld = 0x9d;
		sync.b.eav_blk = 0xb6;
		disp_bt_set(VoDev, enc, sync);
	} else if (cfg->intf_type == CVI_VIP_DISP_INTF_BT) {
		union disp_bt_enc enc;
		union disp_bt_sync_code sync;

		if (cfg->bt_cfg.mode == BT_MODE_1120)
			disp_set_intf(VoDev, DISP_VO_INTF_BT1120);
		else if (cfg->bt_cfg.mode == BT_MODE_656)
			disp_set_intf(VoDev, DISP_VO_INTF_BT656);
		else if (cfg->bt_cfg.mode == BT_MODE_601)
			disp_set_intf(VoDev, DISP_VO_INTF_BT601);
		else {
			CVI_TRACE_VO(CVI_DBG_ERR, "invalid bt-mode(%d)\n", cfg->bt_cfg.mode);
			return -1;
		}

		//set csc value
		disp_set_out_csc(VoDev, DISP_CSC_601_FULL_RGB2YUV);
#if 0 // TODO: to be removed, should be done in uboot
		_disp_sel_pinmux(cfg->intf_type, &cfg->bt_cfg);
#endif
		enc.raw = 0;
		enc.b.fmt_sel = cfg->bt_cfg.mode;
		sync.b.sav_vld = 0x80;
		sync.b.sav_blk = 0xab;
		sync.b.eav_vld = 0x9d;
		sync.b.eav_blk = 0xb6;
		disp_bt_set(VoDev, enc, sync);
	} else {
		CVI_TRACE_VO(CVI_DBG_ERR, "invalid disp-intf(%d)\n", cfg->intf_type);
		return -1;
	}
	disp_reg_force_up(VoDev);

	vdev->vo_core[VoDev].disp_interface = cfg->intf_type;

	return CVI_SUCCESS;
}


/*******************************************************
 *  File operations for core
 ******************************************************/
void vo_fill_disp_timing(struct disp_timing *timing,
		struct vo_bt_timings *bt_timing)
{

	timing->vtotal = VO_DV_BT_FRAME_HEIGHT(bt_timing) - 1;
	timing->htotal = VO_DV_BT_FRAME_WIDTH(bt_timing) - 1;
	timing->vsync_start = 0;
	timing->vsync_end = timing->vsync_start + bt_timing->vsync - 1;
	timing->vfde_start = timing->vmde_start =
		timing->vsync_start + bt_timing->vsync + bt_timing->vbackporch;
	timing->vfde_end = timing->vmde_end =
		timing->vfde_start + bt_timing->height - 1;
	timing->hsync_start = 0;
	timing->hsync_end = timing->hsync_start + bt_timing->hsync - 1;
	timing->hfde_start = timing->hmde_start =
		timing->hsync_start + bt_timing->hsync + bt_timing->hbackporch;
	timing->hfde_end = timing->hmde_end =
		timing->hfde_start + bt_timing->width - 1;
	timing->vsync_pol = bt_timing->polarities & VO_DV_VSYNC_POS_POL;
	timing->hsync_pol = bt_timing->polarities & VO_DV_HSYNC_POS_POL;
}


static void _vo_hw_enque(VO_DEV VoDev, struct cvi_vo_layer_ctx *pstLayerCtx)
{
	struct vo_buffer *buf;
	struct cvi_disp_buffer *b = NULL;
	struct cvi_disp_buffer *work_buf = NULL;
	struct disp_cfg *cfg;
	s32 i = 0;
	unsigned long flags;

	spin_lock_irqsave(&pstLayerCtx->list_lock, flags);
	if (!list_empty(&pstLayerCtx->list_work)) {
		work_buf = list_first_entry(&pstLayerCtx->list_work,
			struct cvi_disp_buffer, list);
		list_move_tail(&work_buf->list, &pstLayerCtx->list_done);
	}
	if (!list_empty(&pstLayerCtx->list_wait)) {
		b = list_first_entry(&pstLayerCtx->list_wait,
			struct cvi_disp_buffer, list);
		list_move_tail(&b->list, &pstLayerCtx->list_work);
	}
	spin_unlock_irqrestore(&pstLayerCtx->list_lock, flags);

	if (b == NULL)
		return;

	buf = &b->buf;
	for (i = 0; i < 3; i++) {
		CVI_TRACE_VO(CVI_DBG_INFO, "b->buf.planes[%d].addr=%llx\n", i, b->buf.planes[i].addr);
	}

	disp_enable_window_bgcolor(VoDev, false);
	cfg = disp_get_cfg(VoDev);
	cfg->mem.addr0 = b->buf.planes[0].addr;
	cfg->mem.addr1 = b->buf.planes[1].addr;
	cfg->mem.addr2 = b->buf.planes[2].addr;
	cfg->mem.pitch_y = b->buf.planes[0].bytesused;
	cfg->mem.pitch_c = b->buf.planes[1].bytesused;
	disp_set_mem(VoDev, &cfg->mem);

	pstLayerCtx->u64DisplayPts = ((struct vb_s *)b->blk)->buf.u64PTS;
}


static void _i80_package_eol(u8 *buffer)
{
	// pull high i80-lane
	buffer[0] = 0xff;
	buffer[1] = i80_ctrl[I80_CTRL_EOF];
	buffer[2] = I80_OP_GO;
}

static void _i80_package_eof(u8 *buffer)
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

static u32 _MAKECOLOR(u8 r, u8 g, u8 b, VO_I80_FORMAT fmt)
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


static void _i80_package_rgb(VO_I80_FORMAT i80_fmt, u8 r, u8 g,
				u8 b, u8 *buffer, u8 byte_cnt)
{
	u32 pixel, i, offset;

	pixel = _MAKECOLOR(r, g, b, i80_fmt);

	for (i = 0, offset = 0; i < byte_cnt; ++i) {
		*(buffer + offset++) = pixel >> ((byte_cnt - i - 1) << 3);
		*(buffer + offset++) = i80_ctrl[I80_CTRL_DATA];
		*(buffer + offset++) = I80_OP_GO;
	}
}

static void _i80_package_frame(VO_I80_FORMAT i80_fmt, PIXEL_FORMAT_E layer_fmt,
				struct vb_s *in, u8 *buffer, u8 byte_cnt)
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
			_get_frame_rgb(layer_fmt, in_buf_vir,
						in->buf.stride, x, y, &r, &g, &b);
			_i80_package_rgb(i80_fmt, r, g, b, buffer + out_offset, byte_cnt);
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

static s32 _i80_transform_frame(VO_I80_FORMAT i80_fmt,
		PIXEL_FORMAT_E layer_fmt, VB_BLK blk_in, VB_BLK blk_out)
{
	struct vb_s *vb_i80;
	struct vb_s *vb_in = (struct vb_s *)blk_in;
	u32 buf_size;
	u8 byte_cnt = (i80_fmt == VO_I80_FORMAT_RGB666) ? 3 : 2;

	buf_size = ALIGN((vb_in->buf.size.u32Width * byte_cnt + 1) * 3, 32) * vb_in->buf.size.u32Height;

	vb_i80 = (struct vb_s *)blk_out;
	memset(&vb_i80->buf, 0, sizeof(vb_i80->buf));

	//vb_i80->vir_addr = CVI_SYS_MmapCache(vb_i80->phy_addr, buf_size);
	if (vb_i80->vir_addr == CVI_NULL) {
		CVI_TRACE_VO(CVI_DBG_INFO, "vb_i80->vir_addr NULL.\n");
		return CVI_FAILURE;
	}
	_i80_package_frame(i80_fmt, layer_fmt, vb_in, vb_i80->vir_addr, byte_cnt);

	//CVI_SYS_IonFlushCache(vb_i80->phy_addr, vb_i80->vir_addr, buf_size);
	//CVI_SYS_Munmap(vb_i80->vir_addr, buf_size);
	vb_i80->buf.enPixelFormat = PIXEL_FORMAT_RGB_888;
	vb_i80->buf.phy_addr[0] = vb_i80->phy_addr;
	vb_i80->buf.length[0] = buf_size;
	vb_i80->buf.stride[0] = ALIGN((vb_in->buf.size.u32Width * byte_cnt + 1) * 3, 32);

	return CVI_SUCCESS;
}

static s32 simplify_rate(u32 dst_in, u32 src_in, u32 *dst_out, u32 *src_out)
{
	u32 i = 1;
	u32 a, b;

	while (i < dst_in + 1) {
		a = dst_in % i;
		b = src_in % i;
		if (a == 0 && b == 0) {
			dst_in = dst_in / i;
			src_in = src_in / i;
			i = 1;
		}
		i++;
	}
	*dst_out = dst_in;
	*src_out = src_in;
	return CVI_SUCCESS;
}

static u8 vo_frame_ctrl(u64 u64FrameIndex, FRAME_RATE_CTRL_S *pstFrameRate)
{
	u32 src_simp;
	u32 dst_simp;
	u32 u32Index;
	u32 srcDur, dstDur;
	u32 curIndx, nextIndx;

	simplify_rate(pstFrameRate->s32DstFrameRate, pstFrameRate->s32SrcFrameRate,
		&dst_simp, &src_simp);

	u32Index = u64FrameIndex % src_simp;
	if (u32Index == 0) {
		return CVI_TRUE;
	}
	srcDur = 100;
	dstDur = (srcDur * src_simp) / dst_simp;
	curIndx = (u32Index - 1) * srcDur / dstDur;
	nextIndx = u32Index * srcDur / dstDur;

	if (nextIndx == curIndx)
		return CVI_FALSE;

	return CVI_TRUE;
}

static void vo_snap(MMF_CHN_S chn, struct vb_jobs_t *jobs, VB_BLK blk)
{
	struct vb_s *p = (struct vb_s *)blk;
	struct vbq *doneq;
	struct snap_s *s, *s_tmp;

	if (jobs == NULL) {
		pr_err("handle snap fail, Null parameter\n");
		return;
	}

	if (!jobs->inited) {
		pr_err("handle snap fail, job not inited yet\n");
		return;
	}

	mutex_lock(&jobs->dlock);
	TAILQ_FOREACH_SAFE(s, &jobs->snap_jobs, tailq, s_tmp) {
		if (CHN_MATCH(&s->chn, &chn)) {
			TAILQ_REMOVE(&jobs->snap_jobs, s, tailq);
			s->blk = blk;
			atomic_fetch_add(1, &p->usr_cnt);
			atomic_long_fetch_or(BIT(CVI_ID_USER), &p->mod_ids);
			s->avail = CVI_TRUE;
			wake_up(&s->cond_queue);
			mutex_unlock(&jobs->dlock);
			return;
		}
	}

	doneq = &jobs->doneq;
	// check if there is a snap-queue
	if (FIFO_CAPACITY(doneq)) {
		if (FIFO_FULL(doneq)) {
			struct vb_s *vb = NULL;

			FIFO_POP(doneq, &vb);
			atomic_long_fetch_and(~BIT(chn.enModId), &vb->mod_ids);
			vb_release_block((VB_BLK)vb);
		}
		atomic_fetch_add(1, &p->usr_cnt);
		atomic_long_fetch_or(BIT(chn.enModId), &p->mod_ids);
		FIFO_PUSH(doneq, p);
	}
	mutex_unlock(&jobs->dlock);
}

/* aspect_ratio_resize: calculate the new rect to keep aspect ratio
 *   according to given in/out size.
 *
 * @param in: input video size.
 * @param out: output display size.
 *
 * @return: the rect which describe the video on output display.
 */
static RECT_S aspect_ratio_resize(SIZE_S in, SIZE_S out)
{
	RECT_S rect;
	u32 scale = in.u32Height * in.u32Width;
	u32 ratio_int = MIN2(out.u32Width * in.u32Height, out.u32Height * in.u32Width);
	u64 height, width;

	height = (u64)in.u32Height * ratio_int + scale/2;
	do_div(height, scale);
	rect.u32Height = (u32)height;

	width = (u64)in.u32Width * ratio_int + scale/2;
	do_div(width, scale);
	rect.u32Width = (u32)width;

	rect.s32X = (out.u32Width - rect.u32Width) >> 1;
	rect.s32Y = (out.u32Height - rect.u32Height) >> 1;

	return rect;
}

static void _vo_gdc_callback(void *pParam, VB_BLK blk)
{
	struct _vo_gdc_cb_param *cb_param = NULL;
	struct cvi_vo_chn_ctx *pstChnCtx;
	MMF_CHN_S chn;
	struct vb_jobs_t *jobs;
	struct vb_s *vb = (struct vb_s *)blk;

	if (!pParam)
		return;

	cb_param = (struct _vo_gdc_cb_param *)pParam;
	chn = cb_param->chn;
	pstChnCtx = &gVoCtx->astLayerCtx[chn.s32DevId].astChnCtx[chn.s32ChnId];

	if (!pstChnCtx->is_chn_enable) {
		CVI_TRACE_VO(CVI_DBG_INFO, "layer(%d) chn(%d) disable.\n", chn.s32DevId, chn.s32ChnId);
		mutex_unlock(&pstChnCtx->gdc_lock);
		vb_release_block(blk);
		vfree(pParam);
		pParam = NULL;
		return;
	}

	if ((pstChnCtx->bPause && (!pstChnCtx->bRefresh)) || pstChnCtx->bHide) {
		CVI_TRACE_VO(CVI_DBG_INFO, "layer(%d) chn(%d) pause/hide.\n", chn.s32DevId, chn.s32ChnId);
		mutex_unlock(&pstChnCtx->gdc_lock);
		vb_release_block(blk);
		vfree(pParam);
		pParam = NULL;
		return;
	}
	if (IS_VB_OFFSET_INVALID(vb->buf)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) vb offset (%d %d %d %d) invalid\n",
			chn.s32DevId, chn.s32ChnId,
			vb->buf.s16OffsetLeft, vb->buf.s16OffsetRight,
			vb->buf.s16OffsetTop, vb->buf.s16OffsetBottom);
		mutex_unlock(&pstChnCtx->gdc_lock);
		vb_release_block(blk);
		vfree(pParam);
		pParam = NULL;
		return;
	}

	mutex_unlock(&pstChnCtx->gdc_lock);

	jobs = &pstChnCtx->chn_jobs;
	mutex_lock(&jobs->lock);
	if (FIFO_FULL(&jobs->waitq)) {
		struct vb_s *vb_old = NULL;

		FIFO_POP(&jobs->waitq, &vb_old);

		atomic_long_fetch_and(~BIT(chn.enModId), &vb_old->mod_ids);
		vb_release_block((VB_BLK)vb_old);
	}
	FIFO_PUSH(&jobs->waitq, vb);
	mutex_unlock(&jobs->lock);
	CVI_TRACE_VO(CVI_DBG_INFO, "VoLayer(%d) VoChn(%d) push vb(0x%llx).\n",
		chn.s32DevId, chn.s32ChnId, vb->phy_addr);

	atomic_long_fetch_or(BIT(chn.enModId), &vb->mod_ids);
	gVoCtx->astLayerCtx[chn.s32DevId].bLayerUpdate = CVI_TRUE;
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


static s32 _vo_get_chn_buffers(struct cvi_vo_layer_ctx *pstLayerCtx, VB_BLK *blk)
{
	VO_CHN VoChn;
	struct cvi_vo_chn_ctx *pstChnCtx;
	struct vb_jobs_t *jobs;
	struct vb_s *old_workq = NULL;
	struct vb_s *new_workq = NULL;
	struct vb_s *vb;
	s32 s32ChnNum = 0;
	VO_DEV VoDev = pstLayerCtx->s32BindDevId;
	MMF_CHN_S chn = {.enModId = CVI_ID_VO, .s32DevId = VoDev, .s32ChnId = 0};

	for (VoChn = 0; VoChn < VO_MAX_CHN_NUM; ++VoChn) {
		pstChnCtx = &pstLayerCtx->astChnCtx[VoChn];
		jobs = &pstChnCtx->chn_jobs;

		if (!pstChnCtx->is_chn_enable)
			continue;
		if (pstChnCtx->bHide)
			continue;
		if (pstChnCtx->bPause) {
			mutex_lock(&jobs->lock);
			if (!FIFO_EMPTY(&jobs->waitq) && pstChnCtx->bRefresh == CVI_TRUE) {
				if (!FIFO_EMPTY(&jobs->workq))
					FIFO_POP(&jobs->workq, &old_workq);

				FIFO_POP(&jobs->waitq, &vb);
				FIFO_PUSH(&jobs->workq, vb);

				FIFO_GET_FRONT(&jobs->workq, &new_workq);
				blk[VoChn] = (VB_BLK)new_workq;
				if (old_workq) {
					vb_release_block((VB_BLK)old_workq);
					old_workq = NULL;
				}
				s32ChnNum++;
				pstChnCtx->bRefresh = CVI_FALSE;
			} else if (!FIFO_EMPTY(&jobs->workq)) {
				FIFO_GET_FRONT(&jobs->workq, &vb);
				blk[VoChn] = (VB_BLK)vb;
				s32ChnNum++;
			}
			mutex_unlock(&jobs->lock);
			continue;
		} else if (pstChnCtx->bStep) {
			mutex_lock(&jobs->lock);
			if (!FIFO_EMPTY(&jobs->waitq) && pstChnCtx->bStepTrigger == CVI_TRUE) {
				if (!FIFO_EMPTY(&jobs->workq))
					FIFO_POP(&jobs->workq, &old_workq);

				FIFO_POP(&jobs->waitq, &vb);
				FIFO_PUSH(&jobs->workq, vb);

				FIFO_GET_FRONT(&jobs->workq, &new_workq);
				blk[VoChn] = (VB_BLK)new_workq;
				if (old_workq) {
					vb_release_block((VB_BLK)old_workq);
					old_workq = NULL;
				}
				s32ChnNum++;
				pstChnCtx->bStepTrigger = CVI_FALSE;
			} else if (!FIFO_EMPTY(&jobs->workq)) {
				FIFO_GET_FRONT(&jobs->workq, &vb);
				blk[VoChn] = (VB_BLK)vb;
				s32ChnNum++;
			}
			mutex_unlock(&jobs->lock);
			continue;
		}

		mutex_lock(&jobs->lock);
		if (FIFO_EMPTY(&jobs->workq) && FIFO_EMPTY(&jobs->waitq)) {
			mutex_unlock(&jobs->lock);
			continue;
		}

		if (!FIFO_EMPTY(&jobs->waitq)) {
			if (!FIFO_EMPTY(&jobs->workq))
				FIFO_POP(&jobs->workq, &old_workq);

			FIFO_POP(&jobs->waitq, &vb);
			FIFO_PUSH(&jobs->workq, vb);
		}
		mutex_unlock(&jobs->lock);

		FIFO_GET_FRONT(&jobs->workq, &new_workq);
		blk[VoChn] = (VB_BLK)new_workq;
		if (old_workq) {
			pstChnCtx->u64PreDonePts = old_workq->buf.u64PTS;
			chn.s32ChnId = VoChn;
			if (pstChnCtx->stChnAttr.u32Depth)
				vo_snap(chn, &pstChnCtx->chn_jobs, (VB_BLK)old_workq);
			vb_release_block((VB_BLK)old_workq);
			old_workq = NULL;
		}
		s32ChnNum++;
	}

	pstLayerCtx->bLayerUpdate = CVI_FALSE;

	return s32ChnNum;
}

void vo_stitch_wakeup(void *data)
{
	struct vo_stitch_cb_data *pstStitchData = (struct vo_stitch_cb_data *)data;

	CVI_TRACE_VO(CVI_DBG_INFO, "vo stitch wakeup.\n");
	pstStitchData->u8Flag = 1;
	wake_up(&pstStitchData->wait);
}

void vo_sort_chn_priority(s32 *u32Priority, u32 length, s32 *u32Index)
{
    s32 i, j, t1, t2;

    for (j = 0; j < length; j++) {
        for (i = 0; i < (length - 1 - j); i++) {
            if (u32Priority[i] > u32Priority[i + 1])
            {
                t2 = u32Priority[i];
                u32Priority[i] = u32Priority[i + 1];
                u32Priority[i + 1] = t2;
                t1 = u32Index[i];
                u32Index[i] = u32Index[i + 1];
                u32Index[i + 1] = t1;
            }
		}
	}
}

static s32 layer_process(struct cvi_vo_layer_ctx *pstLayerCtx)
{
	s32 ret = CVI_SUCCESS;
	VO_DEV VoDev = pstLayerCtx->s32BindDevId;
	struct cvi_vo_chn_ctx *pstChnCtx;
	struct vpss_stitch_cfg stStitchCfg;
	struct stitch_dst_cfg *pstDstcfg = &stStitchCfg.dst_cfg;
	struct stitch_chn_cfg *pstChnfg = NULL;
	unsigned long timeout = msecs_to_jiffies(WAIT_TIMEOUT_MS);
	u8 i, n = 0;
	u32 u32ChnNum;
	struct vo_stitch_cb_data stStitchData;
	VB_BLK blk_next = VB_INVALID_HANDLE;
	VB_BLK blk_out = VB_INVALID_HANDLE;
	struct vb_s *vb;
	SIZE_S stOutSize = pstLayerCtx->stLayerAttr.stImageSize;
	VB_BLK blks[VO_MAX_CHN_NUM] = { [0 ... VO_MAX_CHN_NUM - 1] = VB_INVALID_HANDLE };
	struct cvi_disp_buffer *pstDispBuf;
	unsigned long flags;
	VB_CAL_CONFIG_S stVbCalConfig;
	MMF_CHN_S chn = {.enModId = CVI_ID_VO, .s32DevId = VoDev, .s32ChnId = 0};
	struct timespec64 time;
	u64 u64PTS;
	u8 is_wbc_match = 0;
	FRAME_RATE_CTRL_S layer_frame_ctrl;
	VO_CHN_ZOOM_RATIO stZoomRatio;
	RECT_S stZoomRect;
	u8 bNeedZoom;
	s32 as32Priority[VO_MAX_CHN_NUM] = { 0 };
	s32 as32Index[VO_MAX_CHN_NUM] = { 0 };
	u8 j = 0;

	pstLayerCtx->u32FrameIndex++;
	pstLayerCtx->u32SrcFrameNum++;

	u32ChnNum = _vo_get_chn_buffers(pstLayerCtx, blks);
	if (!u32ChnNum)
		return CVI_SUCCESS;

	layer_frame_ctrl.s32SrcFrameRate = pstLayerCtx->u32LayerSrcFrameRate;
	layer_frame_ctrl.s32DstFrameRate = pstLayerCtx->stLayerAttr.u32DispFrmRt;

	if ((((pstLayerCtx->u64DisplayPts - pstLayerCtx->u64PreDonePts) / 1000)
		> pstLayerCtx->u32Toleration) && (pstLayerCtx->u32Toleration > 0)) {
		pstLayerCtx->u32FrameIndex = 0;
	}

	if (!FRC_INVALID(layer_frame_ctrl) && (!vo_frame_ctrl(pstLayerCtx->u32FrameIndex, &layer_frame_ctrl))) {
		pstLayerCtx->bIsDrop = CVI_TRUE;
	}

	if (pstLayerCtx->bIsDrop) {
		pstLayerCtx->bIsDrop = CVI_FALSE;
		return CVI_SUCCESS;
	}

	if (!list_empty(&pstLayerCtx->list_done)) {
		//get a new vb
		COMMON_GetPicBufferConfig(pstLayerCtx->stLayerAttr.stImageSize.u32Width,
		pstLayerCtx->stLayerAttr.stImageSize.u32Height,
		pstLayerCtx->stLayerAttr.enPixFormat, DATA_BITWIDTH_8
		, COMPRESS_MODE_NONE, DEFAULT_ALIGN, &stVbCalConfig);

		blk_next = vb_get_block_with_id(VB_INVALID_POOLID, stVbCalConfig.u32VBSize, CVI_ID_VO);
		if (blk_next == VB_INVALID_HANDLE) {
			CVI_TRACE_VO(CVI_DBG_ERR, "get vb block fail.\n");
			return CVI_FAILURE;
		}

		spin_lock_irqsave(&pstLayerCtx->list_lock, flags);
		pstDispBuf = list_first_entry(&pstLayerCtx->list_done,
			struct cvi_disp_buffer, list);
		list_del_init(&pstDispBuf->list);
		spin_unlock_irqrestore(&pstLayerCtx->list_lock, flags);

		blk_out = pstDispBuf->blk;
		pstDispBuf->blk = blk_next;

		vb = (struct vb_s *)blk_out;
		vb->buf.dev_num = VoDev;
		vb->buf.frm_num = pstLayerCtx->u32DoneCnt;
		pstLayerCtx->u64PreDonePts = vb->buf.u64PTS;

		//for wbc get layer frame
		for (i = 0; i < VO_MAX_WBC_NUM; ++i) {
			if ((gVoCtx->astWbcCtx[i].is_wbc_enable)
				&& (gVoCtx->astWbcCtx[i].stWbcSrc.enSrcType == VO_WBC_SRC_VIDEO)
				&& (gVoCtx->astWbcCtx[i].stWbcSrc.u32SrcId == pstLayerCtx->s32BindDevId)) {
				if (pstLayerCtx->stLayerAttr.u32Depth) {
					vb_done_handler(chn, CHN_TYPE_OUT, &pstLayerCtx->layer_jobs, blk_out);
					gVoCtx->astWbcCtx[i].u32FrameNum++;
				} else {
					vb_release_block(blk_out);
				}
				is_wbc_match = CVI_TRUE;
			}
		}

		//for get screen frame
		if (!is_wbc_match) {
			if (pstLayerCtx->stLayerAttr.u32Depth)
				vo_snap(chn, &pstLayerCtx->layer_jobs, blk_out);
			vb_release_block(blk_out);
		}

		pstLayerCtx->u32DoneCnt++;

	} else
		return CVI_FAILURE;

	pstChnfg = (struct stitch_chn_cfg *)vmalloc(sizeof(struct stitch_chn_cfg) * u32ChnNum);
	if (!pstChnfg) {
		CVI_TRACE_VO(CVI_DBG_ERR, "vmalloc fail.\n");
		goto err0;
	}

	mutex_lock(&pstLayerCtx->layer_lock);
	//Chn priority ctrl
	for (i = 0; i < VO_MAX_CHN_NUM; i++) {
		if (blks[i] == VB_INVALID_HANDLE)
			continue;
		pstChnCtx = &pstLayerCtx->astChnCtx[i];
		as32Priority[j] = pstChnCtx->stChnAttr.u32Priority;
		as32Index[j] = i;
		j++;
	}

	vo_sort_chn_priority(as32Priority, u32ChnNum, as32Index);

	for (i = 0; i < u32ChnNum; i++) {
		vb = (struct vb_s *)blks[as32Index[i]];
		pstChnCtx = &pstLayerCtx->astChnCtx[as32Index[i]];

		CVI_TRACE_VO(CVI_DBG_INFO, "layer(%d) chn(%d) get vb(0x%llx).\n",
			pstLayerCtx->VoLayer, as32Index[i], vb->phy_addr);

		pstChnCtx->u64DisplayPts = vb->buf.u64PTS;

		pstChnfg[n].pixelformat = vb->buf.enPixelFormat;
		pstChnfg[n].bytesperline[0] = vb->buf.stride[0];
		pstChnfg[n].bytesperline[1] = vb->buf.stride[1];
		pstChnfg[n].addr[0] = vb->buf.phy_addr[0];
		pstChnfg[n].addr[1] = vb->buf.phy_addr[1];
		pstChnfg[n].addr[2] = vb->buf.phy_addr[2];
		pstChnfg[n].src_size.width = vb->buf.size.u32Width;
		pstChnfg[n].src_size.height = vb->buf.size.u32Height;
		pstChnCtx->u32SrcWidth = vb->buf.size.u32Width;
		pstChnCtx->u32SrcHeight = vb->buf.size.u32Height;

		//Zoom
		if (pstChnCtx->stChnZoomAttr.enZoomType == VO_CHN_ZOOM_IN_RECT) {
			stZoomRect = pstChnCtx->stChnZoomAttr.stRect;
			bNeedZoom = stZoomRect.s32X || stZoomRect.s32Y || stZoomRect.u32Width || stZoomRect.u32Height;
		} else if (pstChnCtx->stChnZoomAttr.enZoomType == VO_CHN_ZOOM_IN_RATIO) {
			stZoomRatio = pstChnCtx->stChnZoomAttr.stZoomRatio;
			bNeedZoom = stZoomRatio.u32Xratio || stZoomRatio.u32Yratio || stZoomRatio.u32WidthRatio || stZoomRatio.u32HeightRatio;
		} else {
			bNeedZoom = CVI_FALSE;
		}

		if ((pstChnCtx->stChnZoomAttr.enZoomType == VO_CHN_ZOOM_IN_RECT) && bNeedZoom) {
			pstChnfg[n].rect_crop.left = vb->buf.s16OffsetLeft + stZoomRect.s32X;
			pstChnfg[n].rect_crop.top = vb->buf.s16OffsetTop + stZoomRect.s32Y;
			pstChnfg[n].rect_crop.width = stZoomRect.u32Width;
			pstChnfg[n].rect_crop.height = stZoomRect.u32Height;

			if (stZoomRect.s32X < 0) {
				pstChnfg[n].rect_crop.left = 0;
				pstChnfg[n].rect_crop.width = (stZoomRect.u32Width - ABS(stZoomRect.s32X));
			}

			if (stZoomRect.s32X + stZoomRect.u32Width > pstChnfg[n].src_size.width) {
				pstChnfg[n].rect_crop.width = pstChnfg[n].src_size.width - pstChnfg[n].rect_crop.left;
			}

			if (stZoomRect.s32Y < 0) {
				pstChnfg[n].rect_crop.top = 0;
				pstChnfg[n].rect_crop.height = (stZoomRect.u32Height - ABS(stZoomRect.s32Y));
			}

			if (stZoomRect.s32Y + stZoomRect.u32Height > pstChnfg[n].src_size.height) {
				pstChnfg[n].rect_crop.height = pstChnfg[n].src_size.height - pstChnfg[n].rect_crop.top;
			}

			if((pstChnfg[n].rect_crop.width < VO_MIN_CHN_WIDTH) || (pstChnfg[n].rect_crop.height < VO_MIN_CHN_HEIGHT)) {
				CVI_TRACE_VO(CVI_DBG_INFO, "rect_crop(%d %d) < 32 invalid.\n", pstChnfg[n].rect_crop.width, pstChnfg[n].rect_crop.height);
				pstChnfg[n].rect_crop.left = vb->buf.s16OffsetLeft;
				pstChnfg[n].rect_crop.top = vb->buf.s16OffsetTop;
				pstChnfg[n].rect_crop.width = vb->buf.size.u32Width - vb->buf.s16OffsetLeft - vb->buf.s16OffsetRight;
				pstChnfg[n].rect_crop.height = vb->buf.size.u32Height - vb->buf.s16OffsetTop - vb->buf.s16OffsetBottom;
			}
		} else if ((pstChnCtx->stChnZoomAttr.enZoomType == VO_CHN_ZOOM_IN_RATIO) && bNeedZoom) {
			pstChnfg[n].rect_crop.left = stZoomRatio.u32Xratio * pstChnfg[n].src_size.width / 1000;
			pstChnfg[n].rect_crop.top = stZoomRatio.u32Yratio * pstChnfg[n].src_size.height / 1000;
			pstChnfg[n].rect_crop.width = stZoomRatio.u32WidthRatio * pstChnfg[n].src_size.width & ~0x01;
			pstChnfg[n].rect_crop.height = stZoomRatio.u32HeightRatio * pstChnfg[n].src_size.height & ~0x01;

			if (pstChnfg[n].rect_crop.left + pstChnfg[n].rect_crop.width > pstChnfg[n].src_size.width) {
				pstChnfg[n].rect_crop.width = pstChnfg[n].src_size.width - pstChnfg[n].rect_crop.left;
			}

			if (pstChnfg[n].rect_crop.top + pstChnfg[n].rect_crop.height > pstChnfg[n].src_size.height) {
				pstChnfg[n].rect_crop.height = pstChnfg[n].src_size.height - pstChnfg[n].rect_crop.top;
			}

			if((pstChnfg[n].rect_crop.width < VO_MIN_CHN_WIDTH) || (pstChnfg[n].rect_crop.height < VO_MIN_CHN_HEIGHT)) {
				CVI_TRACE_VO(CVI_DBG_INFO, "rect_crop(%d %d) < 32 invalid.\n", pstChnfg[n].rect_crop.width, pstChnfg[n].rect_crop.height);
				pstChnfg[n].rect_crop.left = vb->buf.s16OffsetLeft;
				pstChnfg[n].rect_crop.top = vb->buf.s16OffsetTop;
				pstChnfg[n].rect_crop.width = vb->buf.size.u32Width - vb->buf.s16OffsetLeft - vb->buf.s16OffsetRight;
				pstChnfg[n].rect_crop.height = vb->buf.size.u32Height - vb->buf.s16OffsetTop - vb->buf.s16OffsetBottom;
			}
		} else {
			pstChnfg[n].rect_crop.left = vb->buf.s16OffsetLeft;
			pstChnfg[n].rect_crop.top = vb->buf.s16OffsetTop;
			pstChnfg[n].rect_crop.width = vb->buf.size.u32Width - vb->buf.s16OffsetLeft - vb->buf.s16OffsetRight;
			pstChnfg[n].rect_crop.height = vb->buf.size.u32Height - vb->buf.s16OffsetTop - vb->buf.s16OffsetBottom;
		}

		//AspectRatio
		if (pstChnCtx->stChnParam.stAspectRatio.enMode == ASPECT_RATIO_AUTO) {
			SIZE_S in, out;
			RECT_S rect;
			in.u32Width = pstChnfg[n].rect_crop.width;
			in.u32Height = pstChnfg[n].rect_crop.height;
			out.u32Width = pstChnCtx->stChnAttr.stRect.u32Width;
			out.u32Height = pstChnCtx->stChnAttr.stRect.u32Height;
			rect = aspect_ratio_resize(in, out);
			pstChnfg[n].window.rect_in.width = rect.u32Width & ~0x01;
			pstChnfg[n].window.rect_in.height = rect.u32Height & ~0x01;
			pstChnfg[n].window.rect_in.left = (pstChnCtx->stChnAttr.stRect.s32X + rect.s32X) & ~0x01;
			pstChnfg[n].window.rect_in.top = (pstChnCtx->stChnAttr.stRect.s32Y + rect.s32Y) & ~0x01;
			if((pstChnfg[n].window.rect_in.width < VO_MIN_CHN_WIDTH) || (pstChnfg[n].window.rect_in.height < VO_MIN_CHN_HEIGHT)) {
				CVI_TRACE_VO(CVI_DBG_INFO, "rect_in(%d %d) < 32 invalid.\n", pstChnfg[n].window.rect_in.width, pstChnfg[n].window.rect_in.height);
				pstChnfg[n].window.rect_in.width = pstChnCtx->stChnAttr.stRect.u32Width & ~0x01;
				pstChnfg[n].window.rect_in.height = pstChnCtx->stChnAttr.stRect.u32Height & ~0x01;
				pstChnfg[n].window.rect_in.left = pstChnCtx->stChnAttr.stRect.s32X & ~0x01;
				pstChnfg[n].window.rect_in.top = pstChnCtx->stChnAttr.stRect.s32Y & ~0x01;
			}
		} else if (pstChnCtx->stChnParam.stAspectRatio.enMode == ASPECT_RATIO_MANUAL) {
			RECT_S rect = pstChnCtx->stChnParam.stAspectRatio.stVideoRect;
			pstChnfg[n].window.rect_in.width = rect.u32Width & ~0x01;
			pstChnfg[n].window.rect_in.height = rect.u32Height & ~0x01;
			pstChnfg[n].window.rect_in.left = (pstChnCtx->stChnAttr.stRect.s32X + rect.s32X) & ~0x01;
			pstChnfg[n].window.rect_in.top = (pstChnCtx->stChnAttr.stRect.s32Y + rect.s32Y) & ~0x01;
			if((pstChnfg[n].window.rect_in.width < VO_MIN_CHN_WIDTH) || (pstChnfg[n].window.rect_in.height < VO_MIN_CHN_HEIGHT)) {
				CVI_TRACE_VO(CVI_DBG_INFO, "rect_in(%d %d) < 32 invalid.\n", pstChnfg[n].window.rect_in.width, pstChnfg[n].window.rect_in.height);
				pstChnfg[n].window.rect_in.width = pstChnCtx->stChnAttr.stRect.u32Width & ~0x01;
				pstChnfg[n].window.rect_in.height = pstChnCtx->stChnAttr.stRect.u32Height & ~0x01;
				pstChnfg[n].window.rect_in.left = pstChnCtx->stChnAttr.stRect.s32X & ~0x01;
				pstChnfg[n].window.rect_in.top = pstChnCtx->stChnAttr.stRect.s32Y & ~0x01;
			}
		} else {
			pstChnfg[n].window.rect_in.width = pstChnCtx->stChnAttr.stRect.u32Width & ~0x01;
			pstChnfg[n].window.rect_in.height = pstChnCtx->stChnAttr.stRect.u32Height & ~0x01;
			pstChnfg[n].window.rect_in.left = pstChnCtx->stChnAttr.stRect.s32X & ~0x01;
			pstChnfg[n].window.rect_in.top = pstChnCtx->stChnAttr.stRect.s32Y & ~0x01;
		}

		if (pstChnCtx->stChnParam.stAspectRatio.bEnableBgColor) {
			pstChnfg[n].window.bgcolor[0] = (pstChnCtx->stChnParam.stAspectRatio.u32BgColor >> 16) & 0xff; //R
			pstChnfg[n].window.bgcolor[1] = (pstChnCtx->stChnParam.stAspectRatio.u32BgColor >> 8) & 0xff; //G
			pstChnfg[n].window.bgcolor[2] = pstChnCtx->stChnParam.stAspectRatio.u32BgColor & 0xff; //B
		} else {
			pstChnfg[n].window.bgcolor[0] = 0; //R
			pstChnfg[n].window.bgcolor[1] = 0; //G
			pstChnfg[n].window.bgcolor[2] = 0; //B
		}

		pstChnfg[n].window.rect_out.width = pstChnCtx->stChnAttr.stRect.u32Width & ~0x01;
		pstChnfg[n].window.rect_out.height = pstChnCtx->stChnAttr.stRect.u32Height & ~0x01;
		pstChnfg[n].window.rect_out.left = pstChnCtx->stChnAttr.stRect.s32X & ~0x01;
		pstChnfg[n].window.rect_out.top = pstChnCtx->stChnAttr.stRect.s32Y & ~0x01;

		//Border
		if (pstChnCtx->stChnBorder.enable) {
			pstChnfg[n].window.top_width = pstChnCtx->stChnBorder.stBorder.u32TopWidth;
			pstChnfg[n].window.bottom_width = pstChnCtx->stChnBorder.stBorder.u32BottomWidth;
			pstChnfg[n].window.left_width = pstChnCtx->stChnBorder.stBorder.u32LeftWidth;
			pstChnfg[n].window.right_width = pstChnCtx->stChnBorder.stBorder.u32RightWidth;
			pstChnfg[n].window.border_color[0] = (pstChnCtx->stChnBorder.stBorder.u32Color >> 16) & 0xff; //R
			pstChnfg[n].window.border_color[1] = (pstChnCtx->stChnBorder.stBorder.u32Color >> 8) & 0xff; //G
			pstChnfg[n].window.border_color[2] = pstChnCtx->stChnBorder.stBorder.u32Color & 0xff; //B
		} else {
			pstChnfg[n].window.top_width = 0;
			pstChnfg[n].window.bottom_width = 0;
			pstChnfg[n].window.left_width = 0;
			pstChnfg[n].window.right_width = 0;
			pstChnfg[n].window.border_color[0] = 0; //R
			pstChnfg[n].window.border_color[1] = 0; //G
			pstChnfg[n].window.border_color[2] = 0; //B
		}

		pstChnfg[n].window.flip = pstChnCtx->enChnMirror;

		CVI_TRACE_VO(CVI_DBG_INFO, "layer(%d) pixelformat(%d) bytesperline(%d %d) addr(%llx, %llx, %llx) "
			"src_size(%d %d ) crop(%d %d %d %d) rect_dst_in(%d %d %d %d) rect_dst_out(%d %d %d %d) "
			"border(%d %d %d %d) mirror(%d).\n",
			pstLayerCtx->VoLayer, pstChnfg[n].pixelformat,
			pstChnfg[n].bytesperline[0], pstChnfg[n].bytesperline[1],
			pstChnfg[n].addr[0], pstChnfg[n].addr[1], pstChnfg[n].addr[2],
			pstChnfg[n].src_size.width, pstChnfg[n].src_size.height,
			pstChnfg[n].rect_crop.left, pstChnfg[n].rect_crop.top,
			pstChnfg[n].rect_crop.width, pstChnfg[n].rect_crop.height,
			pstChnfg[n].window.rect_in.left, pstChnfg[n].window.rect_in.top,
			pstChnfg[n].window.rect_in.width, pstChnfg[n].window.rect_in.height,
			pstChnfg[n].window.rect_out.left, pstChnfg[n].window.rect_out.top,
			pstChnfg[n].window.rect_out.width, pstChnfg[n].window.rect_out.height,
			pstChnfg[n].window.top_width, pstChnfg[n].window.bottom_width,
			pstChnfg[n].window.left_width, pstChnfg[n].window.right_width,
			pstChnfg[n].window.flip);
		n++;
	}
	mutex_unlock(&pstLayerCtx->layer_lock);

	vb = (struct vb_s *)blk_next;
	base_get_frame_info(pstLayerCtx->stLayerAttr.enPixFormat
			   , stOutSize
			   , &vb->buf
			   , vb_handle2phys_addr(blk_next)
			   , DEFAULT_ALIGN);

	pstDstcfg->bytesperline[0] = vb->buf.stride[0];
	pstDstcfg->bytesperline[1] = vb->buf.stride[1];
	pstDstcfg->addr[0] = vb->buf.phy_addr[0];
	pstDstcfg->addr[1] = vb->buf.phy_addr[1];
	pstDstcfg->addr[2] = vb->buf.phy_addr[2];
	pstDstcfg->pixelformat = vb->buf.enPixelFormat;
	pstDstcfg->color[0] = 0;
	pstDstcfg->color[1] = 0;
	pstDstcfg->color[2] = 0;
	pstDstcfg->dst_size.width = stOutSize.u32Width;
	pstDstcfg->dst_size.height = stOutSize.u32Height;

	CVI_TRACE_VO(CVI_DBG_INFO, "Dst layer(%d) pixelformat(%d) bytesperline(%d %d) addr(%llx, %llx, %llx) "
		"rect_dst_out(%d %d) ", pstLayerCtx->VoLayer, pstDstcfg->pixelformat,
		pstDstcfg->bytesperline[0], pstDstcfg->bytesperline[1],
		pstDstcfg->addr[0], pstDstcfg->addr[1], pstDstcfg->addr[2],
		pstDstcfg->dst_size.width, pstDstcfg->dst_size.height);

	init_waitqueue_head(&stStitchData.wait);
	stStitchData.u8Flag = 0;

	stStitchCfg.num = u32ChnNum;
	stStitchCfg.chn_cfg = pstChnfg;
	stStitchCfg.data = (void *)&stStitchData;
	stStitchCfg.pfnJobCB = vo_stitch_wakeup;

	ret = _vo_stitch_call_vpss(&stStitchCfg);
	if (ret) {
		CVI_TRACE_VO(CVI_DBG_ERR, "_vo_stitch_call_vpss fail.\n");
		goto err1;
	}

	ret = wait_event_timeout(stStitchData.wait, stStitchData.u8Flag, timeout);
	if (ret < 0) {
		CVI_TRACE_VO(CVI_DBG_ERR, "-ERESTARTSYS!.\n");
	} else if (ret == 0) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoDev(%d) stitch timeout.\n", VoDev);
	} else {
		VO_DEV VoDev = pstLayerCtx->s32BindDevId;

		if ((VoDev >= 0) &&
			(gVoCtx->astDevCtx[VoDev].stPubAttr.enIntfType == VO_INTF_I80)) {
			ret = _i80_transform_frame(gVoCtx->astDevCtx[VoDev].stPubAttr.sti80Cfg.fmt,
					pstLayerCtx->stLayerAttr.enPixFormat, blk_next, pstDispBuf->blk_i80);
			if (ret) {
				CVI_TRACE_VO(CVI_DBG_INFO, "i80 transform NG.\n");
				vfree(pstChnfg);
				return CVI_FAILURE;
			}
			vb = (struct vb_s *)pstDispBuf->blk_i80;
		}

		for (i = 0; i < pstDispBuf->buf.length; i++) {
			pstDispBuf->buf.planes[i].addr = vb->buf.phy_addr[i];
			pstDispBuf->buf.planes[i].bytesused = vb->buf.stride[i];
		}

		ktime_get_ts64(&time);
		u64PTS = timespec64_to_ns(&time);
		do_div(u64PTS, 1000);
		vb->buf.u64PTS = u64PTS;

		spin_lock_irqsave(&pstLayerCtx->list_lock, flags);
		list_add_tail(&pstDispBuf->list, &pstLayerCtx->list_wait);
		spin_unlock_irqrestore(&pstLayerCtx->list_lock, flags);
		pstLayerCtx->u32FrameNum++;

		CVI_TRACE_VO(CVI_DBG_INFO, "layer(%d) add buffer(0x%llx) to wait list.\n",
			pstLayerCtx->VoLayer, vb->phy_addr);

		vfree(pstChnfg);

		return CVI_SUCCESS;
	}

err1:
	vfree(pstChnfg);

err0:
	spin_lock_irqsave(&pstLayerCtx->list_lock, flags);
	list_add_tail(&pstDispBuf->list, &pstLayerCtx->list_done);
	spin_unlock_irqrestore(&pstLayerCtx->list_lock, flags);

	return CVI_FAILURE;
}

//Wbc function
void vo_wbc_rdy_buf_queue(struct cvi_vo_wbc_ctx *pstWbcCtx, struct cvi_wbc_buffer *qbuf)
{
	unsigned long flags;
	spin_lock_irqsave(&pstWbcCtx->qbuf_lock, flags);
	list_add_tail(&qbuf->list, &pstWbcCtx->qbuf_list);
	++pstWbcCtx->qbuf_num;
	spin_unlock_irqrestore(&pstWbcCtx->qbuf_lock, flags);
}

struct cvi_wbc_buffer *vo_wbc_rdy_buf_next(struct cvi_vo_wbc_ctx *pstWbcCtx)
{
	unsigned long flags;
	struct cvi_wbc_buffer *buf = NULL;

	spin_lock_irqsave(&pstWbcCtx->qbuf_lock, flags);
	if (!list_empty(&pstWbcCtx->qbuf_list))
		buf = list_first_entry(&pstWbcCtx->qbuf_list, struct cvi_wbc_buffer, list);
	spin_unlock_irqrestore(&pstWbcCtx->qbuf_lock, flags);

	return buf;
}

int vo_wbc_rdy_buf_empty(struct cvi_vo_wbc_ctx *pstWbcCtx)
{
	unsigned long flags;
	int empty = 0;

	spin_lock_irqsave(&pstWbcCtx->qbuf_lock, flags);
	empty = (pstWbcCtx->qbuf_num == 0);
	spin_unlock_irqrestore(&pstWbcCtx->qbuf_lock, flags);

	return empty;
}

void vo_wbc_rdy_buf_pop(struct cvi_vo_wbc_ctx *pstWbcCtx)
{
	unsigned long flags;

	spin_lock_irqsave(&pstWbcCtx->qbuf_lock, flags);
	pstWbcCtx->qbuf_num--;
	spin_unlock_irqrestore(&pstWbcCtx->qbuf_lock, flags);
}

void vo_wbc_rdy_buf_remove(struct cvi_vo_wbc_ctx *pstWbcCtx)
{
	unsigned long flags;
	struct cvi_wbc_buffer *buf = NULL;

	spin_lock_irqsave(&pstWbcCtx->qbuf_lock, flags);
	if (!list_empty(&pstWbcCtx->qbuf_list)) {
		buf = list_first_entry(&pstWbcCtx->qbuf_list, struct cvi_wbc_buffer, list);
		list_del_init(&buf->list);
		kfree(buf);
	}
	spin_unlock_irqrestore(&pstWbcCtx->qbuf_lock, flags);
}

void vo_wbc_dqbuf_list(struct cvi_vo_wbc_ctx *pstWbcCtx)
{
	unsigned long flags;
	struct cvi_wbc_buffer *buf;

	buf = kzalloc(sizeof(struct cvi_wbc_buffer), GFP_ATOMIC);
	if (buf == NULL) {
		CVI_TRACE_VO(CVI_DBG_ERR, "DQbuf kmalloc size(%zu) fail\n", sizeof(struct cvi_wbc_buffer));
		return;
	}

	spin_lock_irqsave(&pstWbcCtx->dqbuf_lock, flags);
	list_add_tail(&buf->list, &pstWbcCtx->dqbuf_list);
	spin_unlock_irqrestore(&pstWbcCtx->dqbuf_lock, flags);
}

int vo_wbc_dqbuf(struct cvi_vo_wbc_ctx *pstWbcCtx)
{
	unsigned long flags;
	struct cvi_wbc_buffer *buf = NULL;
	int ret = -1;

	spin_lock_irqsave(&pstWbcCtx->dqbuf_lock, flags);
	if (!list_empty(&pstWbcCtx->dqbuf_list)) {
		buf = list_first_entry(&pstWbcCtx->dqbuf_list, struct cvi_wbc_buffer, list);
		list_del_init(&buf->list);
		kfree(buf);
		ret = 0;
	}
	spin_unlock_irqrestore(&pstWbcCtx->dqbuf_lock, flags);

	return ret;
}

static void vo_wbc_submit(struct cvi_vo_wbc_ctx *pstWbcCtx)
{
	struct cvi_wbc_buffer *wbc_qbuf = NULL;
	VO_DEV VoDev;
	PIXEL_FORMAT_E enPixFormat;
	struct disp_odma_cfg *odma_cfg;
	struct vo_fmt *fmt;
	struct disp_cfg *disp_cfg;

	vo_wbc_rdy_buf_pop(pstWbcCtx);
	wbc_qbuf = vo_wbc_rdy_buf_next(pstWbcCtx);
	if (wbc_qbuf == NULL) {
		CVI_TRACE_VO(CVI_DBG_ERR, "vo_wbc_rdy_buf_next empty");
		return;
	}

	// CVI_TRACE_VO(CVI_DBG_INFO, "update wbc outbuf: 0x%llx\n",
	// 		b->buf.addr[0]);

	// submit to odma hw
	VoDev = pstWbcCtx->stWbcSrc.u32SrcId;
	odma_cfg = disp_odma_get_cfg(VoDev);
	enPixFormat = pstWbcCtx->stWbcAttr.enPixFormat;
	fmt = vo_sdk_get_format(enPixFormat);

	odma_cfg->fmt = fmt->fmt;
	odma_cfg->burst = 1;
	odma_cfg->mem.addr0 = wbc_qbuf->buf.addr[0];
	odma_cfg->mem.addr1 = wbc_qbuf->buf.addr[1];
	odma_cfg->mem.addr2 = wbc_qbuf->buf.addr[2];
	odma_cfg->mem.pitch_y = wbc_qbuf->buf.pitch_y;
	odma_cfg->mem.pitch_c = wbc_qbuf->buf.pitch_c;
	odma_cfg->mem.start_x = 0;
	odma_cfg->mem.start_y = 0;
	odma_cfg->mem.width = wbc_qbuf->buf.width;
	odma_cfg->mem.height = wbc_qbuf->buf.height;

	disp_cfg = disp_get_cfg(VoDev);

	if ((disp_cfg[VoDev].out_csc >= DISP_CSC_601_LIMIT_RGB2YUV)
	&& (disp_cfg[VoDev].out_csc <= DISP_CSC_709_FULL_RGB2YUV)) {
		if (IS_YUV_FMT(odma_cfg->fmt)) {
			odma_cfg->csc = DISP_CSC_NONE;
		} else {
			odma_cfg->csc = disp_cfg[VoDev].out_csc - 4;
		}
	} else {
		if (IS_YUV_FMT(odma_cfg->fmt)) {
			//Do we need to open it up to customers
			odma_cfg->csc = DISP_CSC_601_FULL_RGB2YUV;
		} else {
			odma_cfg->csc = DISP_CSC_NONE;
		}
	}

	disp_odma_set_cfg(VoDev, odma_cfg);
}

static s32 wbc_process(struct cvi_vo_wbc_ctx *pstWbcCtx)
{
	s32 ret = CVI_FAILURE;
	VO_DEV VoDev = pstWbcCtx->stWbcSrc.u32SrcId;
	MMF_CHN_S chn = {.enModId = CVI_ID_VO, .s32DevId = VoDev, .s32ChnId = 0};
	VB_BLK blk;
	struct vb_s *vb;
	struct timespec64 time;
	u64 u64PTS;

	ret = vo_wbc_dqbuf(pstWbcCtx);
	if (ret == CVI_FAILURE) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoDev(%d) vo_wbc_dqbuf failed\n", VoDev);
		return ret;
	}

	// prev frame odma done
	ret = vb_dqbuf(chn, &pstWbcCtx->wbc_jobs, &blk);
	if (ret != 0) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoDev(%d) wbc vb_dqbuf failed\n", VoDev);
		return ret;
	}

	vb = (struct vb_s *)blk;
	ktime_get_ts64(&time);
	u64PTS = timespec64_to_ns(&time);
	do_div(u64PTS, 1000);
	vb->buf.dev_num = VoDev;
	vb->buf.u64PTS = u64PTS;
	vb->buf.frm_num = pstWbcCtx->u32DoneCnt;

	pstWbcCtx->u32DoneCnt++;
	pstWbcCtx->u32FrameNum++;
	// push to vpss/venc/vo
	vb_done_handler(chn, CHN_TYPE_OUT, &pstWbcCtx->wbc_jobs, blk);

	// get another vb for next frame
	ret = vo_wbc_qbuf(pstWbcCtx);
	if (ret != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "vo_wbc_qbuf error (%d)", ret);
		return ret;
	}

	return CVI_SUCCESS;
}

static s32 disp_event_handler(void *arg)
{
	struct cvi_vo_layer_ctx *pstLayerCtx = (struct cvi_vo_layer_ctx *)arg;
	unsigned long timeout = msecs_to_jiffies(WAIT_TIMEOUT_MS);
	s32 ret;

	while (!kthread_should_stop()) {
		ret = wait_event_interruptible_timeout(pstLayerCtx->wq,
			pstLayerCtx->event || kthread_should_stop(), timeout);

		/* -%ERESTARTSYS */
		if (ret < 0 || kthread_should_stop())
			break;

		/* timeout */
		if (!ret)
			continue;

		//CVI_TRACE_VO(CVI_DBG_INFO, "[%d] thread run.\n", pstLayerCtx->VoLayer);
		pstLayerCtx->event = 0;
		if (pstLayerCtx->bLayerUpdate) {
			layer_process(pstLayerCtx);
		}
	}

	return CVI_SUCCESS;
}

static s32 wbc_event_handler(void *arg)
{
	struct cvi_vo_wbc_ctx *pstWbcCtx = (struct cvi_vo_wbc_ctx *)arg;
	unsigned long timeout = msecs_to_jiffies(WAIT_TIMEOUT_MS);
	s32 ret;


	while (!kthread_should_stop()) {
		ret = wait_event_interruptible_timeout(pstWbcCtx->wq,
			pstWbcCtx->event || kthread_should_stop(), timeout);

		/* -%ERESTARTSYS */
		if (ret < 0 || kthread_should_stop())
			break;

		/* timeout */
		if (!ret)
			continue;

		pstWbcCtx->event = 0;
		wbc_process(pstWbcCtx);
	}

	return CVI_SUCCESS;
}

s32 vo_create_thread(VO_LAYER VoLayer)
{
	struct sched_param param;
	CVI_CHAR aszName[32];
	struct cvi_vo_layer_ctx *pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];

	snprintf(aszName, sizeof(aszName), "cvitask_vo_layer%d", VoLayer);

	pstLayerCtx->thread = kthread_create(disp_event_handler, (void *)pstLayerCtx, aszName);

	if (IS_ERR(pstLayerCtx->thread)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "Unable to create %s.\n", aszName);
		return CVI_FAILURE;
	}

	param.sched_priority = MAX_USER_RT_PRIO - 10;
	sched_setscheduler(pstLayerCtx->thread, SCHED_FIFO, &param);
	wake_up_process(pstLayerCtx->thread);

	CVI_TRACE_VO(CVI_DBG_INFO, "[VoLayer%d]create thread.\n", VoLayer);

	return CVI_SUCCESS;
}

s32 vo_destroy_thread(VO_LAYER VoLayer)
{
	struct cvi_vo_layer_ctx *pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];

	if (kthread_stop(pstLayerCtx->thread)){
		CVI_TRACE_VO(CVI_DBG_ERR, "fail to stop disp thread.\n");
	}

	pstLayerCtx->thread = NULL;
	disp_enable_window_bgcolor(pstLayerCtx->s32BindDevId, true);
	CVI_TRACE_VO(CVI_DBG_INFO, "[VoLayer%d]destroy thread.\n", VoLayer);

	return CVI_SUCCESS;
}

s32 vo_wbc_create_thread(VO_WBC VoWbc)
{
	struct sched_param param;
	CVI_CHAR aszName[32];
	struct cvi_vo_wbc_ctx *pstWbcCtx = &gVoCtx->astWbcCtx[VoWbc];

	snprintf(aszName, sizeof(aszName), "cvitask_vo_wbc%d", VoWbc);

	pstWbcCtx->thread = kthread_create(wbc_event_handler, (void *)pstWbcCtx, aszName);

	if (IS_ERR(pstWbcCtx->thread)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "Unable to create %s.\n", aszName);
		return CVI_FAILURE;
	}

	param.sched_priority = MAX_USER_RT_PRIO - 10;
	sched_setscheduler(pstWbcCtx->thread, SCHED_FIFO, &param);
	wake_up_process(pstWbcCtx->thread);

	CVI_TRACE_VO(CVI_DBG_INFO, "[VoWbc%d]create thread.\n", VoWbc);

	return CVI_SUCCESS;
}

s32 vo_wbc_destroy_thread(VO_WBC VoWbc)
{
	struct cvi_vo_wbc_ctx *pstWbcCtx = &gVoCtx->astWbcCtx[VoWbc];

	if (kthread_stop(pstWbcCtx->thread)){
		CVI_TRACE_VO(CVI_DBG_ERR, "fail to stop wbc thread.\n");
	}

	pstWbcCtx->thread = NULL;
	CVI_TRACE_VO(CVI_DBG_INFO, "[VoWbc%d]destroy thread.\n", VoWbc);

	return CVI_SUCCESS;
}

s32 vo_start_streaming(VO_DEV VoDev)
{
	s32 rc = CVI_SUCCESS;
	struct cvi_vo_dev *vdev = gVdev;

	disp_enable_window_bgcolor(VoDev, true);

	vdev->vo_core[VoDev].align = VIP_ALIGNMENT;
	vdev->vo_core[VoDev].frame_number = 0;

	if (vdev->vo_core[VoDev].disp_interface != CVI_VIP_DISP_INTF_I80)
		disp_tgen_enable(VoDev, true);

	atomic_set(&vdev->vo_core[VoDev].disp_streamon, 1);
	CVI_TRACE_VO(CVI_DBG_INFO, "[VoDev%d]start streaming.\n", VoDev);

	return rc;
}

s32 vo_stop_streaming(VO_DEV VoDev)
{
	s32 rc = CVI_SUCCESS;
	struct cvi_vo_dev *vdev = gVdev;

#if 0 //TODO: no use anymore, to be removed!
	// struct sclr_top_cfg *cfg = sclr_top_get_cfg();
	// cfg->disp_enable = false;
	// sclr_top_set_cfg(cfg);
#endif

	disp_enable_window_bgcolor(VoDev, true);

	if (!smooth[VoDev] && (vdev->vo_core[VoDev].disp_interface != CVI_VIP_DISP_INTF_LVDS))
		disp_tgen_enable(VoDev, false);

	atomic_set(&vdev->vo_core[VoDev].disp_streamon, 0);
	memset(&vdev->vo_core[VoDev].compose_out, 0, sizeof(vdev->vo_core[VoDev].compose_out));
	CVI_TRACE_VO(CVI_DBG_INFO, "[VoDev%d]stop streaming.\n", VoDev);
	CVI_TRACE_VO(CVI_DBG_DEBUG, "end...\n");

	return rc;
}

static long _vo_s_ctrl(struct cvi_vo_dev *vdev, struct vo_ext_control *p)
{
	u32 id = p->id;
	long rc = CVI_SUCCESS;

	switch (id) {
	case VO_IOCTL_SDK_CTRL: {
		rc = vo_sdk_ctrl(vdev, p);
	}
	break;

	case VO_IOCTL_START_STREAMING: {
		VO_DEV VoDev = p->reserved[0];

		if (VoDev >= VO_MAX_DEV_NUM) {
			rc = -EINVAL;
			CVI_TRACE_VO(CVI_DBG_ERR, "Invalid vo device(%d)!\n", VoDev);
			break;
		}
		rc = vo_start_streaming(VoDev);
		if (rc != CVI_SUCCESS) {
			CVI_TRACE_VO(CVI_DBG_ERR, "Failed to vo start streaming\n");
		}
	}
	break;

	case VO_IOCTL_STOP_STREAMING: {
		VO_DEV VoDev = p->reserved[0];

		if (VoDev >= VO_MAX_DEV_NUM) {
			rc = -EINVAL;
			CVI_TRACE_VO(CVI_DBG_ERR, "Invalid vo device(%d)!\n", VoDev);
			break;
		}
		rc = vo_stop_streaming(VoDev);
		if (rc != CVI_SUCCESS) {
			CVI_TRACE_VO(CVI_DBG_ERR, "Failed to vo stop streaming\n");
		}
	}
	break;

	case VO_IOCTL_SET_DV_TIMINGS: {
		struct vo_dv_timings dv_timings;
		struct disp_timing disp_timing;
		VO_DEV VoDev = p->reserved[0];

		if (gVoCtx->astDevCtx[VoDev].stPubAttr.enIntfType == VO_INTF_MIPI) {
			CVI_TRACE_VO(CVI_DBG_INFO, "device(%d) timing shoud be setup by mipi-tx!\n", VoDev);
			break;
		}
		if (gVoCtx->astDevCtx[VoDev].stPubAttr.enIntfType == VO_INTF_HDMI) {
			CVI_TRACE_VO(CVI_DBG_INFO, "device(%d) timing shoud be setup by hdmi-tx!\n", VoDev);
			break;
		}

		if (VoDev >= VO_MAX_DEV_NUM) {
			rc = -EINVAL;
			CVI_TRACE_VO(CVI_DBG_ERR, "Invalid vo device(%d)!\n", VoDev);
			break;
		}
		if (copy_from_user(&dv_timings, (void *)p->ptr, sizeof(struct vo_dv_timings))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "Set DV timing copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		vdev->vo_core[VoDev].dv_timings = dv_timings;
		vdev->vo_core[VoDev].sink_rect.width = dv_timings.bt.width;
		vdev->vo_core[VoDev].sink_rect.height = dv_timings.bt.height;
		vdev->vo_core[VoDev].compose_out = vdev->vo_core[VoDev].sink_rect;
		CVI_TRACE_VO(CVI_DBG_DEBUG, "timing %d-%d\n", dv_timings.bt.width, dv_timings.bt.height);

		vo_fill_disp_timing(&disp_timing, &dv_timings.bt);
		disp_set_timing(VoDev, &disp_timing);
	}
	break;

	case VO_IOCTL_SEL_TGT_COMPOSE: {
		struct vo_rect area;
		VO_LAYER VoLayer = p->reserved[0];
		VO_DEV VoDev = VoLayer;

		if (VoDev >= VO_MAX_DEV_NUM) {
			rc = -EINVAL;
			CVI_TRACE_VO(CVI_DBG_ERR, "Invalid vo device(%d)!\n", VoDev);
			break;
		}
		if (copy_from_user(&area, p->ptr, sizeof(area)) != 0) {
			CVI_TRACE_VO(CVI_DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", p->id);
			rc = -EFAULT;
			break;
		}

		if (memcmp(&vdev->vo_core[VoDev].compose_out, &area, sizeof(area))) {
			struct disp_rect rect;

			rect.x = area.left;
			rect.y = area.top;
			rect.w = area.width;
			rect.h = area.height;

			CVI_TRACE_VO(CVI_DBG_DEBUG, "Compose Area (%d,%d,%d,%d)\n", rect.x, rect.y, rect.w, rect.h);
			rc = disp_set_rect(VoDev, rect);
		}
	}
	break;

	case VO_IOCTL_SEL_TGT_CROP: {
		struct disp_cfg *cfg;
		struct vo_rect area;
		VO_LAYER VoLayer = p->reserved[0];
		VO_DEV VoDev = VoLayer;

		if (VoDev >= VO_MAX_DEV_NUM) {
			rc = -EINVAL;
			CVI_TRACE_VO(CVI_DBG_ERR, "Invalid vo device(%d)!\n", VoDev);
			break;
		}
		if (copy_from_user(&area, (void __user *)p->ptr, sizeof(struct vo_rect))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", p->id);
			rc = -EFAULT;
			break;
		}

		cfg = disp_get_cfg(VoDev);
		cfg->mem.start_x = area.left;
		cfg->mem.start_y = area.top;
		cfg->mem.width	 = area.width;
		cfg->mem.height  = area.height;

		CVI_TRACE_VO(CVI_DBG_INFO, "Crop Area (%d, %d, %d, %d)\n", cfg->mem.start_x,
				cfg->mem.start_y, cfg->mem.width, cfg->mem.height);
		disp_set_mem(VoDev, &cfg->mem);
		vdev->vo_core[VoDev].crop_rect = area;
	}
	break;

	case VO_IOCTL_SET_ALIGN: {
		VO_LAYER VoLayer = p->reserved[0];
		VO_DEV VoDev = VoLayer;

		if (VoDev >= VO_MAX_DEV_NUM) {
			rc = -EINVAL;
			CVI_TRACE_VO(CVI_DBG_ERR, "Invalid vo device(%d)!\n", VoDev);
			break;
		}
		if (p->value >= VIP_ALIGNMENT) {
			vdev->vo_core[VoDev].align = p->value = VIP_ALIGNMENT;
			CVI_TRACE_VO(CVI_DBG_INFO, "Set Align(%d)\n", vdev->vo_core[VoDev].align);
		}
	}
	break;

	case VO_IOCTL_SET_RGN: {
		struct cvi_rgn_cfg cfg;
		VO_LAYER VoLayer = p->reserved[0];

		if (copy_from_user(&cfg, p->ptr, sizeof(struct cvi_rgn_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", p->id);
			rc = -EFAULT;
			break;
		}
		rc = vo_cb_set_rgn_cfg(VoLayer, &cfg);
	}
	break;

	// TODO: no use, can be removed!!!
#if 0
	case VO_IOCTL_I80_SW_MODE: {
		i80_sw_mode(p->value);

		rc = CVI_SUCCESS;
	}
	break;

	case VO_IOCTL_I80_CMD: {
		i80_packet(p->value);

		rc = CVI_SUCCESS;
	}
	break;
#endif
	case VO_IOCTL_SET_CUSTOM_CSC: {
		struct disp_csc_matrix cfg;
		VO_LAYER VoLayer = p->reserved[0];
		VO_DEV VoDev = VoLayer;

		if (VoDev >= VO_MAX_DEV_NUM) {
			rc = -EINVAL;
			CVI_TRACE_VO(CVI_DBG_ERR, "Invalid vo device(%d)!\n", VoDev);
			break;
		}
		if (copy_from_user(&cfg, p->ptr, sizeof(struct disp_csc_matrix))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", p->id);
			rc = -EFAULT;
			break;
		}
		disp_set_csc(VoDev, &cfg);
	}
	break;

	case VO_IOCTL_SET_CLK: {
		VO_DEV VoDev = p->reserved[0];

		if (VoDev >= VO_MAX_DEV_NUM) {
			rc = -EINVAL;
			CVI_TRACE_VO(CVI_DBG_ERR, "Invalid vo device(%d)!\n", VoDev);
			break;
		}
		if (p->value < 8000) {
			CVI_TRACE_VO(CVI_DBG_ERR, "V4L2_CID_DV_VIP_DISP_SET_CLK clk(%d) less than 8000 kHz.\n",
				p->value);
			rc = -EINVAL;
			break;
		}
		dphy_dsi_set_pll(VoDev, p->value, 4, 24);
	}
	break;

	case VO_IOCTL_OUT_CSC: {
		VO_LAYER VoLayer = p->reserved[0];
		VO_DEV VoDev = VoLayer;

		if (VoDev >= VO_MAX_DEV_NUM) {
			rc = -EINVAL;
			CVI_TRACE_VO(CVI_DBG_ERR, "Invalid vo device(%d)!\n", VoDev);
			break;
		}
		if (p->value >= DISP_CSC_601_LIMIT_YUV2RGB &&
			p->value <= DISP_CSC_709_FULL_YUV2RGB) {
			CVI_TRACE_VO(CVI_DBG_ERR, "invalid disp-out-csc(%d)\n", p->value);
			rc = -EINVAL;
			break;
		}
		disp_set_out_csc(VoDev, p->value);
	}
	break;

	case VO_IOCTL_PATTERN: {
		VO_LAYER VoDev = p->reserved[0];

		if (VoDev >= VO_MAX_DEV_NUM) {
			rc = -EINVAL;
			CVI_TRACE_VO(CVI_DBG_ERR, "Invalid vo device(%d)!\n", VoDev);
			break;
		}
		if (p->value >= CVI_VIP_PAT_MAX) {
			CVI_TRACE_VO(CVI_DBG_ERR, "invalid disp-pattern(%d)\n", p->value);
			rc = -EFAULT;
			break;
		}
		disp_set_pattern(VoDev, patterns[p->value].type, patterns[p->value].color,
						patterns[p->value].rgb);
		if (!disp_check_tgen_enable(VoDev))
				disp_tgen_enable(VoDev, true);
	}
	break;

	case VO_IOCTL_FRAME_BGCOLOR: {
		u16 u16_rgb[3];
		VO_LAYER VoLayer = p->reserved[0];
		VO_DEV VoDev = VoLayer;

		if (VoDev >= VO_MAX_DEV_NUM) {
			rc = -EINVAL;
			CVI_TRACE_VO(CVI_DBG_ERR, "Invalid vo device(%d)!\n", VoDev);
			break;
		}

		if (copy_from_user(u16_rgb, p->ptr, sizeof(u16_rgb))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", p->id);
			rc = -EFAULT;
			break;
		}

		CVI_TRACE_VO(CVI_DBG_INFO, "Set Frame BG color (R,G,B) = (0x%x, 0x%x, 0x%x)\n",
				u16_rgb[1], u16_rgb[2], u16_rgb[3]);

		disp_set_frame_bgcolor(VoDev, u16_rgb[0], u16_rgb[1], u16_rgb[2]);
	}
	break;

	case VO_IOCTL_WINDOW_BGCOLOR: {
		u16 u16_rgb[3];
		VO_LAYER VoLayer = p->reserved[0];
		VO_DEV VoDev = VoLayer;

		if (VoDev >= VO_MAX_DEV_NUM) {
			rc = -EINVAL;
			CVI_TRACE_VO(CVI_DBG_ERR, "Invalid vo device(%d)!\n", VoDev);
			break;
		}

		if (copy_from_user(u16_rgb, p->ptr, sizeof(u16_rgb))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", p->id);
			rc = -EFAULT;
			break;
		}

		CVI_TRACE_VO(CVI_DBG_INFO, "Set window BG color (R,G,B) = (0x%x, 0x%x, 0x%x)\n",
				u16_rgb[1], u16_rgb[2], u16_rgb[3]);

		disp_set_window_bgcolor(VoDev, u16_rgb[0], u16_rgb[1], u16_rgb[2]);
	}
	break;

	case VO_IOCTL_ONLINE: {
		VO_DEV VoDev = p->reserved[0];

		if (VoDev >= VO_MAX_DEV_NUM) {
			rc = -EINVAL;
			CVI_TRACE_VO(CVI_DBG_ERR, "Invalid vo device(%d)!\n", VoDev);
			break;
		}
		if (atomic_read(&vdev->vo_core[VoDev].disp_streamon) == 1) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_IOCTL_ONLINE can't be control if streaming.\n");
			rc = -EFAULT;
			break;
		}
		vdev->vo_core[VoDev].disp_online = p->value;

		// FIXME:
		// disp_ctrl_set_disp_src(VoDev, vdev->vo_core[VoDev].disp_online);
	}
	break;

	case VO_IOCTL_INTF: {
		struct cvi_disp_intf_cfg cfg;
		VO_DEV VoDev = p->reserved[0];

		if (VoDev >= VO_MAX_DEV_NUM) {
			rc = -EINVAL;
			CVI_TRACE_VO(CVI_DBG_ERR, "Invalid vo device(%d)!\n", VoDev);
			break;
		}
		if (copy_from_user(&cfg, p->ptr, sizeof(struct cvi_disp_intf_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "ioctl-%#x, copy_from_user failed.\n", p->id);
			rc = -EFAULT;
			break;
		}

		if (smooth[VoDev]) {
			CVI_TRACE_VO(CVI_DBG_DEBUG, "VO_IOCTL_INTF won't apply if smooth.\n");
			disp_reg_force_up(VoDev);
			vdev->vo_core[VoDev].disp_interface = cfg.intf_type;
			rc = CVI_SUCCESS;
			break;
		}

		if (atomic_read(&vdev->vo_core[VoDev].disp_streamon) == 1) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_IOCTL_INTF can't be control if streaming.\n");
			rc = -EAGAIN;
			break;
		}

		if (cfg.intf_type == CVI_VIP_DISP_INTF_DSI) {
			CVI_TRACE_VO(CVI_DBG_INFO, "MIPI use mipi_tx to control.\n");
		} else if (cfg.intf_type == CVI_VIP_DISP_INTF_HDMI) {
			CVI_TRACE_VO(CVI_DBG_INFO, "HDMI use hdmi_tx to control.\n");
		} else if (cfg.intf_type == CVI_VIP_DISP_INTF_LVDS) {
			s32 i = 0;
			union disp_lvdstx lvds_cfg;
			bool data_en[LANE_MAX_NUM] = {false, false, false, false, false};

			for (i = 0; i < LANE_MAX_NUM; i++) {
				if ((cfg.lvds_cfg.lane_id[i] < 0) ||
					(cfg.lvds_cfg.lane_id[i] >= LANE_MAX_NUM)) {
					dphy_dsi_set_lane(VoDev, i, DSI_LANE_MAX, false, false);
					continue;
				}
				dphy_dsi_set_lane(VoDev, i, cfg.lvds_cfg.lane_id[i],
						  cfg.lvds_cfg.lane_pn_swap[i], false);
				if (cfg.lvds_cfg.lane_id[i] != MIPI_TX_LANE_CLK) {
					data_en[cfg.lvds_cfg.lane_id[i] - 1] = true;
				}
			}
			dphy_dsi_lane_en(VoDev, true, data_en, false);
			disp_set_intf(VoDev, DISP_VO_INTF_LVDS);

			if (cfg.lvds_cfg.pixelclock == 0) {
				CVI_TRACE_VO(CVI_DBG_ERR, "lvds pixelclock 0 invalid\n");
				rc = -EINVAL;
				break;
			}
			lvds_cfg.b.out_bit = cfg.lvds_cfg.out_bits;
			lvds_cfg.b.vesa_mode = cfg.lvds_cfg.mode;
			if (cfg.lvds_cfg.chn_num == 1)
				lvds_cfg.b.dual_ch = 0;
			else if (cfg.lvds_cfg.chn_num == 2)
				lvds_cfg.b.dual_ch = 1;
			else {
				lvds_cfg.b.dual_ch = 0;
				CVI_TRACE_VO(CVI_DBG_ERR, "invalid lvds chn_num(%d). Use 1 instead."
					, cfg.lvds_cfg.chn_num);
			}
			lvds_cfg.b.vs_out_en = cfg.lvds_cfg.vs_out_en;
			lvds_cfg.b.hs_out_en = cfg.lvds_cfg.hs_out_en;
			lvds_cfg.b.hs_blk_en = cfg.lvds_cfg.hs_blk_en;
			lvds_cfg.b.ml_swap = cfg.lvds_cfg.msb_lsb_data_swap;
			lvds_cfg.b.ctrl_rev = cfg.lvds_cfg.serial_msb_first;
			lvds_cfg.b.oe_swap = cfg.lvds_cfg.even_odd_link_swap;
			lvds_cfg.b.en = cfg.lvds_cfg.enable;
			dphy_lvds_set_pll(VoDev, cfg.lvds_cfg.pixelclock, cfg.lvds_cfg.chn_num);
			disp_lvdstx_set(VoDev, lvds_cfg);
		} else if (cfg.intf_type == CVI_VIP_DISP_INTF_I80) {
			union disp_bt_enc enc;
			union disp_bt_sync_code sync;

			// TODO: to be removed, should be done in uboot or by user
			// _disp_sel_pinmux(cfg.intf_type, &cfg.bt_cfg);
			disp_set_intf(VoDev, DISP_VO_INTF_I80);
			enc.raw = 0;
			enc.b.fmt_sel = 2;
			enc.b.clk_inv = 1;
			sync.raw = 0;
			sync.b.sav_vld = 0x80;
			sync.b.sav_blk = 0xab;
			sync.b.eav_vld = 0x9d;
			sync.b.eav_blk = 0xb6;
			disp_bt_set(VoDev, enc, sync);
		} else if (cfg.intf_type == CVI_VIP_DISP_INTF_BT) {
			union disp_bt_enc enc;
			union disp_bt_sync_code sync;

			if (cfg.bt_cfg.mode == BT_MODE_1120)
				disp_set_intf(VoDev, DISP_VO_INTF_BT1120);
			else if (cfg.bt_cfg.mode == BT_MODE_656)
				disp_set_intf(VoDev, DISP_VO_INTF_BT656);
			else if (cfg.bt_cfg.mode == BT_MODE_601)
				disp_set_intf(VoDev, DISP_VO_INTF_BT601);
			else {
				CVI_TRACE_VO(CVI_DBG_ERR, "invalid bt-mode(%d)\n", cfg.bt_cfg.mode);
				break;
			}

			//set csc value
			disp_set_out_csc(VoDev, DISP_CSC_601_FULL_RGB2YUV);
			// TODO: to be removed, should be done in uboot or by user
			// _disp_sel_pinmux(cfg.intf_type, &cfg.bt_cfg);

			enc.raw = 0;
			enc.b.fmt_sel = cfg.bt_cfg.mode;
			sync.b.sav_vld = 0x80;
			sync.b.sav_blk = 0xab;
			sync.b.eav_vld = 0x9d;
			sync.b.eav_blk = 0xb6;
			disp_bt_set(VoDev, enc, sync);
		} else {
			CVI_TRACE_VO(CVI_DBG_ERR, "invalid disp-intf(%d)\n", cfg.intf_type);
			rc = EINVAL;
			break;
		}
		disp_reg_force_up(VoDev);

		vdev->vo_core[VoDev].disp_interface = cfg.intf_type;
	}
	break;

	case VO_IOCTL_ENABLE_WIN_BGCOLOR: {
		VO_LAYER VoLayer = p->reserved[0];
		VO_DEV VoDev = VoLayer;

		if (VoDev >= VO_MAX_DEV_NUM) {
			rc = -EINVAL;
			CVI_TRACE_VO(CVI_DBG_ERR, "Invalid vo device(%d)!\n", VoDev);
			break;
		}
		vdev->vo_core[VoDev].bgcolor_enable = p->value;
		disp_enable_window_bgcolor(VoDev, p->value);
	}
	break;

	case VO_IOCTL_GAMMA_LUT_UPDATE: {
		s32 i = 0;
		struct disp_gamma_attr gamma_attr_sclr;
		VO_GAMMA_INFO_S gamma_attr;
		VO_DEV VoDev = p->reserved[0];

		if (VoDev >= VO_MAX_DEV_NUM) {
			rc = -EINVAL;
			CVI_TRACE_VO(CVI_DBG_ERR, "Invalid vo device(%d)!\n", VoDev);
			break;
		}
		if (copy_from_user(&gamma_attr, (void *)p->ptr, sizeof(gamma_attr))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "gamma lut update copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		gamma_attr_sclr.enable = gamma_attr.enable;
		gamma_attr_sclr.pre_osd = gamma_attr.osd_apply;

		for (i = 0; i < DISP_GAMMA_NODE; ++i) {
			gamma_attr_sclr.table[i] = gamma_attr.value[i];
		}

		disp_gamma_ctrl(VoDev, gamma_attr_sclr.enable, gamma_attr_sclr.pre_osd);
		disp_gamma_lut_update(VoDev, gamma_attr_sclr.table, gamma_attr_sclr.table, gamma_attr_sclr.table);
	}
	break;

	default:
		break;
	}

	return rc;
}

static long _vo_g_ctrl(struct cvi_vo_dev *vdev, struct vo_ext_control *p)
{
	u32 id = p->id;
	long rc = CVI_SUCCESS;

	switch (id) {
	case VO_IOCTL_GET_DV_TIMINGS: {
		VO_DEV VoDev = p->reserved[0];

		if (copy_to_user(p->ptr, &vdev->vo_core[VoDev].dv_timings, sizeof(struct vo_dv_timings))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_IOCTL_GET_DV_TIMINGS copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_IOCTL_GET_VLAYER_SIZE: {
		VO_LAYER VoLayer = p->reserved[0];
		VO_DEV VoDev = VoLayer;
		struct disp_timing *timing = disp_get_timing(VoDev);
		struct dsize {
			u32 width;
			u32 height;
		} vsize;

		vsize.width = timing->hfde_end - timing->hfde_start + 1;
		vsize.height = timing->vfde_end - timing->vfde_start + 1;

		if (copy_to_user(p->ptr, &vsize, sizeof(struct dsize))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_IOCTL_GET_VLAYER_SIZE copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_IOCTL_GET_INTF_TYPE: {
		VO_INTF_TYPE_E inft;
		VO_DEV VoDev = p->reserved[0];

		inft = gVoCtx->astDevCtx[VoDev].stPubAttr.enIntfType;
		if (copy_to_user(p->ptr, &inft, sizeof(inft))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_IOCTL_GET_INTF_TYPE copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_IOCTL_GET_PANEL_STATUS: {
		s32 is_init = 0;
		struct vo_panel_status_cfg cfg;
		VO_DEV VoDev;

		if (copy_from_user(&cfg, (void *)p->ptr, sizeof(struct vo_panel_status_cfg))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_IOCTL_GET_PANEL_STATUS copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}
		VoDev = cfg.VoLayer;
		if (disp_mux_get(VoDev) == DISP_VO_SEL_I80) {
			is_init = disp_check_i80_enable(VoDev);
		} else {
			is_init = disp_check_tgen_enable(VoDev);
		}

		if (copy_to_user(p->ptr, &is_init, sizeof(is_init))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_IOCTL_GET_PANEL_STATUS copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_IOCTL_GAMMA_LUT_READ: {
		s32 i = 0;
		VO_GAMMA_INFO_S gamma_attr;
		struct disp_gamma_attr gamma_attr_sclr;
		VO_DEV VoDev = p->reserved[0];

		disp_gamma_lut_read(VoDev, &gamma_attr_sclr);

		gamma_attr.enable = gamma_attr_sclr.enable;
		gamma_attr.osd_apply = gamma_attr_sclr.pre_osd;
		gamma_attr.s32VoDev = VoDev;

		for (i = 0; i < DISP_GAMMA_NODE; ++i) {
			gamma_attr.value[i] = gamma_attr_sclr.table[i];
		}

		if (copy_to_user((void *)p->ptr, &gamma_attr, sizeof(VO_GAMMA_INFO_S))) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_IOCTL_GAMMA_LUT_READ copy_to_user failed.\n");
			rc = -EFAULT;
		}
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
		return -EFAULT;

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
		return -EFAULT;

	return ret;

}


s32 vo_open(struct inode *inode, struct file *file)
{
	s32 ret = 0;
	VO_DEV VoDev;
	struct cvi_vo_dev *vdev;

	vdev = container_of(inode->i_cdev, struct cvi_vo_dev, cdev);
	file->private_data = vdev;
	if (!atomic_read(&dev_open_cnt)) {
		for (VoDev = 0; VoDev < VO_MAX_DEV_NUM; ++VoDev) {
			atomic_set(&vdev->vo_core[VoDev].disp_streamon, 0);

			disp_reg_shadow_sel(VoDev, false);
			if (!smooth[VoDev])
				disp_set_cfg(VoDev, disp_get_cfg(VoDev));
		}
	}

	atomic_inc(&dev_open_cnt);

	return ret;
}

void _vo_sdk_release(struct cvi_vo_dev *vdev)
{
	int i, j;

	for (i = 0; i < VO_MAX_LAYER_NUM; i++) {
		if (gVoCtx->astLayerCtx[i].is_layer_enable) {
			for (j = 0; j < VO_MAX_CHN_NUM; j++) {
				if (gVoCtx->astLayerCtx[i].astChnCtx[j].is_chn_enable)
					vo_disable_chn(i, j);
			}
			vo_disablevideolayer(i);
		}
	}
	for (i = 0; i < VO_MAX_DEV_NUM; i++) {
		if (gVoCtx->astDevCtx[i].is_dev_enable)
			vo_disable(i);
	}
}

s32 vo_release(struct inode *inode, struct file *file)
{
	s32 ret = 0;

	atomic_dec(&dev_open_cnt);
	if (!atomic_read(&dev_open_cnt)) {
		struct cvi_vo_dev *vdev;

		vdev = container_of(inode->i_cdev, struct cvi_vo_dev, cdev);
		_vo_sdk_release(vdev);
	}
	return ret;
}

u32 vo_poll(struct file *file, struct poll_table_struct *wait)
{
	return CVI_SUCCESS;
}

s32 vo_cb(void *dev, enum ENUM_MODULES_ID caller, u32 cmd, void *arg)
{
	struct cvi_vo_dev *vdev = (struct cvi_vo_dev *)dev;
	s32 rc = -1;

	UNUSED(vdev);
	switch (cmd) {
	case VO_CB_GET_RGN_HDLS:
	{
		struct _rgn_hdls_cb_param *attr = (struct _rgn_hdls_cb_param *)arg;
		VO_LAYER VoLayer = attr->stChn.s32DevId;
		RGN_HANDLE *pstHandle = attr->hdls;
		RGN_TYPE_E enType = attr->enType;

		CVI_TRACE_VO(CVI_DBG_INFO, "VO_CB_GET_RGN_HDLS\n");
		rc = vo_cb_get_rgn_hdls(VoLayer, enType, pstHandle);
		if (rc != CVI_SUCCESS) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_CB_GET_RGN_HDLS failed.\n");
		}
		break;
	}

	case VO_CB_SET_RGN_HDLS:
	{
		struct _rgn_hdls_cb_param *attr = (struct _rgn_hdls_cb_param *)arg;
		VO_LAYER VoLayer = attr->stChn.s32DevId;
		RGN_HANDLE *pstHandle = attr->hdls;
		RGN_TYPE_E enType = attr->enType;

		CVI_TRACE_VO(CVI_DBG_INFO, "VO_CB_SET_RGN_HDLS\n");
		rc = vo_cb_set_rgn_hdls(VoLayer, enType, pstHandle);
		if (rc != CVI_SUCCESS) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_CB_SET_RGN_HDLS failed.\n");
		}
		break;
	}

	case VO_CB_SET_RGN_CFG:
	{
		struct _rgn_cfg_cb_param *attr = (struct _rgn_cfg_cb_param *)arg;
		struct cvi_rgn_cfg *pstRgnCfg = &attr->rgn_cfg;
		VO_LAYER VoLayer = attr->stChn.s32DevId;

		CVI_TRACE_VO(CVI_DBG_INFO, "VO_CB_SET_RGN_CFG\n");
		if (vo_cb_set_rgn_cfg(VoLayer, pstRgnCfg)) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_CB_SET_RGN_CFG is failed.\n");
		}

		rc = CVI_SUCCESS;
		break;
	}

	case VO_CB_SET_RGN_COVEREX_CFG:
	{
		struct _rgn_coverex_cfg_cb_param *attr = (struct _rgn_coverex_cfg_cb_param *)arg;
		struct cvi_rgn_coverex_cfg *pstRgnCoverExCfg = &attr->rgn_coverex_cfg;
		VO_LAYER VoLayer = attr->stChn.s32DevId;

		CVI_TRACE_VO(CVI_DBG_INFO, "VO_CB_SET_RGN_COVER_CFG\n");
		rc = vo_cb_set_rgn_coverex_cfg(VoLayer, pstRgnCoverExCfg);
		if (rc != CVI_SUCCESS) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_CB_SET_RGN_COVER_CFG is failed.\n");
			break;
		}

		break;
	}

	case VO_CB_GET_CHN_SIZE:
	{
		struct _rgn_chn_size_cb_param *param = (struct _rgn_chn_size_cb_param *)arg;
		VO_LAYER VoLayer = param->stChn.s32DevId;

		CVI_TRACE_VO(CVI_DBG_INFO, "VO_CB_GET_CHN_SIZE\n");
		rc = vo_cb_get_chn_size(VoLayer, &param->rect);
		if (rc != CVI_SUCCESS) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO_CB_GET_CHN_SIZE failed\n");
		}
		break;
	}

	case VO_CB_GDC_OP_DONE:
	{
		struct ldc_op_done_cfg *cfg = (struct ldc_op_done_cfg *)arg;

		_vo_gdc_callback(cfg->pParam, cfg->blk);
		rc = 0;
		break;
	}

	case VO_CB_QBUF_VO_GET_CHN_ROTATION:
	{
		struct vo_get_chnrotation_cfg *cfg = (struct vo_get_chnrotation_cfg *)arg;

		rc = vo_get_chnrotation(cfg->VoLayer, cfg->VoChn, (ROTATION_E *)&cfg->enRotation);
		break;
	}

	default:
		break;
	}

	return rc;
}


u64 timespec64_to_us(struct timespec64 *pstTimeSpec)
{
	return pstTimeSpec->tv_sec * 1000000L + pstTimeSpec->tv_nsec / 1000L;
}

void ddr_retrain(VO_DEV VoDev, union disp_intr intr_status)
{
	#define DISP_FPS_CNT 10
	#define DISP_FPS_TABLE_CNT 6

	static struct timespec64 stDispFrameendTime[DISP_MAX_INST], stLastOverlapTime;
	static const u8 au8Fpstable[DISP_FPS_TABLE_CNT] = {24, 25, 30, 48, 50, 60};
	static const u16 au16MarginTable[DISP_FPS_TABLE_CNT] = {489, 460, 345, 129, 115, 57};
	static u8 u8DispFps[DISP_MAX_INST], u8dispFpsCnt[DISP_MAX_INST];
	static struct timespec64 stDispFpsTime[DISP_MAX_INST];
	static u16 u16Margin[DISP_MAX_INST];
	u64 u64DispFrameendTimeUs[DISP_MAX_INST], u64FrameGapMs[DISP_MAX_INST];
	struct timespec64 curOverlapTime;
	u8 i = 0;

	if (intr_status.b.disp_frame_end) {
		curOverlapTime = stDispFrameendTime[VoDev] = ktime_to_timespec64(ktime_get());
		u64DispFrameendTimeUs[VoDev] = timespec64_to_us(&stDispFrameendTime[VoDev]);
		u64DispFrameendTimeUs[!VoDev] = timespec64_to_us(&stDispFrameendTime[!VoDev]);

		if (u8dispFpsCnt[VoDev] < DISP_FPS_CNT) {
			if (!u8dispFpsCnt[VoDev])
				stDispFpsTime[VoDev] = stDispFrameendTime[VoDev];
			u8dispFpsCnt[VoDev]++;
		} else if (u8dispFpsCnt[VoDev] == DISP_FPS_CNT) {
			u8dispFpsCnt[VoDev]++;
			u64FrameGapMs[VoDev] = u64DispFrameendTimeUs[VoDev] / 1000L
							- timespec64_to_us(&stDispFpsTime[VoDev]) / 1000L;
			u8DispFps[VoDev] = (DISP_FPS_CNT * 10000 / u64FrameGapMs[VoDev] + 5) / 10;
			pr_info("disp%d fps:%d\n", VoDev, u8DispFps[VoDev]);
		}

		if ((u64DispFrameendTimeUs[VoDev] > u64DispFrameendTimeUs[!VoDev])) {
			if ((u16Margin[VoDev] == 0) && (u8DispFps[!VoDev] != 0)) {
				for (i = 0; i < DISP_FPS_TABLE_CNT; i++) {
					if ((u8DispFps[!VoDev] <= au8Fpstable[i])
						|| (i == DISP_FPS_TABLE_CNT - 1)) {
						u16Margin[VoDev] = au16MarginTable[i];
						pr_info("disp%d ahead disp%d, margin time(%d)\n"
							, !VoDev, VoDev, u16Margin[VoDev]);
						break;
					}
				}
			}
			if ((u64DispFrameendTimeUs[VoDev] - u64DispFrameendTimeUs[!VoDev]
				< u16Margin[VoDev]) || !disp_check_tgen_enable(!VoDev)) {
				if (ddr_need_retrain())
					trigger_8051();
				if (timespec64_to_us(&curOverlapTime)
					- timespec64_to_us(&stLastOverlapTime) > 900000L)
					pr_info("disp%d ahead disp%d, vblanking overlap. "
						"margin(%lld)us last overlap is (%lld)ms before.\n"
						, !VoDev, VoDev
						, u64DispFrameendTimeUs[VoDev] - u64DispFrameendTimeUs[!VoDev]
						, (timespec64_to_us(&curOverlapTime)
							- timespec64_to_us(&stLastOverlapTime)) / 1000);
				stLastOverlapTime = curOverlapTime;
			}
		}
	}
}
/*******************************************************
 *  Irq handlers
 ******************************************************/
static void irq_handler(struct cvi_vo_core *pCore, union disp_intr intr_status)
{
	VO_DEV VoDev = pCore->core_id;
	union disp_dbg_status status = disp_get_dbg_status(VoDev, true);
	VO_LAYER VoLayer = gVoCtx->astDevCtx[VoDev].s32BindLayerId;
	struct cvi_vo_layer_ctx *pstLayerCtx;
	struct cvi_vo_wbc_ctx *pstWbcCtx;
	unsigned long flags;
	bool wait_empty;
	int i;
	union disp_online_odma_intr_sel online_odma_mask;
	FRAME_RATE_CTRL_S wbc_frame_ctrl;

	if (intr_status.b.disp_frame_end) {
		ddr_retrain(VoDev, intr_status);
	}

	if (atomic_read(&pCore->disp_streamon) == 0)
		return;

	if (intr_status.b.disp_frame_end) {
		++pCore->frame_number;

		pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];
		if (!pstLayerCtx->is_layer_enable)
			return;

		if (status.b.bw_fail){
			pstLayerCtx->u32BwFail++;
			CVI_TRACE_VO(CVI_DBG_WARN, "VoDev(%d) disp bw failed at frame#%d\n", VoDev, pCore->frame_number);
		}
		if (status.b.osd_bw_fail){
			pstLayerCtx->u32OsdBwFail++;
			CVI_TRACE_VO(CVI_DBG_WARN, "VoDev(%d) osd bw failed at frame#%d\n", VoDev, pCore->frame_number);
		}

		spin_lock_irqsave(&pstLayerCtx->list_lock, flags);
		wait_empty = list_empty(&pstLayerCtx->list_wait);
		spin_unlock_irqrestore(&pstLayerCtx->list_lock, flags);

		if (!pCore->disp_online) {
			// i80 won't need to keep one frame for read, but others need.
			if(!wait_empty || (pCore->disp_interface == CVI_VIP_DISP_INTF_I80)) {
				_vo_hw_enque(VoDev, pstLayerCtx);
				if (pCore->disp_interface == CVI_VIP_DISP_INTF_I80) {
					disp_reg_force_up(VoDev);
					i80_run(VoDev);
				}
			}

			pstLayerCtx->event = 1;
			wake_up_interruptible(&pstLayerCtx->wq);
			//CVI_TRACE_VO(CVI_DBG_INFO, "wakeup thread-%d.\n", VoLayer);
		}

		for (i = 0; i < VO_MAX_WBC_NUM; ++i) {
			if ((gVoCtx->astWbcCtx[i].is_wbc_enable)
				&& (gVoCtx->astWbcCtx[i].stWbcSrc.enSrcType == VO_WBC_SRC_DEV)
				&& (gVoCtx->astWbcCtx[i].stWbcSrc.u32SrcId == VoDev)) {

				pstWbcCtx = &gVoCtx->astWbcCtx[i];

				if (intr_status.b.disp_odma_fifo_full_err)
					pstWbcCtx->u32OdmaFifoFull++;

				wbc_frame_ctrl.s32SrcFrameRate = gVoCtx->astDevCtx[VoDev].stPubAttr.stSyncInfo.u16FrameRate;
				wbc_frame_ctrl.s32DstFrameRate = pstWbcCtx->stWbcAttr.u32FrameRate;

				if (!FRC_INVALID(wbc_frame_ctrl) && (!vo_frame_ctrl(pCore->frame_number, &wbc_frame_ctrl)))
					pstWbcCtx->bIsDrop = CVI_TRUE;

				if (pstWbcCtx->bIsDrop) {
					pstWbcCtx->bIsDrop = CVI_FALSE;
					continue;
				}

				vo_wbc_rdy_buf_remove(pstWbcCtx);
				vo_wbc_dqbuf_list(pstWbcCtx);

				if (vo_wbc_rdy_buf_empty(pstWbcCtx))
					CVI_TRACE_VO(CVI_DBG_ERR, "VoWbc(%d) wbc outbuf is empty\n", i);
				else
					vo_wbc_submit(pstWbcCtx);

				if (pstWbcCtx->is_odma_enable != true) {
					// odma enable
					disp_get_odma_intr_mask(VoDev, &online_odma_mask);
					online_odma_mask.b.disp_online_frame_end = true; //true means disable
					online_odma_mask.b.disp_odma_frame_end = false; //true means disable
					disp_set_odma_intr_mask(VoDev, online_odma_mask);
					disp_odma_enable(VoDev, true);
					pstWbcCtx->is_odma_enable = true;
				}

				pstWbcCtx->event = 1;
				wake_up_interruptible(&pstWbcCtx->wq);
			}
		}
	}
}

irqreturn_t vo_irq_handler(int irq, void *data)
{
	struct cvi_vo_core *pCore = (struct cvi_vo_core *)data;
	union disp_intr intr_status;
	union disp_intr_clr intr_clr;
	VO_DEV VoDev = pCore->core_id;

	if (pCore->irq_num != irq) {
		CVI_TRACE_VO(CVI_DBG_ERR, "irq(%d) Error.\n", irq);
		return IRQ_HANDLED;
	}

	intr_status = disp_intr_status(VoDev);
	intr_clr.b.disp_frame_end = intr_status.b.disp_frame_end;
	intr_clr.b.disp_frame_start = intr_status.b.disp_frame_start;
	intr_clr.b.disp_tgen_lite = intr_status.b.disp_tgen_lite;

	if(intr_status.b.disp_odma_fifo_full_err) {
		// CVI_TRACE_VO(CVI_DBG_ERR, "VoDev(%d) disp_odma_fifo_full_err", VoDev);
		disp_odma_fifofull_clr(VoDev);
	}
	//IC bug, need to clear two times.
	disp_intr_clr(VoDev, intr_clr);
	disp_intr_clr(VoDev, intr_clr);

	// CVI_TRACE_VO(CVI_DBG_INFO, "VoDev(%d) intr_status (0x%x)", VoDev, intr_status.raw);

	irq_handler(pCore, intr_status);

	return IRQ_HANDLED;
}


static s32 _vo_init_param(struct cvi_vo_ctx *ctx)
{
	s32 ret = 0, i, j;
	u16 rgb[3] = {0, 0, 0};

	for (i = 0; i < VO_MAX_DEV_NUM; ++i) {
		ctx->astDevCtx[i].s32BindLayerId = i;
		if (hide_vo) {
			disp_set_pattern(i, PAT_TYPE_FULL, PAT_COLOR_USR, rgb);
			disp_set_frame_bgcolor(i, 0, 0, 0);
		}
	}

	for (i = 0; i < VO_MAX_LAYER_NUM; ++i) {
		ctx->astLayerCtx[i].s32BindDevId = i;
		ctx->astLayerCtx[i].VoLayer = i;
		ctx->astLayerCtx[i].u32DisBufLen = 2;

		spin_lock_init(&ctx->astLayerCtx[i].list_lock);
		INIT_LIST_HEAD(&ctx->astLayerCtx[i].list_wait);
		INIT_LIST_HEAD(&ctx->astLayerCtx[i].list_work);
		INIT_LIST_HEAD(&ctx->astLayerCtx[i].list_done);
		mutex_init(&ctx->astLayerCtx[i].layer_lock);

		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			mutex_init(&ctx->astLayerCtx[i].astChnCtx[j].gdc_lock);
		}
	}

	for (i = 0; i < VO_MAX_WBC_NUM; ++i) {
		ctx->astWbcCtx[i].is_wbc_enable = CVI_FALSE;
		ctx->astWbcCtx[i].is_wbc_src_cfg = CVI_FALSE;
		ctx->astWbcCtx[i].is_wbc_attr_cfg = CVI_FALSE;
		ctx->astWbcCtx[i].stWbcSrc.enSrcType = VO_WBC_SRC_DEV;
		ctx->astWbcCtx[i].stWbcSrc.u32SrcId = 0;
		ctx->astWbcCtx[i].stWbcAttr.stTargetSize.u32Width = 0;
		ctx->astWbcCtx[i].stWbcAttr.stTargetSize.u32Height = 0;
		ctx->astWbcCtx[i].stWbcAttr.enPixFormat = PIXEL_FORMAT_NV21;
		ctx->astWbcCtx[i].stWbcAttr.u32FrameRate = 0;
		ctx->astWbcCtx[i].stWbcAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;
		ctx->astWbcCtx[i].stWbcAttr.enCompressMode = COMPRESS_MODE_NONE;
		ctx->astWbcCtx[i].enWbcMode = VO_WBC_MODE_NORM;
		ctx->astWbcCtx[i].u32Depth = VO_WBC_DONEQ;
		ctx->astWbcCtx[i].u32DoneCnt = 0;
		ctx->astWbcCtx[i].u32FrameNum = 0;
		ctx->astWbcCtx[i].u32WbcFrameRate = 0;
		mutex_init(&ctx->astWbcCtx[i].wbc_lock);
	}

	return ret;
}

s32 vo_recv_frame(MMF_CHN_S chn, VB_BLK blk)
{
	s32 ret;
	struct cvi_vo_chn_ctx *pstChnCtx;
	struct vb_jobs_t *jobs;
	struct vb_s *vb = (struct vb_s *)blk;
	FRAME_RATE_CTRL_S chn_frame_ctrl;

	ret = CHECK_VO_CHN_VALID(chn.s32DevId, chn.s32ChnId);
	if (ret != CVI_SUCCESS)
		return ret;

	pstChnCtx = &gVoCtx->astLayerCtx[chn.s32DevId].astChnCtx[chn.s32ChnId];

	if (!pstChnCtx->is_chn_enable) {
		CVI_TRACE_VO(CVI_DBG_INFO, "layer(%d) chn(%d) disable.\n", chn.s32DevId, chn.s32ChnId);
		return CVI_FAILURE;
	}

	pstChnCtx->u32FrameIndex++;
	pstChnCtx->u32SrcFrameNum++;
	chn_frame_ctrl.s32SrcFrameRate = pstChnCtx->u32ChnSrcFrameRate;
	chn_frame_ctrl.s32DstFrameRate = pstChnCtx->u32FrameRateUserSet;
	pstChnCtx->bIsDrop = CVI_FALSE;

	if (!FRC_INVALID(chn_frame_ctrl) && (!vo_frame_ctrl(pstChnCtx->u32FrameIndex, &chn_frame_ctrl))) {
		pstChnCtx->bIsDrop = CVI_TRUE;
	}

	if ((pstChnCtx->bPause && (!pstChnCtx->bRefresh)) || pstChnCtx->bHide || pstChnCtx->bIsDrop)
		return CVI_SUCCESS;
	if (IS_VB_OFFSET_INVALID(vb->buf)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) vb offset (%d %d %d %d) invalid\n",
			chn.s32DevId, chn.s32ChnId,
			vb->buf.s16OffsetLeft, vb->buf.s16OffsetRight,
			vb->buf.s16OffsetTop, vb->buf.s16OffsetBottom);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	pstChnCtx->u32FrameNum++;

	if (pstChnCtx->enRotation != ROTATION_0) {
		if (mutex_trylock(&pstChnCtx->gdc_lock)) {
			struct _vo_gdc_cb_param cb_param = { .chn = chn, .usage = GDC_USAGE_LDC};
			SIZE_S size;

			if (vb->buf.size.u32Width % LDC_ALIGN) {
				size.u32Width = ALIGN(vb->buf.size.u32Width, LDC_ALIGN);
				vb->buf.s16OffsetLeft = 0;
				vb->buf.s16OffsetRight =
					size.u32Width - vb->buf.size.u32Width;
				vb->buf.size.u32Width = size.u32Width;
			}
			if (vb->buf.size.u32Height % LDC_ALIGN) {
				size.u32Height = ALIGN(vb->buf.size.u32Height, LDC_ALIGN);
				vb->buf.s16OffsetTop = 0;
				vb->buf.s16OffsetBottom =
					size.u32Height - vb->buf.size.u32Height;
				vb->buf.size.u32Height = size.u32Height;
			}
			CVI_TRACE_VO(CVI_DBG_INFO, "VoLayer(%d) VoChn(%d) gdc vb offset(%d %d %d %d).\n",
				chn.s32DevId, chn.s32ChnId,
				vb->buf.s16OffsetLeft, vb->buf.s16OffsetTop,
				vb->buf.s16OffsetRight, vb->buf.s16OffsetBottom);

			atomic_fetch_add(1, &vb->usr_cnt);
			atomic_long_fetch_or(BIT(CVI_ID_GDC), &vb->mod_ids);

			if (_mesh_gdc_do_op_cb(GDC_USAGE_ROTATION
				, NULL
				, vb
				, gVoCtx->astLayerCtx[chn.s32DevId].stLayerAttr.enPixFormat
				, pstChnCtx->mesh.paddr
				, CVI_FALSE, &cb_param
				, sizeof(cb_param)
				, CVI_ID_VO
				, pstChnCtx->enRotation) != CVI_SUCCESS) {
				mutex_unlock(&pstChnCtx->gdc_lock);
				CVI_TRACE_VO(CVI_DBG_ERR, "gdc rotation failed.\n");
				return CVI_FAILURE;
			}
			CVI_TRACE_VO(CVI_DBG_DEBUG, "VoLayer(%d) VoChn(%d) push vb(0x%llx) to gdc job.\n",
				chn.s32DevId, chn.s32ChnId, vb->phy_addr);

			return CVI_SUCCESS;
		} else {
			CVI_TRACE_VO(CVI_DBG_ERR, "layer(%d) chn(%d) drop frame due to gdc op blocked.\n",
				chn.s32DevId, chn.s32ChnId);
			return CVI_FAILURE;
		}
	}

	jobs = &pstChnCtx->chn_jobs;
	mutex_lock(&jobs->lock);
	if (FIFO_FULL(&jobs->waitq)) {
		struct vb_s *vb_old = NULL;

		FIFO_POP(&jobs->waitq, &vb_old);
		atomic_long_fetch_and(~BIT(chn.enModId), &vb_old->mod_ids);
		vb_release_block((VB_BLK)vb_old);
	}
	FIFO_PUSH(&jobs->waitq, vb);
	mutex_unlock(&jobs->lock);

	atomic_fetch_add(1, &vb->usr_cnt);
	atomic_long_fetch_or(BIT(chn.enModId), &vb->mod_ids);

	mutex_lock(&gVoCtx->astLayerCtx[chn.s32DevId].layer_lock);
	gVoCtx->astLayerCtx[chn.s32DevId].bLayerUpdate = CVI_TRUE;
	mutex_unlock(&gVoCtx->astLayerCtx[chn.s32DevId].layer_lock);

	CVI_TRACE_VO(CVI_DBG_INFO, "VoLayer(%d) VoChn(%d) push vb(0x%llx).\n",
		chn.s32DevId, chn.s32ChnId, vb->phy_addr);

	return ret;
}

static void _update_vo_real_frame_rate(struct timer_list *timer)
{
	int i, j;

	UNUSED(timer);

	for (i = 0; i < VO_MAX_LAYER_NUM; ++i) {
		if (!gVoCtx->astLayerCtx[i].is_layer_enable)
			continue;
		gVoCtx->astLayerCtx[i].u32LayerFrameRate = gVoCtx->astLayerCtx[i].u32FrameNum;
		gVoCtx->astLayerCtx[i].u32FrameNum = 0;

		gVoCtx->astLayerCtx[i].u32LayerSrcFrameRate = gVoCtx->astLayerCtx[i].u32SrcFrameNum;
		gVoCtx->astLayerCtx[i].u32SrcFrameNum = 0;

		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (!gVoCtx->astLayerCtx[i].astChnCtx[j].is_chn_enable)
				continue;
			gVoCtx->astLayerCtx[i].astChnCtx[j].u32ChnFrameRate =
				gVoCtx->astLayerCtx[i].astChnCtx[j].u32FrameNum;
			gVoCtx->astLayerCtx[i].astChnCtx[j].u32FrameNum = 0;

			gVoCtx->astLayerCtx[i].astChnCtx[j].u32ChnSrcFrameRate =
				gVoCtx->astLayerCtx[i].astChnCtx[j].u32SrcFrameNum;
			gVoCtx->astLayerCtx[i].astChnCtx[j].u32SrcFrameNum = 0;
		}
	}

	for (i = 0; i < VO_MAX_WBC_NUM; ++i) {
		if (!gVoCtx->astWbcCtx[i].is_wbc_enable)
			continue;

		gVoCtx->astWbcCtx[i].u32WbcFrameRate = gVoCtx->astWbcCtx[i].u32FrameNum;
		gVoCtx->astWbcCtx[i].u32FrameNum = 0;
	}


	mod_timer(&vo_timer_proc, jiffies + msecs_to_jiffies(1000));
}

/*******************************************************
 *  Common interface for core
 ******************************************************/
s32 vo_create_instance(struct platform_device *pdev)
{
	s32 ret = 0;

	gVdev = dev_get_drvdata(&pdev->dev);
	if (!gVdev) {
		CVI_TRACE_VO(CVI_DBG_ERR, "invalid data\n");
		return -EINVAL;
	}

#if 0 // TODO: set clk
	s32 i;

	for (i = 0; i < VO_MAX_DEV_NUM; ++i) {
		if (disp_mux_get(i) == DISP_VO_SEL_I80) {
			smooth[i] = disp_check_i80_enable(i);
		} else {
			smooth[i] = disp_check_tgen_enable(i);
		}

		if (!smooth[i]) {
			if (gVdev->vo_core[i].clk_disp && __clk_is_enabled(gVdev->vo_core[i].clk_disp))
				clk_disable_unprepare(gVdev->vo_core[i].clk_disp);
			if (gVdev->vo_core[i].clk_bt && __clk_is_enabled(gVdev->vo_core[i].clk_bt))
				clk_disable_unprepare(gVdev->vo_core[i].clk_bt);
			if (gVdev->vo_core[i].clk_dsi && __clk_is_enabled(gVdev->vo_core[i].clk_dsi))
				clk_disable_unprepare(gVdev->vo_core[i].clk_dsi);
		}
	}
#endif

	gVoCtx = kzalloc(sizeof(struct cvi_vo_ctx), GFP_ATOMIC);
	if (!gVoCtx) {
		CVI_TRACE_VO(CVI_DBG_ERR, "gVoCtx alloc size(%ld) failed\n", sizeof(struct cvi_vo_ctx));
		return -ENOMEM;
	}
	_vo_init_param(gVoCtx);

	ret = _vo_create_proc(gVoCtx);
	if (ret) {
		CVI_TRACE_VO(CVI_DBG_ERR, "Failed to create proc\n");
		return ret;
	}

	base_register_recv_cb(CVI_ID_VO, vo_recv_frame);

	add_timer(&vo_timer_proc);
	mod_timer(&vo_timer_proc, jiffies + msecs_to_jiffies(1000));

	return CVI_SUCCESS;
}

s32 vo_destroy_instance(struct platform_device *pdev)
{
	if (!gVdev) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VO has been destroyed!\n");
		return -EINVAL;
	}

	del_timer_sync(&vo_timer_proc);
	base_unregister_recv_cb(CVI_ID_VO);
	_vo_destroy_proc();
	kfree(gVoCtx);
	gVoCtx = NULL;
	gVdev = NULL;

	return CVI_SUCCESS;
}
