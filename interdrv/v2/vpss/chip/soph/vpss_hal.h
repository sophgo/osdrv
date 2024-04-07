#ifndef __VPSS_HAL_H__
#define __VPSS_HAL_H__

#include <linux/cvi_vip.h>
#include <linux/list.h>
#include <linux/cvi_defines.h>

#include "vpss_core.h"
#include "vpss_cb.h"

#define YRATIO_SCALE         100
#define VPSS_CMDQ_BUF_SIZE (0x8000)

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
	CVI_SC_SCALING_COEF_BICUBIC_OPENCV,
	CVI_SC_SCALING_COEF_MAX,
};

enum cvi_job_state {
	CVI_JOB_WAIT,
	CVI_JOB_WORKING,
	CVI_JOB_HALF,
	CVI_JOB_END,
	CVI_JOB_INVALID,
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

struct cvi_convertto_param {
	bool enable;
	__u32 a_frac[3];
	__u32 b_frac[3];
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

struct cvi_sc_border_vpp_param {
	bool enable;
	__u8 bg_color[3];
	struct cvi_range inside;
	struct cvi_range outside;
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

struct cvi_odma_sb_cfg {
	__u8 sb_mode;
	__u8 sb_size;
	__u8 sb_nb;
	__u8 sb_full_nb;
	__u8 sb_wr_ctrl_idx;
};

struct cvi_vpss_chn_cfg {
	__u32 pixelformat;
	__u32 bytesperline[2];
	__u64 addr[3];
	__u32 YRatio;
	struct cvi_vpss_frmsize src_size;
	struct cvi_rect crop;
	struct cvi_rect dst_rect;
	struct cvi_vpss_frmsize dst_size;
	struct cvi_rgn_cfg rgn_cfg[RGN_MAX_LAYER_VPSS];
	struct cvi_rgn_coverex_cfg rgn_coverex_cfg;
	struct cvi_rgn_mosaic_cfg rgn_mosaic_cfg;
	struct cvi_sc_quant_param quant_cfg;
	struct cvi_convertto_param convert_to_cfg;
	struct cvi_sc_border_param border_cfg;
	struct cvi_sc_border_vpp_param border_vpp_cfg[VPSS_RECT_NUM];
	struct cvi_sc_mute mute_cfg;
	struct cvi_csc_cfg csc_cfg;
	enum cvi_sc_flip_mode flip;
	enum cvi_sc_scaling_coef sc_coef;
};

struct cvi_vpss_grp_cfg {
	bool fbd_enable;
	bool online_from_isp;
	bool upsample;
	bool vpss_v_priority;
	__u32 pixelformat;
	__u32 bytesperline[2];
	__u64 addr[4];
	struct cvi_vpss_frmsize src_size;
	struct cvi_rect crop;
	struct cvi_csc_cfg csc_cfg;
};

struct cvi_vpss_online_cb {
	__u8 snr_num;
	__u8 is_tile;
	__u8 is_left_tile;
	struct cvi_line in;
	struct cvi_line out;
};

struct cvi_vpss_hw_cfg {
	__u8 chn_num;
	__u8 chn_enable[VPSS_MAX_CHN_NUM];
	struct cvi_vpss_grp_cfg stGrpCfg;
	struct cvi_vpss_chn_cfg astChnCfg[VPSS_MAX_CHN_NUM];
};

typedef void (*vpss_job_cb)(void *data);

struct cvi_vpss_job {
	struct cvi_vpss_hw_cfg cfg;
	__u8 grp_id;
	__u8 workingMask;
	__u8 dev_idx_start;
	__u8 tile_mode;
	bool is_online;
	bool is_tile;
	bool is_work_on_r_tile;
	bool is_v_tile;
	vpss_job_cb pfnJobCB;
	void *data;
	atomic_t enJobState;
	__u32 vpss_dev_mask;
	__u32 checksum[VPSS_MAX_CHN_NUM];
	struct list_head list;
	u32 u32HwDuration;
	struct work_struct job_work;
};

void cvi_vpss_irq_handler(struct vpss_core *vpss_dev);
int cvi_vpss_hal_init(struct cvi_vpss_device *dev);
void cvi_vpss_hal_deinit(void);

int cvi_vpss_hal_push_job(struct cvi_vpss_job *pstJob);
int cvi_vpss_hal_push_online_job(struct cvi_vpss_job *pstJob);
int cvi_vpss_hal_remove_job(struct cvi_vpss_job *pstJob);
int cvi_vpss_hal_try_schedule(void);
int cvi_vpss_hal_online_run(struct cvi_vpss_online_cb *param);
void cvi_vpss_hal_online_release_dev(void);

int cvi_vpss_hal_stitch_schedule(struct vpss_stitch_cfg *pstCfg);
void cvi_vpss_cmdq_irq_handler(struct vpss_core *vpss_dev);

#endif // _U_SC_UAPI_H_
