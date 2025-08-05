#include "vi.h"
#include "comm_errno.h"
#include "base_cb.h"
#include "cif_cb.h"
#include "vpss_cb.h"
#include "vi_cb.h"
#include "ldc_cb.h"
#include "snsr_cb.h"
#include "vbq.h"
#include "base_common.h"
#include "cmdq.h"
#include "vi_sys.h"
#include "comm_math.h"
#include "vi_dma_setup.h"
#include "vi_misc.h"

#define VI_MAX_LIST_NUM		(0x40)
#define VI_TIMEOUT_MS		(1000)

/*******************************************************
 *  Global variables
 ******************************************************/
struct overflow_info *g_overflow_info;
struct _vi_gdc_cb_param {
	mmf_chn_s chn;
	enum gdc_usage usage;
};

/*******************************************************
 *  Internal APIs
 ******************************************************/

/**
 * try to trigger preraw and linespliter after postraw done or drop frame done
 */
static void isp_trig_whole_preraw(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	s8 ret = ISP_SUCCESS;

	ret = _pre_hw_enque(vdev, raw_num, ISP_FE_CH0);
	if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
		ret |= _pre_hw_enque(vdev, raw_num, ISP_FE_CH1);
	}
}

static inline void _vi_wake_up_post_th(struct sop_vi_dev *vdev)
{
	enum E_VI_TH th_id = E_VI_TH_POSTRAW;

	osal_atomic_set(&vdev->vi_th[th_id].flag, ISP_PRERAW_MAX);
	osal_wait_wakeup(&vdev->vi_th[th_id].wq);
}

static void vi_gdc_callback(struct sop_vi_dev *vdev, void *param, vb_blk blk)
{
	struct _vi_gdc_cb_param *_param = param;
	int pipe = _param->chn.dev_id;
	int chn = _param->chn.chn_id;

	if (!param)
		return;

	vi_pr(VI_DBG, "vi pipe(%d) chn(%d) usage(%d)\n", pipe, chn, _param->usage);
	osal_atomic_set(&vdev->mesh[pipe][chn].gdc_flag, 0);
	if (blk != VB_INVALID_HANDLE)
		vb_done_handler(_param->chn, CHN_TYPE_OUT, &vdev->vi_jobs[pipe][chn], blk);
	osal_vfree(param);
}

static s32 _mesh_gdc_do_op_cb(enum gdc_usage usage, const void *usage_param,
			      struct vb_s *vb_in, pixel_format_e pixformat, u64 mesh_addr,
			      u8 sync_io, void *cb_param, u32 cb_param_size,
			      mod_id_e mod_id, rotation_e rotation)
{
	struct mesh_gdc_cfg cfg;
	struct base_exe_m_cb exe_cb;

	osal_memset(&cfg, 0, sizeof(cfg));
	cfg.usage = usage;
	cfg.usage_param = usage_param;
	cfg.vb_in = vb_in;
	cfg.pix_format = pixformat;
	cfg.mesh_addr = mesh_addr;
	cfg.sync_io = sync_io;
	cfg.cb_param = cb_param;
	cfg.cb_param_size = cb_param_size;
	cfg.rotation = rotation;

	exe_cb.callee = E_MODULE_LDC;
	exe_cb.caller = E_MODULE_VI;
	exe_cb.cmd_id = LDC_CB_MESH_GDC_OP;
	exe_cb.data   = &cfg;
	return base_exe_module_cb(&exe_cb);
}

static void _vi_release_ext_buf(u64 phy_addr)
{
	vb_blk blk = 0;

	blk = vb_phys_addr2handle(phy_addr);
	if (blk != VB_INVALID_HANDLE) {
		vb_release_block(blk);
	}
}

void isp_snr_cfg_enq(struct sop_isp_snr_update *snr_node, const enum sop_isp_raw raw_num)
{
	unsigned long flags;
	struct _isp_snr_i2c_node *n, *q;
	struct _isp_crop_node  *c_n;

	if (snr_node == NULL)
		return;

	osal_spin_lock_irqsave(&snr_node_lock[raw_num], &flags);

	if (snr_node->snr_cfg_node.isp.need_update) {
		c_n = osal_kmalloc(sizeof(*c_n), OSAL_GFP_ATOMIC);
		if (c_n == NULL) {
			vi_pr(VI_ERR, "SNR cfg node alloc size(%zu) fail\n", sizeof(*n));
			osal_spin_unlock_irqrestore(&snr_node_lock[raw_num], &flags);
			return;
		}
		memcpy(&c_n->n, &snr_node->snr_cfg_node.isp, sizeof(struct snsr_isp_s));
		osal_list_add_tail(&c_n->list, &isp_crop_queue[raw_num].list);
	}

	if (snr_node->snr_cfg_node.snsr.need_update) {
		n = osal_kmalloc(sizeof(*n), OSAL_GFP_ATOMIC);
		if (n == NULL) {
			vi_pr(VI_ERR, "SNR cfg node alloc size(%zu) fail\n", sizeof(*n));
			osal_spin_unlock_irqrestore(&snr_node_lock[raw_num], &flags);
			return;
		}
		osal_memcpy(&n->n, &snr_node->snr_cfg_node.snsr, sizeof(struct snsr_regs_s));

		while (!osal_list_empty(&isp_snr_i2c_queue[raw_num].list)
			&& (isp_snr_i2c_queue[raw_num].num_rdy >= (VI_MAX_LIST_NUM - 1))) {
			q = osal_list_first_entry(&isp_snr_i2c_queue[raw_num].list, struct _isp_snr_i2c_node, list);
			osal_list_del_init(&q->list);
			--isp_snr_i2c_queue[raw_num].num_rdy;
			osal_kfree(q);
		}
		osal_list_add_tail(&n->list, &isp_snr_i2c_queue[raw_num].list);
		++isp_snr_i2c_queue[raw_num].num_rdy;
	}

	osal_spin_unlock_irqrestore(&snr_node_lock[raw_num], &flags);
}

void sop_isp_rdy_buf_queue(struct sop_vi_dev *vdev, struct sop_isp_buf *b)
{
	unsigned long flags;
	struct isp_ctx *ctx = &vdev->ctx;
	int pipe = b->buf.index;
	int chn = b->buf.reserved;
	enum sop_isp_raw raw_num;

	vi_pr(VI_DBG, "ISP buf_queue pipe=%d chn=%d\n", pipe, chn);

	osal_spin_lock_irqsave(&vdev->qbuf_lock, &flags);
	osal_list_add_tail(&b->list, &vdev->qbuf_list[pipe][chn]);
	++vdev->qbuf_num[pipe][chn];
	osal_spin_unlock_irqrestore(&vdev->qbuf_lock, &flags);

	if (osal_atomic_read(&vdev->stream.isp_streamon[pipe]) && vdev->qbuf_num[pipe][chn] == 1) {
		raw_num = ctx->isp_pipe_cfg[pipe].bind_raw;
		if (ctx->isp_pipe_cfg[pipe].is_yuv_sensor &&
			ctx->isp_pipe_cfg[pipe].yuv_scene_mode == ISP_YUV_SCENE_BYPASS)
			_isp_yuv_bypass_trigger(vdev, raw_num, ISP_FE_CH0);
		else {
			if (!_is_fe_post_slice(&vdev->ctx))
				isp_trig_whole_preraw(vdev, raw_num);
		}
	}
}

struct sop_isp_buf *sop_isp_rdy_buf_next(struct sop_vi_dev *vdev, const u8 pipe, const u8 chn)
{
	unsigned long flags;
	struct sop_isp_buf *b = NULL;

	osal_spin_lock_irqsave(&vdev->qbuf_lock, &flags);
	if (!osal_list_empty(&vdev->qbuf_list[pipe][chn]))
		b = osal_list_first_entry(&vdev->qbuf_list[pipe][chn], struct sop_isp_buf, list);
	osal_spin_unlock_irqrestore(&vdev->qbuf_lock, &flags);

	return b;
}

int sop_isp_rdy_buf_empty(struct sop_vi_dev *vdev, const u8 pipe, const u8 chn)
{
	unsigned long flags;
	int empty = 0;

	osal_spin_lock_irqsave(&vdev->qbuf_lock, &flags);
	empty = (vdev->qbuf_num[pipe][chn] == 0);
	osal_spin_unlock_irqrestore(&vdev->qbuf_lock, &flags);

	return empty;
}

void sop_isp_rdy_buf_pop(struct sop_vi_dev *vdev, const u8 pipe, const u8 chn)
{
	unsigned long flags;

	osal_spin_lock_irqsave(&vdev->qbuf_lock, &flags);
	vdev->qbuf_num[pipe][chn]--;
	osal_spin_unlock_irqrestore(&vdev->qbuf_lock, &flags);
}

void sop_isp_rdy_buf_remove(struct sop_vi_dev *vdev, const u8 pipe, const u8 chn)
{
	unsigned long flags;
	struct sop_isp_buf *b = NULL;

	osal_spin_lock_irqsave(&vdev->qbuf_lock, &flags);
	if (!osal_list_empty(&vdev->qbuf_list[pipe][chn])) {
		b = osal_list_first_entry(&vdev->qbuf_list[pipe][chn], struct sop_isp_buf, list);
		osal_list_del_init(&b->list);
		osal_kfree(b);
	}
	osal_spin_unlock_irqrestore(&vdev->qbuf_lock, &flags);
}

void _vi_dma_set_sw_mode(struct isp_ctx *ctx, enum sop_isp_raw raw_num, bool is_csi)
{
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, raw_num);

	if (is_csi) {
		switch (phy_raw) {
		case ISP_PRERAW0:
			//FE
			ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI0_BDG0, false);
			ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI0_BDG1, false);
			ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI0_BDG2, false);
			ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI0_BDG3, false);
			ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_FE0_CLSC_LE, false);
			ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_FE0_AE_HIST_LE, false);
			ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_FE0_AE_HIST_SE, false);
			break;
		case ISP_PRERAW1:
			ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI1_BDG0, false);
			ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI1_BDG1, false);
			ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_FE1_CLSC_LE, false);
			ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_FE1_AE_HIST_LE, false);
			ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_FE1_AE_HIST_SE, false);
			break;
		case ISP_PRERAW2:
			ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI2_BDG0, false);
			ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_FE2_CLSC_LE, false);
			ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_FE2_AE_HIST_LE, false);
			break;
		case ISP_PRERAW_LITE0:
			ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_BT0_LITE0, false);
			ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_BT0_LITE1, false);
			ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_BT0_LITE2, false);
			ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_BT0_LITE3, false);
			break;
		default:
			vi_pr(VI_ERR, "raw_num(%d) is wrong\n", raw_num);
			break;
		}
	} else {
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_LE,
				_is_fe_post_slice(ctx) ? false : true);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_SE,
				_is_fe_post_slice(ctx) ? false : true);
		//RAWTOP
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_AF_W, false);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_LSCR_HIST, false);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_DRC_POLY_R, false);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_DRC_POLY_W, false);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_DRC_HIST, false);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CLUT_R, false);

		//YUVTOP
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_LDCI_HIST, true);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_LDCI_W, false);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_LDCI_R, false);

		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_ST_Y, true);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_ST_C, true);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_ST_MV, false);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_ST_MO, false);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_ST_FCB, false);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_ST_MSP, false);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_RESIZE, false);

		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CNR_Y_W, true);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CNR_Y_R, true);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CNR_C_W, true);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CNR_C_R, true);

		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_LD_Y, false);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_LD_C, false);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_LD_MV, false);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_LD_MO, false);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_LD_FCB, false);

		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_Y, false);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_C, false);
	}
}

static void _isp_rawtop_init(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	bool is_hdr = ctx->cfg_info.is_hdr_on;

	// raw_top
	ispblk_rawtop_config(ctx);

	//liner use wb0/hdr use wb1
	ispblk_blc_dg_wb_config(ctx, ISP_BLK_ID_BLC_DG_WB0, false);
	ispblk_blc_dg_wb_config(ctx, ISP_BLK_ID_BLC_DG_WB1, false);

	ispblk_fusion_config(ctx, !_is_all_online(ctx), is_hdr ? ISP_FS_OUT_FS : ISP_FS_OUT_LONG);

	ispblk_map_curve_config(ctx, false);

	ispblk_dpc_config(ctx, false, 0);

	ispblk_af_config(ctx, true);

	ispblk_bnr_config(ctx, false);

	ispblk_lscr_config(ctx, false, lscr_lut, ARRAY_SIZE(lscr_lut));

	ispblk_drc_config(ctx, false);

	ispblk_cfa_config(ctx, ISP_BLK_ID_CFA);
}

static void _isp_rgbtop_init(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;

	ispblk_rgbtop_config(ctx);

	ispblk_ccm_config(ctx, ISP_BLK_ID_CCM, false, &ccm_hw_cfg);

	ispblk_pfr_config(ctx, false);
	ispblk_edge_ext_config(ctx, false);

	ispblk_gamma_config(ctx, false, vdev->gamma_tbl_idx, gamma_data, 0);
	ispblk_gamma_enable(ctx, false);

	ispblk_rgbdither_config(ctx, true, false, false, false);
	// ispblk_clut_config(ctx, true, true, c_lut_r_lut, c_lut_g_lut, c_lut_b_lut);

	ispblk_csc_config(ctx);
}

static void _isp_yuvtop_init(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;

	ispblk_yuvtop_config(ctx);

	ispblk_dci_map_config(ctx, 1, dci_map_lut_50);
	ispblk_ldci_config(ctx, false, false);
	ispblk_post_ee_config(ctx, false);
	ispblk_ee_front_back_config(ctx, false);

	ispblk_mctf_config(ctx, ctx->is_3dnr_on, 0);

	if (ctx->is_3dnr_on) {
		ispblk_fbce_config(ctx, ctx->is_fbc_on);
		ispblk_fbcd_config(ctx, ctx->is_fbc_on);
		ispblk_fbc_ring_buf_config(ctx, ctx->is_fbc_on);
	}

	ispblk_yuvdither_config(ctx, 0, true, true, true, true);
	ispblk_yuvdither_config(ctx, 1, true, true, true, true);

	ispblk_ycur_config(ctx, false, 0, ycur_data);
	ispblk_ycur_enable(ctx, false, 0);

	ispblk_ca_config(ctx, false, 1);
	ispblk_ca_lite_config(ctx, false);

	ispblk_cnr_config(ctx, true, 0, 0, 0);

	ispblk_crop_enable(ctx, ISP_BLK_ID_YUV_CROP_Y, false);
	ispblk_crop_enable(ctx, ISP_BLK_ID_YUV_CROP_C, false);
}

static u32 _is_drop_next_frame(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const enum sop_isp_fe_chn_num chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	u32 start_drop_num = ctx->isp_csi_cfg[raw_num].drop_ref_frm_num;
	u32 end_drop_num = start_drop_num + ctx->isp_csi_cfg[raw_num].drop_frm_cnt;
	u32 frm_num = 0;
	u8 pipe = 0;

	if (ctx->isp_csi_cfg[raw_num].is_drop_next_frame) {
		//for tuning_dis, shoudn't trigger preraw;
		if ((ctx->is_multi_sensor) && (!ctx->isp_csi_cfg[raw_num].is_yuv_sensor)) {
			pipe = tuning_dis[0] > 0 ? tuning_dis[0] - 1 : 0;
			if ((tuning_dis[0] > 0) && (ctx->isp_pipe_cfg[pipe].bind_raw != raw_num)) {
				vi_pr(VI_DBG, "input buf is not equal to current tuning number\n");
				return 1;
			}
		}

		if (osal_atomic_read(&vdev->is_drop))
			return 1;

		//if sof_num in [start_sof, end_sof), shoudn't trigger preraw;
		frm_num = vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0];

		if ((start_drop_num != 0) && (frm_num >= start_drop_num) && (frm_num < end_drop_num))
			return 1;
	}

	return 0;
}

static void _set_drop_frm_info(
	const struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	struct isp_i2c_data *i2c_data)
{
	struct isp_ctx *ctx = (struct isp_ctx *)(&vdev->ctx);

	if (!i2c_data->drop_frame)
		return;

	ctx->isp_csi_cfg[raw_num].drop_frm_cnt = i2c_data->drop_frame_cnt;

	ctx->isp_csi_cfg[raw_num].is_drop_next_frame = true;
	ctx->isp_csi_cfg[raw_num].drop_ref_frm_num = vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0];

	vi_pr(VI_DBG, "raw_%d, drop_ref_frm_num=%d, drop frame=%d\n", raw_num,
				ctx->isp_csi_cfg[raw_num].drop_ref_frm_num,
				i2c_data->drop_frame_cnt);
}

static void _clear_drop_frm_info(
	const struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num)
{
	struct isp_ctx *ctx = (struct isp_ctx *)(&vdev->ctx);

	ctx->isp_csi_cfg[raw_num].drop_frm_cnt = 0;
	ctx->isp_csi_cfg[raw_num].drop_ref_frm_num = 0;
	ctx->isp_csi_cfg[raw_num].is_drop_next_frame = false;
}

static void _isp_crop_update_chk(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	struct _isp_crop_node *node)
{
	struct isp_ctx *ctx = &vdev->ctx;
	u16 del_node = true;
	unsigned long flags;
	enum sop_isp_fe_chn_num chn_num = ISP_FE_CH0;
	u8 chn_max = ctx->isp_csi_cfg[raw_num].is_hdr_on ? ISP_FE_CH1 : ISP_FE_CH0;

	if (node->n.dly_frm_num == 0) {

		for (chn_num = ISP_FE_CH0; chn_num <= chn_max; chn_num++) {
			ctx->isp_csi_cfg[raw_num].crop[chn_num].x = node->n.wdr.img_size[chn_num].start_x;
			ctx->isp_csi_cfg[raw_num].crop[chn_num].y = node->n.wdr.img_size[chn_num].start_y;
			ctx->isp_csi_cfg[raw_num].crop[chn_num].w = node->n.wdr.img_size[chn_num].active_w;
			ctx->isp_csi_cfg[raw_num].crop[chn_num].h = node->n.wdr.img_size[chn_num].active_h;

			vi_pr(VI_DBG, "Preraw_%d, crop[%d] x:y:w:h=%d:%d:%d:%d\n", raw_num, chn_num,
						ctx->isp_csi_cfg[raw_num].crop[chn_num].x,
						ctx->isp_csi_cfg[raw_num].crop[chn_num].y,
						ctx->isp_csi_cfg[raw_num].crop[chn_num].w,
						ctx->isp_csi_cfg[raw_num].crop[chn_num].h);
		}

		//TODO maybe have different chn size eg:ov48b
		ctx->isp_csi_cfg[raw_num].csibdg_width =
					node->n.wdr.img_size[0].width;
		ctx->isp_csi_cfg[raw_num].csibdg_height =
					node->n.wdr.img_size[0].height;
		ctx->isp_csi_cfg[raw_num].rgb_color_mode_pre_crop =
						bayer_type_before_crop(ctx->isp_csi_cfg[raw_num].rgb_color_mode,
								ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].x,
								ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].y);
		/*mux sensor update at fe_done*/
		ispblk_csibdg_crop_update(ctx, raw_num, true);
		ispblk_csibdg_update_size(ctx, raw_num);
	} else {
		node->n.dly_frm_num--;
		del_node = false;
	}

	if (del_node) {
		vi_pr(VI_DBG, "crop del node and free\n");
		osal_spin_lock_irqsave(&snr_node_lock[raw_num], &flags);
		osal_list_del_init(&node->list);
		osal_kfree(node);
		osal_spin_unlock_irqrestore(&snr_node_lock[raw_num], &flags);
	}
}

static void _isp_crop_update(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	struct _isp_crop_node **_crop_n,
	const u16 _crop_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct _isp_crop_node *node;
	u16 i = 0;

	if (ctx->is_rawreplay || ctx->isp_csi_cfg[raw_num].is_patgen_en)
		return;

	for (i = 0; i < _crop_num; i++) {
		node = _crop_n[i];

		_isp_crop_update_chk(vdev, raw_num, node);
	}
}

static void _snr_i2c_update(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	struct _isp_snr_i2c_node **_i2c_n,
	const u16 _i2c_num,
	int is_vblank_update)
{
	struct _isp_snr_i2c_node *node;
	struct _isp_snr_i2c_node *next_node;
	struct isp_i2c_data *i2c_data;
	struct isp_i2c_data *next_i2c_data;
	unsigned long flags;
	u16 i = 0, j = 0;
	// 0: Delete Node, 1: Postpone Node, 2: Do nothing
	u16 del_node = 0;
	u32 dev_mask = 0;
	u32 cmd = burst_i2c_en ? CVI_SNS_I2C_BURST_QUEUE : CVI_SNS_I2C_WRITE;
	u8 no_update = 1;
	u32 fe_frm_num = 0;

	if (vdev->ctx.is_rawreplay || vdev->ctx.isp_csi_cfg[raw_num].is_patgen_en)
		return;

	fe_frm_num = vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0];

	for (j = 0; j < _i2c_num; j++) {
		node = _i2c_n[j];
		no_update = 1;

		vi_pr(VI_DBG, "raw_num=%d, i2c_num=%d, j=%d, magic_num=%d, fe_frm_num=%d, v_blank_update=%d\n",
				raw_num, _i2c_num, j, node->n.magic_num, fe_frm_num, is_vblank_update);

		//magic num set by ISP team. fire i2c when magic num same as last fe frm num.
		if (((node->n.magic_num == fe_frm_num ||
			 (node->n.magic_num < fe_frm_num && (j + 1) >= _i2c_num)) && (!is_vblank_update)) ||
			 ((node->n.magic_num_vblank  == fe_frm_num ||
			 (node->n.magic_num_vblank  < fe_frm_num && (j + 1) >= _i2c_num)) && (is_vblank_update))) {

			if ((node->n.magic_num != fe_frm_num && !is_vblank_update) ||
				(node->n.magic_num_vblank != fe_frm_num && is_vblank_update)) {
				vi_pr(VI_WARN, "exception handle, send delayed i2c data.\n");
			}

			for (i = 0; i < node->n.regs_num; i++) {
				i2c_data = &node->n.i2c_data[i];

				vi_pr(VI_DBG, "i2cdata[%d]:i2c_addr=0x%x write:0x%x needvblank:%d needupdate:%d\n", i,
				i2c_data->reg_addr, i2c_data->data, i2c_data->vblank_update, i2c_data->update);

				if (burst_i2c_en)
					dev_mask |= BIT(i2c_data->i2c_dev);

				if (i2c_data->update && (i2c_data->dly_frm_num == 0)) {
					if ((i2c_data->vblank_update && is_vblank_update)
					|| (!i2c_data->vblank_update && !is_vblank_update)) {
						vi_sys_cmm_cb_i2c(cmd, (void *)i2c_data);
						i2c_data->update = 0;

						_set_drop_frm_info(vdev, raw_num, i2c_data);
					} else {
						no_update = 0;
					}
				} else if (i2c_data->update && !(i2c_data->dly_frm_num == 0)) {
					vi_pr(VI_DBG, "addr=0x%x, dly_frm=%d\n",
							i2c_data->reg_addr, i2c_data->dly_frm_num);
					i2c_data->dly_frm_num--;
					del_node = 1;
				}
			}

		} else if ((node->n.magic_num < fe_frm_num && !is_vblank_update) ||
					(node->n.magic_num_vblank < fe_frm_num && is_vblank_update)) {

			if ((j + 1) < _i2c_num) {

				next_node = _i2c_n[j + 1];

				for (i = 0; i < next_node->n.regs_num; i++) {
					next_i2c_data = &next_node->n.i2c_data[i];
					i2c_data = &node->n.i2c_data[i];

					if (i2c_data->update && next_i2c_data->update == 0) {
						next_i2c_data->update = i2c_data->update;
						vi_pr(VI_WARN, "exception handle, i2c node merge, addr: 0x%x\n",
							i2c_data->reg_addr);
					}
				}

				del_node = 0;
			} else {
				// impossible case
			}
		} else {
			del_node = 2;
		}

		if (del_node == 0 && no_update) {
			vi_pr(VI_DBG, "i2c node %d del node and free\n", j);
			osal_spin_lock_irqsave(&snr_node_lock[raw_num], &flags);
			osal_list_del_init(&node->list);
			--isp_snr_i2c_queue[raw_num].num_rdy;
			osal_kfree(node);
			osal_spin_unlock_irqrestore(&snr_node_lock[raw_num], &flags);
		} else if (del_node == 1) {
			if (is_vblank_update)
				node->n.magic_num_vblank++;
			else
				node->n.magic_num++;
			vi_pr(VI_DBG, "postpone i2c node\n");
		}
	}

	while (dev_mask) {
		u32 tmp = ffs(dev_mask) - 1;

		vi_sys_cmm_cb_i2c(CVI_SNS_I2C_BURST_FIRE, (void *)&tmp);
		dev_mask &= ~BIT(tmp);
	}
}

static void _isp_snr_cfg_deq_and_fire(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	int needvblank)
{
	struct osal_list_head *pos, *temp;
	struct _isp_snr_i2c_node *i2c_n[VI_MAX_LIST_NUM], *i2c_n_temp;
	struct _isp_crop_node *crop_n[VI_MAX_LIST_NUM];
	unsigned long flags;
	u16 i2c_num = 0, crop_num = 0;
	int i;

	osal_spin_lock_irqsave(&snr_node_lock[raw_num], &flags);
	if (needvblank == 0) {
		osal_list_for_each_safe(pos, temp, &isp_snr_i2c_queue[raw_num].list) {
			i2c_n[i2c_num] = osal_list_entry(pos, struct _isp_snr_i2c_node, list);
			i2c_num++;
		}

		osal_list_for_each_safe(pos, temp, &isp_crop_queue[raw_num].list) {
			if (crop_num < i2c_num) {
				crop_n[crop_num] = list_entry(pos, struct _isp_crop_node, list);
				crop_num++;
			}
		}
	} else {
		osal_list_for_each_safe(pos, temp, &isp_snr_i2c_queue[raw_num].list) {
			i2c_n_temp = osal_list_entry(pos, struct _isp_snr_i2c_node, list);
			for (i = 0; i < i2c_n_temp->n.regs_num; i++) {
				if (i2c_n_temp->n.i2c_data[i].vblank_update &&
				    i2c_n_temp->n.i2c_data[i].update) {
					i2c_n[i2c_num] = i2c_n_temp;
					i2c_num++;
					break;
				}
			}
		}
	}

	osal_spin_unlock_irqrestore(&snr_node_lock[raw_num], &flags);

	if (i2c_num > 0)
		_snr_i2c_update(vdev, raw_num, i2c_n, i2c_num, needvblank);

	if (crop_num > 0)
		_isp_crop_update(vdev, raw_num, crop_n, crop_num);
}

static inline void _vi_clear_mmap_fbc_ring_base(struct sop_vi_dev *vdev, const u8 pipe)
{
	struct isp_ctx *ctx = &vdev->ctx;

	//Clear mmap previous ring base to start addr after first frame done.
	if (ctx->is_3dnr_on && (ctx->isp_pipe_cfg[pipe].first_frm_cnt == 1)) {
		if (ctx->is_fbc_on) {
			ispblk_fbc_chg_to_sw_mode(&vdev->ctx);
			ispblk_fbc_clear_fbcd_ring_base(&vdev->ctx);
		}
	}
}

static inline void _vi_wake_up_preraw_th(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num)
{
	unsigned long flags;
	struct _isp_raw_num_n *n;
	enum E_VI_TH th_id = E_VI_TH_PRERAW;

	n = osal_kzalloc(sizeof(*n), OSAL_GFP_ATOMIC);
	if (n == NULL) {
		vi_pr(VI_ERR, "pre_raw_num_q kmalloc size(%zu) fail\n", sizeof(*n));
		return;
	}
	n->raw_num = raw_num;

	osal_spin_lock_irqsave(&raw_num_lock, &flags);
	osal_list_add_tail(&n->list, &pre_raw_num_q.list);
	osal_spin_unlock_irqrestore(&raw_num_lock, &flags);

	osal_atomic_set(&vdev->vi_th[th_id].flag, ISP_PRERAW_MAX);
	osal_wait_wakeup(&vdev->vi_th[th_id].wq);
}

void vi_event_queue(struct sop_vi_dev *vdev, const u32 type, const u32 frm_num)
{
	unsigned long flags;
	struct vi_event_k *ev_k = NULL;
	struct isp_event_q *event_q;
	osal_timeval osal_tv;

	if (!vdev || type >= VI_EVENT_MAX) {
		vi_pr(VI_ERR, "Param invalid, vdev is NULL or type(%d) error\n", type);
		return;
	}

	event_q = &vdev->event_q;

	osal_spin_lock_irqsave(&event_q->lock, &flags);

	if (event_q->count < VI_MAX_LIST_NUM) {
		ev_k = osal_kzalloc(sizeof(*ev_k), OSAL_GFP_ATOMIC);
		if (ev_k) {
			event_q->count++;
		} else {
			vi_pr(VI_ERR, "event queue osal_kzalloc size(%zu) fail\n", sizeof(*ev_k));
			goto unlock;
		}
	} else if (!osal_list_empty(&event_q->list)) {
		ev_k = osal_list_first_entry(&event_q->list, struct vi_event_k, list);
		osal_list_del_init(&ev_k->list);
	}

	if (!ev_k) {
		vi_pr(VI_ERR, "Failed to get event node\n");
		goto unlock;
	}

	ev_k->ev.type = type;
	ev_k->ev.frame_sequence = frm_num;

	osal_gettimeofday(&osal_tv);
	ev_k->ev.pts = osal_tv.tv_sec * 1000000 + osal_tv.tv_usec;

	osal_list_add_tail(&ev_k->list, &event_q->list);

unlock:
	osal_spin_unlock_irqrestore(&event_q->lock, &flags);

	if (ev_k) {
		osal_wait_wakeup(&vdev->isp_event_wait_q);
	}
}

int user_pic_trig(struct sop_vi_dev *vdev, bool is_frm_rst)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	u8 pipe = 0;

	if (!osal_atomic_read(&vdev->stream.isp_streamon[pipe])) {
		vi_pr(VI_DBG, "isp stream is not on, can't trigger preraw\n");
		return 1;
	}

	if (ctx->is_rawreplay) {
		if (osal_atomic_read(&vdev->postraw_state) != ISP_STATE_IDLE)
			return 1;

		pipe = ctx->isp_csi_cfg[raw_num].bind_pipe[ISP_FE_CH0];

		isp_first_frm_reset(ctx, vdev->postraw_frame_number[pipe] == 0, false);

		_vi_clear_mmap_fbc_ring_base(vdev, pipe);

		vi_tuning_gamma_ips_update(ctx, pipe);
		vi_tuning_dci_update(ctx, pipe);
		vi_tuning_drc_update(ctx, pipe);

		ispblk_fbc_debug_info(ctx);

		_post_dma_update(vdev, pipe);

		_pre_hw_enque(vdev, raw_num, ISP_FE_CH0);

		_vi_wake_up_preraw_th(vdev, raw_num);

		vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0]++;

		if (!ctx->isp_csi_cfg[raw_num].is_yuv_sensor) {
			vi_event_queue(vdev, VI_EVENT_PRE0_EOF + ctx->isp_csi_cfg[raw_num].bind_pipe[ISP_FE_CH0],
					vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0]);
		}
	}
	return 0;
}

/* notice this function will be interrupted by ipcmsg,
 * so we need to check the param, insure the data is not NULL
 */
static void _usr_pic_timer_handler(unsigned long data)
{
	struct sop_vi_dev *vdev = NULL;

	if ((void *)data == NULL) {
		vi_pr(VI_WARN, "timer data is NULL\n");
		return;
	}

	vdev = (struct sop_vi_dev *)osal_timer_get_private_data((void *)data);

	if (user_pic_trig(vdev, false)) {
		goto EXIT;
	}

EXIT:
	vi_pr(VI_DBG, "wait next trig\n");

	if (vdev->usr_pic_timer.timer)
		osal_timer_mod(&vdev->usr_pic_timer, vdev->usr_pic_delay);
}

int usr_pic_time_remove(struct sop_vi_dev *vdev)
{
	int ret = 0;

	osal_timer_stop(&vdev->usr_pic_timer);

	ret = osal_timer_destroy_sync(&vdev->usr_pic_timer);
	if (ret) {
		vi_pr(VI_ERR, "osal_timer_destroy_sync fail\n");
		return ret;
	}

	vdev->usr_pic_timer.timer = NULL;
	vdev->usr_pic_timer.handler = NULL;

	return ret;
}

int usr_pic_timer_init(struct sop_vi_dev *vdev)
{
	int ret = 0;

	memset(&vdev->usr_pic_timer, 0, sizeof(vdev->usr_pic_timer));
	vdev->usr_pic_timer.handler = _usr_pic_timer_handler;
	vdev->usr_pic_timer.data = (uintptr_t)vdev;
	vdev->usr_pic_timer.interval = vdev->usr_pic_delay ? vdev->usr_pic_delay : 30;

	ret = osal_timer_init(&vdev->usr_pic_timer);
	if (ret) {
		vi_pr(VI_ERR, "osal_timer_init fail\n");
		return ret;
	}

	return ret;
}

int usr_pic_timer_start(struct sop_vi_dev *vdev)
{
	int ret = 0;

	vdev->usr_pic_timer.interval = vdev->usr_pic_delay;

	ret = osal_timer_start(&vdev->usr_pic_timer);
	if (ret) {
		vi_pr(VI_ERR, "osal_timer_start fail\n");
		return ret;
	}

	return ret;
}

void sop_isp_dqbuf_list(struct sop_vi_dev *vdev, const u32 frm_num,
			const u8 pipe_id, const u8 chn_id, osal_timeval tv)
{
	unsigned long flags;
	struct _isp_dqbuf_n *n;

	n = osal_kzalloc(sizeof(struct _isp_dqbuf_n), OSAL_GFP_ATOMIC);
	if (n == NULL) {
		vi_pr(VI_ERR, "DQbuf kmalloc size(%zu) fail\n", sizeof(struct _isp_dqbuf_n));
		return;
	}
	n->pipe_id	= pipe_id;
	n->chn_id	= chn_id;
	n->frm_num	= frm_num;
	n->tv		= tv;

	osal_spin_lock_irqsave(&dq_lock, &flags);
	osal_list_add_tail(&n->list, &dqbuf_q.list);
	osal_spin_unlock_irqrestore(&dq_lock, &flags);
}

int vi_dqbuf(struct _vi_buffer *b)
{
	unsigned long flags;
	struct _isp_dqbuf_n *n = NULL;
	int ret = -1;

	osal_spin_lock_irqsave(&dq_lock, &flags);
	if (!osal_list_empty(&dqbuf_q.list)) {
		n = osal_list_first_entry(&dqbuf_q.list, struct _isp_dqbuf_n, list);
		b->pipe_id		= n->pipe_id;
		b->chn_id		= n->chn_id;
		b->sequence		= n->frm_num;
		b->tv			= n->tv;
		osal_list_del_init(&n->list);
		osal_kfree(n);
		ret = 0;
	}
	osal_spin_unlock_irqrestore(&dq_lock, &flags);

	return ret;
}

static int _vi_call_cb(u32 m_id, u32 cmd_id, void *data)
{
	struct base_exe_m_cb exe_cb;

	exe_cb.callee = m_id;
	exe_cb.caller = E_MODULE_VI;
	exe_cb.cmd_id = cmd_id;
	exe_cb.data   = (void *)data;

	return base_exe_module_cb(&exe_cb);
}

static int _isp_yuv_bypass_buf_enq(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const u8 hw_chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct sop_isp_buf *b = NULL;
	enum isp_blk_id_t dmaid;
	u64 tmp_addr = 0, i = 0;
	u8 pipe = ctx->isp_csi_cfg[raw_num].bind_pipe[hw_chn_num];
	u8 chn = 0;

	if (sop_isp_rdy_buf_empty(vdev, pipe, chn)) {
		vi_pr(VI_WARN, "fe_%d chn_num_%d yuv bypass outbuf is empty\n", raw_num, hw_chn_num);
		return 0;
	}

	sop_isp_rdy_buf_pop(vdev, pipe, chn);
	b = sop_isp_rdy_buf_next(vdev, pipe, chn);
	if (b == NULL)
		return 0;

	vi_pr(VI_DBG, "update yuv bypass outbuf: 0x%llx pipe_%d chn(%d)\n",
			b->buf.planes[0].addr, pipe, chn);

	dmaid = csibdg_dma_find_hwid(raw_num, hw_chn_num);

	if (ctx->isp_csi_cfg[raw_num].is_422_to_420) {
		for (i = 0; i < 2; i++) {
			tmp_addr = b->buf.planes[i].addr;
			if (vdev->pre_fe_frm_num[raw_num][hw_chn_num] == 0)
				ispblk_dma_yuv_bypass_config(ctx, dmaid + i, tmp_addr, raw_num);
			else
				ispblk_dma_setaddr(ctx, dmaid + i, tmp_addr);
		}
	} else {
		tmp_addr = b->buf.planes[0].addr;

		if (vdev->pre_fe_frm_num[raw_num][hw_chn_num] == 0)
			ispblk_dma_yuv_bypass_config(ctx, dmaid, tmp_addr, raw_num);
		else
			ispblk_dma_setaddr(ctx, dmaid, tmp_addr);
	}

	return 1;
}

static void _isp_yuv_bypass_trigger(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num, const u8 hw_chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;

	if (osal_atomic_read(&vdev->isp_err_handle_flag) == 1)
		return;

	if (osal_atomic_read(&vdev->stream.csi_streamon[raw_num])) {
		if (osal_atomic_cmpxchg(&vdev->pre_fe_state[raw_num][hw_chn_num],
					ISP_STATE_IDLE, ISP_STATE_RUNNING) ==
					ISP_STATE_RUNNING) {
			vi_pr(VI_DBG, "fe_%d chn_num_%d is running\n", raw_num, hw_chn_num);
			return;
		}

		if (_isp_yuv_bypass_buf_enq(vdev, raw_num, hw_chn_num))
			isp_pre_trig(ctx, raw_num, hw_chn_num);
		else {
			osal_atomic_set(&vdev->pre_fe_state[raw_num][hw_chn_num], ISP_STATE_IDLE);
		}
	}
}

void _vi_postraw_ctrl_setup(struct sop_vi_dev *vdev, uint8_t pipe)
{
	struct isp_ctx *ctx = &vdev->ctx;

	if (!ctx->isp_pipe_cfg[pipe].is_enable)
		return;

	if (ctx->isp_pipe_cfg[pipe].is_yuv_sensor && ctx->isp_pipe_cfg[pipe].yuv_scene_mode == ISP_YUV_SCENE_BYPASS)
		return;

	_isp_rawtop_init(vdev);
	_isp_rgbtop_init(vdev);
	_isp_yuvtop_init(vdev);

	ispblk_preraw_vi_sel_config(ctx);
}

void _vi_pre_fe_ctrl_setup(struct sop_vi_dev *vdev, enum sop_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct cif_yuv_swap_s swap = {0};

	if (ctx->isp_csi_cfg[raw_num].is_yuv_sensor) {//YUV sensor
		if (ctx->isp_csi_cfg[raw_num].is_422_to_420) {//uyvy to yuyv to 420
			swap.devno = raw_num;
			swap.yc_swap = 1;
			swap.uv_swap = 1;
			_vi_call_cb(E_MODULE_CIF, MIPI_SET_YUV_SWAP, &swap);
		}

		if (ctx->isp_csi_cfg[raw_num].is_bt_demux)
			ispblk_csibdg_lite_config(ctx, raw_num);
		else {
			ispblk_csibdg_yuv_bypass_config(ctx, raw_num);
		}

		ispblk_isptop_fe_config(ctx, raw_num, false);
		ispblk_preraw_fe_config(ctx, raw_num);
		ispblk_csibdg_crop_update(ctx, raw_num, true);
		ispblk_clsc_config(ctx, raw_num, ISP_FE_CH0, false);
		ispblk_clsc_config(ctx, raw_num, ISP_FE_CH1, false);
		ispblk_aehist_config(ctx, raw_num, false);
	} else { //RGB sensor
		ispblk_isptop_fe_config(ctx, raw_num, true);
		ispblk_preraw_fe_config(ctx, raw_num);
		ispblk_csibdg_config(ctx, raw_num);
		ispblk_csibdg_crop_update(ctx, raw_num, !ctx->isp_csi_cfg[raw_num].is_mux_dev);
		ispblk_csibdg_wdma_crop_config(ctx, raw_num, ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0],
							ctx->isp_csi_cfg[raw_num].is_mux_dev);
		ispblk_clsc_config(ctx, raw_num, ISP_FE_CH0, false);
		if (ctx->isp_csi_cfg[raw_num].is_hdr_on)
			ispblk_clsc_config(ctx, raw_num, ISP_FE_CH1, false);
		ispblk_aehist_config(ctx, raw_num, true);
	}
}

void _vi_csi_ctrl_init(struct sop_vi_dev *vdev, enum sop_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw phy_raw = raw_num;
	u8 i = 0;
	struct cif_attr_s cif_attr;

	if (!ctx->isp_csi_cfg[raw_num].is_patgen_en) {
		phy_raw = find_phy_raw_num(ctx, raw_num);
		/* Get stagger vsync info from cif */
		cif_attr.devno = phy_raw;
		if (_vi_call_cb(E_MODULE_CIF, CIF_CB_GET_CIF_ATTR, &cif_attr) == 0) {
			ctx->isp_csi_cfg[raw_num].is_stagger_vsync = cif_attr.stagger_vsync;

			ctx->isp_csi_cfg[raw_num].csibdg_width  = cif_attr.img_size.width;
			ctx->isp_csi_cfg[raw_num].csibdg_height = cif_attr.img_size.height;
			ctx->isp_csi_cfg[raw_num].max_width     = cif_attr.img_size.max_width;
			ctx->isp_csi_cfg[raw_num].max_height    = cif_attr.img_size.max_height;

			for (i = 0; i < ctx->isp_csi_cfg[raw_num].chn_num; i++) {
				ctx->isp_csi_cfg[raw_num].crop[i].w = cif_attr.img_size.active_w;
				ctx->isp_csi_cfg[raw_num].crop[i].h = cif_attr.img_size.active_h;
				ctx->isp_csi_cfg[raw_num].crop[i].x = cif_attr.img_size.start_x;
				ctx->isp_csi_cfg[raw_num].crop[i].y = cif_attr.img_size.start_y;
			}

			ctx->isp_csi_cfg[raw_num].rgb_color_mode_pre_crop =
						bayer_type_before_crop(ctx->isp_csi_cfg[raw_num].rgb_color_mode,
								cif_attr.img_size.start_x, cif_attr.img_size.start_y);
		} else {
			vi_pr(VI_ERR, "get cif attr fail\n");
			return;
		}
	}

	if (ctx->cfg_info.raw_num == ISP_PRERAW_MAX && !ctx->isp_csi_cfg[raw_num].is_yuv_sensor) {
		//Postraw out size
		ctx->cfg_info.raw_num = raw_num;
	}

	if (_is_fe_post_slice(ctx))
		vi_calculate_slice_buf_setting(ctx, raw_num);

	vi_pr(VI_DBG, "raw_num(%d) is_hdr(%d) is_yuv_sensor(%d) mux_mode(%d)\n", raw_num,
							ctx->isp_csi_cfg[raw_num].is_hdr_on,
							ctx->isp_csi_cfg[raw_num].is_yuv_sensor,
							ctx->isp_csi_cfg[raw_num].mux_mode);
	vi_pr(VI_INFO, "color_mode(%d) csibdg(%d:%d) max(%d:%d)\n",
		ctx->isp_csi_cfg[raw_num].rgb_color_mode,
		ctx->isp_csi_cfg[raw_num].csibdg_width, ctx->isp_csi_cfg[raw_num].csibdg_height,
		ctx->isp_csi_cfg[raw_num].max_width, ctx->isp_csi_cfg[raw_num].max_height);
}

void _vi_isp_ctrl_init(struct sop_vi_dev *vdev, uint8_t pipe)
{
	struct isp_ctx *ctx = &vdev->ctx;
	uint8_t	raw_num = ctx->isp_pipe_cfg[pipe].bind_raw;
	uint8_t chn = 0;

	ctx->isp_pipe_cfg[pipe].is_enable = true;

	//TODO need know which chn is bind to pipe
	osal_memcpy(&ctx->isp_pipe_cfg[pipe].crop, &ctx->isp_csi_cfg[raw_num].crop[chn], sizeof(struct vi_rect));
	ctx->isp_pipe_cfg[pipe].yuv_scene_mode = ctx->isp_csi_cfg[raw_num].yuv_scene_mode;
	ctx->isp_pipe_cfg[pipe].is_yuv_sensor = ctx->isp_csi_cfg[raw_num].is_yuv_sensor;

	if (ctx->is_rawreplay) {
		ctx->isp_pipe_cfg[pipe].crop.w = vdev->usr_crop.width;
		ctx->isp_pipe_cfg[pipe].crop.h = vdev->usr_crop.height;
		ctx->isp_pipe_cfg[pipe].crop.x = vdev->usr_crop.left;
		ctx->isp_pipe_cfg[pipe].crop.y = vdev->usr_crop.top;

		ctx->isp_pipe_cfg[pipe].crop_se.w = vdev->usr_crop.width;
		ctx->isp_pipe_cfg[pipe].crop_se.h = vdev->usr_crop.height;
		ctx->isp_pipe_cfg[pipe].crop_se.x = vdev->usr_crop.left;
		ctx->isp_pipe_cfg[pipe].crop_se.y = vdev->usr_crop.top;

		ctx->isp_pipe_cfg[pipe].rgb_color_mode = bayer_type_mapping(vdev->usr_fmt.code);
	}

	ctx->isp_pipe_cfg[pipe].post_img_w = ctx->isp_pipe_cfg[pipe].crop.w;
	ctx->isp_pipe_cfg[pipe].post_img_h = ctx->isp_pipe_cfg[pipe].crop.h;
	ctx->isp_pipe_cfg[pipe].rgb_color_mode = ctx->isp_csi_cfg[raw_num].rgb_color_mode;

	//cnr config
	ctx->isp_pipe_cfg[pipe].cnr_scale_shift = (ctx->isp_pipe_cfg[pipe].crop.w > 2688) ? 3 : 2;
	ctx->isp_pipe_cfg[pipe].cnr_cur_scale_shift =
			ctx->isp_pipe_cfg[pipe].cnr_pre_scale_shift = ctx->isp_pipe_cfg[pipe].cnr_scale_shift;
	ctx->isp_pipe_cfg[pipe].is_hdr_on = ctx->isp_csi_cfg[raw_num].is_hdr_on;

	if (ctx->cfg_info.pipe == VI_MAX_PIPE_NUM) {
		//Postraw out size
		ctx->cfg_info.is_hdr_on = ctx->isp_pipe_cfg[pipe].is_hdr_on = ctx->isp_csi_cfg[raw_num].is_hdr_on;
		ctx->cfg_info.pipe = pipe;
		ctx->cfg_info.img_width = ctx->isp_pipe_cfg[pipe].crop.w;
		ctx->cfg_info.img_height = ctx->isp_pipe_cfg[pipe].crop.h;

		vi_pr(VI_INFO, "Rgb raw_num(%d) pipe(%d) init_w_h(%d:%d)\n", raw_num, ctx->cfg_info.pipe,
		      ctx->cfg_info.img_width, ctx->cfg_info.img_height);
	}

	vi_pr(VI_INFO, "pipe(%d) yuv((%d) crop_x_y_w_h(%d:%d:%d:%d), cnr_shift(%d)",
			pipe, ctx->isp_pipe_cfg[pipe].is_yuv_sensor,
			ctx->isp_pipe_cfg[pipe].crop.x, ctx->isp_pipe_cfg[pipe].crop.y,
			ctx->isp_pipe_cfg[pipe].crop.w, ctx->isp_pipe_cfg[pipe].crop.h,
			ctx->isp_pipe_cfg[pipe].cnr_scale_shift);
}

void _vi_scene_ctrl(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num = ISP_PRERAW0;

	if (ctx->is_ctrl_inited)
		return;

	if (_is_fe_post_offline(ctx)) { //Multi sensor scenario
		ctx->is_fbc_on = false;
		ctx->is_multi_sensor = true;
		if (ctx->is_rawreplay) { // dram->postraw
			ctx->is_slice_buf_on = false;
			ctx->is_fbc_on = true;
		}
	} else { //Single sensor onthefly or slicebuf
		raw_num = ctx->cfg_info.raw_num;
		ctx->is_multi_sensor = false;
		ctx->is_fbc_on = true;

		if (ctx->isp_csi_cfg[raw_num].is_yuv_sensor) { // yuv sensor
			ctx->isp_csi_cfg[raw_num].is_422_to_420 = false;

			if (ctx->isp_csi_cfg[raw_num].mux_mode > 0 &&
			    ctx->isp_csi_cfg[raw_num].inf_mode >= VI_MODE_BT656 &&
			    ctx->isp_csi_cfg[raw_num].inf_mode <= VI_MODE_BT1120_INTERLEAVED) {
				ctx->isp_csi_cfg[raw_num].is_bt_demux = true;
				if (ctx->isp_csi_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
					vi_pr(VI_WARN, "bt_demux sensor switch scene_mode to YUV_SCENE_ONLINE\n");
					ctx->isp_csi_cfg[raw_num].yuv_scene_mode = ISP_YUV_SCENE_ONLINE;
				}
			}
		}
	}

	ctx->is_ctrl_inited = true;

	vi_pr(VI_INFO, "is_3dnr_on[%d], is_fbc_on[%d]\n", ctx->is_3dnr_on, ctx->is_fbc_on);
}

void _vi_bw_cal_set(struct sop_vi_dev *vdev)
{
#ifdef BW_CAL
	struct isp_ctx *ctx = &vdev->ctx;
	struct sop_vi_ctx *vi_proc_ctx = NULL;
	void __iomem *bw_limiter;
	u64 bwladdr[ISP_BW_LIMIT_MAX] = {0x0A074020, 0x0A072020, 0x0A078020}; // rdma, wdma0, wdma1
	u32 data_size[ISP_BW_LIMIT_MAX] = {0}, BW[ISP_PRERAW_MAX][ISP_BW_LIMIT_MAX] = {0};
	u32 total_bw = 0, bwlwin = 0, bwltxn = 0, margin = 125, fps = 25;
	u32 def_bwltxn = 4, def_fps = 25;
	u32 width, height;
	u8 i, yuv_chn_num;
	enum sop_isp_raw raw_num = ISP_PRERAW0;

	vi_proc_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ctx->isp_csi_cfg[raw_num].is_enable)
			continue;

		fps = vi_proc_ctx->dev_attr[raw_num].snr_fps ?
			vi_proc_ctx->dev_attr[raw_num].snr_fps :
			def_fps;
		width = ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].w;
		height = ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].h;

		if (!ctx->isp_csi_cfg[raw_num].is_yuv_sensor) {//RGB sensor
			if (!ctx->isp_csi_cfg[raw_num].is_hdr_on) {
				data_size[ISP_BW_LIMIT_RDMA] = (411 * width * height) / 128 + 50052;
				data_size[ISP_BW_LIMIT_WDMA0] = (396 * width * height) / 128 + 8160;
				data_size[ISP_BW_LIMIT_WDMA1] = (391 * width * height) / 128 + 21536;
			} else {
				data_size[ISP_BW_LIMIT_RDMA] = (630 * width * height) / 128 + 50052;
				data_size[ISP_BW_LIMIT_WDMA0] = (792 * width * height) / 128 + 8160;
				data_size[ISP_BW_LIMIT_WDMA1] = (394 * width * height) / 128 + 38496;
			}
		} else { //YUV sensor
			yuv_chn_num = ctx->isp_csi_cfg[raw_num].mux_mode + 1;
			data_size[ISP_BW_LIMIT_RDMA] = (192 * yuv_chn_num * width * height) / 128;
			data_size[ISP_BW_LIMIT_WDMA0] = (192 * yuv_chn_num * width * height) / 128;
			data_size[ISP_BW_LIMIT_WDMA1] = 0;
		}

		for (i = 0; i < ISP_BW_LIMIT_MAX; ++i) {
			BW[raw_num][i] = (fps * data_size[i]) / 1000000 + 1;
		}
	}

	// TODO
	// just restrain RDMA now, WDMA0/WDMA1 wait for Brian
	// for (i = 0; i < BW_LIMIT_MAX; ++i) {
	for (i = 0; i < 1; ++i) {
		total_bw = 0;

		for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
			total_bw += BW[raw_num][i];
		}

		for (bwltxn = def_bwltxn; bwltxn > 1; --bwltxn) {
			bwlwin = bwltxn * 256000 / ((((total_bw * 33) / 10) * margin) / 100);
			if (bwlwin <= 1024)
				break;
		}

		bw_limiter = osal_ioremap(bwladdr[i], 0x4);
		_reg_write(bw_limiter, ((bwltxn << 10) | bwlwin));
		vi_pr(VI_INFO, "isp %s bw_limiter=0x%x, BW=%d, bwltxn=%d, bwlwin=%d\n",
				(i == 0) ? "rdma" : ((i == 1) ? "wdma0" : "wdma1"),
				_reg_read(bw_limiter), total_bw, bwltxn, bwlwin);
		osal_iounmap(bw_limiter, 0x4);
	}
#endif
}

static int _vi_mux_dev_gpio_init(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	int rc = 0;
	u8 i = 0, j = 0;
	u8 cur_idx = 0;
	char label[16] = {0};
	struct csi_gpio *gpio = NULL;
	struct _csi_switch_info *info = &ctx->isp_csi_cfg[raw_num].switch_info;

	if (!ctx->isp_csi_cfg[raw_num].is_mux_dev)
		return 0;

	cur_idx = info->cur_idx;

	for (i = 0; i < VI_MAX_DEV_SWITCH_DEPTH; i++) {
		gpio = &info->snr_arr[0].gpio[i];
		if (!gpio->enable)
			continue;
		sprintf(label, "gpio%d(%d, %d)", i, gpio->port, gpio->pin);
		rc = vi_misc_gpio_request(&gpio->handle, gpio->port, gpio->pin, label);
		if (rc) {
			vi_pr(VI_ERR, "request for gpio(%d) failed:%d\n", gpio->pin, rc);
			goto err_gpio_req;
		}

		rc = vi_misc_gpio_dir(&gpio->handle, gpio->port, gpio->pin, 1, gpio->pol);
		if (rc) {
			vi_pr(VI_ERR, "fail to set gpio(%d) dir failed:%d\n",
				gpio->pin, rc);
			goto err_gpio_req;
		}
	}

	return rc;

err_gpio_req:
	for (j = 0; j < i; j++) {
		gpio = &info->snr_arr[cur_idx].gpio[i];
		if (!gpio->enable)
			continue;
		vi_misc_gpio_free(&gpio->handle, gpio->port, gpio->pin);
	}

	return rc;
}

static int _vi_mux_dev_gpio_deinit(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	u8 i = 0;
	u8 cur_idx = 0;
	struct csi_gpio *gpio = NULL;
	struct _csi_switch_info *info = &ctx->isp_csi_cfg[raw_num].switch_info;

	if (!ctx->isp_csi_cfg[raw_num].is_mux_dev)
		return 0;

	for (i = 0; i < VI_MAX_DEV_SWITCH_DEPTH; i++) {
		gpio = &info->snr_arr[cur_idx].gpio[i];
		if (!gpio->enable)
			continue;
		vi_misc_gpio_free(&gpio->handle, gpio->port, gpio->pin);
	}

	return 0;
}

int clean_misc_resources(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct _isp_dqbuf_n *n = NULL, *dq_tmp = NULL;
	struct _isp_raw_num_n *raw_n = NULL, *raw_tmp = NULL;
	struct vi_event_k *ev_k = NULL, *ev_tmp = NULL;
	struct isp_event_q *event_q = &vdev->event_q;
	unsigned long flags;
	struct isp_buffer *isp_b;
	uint8_t i = 0, j = 0;
	union vi_sys_reset_apb reset_apb;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	vi_pr(VI_DBG, "+\n");

	ctx->is_rawreplay = false;

	usr_pic_time_remove(vdev);
	isp_raw_dump_deinit(vdev);

	osal_spin_lock_irqsave(&dq_lock, &flags);
	osal_list_for_each_entry_safe(n, dq_tmp, &dqbuf_q.list, list) {
		osal_list_del_init(&n->list);
		osal_kfree(n);
	}
	osal_spin_unlock_irqrestore(&dq_lock, &flags);

	osal_spin_lock_irqsave(&event_q->lock, &flags);
	osal_list_for_each_entry_safe(ev_k, ev_tmp, &event_q->list, list) {
		osal_list_del_init(&ev_k->list);
		osal_kfree(ev_k);
	}
	event_q->count = 0;
	osal_spin_unlock_irqrestore(&event_q->lock, &flags);

	osal_spin_lock_irqsave(&raw_num_lock, &flags);
	osal_list_for_each_entry_safe(raw_n, raw_tmp, &pre_raw_num_q.list, list) {
		osal_list_del_init(&raw_n->list);
		osal_kfree(raw_n);
	}
	osal_spin_unlock_irqrestore(&raw_num_lock, &flags);

	for (i = 0; i < ISP_RAW_PATH_MAX; i++) {
		while ((isp_b = isp_buf_remove(&vdev->postraw_wdr_in_q[i])) != NULL) {
			if (isp_b->is_ext == EXTERNAL_BUFFER) {
				_vi_release_ext_buf(isp_b->addr);
				osal_kfree(isp_b);
			} else {
				osal_vfree(isp_b);
			}
		}
	}

	while ((isp_b = isp_buf_remove(&vdev->postraw_in_q)) != NULL) {
		if (isp_b->is_ext == EXTERNAL_BUFFER) {
			_vi_release_ext_buf(isp_b->addr);
			osal_kfree(isp_b);
		} else {
			osal_vfree(isp_b);
		}
	}

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		osal_memset(&ctx->isp_csi_cfg[i], 0, sizeof(struct _csi_cfg));
		osal_spin_lock_destroy(&ctx->csi_bufpool[i].pre_fe_sts_lock);
		osal_wait_destroy(&vdev->isp_int_wait_q[i]);
		osal_wait_destroy(&vdev->isp_ai_wait_q[i]);

		for (j = 0; j < ISP_FE_CHN_MAX; j++) {
			isp_buf_destroy(&vdev->pre_fe_out_q[i][j]);
			isp_buf_destroy(&vdev->pre_ai_isp_in_q[i][j]);
			isp_buf_destroy(&vdev->pre_ai_isp_out_q[i][j]);
			isp_buf_destroy(&vdev->raw_dump[i].buf_q[j]);
			isp_buf_destroy(&vdev->raw_dump[i].buf_dq[j]);
		}
	}

	for (i = 0; i < VI_MAX_PIPE_NUM; i++) {
		osal_memset(&ctx->isp_pipe_cfg[i], 0, sizeof(struct _isp_cfg));
		ctx->isp_pipe_cfg[i].is_offline_scaler = true;

		for (j = 0; j < VI_MAX_CHN_NUM; j++) {
			osal_mutex_destroy(&vdev->mesh[i][j].lock);
		}

		osal_spin_lock_destroy(&ctx->isp_bufpool[i].post_sts_lock);
		if (vi_ctx->pipe_lock[i].mutex)
			osal_mutex_destroy(&vi_ctx->pipe_lock[i]);
	}

	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (vi_ctx->dev_lock[i].mutex)
			osal_mutex_destroy(&vi_ctx->dev_lock[i]);
	}

	for (i = 0; i < ISP_RAW_PATH_MAX; i++) {
		isp_buf_destroy(&vdev->postraw_wdr_in_q[i]);
	}

	isp_buf_destroy(&vdev->postraw_in_q);

	isp_reset(ctx);

	reset_apb.raw = 0;
	reset_apb.b.isp_top = 1;
	vi_sys_toggle_reset_apb(reset_apb);

	osal_atomic_set(&vdev->stream.isp_init, 0);

	vi_pr(VI_DBG, "-\n");

	return 0;
}

int vi_csi_start_streaming(struct sop_vi_dev *vdev, uint8_t raw_num)
{
	int ret = 0;
	struct isp_ctx *ctx = &vdev->ctx;
	struct cif_attr_s cif_attr;
	uint8_t total_chn = (ctx->isp_csi_cfg[raw_num].is_yuv_sensor)
			? ctx->isp_csi_cfg[raw_num].mux_mode + 1
			: ctx->isp_csi_cfg[raw_num].is_hdr_on + 1;
	uint8_t chn_str = 0, pipe = 0;

	if (osal_atomic_read(&vdev->stream.csi_streamon[raw_num])) {
		vi_pr(VI_DBG, "raw_num(%d) is already streaming\n", raw_num);
		return 0;
	}

	ret = vi_get_csi_ion_buf(vdev, raw_num);
	if (ret) {
		vi_pr(VI_ERR, "get csi ion buf failed\n");
		return ERR_VI_NOMEM;
	}

	_vi_mempool_reset(&ctx->csi_mempool[raw_num]);

	_isp_pre_fe_dma_setup(vdev, raw_num);

	if (raw_num >= ISP_PRERAW_VIRT0) {
		vi_pr(VI_DBG, "virtual raw should not set hw\n");
		goto skip_hw_setup;
	}

	//TODO: workaround for csibdg reset issue, but need to check if it is necessary
	ispblk_csibdg_reset(ctx, raw_num);

	//sw workaround to disable csibdg enable first due to csibdg enable is on as default.
	isp_streaming(ctx, false, raw_num);

	if (ctx->is_rawreplay)
		return 0;

	if (ctx->is_ai_isp) {
		ret = vi_create_thread(vdev, E_VI_TH_AI_ISP);
		if (ret) {
			vi_pr(VI_ERR, "Failed to create ai_isp thread\n");
			goto err_create_ai_isp;
		}
	}

	_vi_call_cb(E_MODULE_CIF, CIF_CB_RESET_LVDS, &raw_num);

	/* Get stagger vsync info from cif */
	cif_attr.devno = raw_num;
	if (_vi_call_cb(E_MODULE_CIF, CIF_CB_GET_CIF_ATTR, &cif_attr) == 0)
		ctx->isp_csi_cfg[raw_num].is_stagger_vsync = cif_attr.stagger_vsync;

	ret = _vi_mux_dev_gpio_init(ctx, raw_num);
	if (ret) {
		vi_pr(VI_ERR, "fail to init gpio\n");
		goto fail_to_init_gpio;
	}

	_vi_pre_fe_ctrl_setup(vdev, raw_num);
	_isp_pre_ai_isp_dma_setup(vdev, raw_num);

	if (ctx->isp_csi_cfg[raw_num].is_yuv_sensor) {
		if (ctx->isp_csi_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			pipe = ctx->isp_csi_cfg[raw_num].bind_pipe[ISP_FE_CH0];
			if (ctx->isp_pipe_cfg[pipe].is_offline_scaler) {
				vi_pr(VI_INFO, "raw[%d] pipe[%d] YUV ISP & offline scaler\n", raw_num, pipe);
			} else {
				vi_pr(VI_INFO, "raw[%d] pipe[%d] YUV ISP & online scaler\n", raw_num, pipe);
			}
		} else {
			vi_pr(VI_INFO, "raw_num [%d] YUV bypass ISP\n", raw_num);
		}
	}

	_vi_dma_set_sw_mode(ctx, raw_num, true);

	if (_is_fe_post_offline(ctx)) {
		for (; chn_str < total_chn; chn_str++)
			isp_pre_trig(ctx, raw_num, chn_str);
	}

skip_hw_setup:
	osal_atomic_set(&vdev->stream.csi_streamon[raw_num], 1);

	return ret;

fail_to_init_gpio:
	vi_destroy_thread(vdev, E_VI_TH_AI_ISP);
err_create_ai_isp:
	vi_free_ion_buf(vdev, &ctx->csi_mempool[raw_num]);

	return ret;
}

int vi_csi_stop_streaming(struct sop_vi_dev *vdev, uint8_t raw_num)
{
	int ret = 0;
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_buffer *isp_b;
	struct _isp_snr_i2c_node *i2c_n = NULL, *i2c_tmp = NULL;
	struct _isp_crop_node *crop_n = NULL, *crop_tmp = NULL;
	unsigned long flags;
	int count = 10;
	int j = 0;

	if (ctx->is_rawreplay) {
		vi_pr(VI_DBG, "raw_num(%d) is raw replay, no need to stop streaming\n", raw_num);
		return 0;
	}

	osal_atomic_set(&vdev->stream.csi_streamon[raw_num], 0);

	while (--count > 0) {
		if (osal_atomic_read(&vdev->pre_fe_state[raw_num][ISP_FE_CH0]) == ISP_STATE_IDLE &&
			osal_atomic_read(&vdev->pre_fe_state[raw_num][ISP_FE_CH1]) == ISP_STATE_IDLE &&
			osal_atomic_read(&vdev->pre_fe_state[raw_num][ISP_FE_CH2]) == ISP_STATE_IDLE &&
			osal_atomic_read(&vdev->pre_fe_state[raw_num][ISP_FE_CH3]) == ISP_STATE_IDLE)
			break;
		vi_pr(VI_DBG, "wait raw_num(%d) count(%d)\n", raw_num, count);
#ifdef FPGA_PORTING
		osal_msleep(500);
#else
		osal_msleep(20);
#endif
	}

	if (count == 0) {
		vi_pr(VI_WARN, "raw_num(%d) stop streaming timeout\n", raw_num);
	}

	isp_streaming(ctx, false, raw_num);

	for (j = 0; j < ISP_FE_CHN_MAX; j++) {
		while ((isp_b = isp_buf_remove(&vdev->pre_fe_out_q[raw_num][j])) != NULL)
			osal_vfree(isp_b);
		while ((isp_b = isp_buf_remove(&vdev->pre_ai_isp_in_q[raw_num][j])) != NULL)
			osal_vfree(isp_b);
		while ((isp_b = isp_buf_remove(&vdev->pre_ai_isp_out_q[raw_num][j])) != NULL)
			osal_vfree(isp_b);
		while ((isp_b = isp_buf_remove(&vdev->raw_dump[raw_num].buf_dq[j])) != NULL)
			osal_vfree(isp_b);
		while ((isp_b = isp_buf_remove(&vdev->raw_dump[raw_num].buf_q[j])) != NULL)
			osal_vfree(isp_b);
	}

	osal_spin_lock_irqsave(&snr_node_lock[raw_num], &flags);
	osal_list_for_each_entry_safe(i2c_n, i2c_tmp, &isp_snr_i2c_queue[raw_num].list, list) {
		osal_list_del_init(&i2c_n->list);
		osal_kfree(i2c_n);
	}
	isp_snr_i2c_queue[raw_num].num_rdy = 0;

	osal_list_for_each_entry_safe(crop_n, crop_tmp, &isp_crop_queue[raw_num].list, list) {
		osal_list_del_init(&crop_n->list);
		osal_kfree(crop_n);
	}
	isp_crop_queue[raw_num].num_rdy = 0;
	osal_spin_unlock_irqrestore(&snr_node_lock[raw_num], &flags);

	_vi_mux_dev_gpio_deinit(ctx, raw_num);

	if (ctx->is_ai_isp) {
		vi_destroy_thread(vdev, E_VI_TH_AI_ISP);
	}

	ret = vi_free_ion_buf(vdev, &ctx->csi_mempool[raw_num]);
	if (ret != 0) {
		vi_pr(VI_ERR, "free ion buf failed\n");
		return ret;
	}

	return 0;
}

/*start postraw*/
int vi_isp_start_streaming(struct sop_vi_dev *vdev, uint8_t pipe, uint8_t chn)
{
	bool trig_pre = true;
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num = ctx->isp_pipe_cfg[pipe].bind_raw;
	bool stream_on = true;

	if (osal_atomic_read(&vdev->stream.isp_streamon[pipe])) {
		vi_pr(VI_DBG, "pipe %d chn %d already start streaming\n", pipe, chn);
		return 0;
	}

	osal_atomic_set(&vdev->stream.isp_streamon[pipe], 1);

	ctx->isp_pipe_cfg[pipe].chn_enable |= BIT(chn);

	vi_tuning_buf_setup(ctx, pipe);
	vi_tuning_buf_clear(pipe);

	_vi_mempool_reset(&ctx->isp_mempool[pipe]);

	_vi_dma_setup(vdev, pipe);

	if (!osal_atomic_read(&vdev->stream.isp_init)) {
		ispblk_isptop_config(ctx);
		_vi_dma_set_sw_mode(ctx, raw_num, false);
		_vi_postraw_ctrl_setup(vdev, pipe);
		_swap_post_sts_buf(ctx, pipe);
		_post_dma_update(vdev, pipe);
	}

	if (_is_all_online(ctx) || _is_fe_post_slice(ctx)) {
		if (ctx->isp_pipe_cfg[pipe].is_offline_scaler) { //offline mode
			_postraw_outbuf_enq(vdev, pipe, chn);
		} else { //online mode
			struct vpss_online_cb_info post_para = {0};

			/* VI Online VPSS sc cb trigger */
			post_para.snr_num = pipe;
			post_para.is_tile = false;
			osal_gettimeofday(&post_para.ts);
			if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_VI_ONLINE_TRIGGER, &post_para) != 0) {
				vi_pr(VI_INFO, "snr_num_%d, sc_%d is not ready. try later\n", raw_num, pipe);
				trig_pre = false;
			}
		}

		if (!ctx->is_rawreplay && trig_pre) {
			if (_is_fe_post_slice(ctx)) {
				vi_record_post_trigger(vdev, pipe);
				isp_post_trig(ctx);
			}

			isp_first_frm_reset(ctx, true, false);
			ctx->isp_pipe_cfg[pipe].first_frm_rst = false;

			if (!ctx->isp_csi_cfg[raw_num].is_yuv_sensor) { //RGB sensor
				isp_pre_trig(ctx, raw_num, ISP_FE_CH0);
				if (ctx->isp_csi_cfg[raw_num].is_hdr_on)
					isp_pre_trig(ctx, raw_num, ISP_FE_CH1);
			}

			if (_is_all_online(ctx))
				osal_atomic_set(&vdev->postraw_state, ISP_STATE_RUNNING);
			else {
				osal_atomic_set(&vdev->pre_fe_state[raw_num][ISP_FE_CH0], ISP_STATE_RUNNING);
				if (ctx->isp_csi_cfg[raw_num].is_hdr_on)
					osal_atomic_set(&vdev->pre_fe_state[raw_num][ISP_FE_CH1], ISP_STATE_RUNNING);
			}
		}
	} else {
		if (ctx->isp_csi_cfg[raw_num].is_yuv_sensor &&
			ctx->isp_csi_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_BYPASS) { //vi vpss offline mode
			u8 chn_str = 0, pipe_num;

			//for yuv sensor, we need to enque all chn_str
			for (; chn_str < ctx->isp_csi_cfg[raw_num].mux_mode + 1; chn_str++) {
				pipe_num = ctx->isp_csi_cfg[raw_num].bind_pipe[chn_str];
				if (!osal_atomic_read(&vdev->stream.isp_streamon[pipe_num])) {
					stream_on = false;
					break;
				}
			}

			if (stream_on) {
				for (chn_str = 0; chn_str < ctx->isp_csi_cfg[raw_num].mux_mode + 1; chn_str++)
					_isp_yuv_bypass_buf_enq(vdev, raw_num, chn_str);
			}
		}
	}

	if (osal_atomic_read(&vdev->stream.isp_init) == 0) {
		osal_atomic_set(&vdev->stream.isp_init, 1);
	}

	isp_streaming(ctx, stream_on, raw_num);

	vi_pr(VI_INFO, "pipe %d chn %d start streaming\n", pipe, chn);

	return 0;
}


int vi_isp_stop_streaming(struct sop_vi_dev *vdev, uint8_t pipe, uint8_t chn)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct sop_isp_buf *sop_vb, *tmp;
	unsigned long flags;
	int count = 5;

	if (!ctx->isp_pipe_cfg[pipe].chn_enable) {
		vi_pr(VI_DBG, "pipe %d chn %d already stop streaming\n", pipe, chn);
		return 0;
	}

	ctx->isp_pipe_cfg[pipe].chn_enable &= ~BIT(chn);

	if (!ctx->isp_pipe_cfg[pipe].chn_enable) {
		osal_atomic_set(&vdev->stream.isp_streamon[pipe], 0);
	}

	while (--count > 0) {
		if (osal_atomic_read(&vdev->postraw_state) == ISP_STATE_IDLE)
			break;
		vi_pr(VI_DBG, "wait pipe(%d) chn(%d) count(%d)\n", pipe, chn, count);
#ifdef FPGA_PORTING
		osal_msleep(500);
#else
		osal_msleep(20);
#endif
	}

	if (count == 0) {
		vi_pr(VI_WARN, "pipe(%d) chn(%d) stop streaming timeout\n", pipe, chn);
	}

	osal_spin_lock_irqsave(&vdev->qbuf_lock, &flags);
	osal_list_for_each_entry_safe(sop_vb, tmp, &(vdev->qbuf_list[pipe][chn]), list) {
		osal_kfree(sop_vb);
	}
	vdev->qbuf_num[pipe][chn] = 0;
	OSAL_INIT_LIST_HEAD(&vdev->qbuf_list[pipe][chn]);
	osal_spin_unlock_irqrestore(&vdev->qbuf_lock, &flags);

	vi_tuning_buf_release(ctx, pipe);

	vi_pr(VI_DBG, "done\n");

	return 0;
}

static int _pre_fe_outbuf_enque(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const enum sop_isp_fe_chn_num chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum isp_blk_id_t dmaid;
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, raw_num);
	struct isp_queue *fe_out_q = &vdev->pre_fe_out_q[phy_raw][chn_num];
	struct isp_buffer *b = NULL;
	bool is_dump_raw = false;

	dmaid = csibdg_dma_find_hwid(phy_raw, chn_num);

	if (osal_atomic_read(&vdev->raw_dump[raw_num].raw_dump_en[chn_num]) == RAWDUMP_START) {
		if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
			// For staggered sensor, dump raw only when both channels are ready
			if (vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0] == vdev->pre_fe_frm_num[raw_num][ISP_FE_CH1]) {
				osal_atomic_set(&vdev->raw_dump[raw_num].raw_dump_en[ISP_FE_CH0], RAWDUMP_PREPARE);
				osal_atomic_set(&vdev->raw_dump[raw_num].raw_dump_en[ISP_FE_CH1], RAWDUMP_PREPARE);
			}
		} else { //for linear sensor, dump raw directly
			osal_atomic_set(&vdev->raw_dump[raw_num].raw_dump_en[chn_num], RAWDUMP_PREPARE);
			is_dump_raw = true;
		}
	} else if (osal_atomic_read(&vdev->raw_dump[raw_num].raw_dump_en[chn_num]) == RAWDUMP_PREPARE) {
		is_dump_raw = true;
	} else if (osal_atomic_read(&vdev->raw_dump[raw_num].raw_dump_en[chn_num]) == RAWDUMP_DONE
			&& osal_atomic_read(&vdev->raw_dump[raw_num].isp_smooth_raw_dump_en) == SMOOTH_RAWDUMP_START) {
		is_dump_raw = true;
	}

	if (is_dump_raw) {
		fe_out_q = &vdev->raw_dump[raw_num].buf_q[chn_num];

		vi_pr(VI_DBG, "pre_fe raw_dump cfg start\n");

		b = isp_buf_next(fe_out_q);
		if (b == NULL) {
			vi_pr(VI_ERR, "Pre_fe_%d LE raw_dump outbuf is empty\n", raw_num);
			return 0;
		}

		ispblk_dma_setaddr(ctx, dmaid, b->addr);

		osal_atomic_set(&vdev->raw_dump[raw_num].raw_dump_en[chn_num], RAWDUMP_PREPARE_DONE);

		osal_atomic_set(&ctx->isp_csi_cfg[raw_num].clsc_en[chn_num],
					ispblk_clsc_is_enable(ctx, raw_num, chn_num));

		ispblk_clsc_config(ctx, raw_num, chn_num, false);
	} else {
		b = isp_buf_next(fe_out_q);
		if (!b) {
			vi_pr(VI_INFO, "pre_fe_%d chn_num_%d outbuf is empty\n", raw_num, chn_num);
			return 0;
		}

		ispblk_dma_setaddr(ctx, dmaid, b->addr);
	}

	return 1;
}

static int _postraw_inbuf_enq_check(
	struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_queue *post_in_q = NULL, *post_in_se_q = NULL;
	struct isp_buffer *b = NULL, *b_se = NULL;
	int ret = 0;

	post_in_q = ctx->is_hdr_on ? &vdev->postraw_wdr_in_q[ISP_FE_CH0] : &vdev->postraw_in_q;

retry:
	b = isp_buf_next(post_in_q);
	if (!b) {
		if (post_in_q == &vdev->postraw_wdr_in_q[ISP_FE_CH0]) {
			post_in_q = &vdev->postraw_in_q;
			goto retry;
		} else {
			vi_pr(VI_DBG, "postraw_in_q buf is empty\n");
		}
		return 1;
	}

	post_in_se_q = (post_in_q == &vdev->postraw_wdr_in_q[ISP_FE_CH0])
					? &vdev->postraw_wdr_in_q[ISP_FE_CH1]
					: NULL;
	if (post_in_se_q) {
		b_se = isp_buf_next(post_in_se_q);
		if (b_se == NULL) {
			post_in_q = &vdev->postraw_in_q;
			vi_pr(VI_DBG, "Pre_be se input syn buf is empty\n");
			goto retry;
		}

		if ((tuning_dis[0] > 0 && tuning_dis[0] - 1 != b_se->pipe) ||
			!ctx->isp_pipe_cfg[b_se->pipe].chn_enable) {
			b_se = isp_buf_remove(post_in_se_q);
			isp_buf_queue(&vdev->pre_fe_out_q[b_se->raw_num][b_se->chn_num], b_se);
			vi_pr(VI_DBG, "return se to fe_out_q\n");
		}
	}

	//wdr mode check se first
	if ((tuning_dis[0] > 0 && tuning_dis[0] - 1 != b->pipe) ||
		!ctx->isp_pipe_cfg[b->pipe].chn_enable) {
		b = isp_buf_remove(post_in_q);
		isp_buf_queue(&vdev->pre_fe_out_q[b->raw_num][b->chn_num], b);
		vi_pr(VI_DBG, "return le to fe_out_q\n");
		goto retry;
	}

	ctx->cfg_info.raw_num = b->raw_num;
	ctx->cfg_info.chn_num = b->chn_num;
	ctx->cfg_info.pipe = b->pipe;
	ctx->cfg_info.is_hdr_on = b_se ? true : false;
	ctx->cfg_info.is_yuv = false;

	ctx->isp_pipe_cfg[b->pipe].crop.x = b->crop.x;
	ctx->isp_pipe_cfg[b->pipe].crop.y = b->crop.y;
	ctx->isp_pipe_cfg[b->pipe].crop.w = ctx->cfg_info.img_width =
							ctx->isp_pipe_cfg[b->pipe].post_img_w;
	ctx->isp_pipe_cfg[b->pipe].crop.h = ctx->cfg_info.img_height =
							ctx->isp_pipe_cfg[b->pipe].post_img_h;

	ctx->isp_pipe_cfg[b->pipe].tv = b->tv;

	//YUV sensor, offline return error, online than config be read dma.
	if (ctx->isp_pipe_cfg[b->pipe].is_yuv_sensor) {
		ctx->cfg_info.is_yuv = true;
		if (ctx->isp_pipe_cfg[b->pipe].yuv_scene_mode == ISP_YUV_SCENE_BYPASS) {
			ret = 1;
		} else if (ctx->isp_pipe_cfg[b->pipe].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			vi_pr(VI_DBG, "YUV fe->dram->yuv post input buf_addr = 0x%llx\n", b->addr);
			ispblk_dma_yuv_bypass_config(ctx, ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_LE, b->addr, b->raw_num);
		}
		return ret;
	}

	ispblk_dma_config(ctx, b->pipe, ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_LE, b->addr);
	if (b_se)
		ispblk_dma_config(ctx, b->pipe, ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_SE, b_se->addr);

	vi_pr(VI_DBG, "next buf raw[%d]chn[%d] pipe(%d) w:h(%d:%d) addr(0x%llx)\n",
			ctx->cfg_info.raw_num, ctx->cfg_info.chn_num, ctx->cfg_info.pipe,
			ctx->cfg_info.img_width, ctx->cfg_info.img_height, b->addr);

	return ret;
}

static void _postraw_outbuf_enque(
	struct sop_vi_dev *vdev,
	const u8 pipe,
	const u8 chn_num)
{
	struct vi_buffer *vb2_buf;
	struct sop_isp_buf *b = NULL;
	struct isp_ctx *ctx = &vdev->ctx;
	u64 dma_addr = 0;
	u32 dma_id = 0;
	u8 i = 0;

	//Get the buffer for postraw output buffer
	b = sop_isp_rdy_buf_next(vdev, pipe, chn_num);
	if (b == NULL) {
		vi_pr(VI_DBG, "postraw_%d chn_%d next buffer is empty\n", pipe, chn_num);
		return;
	}

	vb2_buf = &b->buf;

	vi_pr(VI_DBG, "update isp-buf: 0x%llx-0x%llx\n",
		vb2_buf->planes[0].addr, vb2_buf->planes[1].addr);

	for (i = 0; i < 2; i++) {
		dma_addr = (uint64_t)vb2_buf->planes[i].addr;
		dma_id = (i == 0) ? ISP_BLK_ID_DMA_CTL_YUV_CROP_Y : ISP_BLK_ID_DMA_CTL_YUV_CROP_C;

		ispblk_dma_setaddr(ctx, dma_id, dma_addr);
	}
}

static u8 _postraw_outbuf_empty(
	struct sop_vi_dev *vdev,
	const u8 pipe,
	const u8 hw_chn_num)
{
	u8 ret = 0;

	if (sop_isp_rdy_buf_empty(vdev, pipe, hw_chn_num)) {
		vi_pr(VI_DBG, "postraw pipe_%d chn_%d output buffer is empty\n", pipe, hw_chn_num);
		ret = 1;
	}

	return ret;
}

void _postraw_outbuf_enq(
	struct sop_vi_dev *vdev,
	const u8 pipe,
	const u8 chn_num)
{
	sop_isp_rdy_buf_pop(vdev, pipe, chn_num);
	_postraw_outbuf_enque(vdev, pipe, chn_num);
}

/*
 * for postraw offline only.
 *  trig preraw if there is output buffer in preraw output.
 */
s8 _pre_hw_enque(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const enum sop_isp_fe_chn_num chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw phy_raw = raw_num;
	int pipe = 0;

	//ISP frame error handling
	if (osal_atomic_read(&vdev->isp_err_handle_flag) == 1) {
		vi_pr(VI_DBG, "wait err_handling done\n");
		return -ISP_ERROR;
	}

	if (!osal_atomic_read(&vdev->stream.csi_streamon[raw_num]) && !ctx->is_rawreplay) {
		vi_pr(VI_DBG, "VI not ready\n");
		return -ISP_STOP;
	}

	if (_is_drop_next_frame(vdev, raw_num, chn_num)) {
		vi_pr(VI_DBG, "Pre_fe_%d chn_num_%d drop_frame_num %d\n",
				raw_num, chn_num, vdev->drop_frame_number[raw_num]);
		return -ISP_DROP_FRM;
	}

	if (_is_fe_post_offline(ctx)) { //fe->dram->post
		if (ctx->is_rawreplay) { //TODO mv to post_hw_enque
			if (osal_atomic_cmpxchg(&vdev->postraw_state,
						ISP_STATE_IDLE, ISP_STATE_RUNNING) ==
						ISP_STATE_RUNNING) {
				vi_pr(VI_DBG, "Postraw is running\n");
				return -ISP_RUNNING;
			}

			pipe = ctx->isp_csi_cfg[raw_num].bind_pipe[chn_num];
			if (ctx->isp_pipe_cfg[pipe].is_offline_scaler) { //Scaler onffline mode
				if (_postraw_outbuf_empty(vdev, pipe, chn_num)) {
					osal_atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
					return -ISP_NO_BUFFER;
				}

				_postraw_update_cfg_from_user(vdev, pipe, chn_num);
				_postraw_outbuf_enq(vdev, pipe, chn_num);
			} else { //Scaler online mode
				struct vpss_online_cb_info post_para = {0};

				/* VI Online VPSS sc cb trigger */
				post_para.snr_num = pipe;
				post_para.is_tile = false;
				if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_VI_ONLINE_TRIGGER, &post_para) != 0) {
					vi_pr(VI_DBG, "raw_%d, sc_%d is running\n", raw_num, pipe);
					osal_atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
					return -ISP_RUNNING;
				}
			}

			if (osal_atomic_read(&vdev->raw_dump[raw_num].raw_dump_en[chn_num]) == RAWDUMP_START)
				_isp_fe_raw_dump_cfg(vdev, raw_num, chn_num);

			isp_pre_trig(ctx, raw_num, chn_num);
		} else {
			phy_raw = find_phy_raw_num(ctx, raw_num);
			if (osal_atomic_cmpxchg(&vdev->pre_fe_state[phy_raw][chn_num],
						ISP_STATE_IDLE, ISP_STATE_RUNNING) ==
						ISP_STATE_RUNNING) {
				vi_pr(VI_DBG, "Pre_fe_%d chn_num_%d is running\n", raw_num, chn_num);
				return -ISP_RUNNING;
			}

			// only if fe->dram
			if (_pre_fe_outbuf_enque(vdev, raw_num, chn_num)) {
				isp_pre_trig(ctx, raw_num, chn_num);
			} else {
				osal_atomic_set(&vdev->pre_fe_state[phy_raw][chn_num], ISP_STATE_IDLE);
				return -ISP_NO_BUFFER;
			}
		}
	} else if (_is_all_online(ctx)) {
		if (osal_atomic_cmpxchg(&vdev->postraw_state,
					ISP_STATE_IDLE, ISP_STATE_RUNNING) ==
					ISP_STATE_RUNNING) {
			vi_pr(VI_DBG, "Postraw is running\n");
			return -ISP_RUNNING;
		}

		pipe = ctx->isp_csi_cfg[raw_num].bind_pipe[chn_num];
		if (ctx->isp_pipe_cfg[pipe].is_offline_scaler) { //Scaler onffline mode
			if (_postraw_outbuf_empty(vdev, pipe, chn_num)) {
				osal_atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
				return -ISP_NO_BUFFER;
			}

			_postraw_outbuf_enq(vdev, pipe, chn_num);
		}

		if (osal_atomic_read(&vdev->raw_dump[raw_num].raw_dump_en[chn_num]) == RAWDUMP_START)
			_isp_fe_raw_dump_cfg(vdev, raw_num, chn_num);

		isp_pre_trig(ctx, raw_num, chn_num);
	} else if (_is_fe_post_slice(ctx)) { //fe->slice->post
		if (osal_atomic_cmpxchg(&vdev->pre_fe_state[raw_num][chn_num],
					ISP_STATE_IDLE, ISP_STATE_RUNNING) ==
					ISP_STATE_RUNNING) {
			vi_pr(VI_DBG, "Pre_fe_%d chn_num_%d is running\n", raw_num, chn_num);
			return -ISP_RUNNING;
		}

		/*don't care fe state, only call by post_hw_enque*/
		if (osal_atomic_read(&vdev->raw_dump[raw_num].raw_dump_en[chn_num]) == RAWDUMP_START)
			_isp_fe_raw_dump_cfg(vdev, raw_num, chn_num);
		isp_pre_trig(ctx, raw_num, chn_num);
	}

	return ISP_SUCCESS;
}

static inline void _swap_post_sts_buf(struct isp_ctx *ctx, const u8 pipe)
{
	struct _membuf *pool;
	unsigned long flags;
	u8 idx;
	pool = &ctx->isp_bufpool[pipe];

	osal_spin_lock_irqsave(&pool->post_sts_lock, &flags);
	if (pool->post_sts_in_use == 1) {
		osal_spin_unlock_irqrestore(&pool->post_sts_lock, &flags);
		vi_pr(VI_DBG, "post sts is in use\n");
		return;
	}
	pool->post_sts_busy_idx ^= 1;
	osal_spin_unlock_irqrestore(&pool->post_sts_lock, &flags);

	idx = pool->post_sts_busy_idx ^ 1;

	//af
	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_AF_W, pool->sts_mem[idx].af.phy_addr);

	//lsc_hist
	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_LSCR_HIST, pool->sts_mem[idx].lsc_hist.phy_addr);

	//drc_hist
	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_DRC_HIST, pool->sts_mem[idx].drc_hist.phy_addr);

	//yuvtop
	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_LDCI_HIST, pool->sts_mem[idx].ldci_hist.phy_addr);
}

static inline void _post_drc_poly_update(struct isp_ctx *ctx, const u8 pipe)
{
	u64 drc_poly = ctx->isp_bufpool[pipe].drc_poly[BUF_IDX0];

	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_DRC_POLY_W, drc_poly);
	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_DRC_POLY_R, drc_poly);
}

static inline void _post_ldci_update(struct isp_ctx *ctx, const u8 pipe, u64 frm)
{
	u8 cur_idx = frm % BUF_MAX;
	u8 pre_idx = (frm + 1) % BUF_MAX;
	u64 ldci_lmap_cur = ctx->isp_bufpool[pipe].ldci_lmap[cur_idx];
	u64 ldci_lmap_pre = ctx->isp_bufpool[pipe].ldci_lmap[pre_idx];

	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_LDCI_W, ldci_lmap_cur);
	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_LDCI_R, ldci_lmap_pre);
}

static inline void _post_clut_update(struct isp_ctx *ctx, const u8 pipe)
{
	u64 clut = ctx->isp_bufpool[pipe].clut;

	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_CLUT_R, clut);
}

static inline void _post_cnr_update(struct isp_ctx *ctx, const u8 pipe)
{
	u64 cnr_y = ctx->isp_bufpool[pipe].cnr_y;
	u64 cnr_c = ctx->isp_bufpool[pipe].cnr_c;

	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_CNR_Y_W, cnr_y);
	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_CNR_Y_R, cnr_y);

	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_CNR_C_W, cnr_c);
	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_CNR_C_R, cnr_c);
}

static inline void _post_3dnr_update(struct isp_ctx *ctx, const u8 pipe)
{
	u64 r_y_addr, r_uv_addr;
	u64 w_y_addr, w_uv_addr;
	u64 w_mv_addr, r_mv_addr;
	u64 w_mo_addr, r_mo_addr;
	u64 w_fcb_addr, r_fcb_addr;

	if (!ctx->is_3dnr_on)
		return;

	r_y_addr = w_y_addr = ctx->isp_bufpool[pipe].tdnr[TNR_ST_Y][BUF_IDX0];
	r_uv_addr = w_uv_addr = ctx->isp_bufpool[pipe].tdnr[TNR_ST_C][BUF_IDX0];
	r_mv_addr = w_mv_addr = ctx->isp_bufpool[pipe].tdnr[TNR_ST_MV][BUF_IDX0];
	r_mo_addr = w_mo_addr = ctx->isp_bufpool[pipe].tdnr[TNR_ST_MO][BUF_IDX0];
	r_fcb_addr = w_fcb_addr = ctx->isp_bufpool[pipe].tdnr[TNR_ST_FCB][BUF_IDX0];

	//3dnr y
	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_Y, w_y_addr);
	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_LD_Y, r_y_addr);

	//3dnr uv
	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_C, w_uv_addr);
	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_LD_C, r_uv_addr);

	//3dnr mv
	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_MV, w_mv_addr);
	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_LD_MV, r_mv_addr);

	//3dnr mo
	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_MO, w_mo_addr);
	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_LD_MO, r_mo_addr);

	//3dnr fcb
	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_FCB, w_fcb_addr);
	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_LD_FCB, r_fcb_addr);
}

static inline void _post_msp_update(struct isp_ctx *ctx, const u8 pipe, u64 frm)
{
	uint8_t idx = osal_div_u64_rem(frm, MSP_BUF_MAX);

	//3dnr msp
	ispblk_dma_config(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_MSP,
			ctx->isp_bufpool[pipe].msp[MOTION_MASTER_CHN][idx]);

	//msp slave
	ispblk_mctf_resize_config(ctx, pipe, ctx->isp_bufpool[pipe].msp[MOTION_SLAVE_CHN][idx],
				osal_atomic_read(&ctx->isp_pipe_cfg[pipe].resize_en));
}

static inline void _post_yuv_crop_update(struct isp_ctx *ctx, const u8 pipe)
{
	struct vi_rect crop;

	if (ctx->isp_pipe_cfg[pipe].is_offline_scaler) {
		ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_Y, true, false);
		ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_C, true, false);

		if (ctx->isp_pipe_cfg[pipe].is_postout_crop) {
			crop = ctx->isp_pipe_cfg[pipe].postout_crop;

			ispblk_crop_config(ctx, ISP_BLK_ID_YUV_CROP_Y, crop);
			crop.x >>= 1;
			crop.y >>= 1;
			crop.w >>= 1;
			crop.h >>= 1;
			ispblk_crop_config(ctx, ISP_BLK_ID_YUV_CROP_C, crop);
		} else {
			if (ctx->isp_pipe_cfg[pipe].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
				crop.x = 0;
				crop.y = 0;
				crop.w = ctx->cfg_info.img_width;
				crop.h = ctx->cfg_info.img_height;
				ispblk_crop_config(ctx, ISP_BLK_ID_YUV_CROP_Y, crop);
				crop.x >>= 1;
				crop.y >>= 1;
				crop.w >>= 1;
				crop.h >>= 1;
				ispblk_crop_config(ctx, ISP_BLK_ID_YUV_CROP_C, crop);
			}
			ispblk_crop_enable(ctx, ISP_BLK_ID_YUV_CROP_Y, false);
			ispblk_crop_enable(ctx, ISP_BLK_ID_YUV_CROP_C, false);
		}
	} else {
		ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_Y, false, false);
		ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_C, false, false);
	}
}

static inline void _post_dma_update(struct sop_vi_dev *vdev, const u8 pipe)
{
	struct isp_ctx *ctx = &vdev->ctx;

	if (!ctx->isp_pipe_cfg[pipe].is_yuv_sensor) { //RGB Sensor

		//update drc_poly dma
		_post_drc_poly_update(ctx, pipe);

		//update ldci_lmap dma
		_post_ldci_update(ctx, pipe, vdev->postraw_frame_number[pipe]);

		_post_clut_update(ctx, pipe);

		//update 3dnr dma
		_post_3dnr_update(ctx, pipe);

		//update msp dma
		_post_msp_update(ctx, pipe, vdev->postraw_frame_number[pipe]);

		//updated cnr update
		_post_cnr_update(ctx, pipe);

		//update yuv_crop dma
		_post_yuv_crop_update(ctx, pipe);
	} else {
		//YUV Sensor
		if (ctx->isp_pipe_cfg[pipe].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			if (ctx->is_3dnr_on) {
				//update 3dnr dma
				_post_3dnr_update(ctx, pipe);
			}
			//update yuv_crop dma
			_post_yuv_crop_update(ctx, pipe);
		}
	}
}

static u32 _is_fisrt_frm_after_drop(
	struct sop_vi_dev *vdev,
	const u8 pipe)
{
	struct isp_ctx *ctx = &vdev->ctx;
	u32 first_frm_num_after_drop = ctx->isp_pipe_cfg[pipe].isp_reset_frm;
	u32 frm_num = 0;

	frm_num = vdev->postraw_frame_number[pipe];

	if ((first_frm_num_after_drop != 0) && (frm_num == first_frm_num_after_drop)) {
		vi_pr(VI_DBG, "reset isp frm_num[%d]\n", frm_num);
		ctx->isp_pipe_cfg[pipe].isp_reset_frm = 0;
		return 1;
	} else
		return 0;
}

static inline void _post_ctrl_update(struct sop_vi_dev *vdev, const u8 pipe)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw dst_raw = ctx->isp_pipe_cfg[pipe].bind_raw;
	_vi_clear_mmap_fbc_ring_base(vdev, pipe);

	//TODO need refator
	if (ctx->is_3dnr_on) {
		//To set apply the prev frm or not for manr/3dnr
		if (ctx->isp_pipe_cfg[pipe].first_frm_rst ||
		    _is_fisrt_frm_after_drop(vdev, dst_raw)) {
			ctx->isp_pipe_cfg[pipe].first_frm_rst = false;
			isp_first_frm_reset(ctx, 1, vdev->postraw_frame_number[pipe] > 0);
		} else {
			isp_first_frm_reset(ctx, 0, false);
		}
	}
}

static inline int _isp_clk_dynamic_en(struct sop_vi_dev *vdev, bool en)
{
#ifdef TODO_VI
	if (clk_dynamic_en && vdev->isp_clk[5]) {
		struct isp_ctx *ctx = &vdev->ctx;

		if (en && !__clk_is_enabled(vdev->isp_clk[5])) {
			if (clk_enable(vdev->isp_clk[5])) {
				vi_pr(VI_ERR, "[ERR] ISP_CLK(%s) enable fail\n", CLK_ISP_NAME[5]);
				if (_is_fe_be_online(ctx))
					osal_atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
				else if (_is_be_post_online(ctx)) {
					osal_atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_STATE_IDLE);
					osal_atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
				}

				return -1;
			}

			vi_pr(VI_DBG, "enable clk(%s)\n", CLK_ISP_NAME[5]);
		} else if (!en && __clk_is_enabled(vdev->isp_clk[5])) {
			clk_disable(vdev->isp_clk[5]);

			vi_pr(VI_DBG, "disable clk(%s)\n", CLK_ISP_NAME[5]);
		}
	} else { //check isp_top_clk is enabled
		struct isp_ctx *ctx = &vdev->ctx;

		if (!__clk_is_enabled(vdev->isp_clk[5])) {
			if (clk_enable(vdev->isp_clk[5])) {
				vip_pr(ERR, "[ERR] ISP_CLK(%s) enable fail\n", CLK_ISP_NAME[5]);
				if (_is_fe_be_online(ctx))
					osal_atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
				else if (_is_be_post_online(ctx)) {
					osal_atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_STATE_IDLE);
					osal_atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
				}
			}
		}
	}
#endif
	return 0;
}

/*
 * update cfg by raw_num when post_num==0, maybe will update by raw_num and chn_num.
 */
static void _postraw_update_cfg_from_user(struct sop_vi_dev *vdev, const u8 pipe, const u8 chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	if (!vi_ctx->chn_crop[pipe][chn_num].enable) {
		ctx->isp_pipe_cfg[pipe].is_postout_crop = false;
		return;
	}

	ctx->isp_pipe_cfg[pipe].postout_crop.x = vi_ctx->chn_crop[pipe][chn_num].crop_rect.x;
	ctx->isp_pipe_cfg[pipe].postout_crop.y = vi_ctx->chn_crop[pipe][chn_num].crop_rect.y;
	ctx->isp_pipe_cfg[pipe].postout_crop.w = vi_ctx->chn_crop[pipe][chn_num].crop_rect.width;
	ctx->isp_pipe_cfg[pipe].postout_crop.h = vi_ctx->chn_crop[pipe][chn_num].crop_rect.height;
	ctx->isp_pipe_cfg[pipe].is_postout_crop = true;

	vi_pr(VI_DBG, "chn_num[%d] crop(%d,%d,%d,%d)\n",
			chn_num, ctx->isp_pipe_cfg[pipe].postout_crop.x,
			ctx->isp_pipe_cfg[pipe].postout_crop.y,
			ctx->isp_pipe_cfg[pipe].postout_crop.w,
			ctx->isp_pipe_cfg[pipe].postout_crop.h);
}

static void vi_calc_pre_dci_motion(struct sop_vi_dev *vdev, uint8_t pipe)
{
	struct isp_ctx *ctx = &vdev->ctx;
	unsigned long flags;
	uint8_t dci_idx = 0;
	uint32_t i = 0;
	uint64_t dci_sum = 0;
	struct _membuf *pool;
	uint8_t *dci_vaddr = NULL;
	uint16_t dci_lv = 0;
	int total_cnt = 0, dci_means = 0;
	int dci_value = 0;
	uint32_t still, trans, motion;
	uint32_t w, h;
	uint8_t motion_lv;
	uint8_t idx = (vdev->postraw_frame_number[pipe] - 1) % MSP_BUF_MAX;
	struct mlv_i_s *mlv_i = &vdev->mlv_i[pipe][idx];

	if (vdev->postraw_frame_number[pipe] == 0 || mlv_i->frm_num == vdev->postraw_frame_number[pipe]) {
		return;
	}

	pool = &ctx->isp_bufpool[pipe];

	osal_spin_lock_irqsave(&pool->post_sts_lock, &flags);
	if (pool->post_sts_in_use == 1) {
		dci_idx = pool->post_sts_busy_idx ^ 1;
	} else {
		if (_is_fe_post_offline(ctx))
			dci_idx = pool->post_sts_busy_idx;
		else
			dci_idx = pool->post_sts_busy_idx ^ 1;
	}
	osal_spin_unlock_irqrestore(&pool->post_sts_lock, &flags);

	dci_vaddr = (uint8_t *)osal_phys_to_virt(pool->sts_mem[dci_idx].ldci_hist.phy_addr);

	for (i = 0; i < 256; i++) {
		dci_value = *(uint16_t *)&dci_vaddr[i * 2];
		total_cnt += dci_value;
	}

	dci_means = total_cnt >> 8;

	for (i = 0; i < 256; i++) {
		dci_value = *(uint16_t *)&dci_vaddr[i * 2];
		dci_sum += ABS(dci_value - dci_means);
	}

	if (dci_means)
		dci_lv = 511 - osal_div_u64(dci_sum, dci_means);

	//calc motion lv
	ispblk_mctf_mmap_status(ctx, &still, &trans, &motion);

	if((still + trans + motion) != 0){
		motion_lv = osal_div_u64((trans + motion) * 255, (still + trans + motion));
	} else {
		motion_lv = 0;
	}

	mlv_i->msp[MSP_MASTER_CHN].width = w = ctx->isp_pipe_cfg[pipe].crop.w;
	mlv_i->msp[MSP_MASTER_CHN].height = h = ctx->isp_pipe_cfg[pipe].crop.h;
	mlv_i->msp[MSP_MASTER_CHN].phy_addr = pool->msp[MSP_MASTER_CHN][idx];
	mlv_i->msp[MSP_MASTER_CHN].len = ((UPPER(w, 2) * UPPER(h, 2) >> 4) + 3) >> 2;

	mlv_i->msp[MSP_SLAVE_CHN].width = w = ctx->isp_pipe_cfg[pipe].resize_w;
	mlv_i->msp[MSP_SLAVE_CHN].height = h = ctx->isp_pipe_cfg[pipe].resize_h;
	mlv_i->msp[MSP_SLAVE_CHN].phy_addr = pool->msp[MSP_SLAVE_CHN][idx];
	mlv_i->msp[MSP_SLAVE_CHN].len = ((UPPER(w, 2) * UPPER(h, 2) >> 4) + 3) >> 2;

	mlv_i->dci_lv = dci_lv;
	mlv_i->motion_lv = motion_lv;
	mlv_i->frm_num = vdev->postraw_frame_number[pipe];

	vi_pr(VI_DBG, "pipe(%d) idx(%d) dci_lv=%d, motion_lv=%d frm=%d\n",
			pipe, idx, dci_lv, motion_lv, vdev->postraw_frame_number[pipe]);
}

/*
 * - postraw offline -
 *  trig postraw if there is in/out buffer for postraw
 * - postraw online -
 *  trig preraw if there is output buffer for postraw
 */
static void _post_hw_enque(
	struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	enum sop_isp_fe_chn_num chn_num = ISP_FE_CH0;
	u8 pipe = 0, pre_pipe = ctx->cfg_info.pipe;

	if (osal_atomic_read(&vdev->isp_err_handle_flag) == 1) {
		vi_pr(VI_DBG, "wait err_handing done\n");
		return;
	}

	if (_is_fe_post_offline(ctx)) { //fe->be->dram->post
		if (osal_atomic_cmpxchg(&vdev->postraw_state,
					ISP_STATE_IDLE, ISP_STATE_RUNNING) ==
					ISP_STATE_RUNNING) {
			vi_pr(VI_DBG, "Postraw is running\n");
			return;
		}

		if (_postraw_inbuf_enq_check(vdev)) {
			osal_atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
			return;
		}

		raw_num = ctx->cfg_info.raw_num;
		pipe = ctx->cfg_info.pipe;
		chn_num = ctx->cfg_info.chn_num;

		if (!ctx->isp_pipe_cfg[pipe].is_offline_scaler) { //Scaler online mode
			struct vpss_online_cb_info post_para = {0};

			/* VI Online VPSS sc cb trigger */
			post_para.snr_num = pipe;
			post_para.is_tile = false;
			post_para.frm_num = vdev->postraw_frame_number[pipe] + 1;
			post_para.ts = ctx->isp_pipe_cfg[pipe].tv;
			if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_VI_ONLINE_TRIGGER, &post_para) != 0) {
				vi_pr(VI_DBG, "snr_num_%d, sc_%d is running\n", raw_num, pipe);
				osal_atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
				return;
			}
		} else { //Scaler offline mode
			if (_postraw_outbuf_empty(vdev, pipe, chn_num)) {
				osal_atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
				return;
			}

			_postraw_update_cfg_from_user(vdev, pipe, chn_num);
			_postraw_outbuf_enq(vdev, pipe, chn_num);
		}

		if (_isp_clk_dynamic_en(vdev, true) < 0)
			return;

		ispblk_post_yuv_cfg_update(ctx, pipe);

		if (ctx->isp_pipe_cfg[pipe].is_yuv_sensor) { //YUV sensor
			if (ctx->isp_pipe_cfg[pipe].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
				postraw_tuning_update(ctx, pipe);

				//Update postraw dma size/addr
				_post_dma_update(vdev, pipe);
			}
		} else { //RGB sensor
			//Update postraw size/ctrl flow
			postraw_tuning_update(ctx, pipe);

			_post_ctrl_update(vdev, pipe);

			//Update postraw dma size/addr
			_post_dma_update(vdev, pipe);
			//Update postraw stt gms/ae/hist_edge_v dma size/addr
			_swap_post_sts_buf(ctx, pipe);
		}

		ispblk_fbc_debug_info(ctx);

		vi_calc_pre_dci_motion(vdev, pre_pipe);

		isp_post_trig(ctx);

		vi_record_post_trigger(vdev, pipe);

	} else if (_is_all_online(ctx)) { //on-the-fly
		if (osal_atomic_read(&vdev->postraw_state) == ISP_STATE_RUNNING) {
			vi_pr(VI_DBG, "Postraw is running\n");
			return;
		}

		raw_num = ctx->cfg_info.raw_num;
		pipe = ctx->cfg_info.pipe;
		chn_num = ctx->cfg_info.chn_num;

		if (!ctx->isp_pipe_cfg[pipe].is_offline_scaler) { //Scaler online mode
			struct vpss_online_cb_info post_para = {0};

			/* VI Online VPSS sc cb trigger */
			post_para.snr_num = pipe;
			post_para.is_tile = false;
			post_para.frm_num = vdev->postraw_frame_number[pipe] + 1;
			osal_gettimeofday(&post_para.ts);
			if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_VI_ONLINE_TRIGGER, &post_para) != 0) {
				vi_pr(VI_DBG, "snr_num_%d, sc_%d is running\n", raw_num, pipe);
				osal_atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
				return;
			}
		}

		if (ctx->isp_pipe_cfg[pipe].first_frm_rst) {
			ctx->isp_pipe_cfg[pipe].first_frm_rst = false;
			isp_first_frm_reset(ctx, true, false);
		} else {
			isp_first_frm_reset(ctx, false, false);
		}

		_vi_clear_mmap_fbc_ring_base(vdev, pipe);
		// vi_tuning_gamma_ips_update(ctx, raw_num);
		// vi_tuning_clut_update(ctx, raw_num);

		ispblk_fbc_debug_info(ctx);

		if (!ctx->is_rawreplay)
			_pre_hw_enque(vdev, raw_num, ISP_FE_CH0);

		vi_calc_pre_dci_motion(vdev, pre_pipe);
	} else if (_is_fe_post_slice(ctx)) {
		raw_num = ctx->cfg_info.raw_num;
		pipe = ctx->cfg_info.pipe;
		chn_num = ctx->cfg_info.chn_num;

		if (osal_atomic_read(&vdev->stream.isp_streamon[pipe]) == 0) {
			vi_pr(VI_DBG, "stop streaming\n");
			return;
		}

		//Things have to be done in post done
		if (osal_atomic_cmpxchg(&ctx->is_post_done, 1, 0) == 1) { //Change is_post_done flag to 0
			if (ctx->isp_pipe_cfg[pipe].is_offline_scaler) { //Scaler offline mode
				if (!_postraw_outbuf_empty(vdev, pipe, ISP_FE_CH0)) {
					_postraw_update_cfg_from_user(vdev, pipe, ISP_FE_CH0);
					_postraw_outbuf_enq(vdev, pipe, ISP_FE_CH0);
				}
			}

			_vi_clear_mmap_fbc_ring_base(vdev, pipe);

			vi_tuning_gamma_ips_update(ctx, pipe);
			vi_tuning_clut_update(ctx, pipe);

			ispblk_fbc_debug_info(ctx);

		} else { //Things have to be done in be done for fps issue
			if (osal_atomic_read(&vdev->stream.isp_streamon[pipe])) {
				if (osal_atomic_read(&vdev->pre_fe_state[raw_num][ISP_FE_CH0]) == ISP_STATE_RUNNING ||
				    osal_atomic_read(&vdev->pre_fe_state[raw_num][ISP_FE_CH1]) == ISP_STATE_RUNNING) {
					vi_pr(VI_DBG, "fe is running\n");
					return;
				}

				if (!ctx->isp_pipe_cfg[pipe].is_offline_scaler) { //Scaler online mode
					struct vpss_online_cb_info post_para = {0};

					/* VI Online VPSS sc cb trigger */
					post_para.snr_num = pipe;
					post_para.is_tile = false;
					post_para.frm_num = vdev->postraw_frame_number[pipe] + 1;
					osal_gettimeofday(&post_para.ts);
					if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_VI_ONLINE_TRIGGER, &post_para) != 0) {
						vi_pr(VI_DBG, "snr_num_%d, sc_%d is not ready\n", raw_num, pipe);
						return;
					}
				}
			}

			osal_atomic_set(&vdev->postraw_state, ISP_STATE_RUNNING);

			isp_post_trig(ctx);
			vi_record_post_trigger(vdev, pipe);

			if (!ctx->is_rawreplay) {
				_pre_hw_enque(vdev, raw_num, ISP_FE_CH0);
				if (ctx->isp_csi_cfg[raw_num].is_hdr_on)
					_pre_hw_enque(vdev, raw_num, ISP_FE_CH1);
			}

			vi_calc_pre_dci_motion(vdev, pre_pipe);
		}
	}
}

void vi_suspend(struct sop_vi_dev *vdev)
{
	vi_pr(VI_WARN, "vi_suspend not suspend\n");
}

void vi_resume(struct sop_vi_dev *vdev)
{
	vi_pr(VI_WARN, "vi_resume not suspend\n");
}

void vi_destroy_thread(struct sop_vi_dev *vdev, enum E_VI_TH th_id)
{
	int cnt = 0;

	if (th_id < 0 || th_id >= E_VI_TH_MAX) {
		vi_pr(VI_ERR, "No such thread_id(%d)\n", th_id);
		return;
	}

	if (vdev->vi_th[th_id].w_thread != NULL) {
		osal_atomic_set(&vdev->vi_th[th_id].exit_flag, 1);
		osal_kthread_destroy(vdev->vi_th[th_id].w_thread, 1);
		while (osal_atomic_read(&vdev->vi_th[th_id].thread_exit) == 0 && cnt < 10) {
			vi_pr(VI_INFO, "wait for %s exit\n", vdev->vi_th[th_id].th_name);
			osal_usleep_range(5 * 1000, 10 * 1000);
			cnt++;
		}

		vdev->vi_th[th_id].w_thread = NULL;
		osal_atomic_set(&vdev->vi_th[th_id].exit_flag, 0);
		osal_atomic_set(&vdev->vi_th[th_id].thread_exit, 0);
		osal_wait_destroy(&vdev->vi_th[th_id].wq);
	}
}

int vi_create_thread(struct sop_vi_dev *vdev, enum E_VI_TH th_id)
{
	int rc = 0;
	uint32_t prio = OSAL_TASK_PRIORITY_HIGH;

	if (th_id < 0 || th_id >= E_VI_TH_MAX) {
		vi_pr(VI_ERR, "No such thread_id(%d)\n", th_id);
		return -1;
	}

	if (vdev->vi_th[th_id].w_thread == NULL) {
		switch (th_id) {
		case E_VI_TH_POSTRAW:
			osal_memcpy(vdev->vi_th[th_id].th_name, "isp_post", sizeof(vdev->vi_th[th_id].th_name));
			vdev->vi_th[th_id].th_handler = isp_post_tasklet;
			prio = OSAL_TASK_PRIORITY_ABOVE_HIGH;
			break;
		case E_VI_TH_PRERAW:
			osal_memcpy(vdev->vi_th[th_id].th_name, "task_isp_pre", sizeof(vdev->vi_th[th_id].th_name));
			vdev->vi_th[th_id].th_handler = _vi_preraw_thread;
			break;
		case E_VI_TH_VBLANK_HANDLER:
			osal_memcpy(vdev->vi_th[th_id].th_name, "task_isp_blank", sizeof(vdev->vi_th[th_id].th_name));
			vdev->vi_th[th_id].th_handler = _vi_vblank_handler_thread;
			break;
		case E_VI_TH_ERR_HANDLER:
			osal_memcpy(vdev->vi_th[th_id].th_name, "task_isp_err", sizeof(vdev->vi_th[th_id].th_name));
			vdev->vi_th[th_id].th_handler = _vi_err_handler_thread;
			break;
		case E_VI_TH_EVENT_HANDLER:
			osal_memcpy(vdev->vi_th[th_id].th_name, "vi_event_handler", sizeof(vdev->vi_th[th_id].th_name));
			vdev->vi_th[th_id].th_handler = _vi_event_handler_thread;
			break;
		case E_VI_TH_AI_ISP:
			osal_memcpy(vdev->vi_th[th_id].th_name, "vi_ai_isp", sizeof(vdev->vi_th[th_id].th_name));
			vdev->vi_th[th_id].th_handler = _vi_ai_isp_handler_thread;
			break;
		default:
			vi_pr(VI_ERR, "No such thread(%d)\n", th_id);
			return -1;
		}

		osal_atomic_set(&vdev->vi_th[th_id].flag, 0);
		osal_atomic_set(&vdev->vi_th[th_id].exit_flag, 0);
		osal_atomic_set(&vdev->vi_th[th_id].thread_exit, 0);
		osal_wait_init(&vdev->vi_th[th_id].wq);

		vdev->vi_th[th_id].w_thread = osal_kthread_create(vdev->vi_th[th_id].th_handler,
								(void *)vdev,
								vdev->vi_th[th_id].th_name, 8192);
		if (!vdev->vi_th[th_id].w_thread) {
			vi_pr(VI_ERR, "Unable to start %s.\n", vdev->vi_th[th_id].th_name);
			return -1;
		}

		osal_kthread_set_priority(vdev->vi_th[th_id].w_thread, prio);
	}

	return rc;
}

//open vi device
void vi_sw_init(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);
	u8 i = 0, j = 0;

	osal_memset(vi_ctx, 0, sizeof(struct sop_vi_ctx));

	for (i = 0; i < VI_MAX_PIPE_NUM; i++) {
		for (j = 0; j < VI_MAX_CHN_NUM; ++j) {
			osal_mutex_init(&vdev->mesh[i][j].lock);
			osal_atomic_set(&vdev->mesh[i][j].gdc_flag, 0);
		}
		osal_mutex_init(&vi_ctx->pipe_lock[i]);
	}

	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		osal_mutex_init(&vi_ctx->dev_lock[i]);
	}

	usr_pic_timer_init(vdev);
	isp_raw_dump_init(vdev);

	ctx->is_3dnr_on         = true;
	ctx->is_dpcm_on         = false;
	ctx->is_hdr_on          = false;
	ctx->is_multi_sensor    = false;
	ctx->is_sublvds_path    = false;
	ctx->is_fbc_on          = true;
	ctx->is_ctrl_inited     = false;
	ctx->is_ai_isp          = false;
	ctx->virt_raw_offset    = 0;
	ctx->cfg_info.raw_num   = ISP_PRERAW_MAX;
	ctx->cfg_info.pipe      = VI_MAX_PIPE_NUM;
	vdev->isp_source        = ISP_SOURCE_DEV;

	if (vi_ctx->vi_stt != VI_SUSPEND)
		osal_memset(vdev->snr_info, 0, sizeof(struct sop_isp_snr_info) * ISP_PRERAW_MAX);

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		vdev->drop_frame_number[i]    = 0;
		vdev->dump_frame_number[i]    = 0;
		vdev->isp_int_flag[i]         = false;
		ctx->raw_chnstr_num[i]        = 0;

		for (j = 0; j < ISP_FE_CHN_MAX; j++) {
			vdev->pre_fe_sof_cnt[i][j] = 0;
			vdev->pre_fe_frm_num[i][j] = 0;
			vdev->raw_dump[i].isp_byr[j] = 0;

			osal_atomic_set(&vdev->pre_fe_state[i][j], ISP_STATE_IDLE);
			osal_atomic_set(&vdev->raw_dump[i].raw_dump_en[j], RAWDUMP_IDLE);
		}

		osal_spin_lock_init(&ctx->csi_bufpool[i].pre_fe_sts_lock);

		osal_atomic_set(&vdev->raw_dump[i].isp_smooth_raw_dump_en, SMOOTH_RAWDUMP_IDLE);
		osal_atomic_set(&vdev->isp_err_times[i], 0);
	}

	for (i = 0; i < VI_MAX_PIPE_NUM; i++) {
		vdev->postraw_frame_number[i] = 0;
		ctx->isp_pipe_cfg[i].is_enable = false;
		ctx->isp_pipe_cfg[i].first_frm_rst = true;
		osal_spin_lock_init(&ctx->isp_bufpool[i].post_sts_lock);
	}

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		for (j = 0; j < ISP_FE_CHN_MAX; j++) {
			isp_buf_init(&vdev->pre_fe_out_q[i][j]);
			vdev->pre_fe_out_q[i][j].raw_num = i;

			isp_buf_init(&vdev->pre_ai_isp_in_q[i][j]);
			vdev->pre_ai_isp_in_q[i][j].raw_num = i;

			isp_buf_init(&vdev->pre_ai_isp_out_q[i][j]);
			vdev->pre_ai_isp_out_q[i][j].raw_num = i;

			isp_buf_init(&vdev->raw_dump[i].buf_q[j]);
			vdev->raw_dump[i].buf_q[j].raw_num = i;

			isp_buf_init(&vdev->raw_dump[i].buf_dq[j]);
			vdev->raw_dump[i].buf_dq[j].raw_num = i;
		}

		OSAL_INIT_LIST_HEAD(&isp_snr_i2c_queue[i].list);
		isp_snr_i2c_queue[i].num_rdy = 0;
		OSAL_INIT_LIST_HEAD(&isp_crop_queue[i].list);
		isp_crop_queue[i].num_rdy = 0;

		vdev->isp_int_flag[i] = false;
		osal_wait_init(&vdev->isp_int_wait_q[i]);

		osal_atomic_set(&vdev->isp_ai_int_flag[i], E_AI_WAKE_TYPE_NONE);
		osal_wait_init(&vdev->isp_ai_wait_q[i]);
	}

	OSAL_INIT_LIST_HEAD(&pre_raw_num_q.list);
	OSAL_INIT_LIST_HEAD(&dqbuf_q.list);
	OSAL_INIT_LIST_HEAD(&vdev->event_q.list);
	vdev->event_q.count = 0;

	for (i = 0; i < ISP_RAW_PATH_MAX; i++) {
		isp_buf_init(&vdev->postraw_wdr_in_q[i]);
	}

	isp_buf_init(&vdev->postraw_in_q);

	osal_atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
	osal_atomic_set(&vdev->isp_err_handle_flag, 0);
	osal_atomic_set(&vdev->isp_dbg_flag, 0);
	osal_atomic_set(&ctx->is_post_done, 0);
}

static void _vi_init_param(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	u8 i = 0, j = 0;

	osal_memset(ctx, 0, sizeof(*ctx));

	ctx->phys_regs = isp_get_phys_reg_bases();

	for (i = 0; i < VI_MAX_PIPE_NUM; i++) {
		ctx->isp_pipe_cfg[i].rgb_color_mode     = ISP_BAYER_TYPE_GB;
		ctx->isp_pipe_cfg[i].yuv_scene_mode     = ISP_YUV_SCENE_BYPASS;
		ctx->isp_pipe_cfg[i].is_offline_scaler  = true;
	}

	osal_memset(&vdev->usr_crop, 0, sizeof(vdev->usr_crop));

	for (i = 0; i < VI_MAX_PIPE_NUM; i++) {
		for (j = 0; j < VI_MAX_CHN_NUM; j++) {
			OSAL_INIT_LIST_HEAD(&vdev->qbuf_list[i][j]);
			vdev->qbuf_num[i][j] = 0;
		}
	}

	//ToDo sync_task_ext
	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		osal_spin_lock_init(&snr_node_lock[i]);
		sync_task_init(i);
		OSAL_INIT_LIST_HEAD(&isp_snr_i2c_queue[i].list);
		isp_snr_i2c_queue[i].num_rdy = 0;
		OSAL_INIT_LIST_HEAD(&isp_crop_queue[i].list);
		isp_crop_queue[i].num_rdy = 0;
	}

	OSAL_INIT_LIST_HEAD(&pre_raw_num_q.list);
	OSAL_INIT_LIST_HEAD(&dqbuf_q.list);

	osal_spin_lock_init(&raw_num_lock);
	osal_spin_lock_init(&dq_lock);
	osal_spin_lock_init(&vdev->event_q.lock);
	osal_spin_lock_init(&vdev->qbuf_lock);

	osal_wait_init(&vdev->isp_event_wait_q);
	osal_wait_init(&vdev->isp_dbg_wait_q);
}

/*******************************************************
 *  File operations for core
 ******************************************************/

static int vi_cb_msp_config(struct sop_vi_dev *vdev, struct vi_cb_msp_cfg *msp_cfg)
{
	struct isp_ctx *ctx = &vdev->ctx;
	uint8_t pipe = 0;

	if (!vdev || !msp_cfg) {
		vi_pr(VI_ERR, "Invalid input: vdev or msp_cfg is null\n");
		return OSAL_EINVAL;
	}

	pipe = msp_cfg->vi_pipe;

	if (pipe >= VI_MAX_PIPE_NUM) {
		vi_pr(VI_ERR, "Invalid pipe: %d\n", pipe);
		return OSAL_EINVAL;
	}

	if (!osal_atomic_read(&vdev->stream.isp_streamon[pipe])) {
		vi_pr(VI_ERR, "pipe%d stream_off\n", pipe);
		return OSAL_EINVAL;
	}

	if (!msp_cfg->is_resize) {
		osal_atomic_set(&ctx->isp_pipe_cfg[pipe].resize_en, 0);
		return 0;
	}

	if (msp_cfg->width > ctx->isp_pipe_cfg[pipe].crop.w ||
		msp_cfg->height > ctx->isp_pipe_cfg[pipe].crop.h) {
		vi_pr(VI_ERR, "Invalid resize: width=%d, height=%d\n", msp_cfg->width, msp_cfg->height);
		return OSAL_EINVAL;
	}

	ctx->isp_pipe_cfg[pipe].resize_w = msp_cfg->width;
	ctx->isp_pipe_cfg[pipe].resize_h = msp_cfg->height;

	osal_atomic_set(&ctx->isp_pipe_cfg[pipe].resize_en, 1);

	return 0;
}

static int vi_cb_msp_info(struct sop_vi_dev *vdev, struct vi_cb_msp_info *msp_info)
{
	uint8_t pipe = 0, idx = 0;
	struct mlv_i_s *mlv_i;

	if (!vdev || !msp_info) {
		vi_pr(VI_ERR, "Invalid input: vdev or msp_info is null\n");
		return OSAL_EINVAL;
	}

	pipe = msp_info->vi_pipe;

	if (pipe >= VI_MAX_PIPE_NUM || msp_info->frm_num == 0) {
		vi_pr(VI_ERR, "Invalid pipe: %d, frm_%lld\n", pipe, msp_info->frm_num);
		return OSAL_EINVAL;
	}

	if (!osal_atomic_read(&vdev->stream.isp_streamon[pipe])) {
		vi_pr(VI_ERR, "pipe%d stream_off\n", pipe);
		return OSAL_EINVAL;
	}

	idx = osal_div_u64_rem(msp_info->frm_num - 1, MSP_BUF_MAX);
	mlv_i = &vdev->mlv_i[pipe][idx];

	if (msp_info->frm_num == mlv_i->frm_num)
		goto find_msp_info;

	// If the frame number is not equal, it means the frame is not ready yet, try to get the latest one
	if (msp_info->frm_num < 2) {
		vi_pr(VI_ERR, "Invalid frame number: %lld\n", msp_info->frm_num);
		return OSAL_EINVAL;
	}

	idx = osal_div_u64_rem(msp_info->frm_num - 2, MSP_BUF_MAX);
	mlv_i = &vdev->mlv_i[pipe][idx];

find_msp_info:

	memcpy(&msp_info->mlv_i, mlv_i, sizeof(struct mlv_i_s));

	vi_pr(VI_DBG, "pipe_%d expected: %lld, now: %lld\n", pipe, msp_info->frm_num, mlv_i->frm_num);

	return 0;
}

int vi_cb(void *dev, cb_modules_id caller, u32 cmd, void *arg)
{
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)dev;
	struct isp_ctx *ctx = &vdev->ctx;
	int rc = -1;

	if (dev == NULL) {
		vi_pr(VI_ERR, "dev is null\n");
		return -1;
	}

	switch (cmd) {
	case VI_CB_QBUF_TRIGGER:
		vi_pr(VI_INFO, "isp_ol_sc_trig_post\n");

		_vi_wake_up_post_th(vdev);

		rc = 0;
		break;
	case VI_CB_SC_FRM_DONE:
		vi_pr(VI_DBG, "sc frm done cb\n");

		_vi_wake_up_post_th(vdev);

		rc = 0;
		break;
	case VI_CB_SET_VIVPSSMODE:
	{
		vi_vpss_mode_s vi_vpss_mode;
		u8 pipe = 0;
		u8 vi_online = 0;
		u8 vi_slice = 0;

		osal_memcpy(&vi_vpss_mode, arg, sizeof(vi_vpss_mode_s));

		vi_online = (vi_vpss_mode.mode[0] == VI_ONLINE_VPSS_ONLINE) ||
			    (vi_vpss_mode.mode[0] == VI_ONLINE_VPSS_OFFLINE);

		vi_slice = (vi_vpss_mode.mode[0] == VI_SLICE_VPSS_ONLINE) ||
			    (vi_vpss_mode.mode[0] == VI_SLICE_VPSS_OFFLINE);


		ctx->is_offline_postraw = !vi_online;
		ctx->is_slice_buf_on = vi_slice;

		vi_pr(VI_DBG, "Caller_Mod(%d) set vi_online:%d, is_offline_postraw=%d\n",
				caller, vi_online, ctx->is_offline_postraw);

		for (pipe = 0; pipe < VI_MAX_PIPE_NUM; pipe++) {
			u8 is_vpss_online = (vi_vpss_mode.mode[pipe] == VI_ONLINE_VPSS_ONLINE) ||
						  (vi_vpss_mode.mode[pipe] == VI_OFFLINE_VPSS_ONLINE);

			ctx->isp_pipe_cfg[pipe].is_offline_scaler = !is_vpss_online;
			vi_pr(VI_DBG, "pipe_%d set is_offline_scaler:%d\n", pipe, !is_vpss_online);
		}

		rc = 0;
		break;
	}
	case VI_CB_GDC_OP_DONE:
	{
		struct ldc_op_done_cfg *cfg =
			(struct ldc_op_done_cfg *)arg;

		vi_gdc_callback(vdev, cfg->param, cfg->blk);
		rc = 0;
		break;
	}
	case VI_CB_GDC_GET_CHN_ATTR:
	{
		struct ldc_op_done_cfg *ldc_cfg =
			(struct ldc_op_done_cfg *)arg;
		vi_chn_attr_s chn_attr;

		struct vi_ldc_internal_chn_attr *attr = (struct vi_ldc_internal_chn_attr *)ldc_cfg->param;
		rc = vi_get_chn_attr(vdev, attr->vi_chn_attr.vi_pipe, attr->vi_chn_attr.vi_chn, &chn_attr);
		attr->vi_chn_attr.size = chn_attr.size;
		break;
	}
	case VI_CB_GDC_SET_CHN_CFG:
	{
		struct ldc_op_done_cfg *ldc_cfg =
			(struct ldc_op_done_cfg *)arg;

		struct vi_ldc_internal_chn_ldc_cfg *cfg = (struct vi_ldc_internal_chn_ldc_cfg *)ldc_cfg->param;
		rc = vi_set_chn_ldc_attr(vdev, cfg->vi_cfg.vi_pipe, cfg->vi_cfg.vi_chn,
					 &cfg->vi_cfg.ldc_attr, cfg->vi_cfg.mesh_handle);
		break;
	}
	case VI_CB_MSP_CFG:
	{
		struct vi_cb_msp_cfg *msp_cfg = (struct vi_cb_msp_cfg *)arg;

		rc = vi_cb_msp_config(vdev, msp_cfg);
		if (rc) {
			vi_pr(VI_ERR, "msp config failed\n");
		}

		break;
	}
	case VI_CB_MSP_GET_INFO:
	{
		rc = vi_cb_msp_info(vdev, (struct vi_cb_msp_info *)arg);
		if (rc) {
			vi_pr(VI_ERR, "msp get info failed\n");
		}

		break;
	}
	case VI_CB_RESET_ISP:
	{
		vi_pr(VI_ERR, "VI_CB_RESET_ISP\n");
		_vi_record_debug_info(ctx);
		vi_err_wake_up_th(vdev, ctx->cfg_info.raw_num);
	}
	default:
		break;
	}

	return rc;
}

/********************************************************************************
 *  VI event handler related
 *******************************************************************************/
void vi_destory_dbg_thread(struct sop_vi_dev *vdev)
{
	osal_atomic_set(&vdev->isp_dbg_flag, 1);
	osal_wait_wakeup(&vdev->isp_dbg_wait_q);
}

static void _vi_timeout_chk(struct sop_vi_dev *vdev)
{
	if (++vdev->timeout_cnt >= 2) {
		osal_atomic_set(&vdev->isp_dbg_flag, 1);
		osal_wait_wakeup(&vdev->isp_dbg_wait_q);
		vdev->timeout_cnt = 0;
	}
}

static void _vi_update_chn_real_frame_rate(vi_chn_status_s *vi_chn_status)
{
	u64 duration, cur_time;
	osal_timeval osal_tv;

	osal_gettimeofday(&osal_tv);
	cur_time = osal_tv.tv_sec * 1000000L + osal_tv.tv_usec;
	duration = cur_time - vi_chn_status->prev_time;

	if (duration >= 1000000) {
		vi_chn_status->frame_rate = vi_chn_status->frame_num;
		vi_chn_status->frame_num = 0;
		vi_chn_status->prev_time = cur_time;
	}

	vi_pr(VI_DBG, "FrameRate=%d\n", vi_chn_status->frame_rate);
}

static int vi_event_thread_wait_cond_func(const void *param)
{
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)param;
	enum E_VI_TH th_id = E_VI_TH_EVENT_HANDLER;

	return osal_atomic_read(&vdev->vi_th[th_id].flag)
		|| !osal_list_empty(&dqbuf_q.list)
		|| osal_atomic_read(&vdev->vi_th[th_id].exit_flag);
}

static int _vi_event_handler_thread(void *arg)
{
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)arg;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);
	int ret = 0;
	enum E_VI_TH th_id = E_VI_TH_EVENT_HANDLER;
	mmf_chn_s chn = {.mod_id = ID_VI, .dev_id = 0, .chn_id = 0};
	int pipe_id, chn_id;
	osal_timeval time[2];
	u32 sum = 0, duration, duration_max = 0, duration_min = 1000 * 1000;
	u8 count = 0;
	uint32_t flag = 0;

	while (!osal_atomic_read(&vdev->vi_th[th_id].exit_flag)) {
		_vi_update_chn_real_frame_rate(&vi_ctx->chn_status[chn.dev_id][chn.chn_id]);
		osal_gettimeofday(&time[0]);

		ret = osal_wait_timeout_uninterruptible(&vdev->vi_th[th_id].wq,
					vi_event_thread_wait_cond_func,
					vdev,
					VI_TIMEOUT_MS);

		if (osal_atomic_read(&vdev->vi_th[th_id].exit_flag))
			break;

		if (osal_atomic_read(&vdev->vi_th[th_id].flag) != 0) {
			flag = osal_atomic_read(&vdev->vi_th[th_id].flag) - 1;
			osal_atomic_set(&vdev->vi_th[th_id].flag, 0);
		}

		if (!ret) {
			if (vdev->usr_pic_delay) {
				vi_pr(VI_DBG, "vi_event_handler timeout(%d)ms\n", VI_TIMEOUT_MS);
				_vi_timeout_chk(vdev);
			}
			continue;
		} else {
			struct _vi_buffer b;
			vb_blk blk = 0;
			struct gdc_mesh *pmesh = NULL;
			struct vb_s *vb = NULL;

			//DQbuf from list.
			if (vi_dqbuf(&b)) {
				vi_pr(VI_DBG, "illegal wakeup raw_num[%d]\n", flag);
				continue;
			}

			chn.dev_id = pipe_id = b.pipe_id;
			chn.chn_id = chn_id = b.chn_id;
			ret = vb_dqbuf(chn, &vdev->vi_jobs[pipe_id][chn_id], &blk);
			if (ret) {
				if (blk == VB_INVALID_HANDLE)
					vi_pr(VI_ERR, "pipe(%d) chn(%d) can't get vb-blk.\n", pipe_id, chn_id);
				continue;
			}

			vb = (struct vb_s *)(uintptr_t)blk;

			vb->buf.dev_num = b.pipe_id;
			vb->buf.frm_num = b.sequence;
			vb->buf.pts = (uint64_t)b.tv.tv_sec * 1000000 + b.tv.tv_usec;

			vi_ctx->chn_status[pipe_id][chn_id].int_cnt++;
			vi_ctx->chn_status[pipe_id][chn_id].frame_num++;
			vi_ctx->chn_status[pipe_id][chn_id].recv_pic = b.sequence;

			vi_pr(VI_DBG, "dqbuf(%#llx) cnt(%d) pipe_id=%d chn_id=%d, skip_frm(%d) frm_num=%d\n",
					(unsigned long long)vb->phy_addr, osal_atomic_read(&vb->usr_cnt), pipe_id,
					chn_id, vi_ctx->bypass_frm[pipe_id], b.sequence);

			if (vi_ctx->bypass_frm[pipe_id] >= b.sequence) {
				//Release buffer if bypass_frm is not zero
				vi_pr(VI_DBG, "skip vb\n");
				vb_release_block(blk);
				goto QBUF;
			}

			pmesh = &vdev->mesh[pipe_id][chn_id];

			if (!osal_atomic_cmpxchg(&pmesh->gdc_flag, 0, 1)) {
				if (vi_ctx->ldc_attr[pipe_id][chn_id].enable) {
					struct _vi_gdc_cb_param cb_param = { .chn = chn, .usage = GDC_USAGE_LDC};

					if (_mesh_gdc_do_op_cb(GDC_USAGE_LDC
						, &vi_ctx->ldc_attr[pipe_id][chn_id].attr
						, vb, vi_ctx->chn_attr[pipe_id][chn_id].pixel_format
						, pmesh->paddr
						, 0, &cb_param
						, sizeof(cb_param), ID_VI
						, vi_ctx->ldc_attr[pipe_id][chn_id].attr.rotation) != 0) {
						osal_atomic_set(&pmesh->gdc_flag, 0);
						vi_pr(VI_ERR, "gdc LDC failed.\n");
					}
					goto QBUF;
				} else if (vi_ctx->rotation[pipe_id][chn_id] != ROTATION_0) {
					struct _vi_gdc_cb_param cb_param = { .chn = chn, .usage = GDC_USAGE_ROTATION};

					if (_mesh_gdc_do_op_cb(GDC_USAGE_ROTATION
						, NULL
						, vb, vi_ctx->chn_attr[pipe_id][chn_id].pixel_format
						, pmesh->paddr
						, 0, &cb_param
						, sizeof(cb_param), ID_VI
						, vi_ctx->rotation[pipe_id][chn_id]) != 0) {
						osal_atomic_set(&pmesh->gdc_flag, 0);
						vi_pr(VI_ERR, "gdc rotation failed.\n");
					}
					goto QBUF;
				}
				osal_atomic_set(&pmesh->gdc_flag, 0);
			} else {
				vi_pr(VI_WARN, "pipe(%d) chn(%d) drop frame due to gdc op blocked.\n", pipe_id, chn_id);
				// release blk if gdc not done yet
				vb_release_block(blk);
				goto QBUF;
			}

			vb_done_handler(chn, CHN_TYPE_OUT, &vdev->vi_jobs[pipe_id][chn_id], blk);
QBUF:
			if (!(vdev->ctx.isp_pipe_cfg[pipe_id].chn_enable | BIT(chn_id)))
				continue;

			if (vi_sdk_qbuf(chn, vdev) != 0) {
				vb_pool poolid = VB_INVALID_POOLID;

				if (vi_ctx->chn_attr[pipe_id][chn_id].bind_vb_pool == VB_INVALID_POOLID)
					poolid = find_vb_pool(vi_ctx->blk_size[pipe_id][chn_id]);
				else
					poolid = vi_ctx->chn_attr[pipe_id][chn_id].bind_vb_pool;

				if (poolid != VB_INVALID_POOLID)
					vb_acquire_block(vi_sdk_qbuf, chn, poolid, vdev);
			}
		}

		osal_gettimeofday(&time[1]);
		duration = get_diff_in_us(time[0], time[1]);
		duration_max = MAX(duration, duration_max);
		duration_min = MIN(duration, duration_min);
		sum += duration;
		if (++count == 100) {
			vi_pr(VI_DBG, "VI duration(ms): average(%d), max(%d) min(%d)\n"
				, sum / count / 1000, duration_max / 1000, duration_min / 1000);
			count = 0;
			sum = duration_max = 0;
			duration_min = 1000 * 1000;
		}
	}

	vi_pr(VI_INFO, "%s exit\n", vdev->vi_th[th_id].th_name);
	osal_atomic_set(&vdev->vi_th[th_id].thread_exit, 1);
	osal_kthread_exit();

	return 0;
}

/*******************************************************
 *  Irq handlers
 ******************************************************/

static void _vi_record_debug_info(struct isp_ctx *ctx)
{
	struct vi_reg_info *vi_info = NULL;

	if (g_overflow_info)
		return;

	g_overflow_info = osal_kzalloc(sizeof(*g_overflow_info), OSAL_GFP_ATOMIC);
	if (!g_overflow_info) {
		vi_pr(VI_ERR, "g_overflow_info osal_kzalloc size(%zu) fail\n", sizeof(struct overflow_info));
		return;
	}

	vi_info = &g_overflow_info->vi_info;

	if (_is_fe_post_slice(ctx) &&
	    !ctx->isp_pipe_cfg[ISP_PRERAW0].is_offline_scaler) { //VPSS online
		if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_OVERFLOW_CHECK, g_overflow_info) != 0)
			vi_pr(VI_ERR, "VPSS_CB_OVERFLOW_CHECK is failed\n");
	}

	ispblk_overflow_record_reg_info(ctx, (void *)vi_info);
}

static void _vi_show_debug_info(void)
{
	struct vi_reg_info *vi_info = NULL;
	u8 i = 0;

	if (!g_overflow_info)
		return;

	vi_info = &g_overflow_info->vi_info;

	if (vi_info->enable) {
		vi_info->enable = false;
		vi_pr(VI_INFO, "ISP_TOP, blk_idle(h38)=0x%x\n", vi_info->isp_top.blk_idle);
		for (i = 0; i <= 6; i++) {
			vi_pr(VI_INFO, "dbus_sel=%d, r_0=0x%x, r_4=0x%x, r_8=0x%x, r_c=0x%x\n",
				i,
				vi_info->isp_top.dbus_sel[i].r_0,
				vi_info->isp_top.dbus_sel[i].r_4,
				vi_info->isp_top.dbus_sel[i].r_8,
				vi_info->isp_top.dbus_sel[i].r_c);
		}
		vi_pr(VI_INFO, "PRE_RAW_FE0, preraw(h34)=0x%x, fe_idle(h50)=0x%x\n",
			vi_info->preraw_fe.preraw_info,
			vi_info->preraw_fe.fe_idle_info);
		vi_pr(VI_INFO, "PRE_RAW_BE, preraw_be(h14)=0x%x, be_dma_idle(h18)=0x%x, ip_idle(h1c)=0x%x\n",
			vi_info->preraw_be.preraw_be_info,
			vi_info->preraw_be.be_dma_idle_info,
			vi_info->preraw_be.ip_idle_info);
		vi_pr(VI_INFO, "PRE_RAW_BE, stvalid_status(h28)=0x%x, stready_status(h2c)=0x%x\n",
			vi_info->preraw_be.stvalid_status,
			vi_info->preraw_be.stready_status);
		vi_pr(VI_INFO, "RAW_TOP, stvalid_status(h40)=0x%x, stready_status(h44)=0x%x, dma_idle(h60)=0x%x\n",
			vi_info->rawtop.stvalid_status,
			vi_info->rawtop.stready_status,
			vi_info->rawtop.dma_idle);
		vi_pr(VI_INFO, "RGB_TOP, ip_stvalid_status(h50)=0x%x, ip_stready_status(h54)=0x%x\n",
			vi_info->rgbtop.ip_stvalid_status,
			vi_info->rgbtop.ip_stready_status);
		vi_pr(VI_INFO, "RGB_TOP, dmi_stvalid_status(h58)=0x%x, dmi_stready_status(h5c)=0x%x\n",
			vi_info->rgbtop.dmi_stvalid_status,
			vi_info->rgbtop.dmi_stready_status);
		vi_pr(VI_INFO, "RGB_TOP xcnt_rpt=0x%x, ycnt_rpt=0x%x\n",
			vi_info->rgbtop.xcnt_rpt,
			vi_info->rgbtop.ycnt_rpt);
		vi_pr(VI_INFO, "YUV_TOP debug_state(h18)=0x%x, stvalid_status(h6c)=0x%x, stready_status(h70)=0x%x\n",
			vi_info->yuvtop.debug_state,
			vi_info->yuvtop.stvalid_status,
			vi_info->yuvtop.stready_status);
		vi_pr(VI_INFO, "YUV_TOP xcnt_rpt=0x%x, ycnt_rpt=0x%x\n",
			vi_info->yuvtop.xcnt_rpt,
			vi_info->yuvtop.ycnt_rpt);
		vi_pr(VI_INFO, "rdma28, dbg_sel(h000)=0x%x, status(h014)=0x%x\n",
			vi_info->rdma28[0].dbg_sel,
			vi_info->rdma28[0].status);
		vi_pr(VI_INFO, "rdma28, dbg_sel(h000)=0x%x, status(h014)=0x%x\n",
			vi_info->rdma28[1].dbg_sel,
			vi_info->rdma28[1].status);
	}

	osal_kfree(g_overflow_info);
	g_overflow_info = NULL;
}

static void _vi_err_retrig_pre_fe(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num;
	enum sop_isp_fe_chn_num fe_max, fe_chn;
	unsigned long flags;
	u8 pipe = 0, chn = 0;
	s8 ret = ISP_SUCCESS;

	if (!_is_fe_post_offline(ctx))
		return;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_VIRT0; raw_num++) {
		if (!ctx->isp_csi_cfg[raw_num].is_enable)
			continue;
		if (osal_atomic_read(&vdev->isp_err_times[raw_num]) > 30) {
			vi_pr(VI_ERR, "raw_%d too much errors happened\n", raw_num);
			continue;
		}

		vi_pr(VI_WARN, "fe_%d trig retry %d times\n", raw_num, osal_atomic_read(&vdev->isp_err_times[raw_num]));

		//yuv sensor offline2sc
		if (ctx->isp_csi_cfg[raw_num].is_yuv_sensor &&
			ctx->isp_csi_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_BYPASS) {
			fe_max = (enum sop_isp_fe_chn_num)ctx->isp_csi_cfg[raw_num].mux_mode;
			for (fe_chn = ISP_FE_CH0; fe_chn <= fe_max; fe_chn++) {
				pipe = ctx->isp_csi_cfg[raw_num].bind_pipe[fe_chn];
				if (osal_atomic_read(&vdev->isp_err_times[raw_num])) {
					osal_spin_lock_irqsave(&vdev->qbuf_lock, &flags);
					vdev->qbuf_num[pipe][chn]++;
					osal_spin_unlock_irqrestore(&vdev->qbuf_lock, &flags);
				}

				if (sop_isp_rdy_buf_empty(vdev, pipe, chn)) {
					vi_pr(VI_INFO, "fe_%d chn_%d yuv bypass outbuf is empty\n", pipe, chn);
				} else {
					_isp_yuv_bypass_trigger(vdev, raw_num, fe_chn);
				}
			}
		} else { //rgb senosr or yuv sensor online2sc
			fe_max = ctx->isp_csi_cfg[raw_num].is_hdr_on
				     ? ISP_FE_CH1
				     : ctx->isp_csi_cfg[raw_num].mux_mode;
			for (fe_chn = ISP_FE_CH0; fe_chn <= fe_max; fe_chn++) {
				ret |= _pre_hw_enque(vdev, raw_num, fe_chn);
			}
		}
	}
}

void _vi_err_handler(struct sop_vi_dev *vdev, const enum sop_isp_raw err_raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num;
	enum sop_isp_fe_chn_num fe_chn;
	int count = 10;
	bool fe_idle, post_idle;
	uint8_t pipe;

	//step 1 : set frm vld = 0
	isp_frm_err_handler(ctx, err_raw_num, 1);

	//step 2 : wait to make sure post and the other fe is done.
	while (--count > 0) {
		if (_is_fe_post_offline(ctx)) {
			fe_idle = post_idle = true;

			for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
				if (!ctx->isp_csi_cfg[raw_num].is_enable)
					continue;
				if (osal_atomic_read(&vdev->isp_err_times[raw_num]) && count < 5)
					continue;
				for (fe_chn = ISP_FE_CH0; fe_chn < ISP_FE_CHN_MAX; fe_chn++) {
					if (osal_atomic_read(&vdev->pre_fe_state[raw_num][fe_chn]) != ISP_STATE_IDLE) {
						fe_idle = false;
						break;
					}
				}
			}

			if (!(osal_atomic_read(&vdev->postraw_state) == ISP_STATE_IDLE))
				post_idle = false;

			if (fe_idle && post_idle)
				break;

			vi_pr(VI_WARN, "wait fe/be/post idle count(%d) for be_post_online\n", count);
		} else {
			/*onthefly or slicebuf skip wait others fe done */
			break;
		}

		osal_usleep_range(5 * 1000, 10 * 1000);
	}

	//If fe/be/post not done;
	if (count == 0) {
		vi_pr(VI_ERR, "isp status fe_0(ch0:%d, ch1:%d, ch2:%d, ch3:%d)\n",
				osal_atomic_read(&vdev->pre_fe_state[ISP_PRERAW0][ISP_FE_CH0]),
				osal_atomic_read(&vdev->pre_fe_state[ISP_PRERAW0][ISP_FE_CH1]),
				osal_atomic_read(&vdev->pre_fe_state[ISP_PRERAW0][ISP_FE_CH2]),
				osal_atomic_read(&vdev->pre_fe_state[ISP_PRERAW0][ISP_FE_CH3]));
		vi_pr(VI_ERR, "isp status fe_1(ch0:%d, ch1:%d)\n",
				osal_atomic_read(&vdev->pre_fe_state[ISP_PRERAW1][ISP_FE_CH0]),
				osal_atomic_read(&vdev->pre_fe_state[ISP_PRERAW1][ISP_FE_CH1]));
		vi_pr(VI_ERR, "isp status fe_2(ch0:%d)\n",
				osal_atomic_read(&vdev->pre_fe_state[ISP_PRERAW2][ISP_FE_CH0]));
		vi_pr(VI_ERR, "isp status fe_lite0(ch0:%d, ch1:%d, ch2:%d, ch3:%d)\n",
				osal_atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE0][ISP_FE_CH0]),
				osal_atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE0][ISP_FE_CH1]),
				osal_atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE0][ISP_FE_CH2]),
				osal_atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE0][ISP_FE_CH3]));
		vi_pr(VI_ERR, "isp status postraw(%d)\n", osal_atomic_read(&vdev->postraw_state));
	}

	//step 3 : set csibdg sw abort and wait abort done
	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ctx->isp_csi_cfg[raw_num].is_enable)
			continue;

		if (isp_frm_err_handler(ctx, raw_num, 3) < 0)
			return;
	}

	//step 4 : isp sw reset and vip reset pull up
	isp_frm_err_handler(ctx, err_raw_num, 4);

	//send err cb to vpss if vpss online
	if (!_is_fe_post_offline(ctx) &&
		!ctx->isp_pipe_cfg[err_raw_num].is_offline_scaler) { //VPSS online
		struct vpss_online_err_handle_info err_cb = {0};

		/* VPSS Online error handle */
		err_cb.snr_num = err_raw_num;
		if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_ONLINE_ERR_HANDLE, &err_cb) != 0) {
			vi_pr(VI_ERR, "VPSS_CB_ONLINE_ERR_HANDLE is failed\n");
		}
	}

	//step 5 : isp sw reset and vip reset pull down
	isp_frm_err_handler(ctx, err_raw_num, 5);

	//step 6 : wait ISP idle
	if (isp_frm_err_handler(ctx, err_raw_num, 6) < 0)
		return;

	//step 7 : reset sw state to idle
	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		osal_atomic_set(&vdev->pre_fe_state[raw_num][ISP_FE_CH0], ISP_STATE_IDLE);
		osal_atomic_set(&vdev->pre_fe_state[raw_num][ISP_FE_CH1], ISP_STATE_IDLE);
		osal_atomic_set(&vdev->pre_fe_state[raw_num][ISP_FE_CH2], ISP_STATE_IDLE);
		osal_atomic_set(&vdev->pre_fe_state[raw_num][ISP_FE_CH3], ISP_STATE_IDLE);
	}
	osal_atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);

	//step 8 : set fbcd dma to hw mode if fbc is on
	if (ctx->is_fbc_on) {
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_ST_Y, false);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_ST_C, false);
	}

	//step 9 : reset first frame count
	for (fe_chn = ISP_FE_CH0; fe_chn < ISP_FE_CHN_MAX; fe_chn++) {
		pipe = ctx->isp_csi_cfg[err_raw_num].bind_pipe[fe_chn];
		ctx->isp_pipe_cfg[pipe].first_frm_cnt = 0;
		ctx->isp_pipe_cfg[pipe].first_frm_rst = true;
	}

	//step 10 : show overflow info
	_vi_show_debug_info();

	//Let postraw trigger go
	osal_atomic_set(&vdev->isp_err_handle_flag, 0);

	//retrig pre_fe
	_vi_err_retrig_pre_fe(vdev);
}

static int vi_err_wait_cond_func(const void *param)
{
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)param;
	enum E_VI_TH th_id = E_VI_TH_ERR_HANDLER;

	return osal_atomic_read(&vdev->vi_th[th_id].flag)
		|| osal_atomic_read(&vdev->vi_th[th_id].exit_flag);
}

static int _vi_err_handler_thread(void *arg)
{
	int ret = 0;
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)arg;
	enum sop_isp_raw err_raw_num = ISP_PRERAW0;
	enum E_VI_TH th_id = E_VI_TH_ERR_HANDLER;

	while (!osal_atomic_read(&vdev->vi_th[th_id].exit_flag)) {
		ret = osal_wait_timeout_uninterruptible(&vdev->vi_th[th_id].wq,
							vi_err_wait_cond_func,
							vdev,
							VI_TIMEOUT_MS);

		if (osal_atomic_read(&vdev->vi_th[th_id].exit_flag))
			break;

		if (ret <= 0)
			continue;

		if (osal_atomic_read(&vdev->vi_th[th_id].flag) != 0) {
			err_raw_num = osal_atomic_read(&vdev->vi_th[th_id].flag) - 1;
			osal_atomic_set(&vdev->vi_th[th_id].flag, 0);
		}

		if (vdev->vi_th[th_id].w_thread && vdev->vi_th[th_id].w_thread->task)
			_vi_err_handler(vdev, err_raw_num);
	}

	vi_pr(VI_INFO, "%s exit\n", vdev->vi_th[th_id].th_name);
	osal_atomic_set(&vdev->vi_th[th_id].thread_exit, 1);
	osal_kthread_exit();

	return 0;
}

static inline void vi_err_wake_up_th(struct sop_vi_dev *vdev, enum sop_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_fe_chn_num fe_chn = ISP_FE_CH0;
	enum sop_isp_fe_chn_num fe_max = ctx->isp_csi_cfg[raw_num].is_hdr_on
					? ISP_FE_CH1
					: ctx->isp_csi_cfg[raw_num].mux_mode;

	//record err_num;
	osal_atomic_inc(&vdev->isp_err_times[raw_num]);

	for (fe_chn = ISP_FE_CH0; fe_chn <= fe_max; fe_chn++) {
		vi_pr(VI_WARN, "dbg_0=0x%x, dbg_1=0x%x, dbg_2=0x%x, dbg_3=0x%x\n",
		      ctx->dg_info.bdg_dbg[raw_num].chn_dbg[fe_chn].dbg_0,
		      ctx->dg_info.bdg_dbg[raw_num].chn_dbg[fe_chn].dbg_1,
		      ctx->dg_info.bdg_dbg[raw_num].chn_dbg[fe_chn].dbg_2,
		      ctx->dg_info.bdg_dbg[raw_num].chn_dbg[fe_chn].dbg_3);
	}

	//Stop pre/postraw trigger go
	if (osal_atomic_read(&vdev->isp_err_handle_flag) == 1) {
		vi_pr(VI_WARN, "err_handler running\n");
		return;
	}

	osal_atomic_set(&vdev->isp_err_handle_flag, 1);
	osal_atomic_set(&vdev->vi_th[E_VI_TH_ERR_HANDLER].flag, raw_num + 1);

	osal_wait_wakeup(&vdev->vi_th[E_VI_TH_ERR_HANDLER].wq);
}

u32 isp_err_chk(
	struct sop_vi_dev *vdev,
	struct isp_ctx *ctx,
	union reg_isp_csi_bdg_interrupt_status_0 *cbdg_0_sts,
	union reg_isp_csi_bdg_interrupt_status_1 *cbdg_1_sts)
{
	u32 ret = 0;
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	enum sop_isp_fe_chn_num fe_chn = ISP_FE_CH0;
	struct _isp_dg_info *dg_info = &ctx->dg_info;

	//TODO need refator
	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ctx->isp_csi_cfg[raw_num].is_enable)
			continue;

		if (cbdg_1_sts[raw_num].bits.fifo_overflow_int) {
			vi_pr(VI_ERR, "CSIBDG_%d fifo overflow\n", raw_num);
			_vi_record_debug_info(ctx);
			dg_info->bdg_dbg[raw_num].bdg_fifo_of_cnt++;
			vi_err_wake_up_th(vdev, raw_num);
			ret = -1;
		}

		if (cbdg_1_sts[raw_num].bits.frame_resolution_over_max_int) {
			vi_pr(VI_ERR, "CSIBDG_%d frm size over max\n", raw_num);
			ret = -1;
		}

		if (cbdg_1_sts[raw_num].bits.dma_error_int) {
			u32 wdma_1_err = ctx->dg_info.dma_sts.wdma_1_err_sts;
			u32 wdma_2_err = ctx->dg_info.dma_sts.wdma_2_err_sts;
			u32 wdma_3_err = ctx->dg_info.dma_sts.wdma_3_err_sts;
			u32 rdma_1_err = ctx->dg_info.dma_sts.rdma_1_err_sts;
			u32 rdma_2_err = ctx->dg_info.dma_sts.rdma_2_err_sts;
			u32 rdma_3_err = ctx->dg_info.dma_sts.rdma_3_err_sts;
			u32 wdma_1_idle = ctx->dg_info.dma_sts.wdma_1_idle;
			u32 wdma_2_idle = ctx->dg_info.dma_sts.wdma_2_idle;
			u32 wdma_3_idle = ctx->dg_info.dma_sts.wdma_3_idle;
			u32 rdma_1_idle = ctx->dg_info.dma_sts.rdma_1_idle;
			u32 rdma_2_idle = ctx->dg_info.dma_sts.rdma_2_idle;
			u32 rdma_3_idle = ctx->dg_info.dma_sts.rdma_3_idle;

			if ((wdma_1_err & 0x10) || (wdma_2_err & 0x10) || (wdma_3_err & 0x10)
			|| (rdma_1_err & 0x10) || (rdma_2_err & 0x10) || (rdma_3_err & 0x10)) {
				vi_pr(VI_ERR, "DMA axi error\n");
				ret = -1;
			} else if ((wdma_1_err & 0x20) || (wdma_2_err & 0x20)
			|| (wdma_3_err & 0x20) || (rdma_1_err & 0x20)
			|| (rdma_2_err & 0x20) || (rdma_3_err & 0x20)) {
				vi_pr(VI_ERR, "DMA axi mismatch\n");
				ret = -1;
			} else if ((wdma_1_err & 0x40) || (wdma_2_err & 0x40)
			|| (wdma_3_err & 0x40)) {
				vi_pr(VI_WARN, "WDMA buffer full\n");
			}

			vi_pr(VI_WARN, "Err status wdma[1(0x%x) 2(0x%x) 3(0x%x)]\n",
					wdma_1_err, wdma_2_err, wdma_3_err);
			vi_pr(VI_WARN, "Err status rdma[1(0x%x) 2(0x%x) 3(0x%x)]",
					rdma_1_err, rdma_2_err, rdma_3_err);
			vi_pr(VI_WARN, "Idle status wdma[1(0x%x) 2(0x%x) 3(0x%x)]",
					wdma_1_idle, wdma_2_idle, wdma_3_idle);
			vi_pr(VI_WARN, "Idle status rdma[1(0x%x) 2(0x%x) 3(0x%x) ]\n",
					rdma_1_idle, rdma_2_idle, rdma_3_idle);
		}

		fe_chn = ISP_FE_CH0;

		if (cbdg_0_sts[raw_num].bits.ch0_frame_width_gt_int) {
			vi_pr(VI_ERR, "CSIBDG_%d CH%d frm width greater than setting(%d)\n",
					raw_num, fe_chn, ctx->isp_csi_cfg[raw_num].csibdg_width);
			dg_info->bdg_dbg[raw_num].bdg_w_gt_cnt[fe_chn]++;
			vi_err_wake_up_th(vdev, raw_num);
			ret = -1;
		}

		if (cbdg_0_sts[raw_num].bits.ch0_frame_width_ls_int) {
			vi_pr(VI_ERR, "CSIBDG_%d CH%d frm width less than setting(%d)\n",
					raw_num, fe_chn, ctx->isp_csi_cfg[raw_num].csibdg_width);
			dg_info->bdg_dbg[raw_num].bdg_w_ls_cnt[fe_chn]++;
			vi_err_wake_up_th(vdev, raw_num);
			ret = -1;
		}

		if (cbdg_0_sts[raw_num].bits.ch0_frame_height_gt_int) {
			vi_pr(VI_ERR, "CSIBDG_%d CH%d frm height greater than setting(%d)\n",
					raw_num, fe_chn, ctx->isp_csi_cfg[raw_num].csibdg_height);
			dg_info->bdg_dbg[raw_num].bdg_h_gt_cnt[fe_chn]++;
			vi_err_wake_up_th(vdev, raw_num);
			ret = -1;
		}

		if (cbdg_0_sts[raw_num].bits.ch0_frame_height_ls_int) {
			vi_pr(VI_ERR, "CSIBDG_%d CH%d frm height less than setting(%d)\n",
					raw_num, fe_chn, ctx->isp_csi_cfg[raw_num].csibdg_height);
			dg_info->bdg_dbg[raw_num].bdg_h_ls_cnt[fe_chn]++;
			vi_err_wake_up_th(vdev, raw_num);
			ret = -1;
		}

		if (ctx->isp_csi_cfg[raw_num].is_hdr_on ||
		    ctx->isp_csi_cfg[raw_num].mux_mode > VI_WORK_MODE_1MULTIPLEX) {
			fe_chn = ISP_FE_CH1;

			if (cbdg_0_sts[raw_num].bits.ch1_frame_width_gt_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm width greater than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_csi_cfg[raw_num].csibdg_width);
				dg_info->bdg_dbg[raw_num].bdg_w_gt_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts[raw_num].bits.ch1_frame_width_ls_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm width less than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_csi_cfg[raw_num].csibdg_width);
				dg_info->bdg_dbg[raw_num].bdg_w_ls_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts[raw_num].bits.ch1_frame_height_gt_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm height greater than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_csi_cfg[raw_num].csibdg_height);
				dg_info->bdg_dbg[raw_num].bdg_h_gt_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts[raw_num].bits.ch1_frame_height_ls_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm height less than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_csi_cfg[raw_num].csibdg_height);
				dg_info->bdg_dbg[raw_num].bdg_h_ls_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}
		}

		if (ctx->isp_csi_cfg[raw_num].mux_mode > VI_WORK_MODE_2MULTIPLEX) {
			fe_chn = ISP_FE_CH2;

			if (cbdg_0_sts[raw_num].bits.ch2_frame_width_gt_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm width greater than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_csi_cfg[raw_num].csibdg_width);
				dg_info->bdg_dbg[raw_num].bdg_w_gt_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts[raw_num].bits.ch2_frame_width_ls_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm width less than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_csi_cfg[raw_num].csibdg_width);
				dg_info->bdg_dbg[raw_num].bdg_w_ls_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts[raw_num].bits.ch2_frame_height_gt_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm height greater than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_csi_cfg[raw_num].csibdg_height);
				dg_info->bdg_dbg[raw_num].bdg_h_gt_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts[raw_num].bits.ch2_frame_height_ls_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm height less than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_csi_cfg[raw_num].csibdg_height);
				dg_info->bdg_dbg[raw_num].bdg_h_ls_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}
		}

		if (ctx->isp_csi_cfg[raw_num].mux_mode > VI_WORK_MODE_3MULTIPLEX) {
			fe_chn = ISP_FE_CH3;

			if (cbdg_0_sts[raw_num].bits.ch3_frame_width_gt_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm width greater than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_csi_cfg[raw_num].csibdg_width);
				dg_info->bdg_dbg[raw_num].bdg_w_gt_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts[raw_num].bits.ch3_frame_width_ls_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm width less than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_csi_cfg[raw_num].csibdg_width);
				dg_info->bdg_dbg[raw_num].bdg_w_ls_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts[raw_num].bits.ch3_frame_height_gt_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm height greater than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_csi_cfg[raw_num].csibdg_height);
				dg_info->bdg_dbg[raw_num].bdg_h_gt_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts[raw_num].bits.ch3_frame_height_ls_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm height less than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_csi_cfg[raw_num].csibdg_height);
				dg_info->bdg_dbg[raw_num].bdg_h_ls_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}
		}
	}

	return ret;
}

static int vi_post_wait_cond_func(const void *param)
{
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)param;
	enum E_VI_TH th_id = E_VI_TH_POSTRAW;

	return osal_atomic_read(&vdev->vi_th[th_id].flag)
		|| osal_atomic_read(&vdev->vi_th[th_id].exit_flag);
}

static int isp_post_tasklet(void *data)
{
	int ret = 0;
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)data;
	enum E_VI_TH th_id = E_VI_TH_POSTRAW;

	while (!osal_atomic_read(&vdev->vi_th[th_id].exit_flag)) {
		ret = osal_wait_timeout_uninterruptible(&vdev->vi_th[th_id].wq,
							vi_post_wait_cond_func,
							vdev,
							VI_TIMEOUT_MS);

		if (osal_atomic_read(&vdev->vi_th[th_id].exit_flag))
			break;

		if (ret <= 0)
			continue;

		osal_atomic_set(&vdev->vi_th[E_VI_TH_POSTRAW].flag, 0);

		if (vdev->vi_th[th_id].w_thread && vdev->vi_th[th_id].w_thread->task)
			_post_hw_enque(vdev);
	}

	vi_pr(VI_INFO, "%s exit\n", vdev->vi_th[th_id].th_name);
	osal_atomic_set(&vdev->vi_th[th_id].thread_exit, 1);
	osal_kthread_exit();
	return 0;
}

static int vi_preraw_wait_cond_func(const void *param)
{
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)param;
	enum E_VI_TH th_id = E_VI_TH_PRERAW;

	return osal_atomic_read(&vdev->vi_th[th_id].flag)
		|| osal_atomic_read(&vdev->vi_th[th_id].exit_flag);
}

static int _vi_preraw_thread(void *arg)
{
	int ret = 0;
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)arg;
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	struct isp_ctx *ctx = &vdev->ctx;
	struct osal_list_head *pos, *temp;
	struct _isp_raw_num_n *n[VI_MAX_LIST_NUM] = {0};
	unsigned long flags;
	u32 enq_num = 0, i = 0;
	u8 pipe = 0;
	enum E_VI_TH th_id = E_VI_TH_PRERAW;

	while (!osal_atomic_read(&vdev->vi_th[th_id].exit_flag)) {
		ret = osal_wait_timeout_uninterruptible(&vdev->vi_th[th_id].wq,
							vi_preraw_wait_cond_func,
							vdev,
							VI_TIMEOUT_MS);

		if (osal_atomic_read(&vdev->vi_th[th_id].exit_flag))
			break;

		if (ret <= 0)
			continue;

		osal_atomic_set(&vdev->vi_th[th_id].flag, 0);

		osal_spin_lock_irqsave(&raw_num_lock, &flags);
		osal_list_for_each_safe(pos, temp, &pre_raw_num_q.list) {
			n[enq_num] = osal_list_entry(pos, struct _isp_raw_num_n, list);
			if (++enq_num >= VI_MAX_LIST_NUM || n[enq_num] == NULL)
				break;
		}
		osal_spin_unlock_irqrestore(&raw_num_lock, &flags);

		for (i = 0; i < enq_num; i++) {
			if (!n[i])
				continue;
			raw_num = n[i]->raw_num;

			osal_spin_lock_irqsave(&raw_num_lock, &flags);
			osal_list_del_init(&n[i]->list);
			osal_kfree(n[i]);
			osal_spin_unlock_irqrestore(&raw_num_lock, &flags);

			pipe = ctx->isp_csi_cfg[raw_num].bind_pipe[0];
			if (ctx->is_rawreplay) {
				postraw_tuning_update(ctx, pipe);
				//Update postraw sts awb/dci/hist_edge_v dma size/addr
				_swap_post_sts_buf(ctx, pipe);
			} else {
				_isp_snr_cfg_deq_and_fire(vdev, raw_num, 0);

				pre_fe_tuning_update(&vdev->ctx, raw_num);

				//fe->be->dram->post or on the fly
				if (_is_all_online(ctx) || _is_fe_post_slice(ctx)) {
					postraw_tuning_update(ctx, pipe);
				}
			}

			if ((ctx->is_multi_sensor) && (!ctx->isp_csi_cfg[raw_num].is_yuv_sensor)) {
				pipe = tuning_dis[0] > 0 ? tuning_dis[0] - 1 : 0;
				if ((tuning_dis[0] > 0) && (ctx->isp_pipe_cfg[pipe].bind_raw != raw_num)) {
					vi_pr(VI_DBG, "raw_%d pipe_%d start drop\n",
							ctx->isp_pipe_cfg[pipe].bind_raw, pipe);
					ctx->isp_csi_cfg[raw_num].is_drop_next_frame = true;
				}
			}

			if (osal_atomic_read(&vdev->is_drop)) {
				vi_pr(VI_DBG, "raw_%d start drop\n", raw_num);
				ctx->isp_csi_cfg[raw_num].is_drop_next_frame = true;
			}

			if (ctx->isp_csi_cfg[raw_num].is_drop_next_frame) {
				//if !is_drop_next_frame, set is_drop_next_frame flags false;
				if (_is_drop_next_frame(vdev, raw_num, ISP_FE_CH0))
					++vdev->drop_frame_number[raw_num];
				else {
					vi_pr(VI_DBG, "raw_%d stop drop\n", raw_num);
					ctx->isp_pipe_cfg[pipe].isp_reset_frm =
						vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0] + 1;
					_clear_drop_frm_info(vdev, raw_num);
				}

				//vi onthefly and vpss online will trigger preraw in post_hw_enque
				if (_is_all_online(ctx) && !ctx->isp_pipe_cfg[pipe].is_offline_scaler)
					continue;

				//TODO raw_num maybe not RAW_)
				if (!ctx->is_rawreplay) {
					isp_trig_whole_preraw(vdev, raw_num);
				}
			}
		}

		enq_num = 0;
	}

	vi_pr(VI_INFO, "%s exit\n", vdev->vi_th[th_id].th_name);
	osal_atomic_set(&vdev->vi_th[th_id].thread_exit, 1);
	osal_kthread_exit();

	return 0;
}

static inline void _vi_wake_up_ai_isp_th(struct sop_vi_dev *vdev, enum sop_isp_raw raw_num)
{
	enum E_VI_TH th_id = E_VI_TH_AI_ISP;

	osal_atomic_set(&vdev->vi_th[th_id].flag, raw_num + 1);
	osal_wait_wakeup(&vdev->vi_th[th_id].wq);
}

static int vi_ai_isp_wait_cond_func(const void *param)
{
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)param;
	enum E_VI_TH th_id = E_VI_TH_AI_ISP;

	return osal_atomic_read(&vdev->vi_th[th_id].flag)
		|| osal_atomic_read(&vdev->vi_th[th_id].exit_flag);
}

static int vi_ai_isp_wait_tpu_cond(const void *param)
{
	osal_atomic *flag = (osal_atomic *)param;

	return osal_atomic_read(flag) == E_AI_WAKE_TYPE_AI_ISP_TH;
}

static void _isp_ai_isp_handler(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num)
{
	int ret = 0, timeout = 200;
	struct isp_ctx *ctx = &vdev->ctx;
	uint8_t pipe = ctx->isp_csi_cfg[raw_num].bind_pipe[ISP_FE_CH0];
	struct isp_buffer *in_buf = NULL, *out_buf = NULL;
	struct isp_queue *fe_out_q = NULL, *post_in_q = NULL;
	struct vi_ai_isp_info *ai_isp_info = &vdev->ai_isp_info[pipe];
	enum sop_isp_fe_chn_num fe_chn = ISP_FE_CH0;
	enum sop_isp_fe_chn_num fe_max = ctx->isp_csi_cfg[raw_num].is_hdr_on
					? ISP_FE_CH1
					: ISP_FE_CH0;
	bool is_skip_ai_isp = true;

	while (fe_chn <= fe_max) {
		in_buf = isp_buf_next(&vdev->pre_ai_isp_in_q[raw_num][fe_chn]);
		if (in_buf == NULL) {
			vi_pr(VI_DBG, "AI_ISP inbuf is empty for raw %d, fe_chn %d\n", raw_num, fe_chn);
			return;
		}

		out_buf = isp_buf_next(&vdev->pre_ai_isp_out_q[raw_num][fe_chn]);
		if (out_buf == NULL) {
			vi_pr(VI_DBG, "AI_ISP outbuf is empty for raw %d, fe_chn %d\n", raw_num, fe_chn);
			return;
		}

		ai_isp_info->input_addr[fe_chn] = in_buf->addr;
		ai_isp_info->output_addr[fe_chn] = out_buf->addr;
		ai_isp_info->size = in_buf->byr_size;
		fe_chn++;
	}

	if (osal_atomic_read(&ctx->isp_pipe_cfg[pipe].ai_isp_en)) {
		ai_isp_info->vi_pipe = pipe;

		osal_atomic_set(&vdev->isp_ai_int_flag[raw_num], E_AI_WAKE_TYPE_MW);
		osal_wait_wakeup(&vdev->isp_ai_wait_q[raw_num]);

		ret = osal_wait_timeout_uninterruptible(&vdev->isp_ai_wait_q[raw_num],
								vi_ai_isp_wait_tpu_cond,
								&vdev->isp_ai_int_flag[raw_num],
								timeout);

		is_skip_ai_isp = ((ret <= 0) || (in_buf->addr == ai_isp_info->output_addr[fe_chn - 1])
					 || !osal_atomic_read(&ctx->isp_pipe_cfg[pipe].ai_isp_en));
	}

	fe_chn = ISP_FE_CH0;
	while (fe_chn <= fe_max) {
		fe_out_q = &vdev->pre_fe_out_q[raw_num][fe_chn];
		post_in_q = (ctx->isp_csi_cfg[raw_num].is_hdr_on)
				? &vdev->postraw_wdr_in_q[fe_chn]
				: &vdev->postraw_in_q;

		in_buf = isp_buf_remove(&vdev->pre_ai_isp_in_q[raw_num][fe_chn]);
		if (in_buf == NULL) {
			vi_pr(VI_ERR, "AI_ISP inbuf is empty for raw %d, fe_chn %d\n", raw_num, fe_chn);
			continue;
		}

		if (is_skip_ai_isp) {
			isp_buf_queue(post_in_q, in_buf);
			vi_pr(VI_DBG, "AI_ISP skip raw %d, fe_chn %d\n", raw_num, fe_chn);
		} else {
			isp_buf_queue(fe_out_q, in_buf);
			out_buf = isp_buf_remove(&vdev->pre_ai_isp_out_q[raw_num][fe_chn]);
			if (out_buf == NULL) {
				vi_pr(VI_ERR, "AI_ISP outbuf is empty for raw %d, fe_chn %d\n", raw_num, fe_chn);
				continue;
			}
			isp_buf_queue(post_in_q, out_buf);
		}
		fe_chn++;
	}

	_vi_wake_up_post_th(vdev);
}

static int _vi_ai_isp_handler_thread(void *arg)
{
	int ret = 0;
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)arg;
	enum E_VI_TH th_id = E_VI_TH_AI_ISP;

	while (!osal_atomic_read(&vdev->vi_th[th_id].exit_flag)) {
		ret = osal_wait_timeout_uninterruptible(&vdev->vi_th[th_id].wq,
							vi_ai_isp_wait_cond_func,
							vdev,
							VI_TIMEOUT_MS);

		if (osal_atomic_read(&vdev->vi_th[th_id].exit_flag))
			break;

		if (ret <= 0)
			continue;

		raw_num = osal_atomic_read(&vdev->vi_th[th_id].flag) - 1;
		osal_atomic_set(&vdev->vi_th[th_id].flag, 0);

		if (vdev->vi_th[th_id].w_thread && vdev->vi_th[th_id].w_thread->task)
			_isp_ai_isp_handler(vdev, raw_num);
	}

	vi_pr(VI_INFO, "%s exit\n", vdev->vi_th[th_id].th_name);
	osal_atomic_set(&vdev->vi_th[th_id].thread_exit, 1);
	osal_kthread_exit();

	return 0;
}

static int vi_vblank_wait_cond_func(const void *param)
{
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)param;
	enum E_VI_TH th_id = E_VI_TH_VBLANK_HANDLER;

	return osal_atomic_read(&vdev->vi_th[th_id].flag)
		|| osal_atomic_read(&vdev->vi_th[th_id].exit_flag);
}

static int _vi_vblank_handler_thread(void *arg)
{
	int ret = 0;
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)arg;
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	enum E_VI_TH th_id = E_VI_TH_VBLANK_HANDLER;

	while (!osal_atomic_read(&vdev->vi_th[th_id].exit_flag)) {
		ret = osal_wait_timeout_uninterruptible(&vdev->vi_th[th_id].wq,
							vi_vblank_wait_cond_func,
							vdev,
							VI_TIMEOUT_MS);

		if (osal_atomic_read(&vdev->vi_th[th_id].exit_flag))
			break;

		if (ret <= 0)
			continue;

		raw_num = osal_atomic_read(&vdev->vi_th[th_id].flag) - 1;

		osal_atomic_set(&vdev->vi_th[th_id].flag, 0);

		if (vdev->vi_th[th_id].w_thread && vdev->vi_th[th_id].w_thread->task)
			_isp_snr_cfg_deq_and_fire(vdev, raw_num, 1);
	}

	vi_pr(VI_INFO, "%s exit\n", vdev->vi_th[th_id].th_name);
	osal_atomic_set(&vdev->vi_th[th_id].thread_exit, 1);
	osal_kthread_exit();

	return 0;
}

static inline void _vi_wake_up_vblank_th(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num)
{
	enum E_VI_TH th_id = E_VI_TH_VBLANK_HANDLER;

	osal_atomic_set(&vdev->vi_th[th_id].flag, raw_num + 1);
	osal_wait_wakeup(&vdev->vi_th[th_id].wq);
}

static void _isp_yuv_online_handler(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num, const u8 hw_chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;

	if (_is_fe_post_offline(ctx)) { //fe->dram->post
		struct isp_buffer *b = NULL;
		struct isp_queue *fe_out_q, *post_in_q;

		fe_out_q = &vdev->pre_fe_out_q[raw_num][hw_chn_num];
		post_in_q = &vdev->postraw_in_q;

		b = isp_buf_remove(fe_out_q);
		if (b == NULL) {
			vi_pr(VI_DBG, "YUV_ISP Pre_fe_%d chn_num_%d outbuf is empty\n", raw_num, hw_chn_num);
			return;
		}

		b->crop.x = 0;
		b->crop.y = 0;
		b->crop.w = ctx->isp_csi_cfg[raw_num].crop[hw_chn_num].w;
		b->crop.h = ctx->isp_csi_cfg[raw_num].crop[hw_chn_num].h;
		b->raw_num = raw_num;
		b->chn_num = 0;
		b->pipe = vi_get_pipe_by_raw_chn(ctx, raw_num, hw_chn_num);
		osal_gettimeofday(&b->tv);

		isp_buf_queue(post_in_q, b);

		osal_atomic_set(&vdev->pre_fe_state[raw_num][hw_chn_num], ISP_STATE_IDLE);

		vi_pr(VI_DBG, "YUV fe->dram->yuv raw_num=%d frame number=%d out buf_addr=0x%llx\n",
			  raw_num, vdev->pre_fe_frm_num[raw_num][hw_chn_num], b->addr);

		_vi_wake_up_post_th(vdev);
		_vi_wake_up_vblank_th(vdev, raw_num);

		if (!ctx->is_rawreplay) {
			_pre_hw_enque(vdev, raw_num, hw_chn_num);
		}
	}
}

static void _vi_wake_up_event_th(struct sop_vi_dev *vdev, const u8 pipe)
{
	enum E_VI_TH th_id = E_VI_TH_EVENT_HANDLER;

	if (!osal_atomic_read(&vdev->stream.isp_streamon[pipe]))
		return;

	osal_atomic_set(&vdev->vi_th[th_id].flag, pipe + 1);
	osal_wait_wakeup(&vdev->vi_th[th_id].wq);
}

static void _isp_yuv_bypass_handler(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num, const u8 hw_chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	u8 pipe = ctx->isp_csi_cfg[raw_num].bind_pipe[hw_chn_num];
	u8 chn = 0;
	osal_timeval osal_tv;

	osal_atomic_set(&vdev->pre_fe_state[raw_num][hw_chn_num], ISP_STATE_IDLE);

	sop_isp_rdy_buf_remove(vdev, pipe, chn);

	osal_gettimeofday(&osal_tv);
	sop_isp_dqbuf_list(vdev, vdev->pre_fe_frm_num[raw_num][hw_chn_num],
			   pipe, chn, osal_tv);

	_vi_wake_up_event_th(vdev, pipe);

	_isp_yuv_bypass_trigger(vdev, raw_num, hw_chn_num);
}

static void _isp_sof_handler(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct _isp_dqbuf_n *n = NULL;
	unsigned long flags;

	if (osal_atomic_read(&vdev->stream.csi_streamon[raw_num]) == 0)
		return;

	if (!(_is_fe_post_slice(ctx)) || ctx->isp_csi_cfg[raw_num].is_drop_next_frame)
		_vi_wake_up_preraw_th(vdev, raw_num);

	_vi_wake_up_post_th(vdev);

	osal_spin_lock_irqsave(&dq_lock, &flags);
	if (!osal_list_empty(&dqbuf_q.list)) {
		n = osal_list_first_entry(&dqbuf_q.list, struct _isp_dqbuf_n, list);
		_vi_wake_up_event_th(vdev, n->pipe_id);
	}
	osal_spin_unlock_irqrestore(&dq_lock, &flags);

	isp_sync_task_process(raw_num);

	vi_event_queue(vdev, VI_EVENT_PRE0_SOF + ctx->isp_csi_cfg[raw_num].bind_pipe[ISP_FE_CH0],
						vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0]);
}

/*call by fe done*/
static inline void _swap_pre_fe_sts_buf(
		struct isp_ctx *ctx,
		const enum sop_isp_raw raw_num,
		const enum sop_isp_fe_chn_num chn_num)
{
	struct _membuf *pool;
	unsigned long flags;
	u8 idx;
	enum sop_isp_raw phy_raw;
	u32 ae_dmaid;
	u64 ae_phy_addr;

	pool = &ctx->csi_bufpool[raw_num];

	osal_spin_lock_irqsave(&pool->pre_fe_sts_lock, &flags);
	if (pool->pre_fe_sts_in_use == 1) {
		osal_spin_unlock_irqrestore(&pool->pre_fe_sts_lock, &flags);
		vi_pr(VI_DBG, "post sts is in use\n");
		return;
	}

	if (chn_num == ISP_FE_CH0) {
		pool->pre_fe_sts_busy_idx ^= 1;
	}

	osal_spin_unlock_irqrestore(&pool->pre_fe_sts_lock, &flags);

	idx = pool->pre_fe_sts_busy_idx;

	phy_raw = find_phy_raw_num(ctx, raw_num);
	ae_dmaid = ae_dma_find_hwid(phy_raw, chn_num);
	if (chn_num == ISP_FE_CH0) {
		ae_phy_addr = pool->sts_mem[idx].ae_le.phy_addr;
	} else if (chn_num == ISP_FE_CH1) {
		ae_phy_addr = pool->sts_mem[idx].ae_se.phy_addr;
	} else {
		vi_pr(VI_ERR, "invalid chn_num %d\n", chn_num);
		return;
	}

	ispblk_dma_setaddr(ctx, ae_dmaid, ae_phy_addr);
}

static enum sop_isp_raw _convert_cur_rtn_next(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num,
	enum sop_isp_raw *cur_raw)
{
	u8 cur_idx = 0;
	u8 next_idx = 0;
	enum sop_isp_raw next_raw = raw_num;
	struct _csi_switch_info *info = &ctx->isp_csi_cfg[raw_num].switch_info;

	if (!ctx->isp_csi_cfg[raw_num].is_mux_dev) {
		*cur_raw = raw_num;
		return raw_num;
	}

	cur_idx = info->cur_idx;
	*cur_raw = info->snr_arr[cur_idx].cur_raw;

	next_idx = (cur_idx + 1) % info->cur_nums;
	next_raw = info->snr_arr[next_idx].cur_raw;

	return next_raw;
}

static void _mux_dev_cfg_gpio(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	u8 i = 0;
	u8 next_idx = 0;
	struct csi_gpio *gpio = NULL;
	void *handle = NULL;
	struct _csi_switch_info *info = &ctx->isp_csi_cfg[raw_num].switch_info;
	enum sop_isp_raw next_raw = raw_num;

	if (!ctx->isp_csi_cfg[raw_num].is_mux_dev)
		return;

	next_idx = (info->cur_idx + 1) % info->cur_nums;
	next_raw = info->snr_arr[next_idx].cur_raw;

	for (i = 0; i < VI_MAX_DEV_SWITCH_DEPTH; i++) {
		handle = info->snr_arr[0].gpio[i].handle;
		gpio = &info->snr_arr[next_idx].gpio[i];
		if (!gpio->enable)
			continue;
		vi_misc_gpio_set_value(&handle, gpio->port, gpio->pin, gpio->pol);
		vi_pr(VI_DBG, "next_idx(%d), next_raw(%d) pin(%d,%d), pol(%d)\n",
				next_idx, next_raw, gpio->port, gpio->pin, gpio->pol);
	}

	info->pre_idx = info->cur_idx;
	info->cur_idx = next_idx;
	ispblk_csibdg_crop_update(ctx, next_raw, false);
	ispblk_csibdg_update_size(ctx, next_raw);
	ispblk_csibdg_wdma_crop_config(ctx, next_raw, ctx->isp_csi_cfg[next_raw].crop[ISP_FE_CH0], true);
}

static inline void _isp_pre_fe_done_handler(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const enum sop_isp_fe_chn_num chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw cur_raw = raw_num;
	enum sop_isp_raw next_raw = raw_num;
	bool trigger = false;

	next_raw = _convert_cur_rtn_next(ctx, raw_num, &cur_raw);

	++vdev->pre_fe_frm_num[cur_raw][chn_num];

	//reset error times when fe_done
	if (osal_atomic_read(&vdev->isp_err_times[raw_num])) {
		osal_atomic_set(&vdev->isp_err_times[raw_num], 0);
	}

	if (ctx->isp_csi_cfg[cur_raw].is_yuv_sensor) {
		if (ctx->isp_csi_cfg[cur_raw].yuv_scene_mode == ISP_YUV_SCENE_BYPASS) {
			vi_pr(VI_DBG, "pre_fe_%d yuv bypass done chn_num=%d frm_num=%d\n",
					cur_raw, chn_num, vdev->pre_fe_frm_num[cur_raw][chn_num]);
			_isp_yuv_bypass_handler(vdev, cur_raw, chn_num);
		} else if (ctx->isp_csi_cfg[cur_raw].yuv_scene_mode == ISP_YUV_SCENE_ONLINE) {
			vi_pr(VI_DBG, "pre_fe_%d yuv online done chn_num=%d frm_num=%d\n",
					cur_raw, chn_num, vdev->pre_fe_frm_num[cur_raw][chn_num]);
			_isp_yuv_online_handler(vdev, cur_raw, chn_num);
		} else if (ctx->isp_csi_cfg[cur_raw].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			vi_pr(VI_DBG, "pre_fe_%d yuv isp done chn_num=%d frm_num=%d\n",
					cur_raw, chn_num, vdev->pre_fe_frm_num[cur_raw][chn_num]);
			_isp_yuv_online_handler(vdev, cur_raw, chn_num);
		} else {
			vi_pr(VI_ERR, "pre_fe_%d isp_domain error, yuv_scene_mode=%d\n",
					cur_raw, ctx->isp_csi_cfg[cur_raw].yuv_scene_mode);
		}
		return;
	}

	vi_pr(VI_DBG, "pre_fe_%d frm_done chn_num=%d frm_num=%d\n",
			cur_raw, chn_num, vdev->pre_fe_frm_num[cur_raw][chn_num]);

	if (_is_fe_post_slice(ctx) || _is_all_online(ctx)) { //fe->slice->post or on the fly mode
		if (osal_atomic_read(&vdev->raw_dump[cur_raw].raw_dump_en[chn_num]) == RAWDUMP_DONE) {
			struct isp_buffer *b = NULL;
			struct isp_queue *fe_out_q = &vdev->raw_dump[cur_raw].buf_q[chn_num];
			struct isp_queue *raw_d_q = &vdev->raw_dump[cur_raw].buf_dq[chn_num];
			u32 x, y, w, h, dmaid;

			if (ctx->isp_csi_cfg[cur_raw].rawdump_crop[chn_num].w &&
				ctx->isp_csi_cfg[cur_raw].rawdump_crop[chn_num].h) {
				x = ctx->isp_csi_cfg[cur_raw].rawdump_crop[chn_num].x;
				y = ctx->isp_csi_cfg[cur_raw].rawdump_crop[chn_num].y;
				w = ctx->isp_csi_cfg[cur_raw].rawdump_crop[chn_num].w;
				h = ctx->isp_csi_cfg[cur_raw].rawdump_crop[chn_num].h;
			} else {
				x = 0;
				y = 0;
				w = ctx->isp_csi_cfg[cur_raw].crop[chn_num].w;
				h = ctx->isp_csi_cfg[cur_raw].crop[chn_num].h;
			}

			dmaid = csibdg_dma_find_hwid(raw_num, chn_num);

			if (chn_num == ISP_FE_CH0)
				++vdev->dump_frame_number[cur_raw];

			ispblk_csidbg_dma_wr_en(ctx, raw_num, chn_num, 0);
			ispblk_clsc_config(ctx, cur_raw, chn_num,
						osal_atomic_read(&ctx->isp_csi_cfg[cur_raw].clsc_en[chn_num]));

			b = isp_buf_remove(fe_out_q);
			if (b == NULL) {
				vi_pr(VI_ERR, "Pre_fe_%d chn_num_%d outbuf is empty\n", cur_raw, chn_num);
				return;
			}

			b->crop.x = x;
			b->crop.y = y;
			b->crop.w = w;
			b->crop.h = h;
			b->byr_size = ispblk_dma_get_size(ctx, dmaid, w, h);
			b->frm_num = vdev->pre_fe_frm_num[cur_raw][chn_num];
			b->pipe = vi_get_pipe_by_raw_chn(ctx, cur_raw, chn_num);

			isp_buf_queue(raw_d_q, b);
		}
	} else if (_is_fe_post_offline(ctx)) { //fe->dram->be->post
		struct isp_buffer *b = NULL, *b_dup = NULL;
		struct isp_queue *fe_out_q, *post_in_q, *raw_d_q;
		struct vb_s *vb = NULL;

		fe_out_q = &vdev->pre_fe_out_q[raw_num][chn_num];
		post_in_q = (ctx->isp_csi_cfg[cur_raw].is_hdr_on) ? &vdev->postraw_wdr_in_q[chn_num]
								   : &vdev->postraw_in_q;

		if (osal_atomic_read(&vdev->raw_dump[cur_raw].raw_dump_en[chn_num]) == RAWDUMP_DONE)
			fe_out_q = &vdev->raw_dump[cur_raw].buf_q[chn_num];

		b = isp_buf_remove(fe_out_q);
		if (b == NULL) {
			vi_pr(VI_ERR, "Pre_fe_%d chn_num_%d outbuf is empty\n", cur_raw, chn_num);
			return;
		}

		b->chn_num = chn_num;
		osal_gettimeofday(&b->tv);
		b->pipe = vi_get_pipe_by_raw_chn(ctx, cur_raw, chn_num);

		if (ctx->is_ai_isp)
			post_in_q = &vdev->pre_ai_isp_in_q[raw_num][chn_num];

		if (osal_atomic_read(&vdev->raw_dump[cur_raw].raw_dump_en[chn_num]) == RAWDUMP_DONE) {
			u32 w = ctx->isp_csi_cfg[cur_raw].crop[chn_num].w;
			u32 h = ctx->isp_csi_cfg[cur_raw].crop[chn_num].h;
			u32 dmaid = csibdg_dma_find_hwid(raw_num, chn_num);

			if (chn_num == ISP_FE_CH0)
				++vdev->dump_frame_number[cur_raw];

			b->crop.x = 0;
			b->crop.y = 0;
			b->crop.w = w;
			b->crop.h = h;
			b->byr_size = ispblk_dma_get_size(ctx, dmaid, w, h);
			b->frm_num = vdev->pre_fe_frm_num[cur_raw][chn_num];

			ispblk_clsc_config(ctx, cur_raw, chn_num,
						osal_atomic_read(&ctx->isp_csi_cfg[cur_raw].clsc_en[chn_num]));

			if (osal_atomic_read(&vdev->raw_dump[cur_raw].isp_smooth_raw_dump_en) != SMOOTH_RAWDUMP_STOP) {
				b_dup = osal_kzalloc(sizeof(struct isp_buffer), OSAL_GFP_ATOMIC);
			}

			if (b_dup) {
				*b_dup = *b;
				b_dup->is_ext = EXTERNAL_BUFFER;
				vb = (struct vb_s *)(uintptr_t)b->vb_blk;
				osal_atomic_inc_return(&vb->usr_cnt);
				osal_atomic_set(&vb->mod_ids, BIT(ID_ISP));
				isp_buf_queue(post_in_q, b_dup);
			}

			raw_d_q = &vdev->raw_dump[cur_raw].buf_dq[chn_num];

			isp_buf_queue(raw_d_q, b);
		} else {
			isp_buf_queue(post_in_q, b);
		}
	}

	osal_atomic_set(&vdev->pre_fe_state[raw_num][chn_num], ISP_STATE_IDLE);

	_swap_pre_fe_sts_buf(ctx, next_raw, chn_num);

	if (ctx->isp_csi_cfg[cur_raw].is_hdr_on) {
		trigger = (vdev->pre_fe_frm_num[cur_raw][ISP_FE_CH0] ==
				vdev->pre_fe_frm_num[cur_raw][ISP_FE_CH1]);
	} else
		trigger = true;

	if (trigger) {
		vi_event_queue(vdev, VI_EVENT_PRE0_EOF + vi_get_pipe_by_raw_chn(ctx, cur_raw, chn_num),
				vdev->pre_fe_frm_num[cur_raw][chn_num]);

		_vi_wake_up_vblank_th(vdev, cur_raw);

		if (osal_atomic_read(&vdev->raw_dump[cur_raw].raw_dump_en[chn_num]) == RAWDUMP_DONE) {
			_isp_raw_dump_chk(vdev, cur_raw, vdev->pre_fe_frm_num[cur_raw][chn_num]);
		}

		if (_is_fe_post_offline(ctx)) {
			if (ctx->is_ai_isp)
				_vi_wake_up_ai_isp_th(vdev, cur_raw);
			else
				_vi_wake_up_post_th(vdev);
		}

	}

	if (_is_fe_post_offline(ctx) && !ctx->is_rawreplay) {
		_mux_dev_cfg_gpio(ctx, raw_num);
		_pre_hw_enque(vdev, next_raw, chn_num);
		vi_pr(VI_DBG, "fe->dram->be->post trigger raw_num=%d\n", next_raw);
	}
}

static void _isp_postraw_shaw_done_handler(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;

	if (_is_fe_post_slice(ctx)) {
		vi_pr(VI_INFO, "postraw shaw done\n");
		_vi_wake_up_preraw_th(vdev, ctx->cfg_info.raw_num);
	}
}

static void _isp_postraw_done_handler(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num = ctx->cfg_info.raw_num;
	enum sop_isp_raw phy_raw, next_raw;
	enum sop_isp_fe_chn_num chn_num = ctx->cfg_info.chn_num;
	uint8_t pipe = ctx->cfg_info.pipe;

	if (_isp_clk_dynamic_en(vdev, false) < 0)
		return;

	++ctx->isp_pipe_cfg[pipe].first_frm_cnt;
	++vdev->postraw_frame_number[pipe];

	if (_is_fe_post_offline(ctx) && !ctx->is_rawreplay) { //fe->dram->post
		struct isp_queue *post_in_le_q = ctx->cfg_info.is_hdr_on
						? &vdev->postraw_wdr_in_q[ISP_FE_CH0]
						: &vdev->postraw_in_q;
		struct isp_queue *pre_out_le_q = NULL, *pre_out_se_q = NULL;
		struct isp_buffer *b = NULL;

		b = isp_buf_remove(post_in_le_q);
		if (b == NULL) {
			vi_pr(VI_ERR, "post_in_le_q is empty 0x%p\n", post_in_le_q);
			return;
		}
		if (b->raw_num >= ISP_PRERAW_MAX) {
			vi_pr(VI_ERR, "buf raw_num_%d is wrong\n", b->raw_num);
			return;
		}
		raw_num = b->raw_num;
		pipe = b->pipe;
		chn_num = b->chn_num;

		if (b->source == PRE_FE) {
			pre_out_le_q = &vdev->pre_fe_out_q[raw_num][ISP_FE_CH0];
			pre_out_se_q = &vdev->pre_fe_out_q[raw_num][ISP_FE_CH1];
		} else {
			pre_out_le_q = &vdev->pre_ai_isp_out_q[raw_num][ISP_FE_CH0];
			pre_out_se_q = &vdev->pre_ai_isp_out_q[raw_num][ISP_FE_CH1];
		}

		if (b->is_ext == EXTERNAL_BUFFER)
			isp_raw_dump_vb_queue(vdev, b, true);
		else
			isp_buf_queue(pre_out_le_q, b);

		if (ctx->isp_pipe_cfg[pipe].is_hdr_on) {
			b = isp_buf_remove(&vdev->postraw_wdr_in_q[ISP_FE_CH1]);
			if (b == NULL) {
				vi_pr(VI_ERR, "[ERROR] post_in_se_q is empty\n");
				return;
			}

			if (b->is_ext == EXTERNAL_BUFFER)
				isp_raw_dump_vb_queue(vdev, b, false);
			else
				isp_buf_queue(pre_out_se_q, b);
		}
	} else if (_is_all_online(ctx) || (_is_fe_post_slice(ctx))) {
		_post_dma_update(vdev, pipe);
		//Update postraw stt gms/ae/hist_edge_v dma size/addr
		_swap_post_sts_buf(ctx, pipe);

		//Change post done flag to be true
		if (_is_fe_post_slice(ctx))
			osal_atomic_set(&ctx->is_post_done, 1);
	}

	osal_atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);

	ctx->isp_pipe_cfg[pipe].cnr_pre_scale_shift = ctx->isp_pipe_cfg[pipe].cnr_cur_scale_shift;
	ctx->isp_pipe_cfg[pipe].cnr_cur_scale_shift = ctx->isp_pipe_cfg[pipe].cnr_scale_shift;

	vi_pr(VI_DBG, "Raw_%d Postraw_%d frm_done frm_num=%d\n", raw_num, pipe, vdev->postraw_frame_number[pipe]);

	if (!ctx->is_rawreplay) {
		_vi_wake_up_post_th(vdev);

		if (_is_fe_post_offline(ctx)) {
			phy_raw = find_phy_raw_num(ctx, raw_num);
			/*why use cur_raw but not next raw, because we swap idx at fe_done*/
			_convert_cur_rtn_next(ctx, phy_raw, &next_raw);
			isp_trig_whole_preraw(vdev, next_raw);
		}
	}

	if (!ctx->isp_pipe_cfg[pipe].is_yuv_sensor) { //ISP team no need yuv post done
		vi_event_queue(vdev, VI_EVENT_POST0_EOF + pipe, vdev->postraw_frame_number[pipe]);
	}

	if (ctx->isp_pipe_cfg[pipe].is_offline_scaler && ctx->isp_pipe_cfg[pipe].chn_enable) {
		sop_isp_rdy_buf_remove(vdev, pipe, chn_num);
		sop_isp_dqbuf_list(vdev, vdev->postraw_frame_number[pipe],
				   pipe, chn_num,
				   ctx->isp_pipe_cfg[pipe].tv);
		_vi_wake_up_event_th(vdev, pipe);
	}
}

static void _isp_cmdq_done_chk(struct sop_vi_dev *vdev, const u32 cmdq_intr)
{
	uintptr_t cmdq = vdev->ctx.phys_regs[ISP_BLK_ID_CMDQ];

	if (cmdq_intr & BIT(0)) {
		u8 cmdq_intr_stat = cmdq_intr_status(cmdq);

		vi_pr(VI_DBG, "cmdq_intr 0x%08x\n", cmdq_intr_stat);
		cmdq_intr_clr(cmdq, cmdq_intr_stat);
	}
}

static void _isp_csibdg_lite_frame_start_chk(struct sop_vi_dev *vdev,
					     const enum sop_isp_raw raw_num,
					     const u32 frame_start)
{
	if (frame_start & BIT(1)) {
		vi_record_sof_perf(vdev, raw_num, ISP_FE_CH0);

		++vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0];

		vi_pr(VI_INFO, "csibgd_lite_%d sof chn_num=%d frm_num=%d\n",
			raw_num - ISP_PRERAW_LITE0, ISP_FE_CH0, vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0]);
	}

	if (frame_start & BIT(9))
		++vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH1];

	if (frame_start & BIT(17))
		++vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH2];

	if (frame_start & BIT(25))
		++vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH3];
}

static void _isp_csibdg_lite_frame_done_chk(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const u32 frame_start)
{
	if (frame_start & BIT(3)) {
		vi_record_fe_perf(vdev, raw_num, ISP_FE_CH0);

		++vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0];
		_isp_pre_fe_done_handler(vdev, raw_num, ISP_FE_CH0);
	}

	if (frame_start & BIT(11)) {
		++vdev->pre_fe_frm_num[raw_num][ISP_FE_CH1];
		_isp_pre_fe_done_handler(vdev, raw_num, ISP_FE_CH1);
	}

	if (frame_start & BIT(19)) {
		++vdev->pre_fe_frm_num[raw_num][ISP_FE_CH2];
		_isp_pre_fe_done_handler(vdev, raw_num, ISP_FE_CH2);
	}

	if (frame_start & BIT(27)) {
		++vdev->pre_fe_frm_num[raw_num][ISP_FE_CH3];
		_isp_pre_fe_done_handler(vdev, raw_num, ISP_FE_CH3);
	}
}

static void _isp_csibdg_line_cnt_done_chk(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	union reg_isp_csi_bdg_interrupt_status_1 cbdg_1_sts)
{
	if (cbdg_1_sts.bits.ch0_precrop_line_intp_int) {
		_isp_pre_fe_done_handler(vdev, raw_num, ISP_FE_CH0);
	}

	if (cbdg_1_sts.bits.ch1_precrop_line_intp_int) {
		_isp_pre_fe_done_handler(vdev, raw_num, ISP_FE_CH1);
	}

	if (cbdg_1_sts.bits.ch2_precrop_line_intp_int) {
		_isp_pre_fe_done_handler(vdev, raw_num, ISP_FE_CH2);
	}

	if (cbdg_1_sts.bits.ch3_precrop_line_intp_int) {
		_isp_pre_fe_done_handler(vdev, raw_num, ISP_FE_CH3);
	}
}

static void _isp_pre_fe_frame_start_chk(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const u8 frame_start)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw cur_raw;

	_convert_cur_rtn_next(ctx, raw_num, &cur_raw);

	if (frame_start & BIT(0)) {
		vi_record_sof_perf(vdev, cur_raw, ISP_FE_CH0);

		if (!ctx->is_rawreplay)
			++vdev->pre_fe_sof_cnt[cur_raw][ISP_FE_CH0];

		vi_pr(VI_DBG, "pre_fe_%d sof chn_num=%d frm_num=%d\n",
			cur_raw, ISP_FE_CH0, vdev->pre_fe_sof_cnt[cur_raw][ISP_FE_CH0]);

		if (ctx->isp_csi_cfg[cur_raw].is_yuv_sensor) { //YUV sensor
			if (ctx->isp_csi_cfg[cur_raw].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
				_vi_wake_up_post_th(vdev);
			}
		} else { //RGB sensor
			if (!ctx->is_rawreplay) {
				_isp_sof_handler(vdev, cur_raw);
			}
		}

		if (osal_atomic_read(&vdev->raw_dump[cur_raw].raw_dump_en[ISP_FE_CH0]) == RAWDUMP_PREPARE_DONE)
			osal_atomic_set(&vdev->raw_dump[cur_raw].raw_dump_en[ISP_FE_CH0], RAWDUMP_DONE);
	}

	if (frame_start & BIT(1)) {
		++vdev->pre_fe_sof_cnt[cur_raw][ISP_FE_CH1];
		if (osal_atomic_read(&vdev->raw_dump[cur_raw].raw_dump_en[ISP_FE_CH1]) == RAWDUMP_PREPARE_DONE)
			osal_atomic_set(&vdev->raw_dump[cur_raw].raw_dump_en[ISP_FE_CH1], RAWDUMP_DONE);
	}

	if (frame_start & BIT(2))
		++vdev->pre_fe_sof_cnt[cur_raw][ISP_FE_CH2];

	if (frame_start & BIT(3))
		++vdev->pre_fe_sof_cnt[cur_raw][ISP_FE_CH3];
}

static void _isp_pre_fe_frame_done_chk(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const u8 frame_done)
{
	//struct isp_ctx *ctx = &vdev->ctx;

	//for mux device, we need to check the frame done by csibdg
	//if (ctx->isp_csi_cfg[raw_num].is_mux_dev)
	//	return;

	if (frame_done & BIT(0)) {
		vi_record_fe_perf(vdev, raw_num, ISP_FE_CH0);

		_isp_pre_fe_done_handler(vdev, raw_num, ISP_FE_CH0);
	}

	if (frame_done & BIT(1)) {
		_isp_pre_fe_done_handler(vdev, raw_num, ISP_FE_CH1);
	}

	if (frame_done & BIT(2)) {
		_isp_pre_fe_done_handler(vdev, raw_num, ISP_FE_CH2);
	}

	if (frame_done & BIT(3)) {
		_isp_pre_fe_done_handler(vdev, raw_num, ISP_FE_CH3);
	}
}

static void _isp_postraw_shadow_done_chk(
	struct sop_vi_dev *vdev,
	const u8 shadow_done)
{
	if (shadow_done)
		_isp_postraw_shaw_done_handler(vdev);
}

static void _isp_postraw_frame_done_chk(
	struct sop_vi_dev *vdev,
	const u8 frame_done)
{
	struct isp_ctx *ctx = &vdev->ctx;

	if (frame_done) {
		vi_record_post_end(vdev, ctx->cfg_info.pipe);

		_isp_postraw_done_handler(vdev);
	}
}

void vi_irq_handler(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	union reg_isp_csi_bdg_interrupt_status_0 cbdg_0_sts[ISP_PRERAW_MAX] = { 0 };
	union reg_isp_csi_bdg_interrupt_status_1 cbdg_1_sts[ISP_PRERAW_MAX] = { 0 };
	union reg_isp_top_int_event0 top_sts_0;
	union reg_isp_top_int_event1 top_sts_1;
	union reg_isp_top_int_event2 top_sts_2;

	u8 i = 0, raw_num = ISP_PRERAW0;

	isp_intr_status(ctx, &top_sts_0, &top_sts_1, &top_sts_2);

	vi_perf_record_dump();

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_VIRT0; raw_num++) {
		if (!ctx->isp_csi_cfg[raw_num].is_enable)
			continue;

		isp_csi_intr_status(ctx, raw_num, &cbdg_0_sts[raw_num], &cbdg_1_sts[raw_num]);

		ctx->dg_info.bdg_dbg[raw_num].bdg_int_sts_0 = cbdg_0_sts[raw_num].raw;
		ctx->dg_info.bdg_dbg[raw_num].bdg_int_sts_1 = cbdg_1_sts[raw_num].raw;

		if (!ctx->isp_csi_cfg[raw_num].is_bt_demux)
			ctx->dg_info.fe_sts[raw_num] = ispblk_fe_dbg_info(ctx, raw_num);

		if (raw_num == ctx->cfg_info.raw_num) {
			ctx->dg_info.post_sts = ispblk_post_dbg_info(ctx);
			ctx->dg_info.dma_sts = ispblk_dma_dbg_info(ctx);
		}

		for (i = 0; i < ISP_FE_CHN_MAX; i++)
			ctx->dg_info.bdg_dbg[raw_num].chn_dbg[i] = ispblk_csibdg_chn_dbg(ctx, raw_num, i);
	}

	isp_err_chk(vdev, ctx, cbdg_0_sts, cbdg_1_sts);

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_VIRT0; raw_num++) {
		if (!ctx->isp_csi_cfg[raw_num].is_enable)
			continue;

		if (ctx->isp_csi_cfg[raw_num].is_bt_demux) {
			_isp_csibdg_lite_frame_start_chk(vdev, raw_num, cbdg_0_sts[raw_num].raw);
			_isp_csibdg_lite_frame_done_chk(vdev, raw_num, cbdg_0_sts[raw_num].raw);
		}

		//TODO for mux device may need to check the line cnt done
		if (ctx->isp_csi_cfg[raw_num].is_mux_dev && 0) {
			_isp_csibdg_line_cnt_done_chk(vdev, raw_num, cbdg_1_sts[raw_num]);
		}
	}

	_isp_cmdq_done_chk(vdev, top_sts_2.bits.cmdq_int);
	_isp_pre_fe_frame_start_chk(vdev, ISP_PRERAW0, top_sts_2.bits.frame_start_fe0);
	_isp_pre_fe_frame_start_chk(vdev, ISP_PRERAW1, top_sts_2.bits.frame_start_fe1);
	_isp_pre_fe_frame_start_chk(vdev, ISP_PRERAW2, top_sts_2.bits.frame_start_fe2);

	_isp_pre_fe_frame_done_chk(vdev, ISP_PRERAW0, top_sts_0.bits.frame_done_fe0);
	_isp_pre_fe_frame_done_chk(vdev, ISP_PRERAW1, top_sts_0.bits.frame_done_fe1);
	_isp_pre_fe_frame_done_chk(vdev, ISP_PRERAW2, top_sts_0.bits.frame_done_fe2);

	_isp_postraw_shadow_done_chk(vdev, top_sts_0.bits.shaw_done_post);
	_isp_postraw_frame_done_chk(vdev, top_sts_0.bits.frame_done_post);
}

/*******************************************************
 *  Common interface for core
 ******************************************************/
int vi_create_instance(struct sop_vi_dev *vdev)
{
	int ret = 0;

	if (!vdev) {
		vi_pr(VI_ERR, "invalid data\n");
		return ERR_VI_INVALID_NULL_PTR;
	}

	vi_set_base_addr(vdev->reg_base);

	_vi_init_param(vdev);

	ret = vi_create_thread(vdev, E_VI_TH_PRERAW);
	if (ret) {
		vi_pr(VI_ERR, "Failed to create preraw thread\n");
		goto err_create_preraw;
	}

	ret = vi_create_thread(vdev, E_VI_TH_VBLANK_HANDLER);
	if (ret) {
		vi_pr(VI_ERR, "Failed to create vblank_update thread\n");
		goto err_create_vblank;
	}

	ret = vi_create_thread(vdev, E_VI_TH_ERR_HANDLER);
	if (ret) {
		vi_pr(VI_ERR, "Failed to create err_handler thread\n");
		goto err_create_err;
	}

	ret = vi_create_thread(vdev, E_VI_TH_POSTRAW);
	if (ret) {
		vi_pr(VI_ERR, "Failed to create postraw thread\n");
		goto err_create_postraw;
	}

	return ret;

err_create_postraw:
	vi_destroy_thread(vdev, E_VI_TH_ERR_HANDLER);
err_create_err:
	vi_destroy_thread(vdev, E_VI_TH_VBLANK_HANDLER);
err_create_vblank:
	vi_destroy_thread(vdev, E_VI_TH_PRERAW);
err_create_preraw:

	return ret;
}

int vi_destroy_instance(struct sop_vi_dev *vdev)
{
	int ret = 0, i = 0;

	if (!vdev) {
		vi_pr(VI_ERR, "invalid data\n");
		return ERR_VI_INVALID_NULL_PTR;
	}

	for (i = 0; i < E_VI_TH_MAX; i++) {
		vi_destroy_thread(vdev, i);
	}

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		sync_task_exit(i);
		osal_spin_lock_destroy(&snr_node_lock[i]);
	}

	osal_spin_lock_destroy(&raw_num_lock);
	osal_spin_lock_destroy(&dq_lock);
	osal_spin_lock_destroy(&vdev->event_q.lock);
	osal_spin_lock_destroy(&vdev->qbuf_lock);

	osal_wait_destroy(&vdev->isp_event_wait_q);
	osal_wait_destroy(&vdev->isp_dbg_wait_q);

	return ret;
}

void vi_set_tuning_dis(int pipe, int fe_ctrl, int post_ctrl)
{
	tuning_dis[0] = pipe;
	tuning_dis[1] = fe_ctrl;
	tuning_dis[2] = post_ctrl;
}
