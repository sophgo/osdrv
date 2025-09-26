#ifndef _VI_YUV_IP_CTRL_H_
#define _VI_YUV_IP_CTRL_H_

#include "vi_ip_comm.h"

int ispblk_yuvdither_config(struct isp_ctx *ctx, u8 sel, bool en,
			    bool mod_en, bool histidx_en, bool fmnum_en);

int ispblk_ee_front_back_config(struct isp_ctx *ctx, bool en);
void ispblk_dci_map_config(struct isp_ctx *ctx, u8 sel, u16 *data);
void ispblk_ldci_config(struct isp_ctx *ctx, bool ldci_en, bool dci_en);
int ispblk_ee_back_config(struct isp_ctx *ctx, bool en);
int ispblk_post_ee_config(struct isp_ctx *ctx, bool en);
void ispblk_mctf_config(struct isp_ctx *ctx, bool en, u8 test_case);

void ispblk_fbcd_config(struct isp_ctx *ctx, bool en);
void ispblk_fbce_config(struct isp_ctx *ctx, bool en);
void ispblk_cnr_config(struct isp_ctx *ctx, bool en, bool pfc_en, u8 str_mode, u8 test_case);
int ispblk_postee_config(struct isp_ctx *ctx, bool en);
void ispblk_ca_config(struct isp_ctx *ctx, bool en, u8 mode);
void ispblk_ca_lite_config(struct isp_ctx *ctx, bool en);
void ispblk_ycur_config(struct isp_ctx *ctx, bool en, u8 sel, u16 *data);
void ispblk_ycur_enable(struct isp_ctx *ctx, bool enable, u8 sel);
void ispblk_yuvtop_out_config(struct isp_ctx *ctx, bool online2sc);

#endif
