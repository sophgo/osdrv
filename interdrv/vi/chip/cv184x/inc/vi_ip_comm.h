#ifndef _VI_IP_COMM_H_
#define _VI_IP_COMM_H_

#include "isp/vi_tun_cfg.h"
#include "isp/vi_isp.h"
#include "comm_vi.h"
#include "vi_common.h"
#include "osal.h"

#define OFFLINE_RAW_BUF_NUM		2
#define OFFLINE_YUV_BUF_NUM		2
#define BUF_NUM				2
#define MSP_CHN_NUM			2
#define MSP_BUF_MAX			3

#ifndef _OFST
#define _OFST(_BLK_T, _REG)       ((uintptr_t)&(((struct _BLK_T *)0)->_REG))
#endif

enum TNR_IDX {
	TNR_ST_Y,
	TNR_ST_C,
	TNR_ST_MV,
	TNR_ST_MO,
	TNR_ST_FCB,
	TNR_ST_MAX,
};

enum BUF_IDX {
	BUF_IDX0,
	BUF_IDX1,
	BUF_MAX,
};

enum sop_isp_raw {
	ISP_PRERAW0,
	ISP_PRERAW1,
	ISP_PRERAW2,
	ISP_PRERAW_LITE0,
	ISP_PRERAW_VIRT0,
	ISP_PRERAW_VIRT1,
	ISP_PRERAW_MAX,
};

enum sop_isp_fe_chn_num {
	ISP_FE_CH0,
	ISP_FE_CH1,
	ISP_FE_CH2,
	ISP_FE_CH3,
	ISP_FE_CHN_MAX,
};
struct _fe_dbg_i {
	u32		fe_idle_sts;
	u32		fe_done_sts;
};

struct _post_dbg_i {
	u32		top_sts_0;
	u32		top_sts_1;
};

struct _dma_dbg_i {
	u32		wdma_1_err_sts;
	u32		wdma_1_idle;
	u32		wdma_2_err_sts;
	u32		wdma_2_idle;
	u32		wdma_3_err_sts;
	u32		wdma_3_idle;
	u32		wdma_4_err_sts;
	u32		wdma_4_idle;
	u32		rdma_1_err_sts;
	u32		rdma_1_idle;
	u32		rdma_2_err_sts;
	u32		rdma_2_idle;
	u32		rdma_3_err_sts;
	u32		rdma_3_idle;
};

struct _csi_bdg_chn_dbg_i {
	u32		dbg_0;
	u32		dbg_1;
	u32		dbg_2;
	u32		dbg_3;
};

struct _csibdg_dbg_i {
	u32				bdg_int_sts_0;
	u32				bdg_int_sts_1;
	u32				bdg_fifo_of_cnt;

	struct _csi_bdg_chn_dbg_i	chn_dbg[ISP_FE_CHN_MAX];

	u8				bdg_w_gt_cnt[ISP_FE_CHN_MAX];
	u8				bdg_w_ls_cnt[ISP_FE_CHN_MAX];
	u8				bdg_h_gt_cnt[ISP_FE_CHN_MAX];
	u8				bdg_h_ls_cnt[ISP_FE_CHN_MAX];
};

struct _isp_dg_info {
	struct _fe_dbg_i	fe_sts[ISP_PRERAW_MAX];
	struct _post_dbg_i	post_sts;
	struct _dma_dbg_i	dma_sts;
	struct _csibdg_dbg_i	bdg_dbg[ISP_PRERAW_MAX];
};

struct vi_rect {
	u16 x;
	u16 y;
	u16 w;
	u16 h;
};

struct isp_ccm_cfg {
	u16 coef[3][3];
};

struct vi_fbc_cfg {
	u8	cu_size;
	u8	target_cr; //compression ratio
	u8	is_lossless; // lossless or lossy
	u32	y_bs_size; //Y WDMA seglen
	u32	c_bs_size; //C WDMA seglen
	u32	y_buf_size; //total Y buf size
	u32	c_buf_size; //total C buf size
};

struct slc_cfg_s {
	u32 le_buf_size;
	u32 se_buf_size;
	u32 le_w_thshd;
	u32 se_w_thshd;
	u32 le_r_thshd;
	u32 se_r_thshd;
};

struct slice_buf_s {
	u16 line_delay; //sensor exposure ratio
	u16 buffer; //cover for read/write latency, axi latency..etc
	u8  main_max_grid_size; //main path rgbmap grid size
	u8  sub_max_grid_size; //sub path rgbmap grid size
	u8  min_r_thshd; // minimum read threshold
	struct slc_cfg_s main_path;
	struct slc_cfg_s sub_path;
};
/*
 * To indicate the 1st two pixel in the bayer_raw.
 */
enum isp_bayer_type_e {
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

enum isp_yuv_scene_e {
	ISP_YUV_SCENE_BYPASS = 0,
	ISP_YUV_SCENE_ONLINE,
	ISP_YUV_SCENE_ISP,
	ISP_YUV_SCENE_MAX,
};

struct isp_cmdq_buf {
	u64 phy_addr;
	void *vir_addr;
	u32 buf_size;
	u16 cmd_idx;
};

enum isp_bnr_out_e {
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

enum isp_fs_out_e {
	ISP_FS_OUT_FS = 0,
	ISP_FS_OUT_SHORT,
	ISP_FS_OUT_LONG,
	ISP_FS_OUT_0,
	ISP_FS_OUT_MAX,
};

enum isp_raw_path_e {
	ISP_RAW_PATH_LE = 0,
	ISP_RAW_PATH_SE,
	ISP_RAW_PATH_MAX,
};

enum isp_blk_id_t {
	ISP_BLK_ID_PRE_RAW_FE0,
	ISP_BLK_ID_CSIBDG0,
	ISP_BLK_ID_DMA_CTL_CSI0_BDG0,
	ISP_BLK_ID_DMA_CTL_CSI0_BDG1,
	ISP_BLK_ID_DMA_CTL_CSI0_BDG2,
	ISP_BLK_ID_DMA_CTL_CSI0_BDG3,
	ISP_BLK_ID_PRE_RAW_FE0_LSC0,
	ISP_BLK_ID_PRE_RAW_FE0_LSC1,
	ISP_BLK_ID_DMA_CTL_FE0_CLSC_LE,
	ISP_BLK_ID_AE_HIST_FE0,
	ISP_BLK_ID_DMA_CTL_FE0_AE_HIST_LE,
	ISP_BLK_ID_DMA_CTL_FE0_AE_HIST_SE,

	ISP_BLK_ID_PRE_RAW_FE1,
	ISP_BLK_ID_CSIBDG1,
	ISP_BLK_ID_DMA_CTL_CSI1_BDG0,
	ISP_BLK_ID_DMA_CTL_CSI1_BDG1,
	ISP_BLK_ID_PRE_RAW_FE1_LSC0,
	ISP_BLK_ID_PRE_RAW_FE1_LSC1,
	ISP_BLK_ID_DMA_CTL_FE1_CLSC_LE,
	ISP_BLK_ID_AE_HIST_FE1,
	ISP_BLK_ID_DMA_CTL_FE1_AE_HIST_LE,
	ISP_BLK_ID_DMA_CTL_FE1_AE_HIST_SE,

	ISP_BLK_ID_PRE_RAW_FE2,
	ISP_BLK_ID_CSIBDG2,
	ISP_BLK_ID_DMA_CTL_CSI2_BDG0,
	ISP_BLK_ID_PRE_RAW_FE2_LSC0,
	ISP_BLK_ID_DMA_CTL_FE2_CLSC_LE,
	ISP_BLK_ID_AE_HIST_FE2,
	ISP_BLK_ID_DMA_CTL_FE2_AE_HIST_LE,

	ISP_BLK_ID_WDMA_CORE1,
	ISP_BLK_ID_WDMA_CORE2,
	ISP_BLK_ID_WDMA_CORE3,
	ISP_BLK_ID_WDMA_CORE4,

	ISP_BLK_ID_RAWTOP0,
	ISP_BLK_ID_RAWTOP1,
	ISP_BLK_ID_BNR,
	ISP_BLK_ID_FUSION,
	ISP_BLK_ID_MAPCURVE,
	ISP_BLK_ID_BLC_DG_WB0,
	ISP_BLK_ID_BLC_DG_WB1,
	ISP_BLK_ID_DPC,
	ISP_BLK_ID_AF,
	ISP_BLK_ID_DMA_CTL_AF_W,

	ISP_BLK_ID_LSCR,
	ISP_BLK_ID_DMA_CTL_LSCR_HIST,
	ISP_BLK_ID_DRC,
	ISP_BLK_ID_DMA_CTL_DRC_POLY_R,
	ISP_BLK_ID_DMA_CTL_DRC_POLY_W,
	ISP_BLK_ID_DMA_CTL_DRC_HIST,
	ISP_BLK_ID_CFA,

	ISP_BLK_ID_RGBTOP,
	ISP_BLK_ID_PRE_EE_EXT,
	ISP_BLK_ID_PFR,
	ISP_BLK_ID_CCM,
	ISP_BLK_ID_RGBGAMMA,
	ISP_BLK_ID_RGB_DITHER,
	ISP_BLK_ID_CLUT,
	ISP_BLK_ID_DMA_CTL_CLUT_R,
	ISP_BLK_ID_CSC,

	ISP_BLK_ID_YUVTOP,
	ISP_BLK_ID_YUV_DITHER,
	ISP_BLK_ID_PRE_EE_FRONT,
	ISP_BLK_ID_PRE_EE_BACK,
	ISP_BLK_ID_LDCI,
	ISP_BLK_ID_DMA_CTL_LDCI_W,
	ISP_BLK_ID_DMA_CTL_LDCI_R,
	ISP_BLK_ID_DMA_CTL_LDCI_HIST,
	ISP_BLK_ID_LDCI_MAP_CORE,
	ISP_BLK_ID_TNR,
	ISP_BLK_ID_DMA_CTL_TNR_LD_Y,
	ISP_BLK_ID_DMA_CTL_TNR_LD_C,
	ISP_BLK_ID_DMA_CTL_TNR_LD_MO,
	ISP_BLK_ID_DMA_CTL_TNR_LD_MV,
	ISP_BLK_ID_DMA_CTL_TNR_LD_FCB,
	ISP_BLK_ID_DMA_CTL_TNR_ST_Y,
	ISP_BLK_ID_DMA_CTL_TNR_ST_C,
	ISP_BLK_ID_DMA_CTL_TNR_ST_MO,
	ISP_BLK_ID_DMA_CTL_TNR_ST_MV,
	ISP_BLK_ID_DMA_CTL_TNR_ST_FCB,
	ISP_BLK_ID_DMA_CTL_TNR_ST_MSP,
	ISP_BLK_ID_FBCE,
	ISP_BLK_ID_FBCD,
	ISP_BLK_ID_CNR,
	ISP_BLK_ID_DMA_CTL_CNR_Y_W,
	ISP_BLK_ID_DMA_CTL_CNR_C_W,
	ISP_BLK_ID_DMA_CTL_CNR_Y_R,
	ISP_BLK_ID_DMA_CTL_CNR_C_R,
	ISP_BLK_ID_CA,
	ISP_BLK_ID_CA_LITE,
	ISP_BLK_ID_POST_EE,
	ISP_BLK_ID_YCURVE,
	ISP_BLK_ID_YUV_CROP_Y,
	ISP_BLK_ID_DMA_CTL_YUV_CROP_Y,
	ISP_BLK_ID_YUV_CROP_C,
	ISP_BLK_ID_DMA_CTL_YUV_CROP_C,
	ISP_BLK_ID_DMA_CTL_YUV_RDMA_Y,
	ISP_BLK_ID_DMA_CTL_YUV_RDMA_C,
	ISP_BLK_ID_RESIZE,
	ISP_BLK_ID_DMA_CTL_RESIZE,

	ISP_BLK_ID_ISPTOP,
	ISP_BLK_ID_RDMA_CORE1,
	ISP_BLK_ID_RDMA_CORE2,
	ISP_BLK_ID_RDMA_CORE3,
	ISP_BLK_ID_CSIBDG0_LITE,
	ISP_BLK_ID_DMA_CTL_BT0_LITE0,
	ISP_BLK_ID_DMA_CTL_BT0_LITE1,
	ISP_BLK_ID_DMA_CTL_BT0_LITE2,
	ISP_BLK_ID_DMA_CTL_BT0_LITE3,
	ISP_BLK_ID_PRE_RAW_VI_SEL,
	ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_LE,
	ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_SE,
	ISP_BLK_ID_PRE_RAW_VI_SEL_CROP_LE,
	ISP_BLK_ID_PRE_RAW_VI_SEL_CROP_SE,
	ISP_BLK_ID_CMDQ,

	ISP_BLK_ID_MAX
};

struct csi_gpio {
	bool enable;
	void *handle; /* gpio handle just for alios, linux no use*/
	s32 port;
	s32 pin;
	s32 pol;
};

struct csi_snr_array {
	enum sop_isp_raw cur_raw;
	struct csi_gpio gpio[VI_MAX_DEV_SWITCH_DEPTH];
};

struct _csi_switch_info {
	u8 pre_idx;
	u8 cur_idx;
	u8 cur_nums;

	struct csi_snr_array snr_arr[VI_MAX_DEV_SWITCH_NUM];
};

struct _csi_cfg {
	u32			csibdg_width;
	u32			csibdg_height;
	u32			max_width;
	u32			max_height;

	u32			drop_ref_frm_num;
	u32			drop_frm_cnt;

	struct vi_rect		crop[ISP_FE_CHN_MAX];
	struct vi_rect		rawdump_crop[ISP_FE_CHN_MAX];

	enum sop_isp_raw	phy_raw;
	struct _csi_switch_info	switch_info;
	enum isp_bayer_type_e	rgb_color_mode_pre_crop;
	enum isp_bayer_type_e	rgb_color_mode;
	enum isp_yuv_scene_e	yuv_scene_mode;
	enum _vi_intf_mode_e	inf_mode;
	enum _vi_work_mode_e	mux_mode;
	enum _vi_yuv_data_seq_e data_seq;

	osal_atomic		clsc_en[ISP_FE_CHN_MAX];

	u8			patgen_fps;
	u8			chn_num;
	u8			bind_dev;
	u8			bind_pipe[ISP_FE_CHN_MAX];

	u32			is_mux_dev		: 1;
	u32			is_patgen_en		: 1;
	u32			is_yuv_sensor		: 1;
	u32			is_422_to_420		: 1;
	u32			is_bt_demux		: 1;
	u32			is_stagger_vsync	: 1;
	u32			is_hdr_on		: 1;
	u32			is_ctrl_inited		: 1;
	u32			is_enable		: 1;
	u32			is_drop_next_frame	: 1;
	u32			is_bind			: 1;
};

struct _isp_cfg {
	enum isp_bayer_type_e	rgb_color_mode;
	enum isp_yuv_scene_e	yuv_scene_mode;

	/*isp info*/
	u8			bind_raw;
	/*bit[chn]*/
	u8			chn_enable;
	u32			post_img_w;
	u32			post_img_h;
	u32			cnr_pre_scale_shift;
	u32			cnr_cur_scale_shift;
	u32			cnr_scale_shift;

	u32			isp_reset_frm;
	u32			first_frm_cnt;

	u32			resize_w;
	u32			resize_h;
	osal_atomic		resize_en;
	osal_atomic		ai_isp_en;

	struct vi_rect		crop;
	struct vi_rect		crop_se;

	struct vi_rect		postout_crop;

	struct isp_cmdq_buf	cmdq_buf;
	osal_timeval		tv;

	bool			first_frm_rst;

	/*csi info*/
	u32			is_yuv_sensor		: 1;
	u32			is_hdr_on		: 1;
	u32			is_bind			: 1;
	u32			is_uv_swap		: 1;

	u32			is_fbc_on		: 1;
	u32			is_3dnr_on		: 1;
	u32			is_offline_scaler	: 1;
	u32			is_postout_crop		: 1;
	u32			is_enable		: 1;
};

/* struct mempool
 * @base: the address of the mempool
 * @size: the size of the mempool
 * @byteused: the number of bytes used
 */
struct _mempool {
	u64 base;
	u32 size;
	u32 byteused;
};

struct _membuf {
	u64 pre_fe[ISP_FE_CHN_MAX][OFFLINE_RAW_BUF_NUM];
	u64 pre_ai_isp[ISP_FE_CHN_MAX][OFFLINE_RAW_BUF_NUM];
	u64 yuv_yuyv[ISP_FE_CHN_MAX][OFFLINE_YUV_BUF_NUM];//yuv sensor is yuyv format
	u64 clsc;
	u64 clut;
	u64 tdnr[TNR_ST_MAX][BUF_MAX];
	u64 ldci_lmap[BUF_MAX];
	u64 drc_poly[BUF_MAX];
	u64 cnr_y;
	u64 cnr_c;
	u64 msp[MSP_CHN_NUM][MSP_BUF_MAX];

	struct sop_isp_sts_mem sts_mem[BUF_MAX];
	u8 pre_fe_sts_busy_idx;
	u8 post_sts_busy_idx;

	osal_spinlock pre_fe_sts_lock;
	u8 pre_fe_sts_in_use;
	osal_spinlock post_sts_lock;
	u8 post_sts_in_use;
};

struct _isp_cfg_info {
	enum sop_isp_raw	raw_num;
	enum sop_isp_fe_chn_num	chn_num;

	u8			pipe;
	u32			img_width;
	u32			img_height;

	u32			is_hdr_on	: 1;
	u32			is_yuv		: 1;

	u64			pts;
};

struct isp_ctx {
	uintptr_t		*phys_regs;

	struct _csi_cfg		isp_csi_cfg[ISP_PRERAW_MAX];
	struct _isp_cfg		isp_pipe_cfg[VI_MAX_PIPE_NUM];

	struct _mempool		csi_mempool[ISP_PRERAW_MAX];
	struct _mempool		isp_mempool[VI_MAX_PIPE_NUM];

	struct _membuf		csi_bufpool[ISP_PRERAW_MAX];
	struct _membuf		isp_bufpool[VI_MAX_PIPE_NUM];

	u8			virt_raw_offset;
	u8			bind_raw[VI_MAX_DEV_NUM];
	u8			csi_patgen_en[VI_MAX_DEV_NUM];
	u8			raw_chnstr_num[ISP_PRERAW_MAX];
	osal_atomic		is_post_done;

	struct _isp_dg_info	dg_info;
	struct _isp_cfg_info	cfg_info;

	u32			is_multi_sensor     : 1;
	u32			is_hdr_on           : 1;
	u32			is_3dnr_on          : 1;
	u32			is_dpcm_on          : 1;
	u32			is_offline_postraw  : 1;
	u32			is_sublvds_path     : 1;
	u32			is_fbc_on           : 1;
	u32			is_ctrl_inited      : 1;
	u32			is_slice_buf_on     : 1;
	u32			is_rawreplay        : 1;
	u32			is_ai_isp           : 1;
};

/**********************************************************
 *	SW scenario path check APIs
 **********************************************************/
u32 _is_fe_post_offline(struct isp_ctx *ctx);
u32 _is_fe_post_slice(struct isp_ctx *ctx);
u32 _is_all_online(struct isp_ctx *ctx);

#endif
