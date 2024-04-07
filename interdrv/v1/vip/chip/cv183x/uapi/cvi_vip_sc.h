#ifndef _U_CVI_VIP_SC_H_
#define _U_CVI_VIP_SC_H_

enum cvi_sc_flip_mode {
	CVI_SC_FLIP_NO,
	CVI_SC_FLIP_HFLIP,
	CVI_SC_FLIP_VFLIP,
	CVI_SC_FLIP_HVFLIP,
	CVI_SC_FLIP_MAX
};

enum cvi_sc_quant_rounding {
	CVI_SC_QUANT_ROUNDING_TO_EVEN = 0,
	CVI_SC_QUANT_ROUNDING_AWAY_FROM_ZERO,
	CVI_SC_QUANT_ROUNDING_TRUNCATE,
	CVI_SC_QUANT_ROUNDING_MAX,
};

enum cvi_sc_scaling_coef {
	CVI_SC_SCALING_COEF_BICUBIC = 0,
	CVI_SC_SCALING_COEF_BILINEAR,
	CVI_SC_SCALING_COEF_NEAREST,
	CVI_SC_SCALING_COEF_Z2,
	CVI_SC_SCALING_COEF_Z3,
	CVI_SC_SCALING_COEF_DOWNSCALE_SMOOTH,
	CVI_SC_SCALING_COEF_OPENCV_BILINEAR,
	CVI_SC_SCALING_COEF_MAX,
};

/* struct cvi_sc_quant_param
 *   parameters for quantization, output format fo sc must be RGB/BGR.
 *
 * @sc_frac: fractional number of the scaling-factor. [13bits]
 * @sub: integer number of the means.
 * @sub_frac: fractional number of the means. [10bits]
 */
struct cvi_sc_quant_param {
	__u16 sc_frac[3];
	__u8  sub[3];
	__u16 sub_frac[3];
	enum cvi_sc_quant_rounding rounding;
	bool enable;
};

/* struct cvi_sc_border_param
 *   only work if sc offline and sc_output size < fmt's setting
 *
 * @bg_color: rgb format
 * @offset_x: offset of x
 * @offset_y: offset of y
 * @enable: enable or disable
 */
struct cvi_sc_border_param {
	__u32 bg_color[3];
	__u16 offset_x;
	__u16 offset_y;
	bool enable;
};

/* struct cvi_sc_mute
 *   cover sc with the specified rgb-color if enabled.
 *
 * @color: rgb format
 * @enable: enable or disable
 */
struct cvi_sc_mute {
	__u8 color[3];
	bool enable;
};

/*
 * enable: Enable low delay or not.
 * lineCnt: line cnt to notify.
 */
struct cvi_sc_low_delay_cfg {
	bool enable;
	__u32 lineCnt;
};

struct cvi_vpss_chn_cfg {
	__u8 grp_id;
	bool is_online;
	struct v4l2_frmsize_discrete src_size;
	__u32 pixelformat;
	__u32 bytesperline[2];
	struct v4l2_rect crop;
	struct cvi_csc_cfg csc_cfg;
	struct v4l2_rect dst_rect;
	struct v4l2_frmsize_discrete dst_size;
	enum cvi_sc_flip_mode flip;
	enum cvi_sc_scaling_coef sc_coef;
	struct cvi_rgn_cfg rgn_cfg;
	struct cvi_sc_quant_param quant_cfg;
	struct cvi_sc_border_param border_cfg;
	struct cvi_sc_mute mute_cfg;
	struct cvi_sc_low_delay_cfg low_delay_cfg;
};

struct cvi_sc_buf_cfg {
	__u8 grp_id;
	bool is_online_from_isp;
};

#endif // _U_CVI_VIP_SC_H_
