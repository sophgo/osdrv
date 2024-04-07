/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: isp_drv.h
 * Description:
 */

#ifndef _ISP_DRV_H_
#define _ISP_DRV_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include "vip_common.h"
#include "../uapi/cvi_vip_tun_cfg.h"
#include "../mw/vpu_base.h"

#ifndef _OFST
#define _OFST(_BLK_T, _REG)       ((uintptr_t)&(((struct _BLK_T *)0)->_REG))
#endif

#define ISP_RD_REG_BA(_BA) \
	(_reg_read(_BA))

#define ISP_RD_REG(_BA, _BLK_T, _REG) \
	(_reg_read(_BA+_OFST(_BLK_T, _REG)))

#define ISP_RD_BITS(_BA, _BLK_T, _REG, _FLD) \
	({\
		typeof(((struct _BLK_T *)0)->_REG) _r;\
		_r.raw = _reg_read(_BA+_OFST(_BLK_T, _REG));\
		_r.bits._FLD;\
	})

#define ISP_WR_REG(_BA, _BLK_T, _REG, _V) \
	(_reg_write((_BA+_OFST(_BLK_T, _REG)), _V))

#define ISP_WR_REG_OFT(_BA, _BLK_T, _REG, _OFT, _V) \
	(_reg_write((_BA+_OFST(_BLK_T, _REG) + _OFT), _V))

#define ISP_WR_BITS(_BA, _BLK_T, _REG, _FLD, _V) \
	do {\
		typeof(((struct _BLK_T *)0)->_REG) _r;\
		_r.raw = _reg_read(_BA+_OFST(_BLK_T, _REG));\
		_r.bits._FLD = _V;\
		_reg_write((_BA+_OFST(_BLK_T, _REG)), _r.raw);\
	} while (0)

#define ISP_WO_BITS(_BA, _BLK_T, _REG, _FLD, _V) \
	do {\
		typeof(((struct _BLK_T *)0)->_REG) _r;\
		_r.raw = 0;\
		_r.bits._FLD = _V;\
		_reg_write((_BA+_OFST(_BLK_T, _REG)), _r.raw);\
	} while (0)

#define ISP_WR_REGS_BURST(_BA, _BLK_T, _REG, _SIZE, _STR)\
	do {\
		u32 k = 0;\
		uintptr_t ofst = _OFST(_BLK_T, _REG);\
		for (; k < sizeof(_SIZE) / 0x4; k++) {\
			u32 val = (&_STR + k)->raw;\
			_reg_write((_BA + ofst + (k * 0x4)), val);\
		} \
	} while (0)

#define ISP_WR_REG_LOOP_SHFT(_BA, _BLK_T, _REG, _TOTAL_SIZE, _SFT_SIZE, _LUT, _SFT_BIT) \
	do {\
		uint16_t i = 0, j = 0;\
		uint32_t val = 0;\
		for (; i < _TOTAL_SIZE / _SFT_SIZE; i++) {\
			val = 0;\
			for (j = 0; j < _SFT_SIZE; j++) {\
				val += (_LUT[(i * _SFT_SIZE) + j] << (_SFT_BIT * j));\
			} \
			_reg_write((_BA + _OFST(_BLK_T, _REG) + (i * 0x4)), val);\
		} \
	} while (0)

enum ISP_RGB_PROB_OUT {
	ISP_RGB_PROB_OUT_CFA = 0,
	ISP_RGB_PROB_OUT_RGBEE,
	ISP_RGB_PROB_OUT_CCM,
	ISP_RGB_PROB_OUT_GMA,
	ISP_RGB_PROB_OUT_DHZ,
	ISP_RGB_PROB_OUT_HSV,
	ISP_RGB_PROB_OUT_RGBDITHER,
	ISP_RGB_PROB_OUT_CSC,
	ISP_RGB_PROB_OUT_MAX,
};

enum ISP_RAW_PATH {
	ISP_RAW_PATH_LE = 0,
	ISP_RAW_PATH_SE,
	ISP_RAW_PATH_MAX,
};

/*
 * To indicate the 1st two pixel in the bayer_raw.
 */
enum ISP_BAYER_TYPE {
	ISP_BAYER_TYPE_BG = 0,
	ISP_BAYER_TYPE_GB,
	ISP_BAYER_TYPE_GR,
	ISP_BAYER_TYPE_RG,
	//for RGBIR
	ISP_BAYER_TYPE_GRGBI = 8,
	ISP_BAYER_TYPE_RGBGI = 9,
	ISP_BAYER_TYPE_GBGRI = 10,
	ISP_BAYER_TYPE_BGRGI = 11,
	ISP_BAYER_TYPE_IGRGB = 12,
	ISP_BAYER_TYPE_IRGBG = 13,
	ISP_BAYER_TYPE_IBGRG = 14,
	ISP_BAYER_TYPE_IGBGR = 15,
//	ISP_BAYER_TYPE_MAX,
};

enum ISP_BNR_OUT {
	ISP_BNR_OUT_BYPASS = 0,
	ISP_BNR_OUT_B_DELAY,
	ISP_BNR_OUT_FACTOR,
	ISP_BNR_OUT_B_NL,
	ISP_BNR_OUT_RESV_0,
	ISP_BNR_OUT_RESV_1,
	ISP_BNR_OUT_RESV_2,
	ISP_BNR_OUT_RESV_3,
	ISP_BNR_OUT_B_OUT,
	ISP_BNR_OUT_INTENSITY,
	ISP_BNR_OUT_DELTA,
	ISP_BNR_OUT_NOT_SM,
	ISP_BNR_OUT_FLAG_V,
	ISP_BNR_OUT_FLAG_H,
	ISP_BNR_OUT_FLAG_D45,
	ISP_BNR_OUT_FLAG_D135,
	ISP_BNR_OUT_MAX,
};

enum ISP_YNR_OUT {
	ISP_YNR_OUT_BYPASS = 0,
	ISP_YNR_OUT_Y_DELAY,
	ISP_YNR_OUT_FACTOR,
	ISP_YNR_OUT_ALPHA,
	ISP_YNR_OUT_Y_BF,
	ISP_YNR_OUT_Y_NL,
	ISP_YNR_OUT_RESV_0,
	ISP_YNR_OUT_RESV_1,
	ISP_YNR_OUT_Y_OUT,
	ISP_YNR_OUT_INTENSITY,
	ISP_YNR_OUT_DELTA,
	ISP_YNR_OUT_NOT_SM,
	ISP_YNR_OUT_FLAG_V,
	ISP_YNR_OUT_FLAG_H,
	ISP_YNR_OUT_FLAG_D45,
	ISP_YNR_OUT_FLAG_D135,
	ISP_YNR_OUT_MAX,
};

enum ISP_FS_OUT {
	ISP_FS_OUT_FS = 0,
	ISP_FS_OUT_LONG,
	ISP_FS_OUT_SHORT,
	ISP_FS_OUT_SHORT_EX,
	ISP_FS_OUT_MOTION_PXL,
	ISP_FS_OUT_LE_BLD_WHT,
	ISP_FS_OUT_SE_BLD_WHT,
	ISP_FS_OUT_MOTION_LUT,
	ISP_FS_OUT_AC_FS,
	ISP_FS_OUT_DELTA_LE,
	ISP_FS_OUT_DELTA_SE,
	ISP_FS_OUT_MAX,
};

enum ISP_AWB_MODE {
	ISP_AWB_LE,
	ISP_AWB_SE,
	ISP_AWB_SWITCH,
	ISP_AWB_MAX,
};

struct isp_param {
	uint32_t img_width;
	uint32_t img_height;
	uint32_t img_plane;
	uint32_t img_format;
	uint32_t img_stride;
};

enum ISPCQ_ID_T {
	ISPCQ_ID_PRERAW0 = 0,
	ISPCQ_ID_PRERAW1,
	ISPCQ_ID_POSTRAW,
	ISPCQ_ID_MAX
};

enum ISPCQ_OP_MODE {
	ISPCQ_OP_SINGLE_CMDSET,
	ISPCQ_OP_ADMA,
};

struct ispcq_config {
	uint32_t op_mode;
	uint32_t intr_en;

	enum ISPCQ_ID_T cq_id;
	union {
		uint64_t adma_table_pa;
		uint64_t cmdset_pa;
	};
	uint32_t cmdset_size;
};

enum isp_dump_grp {
	ISP_DUMP_PRERAW = 0x1,
	ISP_DUMP_POSTRAW = 0x2,
	ISP_DUMP_ALL = 0x4,
	ISP_DUMP_DMA = 0x8,
	ISP_DUMP_ALL_DMA = 0x10,
};

enum ISP_BLC_ID {
	ISP_BLC_ID_FE0_LE = 0,
	ISP_BLC_ID_FE0_SE,
	ISP_BLC_ID_BE_LE,
	ISP_BLC_ID_BE_SE,
	ISP_BLC_ID_FE1_LE,
	ISP_BLC_ID_FE1_SE,
	ISP_BLC_ID_MAX,
};

enum ISP_CCM_ID {
	ISP_CCM_ID_0 = 0,
	ISP_CCM_ID_1,
	ISP_CCM_ID_2,
	ISP_CCM_ID_3,
	ISP_CCM_ID_4,
	ISP_CCM_ID_MAX,
};

enum ISP_WBG_ID {
	ISP_WBG_ID_PRE0_FE_RGBMAP_LE = 0,
	ISP_WBG_ID_PRE0_FE_RGBMAP_SE,
	ISP_WBG_ID_PRE0_FE_LMAP_LE,
	ISP_WBG_ID_PRE0_FE_LMAP_SE,
	ISP_WBG_ID_PRE1_FE_RGBMAP_LE,
	ISP_WBG_ID_PRE1_FE_RGBMAP_SE,
	ISP_WBG_ID_PRE1_FE_LMAP_LE,
	ISP_WBG_ID_PRE1_FE_LMAP_SE,
	ISP_WBG_ID_PRE_BE_LE,
	ISP_WBG_ID_PRE_BE_SE,
	ISP_WBG_ID_PRE_BE_INV_LE,
	ISP_WBG_ID_PRE_BE_INV_SE,
	ISP_WBG_ID_MAX,
};

enum ISP_LSCR_ID {
	ISP_LSCR_ID_PRE0_FE_LE = 0,
	ISP_LSCR_ID_PRE0_FE_SE,
	ISP_LSCR_ID_PRE1_FE_LE,
	ISP_LSCR_ID_PRE1_FE_SE,
	ISP_LSCR_ID_PRE_BE_LE,
	ISP_LSCR_ID_PRE_BE_SE,
	ISP_LSCR_ID_MAX
};

enum ISP_PRE_PROC_ID {
	ISP_IR_PRE_PROC_ID_LE,
	ISP_IR_PRE_PROC_ID_SE,
	ISP_IR_PRE_PROC_ID_MAX
};

struct isp_dump_info {
	uint64_t phy_base;
	uint64_t reg_base;
	uint32_t blk_size;
};

struct isp_vblock_info {
	uint32_t block_id;
	uint32_t block_size;
	uint64_t reg_base;
};

struct isp_dma_cfg {
	uint16_t width;
	uint16_t height;
	uint16_t stride;
	uint16_t format;
};

struct tile {
	u16 start;
	u16 end;
};

struct isp_dhz_cfg {
	uint8_t  strength;
	uint16_t th_smooth;
	uint16_t cum_th;
	uint16_t hist_th;
	uint16_t tmap_min;
	uint16_t tmap_max;
	uint16_t sw_dc_th;
	uint16_t sw_aglobal;
	bool sw_dc_trig;
};

struct isp_ccm_cfg {
	u16 coef[3][3];
};

struct _fe_dbg_i {
	uint32_t		fe_idle_sts;
	uint32_t		fe_done_sts;
};

struct _be_dbg_i {
	uint32_t		be_done_sts;
};

struct _post_dbg_i {
	uint32_t		top_sts;
};

struct _dma_dbg_i {
	uint32_t		wdma_err_sts;
	uint32_t		wdma_idle;
	uint32_t		rdma_err_sts;
	uint32_t		rdma_idle;
};

struct _isp_dg_info {
	struct _fe_dbg_i	fe_sts;
	struct _be_dbg_i	be_sts;
	struct _post_dbg_i	post_sts;
	struct _dma_dbg_i	dma_sts;
	uint32_t		isp_top_sts;
	uint32_t		bdg_chn_debug[ISP_FE_CHN_MAX];
	uint32_t		bdg_int_sts_0;
	uint32_t		bdg_int_sts_1;
	uint32_t		bdg_fifo_of_cnt;
	uint8_t			bdg_w_gt_cnt[ISP_FE_CHN_MAX];
	uint8_t			bdg_w_ls_cnt[ISP_FE_CHN_MAX];
	uint8_t			bdg_h_gt_cnt[ISP_FE_CHN_MAX];
	uint8_t			bdg_h_ls_cnt[ISP_FE_CHN_MAX];
};

struct isp_grid_s_info {
	u8 w_bit;
	u8 h_bit;
};

struct _isp_cfg {
	uint32_t		csibdg_width;
	uint32_t		csibdg_height;
	uint32_t		max_width;
	uint32_t		max_height;
	uint32_t		post_img_w;
	uint32_t		post_img_h;
	struct vip_rect		crop;
	struct vip_rect		crop_se;
	struct vip_rect		postout_crop;
	struct _isp_dg_info	dg_info;
	struct isp_grid_s_info	rgbmap_i;
	struct isp_grid_s_info	lmap_i;
	enum ISP_BAYER_TYPE	rgb_color_mode;
	enum _VI_INTF_MODE_E	infMode;
	enum _VI_WORK_MODE_E	muxMode;

	uint32_t		is_patgen_en		: 1;
	uint32_t		is_offline_preraw	: 1;
	uint32_t		is_yuv_bypass_path	: 1;
	uint32_t		is_hdr_on		: 1;
	uint32_t		is_hdr_detail_en	: 1;
	uint32_t		is_tile			: 1;
	uint32_t		is_fbc_on		: 1;
	uint32_t		is_rgbir_sensor		: 1;
	uint32_t		is_offline_scaler	: 1;
	uint32_t		is_awb_sts_se		: 1;
	uint32_t		is_stagger_vsync	: 1;
};

/*
 * @src_width: width of original image
 * @src_height: height of original image
 * @img_width: width of image after crop
 * @img_height: height of image after crop
 * @pyhs_regs: index by enum ISP_BLK_ID_T, always phys reg
 * @vreg_bases: index by enum ISP_BLK_ID_T
 * @vreg_bases_pa: index by enum ISP_BLK_ID_T
 *
 * @rgb_color_mode: bayer_raw type after crop could change
 *
 * @cam_id: preraw(0,1)
 * @is_offline_preraw: preraw src offline(from dram)
 * @is_offline_postraw: postraw src offline(from dram)
 */
struct isp_ctx {
	struct isp_param	inparm;
	uint32_t		src_width;
	uint32_t		src_height;
	uint32_t		img_width;
	uint32_t		img_height;
	uint32_t		crop_x;
	uint32_t		crop_y;
	uint32_t		crop_se_x;
	uint32_t		crop_se_y;
	struct _tile_cfg {
		struct tile l_in;
		struct tile l_out;
		struct tile r_in;
		struct tile r_out;
	} tile_cfg;


	uintptr_t		*phys_regs;
	uintptr_t		*vreg_bases;
	uintptr_t		*vreg_bases_pa;
	uintptr_t		adma_table[ISPCQ_ID_MAX];
	uintptr_t		adma_table_pa[ISPCQ_ID_MAX];

	struct _isp_cfg		isp_pipe_cfg[ISP_PRERAW_MAX];
	enum ISP_BAYER_TYPE	rgb_color_mode[ISP_PRERAW_MAX];
	uint8_t			sensor_bitdepth;
	uint8_t			rgbmap_prebuf_idx;
	uint8_t			mmap_grid_size[ISP_PRERAW_MAX];
	uint8_t			rawb_chnstr_num;
	uint8_t			total_chn_num;

	uint8_t			cam_id;
	uint32_t		is_dual_sensor      : 1;
	uint32_t		is_yuv_sensor       : 1;
	uint32_t		is_rgbir_sensor     : 1;
	uint32_t		is_hdr_on           : 1;
	uint32_t		is_3dnr_on          : 1;
	uint32_t		is_dpcm_on          : 1;
	uint32_t		is_offline_be       : 1;
	uint32_t		is_offline_postraw  : 1;
	uint32_t		is_tile             : 1;
	uint32_t		is_work_on_r_tile   : 1;
	uint32_t		is_sublvds_path     : 1;
	uint32_t		is_fbc_on           : 1;
	uint32_t		is_ctrl_inited      : 1;

	uint32_t		vreg_page_idx;
};

union isp_intr {
	uint32_t raw;
	struct {
		uint32_t FRAME_DONE_PRE                  : 1;
		uint32_t FRAME_DONE_PRE1                 : 1;
		uint32_t FRAME_DONE_POST                 : 1;
		uint32_t SHAW_DONE_PRE                   : 1;
		uint32_t SHAW_DONE_PRE1                  : 1;
		uint32_t SHAW_DONE_POST                  : 1;
		uint32_t FRAME_ERR_PRE                   : 1;
		uint32_t FRAME_ERR_PRE1                  : 1;
		uint32_t FRAME_ERR_POST                  : 1;
		uint32_t CMDQ1_INT                       : 1;
		uint32_t CMDQ2_INT                       : 1;
		uint32_t CMDQ3_INT                       : 1;
		uint32_t FRAME_START_PRE                 : 1;
		uint32_t FRAME_START_PRE1                : 1;
		uint32_t _rsv_14                         : 2;
		uint32_t PCHK0_ERR_PRE                   : 1;
		uint32_t PCHK0_ERR_PRE1                  : 1;
		uint32_t PCHK0_ERR_RAW                   : 1;
		uint32_t PCHK0_ERR_RGB                   : 1;
		uint32_t PCHK0_ERR_YUV                   : 1;
		uint32_t PCHK1_ERR_PRE                   : 1;
		uint32_t PCHK1_ERR_PRE1                  : 1;
		uint32_t PCHK1_ERR_RAW                   : 1;
		uint32_t PCHK1_ERR_RGB                   : 1;
		uint32_t PCHK1_ERR_YUV                   : 1;
		uint32_t LINE_REACH_INT_PRE              : 1;
		uint32_t LINE_REACH_INT_PRE1             : 1;
	} bits;
};

union isp_csi_intr {
	uint32_t raw;
	struct {
		uint32_t FRAME_DROP_INT                  : 1;
		uint32_t FIFO_OVERFLOW_INT               : 1;
		uint32_t FRAME_WIDTH_GT_INT              : 1;
		uint32_t FRAME_WIDTH_LS_INT              : 1;
		uint32_t FRAME_HEIGHT_GT_INT             : 1;
		uint32_t FRAME_HEIGHT_LS_INT             : 1;
		uint32_t VSYNC_RISING_INT                : 1;
		uint32_t FRAME_START_INT                 : 1;
		uint32_t FRAME_WIDTH_OVER_MAX_INT        : 1;
		uint32_t FRAME_HEIGHT_OVER_MAX_INT       : 1;
		uint32_t LINE_INTP_INT                   : 1;
	} bits;
};

struct dci_param {
	uint16_t cliplimit_sel;
	uint16_t strecth_th_0;		// black stretching threshold
	uint16_t strecth_th_1;		// white stretching threshold
	uint16_t strecth_strength_0;	// black stretching strength
	uint16_t strecth_strength_1;	// white stretching strength
};

/**********************************************************
 *	SW scenario path check APIs
 **********************************************************/
u32 _is_fe_be_online(struct isp_ctx *ctx);
u32 _is_be_post_online(struct isp_ctx *ctx);
u32 _is_all_online(struct isp_ctx *ctx);
u32 _is_post_sclr_online(struct isp_ctx *ctx, enum cvi_isp_raw raw_num);
/**********************************************************/

void isp_debug_dump(struct isp_ctx *ctx);

void isp_set_base_addr(void *base);

/**
 * isp_init - setup isp
 *
 * @param :
 */
void isp_init(struct isp_ctx *ctx);


/**
 * isp_reset - do reset. This can be activated only if dma stop to avoid
 * hang fabric.
 *
 */
void isp_reset(struct isp_ctx *ctx);

/**
 * isp_stream_on - start/stop isp stream.
 *
 * @param on: 1 for stream start, 0 for stream stop
 */
void isp_streaming(struct isp_ctx *ctx, uint32_t on, enum cvi_isp_raw raw_num);


uintptr_t *isp_get_phys_reg_bases(void);

int isp_get_vblock_info(struct isp_vblock_info **pinfo, uint32_t *nblocks,
			enum ISPCQ_ID_T cq_group);

int ispcq_init_cmdset(char *cmdset_ba, int size, uint64_t reg_base);
int ispcq_set_end_cmdset(char *cmdset_ba, int size);
int ispcq_init_adma_table(char *adma_tb, int num_cmdset);
int ispcq_add_descriptor(char *adma_tb, int index, uint64_t cmdset_addr,
			 uint32_t cmdset_size);
uint64_t ispcq_get_desc_addr(char *adma_tb, int index);
int ispcq_set_link_desc(char *adma_tb, int index,
			uint64_t target_desc_addr, int is_link);
int ispcq_set_end_desc(char *adma_tb, int index, int is_end);
int ispcq_engine_config(uint64_t *phys_regs, struct ispcq_config *cfg);
int ispcq_engine_start(uint64_t *phys_regs, enum ISPCQ_ID_T id);


struct isp_grid_s_info ispblk_rgbmap_info(struct isp_ctx *ctx, enum cvi_isp_raw raw_num);
struct isp_grid_s_info ispblk_lmap_info(struct isp_ctx *ctx, enum cvi_isp_raw raw_num);
void ispblk_preraw_fe_config(struct isp_ctx *ctx, enum cvi_isp_raw raw_num);
void ispblk_preraw_be_config(struct isp_ctx *ctx);
void ispblk_rawtop_config(struct isp_ctx *ctx);
void ispblk_rgbtop_config(struct isp_ctx *ctx);
void ispblk_yuvtop_config(struct isp_ctx *ctx);
int ispblk_isptop_config(struct isp_ctx *ctx);

void ispblk_crop_enable(struct isp_ctx *ctx, int crop_id, bool en);
int ispblk_crop_config(struct isp_ctx *ctx, int crop_id, struct vip_rect crop);
void ispblk_blc_set_offset(struct isp_ctx *ctx, int blc_id,
				uint16_t roffset, uint16_t groffset,
				uint16_t gboffset, uint16_t boffset);
void ispblk_blc_set_2ndoffset(struct isp_ctx *ctx, int blc_id,
				uint16_t roffset, uint16_t groffset,
				uint16_t gboffset, uint16_t boffset);
void ispblk_blc_set_gain(struct isp_ctx *ctx, int blc_id,
				uint16_t rgain, uint16_t grgain,
				uint16_t gbgain, uint16_t bgain);
void ispblk_blc_enable(struct isp_ctx *ctx, int blc_id, bool en, bool bypass);
int ispblk_wbg_config(struct isp_ctx *ctx, int wbg_id, uint16_t rgain, uint16_t ggain, uint16_t bgain);
int ispblk_wbg_enable(struct isp_ctx *ctx, int wbg_id, bool enable, bool bypass);
void ispblk_wbg_inv_config(struct isp_ctx *ctx, int wbg_id, bool enable, bool bypass);
void ispblk_lscr_set_lut(struct isp_ctx *ctx, int lscr_id, uint16_t *gain_lut, uint8_t lut_count);
void ispblk_lscr_config(struct isp_ctx *ctx, int lscr_id, bool en);

void ispblk_dma_enable(struct isp_ctx *ctx, uint32_t dmaid, uint32_t on);
int ispblk_dma_buf_get_size(struct isp_ctx *ctx, int dmaid);

/****************************************************************************
 *	PRERAW FE SUBSYS
 ****************************************************************************/
void ispblk_csidbg_dma_wr_en(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num, const u8 chn_num, const u8 en);
void ispblk_csibdg_crop_update(struct isp_ctx *ctx, enum cvi_isp_raw raw_num, bool en);
int ispblk_csibdg_config(struct isp_ctx *ctx, enum cvi_isp_raw raw_num);
void ispblk_rgbmap_config(struct isp_ctx *ctx, int map_id, bool en, enum cvi_isp_raw raw_num);
void ispblk_lmap_chg_size(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num, const enum cvi_isp_pre_chn_num chn_num);
int ispblk_lmap_config(struct isp_ctx *ctx, int map_id, bool en, enum cvi_isp_raw raw_num);

/****************************************************************************
 *	PRE BE SUBSYS
 ****************************************************************************/
void ispblk_fpn_config(struct isp_ctx *ctx, bool enable);
void ispblk_dpc_config(struct isp_ctx *ctx, enum ISP_RAW_PATH path, union REG_ISP_DPC_2 reg2);
void ispblk_dpc_set_static(struct isp_ctx *ctx, enum ISP_RAW_PATH path,
			     uint16_t offset, uint32_t *bps, uint8_t count);
void ispblk_af_config(struct isp_ctx *ctx, bool enable);
void ispblk_aehist_reset(struct isp_ctx *ctx, int blk_id, enum cvi_isp_raw raw_num);
void ispblk_aehist_config(struct isp_ctx *ctx, int blk_id, bool enable);
void ispblk_awb_config(struct isp_ctx *ctx, int blk_id, bool en, enum ISP_AWB_MODE mode);
void ispblk_gms_config(struct isp_ctx *ctx, bool enable);
void ispblk_rgbir_preproc_config(struct isp_ctx *ctx,
	uint8_t *wdata, int16_t *data_r, int16_t *data_g, int16_t *data_b);
void ispblk_ir_proc_config(struct isp_ctx *ctx, uint8_t *gamma);

/****************************************************************************
 *	RAW TOP SUBSYS
 ****************************************************************************/
void ispblk_bnr_config(struct isp_ctx *ctx, enum ISP_BNR_OUT out_sel, bool lsc_en, uint8_t ns_gain, uint8_t str);
void ispblk_cfa_config(struct isp_ctx *ctx);
void ispblk_rgbcac_config(struct isp_ctx *ctx, bool en);

/****************************************************************************
 *	RGB TOP SUBSYS
 ****************************************************************************/
void ispblk_lsc_config(struct isp_ctx *ctx, bool en);
void ispblk_fusion_hdr_cfg(struct isp_ctx *ctx, enum cvi_isp_raw raw_num);
void ispblk_fusion_config(struct isp_ctx *ctx, bool enable, bool mc_enable, enum ISP_FS_OUT out_sel);
void ispblk_ltm_d_lut(struct isp_ctx *ctx, uint8_t sel, uint16_t *data);
void ispblk_ltm_b_lut(struct isp_ctx *ctx, uint8_t sel, uint16_t *data);
void ispblk_ltm_g_lut(struct isp_ctx *ctx, uint8_t sel, uint16_t *data);
void ispblk_ltm_enable(struct isp_ctx *ctx, bool en);
void ispblk_ltm_config(struct isp_ctx *ctx, bool dehn_en, bool dlce_en, bool behn_en, bool blce_en);
void ispblk_ccm_config(struct isp_ctx *ctx, enum ISP_BLK_ID_T blk_id, bool en, struct isp_ccm_cfg *cfg);
void ispblk_hist_edge_v_config(struct isp_ctx *ctx, bool en);
void ispblk_dhz_config(struct isp_ctx *ctx, bool en);
void ispblk_gamma_config(struct isp_ctx *ctx, uint8_t sel, uint16_t *data);
void ispblk_gamma_enable(struct isp_ctx *ctx, bool enable);
void ispblk_clut_config(struct isp_ctx *ctx, bool en,
				int16_t *r_lut, int16_t *g_lut, int16_t *b_lut);
void ispblk_rgbdither_config(struct isp_ctx *ctx, bool en, bool mod_en, bool histidx_en, bool fmnum_en);
void ispblk_cacp_config(struct isp_ctx *ctx, bool en);
void ispblk_csc_config(struct isp_ctx *ctx);
void ispblk_preyee_config(struct isp_ctx *ctx, bool en);
void ispblk_manr_config(struct isp_ctx *ctx, bool en);
void ispblk_ir_merge_config(struct isp_ctx *ctx);

/****************************************************************************
 *	YUV TOP SUBSYS
 ****************************************************************************/
void ispblk_444_422_config(struct isp_ctx *ctx);
int ispblk_yuvdither_config(struct isp_ctx *ctx, uint8_t sel, bool en,
			    bool mod_en, bool histidx_en, bool fmnum_en);
void ispblk_fbcd_enable(struct isp_ctx *ctx, bool en);
void ispblk_fbce_config(struct isp_ctx *ctx, bool en);
void ispblk_ynr_config(struct isp_ctx *ctx, enum ISP_YNR_OUT out_sel, uint8_t ns_gain, uint8_t str);
void ispblk_cnr_config(struct isp_ctx *ctx, bool en, bool pfc_en, uint8_t str_mode);
int ispblk_ee_config(struct isp_ctx *ctx, bool en);
int ispblk_ycur_config(struct isp_ctx *ctx, uint8_t sel, uint16_t *data);
int ispblk_ycur_enable(struct isp_ctx *ctx, bool enable, uint8_t sel);
void ispblk_ca_lite_config(struct isp_ctx *ctx, bool en);

void ispblk_dci_config(struct isp_ctx *ctx, bool en, uint16_t *lut);
void ispblk_dci_hist_gen(uint8_t *raw, uint16_t *hist);
void ispblk_dci_lut_gen(struct isp_ctx *ctx, uint16_t *hist,
			struct dci_param *param);

void ispblk_csibdg_update_size(struct isp_ctx *ctx, enum cvi_isp_raw raw_num);
void ispblk_post_in_dma_update(struct isp_ctx *ctx, int dmaid, enum cvi_isp_raw raw_num);
void ispblk_dma_crop_update(struct isp_ctx *ctx, int dmaid, struct vip_rect crop);
int ispblk_dma_config(struct isp_ctx *ctx, int dmaid, uint64_t buf_addr);
void ispblk_dma_setaddr(struct isp_ctx *ctx, uint32_t dmaid, uint64_t buf_addr);
uint64_t ispblk_dma_getaddr(struct isp_ctx *ctx, uint32_t dmaid);
int ispblk_dma_dbg_st(struct isp_ctx *ctx, uint32_t dmaid, uint32_t bus_sel);

void isp_pchk_config(struct isp_ctx *ctx, uint8_t en_mask);
void isp_pchk_chk_status(struct isp_ctx *ctx, uint8_t en_mask,
			 uint32_t intr_status);

void isp_pre_trig(struct isp_ctx *ctx, enum cvi_isp_raw raw_num, const u8 chn_num);
void isp_post_trig(struct isp_ctx *ctx, enum cvi_isp_raw raw_num);
enum ISP_BAYER_TYPE isp_bayer_remap(enum ISP_BAYER_TYPE bayer_id,
	uint16_t x_offset, uint16_t y_offset);

union isp_intr isp_intr_get_mask(struct isp_ctx *ctx);
void isp_intr_set_mask(struct isp_ctx *ctx, union isp_intr intr_status);
union REG_ISP_TOP_0 isp_intr_status(struct isp_ctx *ctx);
union REG_ISP_TOP_9 isp_int_event1_status(struct isp_ctx *ctx);
void isp_intr_clr(struct isp_ctx *ctx, union isp_intr intr_status);

union REG_ISP_CSI_BDG_INTERRUPT_STATUS_0 isp_csi_intr_status_0(struct isp_ctx *ctx, enum cvi_isp_raw raw_num);
union REG_ISP_CSI_BDG_INTERRUPT_STATUS_1 isp_csi_intr_status_1(struct isp_ctx *ctx, enum cvi_isp_raw raw_num);

void isp_register_dump(struct isp_ctx *ctx, struct seq_file *m, enum isp_dump_grp grp);
int ispblk_rgbmap_get_w_bit(struct isp_ctx *ctx, int dmaid);
void ispblk_tnr_rgbmap_chg(struct isp_ctx *ctx, enum cvi_isp_raw raw_num, const u8 chn_num);
void ispblk_tnr_post_chg(struct isp_ctx *ctx, enum cvi_isp_raw raw_num);
void isp_first_frm_reset(struct isp_ctx *ctx, uint8_t reset);
uint32_t ispblk_preraw_dg_info(struct isp_ctx *ctx, enum cvi_isp_raw raw_num);
void ispblk_csibdg_thermal_config(struct isp_ctx *ctx, enum cvi_isp_raw raw_num);
void ispblk_isptop_thermal_config(struct isp_ctx *ctx);
void ispblk_dma_thermal_config(struct isp_ctx *ctx, int dmaid, uint64_t buf_addr);


/****************************************************************************
 *	Runtime Control Flow Config
 ****************************************************************************/
void ispblk_cfa_softrst(struct isp_ctx *ctx, u8 en);
void isp_first_frm_reset(struct isp_ctx *ctx, uint8_t reset);
void ispblk_post_yuv_cfg_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num);
void ispblk_post_cfg_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num);
void ispblk_pre_be_cfg_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num);
u8 isp_is_awb_se(struct isp_ctx *ctx);
u8 isp_is_fe02be_enable(struct isp_ctx *ctx);
void isp_be_awb_source_chg(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num, u32 frm_num);
uint32_t ispblk_csibdg_chn_dbg(struct isp_ctx *ctx, enum cvi_isp_raw raw_num, enum cvi_isp_pre_chn_num chn);
struct _fe_dbg_i ispblk_fe_dbg_info(struct isp_ctx *ctx, enum cvi_isp_raw raw_num);
struct _be_dbg_i ispblk_be_dbg_info(struct isp_ctx *ctx);
struct _post_dbg_i ispblk_post_dbg_info(struct isp_ctx *ctx);
struct _dma_dbg_i ispblk_dma_dbg_info(struct isp_ctx *ctx);
int ispblk_dma_get_size(struct isp_ctx *ctx, int dmaid, uint32_t _w, uint32_t _h);
int isp_frm_err_handler(struct isp_ctx *ctx, const enum cvi_isp_raw err_raw_num, const u8 step);

/****************************************************************************
 *	YUV Bypass Control Flow Config
 ****************************************************************************/
void ispblk_csibdg_yuv_bypass_config(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num);
u32 ispblk_dma_yuv_bypass_config(struct isp_ctx *ctx, uint32_t dmaid, uint64_t buf_addr,
					const enum cvi_isp_raw raw_num);

/****************************************************************************
 *	Tile Control Flow Config
 ****************************************************************************/
void ispblk_rawtop_tile(struct isp_ctx *ctx);
void ispblk_bnr_tile(struct isp_ctx *ctx);
void ispblk_rgbtop_tile(struct isp_ctx *ctx);
void ispblk_lsc_tile(struct isp_ctx *ctx);
void ispblk_hist_edge_v_tile(struct isp_ctx *ctx);
void ispblk_ltm_tile(struct isp_ctx *ctx, enum cvi_isp_raw raw_num);
void ispblk_manr_tile(struct isp_ctx *ctx, enum cvi_isp_raw raw_num);
void ispblk_dhz_tile(struct isp_ctx *ctx);
void ispblk_dci_tile(struct isp_ctx *ctx);
void ispblk_fbce_tile(struct isp_ctx *ctx);
void ispblk_yuvtop_tile(struct isp_ctx *ctx);

/****************************************************************************
 *	Pre Be Tuning Config
 ****************************************************************************/
void ispblk_blc_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_blc_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_dpc_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_dpc_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_lscr_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_lscr_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_ae_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ae_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_wbg_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_wbg_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_af_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_af_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_awb_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_awb_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_gms_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_gms_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_ge_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ge_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_preproc_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_preproc_config *cfg,
	const enum cvi_isp_raw raw_num);

/****************************************************************************
 *	Postraw Tuning Config
 ****************************************************************************/
void ispblk_bnr_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_bnr_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_demosiac_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_demosiac_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_rgbcac_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_rgbcac_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_ccm_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ccm_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_lsc_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_lsc_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_fswdr_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_fswdr_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_fswdr_update_rpt(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_fswdr_report *cfg);
void ispblk_drc_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_drc_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_gamma_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_gamma_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_dhz_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_dhz_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_clut_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_clut_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_csc_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_csc_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_dci_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_dci_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_cacp_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_cacp_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_preyee_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_preyee_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_tnr_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_tnr_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_ynr_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ynr_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_cnr_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_cnr_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_ee_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ee_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_ca2_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ca2_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_cac_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_cac_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_hist_edge_v_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_hist_edge_v_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_ycur_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_ycur_config *cfg,
	const enum cvi_isp_raw raw_num);
void ispblk_mono_tun_cfg(
	struct isp_ctx *ctx,
	struct cvi_vip_isp_mono_config *cfg,
	const enum cvi_isp_raw raw_num);

#ifdef __cplusplus
}
#endif

#endif /* _ISP_DRV_H_ */
