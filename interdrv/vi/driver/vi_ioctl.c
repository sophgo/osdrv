#include "vi_ioctl.h"
#include "vi_uapi.h"
#include "vi_tun_ip_ctrl.h"
#include "vi.h"

static int vi_ai_isp_wait_cond(const void *param)
{
	osal_atomic *flag = (osal_atomic *)param;

	return osal_atomic_read(flag) == E_AI_WAKE_TYPE_MW;
}

static int ai_isp_resolve_cfg(struct vi_dev *vdev, struct vi_ai_isp_cfg *cfg)
{
	struct isp_ctx *ctx = &vdev->ctx;
	uint8_t pipe = cfg->vi_pipe;
	enum sop_isp_raw raw_num = ISP_PRERAW0;

	if (pipe >= VI_MAX_PIPE_NUM) {
		vi_pr(VI_ERR, "vi_pipe(%d) is invalid\n", pipe);
		return ERR_VI_INVALID_PARA;
	}

	raw_num = ctx->isp_pipe_cfg[pipe].bind_raw;

	switch (cfg->vi_ai_isp_type) {
	case AI_ISP_CFG_INIT:
	{
		ctx->is_ai_isp = true;
		ctx->isp_csi_cfg[raw_num].is_ai_isp = true;
		ctx->isp_csi_cfg[raw_num].ai_cfg.is_raw_planar = cfg->reserved[0] ? true : false;
		ctx->isp_csi_cfg[raw_num].ai_cfg.fmt = cfg->reserved[0];
		ctx->isp_csi_cfg[raw_num].ai_cfg.round = cfg->reserved[1];
		break;
	}
	case AI_ISP_CFG_ENABLE:
	{
		osal_atomic_set(&ctx->isp_pipe_cfg[pipe].ai_isp_en, 1);
		break;
	}
	case AI_ISP_CFG_DISABLE:
	case AI_ISP_CFG_DEINIT:
	{
		/* code */
		osal_atomic_set(&ctx->isp_pipe_cfg[pipe].ai_isp_en, 0);
		osal_atomic_set(&vdev->isp_ai_int_flag[raw_num], E_AI_WAKE_TYPE_AI_ISP_TH);
		osal_wait_wakeup(&vdev->isp_ai_wait_q[raw_num]);
		break;
	}
	default:
		break;
	}

	vi_pr(VI_DBG, "AI_ISP_CFG: vi_pipe %d, raw_num %d, type %d, reserved[0] %lld, reserved[1] %lld\n",
			pipe, raw_num, cfg->vi_ai_isp_type, cfg->reserved[0], cfg->reserved[1]);

	return 0;
}

static long _vi_s_ctrl(struct vi_dev *vdev, struct vi_ctrl *ctrl)
{
	struct isp_ctx *ctx = &vdev->ctx;
	u32 id = ctrl->id;
	long rc = ERR_VI_INVALID_PARA;

	switch (id) {
	case VI_IOCTL_STS_PUT:
	{
		u8 pipe = 0;
		enum sop_isp_raw raw_num = ISP_PRERAW0;
		unsigned long flags;

		pipe = ctrl->val;

		if (pipe >= VI_MAX_PIPE_NUM)
			break;

		raw_num = ctx->isp_pipe_cfg[pipe].bind_raw;
		if (raw_num >= ISP_PRERAW_MAX) {
			vi_pr(VI_ERR, "raw_num(%d) is invalid\n", raw_num);
			break;
		}

		osal_spin_lock_irqsave(&ctx->csi_bufpool[raw_num].pre_fe_sts_lock, &flags);
		ctx->csi_bufpool[raw_num].pre_fe_sts_in_use = 0;
		osal_spin_unlock_irqrestore(&ctx->csi_bufpool[raw_num].pre_fe_sts_lock, &flags);

		rc = 0;
		break;
	}

	case VI_IOCTL_POST_STS_PUT:
	{
		u8 pipe = 0;
		unsigned long flags;

		pipe = ctrl->val;

		if (pipe >= VI_MAX_PIPE_NUM)
			break;

		osal_spin_lock_irqsave(&ctx->isp_bufpool[pipe].post_sts_lock, &flags);
		ctx->isp_bufpool[pipe].post_sts_in_use = 0;
		osal_spin_unlock_irqrestore(&ctx->isp_bufpool[pipe].post_sts_lock, &flags);

		rc = 0;
		break;
	}

	case VI_IOCTL_SET_SNR_CFG_NODE:
	{
		struct sop_isp_snr_update *snr_update;
		u8 raw_num;

		if (vdev->isp_source != ISP_SOURCE_DEV)
			break;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(struct sop_isp_snr_update));

		snr_update = (struct sop_isp_snr_update *)ctrl->ptr;

		//for mw isp, raw_num is the same as pipe
		if (snr_update->raw_num >= VI_MAX_PIPE_NUM) {
			vi_pr(VI_ERR, "snr_update->raw_num(%d) is invalid\n", snr_update->raw_num);
			break;
		}

		raw_num = vi_get_raw_num_by_dev(ctx, snr_update->raw_num);

		if (raw_num >= ISP_PRERAW_MAX) {
			break;
		}

		if (ctx->isp_csi_cfg[raw_num].is_patgen_en) {
			rc = 0;
			break;
		}

		isp_snr_cfg_enq(&vdev->isp_snr_cfg[raw_num], snr_update);

		vi_pr(VI_DBG, "dev_%d, raw_num_%d, magic_num=%d, regs_num=%d, i2c_update=%d, isp_update=%d, list(%d)\n",
			snr_update->raw_num,
			raw_num,
			snr_update->snr_cfg_node.snsr.magic_num,
			snr_update->snr_cfg_node.snsr.regs_num,
			snr_update->snr_cfg_node.snsr.need_update,
			snr_update->snr_cfg_node.isp.need_update,
			vdev->isp_snr_cfg[raw_num].i2c_queue.num_rdy);

		rc = 0;
		break;
	}

	case VI_IOCTL_SET_SNR_INFO:
	{
		vi_pr(VI_WARN, "VI_IOCTL_SET_SNR_INFO is not supported\n");
		rc = 0;
		break;
	}

	case VI_IOCTL_AI_ISP_CFG:
	{
		struct vi_ai_isp_cfg *ai_isp_cfg;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(struct vi_ai_isp_cfg));

		ai_isp_cfg = (struct vi_ai_isp_cfg *)ctrl->ptr;
		if (ai_isp_cfg->vi_pipe >= VI_MAX_PIPE_NUM) {
			vi_pr(VI_ERR, "vi_pipe(%d) is invalid\n", ai_isp_cfg->vi_pipe);
			break;
		}

		rc = ai_isp_resolve_cfg(vdev, ai_isp_cfg);

		break;
	}

	case VI_IOCTL_PUT_AI_ISP_RAW:
	{
		struct vi_ai_isp_info *ai_isp_info;
		u8 pipe = 0;
		enum sop_isp_raw raw_num = ISP_PRERAW0;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(struct vi_ai_isp_info));

		ai_isp_info = (struct vi_ai_isp_info *)ctrl->ptr;

		if (ai_isp_info->vi_pipe >= VI_MAX_PIPE_NUM) {
			vi_pr(VI_ERR, "vi_pipe(%d) is invalid\n", ai_isp_info->vi_pipe);
			break;
		}

		pipe = ai_isp_info->vi_pipe;

		if (!ctx->isp_pipe_cfg[pipe].is_enable) {
			vi_pr(VI_ERR, "pipe(%d) is not enabled\n", pipe);
			break;
		}

		raw_num  = ctx->isp_pipe_cfg[pipe].bind_raw;
		if (raw_num >= ISP_PRERAW_MAX) {
			vi_pr(VI_ERR, "raw_num(%d) is invalid\n", raw_num);
			break;
		}

		osal_memcpy(&vdev->ai_isp_info[pipe], ai_isp_info, sizeof(struct vi_ai_isp_info));

		vi_pr(VI_DBG, "AI_ISP_RAW: vi_pipe %d, raw_num %d, input(0x%llx, 0x%llx), output(0x%llx, 0x%llx)\n",
			pipe, raw_num, ai_isp_info->input_addr[0], ai_isp_info->input_addr[1],
			ai_isp_info->output_addr[0], ai_isp_info->output_addr[1]);

		osal_atomic_set(&vdev->isp_ai_int_flag[raw_num], E_AI_WAKE_TYPE_AI_ISP_TH);
		osal_wait_wakeup(&vdev->isp_ai_wait_q[raw_num]);

		rc = 0;
		break;
	}
	default:
		break;
	}

	return rc;
}

static long _vi_g_ctrl(struct vi_dev *vdev, struct vi_ctrl *ctrl)
{
	u32 id = ctrl->id;
	long rc = ERR_VI_INVALID_PARA;
	struct isp_ctx *ctx = &vdev->ctx;

	switch (id) {
	case VI_IOCTL_STS_GET:
	{
		u8 pipe = ctrl->val;
		enum sop_isp_raw raw_num = ISP_PRERAW0;
		unsigned long flags;

		if (pipe >= VI_MAX_PIPE_NUM)
			break;

		raw_num = ctx->isp_pipe_cfg[pipe].bind_raw;
		if (raw_num >= ISP_PRERAW_MAX) {
			vi_pr(VI_ERR, "raw_num(%d) is invalid\n", raw_num);
			break;
		}

		osal_spin_lock_irqsave(&ctx->csi_bufpool[raw_num].pre_fe_sts_lock, &flags);
		ctx->csi_bufpool[raw_num].pre_fe_sts_in_use = 1;
		ctrl->val = ctx->csi_bufpool[raw_num].pre_fe_sts_busy_idx ^ 1;
		osal_spin_unlock_irqrestore(&ctx->csi_bufpool[raw_num].pre_fe_sts_lock, &flags);

		rc = 0;
		break;
	}

	case VI_IOCTL_POST_STS_GET:
	{
		u8 pipe = ctrl->val;
		unsigned long flags;

		if (pipe >= VI_MAX_PIPE_NUM)
			break;

		osal_spin_lock_irqsave(&ctx->isp_bufpool[pipe].post_sts_lock, &flags);
		ctx->isp_bufpool[pipe].post_sts_in_use = 1;
		ctrl->val = ctx->isp_bufpool[pipe].post_sts_busy_idx ^ 1;
		osal_spin_unlock_irqrestore(&ctx->isp_bufpool[pipe].post_sts_lock, &flags);

		rc = 0;
		break;
	}

	case VI_IOCTL_STS_MEM:
	{
		struct sop_isp_sts_mem *sts_mem;
		enum sop_isp_raw raw_num = ISP_PRERAW0;
		u8 pipe = 0, i = 0;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(struct sop_isp_sts_mem) * 2);

		sts_mem = (struct sop_isp_sts_mem *)ctrl->ptr;

		pipe = sts_mem[0].raw_num;
		if (pipe >= VI_MAX_PIPE_NUM) {
			vi_pr(VI_ERR, "sts_mem wrong dev_num(%d)\n", pipe);
			break;
		}

		raw_num = ctx->isp_pipe_cfg[pipe].bind_raw;
		if (raw_num >= ISP_PRERAW_MAX) {
			vi_pr(VI_ERR, "raw_num(%d) is invalid\n", raw_num);
			break;
		}

		//split fe sts and post sts, so that we can use the same buffer
		for (i = 0; i < BUF_MAX; i++) {
			osal_memcpy(&ctx->isp_bufpool[pipe].sts_mem[i].ae_le,
					&ctx->csi_bufpool[raw_num].sts_mem[i].ae_le,
					sizeof(struct sop_vip_memblock));
			osal_memcpy(&ctx->isp_bufpool[pipe].sts_mem[i].ae_se,
					&ctx->csi_bufpool[raw_num].sts_mem[i].ae_se,
					sizeof(struct sop_vip_memblock));
		}

		osal_memcpy(ctrl->ptr, ctx->isp_bufpool[pipe].sts_mem,
					sizeof(struct sop_isp_sts_mem) * 2);

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_LSC_PHY_BUF:
	{
		struct sop_vip_memblock *isp_mem;
		u8 pipe;
		enum sop_isp_raw raw_num;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(struct sop_vip_memblock));

		isp_mem = (struct sop_vip_memblock *)ctrl->ptr;
		pipe = isp_mem->raw_num;
		raw_num = ctx->isp_pipe_cfg[pipe].bind_raw;
		isp_mem->phy_addr = ctx->csi_bufpool[raw_num].clsc;
		isp_mem->size = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_FE0_CLSC_LE);

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_CLUT_PHY_BUF:
	{
		struct sop_vip_memblock *isp_mem;
		u8 pipe;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(struct sop_vip_memblock));

		isp_mem = (struct sop_vip_memblock *)ctrl->ptr;
		pipe = isp_mem->raw_num;
		isp_mem->phy_addr = ctx->isp_bufpool[pipe].clut;
		isp_mem->size = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_CLUT_R);

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_SC_ONLINE:
	{
		struct sop_isp_sc_online *sc_online;
		u8 pipe;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(struct sop_isp_sc_online));

		sc_online = (struct sop_isp_sc_online *)ctrl->ptr;

		pipe = sc_online->raw_num;
		sc_online->is_sc_online = !ctx->isp_pipe_cfg[pipe].is_offline_scaler;

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_SCENE_INFO:
	{
		u8 pipe = 0;
		enum isp_scene_info info = FE_ON_POST_OFF_SC;

		if (_is_all_online(ctx)) {
			info = FE_ON_POST_OFF_SC;
		} else if (_is_fe_post_offline(ctx)) {
			info = FE_OFF_POST_OFF_SC;
		} else {
			info = FE_SLICE_POST_OFF_SC;
		}

		if (ctx->isp_pipe_cfg[pipe].is_offline_scaler)
			info += FE_ON_POST_ON_SC;

		ctrl->val = info;

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_TUN_ADDR:
	{
		void *tun_addr = NULL;
		u32 size;
		u8 pipe = 0;

		for (pipe = 0; pipe < VI_MAX_PIPE_NUM; pipe++) {
			if (!ctx->isp_pipe_cfg[pipe].is_enable)
				continue;
			vi_tuning_buf_setup(ctx, pipe);
		}

		tun_addr = vi_get_tuning_buf_addr(&size);

		osal_memcpy(ctrl->ptr, tun_addr, size);

		rc = 0;
		break;
	}

	case VI_IOCTL_DQEVENT:
	{
		struct vi_event *ev_u = NULL;
		struct vi_event_k *ev_k;
		unsigned long flags;
		struct isp_event_q *event_q = &vdev->event_q;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(struct vi_event));

		ev_u = (struct vi_event *)ctrl->ptr;

		osal_spin_lock_irqsave(&event_q->lock, &flags);

		if (!osal_list_empty(&event_q->list)) {
			ev_k = osal_list_first_entry(&event_q->list, struct vi_event_k, list);
			ev_u->dev_id		= ev_k->ev.dev_id;
			ev_u->type		= ev_k->ev.type;
			ev_u->frame_sequence	= ev_k->ev.frame_sequence;
			ev_u->pts		= ev_k->ev.pts;
			osal_list_del_init(&ev_k->list);
			osal_kfree(ev_k);
			event_q->count--;
		}
		osal_spin_unlock_irqrestore(&event_q->lock, &flags);

		rc = 0;
		break;
	}
	case VI_IOCTL_GET_AI_ISP_RAW:
	{
		struct vi_ai_isp_info *ai_isp_info;
		int timeout = 200, ret = 0;
		u8 pipe = 0;
		enum sop_isp_raw raw_num = ISP_PRERAW0;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(struct vi_ai_isp_info));

		ai_isp_info = (struct vi_ai_isp_info *)ctrl->ptr;

		if (ai_isp_info->vi_pipe >= VI_MAX_PIPE_NUM) {
			vi_pr(VI_ERR, "vi_pipe(%d) is invalid\n", ai_isp_info->vi_pipe);
			break;
		}

		pipe = ai_isp_info->vi_pipe;

		if (!ctx->isp_pipe_cfg[pipe].is_enable) {
			vi_pr(VI_ERR, "pipe(%d) is not enabled\n", pipe);
			break;
		}

		raw_num  = ctx->isp_pipe_cfg[pipe].bind_raw;
		if (raw_num >= ISP_PRERAW_MAX) {
			vi_pr(VI_ERR, "raw_num(%d) is invalid\n", raw_num);
			break;
		}

		ret = osal_wait_timeout_uninterruptible(&vdev->isp_ai_wait_q[raw_num],
							vi_ai_isp_wait_cond,
							&vdev->isp_ai_int_flag[raw_num],
							timeout);
		if (ret <= 0) {
			vi_pr(VI_WARN, "vi get ai_isp raw timeout(%d)\n", timeout);
			break;
		}

		osal_memcpy(ctrl->ptr, &vdev->ai_isp_info[pipe], sizeof(struct vi_ai_isp_info));

		vi_pr(VI_DBG, "AI_ISP_RAW: vi_pipe %d, raw_num %d, input(0x%llx, 0x%llx), output(0x%llx, 0x%llx)\n",
				pipe, raw_num,
				vdev->ai_isp_info[pipe].input_addr[0], vdev->ai_isp_info[pipe].input_addr[1],
				vdev->ai_isp_info[pipe].output_addr[0], vdev->ai_isp_info[pipe].output_addr[1]);
		rc = 0;
		break;
	}
	default:
		break;
	}

	return rc;
}

long vi_ioctl(struct vi_dev *vdev, u_int cmd, struct vi_ctrl *ctrl)
{
	long ret = 0;

	switch (cmd) {
	case VI_IOC_S_CTRL:
		ret = _vi_s_ctrl(vdev, ctrl);
		break;
	case VI_IOC_G_CTRL:
		ret = _vi_g_ctrl(vdev, ctrl);
		break;
	case VI_IOC_SDK_CTRL:
		ret = vi_sdk_ctrl(vdev, ctrl);
		break;
	default:
		ret = ERR_VI_NOT_SUPPORT;
		break;
	}

	return ret;
}