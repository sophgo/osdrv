#ifndef __VPSS_IP_CTRL_H__
#define __VPSS_IP_CTRL_H__

#include "base_ctx.h"
#include "vpss_cb.h"

#define YRATIO_SCALE         100

struct vpss_cmdq_buf {
	__u64 cmdq_phy_addr;
	void *cmdq_vir_addr;
	__u32 cmdq_buf_size;
};

enum sc_flip_mode {
	SC_FLIP_NO,
	SC_FLIP_HFLIP,
	SC_FLIP_VFLIP,
	SC_FLIP_HVFLIP,
	SC_FLIP_MAX
};

enum sc_quant_rounding {
	SC_QUANT_ROUNDING_TO_EVEN = 0,
	SC_QUANT_ROUNDING_AWAY_FROM_ZERO,
	SC_QUANT_ROUNDING_TRUNCATE,
	SC_QUANT_ROUNDING_MAX,
};

enum sc_scaling_coef {
	SC_SCALING_COEF_BICUBIC = 0,
	SC_SCALING_COEF_BILINEAR,
	SC_SCALING_COEF_NEAREST,
	SC_SCALING_COEF_BICUBIC_OPENCV,
	SC_SCALING_COEF_MAX,
};

/* struct sc_quant_param
 *   parameters for quantization, output format fo sc must be RGB/BGR.
 *
 * @sc_frac: fractional number of the scaling-factor. [13bits]
 * @sub: integer number of the means.
 * @sub_frac: fractional number of the means. [10bits]
 */
struct sc_quant_param {
	__u16 sc_frac[3];
	__u8  sub[3];
	__u16 sub_frac[3];
	enum sc_quant_rounding rounding;
	bool enable;
};

struct convertto_param {
	bool enable;
	__u32 a_frac[3];
	__u32 b_frac[3];
};

/* struct sc_border_param
 *   only work if sc offline and sc_output size < fmt's setting
 *
 * @bg_color: rgb format
 * @offset_x: offset of x
 * @offset_y: offset of y
 * @enable: enable or disable
 */
struct sc_border_param {
	__u32 bg_color[3];
	__u16 offset_x;
	__u16 offset_y;
	bool enable;
};

struct sc_border_vpp_param {
	bool enable;
	__u8 bg_color[3];
	struct vip_range inside;
	struct vip_range outside;
};

/* struct sc_mute
 *   cover sc with the specified rgb-color if enabled.
 *
 * @color: rgb format
 * @enable: enable or disable
 */
struct sc_mute {
	__u8 color[3];
	bool enable;
};

struct odma_sb_cfg {
	__u8 sb_mode; //0: disable, 1: free-run mode, 2: frame-base mode
	__u8 sb_size; //slice buffer line number, 0: 64 line, 1: 128 line
	__u8 sb_nb; //slice buffer depth
	__u8 sb_full_nb;
	__u8 sb_wr_ctrl_idx; //0: sb_wr_ctrl0, 1: sb_wr_ctrl1
};


struct vpss_sc_cfg {
	__u32 pixelformat;
	__u32 bytesperline[2];
	__u64 addr[3];
	__u32 y_ratio;
	struct vip_frmsize src_size;
	struct vip_rect crop;
	struct vip_rect dst_rect;
	struct vip_frmsize dst_size;
	struct rgn_cfg rgn_cfg[RGN_MAX_LAYER_VPSS];
	struct rgn_coverex_cfg rgn_coverex_cfg;
	struct rgn_mosaic_cfg rgn_mosaic_cfg;
	struct sc_quant_param quant_cfg;
	struct convertto_param convert_to_cfg;
	struct sc_border_param border_cfg;
	struct sc_border_vpp_param border_vpp_cfg[VPSS_RECT_NUM];
	struct sc_mute mute_cfg;
	struct csc_cfg csc_cfg;
	struct odma_sb_cfg sb_cfg;
	enum sc_flip_mode flip;
	enum sc_scaling_coef sc_coef;
};

struct vpss_img_in_cfg {
	bool online_from_isp;
	bool upsample;
	__u32 pixelformat;
	__u32 bytesperline[2];
	__u64 addr[4];
	struct vip_frmsize src_size;
	struct vip_rect crop;
	struct csc_cfg csc_cfg;
};

struct vpss_interrupter_status {
	bool sc_end;
	bool cmdq_end;
};

struct vpss_csc_matrix {
	__u16 coef[3][3];
	__u8 sub[3];
	__u8 add[3];
};

enum vpss_csc {
	VPSS_CSC_NONE,
	VPSS_CSC_601_LIMIT_YUV2RGB,
	VPSS_CSC_601_FULL_YUV2RGB,
	VPSS_CSC_709_LIMIT_YUV2RGB,
	VPSS_CSC_709_FULL_YUV2RGB,
	VPSS_CSC_601_LIMIT_RGB2YUV,
	VPSS_CSC_601_FULL_RGB2YUV,
	VPSS_CSC_709_LIMIT_RGB2YUV,
	VPSS_CSC_709_FULL_RGB2YUV,
	VPSS_CSC_DATATYPE,
	VPSS_CSC_MAX,
};


void vpss_set_base_addr(void *base_addr);
void vpss_ip_init(u8 dev_idx, bool is_resume);
void vpss_ip_reset(u8 dev_idx, bool is_sbm, bool toggle);
void img_update(u8 dev_idx, bool is_master, const struct vpss_img_in_cfg *img_cfg);
void sc_update(u8 dev_idx, const struct vpss_sc_cfg *chn_cfg);
void top_update(u8 dev_idx, bool is_share, bool sc_enable);
void img_start(u8 dev_idx, u8 chn_num);
bool img_left_tile_cfg(u8 dev_idx, u16 online_l_width);
bool img_right_tile_cfg(u8 dev_idx, u16 online_r_start, u16 online_r_end);
void vpss_interrupter_clear(u8 dev_idx, struct vpss_interrupter_status *status);
u32 vpss_get_checksum(u8 dev_idx);

void vpss_stauts(u8 dev_idx);
void vpss_error_stauts(u8 dev_idx);
void vpss_get_csc_mtrx(enum vpss_csc csc, struct vpss_csc_matrix *csc_matrix);

int vpss_stitch_run(u8 dev_idx, struct vpss_cmdq_buf *cmdq_buf, struct vpss_stitch_cfg *cfg);

void vpss_get_gop_addr(u8 inst, u8 layer, u8 ow_inst, u64 *addr);
void vpss_get_sbm_pos(u8 inst, int *y_pos, int *uv_pos);
#endif
