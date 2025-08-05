#include "vi_raw_dump.h"
#include "vi_fe_ip_ctrl.h"
#include "comm_errno.h"
#include "vb.h"

void _isp_fe_raw_dump_cfg(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num, const u8 chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_buffer *b = NULL;
	struct isp_queue *fe_out_q = &vdev->raw_dump[raw_num].buf_q[chn_num];
	u32 dmaid;

	dmaid = csibdg_dma_find_hwid(raw_num, chn_num);

	vi_pr(VI_DBG, "fe_be raw_dump cfg start\n");

	b = isp_buf_next(fe_out_q);
	if (b == NULL) {
		vi_pr(VI_ERR, "Pre_fe_%d LE raw_dump outbuf is empty\n", raw_num);
		return;
	}

	if (chn_num == ISP_FE_CH0) {
		if (ctx->isp_csi_cfg[raw_num].rawdump_crop[chn_num].w &&
			ctx->isp_csi_cfg[raw_num].rawdump_crop[chn_num].h) {
			vi_pr(VI_DBG, "rawdump_crop w(%d), h(%d)\n",
				ctx->isp_csi_cfg[raw_num].rawdump_crop[chn_num].w,
				ctx->isp_csi_cfg[raw_num].rawdump_crop[chn_num].h);
			vi_pr(VI_DBG, "b->crop x(%d), y(%d), w(%d), h(%d)\n",
				b->crop.x, b->crop.y, b->crop.w, b->crop.h);
			ispblk_csibdg_wdma_crop_config(ctx, raw_num, b->crop, 1);
		} else {
			ispblk_csibdg_wdma_crop_config(ctx, raw_num, b->crop, 0);
		}
	}

	ispblk_dma_setaddr(ctx, dmaid, b->addr);
	ispblk_csidbg_dma_wr_en(ctx, raw_num, chn_num, 1);

	osal_atomic_set(&vdev->raw_dump[raw_num].raw_dump_en[chn_num], RAWDUMP_PREPARE_DONE);

	osal_atomic_set(&ctx->isp_csi_cfg[raw_num].clsc_en[chn_num], ispblk_clsc_is_enable(ctx, raw_num, chn_num));

	ispblk_clsc_config(ctx, raw_num, chn_num, false);
}

static int isp_dump_raw_wait_cond_func(const void *param)
{
	u32 *flag = (u32 *)param;

	return *flag != 0;
}

int isp_raw_dump(struct sop_vi_dev *vdev, struct sop_vip_isp_raw_blk *dump)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_buffer *b;
	int ret = 0;
	u8 chn = 0;
	u8 raw_num = dump[0].raw_dump.raw_num;
	u8 chn_max = ctx->isp_csi_cfg[raw_num].is_hdr_on + 1;

	if (vdev->raw_dump[raw_num].isp_byr[ISP_FE_CH0] != 0 || vdev->raw_dump[raw_num].isp_byr[ISP_FE_CH1] != 0) {
		vi_pr(VI_ERR, "Release buffer first, call put pipe dump\n");
		dump[0].is_b_not_rls = true;
		ret = ERR_VI_INVALID_PARA;
		return ret;
	}

	for (chn = 0; chn < chn_max; chn++) {
		b = osal_vzalloc(sizeof(*b));
		if (b == NULL) {
			vi_pr(VI_ERR, "fail to malloc isp_buffer\n");
			return ERR_VI_NOMEM;
		}
		b->addr = dump[chn].raw_dump.phy_addr;
		b->raw_num = raw_num;
		b->crop = ctx->isp_csi_cfg[raw_num].rawdump_crop[chn];
		b->vb_blk = vb_phys_addr2handle(dump[chn].raw_dump.phy_addr);
		vi_pr(VI_DBG, "b->crop x(%d), y(%d), w(%d), h(%d)\n",
				b->crop.x, b->crop.y, b->crop.w, b->crop.h);

		vi_pr(VI_DBG, "raw_num=%d enque raw_dump chn_%d\n", raw_num, chn);

		isp_buf_queue(&vdev->raw_dump[raw_num].buf_q[chn], b);

		osal_atomic_set(&vdev->raw_dump[raw_num].raw_dump_en[chn], RAWDUMP_START);
	}

	ret = osal_wait_timeout_interruptible(
		&vdev->isp_int_wait_q[raw_num], isp_dump_raw_wait_cond_func, &vdev->isp_int_flag[raw_num],
		dump[0].time_out);

	vdev->isp_int_flag[raw_num] = 0;
	if (!ret) {
		vi_pr(VI_ERR, "vi get raw timeout(%d)\n", dump[0].time_out);
		dump[0].is_timeout = true;
		ret = ERR_VI_CFG_TIMEOUT;
		goto raw_dump_fail;
	}

	for (chn = 0; chn < chn_max; chn++) {
		vdev->raw_dump[raw_num].isp_byr[chn] = isp_buf_remove(&vdev->raw_dump[raw_num].buf_dq[chn]);
		if (vdev->raw_dump[raw_num].isp_byr[chn] == NULL) {
			vi_pr(VI_ERR, "Get raw_le dump buffer time_out(%d)\n", dump[chn].time_out);
			dump[chn].is_timeout = true;
			ret = ERR_VI_CFG_TIMEOUT;
			goto raw_dump_fail;
		}

		dump[chn].src_w		= vdev->raw_dump[raw_num].isp_byr[chn]->crop.w;
		dump[chn].src_h		= vdev->raw_dump[raw_num].isp_byr[chn]->crop.h;
		dump[chn].crop_x	= vdev->raw_dump[raw_num].isp_byr[chn]->crop.x;
		dump[chn].crop_y	= vdev->raw_dump[raw_num].isp_byr[chn]->crop.y;
		dump[chn].frm_num	= vdev->raw_dump[raw_num].isp_byr[chn]->frm_num;
	}

	return 0;

raw_dump_fail:
	for (chn = 0; chn < chn_max; chn++) {
		if (vdev->raw_dump[raw_num].isp_byr[chn])
			osal_vfree(vdev->raw_dump[raw_num].isp_byr);
		vdev->raw_dump[raw_num].isp_byr[chn] = NULL;
	}

	return ret;
}

void free_isp_byr(struct sop_vi_dev *vdev, u8 pipe)
{
	u8 chn = 0;
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num = ctx->isp_pipe_cfg[pipe].bind_raw;
	u8 chn_max = ctx->isp_csi_cfg[raw_num].is_hdr_on + 1;

	for (chn = 0; chn < chn_max; chn++) {
		if (vdev->raw_dump[raw_num].isp_byr[chn])
			osal_vfree(vdev->raw_dump[raw_num].isp_byr[chn]);
		vdev->raw_dump[raw_num].isp_byr[chn] = NULL;
	}
}

int isp_start_smooth_raw_dump(struct sop_vi_dev *vdev, struct sop_vip_isp_smooth_raw_param *pstSmoothRawParam)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_buffer *b = NULL;
	int ret = 0;
	u8 frm_num, chn, chn_max, i = 0;
	struct vi_rect rawdump_crop;
	enum sop_isp_raw raw_num;
	struct sop_vip_isp_raw_blk *raw_blk;

	if (pstSmoothRawParam->raw_num > VI_MAX_PIPE_NUM - 1)
		return ERR_VI_INVALID_PIPEID;

	raw_blk = (struct sop_vip_isp_raw_blk *)osal_phys_to_virt(pstSmoothRawParam->raw_blk_phyaddr);
	raw_num = ctx->isp_pipe_cfg[pstSmoothRawParam->raw_num].bind_raw;
	frm_num = pstSmoothRawParam->frm_num;
	chn_max = ctx->isp_csi_cfg[raw_num].is_hdr_on + 1;

	rawdump_crop.x = raw_blk->crop_x;
	rawdump_crop.y = raw_blk->crop_y;
	rawdump_crop.w = raw_blk->src_w;
	rawdump_crop.h = raw_blk->src_h;
	ctx->isp_csi_cfg[raw_num].rawdump_crop[ISP_FE_CH0] =
		ctx->isp_csi_cfg[raw_num].rawdump_crop[ISP_FE_CH1] = rawdump_crop;

	do {
		for (chn = 0; chn < chn_max; chn++) {
			b = osal_vzalloc(sizeof(*b));
			if (b == NULL) {
				vi_pr(VI_ERR, "chn_%d osal_vmalloc size(%zu) fail\n", chn, sizeof(*b));
				ret = -1;
				goto err;
			}
			b->addr = (raw_blk + i)->raw_dump.phy_addr;
			b->raw_num = raw_num;
			b->crop = rawdump_crop;
			b->vb_blk = vb_phys_addr2handle((raw_blk + i)->raw_dump.phy_addr);
			vi_pr(VI_DBG, "raw_num=%d enque raw_dump chn_%d 0x%llx, crop x(%d), y(%d), w(%d), h(%d)\n",
				raw_num, chn, b->addr,
				b->crop.x, b->crop.y, b->crop.w, b->crop.h);
			isp_buf_queue(&vdev->raw_dump[raw_num].buf_q[chn], b);
			i++;
		}
	} while (i < frm_num);

	osal_atomic_set(&vdev->raw_dump[raw_num].isp_smooth_raw_dump_en, SMOOTH_RAWDUMP_START);
	osal_atomic_set(&vdev->raw_dump[raw_num].raw_dump_en[ISP_FE_CH0], RAWDUMP_START);
	if (ctx->isp_csi_cfg[raw_num].is_hdr_on)
		osal_atomic_set(&vdev->raw_dump[raw_num].raw_dump_en[ISP_FE_CH1], RAWDUMP_START);

	return ret;
err:
	for (chn = 0; chn < chn_max; chn++) {
		while ((b = isp_buf_remove(&vdev->raw_dump[raw_num].buf_q[chn])) != NULL)
			osal_vfree(b);
		osal_atomic_set(&vdev->raw_dump[raw_num].raw_dump_en[chn], RAWDUMP_IDLE);
	}

	osal_atomic_set(&vdev->raw_dump[raw_num].isp_smooth_raw_dump_en, SMOOTH_RAWDUMP_IDLE);

	return ret;
}

static int isp_stop_dump_raw_wait_cond_func(const void *param)
{
	osal_atomic *isp_smooth_raw_dump_en = (osal_atomic *)param;

	return osal_atomic_read(isp_smooth_raw_dump_en) == SMOOTH_RAWDUMP_IDLE;
}

int isp_stop_smooth_raw_dump(struct sop_vi_dev *vdev, struct sop_vip_isp_smooth_raw_param *pstSmoothRawParam)
{
	int ret = 0;
	enum sop_isp_raw raw_num;
	u8 pipe = pstSmoothRawParam->raw_num;
	struct isp_ctx *ctx = &vdev->ctx;

	if (pipe > VI_MAX_PIPE_NUM - 1)
		return ERR_VI_INVALID_PIPEID;

	raw_num = ctx->isp_pipe_cfg[pipe].bind_raw;

	osal_atomic_set(&vdev->raw_dump[raw_num].isp_smooth_raw_dump_en, SMOOTH_RAWDUMP_STOP);

	ret = osal_wait_timeout_interruptible(
		&vdev->isp_int_wait_q[raw_num], isp_stop_dump_raw_wait_cond_func,
		&vdev->raw_dump[raw_num].isp_smooth_raw_dump_en, 3000);

	if (!ret) {
		vi_pr(VI_ERR, "vi stop raw timeout\n");
		ret = ERR_VI_CFG_TIMEOUT;
		return ret;
	}

	vi_pr(VI_DBG, "stop smooth raw dump\n");

	return 0;
}

int isp_get_smooth_raw_dump(struct sop_vi_dev *vdev, struct sop_vip_isp_raw_blk *dump)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_buffer *b = NULL;
	int ret = 0;
	u8 raw_num = dump[0].raw_dump.raw_num;
	u8 chn, chn_max;

	ret = osal_wait_timeout_interruptible(
		&vdev->isp_int_wait_q[raw_num], isp_dump_raw_wait_cond_func, &vdev->isp_int_flag[raw_num],
		dump->time_out);

	vdev->isp_int_flag[raw_num] = 0;
	if (!ret) {
		vi_pr(VI_ERR, "vi get raw timeout(%d)\n", dump[0].time_out);
		dump[0].is_timeout = true;
		ret = ERR_VI_CFG_TIMEOUT;
		return ret;
	}

	chn_max = ctx->isp_csi_cfg[raw_num].is_hdr_on + 1;
	for (chn = 0; chn < chn_max; chn++) {
		b = isp_buf_remove(&vdev->raw_dump[raw_num].buf_dq[chn]);
		if (b == NULL) {
			vi_pr(VI_ERR, "Get raw_le dump buffer time_out(%d)\n", dump[chn].time_out);
			osal_vfree(b);
			dump[chn].is_timeout = true;
			ret = ERR_VI_CFG_TIMEOUT;
			return ret;
		}

		osal_memset(&dump[chn], 0, sizeof(struct sop_vip_isp_raw_blk));
		vi_pr(VI_DBG, "raw_le phy_addr=0x%llx byr_size=%d frm_num=%d\n",
			b->addr, b->byr_size, b->frm_num);

		dump[chn].src_w             = b->crop.w;
		dump[chn].src_h             = b->crop.h;
		dump[chn].crop_x            = b->crop.x;
		dump[chn].crop_y            = b->crop.y;
		dump[chn].frm_num           = b->frm_num;
		dump[chn].raw_dump.size     = b->byr_size;
		dump[chn].raw_dump.phy_addr = b->addr;
		osal_vfree(b);
	}

	return 0;
}

int isp_put_smooth_raw_dump(struct sop_vi_dev *vdev, struct sop_vip_isp_raw_blk *dump)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_buffer *b = NULL;
	u8 chn, chn_max;
	u8 raw_num;

	raw_num = dump[0].raw_dump.raw_num;
	chn_max = ctx->isp_csi_cfg[raw_num].is_hdr_on + 1;

	for (chn = 0; chn < chn_max; chn++) {
		b = osal_vzalloc(sizeof(*b));
		if (b == NULL) {
			vi_pr(VI_ERR, "le osal_vmalloc size(%zu) fail\n", sizeof(*b));
			osal_vfree(b);
			return -1;
		}
		b->addr = dump[chn].raw_dump.phy_addr;
		b->raw_num = raw_num;
		b->crop = ctx->isp_csi_cfg[raw_num].rawdump_crop[chn];
		b->vb_blk = vb_phys_addr2handle(dump[chn].raw_dump.phy_addr);
		vi_pr(VI_DBG, "raw_num=%d enque raw_dump 0x%llx, crop x(%d), y(%d), w(%d), h(%d)\n",
			raw_num, b->addr,
			b->crop.x, b->crop.y, b->crop.w, b->crop.h);
		isp_buf_queue(&vdev->raw_dump[raw_num].buf_q[chn], b);
	}

	return 0;
}

void _isp_raw_dump_chk(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num, const u32 frm_num)
{
	switch (osal_atomic_read(&vdev->raw_dump[raw_num].isp_smooth_raw_dump_en)) {
	default:
	case SMOOTH_RAWDUMP_IDLE:
	{
		vi_pr(VI_DBG, "wake up wait_q\n");

		vdev->isp_int_flag[raw_num] = 1;
		osal_wait_wakeup_interruptible(&vdev->isp_int_wait_q[raw_num]);

		osal_atomic_set(&vdev->raw_dump[raw_num].raw_dump_en[ISP_FE_CH0], RAWDUMP_IDLE);
		if (vdev->ctx.isp_csi_cfg[raw_num].is_hdr_on)
			osal_atomic_set(&vdev->raw_dump[raw_num].raw_dump_en[ISP_FE_CH1], RAWDUMP_IDLE);
		return;
	}
	case SMOOTH_RAWDUMP_START:
	{
		vi_pr(VI_DBG, "wake up wait_q smooth frm=%d\n", frm_num);

		vdev->isp_int_flag[raw_num] = 1;
		osal_wait_wakeup_interruptible(&vdev->isp_int_wait_q[raw_num]);
		return;
	}
	case SMOOTH_RAWDUMP_STOP:
	{
		struct isp_buffer *b = NULL;

		vi_pr(VI_DBG, "stop dump smooth\n");

		while ((b = isp_buf_remove(&vdev->raw_dump[raw_num].buf_dq[ISP_FE_CH0])) != NULL)
			osal_vfree(b);
		while ((b = isp_buf_remove(&vdev->raw_dump[raw_num].buf_dq[ISP_FE_CH1])) != NULL)
			osal_vfree(b);
		while ((b = isp_buf_remove(&vdev->raw_dump[raw_num].buf_q[ISP_FE_CH0])) != NULL)
			osal_vfree(b);
		while ((b = isp_buf_remove(&vdev->raw_dump[raw_num].buf_q[ISP_FE_CH1])) != NULL)
			osal_vfree(b);

		osal_atomic_set(&vdev->raw_dump[raw_num].raw_dump_en[ISP_FE_CH0], RAWDUMP_IDLE);
		if (vdev->ctx.isp_csi_cfg[raw_num].is_hdr_on)
			osal_atomic_set(&vdev->raw_dump[raw_num].raw_dump_en[ISP_FE_CH1], RAWDUMP_IDLE);
		osal_atomic_set(&vdev->raw_dump[raw_num].isp_smooth_raw_dump_en, SMOOTH_RAWDUMP_IDLE);

		osal_wait_wakeup_interruptible(&vdev->isp_int_wait_q[raw_num]);

		return;
	}
	}
}

static void raw_dump_wq_handler(osal_workqueue *worker)
{
	struct raw_dump_work *dump_work = osal_container_of((void *)worker, struct raw_dump_work, worker);
	struct isp_buffer *buf = NULL;
	vb_blk blk = 0;

	do {
		buf = isp_buf_remove(&dump_work->raw_dump_vb_q);
		if (buf) {
			if (buf->is_ext == INTERNAL_BUFFER) {
				vi_pr(VI_ERR, "buf is invalid\n");
				continue;
			}

			//for ai bnr buf->addr is invalid
			blk = vb_phys_addr2handle(buf->addr);
			if (blk != VB_INVALID_HANDLE)
				vb_release_block(blk);
			osal_kfree(buf);
		}

	} while (buf);
}

void isp_raw_dump_vb_queue(struct sop_vi_dev *vdev, struct isp_buffer *buf, bool try2sched)
{
	isp_buf_queue(&vdev->raw_dump_work.raw_dump_vb_q, buf);

	if (try2sched)
		osal_workqueue_schedule(&vdev->raw_dump_work.worker);
}

void isp_raw_dump_init(struct sop_vi_dev *vdev)
{
	isp_buf_init(&vdev->raw_dump_work.raw_dump_vb_q);
	OSAL_INIT_LIST_HEAD(&vdev->raw_dump_work.raw_dump_vb_q.rdy_queue);
	vdev->raw_dump_work.raw_dump_vb_q.num_rdy = 0;
	vdev->raw_dump_work.raw_dump_vb_q.raw_num = 0;
	osal_workqueue_init(&vdev->raw_dump_work.worker, raw_dump_wq_handler);
}

void isp_raw_dump_deinit(struct sop_vi_dev *vdev)
{
	raw_dump_wq_handler(&vdev->raw_dump_work.worker);
	osal_workqueue_destroy(&vdev->raw_dump_work.worker);
	isp_buf_destroy(&vdev->raw_dump_work.raw_dump_vb_q);
}
