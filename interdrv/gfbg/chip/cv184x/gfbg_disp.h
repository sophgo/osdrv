#ifndef _GFBG_DISP_H_
#define _GFBG_DISP_H_

#include "disp.h"

void gfbg_set_disp_base_addr(unsigned char inst, void *base);
void gfbg_set_oenc_base_addr(unsigned char inst, void *base);
void gfbg_gop_set_cfg(u8 inst, u8 layer, struct disp_gop_cfg *cfg, bool update);
struct disp_gop_cfg *gfbg_gop_get_cfg(u8 inst, u8 layer);
void gfbg_gop_ow_set_cfg(u8 inst, u8 layer, u8 ow_inst, struct disp_gop_ow_cfg *ow_cfg, bool update);
void gfbg_get_disp_hw_timing(unsigned char inst, struct disp_timing *timing);
int gfbg_gop_update_16LUT(u8 inst, u8 layer, u8 index, u16 data);
int gfbg_gop_update_256LUT(u8 inst, u8 layer, u16 index, u16 data);
int oenc_intr_status(void);
void oenc_intr_clr(u32 flag);
void oenc_set_cfg(struct oenc_cfg oenc_cfg);
void oenc_get_cfg(struct oenc_cfg *oenc_cfg);
void gfbg_gop_odec_set_cfg_from_oenc(struct disp_gop_odec_cfg odec_cfg, struct oenc_cfg oenc_cfg);

#endif  // _GFBG_DISP_H_