#ifndef _VI_RGB_IP_CTRL_H_
#define _VI_RGB_IP_CTRL_H_

#include "vi_ip_comm.h"

void ispblk_pfr_config(struct isp_ctx *ctx, bool en);
void ispblk_edge_ext_config(struct isp_ctx *ctx, bool en);
void ispblk_ccm_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id, bool en, struct isp_ccm_cfg *cfg);
void ispblk_gamma_config(struct isp_ctx *ctx, bool en, u8 sel, u16 *data, u8 inv);
void ispblk_gamma_enable(struct isp_ctx *ctx, bool enable);
void ispblk_rgbdither_config(struct isp_ctx *ctx, bool en, bool mod_en,
			    bool histidx_en, bool fmnum_en);
void ispblk_clut_cmdq_config(struct isp_ctx *ctx, const u8 pipe, bool en,
			int16_t *r_lut, int16_t *g_lut, int16_t *b_lut);
void ispblk_clut_config(struct isp_ctx *ctx, bool en, bool is_rdma_mode,
				uint16_t *r_lut, uint16_t *g_lut, uint16_t *b_lut);
void ispblk_csc_config(struct isp_ctx *ctx);
#endif
