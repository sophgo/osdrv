#ifndef _VI_SUB_IP_CTRL_H_
#define _VI_SUB_IP_CTRL_H_

#include "vi_ip_comm.h"

void ispblk_preraw_fe_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num);
void ispblk_preraw_vi_sel_config(struct isp_ctx *ctx);
void ispblk_rawtop_config(struct isp_ctx *ctx);
void ispblk_rgbtop_config(struct isp_ctx *ctx);
void ispblk_yuvtop_config(struct isp_ctx *ctx);
void ispblk_isptop_fe_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num, bool en);
void ispblk_isptop_config(struct isp_ctx *ctx);
void isp_intr_set_mask(struct isp_ctx *ctx);
void isp_reset(struct isp_ctx *ctx);
struct _csi_bdg_chn_dbg_i ispblk_csibdg_chn_dbg(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num,
	enum sop_isp_fe_chn_num chn_num);
struct _fe_dbg_i ispblk_fe_dbg_info(struct isp_ctx *ctx, enum sop_isp_raw raw_num);
struct _post_dbg_i ispblk_post_dbg_info(struct isp_ctx *ctx);
struct _dma_dbg_i ispblk_dma_dbg_info(struct isp_ctx *ctx);

#endif
