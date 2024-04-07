#ifndef _U_CVI_VIP_SNSR_H_
#define _U_CVI_VIP_SNSR_H_

#include "cvi_vip_cif.h"

#define SNS_AWB_LUM_HIST_NUM	6
#define MAX_IMAGE_MODE_NUM	6
#define MAX_WDR_FRAME_NUM	2
#define ISP_MAX_SNS_REGS	32
#define EXP_RATIO_NUM		3

/**
 * struct active_size_s - linear/wdr image information
 *
 * @width: image total width
 * @height: image total height
 * @start_x: horizontal shift of the 1st pixel
 * @start_y: horizontal shift of the 1st pixel
 * @active_w: effective image width
 * @active_h: effective image height
 * @max_width: max width for buffer allocation
 * @max_height: max height for buffer allocation
 */

struct active_size_s {
	unsigned short		width;
	unsigned short		height;
	unsigned short		start_x;
	unsigned short		start_y;
	unsigned short		active_w;
	unsigned short		active_h;
	unsigned short		max_width;
	unsigned short		max_height;
};

/**
 * struct wdr_size_s - structure for CVI_SNSR_G_WDR_SIZE
 *
 * @frm_num: [output] Effective image instance. 1 for linear mode, >1 for wdr mode.
 * @img_size: [output] Image information.
 */

struct wdr_size_s {
	unsigned int		frm_num;
	struct active_size_s	img_size[MAX_WDR_FRAME_NUM];
};

/**
 * struct snsr_rx_attr_s - structure for mipi_rx configuration.
 *
 * @is_hdr: [intput] Ask the sensor driver to return the mipi_rx configuration
 *	with hdr enabled.
 * @dev_attr: [output] mipi_rx configuration.
 */

struct snsr_rx_attr_s {
	unsigned int		is_hdr;
	struct combo_dev_attr_s	dev_attr;
};

enum isp_sns_type_e {
	ISP_SNS_I2C_TYPE = 0,
	ISP_SNS_TYPE_BUTT,
};

enum sns_wdr_e {
	SNS_WDR_MODE_NONE = 0,
	SNS_WDR_MODE_2TO1_LINE,
	SNS_WDR_MODE_BUTT
};

/**
 * struct isp_i2c_data - sensor setting with i2c interface.
 *
 * @update: update this register or not
 * @i2c_dev: i2c device number.
 * @dev_addr: sensor slave address
 * @dly_frm_num: this setting would be set with delay frame number
 * @reg_addr: sensor register address
 * @addr_bytes: sensor register address bytes number
 * @data: sensor register value
 * @data_bytes: sensor register value bytes number
 */

struct isp_i2c_data {
	unsigned char	update;
	unsigned char	i2c_dev;
	unsigned char	dev_addr;
	unsigned char	dly_frm_num;
	unsigned short	reg_addr;
	unsigned short	addr_bytes;
	unsigned short	data;
	unsigned short	data_bytes;
};

/**
 * struct snsr_regs_s - structure of sensor update wrapper
 *
 * @sns_type: i2c or other interface
 * @regs_num: the maximum sensor registers to be updated
 * @i2c_data: sensor registers to be updated
 * @use_snsr_sram: does this sensor support group update
 * @need_update: global flag for sensor update. Ignore this wrapper
 *	when it is zero.
 */

struct snsr_regs_s {
	enum isp_sns_type_e	sns_type;
	unsigned int		regs_num;
	struct isp_i2c_data	i2c_data[ISP_MAX_SNS_REGS];
	unsigned char		cfg_valid_max_dly_frm;
	unsigned char		use_snsr_sram;
	unsigned char		need_update;
};

/**
 * struct snsr_isp_s - structure of isp update wrapper
 *
 * @wdr: the image information for isp driver.
 * @need_update: global flag for isp update. Ignore this wrapper
 *	when it is zero.
 */

struct snsr_isp_s {
	struct wdr_size_s	wdr;
	unsigned char		dly_frm_num;
	unsigned char		need_update;
};

/**
 * struct snsr_cif_s - structure of cif(mipi_rx) update wrapper
 *
 * @wdr: the image information for isp driver.
 * @need_update: global flag for cif update. Ignore this wrapper
 *	when it is zero.
 */

struct snsr_cif_s {
	struct manual_wdr_s	wdr_manu;
	unsigned char		dly_frm_num;
	unsigned char		need_update;
};

/**
 * struct snsr_cfg_node_s - structure of cfg node for runtime update
 *
 * @snsr: [output] snsr wrapper for runtime update
 * @isp: [output] isp wrapper for runtime update
 * @cif: [output] cif wrapper for runtime update
 * @configed: [intput] after CVI_SNSR_G_CFG_NODE is called, this flag
 *	is set as false by sensor driver. The caller shall set it as
 *	true after this cfg_node is passed to isp driver.
 */

struct snsr_cfg_node_s {
	struct snsr_regs_s	snsr;
	struct snsr_isp_s	isp;
	struct snsr_cif_s	cif;
	unsigned char		configed;
};

/**
 * struct snsr_mode_s - describes the image mode.
 *
 * @width: image total width
 * @height: image total height
 * @fps_nume: the numerator of the fps.
 * @fps_denom: the denominator for the fps.
 * @pixel_rate: the pixel rate
 */

struct snsr_mode_s {
	unsigned short		width;
	unsigned short		height;
	unsigned int		fps_nume;
	unsigned int		fps_denom;
	unsigned int		pixel_rate;
};

/**
 * struct snsr_info_s - describes sensor information
 *
 * @mode: support image mode.
 * @mode_num: support image mode numbers.
 * @stable_ms: the stable time after stream on in ms
 * @clk2unrst_us: the mclk to sensor unreset delay in us
 * @unrst2init_us: the unrst to stream on delay in us
 * @code: the support image format in media-bus-format.h
 */

struct snsr_info_s {
	struct snsr_mode_s	mode[MAX_IMAGE_MODE_NUM];
	unsigned short		mode_num;
	unsigned short		stable_ms;
	unsigned short		clk2unrst_us;
	unsigned short		unrst2init_us;
	unsigned int		code;
};

/**
 * struct snsr_gain_calc_s - structure used for gain conversion.
 *
 * @gain_lin: [input] the gain in real value
 * @gain_db: [output] the gain to be set in CVI_SNSR_GAINS_UPDATE
 *
 * sensor would return the closest (and smaller than) gain_db
 *	according the input gain_lin.
 * Ex. when gain_lin = 1024, gain_db = 0.
 *	when gain_lin = 1219, gain_db = 5.
 */

struct snsr_gain_calc_s {
	unsigned int		gain_lin;
	unsigned int		gain_db;
};

/**
 * struct snsr_gain_update_s - structure for gain update
 *
 * @again: [input] the analogue gain value to be updated
 * @dgain: [input] the digital gain value to be updated
 *
 */

struct snsr_gain_update_s {
	unsigned int		again[MAX_WDR_FRAME_NUM];
	unsigned int		dgain[MAX_WDR_FRAME_NUM];
};

/**
 * struct snsr_inttime_max_s - Use in wdr mode to get the inttime ranges.
 *
 * @man_ratio_en: [input] When enaled, sensor driver uses the ratio field to calculate
 *	the max/min inttime.
 * @ratio: [input] the ratio betwen s/vs, m/s, l/m. s:short vs: very short, m: middle l: long.
 *	For 2 frames wdr, only s/vs is used. The ratio is divided by 64 to calculate the
 *	inttime. Ex. when ratio[0] is 1024, the s/vs ratio is 1024/64=16.
 * #inttime_max: [output] The maximum inttime for vs, s, m and l frames.
 * #inttime_min: [output] The mimimum inttime for vs, s, m and l frames.
 *
 */

struct snsr_inttime_max_s {
	unsigned short		man_ratio_en;
	unsigned int		ratio[EXP_RATIO_NUM];	/* input: s/vs, m/s, l/m */
	unsigned int		inttime_max[4];		/* output: vs s m l*/
	unsigned int		inttime_min[4];		/* output: vs s m l*/
};

/**
 * struct snsr_inttime_s - the structure for inttime update
 *
 * @inttime: [input] the updated inttime. The 1st entry is for long frame
 *	and the 2nd is for the short frames in two-frames wdr mode.
 *
 */

struct snsr_inttime_s {
	unsigned int		inttime[MAX_WDR_FRAME_NUM];
};

enum sns_gain_mode_e {
	SNS_FLAG_GAINM_SHARE = 0,	/* gain setting for all wdr frames*/
	SNS_FLAG_GAINM_WDR_2F,		/* separate gain for 2-frame wdr mode*/
	SNS_FALG_GAINM_WDR_3F,		/* separate gain for 3-frame wdr mode*/
	SNS_FALG_GAINM_ONLY_LEF		/* gain setting only apply to lef and sef is fixed to 1x */
};

enum sns_ae_strategy_e {
	SNS_AE_EXP_HIGHLIGHT_PRIOR = 0,
	SNS_AE_EXP_LOWLIGHT_PRIOR  = 1,
	SNS_AE_STRATEGY_MODE_BUTT
};

/**
 * struct snsr_ae_def_s - the structure for get_ae_def
 *
 * @FL_std: The typical sensor output lines of one frame.
 * @FL_max: The maximum sensor output lines of one frame.
 * @FL: The actual sensor output lines of one frame.
 * @max_again: The maximum analogue gain value.
 * @min_again: The minimum analogue gain value.
 * @max_dgain: The maximum digital gain value.
 * @min_dgain: The minimum digital gain value.
 * @max_inttime: The maximum inttime.
 * @min_inttime: The minimum inttime.
 * @ratio: The ratio betwen s/vs, m/s, l/m. s:short vs: very short, m: middle l: long.
 *	For 2 frames wdr, only s/vs is used. The ratio is divided by 64 to calculate the
 *	inttime. Ex. when ratio[0] is 1024, the s/vs ratio is 1024/64=16.
 * @gain_mode: the support gain mode defined in sns_gain_mode_e.
 * @man_ratio_en: Manual ratio enable. When enabled, the sensor driver supports using the
 *	ratio field to calculate the max/min inttime by CVI_SNSR_G_INTTIME_MAX.
 * @init_AESpeed: default AE tuning speed.
 * @init_AETolerance: default AE tolerance value.
 * @ae_compensation: AE compensation value.
 * @hist_thresh:
 * @Hmax_times: the maximum duration for one line.
 * @lines_per_500ms: the maximum lines number per 500ms.
 * @flicker_freq:
 * @fps_nume:
 * @fps_denom:
 * @init_exposure:
 * @ISP_dgain_shift:
 * @max_inttime_step:
 * @LF_max_shorttime:
 * @LF_min_exposure:
 * @ae_exp_mode:
 * @ISO_cal_coef:
 * @ae_run_interval:
 *
 */

struct snsr_ae_def_s {
	unsigned int		FL_std;
	unsigned int		FL_max;
	unsigned int		FL;
	unsigned int		max_again;
	unsigned int		min_again;
	unsigned int		max_dgain;
	unsigned int		min_dgain;
	unsigned int		max_inttime;
	unsigned int		min_inttime;
	unsigned int		ratio[EXP_RATIO_NUM];
	unsigned int		init_AESpeed;
	unsigned int		init_AETolerance;
	unsigned int		Hmax_times;
	unsigned int		lines_per_500ms;
	unsigned int		flicker_freq;
	unsigned int		fps_nume;
	unsigned int		fps_denom;
	unsigned int		init_exposure;
	unsigned int		ISP_dgain_shift;
	unsigned int		max_inttime_step;
	unsigned int		LF_max_shorttime;
	unsigned int		LF_min_exposure;
	unsigned int		ae_exp_mode;
	unsigned short		ISO_cal_coef;
	unsigned char		gain_mode;
	unsigned char		man_ratio_en;
	unsigned char		ae_compensation;
	unsigned char		ae_run_interval;
	unsigned char		hist_thresh[SNS_AWB_LUM_HIST_NUM];
};

/**
 * struct snsr_awb_def_s - the structure for get_awb_def
 * @golden_rgain: G/R value of golden sample
 * @golden_bgain: G/b value of golden sample
 * @sample_rgain: G/R value of current set
 * @sample_bgain: G/b value of current set
 * @init_rgain: default R gain value
 * @init_ggain: default G gain value
 * @init_bgain: default B gain value
 * @awb_run_interval: awb interval
 *
 */

struct snsr_awb_def_s {
	unsigned short		golden_rgain;
	unsigned short		golden_bgain;
	unsigned short		sample_rgain;
	unsigned short		sample_bgain;
	unsigned short		init_rgain;
	unsigned short		init_ggain;
	unsigned short		init_bgain;
	unsigned char		awb_run_interval;
};
/**
 * struct snsr_fps_s - the data structure for CVI_SNSR_S_FPS.
 *
 * @fps_nume: [input] The numerator of the fps.
 * @fps_denom: [input] The denominator for the fps.
 * @max_inttime: [output] The maximum inttime after changing the fps.
 * @FL_std: [output] The typical output lines of one frames after changing the fps.
 *
 */

struct snsr_fps_s {
	unsigned int		fps_nume;
	unsigned int		fps_denom;
	unsigned int		max_inttime;
	unsigned int		FL_std;
	unsigned int		Hmax_times;
	unsigned int		lines_per_500ms;
};

#define CVI_CAMERA_CID_BASE	(V4L2_CTRL_CLASS_CAMERA | 0x3000)

#define CVI_CAMERA_CID_LEXP	(CVI_CAMERA_CID_BASE + 0)
#define CVI_CAMERA_CID_SEXP	(CVI_CAMERA_CID_BASE + 1)
#define CVI_CAMERA_CID_LGAIN	(CVI_CAMERA_CID_BASE + 2)
#define CVI_CAMERA_CID_SGAIN	(CVI_CAMERA_CID_BASE + 3)

/* sensor ioctl commands related definition */
#define CVI_SNSR_IOC_MAGIC	's'
#define CVI_SNSR_G_WDR_SIZE	_IOR(CVI_SNSR_IOC_MAGIC, 1, struct wdr_size_s)
#define CVI_SNSR_G_RX_ATTR	_IOWR(CVI_SNSR_IOC_MAGIC, 2, \
					struct snsr_rx_attr_s)
#define CVI_SNSR_S_POWER	_IOWR(CVI_SNSR_IOC_MAGIC, 3, unsigned int)
#define CVI_SNSR_S_STREAM	_IOWR(CVI_SNSR_IOC_MAGIC, 4, unsigned int)
#define CVI_SNSR_S_IMAGE_MODE	_IOWR(CVI_SNSR_IOC_MAGIC, 5, \
					struct snsr_mode_s)
#define CVI_SNSR_G_IMAGE_MODE	_IOWR(CVI_SNSR_IOC_MAGIC, 6, \
					struct snsr_info_s)
#define CVI_SNSR_S_WDR_MODE	_IOWR(CVI_SNSR_IOC_MAGIC, 7, unsigned int)
#define CVI_SNSR_AGAIN_CALC_TABLE	_IOWR(CVI_SNSR_IOC_MAGIC, 8, struct snsr_gain_calc_s)
#define CVI_SNSR_DGAIN_CALC_TABLE	_IOWR(CVI_SNSR_IOC_MAGIC, 9, struct snsr_gain_calc_s)
#define CVI_SNSR_GAINS_UPDATE	_IOW(CVI_SNSR_IOC_MAGIC, 10, struct snsr_gain_update_s)
#define CVI_SNSR_GLOBAL_INIT	_IOW(CVI_SNSR_IOC_MAGIC, 11, unsigned int)
#define CVI_SNSR_G_CFG_NODE	_IOWR(CVI_SNSR_IOC_MAGIC, 12, struct snsr_cfg_node_s)
#define CVI_SNSR_G_INTTIME_MAX	_IOWR(CVI_SNSR_IOC_MAGIC, 13, struct snsr_inttime_max_s)
#define CVI_SNSR_INTTIME_UPDATE	_IOW(CVI_SNSR_IOC_MAGIC, 14, struct snsr_inttime_s)
#define CVI_SNSR_G_AE_DEF	_IOR(CVI_SNSR_IOC_MAGIC, 16, struct snsr_ae_def_s)
#define CVI_SNSR_S_FPS		_IOWR(CVI_SNSR_IOC_MAGIC, 17, struct snsr_fps_s)
#define CVI_SNSR_G_AWB_DEF	_IOR(CVI_SNSR_IOC_MAGIC, 18, struct snsr_awb_def_s)

#endif // _U_CVI_VIP_SNSR_H_
