#ifndef __VPSS_PLATFORM_H__
#define __VPSS_PLATFORM_H__

#include "vpss_hal.h"

struct vpss_cmdq_buf {
	__u64 cmdq_phy_addr;
	void *cmdq_vir_addr;
	__u32 cmdq_buf_size;
};

void cvi_img_update(u8 dev_idx, bool isMaster, const struct cvi_vpss_grp_cfg *grp_cfg);
void cvi_sc_update(u8 dev_idx, const struct cvi_vpss_chn_cfg *chn_cfg);
void cvi_top_update(u8 dev_idx, bool isShare, bool fbd_enable);
void cvi_img_start(u8 dev_idx, u8 chn_num);
void cvi_img_reset(u8 dev_idx);
bool cvi_img_left_tile_cfg(u8 dev_idx);
bool cvi_img_right_tile_cfg(u8 dev_idx);
bool cvi_img_top_tile_cfg(u8 dev_idx, u8 is_left);
bool cvi_img_down_tile_cfg(u8 dev_idx, u8 is_right);

void cvi_vpss_stauts(u8 dev_idx);
void cvi_vpss_error_stauts(u8 dev_idx);

int cvi_vpss_stitch(u8 dev_idx, struct vpss_cmdq_buf *cmdq_buf, struct vpss_stitch_cfg *cfg);

#endif
