#ifndef _VI_RAW_IP_CTRL_H_
#define _VI_RAW_IP_CTRL_H_

#include "vi_ip_comm.h"

void ispblk_blc_dg_wb_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id, bool enable);
void ispblk_fusion_config(struct isp_ctx *ctx, bool enable, enum isp_fs_out_e out_sel);
void ispblk_map_curve_config(struct isp_ctx *ctx, bool enable);
void ispblk_dpc_config(struct isp_ctx *ctx, bool enable, u8 test_case);
void ispblk_dpc_set_static(struct isp_ctx *ctx, enum isp_raw_path_e path,
			   u16 offset, u32 *bps, u16 count);
void ispblk_af_config(struct isp_ctx *ctx, bool enable);
void ispblk_bnr_config(struct isp_ctx *ctx, bool enable);
void ispblk_lscr_config(struct isp_ctx *ctx, bool enable, u16 *data, u32 size);
void ispblk_drc_config(struct isp_ctx *ctx, bool en);
void ispblk_cfa_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id);
#endif