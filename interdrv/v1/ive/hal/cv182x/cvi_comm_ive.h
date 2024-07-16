#ifndef _CVI_COMM_IVE_H_
#define _CVI_COMM_IVE_H_
#include "cvi_type.h"

#define CVI_IVE2_LENGTH_ALIGN 1

typedef void *IVE_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif
typedef struct CVI_IMG CVI_IMG_S;
#ifdef __cplusplus
}
#endif

typedef struct _IVE_POINT_U16_S {
	unsigned short u16X;
	unsigned short u16Y;
} IVE_POINT_U16_S;

typedef struct _IVE_POINT_S16_S {
	unsigned short s16X;
	unsigned short s16Y;
} IVE_POINT_S16_S;

typedef struct _IVE_POINT_S25Q7_S {
	int s25q7X; /*X coordinate*/
	int s25q7Y; /*Y coordinate*/
} IVE_POINT_S25Q7_S;

typedef struct _IVE_RECT_U16_S {
	unsigned short u16X;
	unsigned short u16Y;
	unsigned short u16Width;
	unsigned short u16Height;
} IVE_RECT_U16_S;

#define IVE_HIST_NUM 256
#define IVE_MAP_NUM 256
#define IVE_MAX_REGION_NUM 254
#define IVE_ST_MAX_CORNER_NUM 500

typedef enum IVE_DMA_MODE {
	IVE_DMA_MODE_DIRECT_COPY = 0x0,
	IVE_DMA_MODE_INTERVAL_COPY = 0x1,
	IVE_DMA_MODE_SET_3BYTE = 0x2,
	IVE_DMA_MODE_SET_8BYTE = 0x3,
	IVE_DMA_MODE_BUTT
} IVE_DMA_MODE_E;

typedef struct IVE_DMA_CTRL {
	IVE_DMA_MODE_E enMode;
	CVI_U64 u64Val;
	CVI_U8 u8HorSegSize;
	CVI_U8 u8ElemSize;
	CVI_U8 u8VerSegRows;
} IVE_DMA_CTRL_S;

typedef struct IVE_MEM_INFO {
	CVI_U32 u32PhyAddr;
	CVI_U8 *pu8VirAddr;
	CVI_U32 u32ByteSize;
} IVE_MEM_INFO_S;

typedef IVE_MEM_INFO_S IVE_SRC_MEM_INFO_S;
typedef IVE_MEM_INFO_S IVE_DST_MEM_INFO_S;

typedef struct _IVE_LOOK_UP_TABLE_S {
	IVE_MEM_INFO_S stTable;
	unsigned short u16ElemNum; /*LUT's elements number*/

	unsigned char u8TabInPreci;
	unsigned char u8TabOutNorm;

	int s32TabInLower; /*LUT's original input lower limit*/
	int s32TabInUpper; /*LUT's original input upper limit*/
} IVE_LOOK_UP_TABLE_S;

typedef struct IVE_DATA {
	CVI_U32 u32PhyAddr;
	CVI_U8 *pu8VirAddr;

	CVI_U16 u16Stride;
	CVI_U16 u16Width;
	CVI_U16 u16Height;

	CVI_U16 u16Reserved;
	CVI_IMG_S *tpu_block;
} IVE_DATA_S;

typedef IVE_DATA_S IVE_SRC_DATA_S;
typedef IVE_DATA_S IVE_DST_DATA_S;

typedef enum IVE_IMAGE_TYPE {
	IVE_IMAGE_TYPE_U8C1 = 0x0,
	IVE_IMAGE_TYPE_S8C1 = 0x1,

	IVE_IMAGE_TYPE_YUV420SP = 0x2,
	IVE_IMAGE_TYPE_YUV422SP = 0x3,
	IVE_IMAGE_TYPE_YUV420P = 0x4,
	IVE_IMAGE_TYPE_YUV422P = 0x5,

	IVE_IMAGE_TYPE_S8C2_PACKAGE = 0x6,
	IVE_IMAGE_TYPE_S8C2_PLANAR = 0x7,

	IVE_IMAGE_TYPE_S16C1 = 0x8,
	IVE_IMAGE_TYPE_U16C1 = 0x9,

	IVE_IMAGE_TYPE_U8C3_PACKAGE = 0xa,
	IVE_IMAGE_TYPE_U8C3_PLANAR = 0xb,

	IVE_IMAGE_TYPE_S32C1 = 0xc,
	IVE_IMAGE_TYPE_U32C1 = 0xd,

	IVE_IMAGE_TYPE_S64C1 = 0xe,
	IVE_IMAGE_TYPE_U64C1 = 0xf,

	IVE_IMAGE_TYPE_BF16C1 = 0x10,
	IVE_IMAGE_TYPE_FP32C1 = 0x11,

	IVE_IMAGE_TYPE_BUTT

} IVE_IMAGE_TYPE_E;

typedef struct IVE_IMAGE {
	IVE_IMAGE_TYPE_E enType;

	CVI_U64 u64PhyAddr[3];
	CVI_U8 *pu8VirAddr[3];

	CVI_U16 u16Stride[3];
	CVI_U16 u16Width;
	CVI_U16 u16Height;

	CVI_U16 u16Reserved;
	CVI_IMG_S *tpu_block;
} IVE_IMAGE_S;

typedef IVE_IMAGE_S IVE_SRC_IMAGE_S;
typedef IVE_IMAGE_S IVE_DST_IMAGE_S;

typedef enum IVE_ITC_TYPE {
	IVE_ITC_SATURATE = 0x0,
	IVE_ITC_NORMALIZE = 0x1,
} IVE_ITC_TYPE_E;

typedef struct IVE_ITC_CRTL {
	IVE_ITC_TYPE_E enType;
} IVE_ITC_CRTL_S;

typedef struct IVE_ADD_CTRL_S {
	CVI_FLOAT aX;
	CVI_FLOAT bY;
} IVE_ADD_CTRL_S;

typedef struct IVE_BLOCK_CTRL {
	CVI_FLOAT f32ScaleSize;
	CVI_U32 u32CellSize;
} IVE_BLOCK_CTRL_S;

typedef struct IVE_BLEND_CTRL_S {
	CVI_U8 u8Weight;
} IVE_BLEND_CTRL_S;

typedef struct IVE_ELEMENT_STRUCTURE_CTRL {
	CVI_U8 au8Mask[25];
} IVE_ELEMENT_STRUCTURE_CTRL_S;

typedef IVE_ELEMENT_STRUCTURE_CTRL_S IVE_DILATE_CTRL_S;
typedef IVE_ELEMENT_STRUCTURE_CTRL_S IVE_ERODE_CTRL_S;

typedef struct IVE_FILTER_CTRL {
	CVI_U8 u8MaskSize;
	CVI_S8 as8Mask[25];
	CVI_U32 u32Norm;
} IVE_FILTER_CTRL_S;

typedef struct IVE_HOG_CTRL {
	CVI_U8 u8BinSize;
	CVI_U32 u32CellSize;
	CVI_U16 u16BlkSizeInCell;
	CVI_U16 u16BlkStepX;
	CVI_U16 u16BlkStepY;
} IVE_HOG_CTRL_S;

typedef enum IVE_MAG_AND_ANG_OUT_CTRL {
	IVE_MAG_AND_ANG_OUT_CTRL_MAG = 0x0,
	IVE_MAG_AND_ANG_OUT_CTRL_ANG = 0x1,
	IVE_MAG_AND_ANG_OUT_CTRL_MAG_AND_ANG = 0x2,
	IVE_MAG_AND_ANG_OUT_CTRL_BUTT
} IVE_MAG_AND_ANG_OUT_CTRL_E;

typedef enum IVE_MAG_DIST {
	IVE_MAG_DIST_L1 = 0x0,
	IVE_MAG_DIST_L2 = 0x1,
	IVE_MAG_DIST_BUTT
} IVE_MAG_DIST_E;

/*
 *Magnitude and angle control parameter
 */
typedef struct IVE_MAG_AND_ANG_CTRL {
	IVE_MAG_AND_ANG_OUT_CTRL_E enOutCtrl;
	IVE_MAG_DIST_E enDistCtrl;
	CVI_U16 u16Thr;
	CVI_S8 as8Mask[25]; /*Template parameter.*/
} IVE_MAG_AND_ANG_CTRL_S;

typedef enum IVE_NORM_GRAD_OUT_CTRL {
	IVE_NORM_GRAD_OUT_CTRL_HOR_AND_VER = 0x0,
	IVE_NORM_GRAD_OUT_CTRL_HOR = 0x1,
	IVE_NORM_GRAD_OUT_CTRL_VER = 0x2,
	IVE_NORM_GRAD_OUT_CTRL_COMBINE = 0x3,

	IVE_NORM_GRAD_OUT_CTRL_BUTT
} IVE_NORM_GRAD_OUT_CTRL_E;

typedef struct IVE_NORM_GRAD_CTRL {
	IVE_NORM_GRAD_OUT_CTRL_E enOutCtrl;
	IVE_MAG_DIST_E enDistCtrl;
	IVE_ITC_TYPE_E enITCType;
	CVI_U8 u8MaskSize;
} IVE_NORM_GRAD_CTRL_S;

typedef enum _IVE_LK_OPTICAL_FLOW_PYR_OUT_MODE_E {
	IVE_LK_OPTICAL_FLOW_PYR_OUT_MODE_NONE = 0, /*Output none*/
	IVE_LK_OPTICAL_FLOW_PYR_OUT_MODE_STATUS = 1, /*Output status*/
	IVE_LK_OPTICAL_FLOW_PYR_OUT_MODE_BOTH = 2, /*Output status and err*/

	IVE_LK_OPTICAL_FLOW_PYR_OUT_MODE_BUTT
} IVE_LK_OPTICAL_FLOW_PYR_OUT_MODE_E;

typedef struct _IVE_LK_OPTICAL_FLOW_PYR_CTRL_S {
	IVE_LK_OPTICAL_FLOW_PYR_OUT_MODE_E enOutMode;
	bool bUseInitFlow; /*where to use initial flow*/
	unsigned short u16PtsNum; /*Number of the feature points,<=500*/
	unsigned char u8MaxLevel; /*0<=u8MaxLevel<=3*/
	unsigned char u0q8MinEigThr; /*Minimum eigenvalue threshold*/
	unsigned char u8IterCnt; /*Maximum iteration times, <=20*/
	unsigned char u0q8Eps; /*Used for exit criteria: dx^2 + dy^2 < u0q8Eps */
} IVE_LK_OPTICAL_FLOW_PYR_CTRL_S;

typedef struct _IVE_ST_MAX_EIG_S {
	unsigned short u16MaxEig; /*S_-Tomasi second step output MaxEig*/
	unsigned char u8Reserved[14]; /*For 16 byte align*/
} IVE_ST_MAX_EIG_S;

typedef struct _IVE_ST_CANDI_CORNER_CTRL_S {
	IVE_MEM_INFO_S stMem;
	unsigned char u0q8QualityLevel;
} IVE_ST_CANDI_CORNER_CTRL_S;

typedef struct _IVE_ST_CORNER_INFO_S {
	unsigned short u16CornerNum;
	IVE_POINT_U16_S astCorner[IVE_ST_MAX_CORNER_NUM];
} IVE_ST_CORNER_INFO_S;

typedef struct _IVE_ST_CORNER_CTRL_S {
	unsigned short u16MaxCornerNum;
	unsigned short u16MinDist;
} IVE_ST_CORNER_CTRL_S;

typedef enum _IVE_GRAD_FG_MODE_E {
	IVE_GRAD_FG_MODE_USE_CUR_GRAD = 0x0,
	IVE_GRAD_FG_MODE_FIND_MIN_GRAD = 0x1,

	IVE_GRAD_FG_MODE_BUTT
} IVE_GRAD_FG_MODE_E;

typedef struct _IVE_GRAD_FG_CTRL_S {
	IVE_GRAD_FG_MODE_E enMode; /*Calculation mode*/
	unsigned short u16EdwFactor;
	/*Edge width adjustment factor (range: 500 to 2000; default: 1000)*/
	unsigned char u8CrlCoefThr;
	/*Gradient vector correlation coefficient threshold (ranges: 50 to*/
	/*100; default: 80)*/
	unsigned char u8MagCrlThr;
	/*Gradient amplitude threshold (range: 0 to 20; default: 4)*/
	unsigned char u8MinMagDiff;
	/*Gradient magnitude difference threshold (range: 2 to 8; default: 2)*/
	unsigned char u8NoiseVal;
	/*Gradient amplitude noise threshold (range: 1 to 8; default: 1)*/
	unsigned char u8EdwDark;
	/*Black pixels enable flag (range: 0 (no), 1 (yes); default: 1)*/
} IVE_GRAD_FG_CTRL_S;

typedef struct _IVE_CANDI_BG_PIX_S {
	unsigned char u8q4f4Mean; /*Candidate background grays value */
	unsigned short u16StartTime; /*Candidate Background start time */
	unsigned short u16SumAccessTime;
	/*Candidate Background cumulative access time */
	unsigned short u16ShortKeepTime;
	/*Candidate background short hold time*/
	unsigned char u8ChgCond;
	/*Time condition for candidate background into the changing state*/
	unsigned char u8PotenBgLife;
	/*Potential background cumulative access time */
} IVE_CANDI_BG_PIX_S;

typedef struct _IVE_WORK_BG_PIX_S {
	unsigned char u8q4f4Mean; /*0# background grays value */
	unsigned short u16AccTime; /*Background cumulative access time */
	unsigned char u8PreGray; /*Gray value of last pixel */
	unsigned char u5q3DiffThr; /*Differential threshold */
	unsigned char u8AccFlag; /*Background access flag */
	unsigned char u8BgGray[3]; /*1# ~ 3# background grays value */
} IVE_WORK_BG_PIX_S;

typedef enum IVE_ORD_STAT_FILTER_MODE {
	IVE_ORD_STAT_FILTER_MODE_MAX = 0x0,
	IVE_ORD_STAT_FILTER_MODE_MIN = 0x1,

	IVE_ORD_STAT_FILTER_MODE_BUTT
} IVE_ORD_STAT_FILTER_MODE_E;

typedef struct IVE_ORD_STAT_FILTER_CTRL {
	IVE_ORD_STAT_FILTER_MODE_E enMode;
} IVE_ORD_STAT_FILTER_CTRL_S;

typedef enum _IVE_MAP_MODE_E {
	IVE_MAP_MODE_U8 = 0x0,
	IVE_MAP_MODE_S16 = 0x1,
	IVE_MAP_MODE_U16 = 0x2,

	IVE_MAP_MODE_BUTT
} IVE_MAP_MODE_E;

typedef struct _IVE_MAP_CTRL_S {
	IVE_MAP_MODE_E enMode;
} IVE_MAP_CTRL_S;

typedef struct _IVE_BG_LIFE_S {
	unsigned char u8WorkBgLife[3]; /*1# ~ 3# background vitality */
	unsigned char u8CandiBgLife; /*Candidate background vitality */
} IVE_BG_LIFE_S;

typedef struct _IVE_BG_MODEL_PIX_S {
	IVE_WORK_BG_PIX_S stWorkBgPixel; /*Working background */
	IVE_CANDI_BG_PIX_S stCandiPixel; /*Candidate background */
	IVE_BG_LIFE_S stBgLife; /*Background vitality */
} IVE_BG_MODEL_PIX_S;

typedef struct _IVE_FG_STAT_DATA_S {
	unsigned int u32PixNum;
	unsigned int u32SumLum;
	unsigned char u8Reserved[8];
} IVE_FG_STAT_DATA_S;

typedef struct _IVE_BG_STAT_DATA_S {
	unsigned int u32PixNum;
	unsigned int u32SumLum;
	unsigned char u8Reserved[8];
} IVE_BG_STAT_DATA_S;

typedef struct _IVE_MATCH_BG_MODEL_CTRL_S {
	unsigned int u32CurFrmNum; /*Current frame timestamp, in frame units */
	unsigned int u32PreFrmNum; /*Previous frame timestamp, in frame units */
	unsigned short u16TimeThr;
	/*Potential background replacement time threshold (range: 2 to 100*/
	/*frames; default: 20) */

	unsigned char u8DiffThrCrlCoef;
	/*Correlation coefficients between differential threshold and*/
	/*gray value (range: 0 to 5; default: 0) */
	unsigned char u8DiffMaxThr;
	/*Maximum of background differential threshold (range: 3 to 15; default: 6) */
	unsigned char u8DiffMinThr;
	/*Minimum of background differential threshold (range: 3 to 15; default: 4) */
	unsigned char u8DiffThrInc;
	/*Dynamic Background differential threshold increment (range: 0 to*/
	/*6; default: 0) */
	unsigned char u8FastLearnRate;
	/*Quick background learning rate (range: 0 to 4; default: 2) */
	unsigned char u8DetChgRegion;
	/*Whether to detect change region (range: 0 (no), 1 (yes); default: 0) */
} IVE_MATCH_BG_MODEL_CTRL_S;

typedef struct _IVE_UPDATE_BG_MODEL_CTRL_S {
	unsigned int u32CurFrmNum;
	/*Current frame timestamp, in frame units */
	unsigned int u32PreChkTime;
	/*The last time when background status is checked */
	unsigned int u32FrmChkPeriod;
	/*Background status checking period (range: 0 to 2000 frames;*/
	/*default: 50) */

	unsigned int u32InitMinTime;
	/*Background initialization shortest time (range: 20 to 6000*/
	/*frames;       default: 100)*/
	unsigned int u32StyBgMinBlendTime;
	/*Steady background integration shortest time (range: 20 to*/
	/*6000 frames; default: 200)*/
	unsigned int u32StyBgMaxBlendTime;
	/*Steady background integration longest time (range: 20 to*/
	/*40000 frames; default: 1500)*/
	unsigned int u32DynBgMinBlendTime;
	/*Dynamic background integration shortest time (range: 0 to*/
	/*6000 frames; default: 0)*/
	unsigned int u32StaticDetMinTime;
	/*Still detection shortest time (range: 20 to 6000 frames;*/
	/*default: 80)*/
	unsigned short u16FgMaxFadeTime;
	/*Foreground disappearing longest time (range: 1 to 255*/
	/*seconds;   default: 15)*/
	unsigned short u16BgMaxFadeTime;
	/*Background disappearing longest time (range: 1 to 255 seconds*/
	/*; default: 60)*/

	unsigned char u8StyBgAccTimeRateThr;
	/*Steady background access time ratio threshold (range: 10*/
	/*to 100; default: 80)*/
	unsigned char u8ChgBgAccTimeRateThr;
	/*Change background access time ratio threshold (range: 10*/
	/*to 100; default: 60)*/
	unsigned char u8DynBgAccTimeThr;
	/*Dynamic background access time ratio threshold (range: 0 to */
	/*50; default: 0)*/
	unsigned char u8DynBgDepth;
	/*Dynamic background depth (range: 0 to 3; default: 3)*/
	unsigned char u8BgEffStaRateThr;
	/*Background state time ratio threshold when initializing */
	/*(range: 90 to 100; default: 90)*/

	unsigned char u8AcceBgLearn;
	/*Whether to accelerate background learning (range: 0 (no), */
	/*1 (yes); default: 0)*/
	unsigned char u8DetChgRegion;
	/*Whether to detect change region (range: 0 (no), 1 (yes); default: 0)*/
} IVE_UPDATE_BG_MODEL_CTRL_S;

typedef enum _IVE_ANN_MLP_ACTIV_FUNC_E {
	IVE_ANN_MLP_ACTIV_FUNC_IDENTITY = 0x0,
	IVE_ANN_MLP_ACTIV_FUNC_SIGMOID_SYM = 0x1,
	IVE_ANN_MLP_ACTIV_FUNC_GAUSSIAN = 0x2,

	IVE_ANN_MLP_ACTIV_FUNC_BUTT
} IVE_ANN_MLP_ACTIV_FUNC_E;

typedef enum _IVE_ANN_MLP_ACCURATE_E {
	IVE_ANN_MLP_ACCURATE_SRC16_WGT16 = 0x0,
	/*input decimals' accurate 16 bit, weight 16bit*/
	IVE_ANN_MLP_ACCURATE_SRC14_WGT20 = 0x1,
	/*input decimals' accurate 14 bit, weight 20bit*/

	IVE_ANN_MLP_ACCURATE_BUTT
} IVE_ANN_MLP_ACCURATE_E;

typedef struct _IVE_ANN_MLP_MODEL_S {
	IVE_ANN_MLP_ACTIV_FUNC_E enActivFunc;
	IVE_ANN_MLP_ACCURATE_E enAccurate;
	IVE_MEM_INFO_S stWeight;
	unsigned int u32TotalWeightSize;

	unsigned short au16LayerCount[8];
	/*8 layers, including input and output layer*/
	unsigned short u16MaxCount; /*MaxCount<=1024*/
	unsigned char u8LayerNum; /*2<layerNum<=8*/
	unsigned char u8Reserved;
} IVE_ANN_MLP_MODEL_S;

typedef enum _IVE_SVM_TYPE_E {
	IVE_SVM_TYPE_C_SVC = 0x0,
	IVE_SVM_TYPE_NU_SVC = 0x1,

	IVE_SVM_TYPE_BUTT
} IVE_SVM_TYPE_E;

typedef enum _IVE_SVM_KERNEL_TYPE_E {
	IVE_SVM_KERNEL_TYPE_LINEAR = 0x0,
	IVE_SVM_KERNEL_TYPE_POLY = 0x1,
	IVE_SVM_KERNEL_TYPE_RBF = 0x2,
	IVE_SVM_KERNEL_TYPE_SIGMOID = 0x3,

	IVE_SVM_KERNEL_TYPE_BUTT
} IVE_SVM_KERNEL_TYPE_E;

typedef struct _IVE_SVM_MODEL_S {
	IVE_SVM_TYPE_E enType;
	IVE_SVM_KERNEL_TYPE_E enKernelType;

	IVE_MEM_INFO_S stSv; /*SV memory*/
	IVE_MEM_INFO_S stDf; /*Decision functions memory*/
	unsigned int u32TotalDfSize; /*All decision functions coef size in byte*/

	unsigned short u16FeatureDim;
	unsigned short u16SvTotal;
	unsigned char u8ClassCount;
} IVE_SVM_MODEL_S;

/*
 * Sad mode
 */
typedef enum IVE_SAD_MODE {
	IVE_SAD_MODE_MB_4X4 = 0x0,
	IVE_SAD_MODE_MB_8X8 = 0x1,
	IVE_SAD_MODE_MB_16X16 = 0x2,

	IVE_SAD_MODE_BUTT
} IVE_SAD_MODE_E;
/*
 *Sad output ctrl
 */
typedef enum IVE_SAD_OUT_CTRL {
	IVE_SAD_OUT_CTRL_16BIT_BOTH = 0x0,
	IVE_SAD_OUT_CTRL_8BIT_BOTH = 0x1,
	IVE_SAD_OUT_CTRL_16BIT_SAD = 0x2,
	IVE_SAD_OUT_CTRL_8BIT_SAD = 0x3,
	IVE_SAD_OUT_CTRL_THRESH = 0x4,

	IVE_SAD_OUT_CTRL_BUTT
} IVE_SAD_OUT_CTRL_E;
/*
 * Sad ctrl param
 */
typedef struct IVE_SAD_CTRL {
	IVE_SAD_MODE_E enMode;
	IVE_SAD_OUT_CTRL_E enOutCtrl;
	CVI_U16 u16Thr;
	CVI_U8 u8MinVal;
	CVI_U8 u8MaxVal;
} IVE_SAD_CTRL_S;

typedef enum IVE_SOBEL_OUT_CTRL {
	IVE_SOBEL_OUT_CTRL_BOTH = 0x0,
	IVE_SOBEL_OUT_CTRL_HOR = 0x1,
	IVE_SOBEL_OUT_CTRL_VER = 0x2,
	IVE_SOBEL_OUT_CTRL_BUTT
} IVE_SOBEL_OUT_CTRL_E;

typedef struct IVE_SOBEL_CTRL {
	IVE_SOBEL_OUT_CTRL_E enOutCtrl;
	CVI_U8 u8MaskSize;
	CVI_S8 as8Mask[25];
} IVE_SOBEL_CTRL_S;

typedef enum IVE_SUB_MODE_E {
	IVE_SUB_MODE_NORMAL = 0x0,
	IVE_SUB_MODE_ABS = 0x1,
	IVE_SUB_MODE_BUTT
} IVE_SUB_MODE_E;

typedef struct IVE_SUB_CTRL {
	IVE_SUB_MODE_E enMode;
} IVE_SUB_CTRL_S;

typedef enum IVE_THRESH_MODE {
	IVE_THRESH_MODE_BINARY,
	IVE_THRESH_MODE_SLOPE
} IVE_THRESH_MODE_E;

typedef struct IVE_THRESH_CTRL {
	CVI_U32 enMode;
	CVI_U8 u8MinVal;
	CVI_U8 u8MaxVal;
	CVI_U8 u8LowThr;
} IVE_THRESH_CTRL_S;

typedef enum hiIVE_THRESH_S16_MODE_E {
	IVE_THRESH_S16_MODE_S16_TO_S8_MIN_MID_MAX = 0x0,
	IVE_THRESH_S16_MODE_S16_TO_S8_MIN_ORI_MAX = 0x1,
	IVE_THRESH_S16_MODE_S16_TO_U8_MIN_MID_MAX = 0x2,
	IVE_THRESH_S16_MODE_S16_TO_U8_MIN_ORI_MAX = 0x3,

	IVE_THRESH_S16_MODE_BUTT
} IVE_THRESH_S16_MODE_E;

typedef union IVE_8BIT {
	CVI_S8 s8Val;
	CVI_U8 u8Val;
} IVE_8BIT_U;

typedef struct hiIVE_THRESH_S16_CTRL {
	IVE_THRESH_S16_MODE_E enMode;
	CVI_S16 s16LowThr;
	CVI_S16 s16HighThr;
	IVE_8BIT_U un8MinVal;
	IVE_8BIT_U un8MidVal;
	IVE_8BIT_U un8MaxVal;
} IVE_THRESH_S16_CTRL_S;

typedef enum IVE_THRESH_U16_MODE {
	IVE_THRESH_U16_MODE_U16_TO_U8_MIN_MID_MAX = 0x0,
	IVE_THRESH_U16_MODE_U16_TO_U8_MIN_ORI_MAX = 0x1,

	IVE_THRESH_U16_MODE_BUTT
} IVE_THRESH_U16_MODE_E;

typedef struct IVE_THRESH_U16_CTRL {
	IVE_THRESH_U16_MODE_E enMode;
	CVI_U16 u16LowThr;
	CVI_U16 u16HighThr;
	CVI_U8 u8MinVal;
	CVI_U8 u8MidVal;
	CVI_U8 u8MaxVal;
} IVE_THRESH_U16_CTRL_S;

// for cpu version

typedef enum IVE_CC_DIR { DIRECTION_4 = 0x0, DIRECTION_8 = 0x1 } IVE_CC_DIR_E;

typedef struct IVE_CC_CTRL {
	IVE_CC_DIR_E enMode;
} IVE_CC_CTRL_S;

// integral image
typedef enum cviIVE_INTEG_OUT_CTRL_E {
	IVE_INTEG_OUT_CTRL_COMBINE = 0x0,
	IVE_INTEG_OUT_CTRL_SUM = 0x1,
	IVE_INTEG_OUT_CTRL_SQSUM = 0x2,
	IVE_INTEG_OUT_CTRL_BUTT
} IVE_INTEG_OUT_CTRL_E;

typedef struct cviIVE_INTEG_CTRL_S {
	IVE_INTEG_OUT_CTRL_E enOutCtrl;
} IVE_INTEG_CTRL_S;

typedef struct cviIVE_EQUALIZE_HIST_CTRL_S {
	IVE_MEM_INFO_S stMem;
} IVE_EQUALIZE_HIST_CTRL_S;

typedef IVE_MEM_INFO_S IVE_DST_MEM_INFO_S;

typedef enum cviIVE_16BIT_TO_8BIT_MODE_E {
	IVE_16BIT_TO_8BIT_MODE_S16_TO_S8 = 0x0,
	IVE_16BIT_TO_8BIT_MODE_S16_TO_U8_ABS = 0x1,
	IVE_16BIT_TO_8BIT_MODE_S16_TO_U8_BIAS = 0x2,
	IVE_16BIT_TO_8BIT_MODE_U16_TO_U8 = 0x3,
	IVE_16BIT_TO_8BIT_MODE_BUTT
} IVE_16BIT_TO_8BIT_MODE_E;

typedef struct cviIVE_16BIT_TO_8BIT_CTRL_S {
	IVE_16BIT_TO_8BIT_MODE_E enMode;
	CVI_U16 u16Denominator;
	CVI_U8 u8Numerator;
	CVI_S8 s8Bias;
} IVE_16BIT_TO_8BIT_CTRL_S;

typedef struct cviIVE_NCC_DST_MEM_S {
	CVI_U64 u64Numerator;
	CVI_U64 u64QuadSum1;
	CVI_U64 u64QuadSum2;
	CVI_U64 u8Reserved[8];
} IVE_NCC_DST_MEM_S;

typedef struct _IVE_REGION_S {
	unsigned int u32Area; /*Represented by the pixel number*/
	unsigned short u16Left; /*Circumscribed rectangle left border*/
	unsigned short u16Right; /*Circumscribed rectangle right border*/
	unsigned short u16Top; /*Circumscribed rectangle top border*/
	unsigned short u16Bottom; /*Circumscribed rectangle bottom border*/
} IVE_REGION_S;

typedef struct _IVE_CCBLOB_S {
	unsigned short u16CurAreaThr; /*Threshold of the result regions' area*/
	char s8LabelStatus; /*-1: Labeled failed ; 0: Labeled successfully*/
	unsigned char u8RegionNum;
	/*Number of valid region, non-continuous stored*/
	IVE_REGION_S
	astRegion[IVE_MAX_REGION_NUM]; /*Valid regions with 'u32Area>0' and 'label = ArrayIndex+1'*/
} IVE_CCBLOB_S;

typedef enum _IVE_CCL_MODE_E {
	IVE_CCL_MODE_4C = 0x0, /*4-connected*/
	IVE_CCL_MODE_8C = 0x1, /*8-connected*/

	IVE_CCL_MODE_BUTT
} IVE_CCL_MODE_E;

typedef struct _IVE_CCL_CTRL_S {
	IVE_CCL_MODE_E enMode; /*Mode*/
	unsigned short u16InitAreaThr; /*Init threshold of region area*/
	unsigned short u16Step; /*Increase area step for once*/
} IVE_CCL_CTRL_S;

typedef struct _IVE_GMM_CTRL_S {
	unsigned int u22q10NoiseVar; /*Initial noise Variance*/
	unsigned int u22q10MaxVar; /*Max  Variance*/
	unsigned int u22q10MinVar; /*Min  Variance*/
	unsigned short u0q16LearnRate; /*Learning rate*/
	unsigned short u0q16BgRatio; /*Background ratio*/
	unsigned char u8q8VarThr; /*Variance Threshold*/
	unsigned short u0q16InitWeight; /*Initial Weight*/
	unsigned char u8ModelNum; /*Model number: 3 or 5*/
} IVE_GMM_CTRL_S;

typedef enum _IVE_GMM2_SNS_FACTOR_MODE_E {
	IVE_GMM2_SNS_FACTOR_MODE_GLB = 0x0, /*Global sensitivity factor mode*/
	IVE_GMM2_SNS_FACTOR_MODE_PIX = 0x1, /*Pixel sensitivity factor mode*/

	IVE_GMM2_SNS_FACTOR_MODE_BUTT
} IVE_GMM2_SNS_FACTOR_MODE_E;

typedef enum _IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_E {
	IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_GLB = 0x0,
	/*Global life update factor mode*/
	IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_PIX = 0x1,
	/*Pixel life update factor mode*/

	IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_BUTT
} IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_E;

typedef struct _IVE_GMM2_CTRL_S {
	IVE_GMM2_SNS_FACTOR_MODE_E enSnsFactorMode; /*Sensitivity factor mode*/
	IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_E enLifeUpdateFactorMode;
	/*Life update factor mode*/
	unsigned short u16GlbLifeUpdateFactor;
	/*Global life update factor (default: 4)*/
	unsigned short u16LifeThr; /*Life threshold (default: 5000)*/
	unsigned short u16FreqInitVal; /*Initial frequency (default: 20000)*/
	unsigned short u16FreqReduFactor;
	/*Frequency reduction factor (default: 0xFF00)*/
	unsigned short u16FreqAddFactor;
	/*Frequency adding factor (default: 0xEF)*/
	unsigned short u16FreqThr; /*Frequency threshold (default: 12000)*/
	unsigned short u16VarRate; /*Variation update rate (default: 1)*/
	unsigned short u9q7MaxVar; /*Max variation (default: (16 * 16)<<7)*/
	unsigned short u9q7MinVar; /*Min variation (default: ( 8 *  8)<<7)*/
	unsigned char u8GlbSnsFactor; /*Global sensitivity factor (default: 8)*/
	unsigned char u8ModelNum; /*Model number (range: 1~5, default: 3)*/
} IVE_GMM2_CTRL_S;

typedef struct _IVE_CANNY_HYS_EDGE_CTRL_S {
	IVE_MEM_INFO_S stMem;
	unsigned short u16LowThr;
	unsigned short u16HighThr;
	char as8Mask[25];
} IVE_CANNY_HYS_EDGE_CTRL_S;

typedef struct _IVE_CANNY_STACK_SIZE_S {
	unsigned int u32StackSize; /*Stack size for output*/
	unsigned char u8Reserved[12]; /*For 16 byte align*/
} IVE_CANNY_STACK_SIZE_S;

typedef enum cviIVE_LBP_CMP_MODE_E {
	IVE_LBP_CMP_MODE_NORMAL = 0x0,
	/* P(x)-P(center)>= un8BitThr.s8Val, s(x)=1; else s(x)=0; */
	IVE_LBP_CMP_MODE_ABS = 0x1,
	/* abs(P(x)- P(center))>=un8BitThr.u8Val, s(x)=1; else s(x)=0; */
	IVE_LBP_CMP_MODE_BUTT
} IVE_LBP_CMP_MODE_E;

typedef struct cviIVE_LBP_CTRL_S {
	IVE_LBP_CMP_MODE_E enMode;
	IVE_8BIT_U un8BitThr;
} IVE_LBP_CTRL_S;

// csc/resize

typedef enum cviIVE_CSC_MODE_E {
	/*CSC: YUV2RGB, video transfer mode, RGB value range [16, 235]*/
	// IVE_CSC_MODE_VIDEO_BT601_YUV2RGB = 0x0,
	/*CSC: YUV2RGB, video transfer mode, RGB value range [16, 235]*/
	// IVE_CSC_MODE_VIDEO_BT709_YUV2RGB = 0x1,
	/*CSC: YUV2RGB, picture transfer mode, RGB value range [0, 255]*/
	// IVE_CSC_MODE_PIC_BT601_YUV2RGB = 0x2,
	/*CSC: YUV2RGB, picture transfer mode, RGB value range [0, 255]*/
	// IVE_CSC_MODE_PIC_BT709_YUV2RGB = 0x3,
	/*CSC: YUV2HSV, picture transfer mode, HSV value range [0, 255]*/
	// IVE_CSC_MODE_PIC_BT601_YUV2HSV = 0x4,
	/*CSC: YUV2HSV, picture transfer mode, HSV value range [0, 255]*/
	// IVE_CSC_MODE_PIC_BT709_YUV2HSV = 0x5,
	/*CSC: YUV2LAB, picture transfer mode, Lab value range [0, 255]*/
	// IVE_CSC_MODE_PIC_BT601_YUV2LAB = 0x6,
	/*CSC: YUV2LAB, picture transfer mode, Lab value range [0, 255]*/
	// IVE_CSC_MODE_PIC_BT709_YUV2LAB = 0x7,
	/*CSC: RGB2YUV, video transfer mode, YUV value range [0, 255]*/
	// IVE_CSC_MODE_VIDEO_BT601_RGB2YUV = 0x8,
	/*CSC: RGB2YUV, video transfer mode, YUV value range [0, 255]*/
	// IVE_CSC_MODE_VIDEO_BT709_RGB2YUV = 0x9,
	/*CSC: RGB2YUV, picture transfer mode, Y:[16, 235],U\V:[16, 240]*/
	// IVE_CSC_MODE_PIC_BT601_RGB2YUV = 0xa,
	/*CSC: RGB2YUV, picture transfer mode, Y:[16, 235],U\V:[16, 240]*/
	// IVE_CSC_MODE_PIC_BT709_RGB2YUV = 0xb,

	IVE_CSC_MODE_PIC_RGB2HSV = 0xb,
	IVE_CSC_MODE_PIC_RGB2GRAY = 0xc,

	// IVE_CSC_MODE_BUTT
} IVE_CSC_MODE_E;

typedef struct cviIVE_CSC_CTRL_S {
	IVE_CSC_MODE_E enMode; /*Working mode*/
} IVE_CSC_CTRL_S;

typedef enum cviIVE_RESIZE_MODE_E {
	IVE_RESIZE_MODE_LINEAR = 0x0, /*Bilinear interpolation*/
	IVE_RESIZE_MODE_AREA = 0x1, /*Area-based (or super) interpolation*/
	IVE_RESIZE_MODE_BUTT
} IVE_RESIZE_MODE_E;

typedef struct cviIVE_RESIZE_CTRL_S {
	IVE_RESIZE_MODE_E enMode;
	IVE_MEM_INFO_S stMem;
	CVI_U16 u16Num;
} IVE_RESIZE_CTRL_S;

typedef enum _IVE_CNN_ACTIV_FUNC_E {
	IVE_CNN_ACTIV_FUNC_NONE = 0x0,
	/*Do not taking a activation, equivalent f(x)=x*/
	IVE_CNN_ACTIV_FUNC_RELU = 0x1, /*f(x)=max(0, x)*/
	IVE_CNN_ACTIV_FUNC_SIGMOID = 0x2, /*f(x)=1/(1+exp(-x)), not support*/

	IVE_CNN_ACTIV_FUNC_BUTT
} IVE_CNN_ACTIV_FUNC_E;

typedef enum _IVE_CNN_POOLING_E {
	IVE_CNN_POOLING_NONE = 0x0, /*Do not taking a pooling action*/
	IVE_CNN_POOLING_MAX = 0x1, /*Using max value of every pooling area*/
	IVE_CNN_POOLING_AVG = 0x2, /*Using average value of every pooling area*/

	IVE_CNN_POOLING_BUTT
} IVE_CNN_POOLING_E;

typedef struct _IVE_CNN_CONV_POOLING_S {
	IVE_CNN_ACTIV_FUNC_E enActivFunc; /*Type of activation function*/
	IVE_CNN_POOLING_E enPooling; /*Mode of pooling method*/

	unsigned char u8FeatureMapNum; /*Number of feature maps*/
	unsigned char u8KernelSize; /*Kernel size, only support 3 currently*/
	unsigned char u8ConvStep; /*Convolution step, only support 1 currently*/

	unsigned char u8PoolSize; /*Pooling size, only support 2 currently*/
	unsigned char u8PoolStep; /*Pooling step, only support 2 currently*/
	unsigned char u8Reserved[3];

} IVE_CNN_CONV_POOLING_S;

typedef struct _IVE_CNN_FULL_CONNECT_S {
	unsigned short au16LayerCnt[8];
	/*Neuron number of every fully connected layers*/
	unsigned short u16MaxCnt;
	/*Max neuron number in all fully connected layers*/
	unsigned char u8LayerNum; /*Number of fully connected layer*/
	unsigned char u8Reserved;
} IVE_CNN_FULL_CONNECT_S;

typedef struct _IVE_CNN_MODEL_S {
	IVE_CNN_CONV_POOLING_S astConvPool[8]; /*Conv-ReLU-Pooling layers info*/
	IVE_CNN_FULL_CONNECT_S stFullConnect; /*Fully connected layers info*/

	IVE_MEM_INFO_S stConvKernelBias;
	/*Conv-ReLU-Pooling layers' kernels and bias*/
	unsigned int u32ConvKernelBiasSize;
	/*Size of Conv-ReLU-Pooling layer' kernels and bias*/

	IVE_MEM_INFO_S stFCLWgtBias; /*Fully Connection Layers' weights and bias*/
	unsigned int u32FCLWgtBiasSize;
	/*Size of fully connection layers weights and bias*/

	unsigned int u32TotalMemSize;
	/*Total memory size of all kernels, weights, bias*/

	IVE_IMAGE_TYPE_E enType; /*Image type used for the CNN model*/
	unsigned int u32Width; /*Image width used for the model*/
	unsigned int u32Height; /*Image height used for the model*/

	unsigned short u16ClassCount; /*Number of classes*/
	unsigned char u8ConvPoolLayerNum; /*Number of Conv-ReLU-Pooling layers*/
	unsigned char u8Reserved;
} IVE_CNN_MODEL_S;

typedef struct _IVE_CNN_CTRL_S {
	IVE_MEM_INFO_S stMem; /*Assist memory*/
	unsigned int u32Num; /*Input image number*/
} IVE_CNN_CTRL_S;

typedef struct _IVE_CNN_RESULT_S {
	int s32ClassIdx; /*The most possible index of the classification*/
	int s32Confidence; /*The confidence of the classification*/
} IVE_CNN_RESULT_S;

typedef enum _IVE_BERNSEN_MODE_E {
	IVE_BERNSEN_MODE_NORMAL = 0x0, /*Simple Bernsen thresh*/
	IVE_BERNSEN_MODE_THRESH =
		0x1, /*Thresh based on the global threshold and local Bernsen threshold*/
	IVE_BERNSEN_MODE_PAPER = 0x2,
	/*This method is same with original paper*/
	IVE_BERNSEN_MODE_BUTT
} IVE_BERNSEN_MODE_E;

typedef struct _IVE_BERNSEN_CTRL_S {
	IVE_BERNSEN_MODE_E enMode;
	unsigned char u8WinSize; /* 3x3 or 5x5 */
	unsigned char u8Thr;
	unsigned char u8ContrastThreshold; // compare with midgray
} IVE_BERNSEN_CTRL_S;

typedef struct cviIVE_FILTER_AND_CSC_CTRL_S {
	IVE_CSC_MODE_E enMode; /*CSC working mode*/
	CVI_S8 as8Mask[25]; /*Template parameter filter coefficient*/
	CVI_U16 u16Norm; /*Normalization parameter, by right shift*/
} IVE_FILTER_AND_CSC_CTRL_S;

#endif // End of _CVI_COMM_IVE.h
