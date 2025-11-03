#include <proc/vi_dbg_proc.h>
#include "vi_isp_buf_ctrl.h"
#include "vi_ip_comm.h"
#include "aos/cli.h"

#define VI_DBG_PROC_NAME	"soph/vi_dbg"

static struct vi_dev *m_vdev;

/*************************************************************************
 *	Proc functions
 *************************************************************************/

static inline void _vi_dbg_proc_show(void)
{
	struct vi_dev *vdev = m_vdev;
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	enum sop_isp_fe_chn_num chn_num = ISP_FE_CH0;
	enum sop_isp_fe_chn_num chn_max = ISP_FE_CHN_MAX;
	osal_timeval tv1, tv2;
	u8 pipe = 0;
	u32 sofCnt1[ISP_PRERAW_MAX], sofCnt2[ISP_PRERAW_MAX];
	u32 frmCnt1[ISP_PRERAW_MAX], frmCnt2[ISP_PRERAW_MAX];
	u64 t2 = 0, t1 = 0;
	int pos = 0;
	char *buf = NULL;

	if (!vdev) {
		vi_pr(VI_ERR, "vdev is NULL\n");
		return;
	}

	buf = osal_calloc(1, 4096);
	if (!buf) {
		vi_pr(VI_ERR, "Failed to alloc buf\n");
		return;
	}

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ctx->isp_csi_cfg[raw_num].is_enable)
			continue;
		pipe = ctx->isp_csi_cfg[raw_num].bind_pipe[ISP_FE_CH0];
		sofCnt1[raw_num] = vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0];
		if (ctx->isp_csi_cfg[raw_num].is_yuv_sensor) //YUV sensor
			frmCnt1[raw_num] = vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0];
		else //RGB sensor
			frmCnt1[raw_num] = vdev->postraw_frame_number[pipe];
	}

	osal_gettimeofday(&tv1);
	t1 = tv1.tv_sec * 1000000L + tv1.tv_usec;

	osal_msleep(940);
	do {
		for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
			if (!ctx->isp_csi_cfg[raw_num].is_enable)
				continue;
			pipe = ctx->isp_csi_cfg[raw_num].bind_pipe[ISP_FE_CH0];
			sofCnt2[raw_num] = vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0];
			if (vdev->ctx.isp_csi_cfg[raw_num].is_yuv_sensor) //YUV sensor
				frmCnt2[raw_num] = vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0];
			else //RGB sensor
				frmCnt2[raw_num] = vdev->postraw_frame_number[pipe];
		}

		osal_gettimeofday(&tv2);
		t2 = tv2.tv_sec * 1000000L + tv2.tv_usec;
	} while ((t2 - t1) < 1000000);

	//post dbg info
	pos += sprintf(buf + pos, "[VI Post_Dbg_Info]\n");
	pos += sprintf(buf + pos, "VIIspTopStatus0\t\t:0x%x\t\tVIIspTopStatus1\t\t:0x%x\n",
			ctx->dg_info.post_sts.top_sts_0,
			ctx->dg_info.post_sts.top_sts_1);
	pos += sprintf(buf + pos, "[VI DMA_Dbg_Info]\n");
	pos += sprintf(buf + pos, "VIWdma1ErrStatus\t:0x%x\tVIWdma1IdleStatus\t:0x%x\n",
			ctx->dg_info.dma_sts.wdma_1_err_sts,
			ctx->dg_info.dma_sts.wdma_1_idle);
	pos += sprintf(buf + pos, "VIWdma2ErrStatus\t:0x%x\tVIWdma2IdleStatus\t:0x%x\n",
			ctx->dg_info.dma_sts.wdma_2_err_sts,
			ctx->dg_info.dma_sts.wdma_2_idle);
	pos += sprintf(buf + pos, "VIWdma3ErrStatus\t:0x%x\tVIWdma3IdleStatus\t:0x%x\n",
			ctx->dg_info.dma_sts.wdma_3_err_sts,
			ctx->dg_info.dma_sts.wdma_3_idle);
	pos += sprintf(buf + pos, "VIRdma1ErrStatus\t:0x%x\tVIRdma1IdleStatus\t:0x%x\n",
			ctx->dg_info.dma_sts.rdma_1_err_sts,
			ctx->dg_info.dma_sts.rdma_1_idle);
	pos += sprintf(buf + pos, "VIRdma2ErrStatus\t:0x%x\tVIRdma2IdleStatus\t:0x%x\n",
			ctx->dg_info.dma_sts.rdma_2_err_sts,
			ctx->dg_info.dma_sts.rdma_2_idle);
	pos += sprintf(buf + pos, "VIRdma3ErrStatus\t:0x%x\tVIRdma3IdleStatus\t:0x%x\n",
			ctx->dg_info.dma_sts.rdma_3_err_sts,
			ctx->dg_info.dma_sts.rdma_3_idle);

	//csi/dbg info
	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ctx->isp_csi_cfg[raw_num].is_enable)
			continue;

		pos += sprintf(buf + pos, "[VI ISP_FE_%d FE_Dbg_Info]\n", raw_num);
		pos += sprintf(buf + pos, "VIPreFERawDbgSts\t:0x%x\t\tVIPreFEDbgInfo\t\t:0x%x\n",
				ctx->dg_info.fe_sts[raw_num].fe_idle_sts,
				ctx->dg_info.fe_sts[raw_num].fe_done_sts);
		pos += sprintf(buf + pos, "[VI ISP_FE_%d]\n", raw_num);

		pos += sprintf(buf + pos, "VIOutImgWidth\t\t:%4d\n", ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].w);
		pos += sprintf(buf + pos, "VIOutImgHeight\t\t:%4d\n", ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].h);
		pos += sprintf(buf + pos, "VIInImgWidth\t\t:%4d\n", ctx->isp_csi_cfg[raw_num].csibdg_width);
		pos += sprintf(buf + pos, "VIInImgHeight\t\t:%4d\n", ctx->isp_csi_cfg[raw_num].csibdg_height);

		pos += sprintf(buf + pos, "VIDevFPS\t\t:%4d\n", sofCnt2[raw_num] - sofCnt1[raw_num]);
		pos += sprintf(buf + pos, "VIFPS\t\t\t:%4d\n", frmCnt2[raw_num] - frmCnt1[raw_num]);

		chn_max = ctx->isp_csi_cfg[raw_num].is_yuv_sensor ?
				ctx->isp_csi_cfg[raw_num].mux_mode + 1 : ctx->isp_csi_cfg[raw_num].is_hdr_on + 1;
		for (chn_num = ISP_FE_CH0; chn_num < chn_max; chn_num++) {
			pos += sprintf(buf + pos,
				       "VISofCh%dCnt\t\t:%4d\n", chn_num, vdev->pre_fe_sof_cnt[raw_num][chn_num]);
		}

		for (chn_num = ISP_FE_CH0; chn_num < chn_max; chn_num++) {
			pipe = ctx->isp_csi_cfg[raw_num].is_hdr_on
					? ctx->isp_csi_cfg[raw_num].bind_pipe[ISP_FE_CH0]
					: ctx->isp_csi_cfg[raw_num].bind_pipe[chn_num];
			pos += sprintf(buf + pos,
				       "VIPreFECh%dCnt\t\t:%4d\n", chn_num, vdev->pre_fe_frm_num[raw_num][chn_num]);
			pos += sprintf(buf + pos, "VIPostPipe%dCnt\t\t:%4d\n", pipe, vdev->postraw_frame_number[pipe]);
		}

		pos += sprintf(buf + pos, "VIDropCnt\t\t:%4d\n", vdev->drop_frame_number[raw_num]);
		pos += sprintf(buf + pos, "VIDumpCnt\t\t:%4d\n", vdev->dump_frame_number[raw_num]);

		pos += sprintf(buf + pos, "[VI ISP_FE_%d Csi_Dbg_Info]\n", raw_num);
		pos += sprintf(buf + pos, "VICsiIntStatus0\t\t:0x%x\n", ctx->dg_info.bdg_dbg[raw_num].bdg_int_sts_0);
		pos += sprintf(buf + pos, "VICsiIntStatus1\t\t:0x%x\n", ctx->dg_info.bdg_dbg[raw_num].bdg_int_sts_1);
		pos += sprintf(buf + pos, "VICsiOverFlowCnt\t:%4d\n", ctx->dg_info.bdg_dbg[raw_num].bdg_fifo_of_cnt);

		for (chn_num = ISP_FE_CH0; chn_num < chn_max; chn_num++) {
			pos += sprintf(buf + pos, "VICsiCh%dDbg0\t\t:0x%x\n", chn_num,
					ctx->dg_info.bdg_dbg[raw_num].chn_dbg[chn_num].dbg_0);
			pos += sprintf(buf + pos, "VICsiCh%dDbg1\t\t:0x%x\n", chn_num,
					ctx->dg_info.bdg_dbg[raw_num].chn_dbg[chn_num].dbg_1);
			pos += sprintf(buf + pos, "VICsiCh%dDbg2\t\t:0x%x\n", chn_num,
					ctx->dg_info.bdg_dbg[raw_num].chn_dbg[chn_num].dbg_2);
			pos += sprintf(buf + pos, "VICsiCh%dDbg3\t\t:0x%x\n", chn_num,
					ctx->dg_info.bdg_dbg[raw_num].chn_dbg[chn_num].dbg_3);

			pos += sprintf(buf + pos, "VICsiCh%dWidthGTCnt\t:%4d\n", chn_num,
					ctx->dg_info.bdg_dbg[raw_num].bdg_w_gt_cnt[chn_num]);
			pos += sprintf(buf + pos, "VICsiCh%dWidthLSCnt\t:%4d\n", chn_num,
					ctx->dg_info.bdg_dbg[raw_num].bdg_w_ls_cnt[chn_num]);
			pos += sprintf(buf + pos, "VICsiCh%dHeightGTCnt\t:%4d\n", chn_num,
					ctx->dg_info.bdg_dbg[raw_num].bdg_h_gt_cnt[chn_num]);
			pos += sprintf(buf + pos, "VICsiCh%dHeightLSCnt\t:%4d\n", chn_num,
					ctx->dg_info.bdg_dbg[raw_num].bdg_h_ls_cnt[chn_num]);
		}

		pos += sprintf(buf + pos, "[VI ISP_PIPE_%d Buf_Dbg_Info]\n", raw_num);
		if (ctx->isp_csi_cfg[raw_num].is_yuv_sensor &&
		    ctx->isp_csi_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_BYPASS) {
			for (chn_num = ISP_FE_CH0; chn_num < chn_max; chn_num++) {
				pipe = ctx->isp_csi_cfg[raw_num].bind_pipe[chn_num];
				pos += sprintf(buf + pos, "VIYuvCh%dOutBufEmpty\t:%4d\n", chn_num,
					       sop_isp_rdy_buf_empty(&vdev->qbuf_q[pipe][chn_num]));
			}
		} else {
			if (_is_fe_post_offline(ctx)) { // fe->dram->post
				pos += sprintf(buf + pos, "VIPostCh0InBufEmpty\t:%4d\n",
						isp_buf_empty(&vdev->postraw_in_q));
				if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
					pos += sprintf(buf + pos, "VIPostCh1InBufEmpty\t:%4d\n",
							isp_buf_empty(&vdev->postraw_wdr_in_q[ISP_FE_CH1]));
				}
				pos += sprintf(buf + pos, "VIPreFECh0OutBufEmpty\t:%4d\n",
						isp_buf_empty(&vdev->pre_fe_out_q[raw_num][ISP_FE_CH0]));
				if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
					pos += sprintf(buf + pos, "VIPreFECh1OutBufEmpty\t:%4d\n",
							isp_buf_empty(&vdev->pre_fe_out_q[raw_num][ISP_FE_CH1]));
				}
			}

			pipe = ctx->isp_csi_cfg[raw_num].bind_pipe[ISP_FE_CH0];
			pos += sprintf(buf + pos, "VIPostOutBufEmpty\t:%4d\n",
				       sop_isp_rdy_buf_empty(&vdev->qbuf_q[pipe][ISP_FE_CH0]));
		}
	}

	osal_printk("%s\n", buf);

	osal_free(buf);
}

static void vi_dbg_proc_show(int32_t argc, char **argv)
{
	_vi_dbg_proc_show();
}

ALIOS_CLI_CMD_REGISTER(vi_dbg_proc_show, proc_vi_dbg, "vi_dbg_proc");

int vi_dbg_proc_init(struct vi_dev *_vdev)
{
	m_vdev = _vdev;

	return 0;
}

int vi_dbg_proc_remove(void)
{
	m_vdev = NULL;

	return 0;
}