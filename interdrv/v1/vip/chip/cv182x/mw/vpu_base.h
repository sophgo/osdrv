/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: vpu_base.h
 * Description:
 */

#ifndef _VPU_BASE_H_
#define _VPU_BASE_H_

#ifdef __cplusplus
	extern "C" {
#endif

#define VI_MAX_VIR_CHN_NUM        2
#define VI_MAX_PHY_CHN_NUM        2
#define VI_MAX_EXT_CHN_NUM        2
#define VI_MAX_CHN_NUM            (VI_MAX_PHY_CHN_NUM + VI_MAX_VIR_CHN_NUM)

#define VI_MAX_DEV_NUM            2
#define VI_MAX_PHY_PIPE_NUM       4
#define VI_MAX_VIR_PIPE_NUM       0
#define VI_MAX_PIPE_NUM           (VI_MAX_PHY_PIPE_NUM + VI_MAX_VIR_PIPE_NUM)

#define VI_MAX_ADCHN_NUM (4UL)

#define VI_PIPE int32_t

#define VPSS_MAX_PHY_CHN_NUM       (3)
#define VPSS_MAX_CHN_NUM           (VPSS_MAX_PHY_CHN_NUM)
#define VPSS_MAX_GRP_NUM           (16)

#define VO_MAX_DEV_NUM             (1)       /* max dev num */
#define VO_MAX_LAYER_NUM           (1)       /* max layer num */
#define VO_MAX_CHN_NUM             (1)       /* max chn num */

/* VO video output interface type */
#define VO_INTF_CVBS (0x01L << 0)
#define VO_INTF_YPBPR (0x01L << 1)
#define VO_INTF_VGA (0x01L << 2)
#define VO_INTF_BT656 (0x01L << 3)
#define VO_INTF_BT1120 (0x01L << 6)
#define VO_INTF_LCD (0x01L << 7)
#define VO_INTF_LCD_18BIT (0x01L << 10)
#define VO_INTF_LCD_24BIT (0x01L << 11)
#define VO_INTF_LCD_30BIT (0x01L << 12)
#define VO_INTF_MIPI (0x01L << 13)
#define VO_INTF_MIPI_SLAVE (0x01L << 14)
#define VO_INTF_HDMI (0x01L << 15)
#define VO_INTF_I80 (0x01L << 16)

#define GDC_PROC_JOB_INFO_NUM      (500)

#define RGN_MAX_BUF_NUM            (2)
#define RGN_MAX_ATTACH_CHN_NUM     (4)
#define RGN_MAX_NUM                (100)
#define RGN_PROC_INFO_OFFSET       (sizeof(struct cvi_vpss_proc_ctx) * VPSS_MAX_GRP_NUM)

/* module params */
struct _VI_MOD_PARAM_S {
	int32_t s32DetectErrFrame;
	uint32_t u32DropErrFrame;
};

/* interface mode of video input */
enum _VI_INTF_MODE_E {
	VI_MODE_BT656 = 0, /* ITU-R BT.656 YUV4:2:2 */
	VI_MODE_BT601, /* ITU-R BT.601 YUV4:2:2 */
	VI_MODE_DIGITAL_CAMERA, /* digatal camera mode */
	VI_MODE_BT1120_STANDARD, /* BT.1120 progressive mode */
	VI_MODE_BT1120_INTERLEAVED, /* BT.1120 interstage mode */
	VI_MODE_MIPI, /* MIPI RAW mode */
	VI_MODE_MIPI_YUV420_NORMAL, /* MIPI YUV420 normal mode */
	VI_MODE_MIPI_YUV420_LEGACY, /* MIPI YUV420 legacy mode */
	VI_MODE_MIPI_YUV422, /* MIPI YUV422 mode */
	VI_MODE_LVDS, /* LVDS mode */
	VI_MODE_HISPI, /* HiSPi mode */
	VI_MODE_SLVS, /* SLVS mode */

	VI_MODE_BUTT
};

/* Work mode */
enum _VI_WORK_MODE_E {
	VI_WORK_MODE_1Multiplex = 0, /* 1 Multiplex mode */
	VI_WORK_MODE_2Multiplex, /* 2 Multiplex mode */
	VI_WORK_MODE_3Multiplex, /* 3 Multiplex mode */
	VI_WORK_MODE_4Multiplex, /* 4 Multiplex mode */

	VI_WORK_MODE_BUTT
};

/* whether an input picture is interlaced or progressive */
enum _VI_SCAN_MODE_E {
	VI_SCAN_INTERLACED = 0, /* interlaced mode */
	VI_SCAN_PROGRESSIVE, /* progressive mode */

	VI_SCAN_BUTT
};

enum _VI_YUV_DATA_SEQ_E {
	VI_DATA_SEQ_VUVU = 0,
	VI_DATA_SEQ_UVUV,

	VI_DATA_SEQ_UYVY, /* The input sequence of YUV is UYVY */
	VI_DATA_SEQ_VYUY, /* The input sequence of YUV is VYUY */
	VI_DATA_SEQ_YUYV, /* The input sequence of YUV is YUYV */
	VI_DATA_SEQ_YVYU, /* The input sequence of YUV is YVYU */

	VI_DATA_SEQ_BUTT
};

/* Attribute of the vertical synchronization signal */
enum _VI_VSYNC_E {
	VI_VSYNC_FIELD = 0, /* Field/toggle mode:a signal reversal means a new frame or a field */
	VI_VSYNC_PULSE, /* Pusle/effective mode:a pusle or an effective signal means a new frame or a field */

	VI_VSYNC_BUTT
};

/* Polarity of the vertical synchronization signal
 *
 * VI_VSYNC_NEG_HIGH: if VIU_VSYNC_E = VIU_VSYNC_FIELD,then the v-sync signal of even field is high-level,
 *		      if VIU_VSYNC_E = VIU_VSYNC_PULSE,then the v-sync pulse is positive pulse.
 * VI_VSYNC_NEG_LOW: if VIU_VSYNC_E = VIU_VSYNC_FIELD,then the v-sync signal of even field is low-level,
 *		     if VIU_VSYNC_E = VIU_VSYNC_PULSE,then the v-sync pulse is negative pulse.
 */
enum _VI_VSYNC_NEG_E {
	VI_VSYNC_NEG_HIGH = 0,
	VI_VSYNC_NEG_LOW,
	VI_VSYNC_NEG_BUTT
};

/* Attribute of the horizontal synchronization signal */
enum _VI_HSYNC_E {
	VI_HSYNC_VALID_SINGNAL = 0, /* the h-sync is valid signal mode */
	VI_HSYNC_PULSE, /* the h-sync is pulse mode, a new pulse means the beginning of a new line */

	VI_HSYNC_BUTT
};

/* Polarity of the horizontal synchronization signal
 *
 * VI_HSYNC_NEG_HIGH: if VI_HSYNC_E = VI_HSYNC_VALID_SINGNAL,then the valid h-sync signal is high-level;
 *		    if VI_HSYNC_E = VI_HSYNC_PULSE,then the h-sync pulse is positive pulse.
 * VI_HSYNC_NEG_LOW: if VI_HSYNC_E = VI_HSYNC_VALID_SINGNAL,then the valid h-sync signal is low-level;
 *		    if VI_HSYNC_E = VI_HSYNC_PULSE,then the h-sync pulse is negative pulse
 */
enum _VI_HSYNC_NEG_E {
	VI_HSYNC_NEG_HIGH = 0,
	VI_HSYNC_NEG_LOW,
	VI_HSYNC_NEG_BUTT
};

/* Attribute of the valid vertical synchronization signal
 *
 * VI_VSYNC_NORM_PULSE: the v-sync is pusle mode, a pusle means a new frame or field
 * VI_VSYNC_VALID_SIGNAL: the v-sync is effective mode, a effective signal means a new frame or field
 */
enum _VI_VSYNC_VALID_E {
	VI_VSYNC_NORM_PULSE = 0,
	VI_VSYNC_VALID_SIGNAL,

	VI_VSYNC_VALID_BUTT
};

/* Polarity of the valid vertical synchronization signal
 *
 * VI_VSYNC_VALID_NEG_HIGH: if VI_VSYNC_VALID_E = VI_VSYNC_NORM_PULSE,a positive pulse means v-sync pulse;
 *			    if VI_VSYNC_VALID_E = VI_VSYNC_VALID_SIGNAL,the valid v-sync signal is high-level
 * VI_VSYNC_VALID_NEG_LOW: if VI_VSYNC_VALID_E = VI_VSYNC_NORM_PULSE,a negative pulse means v-sync pulse
 *			   if VI_VSYNC_VALID_E = VI_VSYNC_VALID_SIGNAL,the valid v-sync signal is low-level
 */
enum _VI_VSYNC_VALID_NEG_E {
	VI_VSYNC_VALID_NEG_HIGH = 0,
	VI_VSYNC_VALID_NEG_LOW,
	VI_VSYNC_VALID_NEG_BUTT
};

/* Blank information of the input timing
 *
 * u32VsyncVfb: RW;Vertical front blanking height of one frame or odd-field frame picture
 * u32VsyncVact: RW;Vertical effetive width of one frame or odd-field frame picture
 * u32VsyncVbb: RW;Vertical back blanking height of one frame or odd-field frame picture
 * u32VsyncVbfb: RW;Even-field vertical front blanking height when input mode is interlace
 *		(invalid when progressive input mode)
 * u32VsyncVbact: RW;Even-field vertical effetive width when input mode is interlace
 *		(invalid when progressive input mode)
 * u32VsyncVbbb: RW;Even-field vertical back blanking height when input mode is interlace
 *		(invalid when progressive input mode)
 */
struct _VI_TIMING_BLANK_S {
	uint32_t u32HsyncHfb; /* RW;Horizontal front blanking width */
	uint32_t u32HsyncAct; /* RW;Horizontal effetive width */
	uint32_t u32HsyncHbb; /* RW;Horizontal back blanking width */
	uint32_t u32VsyncVfb;
	uint32_t u32VsyncVact;
	uint32_t u32VsyncVbb;
	uint32_t u32VsyncVbfb;
	uint32_t u32VsyncVbact;
	uint32_t u32VsyncVbbb;
};

/* synchronization information about the BT.601 or DC timing */
struct _VI_SYNC_CFG_S {
	enum _VI_VSYNC_E enVsync;
	enum _VI_VSYNC_NEG_E enVsyncNeg;
	enum _VI_HSYNC_E enHsync;
	enum _VI_HSYNC_NEG_E enHsyncNeg;
	enum _VI_VSYNC_VALID_E enVsyncValid;
	enum _VI_VSYNC_VALID_NEG_E enVsyncValidNeg;
	struct _VI_TIMING_BLANK_S stTimingBlank;
};

/* Input data type */
enum _VI_DATA_TYPE_E {
	VI_DATA_TYPE_YUV = 0,
	VI_DATA_TYPE_RGB,

	VI_DATA_TYPE_BUTT
};

struct _SIZE_S {
	uint32_t u32Width;
	uint32_t u32Height;
};

enum _WDR_MODE_E {
	WDR_MODE_NONE = 0,
	WDR_MODE_BUILT_IN,
	WDR_MODE_QUDRA,

	WDR_MODE_2To1_LINE,
	WDR_MODE_2To1_FRAME,
	WDR_MODE_2To1_FRAME_FULL_RATE,

	WDR_MODE_3To1_LINE,
	WDR_MODE_3To1_FRAME,
	WDR_MODE_3To1_FRAME_FULL_RATE,

	WDR_MODE_4To1_LINE,
	WDR_MODE_4To1_FRAME,
	WDR_MODE_4To1_FRAME_FULL_RATE,

	WDR_MODE_MAX,
};

/* Attribute of wdr */
struct _VI_WDR_ATTR_S {
	enum _WDR_MODE_E enWDRMode; /* RW; WDR mode.*/
	uint32_t u32CacheLine; /* RW; WDR cache line.*/
};

enum _BAYER_FORMAT_E {
	BAYER_FORMAT_BG = 0,
	BAYER_FORMAT_GB,
	BAYER_FORMAT_GR,
	BAYER_FORMAT_RG,
	BAYER_FORMAT_MAX
};

/* The attributes of a VI device
 *
 * enInputDataType: RW;RGB: CSC-709 or CSC-601, PT YUV444 disable; YUV: default yuv CSC coef PT YUV444 enable.
 */
struct _VI_DEV_ATTR_S {
	enum _VI_INTF_MODE_E enIntfMode; /* RW;Interface mode */
	enum _VI_WORK_MODE_E enWorkMode; /* RW;Work mode */

	enum _VI_SCAN_MODE_E enScanMode; /* RW;Input scanning mode (progressive or interlaced) */
	int32_t as32AdChnId[VI_MAX_ADCHN_NUM]; /* RW;AD channel ID. Typically, the default value -1 is recommended */

	/* The below members must be configured in BT.601 mode or DC mode and are invalid in other modes */
	enum _VI_YUV_DATA_SEQ_E enDataSeq; /* RW;Input data sequence (only the YUV format is supported) */
	struct _VI_SYNC_CFG_S stSynCfg; /* RW;Sync timing. This member must be configured in BT.601 mode or DC mode */

	enum _VI_DATA_TYPE_E enInputDataType;

	struct _SIZE_S stSize; /* RW;Input size */

	struct _VI_WDR_ATTR_S stWDRAttr; /* RW;Attribute of WDR */

	enum _BAYER_FORMAT_E enBayerFormat; /* RW;Bayer format of Device */

	u32 chn_num; /* R; total chnannels sended from dev */

	u32 snrFps; /* R; snr init fps from isp pub attr */
};

/* Information of pipe binded to device */
struct _VI_DEV_BIND_PIPE_S {
	uint32_t u32Num; /* RW;Range [1,VI_MAX_PIPE_NUM] */
	VI_PIPE PipeId[VI_MAX_PIPE_NUM]; /* RW;Array of pipe ID */
};

struct _VI_DEV_TIMING_ATTR_S {
	bool bEnable; /* RW;Whether enable VI generate timing */
	int32_t s32FrmRate; /* RW;Generate timing Frame rate*/
};

enum _PIXEL_FORMAT_E {
	PIXEL_FORMAT_RGB_888 = 0,
	PIXEL_FORMAT_BGR_888,
	PIXEL_FORMAT_RGB_888_PLANAR,
	PIXEL_FORMAT_BGR_888_PLANAR,

	PIXEL_FORMAT_ARGB_1555, // 4,
	PIXEL_FORMAT_ARGB_4444,
	PIXEL_FORMAT_ARGB_8888,

	PIXEL_FORMAT_RGB_BAYER_8BPP, // 7,
	PIXEL_FORMAT_RGB_BAYER_10BPP,
	PIXEL_FORMAT_RGB_BAYER_12BPP,
	PIXEL_FORMAT_RGB_BAYER_14BPP,
	PIXEL_FORMAT_RGB_BAYER_16BPP,

	PIXEL_FORMAT_YUV_PLANAR_422, // 12,
	PIXEL_FORMAT_YUV_PLANAR_420,
	PIXEL_FORMAT_YUV_PLANAR_444,
	PIXEL_FORMAT_YUV_400,

	PIXEL_FORMAT_HSV_888, // 16,
	PIXEL_FORMAT_HSV_888_PLANAR,

	PIXEL_FORMAT_NV12, // 18,
	PIXEL_FORMAT_NV21,
	PIXEL_FORMAT_NV16,
	PIXEL_FORMAT_NV61,
	PIXEL_FORMAT_YUYV,
	PIXEL_FORMAT_UYVY,
	PIXEL_FORMAT_YVYU,
	PIXEL_FORMAT_VYUY,

	PIXEL_FORMAT_FP32_C1 = 32, // 32
	PIXEL_FORMAT_FP32_C3_PLANAR,
	PIXEL_FORMAT_INT32_C1,
	PIXEL_FORMAT_INT32_C3_PLANAR,
	PIXEL_FORMAT_UINT32_C1,
	PIXEL_FORMAT_UINT32_C3_PLANAR,
	PIXEL_FORMAT_BF16_C1,
	PIXEL_FORMAT_BF16_C3_PLANAR,
	PIXEL_FORMAT_INT16_C1,
	PIXEL_FORMAT_INT16_C3_PLANAR,
	PIXEL_FORMAT_UINT16_C1,
	PIXEL_FORMAT_UINT16_C3_PLANAR,
	PIXEL_FORMAT_INT8_C1,
	PIXEL_FORMAT_INT8_C3_PLANAR,
	PIXEL_FORMAT_UINT8_C1,
	PIXEL_FORMAT_UINT8_C3_PLANAR,

	PIXEL_FORMAT_8BIT_MODE = 48, //48

	PIXEL_FORMAT_MAX
};

enum _DYNAMIC_RANGE_E {
	DYNAMIC_RANGE_SDR8 = 0,
	DYNAMIC_RANGE_SDR10,
	DYNAMIC_RANGE_HDR10,
	DYNAMIC_RANGE_HLG,
	DYNAMIC_RANGE_SLF,
	DYNAMIC_RANGE_XDR,
	DYNAMIC_RANGE_MAX
};

/*
 * VIDEO_FORMAT_LINEAR: nature video line.
 */
enum _VIDEO_FORMAT_E {
	VIDEO_FORMAT_LINEAR = 0,
	VIDEO_FORMAT_MAX
};

/*
 * COMPRESS_MODE_NONE: no compress.
 * COMPRESS_MODE_TILE: compress unit is a tile.
 * COMPRESS_MODE_LINE: compress unit is the whole line.
 * COMPRESS_MODE_FRAME: ompress unit is the whole frame.
 */
enum _COMPRESS_MODE_E {
	COMPRESS_MODE_NONE = 0,
	COMPRESS_MODE_TILE,
	COMPRESS_MODE_LINE,
	COMPRESS_MODE_FRAME,
	COMPRESS_MODE_BUTT
};

struct _FRAME_RATE_CTRL_S {
	int32_t s32SrcFrameRate; /* RW; source frame rate */
	int32_t s32DstFrameRate; /* RW; dest frame rate */
};

/* The attributes of channel */
struct _VI_CHN_ATTR_S {
	struct _SIZE_S stSize; /* RW;Channel out put size */
	enum _PIXEL_FORMAT_E enPixelFormat; /* RW;Pixel format */
	enum _DYNAMIC_RANGE_E enDynamicRange; /* RW;Dynamic Range */
	enum _VIDEO_FORMAT_E enVideoFormat; /* RW;Video format */
	enum _COMPRESS_MODE_E enCompressMode; /* RW;256B Segment compress or no compress. */
	bool bMirror; /* RW;Mirror enable */
	bool bFlip; /* RW;Flip enable */
	uint32_t u32Depth; /* RW;Range [0,8];Depth */
	struct _FRAME_RATE_CTRL_S stFrameRate; /* RW;Frame rate */
};

struct _VI_EARLY_INTERRUPT_S {
	bool bEnable;
	uint32_t u32LineCnt;
};

enum _VI_CROP_COORDINATE_E {
	VI_CROP_RATIO_COOR = 0, /* Ratio coordinate */
	VI_CROP_ABS_COOR, /* Absolute coordinate */
	VI_CROP_BUTT
};

/*Angle of rotation*/
enum _ROTATION_E {
	ROTATION_0 = 0,
	ROTATION_90,
	ROTATION_180,
	ROTATION_270,
	ROTATION_MAX
};

struct _RECT_S {
	int32_t s32X;
	int32_t s32Y;
	uint32_t u32Width;
	uint32_t u32Height;
};

/* Information of chn crop */
struct _VI_CROP_INFO_S {
	bool bEnable; /* RW;CROP enable*/
	enum _VI_CROP_COORDINATE_E enCropCoordinate; /* RW;Coordinate mode of the crop start point*/
	struct _RECT_S stCropRect; /* RW;CROP rectangular*/
};

/* The status of chn */
struct _VI_CHN_STATUS_S {
	bool bEnable; /* RO;Whether this channel is enabled */
	uint32_t u32FrameRate; /* RO;current frame rate */
	uint64_t u64PrevTime; // latest time (us)
	uint32_t u32FrameNum;  //The number of Frame in one second
	uint32_t u32LostFrame; /* RO;Lost frame count */
	uint32_t u32VbFail; /* RO;Video buffer malloc failure */
	uint32_t u32IntCnt; /* RO;Receive frame int count */
	uint32_t u32RecvPic; /* RO;Receive frame count */
	uint32_t u32TotalMemByte; /* RO;VI buffer malloc failure */
	struct _SIZE_S stSize; /* RO;chn output size */
};

enum VI_STATE {
	VI_RUNNING,
	VI_SUSPEND,
	VI_MAX,
};

struct cvi_vi_ctx {
	enum VI_STATE	vi_stt;
	u8		total_chn_num;
	u8		is_enable[VI_MAX_CHN_NUM + VI_MAX_EXT_CHN_NUM];
	u8		isDevEnable[VI_MAX_DEV_NUM];

	// mod param
	struct _VI_MOD_PARAM_S		modParam;

	// dev
	struct _VI_DEV_ATTR_S		devAttr[VI_MAX_DEV_NUM];
	struct _VI_DEV_BIND_PIPE_S	devBindPipeAttr[VI_MAX_DEV_NUM];
	struct _VI_DEV_TIMING_ATTR_S	stTimingAttr[VI_MAX_DEV_NUM];

	// chn
	struct _VI_CHN_ATTR_S chnAttr[VI_MAX_CHN_NUM];
	struct _VI_CHN_STATUS_S chnStatus[VI_MAX_CHN_NUM];
	struct _VI_CROP_INFO_S chnCrop[VI_MAX_CHN_NUM];
	struct _VI_EARLY_INTERRUPT_S enEalyInt[VI_MAX_CHN_NUM];
	enum _ROTATION_E enRotation[VI_MAX_CHN_NUM];
};

/*
 * VPSS_CROP_RATIO_COOR: Ratio coordinate.
 * VPSS_CROP_ABS_COOR: Absolute coordinate.
 */
enum _VPSS_CROP_COORDINATE_E {
	VPSS_CROP_RATIO_COOR = 0,
	VPSS_CROP_ABS_COOR,
};

/*
 * ASPECT_RATIO_NONE: full screen
 * ASPECT_RATIO_AUTO: Keep ratio, automatically get the region of video.
 * ASPECT_RATIO_MANUAL: Manully set the region of video.
 */
enum _ASPECT_RATIO_E {
	ASPECT_RATIO_NONE = 0,
	ASPECT_RATIO_AUTO,
	ASPECT_RATIO_MANUAL,
	ASPECT_RATIO_MAX
};

enum _VPSS_ROUNDING_E {
	VPSS_ROUNDING_TO_EVEN = 0,
	VPSS_ROUNDING_AWAY_FROM_ZERO,
	VPSS_ROUNDING_TRUNCATE,
	VPSS_ROUNDING_MAX,
};

/*
 * u32MaxW: Range: Width of source image.
 * u32MaxH: Range: Height of source image.
 * enPixelFormat: Pixel format of target image.
 * stFrameRate: Frame rate control info.
 * u8VpssDev: Only meaningful if VPSS_MODE_DUAL.
 */
struct _VPSS_GRP_ATTR_S {
	uint32_t u32MaxW;
	uint32_t u32MaxH;
	enum _PIXEL_FORMAT_E enPixelFormat;
	struct _FRAME_RATE_CTRL_S stFrameRate;
	uint8_t u8VpssDev;
};

/*
 * bEnable: RW; CROP enable.
 * enCropCoordinate: RW; Coordinate mode of the crop start point.
 * stCropRect: CROP rectangle.
 */
struct _VPSS_CROP_INFO_S {
	bool bEnable;
	enum _VPSS_CROP_COORDINATE_E enCropCoordinate;
	struct _RECT_S stCropRect;
};

/*
 * enMode: aspect ratio mode: none/auto/manual
 * bEnableBgColor: fill bgcolor
 * u32BgColor: background color, RGB 888
 * stVideoRect: valid in ASPECT_RATIO_MANUAL mode
 */
struct _ASPECT_RATIO_S {
	enum _ASPECT_RATIO_E enMode;
	bool bEnableBgColor;
	uint32_t u32BgColor;
	struct _RECT_S stVideoRect;
};

/*
 * bEnable: Whether Normalize is enabled.
 * factor: scaling factors for 3 planes.
 * mean: minus means for 3 planes.
 */
struct _VPSS_NORMALIZE_S {
	bool bEnable;
	float factor[3];
	float mean[3];
	enum _VPSS_ROUNDING_E rounding;
};

struct _VPSS_CHN_ATTR_S {
	uint32_t u32Width;
	uint32_t u32Height;
	enum _VIDEO_FORMAT_E enVideoFormat;
	enum _PIXEL_FORMAT_E enPixelFormat;
	struct _FRAME_RATE_CTRL_S stFrameRate;
	bool bMirror;
	bool bFlip;
	uint32_t u32Depth;
	struct _ASPECT_RATIO_S stAspectRatio;
	struct _VPSS_NORMALIZE_S stNormalize;
};

struct _LDC_ATTR_S {
	bool bAspect; /* RW;Whether aspect ration  is keep */
	int32_t s32XRatio; /* RW; Range: [0, 100], field angle ration of  horizontal,valid when bAspect=0.*/
	int32_t s32YRatio; /* RW; Range: [0, 100], field angle ration of  vertical,valid when bAspect=0.*/
	int32_t s32XYRatio; /* RW; Range: [0, 100], field angle ration of  all,valid when bAspect=1.*/
	int32_t s32CenterXOffset;
	int32_t s32CenterYOffset;
	int32_t s32DistortionRatio;
};

/*
 * bEnable: Whether LDC is enbale.
 * stAttr: LDC Attribute.
 */
struct _VPSS_LDC_ATTR_S {
	bool bEnable;
	struct _LDC_ATTR_S stAttr;
};

/*
 * bEnable: Enable low delay or not.
 * u32LineCnt: line cnt to notify.
 */
struct _VPSS_LOW_DELAY_INFO_S {
	bool bEnable;
	uint32_t u32LineCnt;
};

struct _VPSS_GRP_WORK_STATUS_S {
	uint32_t u32RecvCnt;
	uint32_t u32LostCnt;
	uint32_t u32StartFailCnt; //start job fail cnt
	uint32_t u32CostTime; // current job cost time in us
	uint32_t u32MaxCostTime;
	uint32_t u32HwCostTime; // current job Hw cost time in us
	uint32_t u32HwMaxCostTime;
};

struct _VPSS_CHN_CFG {
	bool isEnabled;
	struct _VPSS_CHN_ATTR_S stChnAttr;
	struct _VPSS_CROP_INFO_S stCropInfo;
	enum _ROTATION_E enRotation;
	struct _VPSS_LDC_ATTR_S stLDCAttr;
	uint32_t u32SendOk; // send OK cnt after latest chn enable
	uint64_t u64PrevTime; // latest time (us)
	uint32_t u32FrameNum;  //The number of Frame in one second
	uint32_t u32RealFrameRate; // chn real time frame rate
	struct _VPSS_LOW_DELAY_INFO_S stLowDelayInfo;
	uint32_t u32LowDelayTime; // time cost in us to receive early interrupt
	uint32_t u32LowDelayMaxTime;
};

struct cvi_vpss_proc_ctx {
	bool isCreated;
	bool isStarted;
	struct _VPSS_GRP_ATTR_S stGrpAttr;
	struct _VPSS_CROP_INFO_S stGrpCropInfo;
	uint8_t chnNum;
	struct _VPSS_CHN_CFG stChnCfgs[VPSS_MAX_CHN_NUM];
	struct _VPSS_GRP_WORK_STATUS_S stGrpWorkStatus;
};

enum _VO_INTF_SYNC_E {
	VO_OUTPUT_PAL = 0, /* PAL standard*/
	VO_OUTPUT_NTSC, /* NTSC standard */

	VO_OUTPUT_1080P24, /* 1920 x 1080 at 24 Hz. */
	VO_OUTPUT_1080P25, /* 1920 x 1080 at 25 Hz. */
	VO_OUTPUT_1080P30, /* 1920 x 1080 at 30 Hz. */

	VO_OUTPUT_720P50, /* 1280 x  720 at 50 Hz. */
	VO_OUTPUT_720P60, /* 1280 x  720 at 60 Hz. */
	VO_OUTPUT_1080P50, /* 1920 x 1080 at 50 Hz. */
	VO_OUTPUT_1080P60, /* 1920 x 1080 at 60 Hz. */

	VO_OUTPUT_576P50, /* 720  x  576 at 50 Hz. */
	VO_OUTPUT_480P60, /* 720  x  480 at 60 Hz. */

	VO_OUTPUT_800x600_60, /* VESA 800 x 600 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1024x768_60, /* VESA 1024 x 768 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1280x1024_60, /* VESA 1280 x 1024 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1366x768_60, /* VESA 1366 x 768 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1440x900_60, /* VESA 1440 x 900 at 60 Hz (non-interlaced) CVT Compliant */
	VO_OUTPUT_1280x800_60, /* 1280*800@60Hz VGA@60Hz*/
	VO_OUTPUT_1600x1200_60, /* VESA 1600 x 1200 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1680x1050_60, /* VESA 1680 x 1050 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1920x1200_60, /* VESA 1920 x 1600 at 60 Hz (non-interlaced) CVT (Reduced Blanking)*/
	VO_OUTPUT_640x480_60, /* VESA 640 x 480 at 60 Hz (non-interlaced) CVT */
	VO_OUTPUT_720x1280_60, /* For MIPI DSI Tx 720 x1280 at 60 Hz */
	VO_OUTPUT_1080x1920_60, /* For MIPI DSI Tx 1080x1920 at 60 Hz */
	VO_OUTPUT_480x800_60, /* For MIPI DSI Tx 480x800 at 60 Hz */
	VO_OUTPUT_USER, /* User timing. */

	VO_OUTPUT_BUTT

};

struct _VO_SYNC_INFO_S {
	bool bSynm;
	bool bIop;
	uint16_t u16FrameRate;

	uint16_t u16Vact;
	uint16_t u16Vbb;
	uint16_t u16Vfb;

	uint16_t u16Hact;
	uint16_t u16Hbb;
	uint16_t u16Hfb;

	uint16_t u16Hpw;
	uint16_t u16Vpw;

	bool bIdv;
	bool bIhs;
	bool bIvs;
};

/* Define I80's lane (0~3)
 *
 * CS: Chip Select
 * RS(DCX): Data/Command
 * WR: MCU Write to bus
 * RD: MCU Read from bus
 */
struct _VO_I80_LANE_S {
	uint8_t CS;
	uint8_t RS;
	uint8_t WR;
	uint8_t RD;
};

enum _VO_I80_FORMAT {
	VO_I80_FORMAT_RGB444 = 0,
	VO_I80_FORMAT_RGB565,
	VO_I80_FORMAT_RGB666,
	VO_I80_FORMAT_MAX
};

/* Define I80's config
 *
 * lane_s: lane mapping
 * fmt: format of data
 * cycle_time: cycle time of WR/RD, unit ns, max 250
 */
struct _VO_I80_CFG_S {
	struct _VO_I80_LANE_S lane_s;
	enum _VO_I80_FORMAT fmt;
	uint16_t cycle_time;
};

typedef enum {
	GPIO_ACTIVE_LOW,
	GPIO_ACTIVE_HIGH,
	GPIO_ACTIVE_BUFF
} GPIO_ACTIVE_E;

enum _VO_LVDS_LANE_ID {
	VO_LVDS_LANE_CLK = 0,
	VO_LVDS_LANE_0,
	VO_LVDS_LANE_1,
	VO_LVDS_LANE_2,
	VO_LVDS_LANE_3,
	VO_LVDS_LANE_MAX,
};

enum _VO_LVDS_OUT_BIT_E {
	VO_LVDS_OUT_6BIT = 0,
	VO_LVDS_OUT_8BIT,
	VO_LVDS_OUT_10BIT,
	VO_LVDS_OUT_MAX,
};

enum _VO_LVDS_MODE_E {
	VO_LVDS_MODE_JEIDA = 0,
	VO_LVDS_MODE_VESA,
	VO_LVDS_MODE_MAX,
};

struct _VO_LVDS_CTL_PIN_S {
	uint32_t gpio_num;
	GPIO_ACTIVE_E active;
};

/* Define LVDS's config
 *
 * lvds_vesa_mode: true for VESA mode; false for JEIDA mode
 * data_big_endian: true for big endian; true for little endian
 * lane_id: lane mapping, -1 no used
 * lane_pn_swap: lane pn-swap if true
 * backlight_pin: backlight GPIO
 */
struct _VO_LVDS_ATTR_S {
	enum _VO_LVDS_OUT_BIT_E out_bits;
	enum _VO_LVDS_MODE_E mode;
	uint8_t chn_num;
	bool data_big_endian;
	enum _VO_LVDS_LANE_ID lane_id[VO_LVDS_LANE_MAX];
	bool lane_pn_swap[VO_LVDS_LANE_MAX];
	struct _VO_SYNC_INFO_S stSyncInfo;
	uint32_t pixelclock;
	struct _VO_LVDS_CTL_PIN_S backlight_pin;
};

/*
 * u32BgColor: Background color of a device, in RGB format.
 * enIntfType: Type of a VO interface.
 * enIntfSync: Type of a VO interface timing.
 * stSyncInfo: Information about VO interface timings if customed type.
 * sti80Cfg: attritube for i80 interface if IntfType is i80
 * stLvdsAttr: attritube for lvds interface if IntfType is lvds
 */
struct _VO_PUB_ATTR_S {
	uint32_t u32BgColor;
	uint32_t enIntfType;
	enum _VO_INTF_SYNC_E enIntfSync;
	struct _VO_SYNC_INFO_S stSyncInfo;
	union {
		struct _VO_I80_CFG_S sti80Cfg;
		struct _VO_LVDS_ATTR_S stLvdsAttr;
	};
};

/*
 * stDispRect: Display resolution
 * stImageSize: Original ImageSize.
 *              Only useful if vo support scaling, otherwise, it should be the same width stDispRect.
 * u32DispFrmRt: frame rate.
 * enPixFormat: Pixel format of the video layer
 */
struct _VO_VIDEO_LAYER_ATTR_S {
	struct _RECT_S stDispRect;
	struct _SIZE_S stImageSize;
	uint32_t u32DispFrmRt;
	enum _PIXEL_FORMAT_E enPixFormat;
};

enum _PROC_AMP_E {
	PROC_AMP_BRIGHTNESS = 0,
	PROC_AMP_CONTRAST,
	PROC_AMP_SATURATION,
	PROC_AMP_HUE,
	PROC_AMP_MAX,
};

/*
 * u32Priority: Video out overlay priority.
 * stRect: Rectangle of video output channel.
 */
struct _VO_CHN_ATTR_S {
	uint32_t u32Priority;
	struct _RECT_S stRect;
};

struct cvi_vo_proc_ctx {
	bool is_dev_enable[VO_MAX_DEV_NUM];
	bool is_layer_enable[VO_MAX_LAYER_NUM];
	bool is_chn_enable[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM];

	// dev
	struct _VO_PUB_ATTR_S stPubAttr[VO_MAX_DEV_NUM];

	// layer
	struct _VO_VIDEO_LAYER_ATTR_S stLayerAttr[VO_MAX_LAYER_NUM];
	uint32_t u32DisBufLen[VO_MAX_LAYER_NUM];
	uint32_t proc_amp[VO_MAX_LAYER_NUM][PROC_AMP_MAX];

	// chn
	struct _VO_CHN_ATTR_S stChnAttr[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM];
	enum _ROTATION_E enRotation[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM];
	bool pause[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM];
	bool show[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM];
	uint64_t u64DisplayPts[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM];
	uint64_t u64PreDonePts[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM];

	// for calculating chn frame rate
	uint32_t u32frameCnt[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM]; // frame cnt in one second
	uint64_t u64PrevTime[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM]; // latest time (us)
	uint32_t u32RealFrameRate[VO_MAX_LAYER_NUM][VO_MAX_CHN_NUM];
};

#define FOREACH_MOD(MOD) {\
	MOD(BASE)   \
	MOD(VB)	    \
	MOD(SYS)    \
	MOD(RGN)    \
	MOD(CHNL)   \
	MOD(VDEC)   \
	MOD(VPSS)   \
	MOD(VENC)   \
	MOD(H264E)  \
	MOD(JPEGE)  \
	MOD(MPEG4E) \
	MOD(H265E)  \
	MOD(JPEGD)  \
	MOD(VO)	    \
	MOD(VI)	    \
	MOD(DIS)    \
	MOD(RC)	    \
	MOD(AIO)    \
	MOD(AI)	    \
	MOD(AO)	    \
	MOD(AENC)   \
	MOD(ADEC)   \
	MOD(AUD)   \
	MOD(VPU)    \
	MOD(ISP)    \
	MOD(IVE)    \
	MOD(USER)   \
	MOD(PROC)   \
	MOD(LOG)    \
	MOD(H264D)  \
	MOD(GDC)    \
	MOD(PHOTO)  \
	MOD(FB)	    \
	MOD(BUTT)   \
}

#define GENERATE_ENUM(ENUM) CVI_ID_ ## ENUM,

typedef enum _MOD_ID_E FOREACH_MOD(GENERATE_ENUM) MOD_ID_E;

struct _MMF_CHN_S {
	MOD_ID_E    enModId;
	int32_t     s32DevId;
	int32_t     s32ChnId;
};

enum gdc_job_state {
	GDC_JOB_SUCCESS = 0,
	GDC_JOB_FAIL,
	GDC_JOB_WORKING,
};

struct gdc_job_info {
	int64_t hHandle;
	MOD_ID_E enModId; // the module submitted gdc job
	uint32_t u32TaskNum; // number of tasks
	enum gdc_job_state eState; // job state
	uint32_t u32InSize;
	uint32_t u32OutSize;
	uint32_t u32CostTime; // From job submitted to job done
	uint32_t u32HwTime; // HW cost time
	uint32_t u32BusyTime; // From job submitted to job commit to driver
	uint64_t u64SubmitTime; // us
};

struct gdc_job_status {
	uint32_t u32Success;
	uint32_t u32Fail;
	uint32_t u32Cancel;
	uint32_t u32BeginNum;
	uint32_t u32BusyNum;
	uint32_t u32ProcingNum;
};

struct gdc_task_status {
	uint32_t u32Success;
	uint32_t u32Fail;
	uint32_t u32Cancel;
	uint32_t u32BusyNum;
};

struct gdc_operation_status {
	uint32_t u32AddTaskSuc;
	uint32_t u32AddTaskFail;
	uint32_t u32EndSuc;
	uint32_t u32EndFail;
	uint32_t u32CbCnt;
};

struct cvi_gdc_proc_ctx {
	struct gdc_job_info stJobInfo[GDC_PROC_JOB_INFO_NUM];
	uint16_t u16JobInfoIdx; // latest job submitted
	struct gdc_job_status stJobStatus;
	struct gdc_task_status stTaskStatus;
	struct gdc_operation_status stFishEyeStatus;
};

/* type of video regions */
enum _RGN_TYPE_E {
	OVERLAY_RGN = 0,
	COVER_RGN,
	COVEREX_RGN,
	OVERLAYEX_RGN,
	MOSAIC_RGN,
	RGN_BUTT
};

/*
 * u32BgColor: background color, format depends on "enPixelFormat"
 * stSize: region size
 * u32CanvasNum: num of canvas. 2 for double buffer.
 */
struct _OVERLAY_ATTR_S {
	enum _PIXEL_FORMAT_E enPixelFormat;
	uint32_t u32BgColor;
	struct _SIZE_S stSize;
	uint32_t u32CanvasNum;
};

/*
 * u32BgColor: background color, format depends on "enPixelFormat"
 * stSize: region size
 * u32CanvasNum: num of canvas. 2 for double buffer.
 */
struct _OVERLAYEX_ATTR_S {
	enum _PIXEL_FORMAT_E enPixelFormat;
	uint32_t u32BgColor;
	struct _SIZE_S stSize;
	uint32_t u32CanvasNum;
};

union _RGN_ATTR_U {
	struct _OVERLAY_ATTR_S stOverlay; /* attribute of overlay region */
	struct _OVERLAYEX_ATTR_S stOverlayEx; /* attribute of overlayex region */
};

/* attribute of a region.
 *
 * enType: region type.
 * unAttr: region attribute.
 */
struct _RGN_ATTR_S {
	enum _RGN_TYPE_E enType;
	union _RGN_ATTR_U unAttr;
};

enum _RGN_AREA_TYPE_E {
	AREA_RECT = 0,
	AREA_QUAD_RANGLE,
	AREA_BUTT
};

struct _POINT_S {
	int32_t s32X;
	int32_t s32Y;
};

/*
 * bSolid: whether solid or dashed quadrangle
 * u32Thick: Line Width of quadrangle, valid when dashed quadrangle
 * stPoint[4]: points of quadrilateral
 */
struct _RGN_QUADRANGLE_S {
	bool bSolid;
	uint32_t u32Thick;
	struct _POINT_S stPoint[4];
};

/*
 * RGN_ABS_COOR: Absolute coordinate.
 * RGN_RATIO_COOR: Ratio coordinate.
 */
enum _RGN_COORDINATE_E {
	RGN_ABS_COOR = 0,
	RGN_RATIO_COOR
};

/*
 * enCoverType: rect or arbitrary quadrilateral COVER
 * stRect: config of rect
 * stQuadRangle: config of arbitrary quadrilateral COVER
 * u32Color: color of region.
 * u32Layer: COVER region layer
 * enCoordinate: ratio coordiante or abs coordinate
 */
struct _COVER_CHN_ATTR_S {
	enum _RGN_AREA_TYPE_E enCoverType;
	union {
		struct _RECT_S stRect;
		struct _RGN_QUADRANGLE_S stQuadRangle;
	};
	uint32_t u32Color;
	uint32_t u32Layer;
	enum _RGN_COORDINATE_E enCoordinate;
};

/*
 * enCoverType: rect or arbitrary quadrilateral COVER
 * stRect: config of rect
 * stQuadRangle: config of arbitrary quadrilateral COVER
 * u32Color: color of region.
 * u32Layer: COVER region layer
 */
struct _COVEREX_CHN_ATTR_S {
	enum _RGN_AREA_TYPE_E enCoverType;
	union {
		struct _RECT_S stRect;
		struct _RGN_QUADRANGLE_S stQuadRangle;
	};
	uint32_t u32Color;
	uint32_t u32Layer;
};

enum _INVERT_COLOR_MODE_E {
	LESSTHAN_LUM_THRESH = 0, /* the lum of the video is less than the lum threshold which is set by u32LumThresh  */
	MORETHAN_LUM_THRESH,     /* the lum of the video is more than the lum threshold which is set by u32LumThresh  */
	INVERT_COLOR_BUTT
};

struct _OVERLAY_INVERT_COLOR_S {
	struct _SIZE_S stInvColArea;
	uint32_t u32LumThresh;
	enum _INVERT_COLOR_MODE_E enChgMod;
	bool bInvColEn;  /* The switch of inverting color. */
};

/*
 * stPoint: position of region.
 * u32Layer: region layer.
 */
struct _OVERLAY_CHN_ATTR_S {
	struct _POINT_S stPoint;
	uint32_t u32Layer;
	struct _OVERLAY_INVERT_COLOR_S stInvertColor;
};

/*
 * stPoint: position of region.
 * u32Layer: region layer.
 */
struct _OVERLAYEX_CHN_ATTR_S {
	struct _POINT_S stPoint;
	uint32_t u32Layer;
	struct _OVERLAY_INVERT_COLOR_S stInvertColor;
};

enum _MOSAIC_BLK_SIZE_E {
	MOSAIC_BLK_SIZE_8 = 0, /* block size 8*8 of MOSAIC */
	MOSAIC_BLK_SIZE_16, /* block size 16*16 of MOSAIC */
	MOSAIC_BLK_SIZE_32, /* block size 32*32 of MOSAIC */
	MOSAIC_BLK_SIZE_64, /* block size 64*64 of MOSAIC */
	MOSAIC_BLK_SIZE_BUTT
};

/*
 * stRect: config of rect
 * enBlkSize: block size of MOSAIC
 * u32Layer: MOSAIC region layer range
 */
struct _MOSAIC_CHN_ATTR_S {
	struct _RECT_S stRect;
	enum _MOSAIC_BLK_SIZE_E enBlkSize;
	uint32_t u32Layer;
};

/*
 * stOverlayChn: attribute of overlay region
 * stCoverChn: attribute of cover region
 * stCoverExChn: attribute of coverex region
 * stOverlayExChn: attribute of overlayex region
 * stMosaicChn: attribute of mosic region
 */
union _RGN_CHN_ATTR_U {
	struct _OVERLAY_CHN_ATTR_S stOverlayChn;
	struct _COVER_CHN_ATTR_S stCoverChn;
	struct _COVEREX_CHN_ATTR_S stCoverExChn;
	struct _OVERLAYEX_CHN_ATTR_S stOverlayExChn;
	struct _MOSAIC_CHN_ATTR_S stMosaicChn;
};

/* attribute of a region
 *
 * bShow: region show or not.
 * enType: region type.
 * unChnAttr: region attribute.
 */
struct _RGN_CHN_ATTR_S {
	bool bShow;
	enum _RGN_TYPE_E enType;
	union _RGN_CHN_ATTR_U unChnAttr;
};

struct _RGN_CANVAS_INFO_S {
	uint64_t u64PhyAddr;
	uint8_t *pu8VirtAddr;
#ifdef __arm__
	__u32 padding; /* padding for keeping same size of this structure */
#endif
	struct _SIZE_S stSize;
	uint32_t u32Stride;
	enum _PIXEL_FORMAT_E enPixelFormat;
};

struct rgn_proc_ctx {
	uint32_t Handle;
	bool bCreated;
	bool bUsed;
	struct _RGN_ATTR_S stRegion;
	struct _MMF_CHN_S stChns[RGN_MAX_ATTACH_CHN_NUM];
	int8_t attachChnNum;
	struct _RGN_CHN_ATTR_S stChnsAttr[RGN_MAX_ATTACH_CHN_NUM];
	struct _RGN_CANVAS_INFO_S stCanvasInfo[RGN_MAX_BUF_NUM];
	uint8_t canvas_idx;
};

#ifdef __cplusplus
}
#endif

#endif /* _VPU_BASE_H_ */
