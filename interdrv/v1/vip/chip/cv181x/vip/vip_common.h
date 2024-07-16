#ifndef _COMMON_VIP_H_
#define _COMMON_VIP_H_

#include "reg_vip_sys.h"

#define FPGA_EARLY_PORTING

#define BASE_OFFSET 0x000000000
#define GOP_FRAME_OFFSET 0x200000

#define VIP_ALIGNMENT 0x40
#define GOP_ALIGNMENT 0x10

#define MIN(a, b) (((a) < (b))?(a):(b))
#define MAX(a, b) (((a) > (b))?(a):(b))
#define CLIP(x, min, max) MAX(MIN(x, max), min)
#define VIP_64_ALIGN(x) (((x) + 0x3F) & ~0x3F)   // for 64byte alignment
#define VIP_ALIGN(x) (((x) + 0xF) & ~0xF)   // for 16byte alignment
#define ISP_ALIGN(x, y) (((x) + (y - 1)) & ~(y - 1))   // for alignment
#define UPPER(x, y) (((x) + ((1 << (y)) - 1)) >> (y))   // for alignment
#define CEIL(x, y) (((x) + ((1 << (y)))) >> (y))   // for alignment

#define R_IDX 0
#define G_IDX 1
#define B_IDX 2

#define VIP_CLK_RATIO_MASK(CLK_NAME) VIP_SYS_REG_CK_COEF_##CLK_NAME##_MASK
#define VIP_CLK_RATIO_OFFSET(CLK_NAME) VIP_SYS_REG_CK_COEF_##CLK_NAME##_OFFSET
#define VIP_CLK_RATIO_CONFIG(CLK_NAME, RATIO) \
	vip_sys_reg_write_mask(VIP_SYS_REG_CK_COEF_##CLK_NAME, \
		VIP_CLK_RATIO_MASK(CLK_NAME), \
		RATIO << VIP_CLK_RATIO_OFFSET(CLK_NAME))
#define VIP_CLK_RATIO_VALUE(CLK_NAME) \
	((vip_sys_reg_read(VIP_SYS_REG_CK_COEF_##CLK_NAME) & VIP_CLK_RATIO_MASK(CLK_NAME)) \
		>> VIP_CLK_RATIO_OFFSET(CLK_NAME))

#define VIP_RST_MASK(MOD_NAME) VIP_SYS_REG_RST_##MOD_NAME##_MASK
#define VIP_RST_OFFSET(MOD_NAME) VIP_SYS_REG_RST_##MOD_NAME##_OFFSET
#define VIP_RST_CONFIG(MOD_NAME, VALUE) \
	vip_sys_reg_write_mask(VIP_SYS_REG_RST_##MOD_NAME, \
		VIP_RST_MASK(MOD_NAME), \
		VALUE << VIP_RST_OFFSET(MOD_NAME))
#define VIP_RST_VALUE(MOD_NAME) \
	((vip_sys_reg_read(VIP_SYS_REG_RST_##MOD_NAME) & VIP_RST_MASK(MOD_NAME)) \
		>> VIP_RST_OFFSET(MOD_NAME))

#define VIP_NORM_CLK_RATIO_MASK(CLK_NAME) VIP_SYS_REG_NORM_DIV_##CLK_NAME##_MASK
#define VIP_NORM_CLK_RATIO_OFFSET(CLK_NAME) VIP_SYS_REG_NORM_DIV_##CLK_NAME##_OFFSET
#define VIP_NORM_CLK_RATIO_CONFIG(CLK_NAME, RATIO) \
	vip_sys_reg_write_mask(VIP_SYS_REG_NORM_DIV_##CLK_NAME, \
		VIP_NORM_CLK_RATIO_MASK(CLK_NAME), \
		RATIO << VIP_NORM_CLK_RATIO_OFFSET(CLK_NAME))
#define VIP_NORM_CLK_RATIO_VALUE(CLK_NAME) \
	((vip_sys_reg_read(VIP_SYS_REG_NORM_DIV_##CLK_NAME) & VIP_NORM_CLK_RATIO_MASK(CLK_NAME)) \
		>> VIP_NORM_CLK_RATIO_OFFSET(CLK_NAME))

#define VIP_IDLE_CLK_RATIO_MASK(CLK_NAME) VIP_SYS_REG_IDLE_DIV_##CLK_NAME##_MASK
#define VIP_IDLE_CLK_RATIO_OFFSET(CLK_NAME) VIP_SYS_REG_IDLE_DIV_##CLK_NAME##_OFFSET
#define VIP_IDLE_CLK_RATIO_CONFIG(CLK_NAME, RATIO) \
	vip_sys_reg_write_mask(VIP_SYS_REG_IDLE_DIV_##CLK_NAME, \
		VIP_IDLE_CLK_RATIO_MASK(CLK_NAME), \
		RATIO << VIP_IDLE_CLK_RATIO_OFFSET(CLK_NAME))
#define VIP_IDLE_CLK_RATIO_VALUE(CLK_NAME) \
	((vip_sys_reg_read(VIP_SYS_REG_IDLE_DIV_##CLK_NAME) & VIP_IDLE_CLK_RATIO_MASK(CLK_NAME)) \
		>> VIP_IDLE_CLK_RATIO_OFFSET(CLK_NAME))

#define VIP_UPDATE_CLK_RATIO_MASK(CLK_NAME) VIP_SYS_REG_UPDATE_##CLK_NAME##_MASK
#define VIP_UPDATE_CLK_RATIO_OFFSET(CLK_NAME) VIP_SYS_REG_UPDATE_##CLK_NAME##_OFFSET
#define VIP_UPDATE_CLK_RATIO(CLK_NAME) \
	vip_sys_reg_write_mask(VIP_SYS_REG_UPDATE_##CLK_NAME, \
		VIP_UPDATE_CLK_RATIO_MASK(CLK_NAME), \
		1 << VIP_UPDATE_CLK_RATIO_OFFSET(CLK_NAME))

struct vip_rect {
	u16 x;
	u16 y;
	u16 w;
	u16 h;
};

union vip_sys_reset {
	struct {
		u32 resv_b0 : 1;
		u32 isp_top : 1;
		u32 img_d : 1;
		u32 img_v : 1;
		u32 sc_top : 1;
		u32 sc_d : 1;
		u32 sc_v1 : 1;
		u32 sc_v2 : 1;
		u32 resv_b8 : 1;
		u32 disp : 1;
		u32 bt : 1;
		u32 dsi_mac : 1;
		u32 csi_mac0 : 1;
		u32 csi_mac1 : 1;
		u32 ldc : 1;
		u32 clk_div : 1;
		u32 rsv_b16 : 1;
		u32 isp_top_apb : 1;
		u32 sc_top_apb : 1;
		u32 ldc_top_apb : 1;
		u32 dsi_mac_apb : 1;
		u32 csi_mac0_apb : 1;
		u32 csi_mac1_apb : 1;
		u32 dsi_phy_apb : 1;
		u32 csi_phy0_apb : 1;
		u32 rsv_b25 : 1;
		u32 dsi_phy : 1;
		u32 csi_phy0 : 1;
		u32 rsv_b28 : 1;
		u32 csi_be : 1;
	} b;
	u32 raw;
};

union vip_sys_isp_clk {
	struct {
		u32 rsv : 18;
		u32 clk_isp_top_en : 1;
		u32 clk_axi_isp_en : 1;
		u32 clk_csi_mac0_en : 1;
		u32 clk_csi_mac1_en : 1;
	} b;
	u32 raw;
};

union vip_sys_clk {
	struct {
		u32 sc_top : 1;
		u32 isp_top : 1;
		u32 ldc_top : 1;
		u32 rev_b3 : 1;
		u32 vip_sys : 1;
		u32 csi_phy : 1;
		u32 dsi_phy : 1;
		u32 rev_b7 : 1;
		u32 csi_mac0 : 1;
		u32 csi_mac1 : 1;
		u32 rsv2 : 2;
		u32 sc_x2p_busy_en : 1;
		u32 isp_x2p_busy_en : 1;
		u32 ldc_x2p_busy_en : 1;
		u32 rev_b15 : 1;
		u32 auto_sc_top : 1;
		u32 auto_isp_top : 1;
		u32 auto_ldc : 1;
		u32 rev_b19 : 1;
		u32 auto_vip_sys : 1;
		u32 auto_csi_phy : 1;
		u32 auto_dsi_phy : 1;
		u32 rev_b23 : 1;
		u32 auto_csi_mac0 : 1;
		u32 auto_csi_mac1 : 1;
	} b;
	u32 raw;
};

union vip_sys_intr {
	struct {
		u32 sc : 1;
		u32 rsv1 : 15;
		u32 isp : 1;
		u32 rsv2 : 7;
		u32 dwa : 1;
		u32 rsv3 : 3;
		u32 rot : 1;
		u32 csi_mac0 : 1;
		u32 csi_mac1 : 1;
	} b;
	u32 raw;
};

enum vip_sys_axi_bus {
	VIP_SYS_AXI_BUS_SC_TOP = 0,
	VIP_SYS_AXI_BUS_ISP_RAW,
	VIP_SYS_AXI_BUS_ISP_YUV,
	VIP_SYS_AXI_BUS_MAX,
};

struct ol_tile {
	u16 start;
	u16 end;
};

struct sc_cfg_cb {
	struct cvi_vip_dev *dev;
	struct _ol_tile_cfg {
		struct ol_tile l_in;
		struct ol_tile l_out;
		struct ol_tile r_in;
		struct ol_tile r_out;
	} ol_tile_cfg;

	u8  snr_num;
	u8  is_tile;
	u8  is_left_tile;
};

/********************************************************************
 *   APIs to replace bmtest's standard APIs
 ********************************************************************/
void vip_set_base_addr(void *base);
union vip_sys_clk vip_get_clk_lp(void);
void vip_set_clk_lp(union vip_sys_clk clk);
union vip_sys_reset vip_get_reset(void);
void vip_set_reset(union vip_sys_reset reset);
void vip_toggle_reset(union vip_sys_reset mask);
union vip_sys_intr vip_get_intr_status(void);
unsigned int vip_sys_reg_read(uintptr_t addr);
void vip_sys_reg_write_mask(uintptr_t addr, u32 mask, u32 data);
void vip_sys_set_offline(enum vip_sys_axi_bus bus, bool offline);
int vip_sys_register_cmm_cb(unsigned long cmm, void *hdlr, void *cb);
int vip_sys_cmm_cb_i2c(unsigned int cmd, void *arg);
int vip_sys_cmm_cb_ssp(unsigned int cmd, void *arg);
int vip_sys_register_cif_cb(void *hdlr, void *cb);
int vip_sys_cif_cb(unsigned int cmd, void *arg);
u32 cvi_sc_cfg_cb(struct sc_cfg_cb *post_para);
void cvi_sc_qbuf_trigger_str(struct cvi_vip_dev *dev);
void cvi_sc_frm_done_cb(struct cvi_vip_dev *dev);
#endif
