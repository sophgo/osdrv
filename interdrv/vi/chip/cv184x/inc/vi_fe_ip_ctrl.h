#ifndef _VI_FE_IP_CTRL_H_
#define _VI_FE_IP_CTRL_H_

#include "vi_ip_comm.h"

enum sop_isp_raw find_phy_raw_num(struct isp_ctx *ctx, enum sop_isp_raw raw_num);
int csibdg_find_hwid(enum sop_isp_raw raw_num);
int fe_find_hwid(enum sop_isp_raw raw_num);
int clsc_find_hwid(enum sop_isp_raw raw_num, enum sop_isp_fe_chn_num chn);

void ispblk_csidbg_dma_wr_en(
	struct isp_ctx *ctx,
	const enum sop_isp_raw raw_num,
	const enum sop_isp_fe_chn_num chn_num,
	const u8 en);

void ispblk_csibdg_wdma_crop_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num, struct vi_rect crop, u8 en);
void ispblk_csibdg_update_size(struct isp_ctx *ctx, enum sop_isp_raw raw_num);
void ispblk_csibdg_crop_update(struct isp_ctx *ctx, enum sop_isp_raw raw_num, bool en);
int ispblk_csibdg_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num);
void ispblk_csibdg_reset(struct isp_ctx *ctx, enum sop_isp_raw raw_num);
void ispblk_csibdg_lite_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num);
void ispblk_csibdg_yuv_bypass_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num);
bool ispblk_clsc_is_enable(struct isp_ctx *ctx, enum sop_isp_raw raw_num, enum sop_isp_fe_chn_num chn);
void ispblk_clsc_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num, enum sop_isp_fe_chn_num chn, bool en);
void ispblk_clsc_dma_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num, uint64_t buf_addr);
void ispblk_aehist_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num, bool enable);
#endif
