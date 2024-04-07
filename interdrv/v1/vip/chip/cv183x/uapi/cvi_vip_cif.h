#ifndef _U_CVI_VIP_CIF_H_
#define _U_CVI_VIP_CIF_H_

#define MIPI_LANE_NUM	4
#define WDR_VC_NUM	2
#define SYNC_CODE_NUM	4

struct img_size_s {
	unsigned int	width;
	unsigned int	height;
};

enum rx_mac_clk_e {
	RX_MAC_CLK_200M = 0,
	RX_MAC_CLK_400M,
	RX_MAC_CLK_500M,
	RX_MAC_CLK_600M,
	RX_MAC_CLK_BUTT,
};

enum cam_pll_freq_e {
	CAMPLL_FREQ_NONE = 0,
	CAMPLL_FREQ_37P125M,
	CAMPLL_FREQ_25M,
	CAMPLL_FREQ_27M,
	CAMPLL_FREQ_24M,
	CAMPLL_FREQ_26M,
	CAMPLL_FREQ_NUM
};

struct mclk_pll_s {
	unsigned int		cam;
	enum cam_pll_freq_e	freq;
};

struct dphy_s {
	unsigned char		enable;
	unsigned char		hs_settle;
};

enum lane_divide_mode_e {
	LANE_DIVIDE_MODE_0 = 0,
	LANE_DIVIDE_MODE_1,
	LANE_DIVIDE_MODE_2,
	LANE_DIVIDE_MODE_3,
	LANE_DIVIDE_MODE_4,
	LANE_DIVIDE_MODE_5,
	LANE_DIVIDE_MODE_6,
	LANE_DIVIDE_MODE_7,
	LANE_DIVIDE_MODE_BUTT
};

enum input_mode_e {
	INPUT_MODE_MIPI = 0,
	INPUT_MODE_SUBLVDS,
	INPUT_MODE_HISPI,
	INPUT_MODE_CMOS,
	INPUT_MODE_BT1120,
	INPUT_MODE_BT601_19B_VHS,
	INPUT_MODE_BT656_9B,
	INPUT_MODE_CUSTOM_0,
	INPUT_MODE_BUTT
};

enum raw_data_type_e {
	RAW_DATA_8BIT = 0,
	RAW_DATA_10BIT,
	RAW_DATA_12BIT,
	YUV422_8BIT,	/* MIPI-CSI only */
	YUV422_10BIT,   /* MIPI-CSI only*/
	RAW_DATA_BUTT
};

enum mipi_wdr_mode_e {
	CVI_MIPI_WDR_MODE_NONE = 0,
	CVI_MIPI_WDR_MODE_VC,
	CVI_MIPI_WDR_MODE_DT,
	CVI_MIPI_WDR_MODE_DOL,
	CVI_MIPI_WDR_MODE_MANUAL,  /* SOI case */
	CVI_MIPI_WDR_MODE_BUTT
};

enum mipi_vs_gen_mode {
	CSI_VSYNC_BY_FS,
	CSI_VSYNC_BY_FE,
	CSI_VSYNC_BY_FS_FE
};

enum wdr_mode_e {
	CVI_WDR_MODE_NONE = 0,
	CVI_WDR_MODE_2F,
	CVI_WDR_MODE_3F,
	CVI_WDR_MODE_DOL_2F,
	CVI_WDR_MODE_DOL_3F,
	CVI_WDR_MODE_DOL_BUTT
};

enum lvds_sync_mode_e {
	LVDS_SYNC_MODE_SOF = 0,
	LVDS_SYNC_MODE_SAV,
	LVDS_SYNC_MODE_BUTT
};

enum lvds_bit_endian {
	LVDS_ENDIAN_LITTLE = 0,
	LVDS_ENDIAN_BIG,
	LVDS_ENDIAN_BUTT
};

enum lvds_vsync_type_e {
	LVDS_VSYNC_NORMAL = 0,
	LVDS_VSYNC_SHARE,
	LVDS_VSYNC_HCONNECT,
	LVDS_VSYNC_BUTT
};

enum lvds_fid_type_e {
	LVDS_FID_NONE = 0,
	LVDS_FID_IN_SAV,
	LVDS_FID_BUTT
};

struct lvds_fid_type_s {
	enum lvds_fid_type_e		fid;
};

struct lvds_vsync_type_s {
	enum lvds_vsync_type_e	sync_type;
	unsigned short			hblank1;
	unsigned short			hblank2;
};

struct lvds_dev_attr_s {
	enum wdr_mode_e			wdr_mode;
	enum lvds_sync_mode_e		sync_mode;
	enum raw_data_type_e		raw_data_type;
	enum lvds_bit_endian		data_endian;
	enum lvds_bit_endian		sync_code_endian;
	short				lane_id[MIPI_LANE_NUM+1];
	short		sync_code[MIPI_LANE_NUM][WDR_VC_NUM+1][SYNC_CODE_NUM];
/*
 * sublvds:
 * sync_code[x][0][0] sync_code[x][0][1] sync_code[x][0][2] sync_code[x][0][3]
 *	n0_lef_sav	   n0_lef_eav	      n1_lef_sav	 n1_lef_eav
 * sync_code[x][1][0] sync_code[x][1][1] sync_code[x][1][2] sync_code[x][1][3]
 *	n0_sef_sav	   n0_sef_eav	      n1_sef_sav	 n1_sef_eav
 * sync_code[x][2][0] sync_code[x][2][1] sync_code[x][2][2] sync_code[x][2][3]
 *	n0_lsef_sav	   n0_lsef_eav	      n1_lsef_sav	 n1_lsef_eav
 *
 * hispi:
 * sync_code[x][0][0] sync_code[x][0][1] sync_code[x][0][2] sync_code[x][0][3]
 *	t1_sol		   tl_eol	      t1_sof		 t1_eof
 * sync_code[x][1][0] sync_code[x][1][1] sync_code[x][1][2] sync_code[x][1][3]
 *	t2_sol		   t2_eol	      t2_sof		 t2_eof
 */
	struct lvds_vsync_type_s	vsync_type;
	struct lvds_fid_type_s		fid_type;
	char				pn_swap[MIPI_LANE_NUM+1];
};

struct mipi_dev_attr_s {
	enum raw_data_type_e		raw_data_type;
	short				lane_id[MIPI_LANE_NUM+1];
	enum mipi_wdr_mode_e		wdr_mode;
	short				data_type[WDR_VC_NUM];
	char				pn_swap[MIPI_LANE_NUM+1];
	struct dphy_s			dphy;
	enum mipi_vs_gen_mode           vs_gen_mode;
};

struct manual_wdr_attr_s {
	unsigned int			manual_en;
	unsigned short			l2s_distance;
	unsigned short			lsef_length;
	unsigned int			discard_padding_lines;
	unsigned int			update;
};

struct combo_dev_attr_s {
	enum input_mode_e		input_mode;
	enum rx_mac_clk_e		mac_clk;
	struct mclk_pll_s		mclk;
	union {
		struct mipi_dev_attr_s	mipi_attr;
		struct lvds_dev_attr_s	lvds_attr;
	};
	unsigned int			devno;
	struct img_size_s		img_size;
	struct manual_wdr_attr_s	wdr_manu;
};

enum clk_edge_e {
	CLK_UP_EDGE = 0,
	CLK_DOWN_EDGE,
	CLK_EDGE_BUTT
};

struct clk_edge_s {
	unsigned int			devno;
	enum clk_edge_e			edge;
};

enum output_msb_e {
	OUTPUT_NORM_MSB = 0,
	OUTPUT_REVERSE_MSB,
	OUTPUT_MSB_BUTT
};

struct msb_s {
	unsigned int			devno;
	enum output_msb_e		msb;
};

struct crop_top_s {
	unsigned int			devno;
	unsigned int			crop_top;
	unsigned int			update;
};

struct manual_wdr_s {
	unsigned int			devno;
	struct manual_wdr_attr_s	attr;
};

struct vsync_gen_s {
	unsigned int			devno;
	unsigned int			distance_fp;
};

enum bt_fmt_out_e {
	BT_FMT_OUT_CBYCRY,
	BT_FMT_OUT_CRYCBY,
	BT_FMT_OUT_YCBYCR,
	BT_FMT_OUT_YCRYCB,
};

struct bt_fmt_out_s {
	unsigned int			devno;
	enum bt_fmt_out_e		fmt_out;
};

struct cif_attr_s {
	unsigned int			devno;
	unsigned int			stagger_vsync;
};

/* mipi_rx ioctl commands related definition */
#define CVI_MIPI_IOC_MAGIC		'm'

/* Support commands */
#define CVI_MIPI_SET_DEV_ATTR		_IOW(CVI_MIPI_IOC_MAGIC, \
						0x01, struct combo_dev_attr_s)
#define CVI_MIPI_SET_OUTPUT_CLK_EDGE	_IOW(CVI_MIPI_IOC_MAGIC, \
						0x02, struct clk_edge_s)
#define CVI_MIPI_RESET_SENSOR		_IOW(CVI_MIPI_IOC_MAGIC, \
						0x05, unsigned int)
#define CVI_MIPI_UNRESET_SENSOR		_IOW(CVI_MIPI_IOC_MAGIC, \
						0x06, unsigned int)
#define CVI_MIPI_RESET_MIPI		_IOW(CVI_MIPI_IOC_MAGIC, \
						0x07, unsigned int)
#define CVI_MIPI_ENABLE_SENSOR_CLOCK	_IOW(CVI_MIPI_IOC_MAGIC, \
						0x10, unsigned int)
#define CVI_MIPI_DISABLE_SENSOR_CLOCK	_IOW(CVI_MIPI_IOC_MAGIC, \
						0x11, unsigned int)
#define CVI_MIPI_SET_CROP_TOP		_IOW(CVI_MIPI_IOC_MAGIC, \
						0x20, struct crop_top_s)
#define CVI_MIPI_SET_WDR_MANUAL		_IOW(CVI_MIPI_IOC_MAGIC, \
						0x21, struct manual_wdr_s)
#define CVI_MIPI_SET_LVDS_FP_VS		_IOW(CVI_MIPI_IOC_MAGIC, \
						0x22, struct vsync_gen_s)
#define CVI_MIPI_SET_HS_MODE		_IOW(CVI_MIPI_IOC_MAGIC, \
						0x0B, unsigned int)
#define CVI_MIPI_RESET_LVDS		_IOW(CVI_MIPI_IOC_MAGIC, \
						0x23, unsigned int)
#define CVI_MIPI_SET_BT_FMT_OUT		_IOW(CVI_MIPI_IOC_MAGIC, \
						0x24, struct bt_fmt_out_s)
#define CVI_MIPI_GET_CIF_ATTR		_IOWR(CVI_MIPI_IOC_MAGIC, \
						0x25, struct cif_attr_s)
#define CVI_MIPI_SET_SENSOR_CLOCK	_IOW(CVI_MIPI_IOC_MAGIC, \
						0x26, struct mclk_pll_s)
#define CVI_MIPI_SET_MAX_MAC_CLOCK	_IOW(CVI_MIPI_IOC_MAGIC, \
						0x27, unsigned int)
/* Unsupport commands */
#define CVI_MIPI_SET_PHY_CMVMODE	_IOW(CVI_MIPI_IOC_MAGIC, \
						0x04, unsigned int)
#define CVI_MIPI_UNRESET_MIPI		_IOW(CVI_MIPI_IOC_MAGIC, \
						0x08, unsigned int)
#define CVI_MIPI_RESET_SLVS		_IOW(CVI_MIPI_IOC_MAGIC, \
						0x09, unsigned int)
#define CVI_MIPI_UNRESET_SLVS		_IOW(CVI_MIPI_IOC_MAGIC, \
						0x0A, unsigned int)
#define CVI_MIPI_ENABLE_MIPI_CLOCK	_IOW(CVI_MIPI_IOC_MAGIC, \
						0x0C, unsigned int)
#define CVI_MIPI_DISABLE_MIPI_CLOCK	_IOW(CVI_MIPI_IOC_MAGIC, \
						0x0D, unsigned int)
#define CVI_MIPI_ENABLE_SLVS_CLOCK	_IOW(CVI_MIPI_IOC_MAGIC, \
						0x0E, unsigned int)
#define CVI_MIPI_DISABLE_SLVS_CLOCK	_IOW(CVI_MIPI_IOC_MAGIC, \
						0x0F, unsigned int)
#endif // _U_CVI_VIP_CIF_H_
