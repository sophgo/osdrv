/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_vcodec_proc_info.h
 * Description:
 */

#ifndef __VPU_DRV_PROC_INFO_H__
#define __VPU_DRV_PROC_INFO_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define CVI_TRUE	1
#define CVI_FALSE	0

#define ALIGN_NUM	4
#define ATTRIBUTE  __aligned(ALIGN_NUM)

#define JPEGD_MAX_WIDTH		(3840)
#define JPEGD_MAX_HEIGHT	(2160)

#define JPEG_MARKER_ORDER_CNT	16

typedef unsigned char           CVI_UCHAR;
typedef unsigned char           CVI_U8;
typedef unsigned short          CVI_U16;
typedef unsigned int            CVI_U32;
typedef unsigned int            CVI_HANDLE;

typedef signed char             CVI_S8;
typedef char                    CVI_CHAR;
typedef short                   CVI_S16;
typedef int                     CVI_S32;

typedef unsigned long           CVI_UL;
typedef signed long             CVI_SL;

typedef float                   CVI_FLOAT;
typedef double                  CVI_DOUBLE;

typedef void                    CVI_VOID;
typedef bool                    CVI_BOOL;

typedef uint64_t                CVI_U64;
typedef int64_t                 CVI_S64;

typedef size_t                  CVI_SIZE_T;


typedef struct _SIZE_S {
	CVI_U32 u32Width;
	CVI_U32 u32Height;
} SIZE_S;

typedef struct _RECT_S {
	CVI_S32 s32X;
	CVI_S32 s32Y;
	CVI_U32 u32Width;
	CVI_U32 u32Height;
} RECT_S;

/*the size of array is 2,that is the maximum*/
typedef struct _VENC_MPF_CFG_S {
	CVI_U8 u8LargeThumbNailNum; /* RW; Range:[0,2]; the large thumbnail pic num of the MPF */
	SIZE_S astLargeThumbNailSize[2]; /* RW; The resolution of large ThumbNail*/
} VENC_MPF_CFG_S;

typedef enum _VENC_PIC_RECEIVE_MODE_E {
	VENC_PIC_RECEIVE_SINGLE = 0,
	VENC_PIC_RECEIVE_MULTI,

	VENC_PIC_RECEIVE_BUTT
} VENC_PIC_RECEIVE_MODE_E;

/**
 * @brief Define the attributes of JPEG Encoder.
 *
 */
typedef struct _VENC_ATTR_JPEG_S {
	CVI_BOOL bSupportDCF;		///< TODO VENC
	VENC_MPF_CFG_S stMPFCfg;	///< TODO VENC
	VENC_PIC_RECEIVE_MODE_E enReceiveMode;	///< TODO VENC
} VENC_ATTR_JPEG_S;

/*the attribute of h264e*/
typedef struct _VENC_ATTR_H264_S {
	CVI_BOOL bRcnRefShareBuf; /* RW; Range:[0, 1]; Whether to enable the Share Buf of Rcn and Ref .*/
	CVI_BOOL bSingleLumaBuf; /* Use single luma buffer*/
	// reserved
} VENC_ATTR_H264_S;

/*the attribute of h265e*/
typedef struct _VENC_ATTR_H265_S {
	CVI_BOOL bRcnRefShareBuf; /* RW; Range:[0, 1]; Whether to enable the Share Buf of Rcn and Ref .*/
	// reserved
} VENC_ATTR_H265_S;

/*the frame rate of PRORES*/
typedef enum _PRORES_FRAMERATE {
	PRORES_FR_UNKNOWN = 0,
	PRORES_FR_23_976,
	PRORES_FR_24,
	PRORES_FR_25,
	PRORES_FR_29_97,
	PRORES_FR_30,
	PRORES_FR_50,
	PRORES_FR_59_94,
	PRORES_FR_60,
	PRORES_FR_100,
	PRORES_FR_119_88,
	PRORES_FR_120,
	PRORES_FR_BUTT
} PRORES_FRAMERATE;

/*the aspect ratio of PRORES*/
typedef enum _PRORES_ASPECT_RATIO {
	PRORES_ASPECT_RATIO_UNKNOWN = 0,
	PRORES_ASPECT_RATIO_SQUARE,
	PRORES_ASPECT_RATIO_4_3,
	PRORES_ASPECT_RATIO_16_9,
	PRORES_ASPECT_RATIO_BUTT
} PRORES_ASPECT_RATIO;

/*the attribute of PRORES*/
typedef struct _VENC_ATTR_PRORES_S {
	char cIdentifier[4];
	PRORES_FRAMERATE enFrameRateCode;
	PRORES_ASPECT_RATIO enAspectRatio;
} VENC_ATTR_PRORES_S;

/* We just copy this value of payload type from RTP/RTSP definition */
typedef enum {
	PT_PCMU          = 0,
	PT_1016          = 1,
	PT_G721          = 2,
	PT_GSM           = 3,
	PT_G723          = 4,
	PT_DVI4_8K       = 5,
	PT_DVI4_16K      = 6,
	PT_LPC           = 7,
	PT_PCMA          = 8,
	PT_G722          = 9,
	PT_S16BE_STEREO  = 10,
	PT_S16BE_MONO    = 11,
	PT_QCELP         = 12,
	PT_CN            = 13,
	PT_MPEGAUDIO     = 14,
	PT_G728          = 15,
	PT_DVI4_3        = 16,
	PT_DVI4_4        = 17,
	PT_G729          = 18,
	PT_G711A         = 19,
	PT_G711U         = 20,
	PT_G726          = 21,
	PT_G729A         = 22,
	PT_LPCM          = 23,
	PT_CelB          = 25,
	PT_JPEG          = 26,
	PT_CUSM          = 27,
	PT_NV            = 28,
	PT_PICW          = 29,
	PT_CPV           = 30,
	PT_H261          = 31,
	PT_MPEGVIDEO     = 32,
	PT_MPEG2TS       = 33,
	PT_H263          = 34,
	PT_SPEG          = 35,
	PT_MPEG2VIDEO    = 36,
	PT_AAC           = 37,
	PT_WMA9STD       = 38,
	PT_HEAAC         = 39,
	PT_PCM_VOICE     = 40,
	PT_PCM_AUDIO     = 41,
	PT_MP3           = 43,
	PT_ADPCMA        = 49,
	PT_AEC           = 50,
	PT_X_LD          = 95,
	PT_H264          = 96,
	PT_D_GSM_HR      = 200,
	PT_D_GSM_EFR     = 201,
	PT_D_L8          = 202,
	PT_D_RED         = 203,
	PT_D_VDVI        = 204,
	PT_D_BT656       = 220,
	PT_D_H263_1998   = 221,
	PT_D_MP1S        = 222,
	PT_D_MP2P        = 223,
	PT_D_BMPEG       = 224,
	PT_MP4VIDEO      = 230,
	PT_MP4AUDIO      = 237,
	PT_VC1           = 238,
	PT_JVC_ASF       = 255,
	PT_D_AVI         = 256,
	PT_DIVX3         = 257,
	PT_AVS             = 258,
	PT_REAL8         = 259,
	PT_REAL9         = 260,
	PT_VP6             = 261,
	PT_VP6F             = 262,
	PT_VP6A             = 263,
	PT_SORENSON          = 264,
	PT_H265          = 265,
	PT_VP8             = 266,
	PT_MVC             = 267,
	PT_PNG           = 268,
	/* add by ourselves */
	PT_AMR           = 1001,
	PT_MJPEG         = 1002,
	PT_BUTT
} PAYLOAD_TYPE_E;

/**
 * @brief Define the attributes of the venc.
 *
 */
typedef struct _VENC_ATTR_S {
	PAYLOAD_TYPE_E enType;		///< the type of payload

	CVI_U32 u32MaxPicWidth;		///< maximum width of a picture to be encoded
	CVI_U32 u32MaxPicHeight;	///< maximum height of a picture to be encoded

	CVI_U32 u32BufSize;			///< size of encoded bitstream buffer
	CVI_U32 u32Profile;			///< TODO VENC
	CVI_BOOL bByFrame;			///< mode of collecting encoded bitstream\n
								///< CVI_TRUE: frame-based\n CVI_FALSE: packet-based
	CVI_U32 u32PicWidth;		///< width of a picture to be encoded
	CVI_U32 u32PicHeight;		///< height of a picture to be encoded
	CVI_BOOL bSingleCore;		///< Use single HW core
	CVI_BOOL bEsBufQueueEn;		///< Use es buffer queue
	CVI_BOOL bIsoSendFrmEn;		///< Isolating SendFrame/GetStream pairing
	union {
		VENC_ATTR_H264_S stAttrH264e;	///< TODO VENC
		VENC_ATTR_H265_S stAttrH265e;	///< TODO VENC
		VENC_ATTR_JPEG_S stAttrJpege;	///< the attibute of JPEG encoder
		VENC_ATTR_PRORES_S stAttrProres;///< TODO VENC
	};
} VENC_ATTR_S;

typedef unsigned int CVI_FR32;

/* rc mode */
typedef enum _VENC_RC_MODE_E {
	VENC_RC_MODE_H264CBR = 1,
	VENC_RC_MODE_H264VBR,
	VENC_RC_MODE_H264AVBR,
	VENC_RC_MODE_H264QVBR,
	VENC_RC_MODE_H264FIXQP,
	VENC_RC_MODE_H264QPMAP,
	VENC_RC_MODE_H264UBR,

	VENC_RC_MODE_MJPEGCBR,
	VENC_RC_MODE_MJPEGVBR,
	VENC_RC_MODE_MJPEGFIXQP,

	VENC_RC_MODE_H265CBR,
	VENC_RC_MODE_H265VBR,
	VENC_RC_MODE_H265AVBR,
	VENC_RC_MODE_H265QVBR,
	VENC_RC_MODE_H265FIXQP,
	VENC_RC_MODE_H265QPMAP,
	VENC_RC_MODE_H265UBR,

	VENC_RC_MODE_BUTT,

} VENC_RC_MODE_E;

/* the attribute of h264e fixqp*/
typedef struct _VENC_H264_FIXQP_S {
	CVI_U32 u32Gop; /* RW; Range:[1, 65536]; the interval of ISLICE. */
	CVI_U32 u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	CVI_FR32 fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	CVI_U32 u32IQp; ///< qp of the I frame, Range:[0, 51]
	CVI_U32 u32PQp; ///< qp of the P frame, Range:[0, 51]
	CVI_U32 u32BQp; ///< Not support
	CVI_BOOL bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} VENC_H264_FIXQP_S;

/* the attribute of h264e cbr*/
typedef struct _VENC_H264_CBR_S {
	CVI_U32 u32Gop; /* RW; Range:[1, 65536]; the interval of I Frame. */
	CVI_U32 u32StatTime; /* RW; Range:[1, 60]; the rate statistic time, the unit is senconds(s) */
	CVI_U32 u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	CVI_FR32 fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	CVI_U32 u32BitRate; /* RW; Range:[2, 409600]; average bitrate */
	CVI_BOOL bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} VENC_H264_CBR_S;

/* the attribute of h264e vbr*/
typedef struct _VENC_H264_VBR_S {
	CVI_U32 u32Gop; /* RW; Range:[1, 65536]; the interval of ISLICE. */
	CVI_U32 u32StatTime; /* RW; Range:[1, 60]; the rate statistic time, the unit is senconds(s) */
	CVI_U32 u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	CVI_FR32 fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	CVI_U32 u32MaxBitRate; /* RW; Range:[2, 409600];the max bitrate */
	CVI_BOOL bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} VENC_H264_VBR_S;

/* the attribute of h264e avbr*/
typedef struct _VENC_H264_AVBR_S {
	CVI_U32 u32Gop; /* RW; Range:[1, 65536]; the interval of ISLICE. */
	CVI_U32 u32StatTime; /* RW; Range:[1, 60]; the rate statistic time, the unit is senconds(s) */
	CVI_U32 u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	CVI_FR32 fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	CVI_U32 u32MaxBitRate; /* RW; Range:[2, 409600];the max bitrate */
	CVI_BOOL bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} VENC_H264_AVBR_S;

/* the attribute of h264e qpmap*/
typedef struct _VENC_H264_QPMAP_S {
	CVI_U32 u32Gop; /* RW; Range:[1, 65536]; the interval of ISLICE. */
	CVI_U32 u32StatTime; /* RW; Range:[1, 60]; the rate statistic time, the unit is senconds(s) */
	CVI_U32 u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	CVI_FR32 fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	CVI_BOOL bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} VENC_H264_QPMAP_S;

typedef struct _VENC_H264_QVBR_S {
	CVI_U32 u32Gop; /*the interval of ISLICE. */
	CVI_U32 u32StatTime; /* the rate statistic time, the unit is senconds(s) */
	CVI_U32 u32SrcFrameRate; /* the input frame rate of the venc channel */
	CVI_FR32 fr32DstFrameRate; /* the target frame rate of the venc channel */
	CVI_U32 u32TargetBitRate; /* the target bitrate */
	CVI_BOOL bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} VENC_H264_QVBR_S;

typedef struct _VENC_H264_UBR_S {
	CVI_U32 u32Gop; /* RW; Range:[1, 65536]; the interval of I Frame. */
	CVI_U32 u32StatTime; /* RW; Range:[1, 60]; the rate statistic time, the unit is senconds(s) */
	CVI_U32 u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	CVI_FR32 fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	CVI_U32 u32BitRate; /* RW; Range:[2, 409600]; average bitrate */
	CVI_BOOL bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} VENC_H264_UBR_S;

/* qpmap mode*/
typedef enum _VENC_RC_QPMAP_MODE_E {
	VENC_RC_QPMAP_MODE_MEANQP = 0,
	VENC_RC_QPMAP_MODE_MINQP,
	VENC_RC_QPMAP_MODE_MAXQP,

	VENC_RC_QPMAP_MODE_BUTT,
} VENC_RC_QPMAP_MODE_E;

/* the attribute of h265e qpmap*/
typedef struct _VENC_H265_QPMAP_S {
	CVI_U32 u32Gop; /* RW; Range:[1, 65536]; the interval of ISLICE. */
	CVI_U32 u32StatTime; /* RW; Range:[1, 60]; the rate statistic time, the unit is senconds(s) */
	CVI_U32 u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	CVI_FR32 fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	VENC_RC_QPMAP_MODE_E enQpMapMode; /* RW;  the QpMap Mode.*/
	CVI_BOOL bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} VENC_H265_QPMAP_S;

typedef struct _VENC_H264_CBR_S VENC_H265_CBR_S;
typedef struct _VENC_H264_VBR_S VENC_H265_VBR_S;
typedef struct _VENC_H264_AVBR_S VENC_H265_AVBR_S;
typedef struct _VENC_H264_FIXQP_S VENC_H265_FIXQP_S;
typedef struct _VENC_H264_QVBR_S VENC_H265_QVBR_S;
typedef struct _VENC_H264_UBR_S VENC_H265_UBR_S;

/* the attribute of mjpege fixqp*/
typedef struct _VENC_MJPEG_FIXQP_S {
	CVI_U32 u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	CVI_FR32 fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	CVI_U32 u32Qfactor; /* RW; Range:[1,99];image quality. */
	CVI_BOOL bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} VENC_MJPEG_FIXQP_S;

/* the attribute of mjpege cbr*/
typedef struct _VENC_MJPEG_CBR_S {
	CVI_U32 u32StatTime; /* RW; Range:[1, 60]; the rate statistic time, the unit is senconds(s) */
	CVI_U32 u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	CVI_FR32 fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	CVI_U32 u32BitRate; /* RW; Range:[2, 409600]; average bitrate */
	CVI_BOOL bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} VENC_MJPEG_CBR_S;

/* the attribute of mjpege vbr*/
typedef struct _VENC_MJPEG_VBR_S {
	CVI_U32 u32StatTime; /* RW; Range:[1, 60]; the rate statistic time, the unit is senconds(s) */
	CVI_U32 u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	CVI_FR32 fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	CVI_U32 u32MaxBitRate; /* RW; Range:[2, 409600];the max bitrate */
	CVI_BOOL bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} VENC_MJPEG_VBR_S;

/* the attribute of rc*/
typedef struct _VENC_RC_ATTR_S {
	VENC_RC_MODE_E enRcMode; /* RW; the type of rc*/
	union {
		VENC_H264_CBR_S stH264Cbr;
		VENC_H264_VBR_S stH264Vbr;
		VENC_H264_AVBR_S stH264AVbr;
		VENC_H264_QVBR_S stH264QVbr;
		VENC_H264_FIXQP_S stH264FixQp;
		VENC_H264_QPMAP_S stH264QpMap;
		VENC_H264_UBR_S stH264Ubr;

		VENC_MJPEG_CBR_S stMjpegCbr;
		VENC_MJPEG_VBR_S stMjpegVbr;
		VENC_MJPEG_FIXQP_S stMjpegFixQp;

		VENC_H265_CBR_S stH265Cbr;
		VENC_H265_VBR_S stH265Vbr;
		VENC_H265_AVBR_S stH265AVbr;
		VENC_H265_QVBR_S stH265QVbr;
		VENC_H265_FIXQP_S stH265FixQp;
		VENC_H265_QPMAP_S stH265QpMap;
		VENC_H265_UBR_S stH265Ubr;
	};
} VENC_RC_ATTR_S;

/* the gop mode */
typedef enum _VENC_GOP_MODE_E {
	VENC_GOPMODE_NORMALP = 0, /* NORMALP */
	VENC_GOPMODE_DUALP = 1, /* DUALP */
	VENC_GOPMODE_SMARTP = 2, /* SMARTP */
	VENC_GOPMODE_ADVSMARTP = 3, /* ADVSMARTP */
	VENC_GOPMODE_BIPREDB = 4, /* BIPREDB */
	VENC_GOPMODE_LOWDELAYB = 5, /* LOWDELAYB */

	VENC_GOPMODE_BUTT,
} VENC_GOP_MODE_E;

/* the attribute of the normalp*/
typedef struct _VENC_GOP_NORMALP_S {
	CVI_S32 s32IPQpDelta; /* RW; Range:[-10,30]; QP variance between P frame and I frame */
} VENC_GOP_NORMALP_S;

/* the attribute of the dualp*/
typedef struct _VENC_GOP_DUALP_S {
	CVI_U32 u32SPInterval; /* RW; Range:[0, 1)U(1, U32Gop -1]; Interval of the special P frames */
	CVI_S32 s32SPQpDelta; /* RW; Range:[-10,30]; QP variance between P frame and special P frame */
	CVI_S32 s32IPQpDelta; /* RW; Range:[-10,30]; QP variance between P frame and I frame */
} VENC_GOP_DUALP_S;

/* the attribute of the smartp*/
typedef struct _VENC_GOP_SMARTP_S {
	CVI_U32 u32BgInterval; /* RW; Range:[U32Gop,4294967295] ;Interval of the long-term reference frame*/
	CVI_S32 s32BgQpDelta; /* RW; Range:[-10,30]; QP variance between P frame and Bg frame */
	CVI_S32 s32ViQpDelta; /* RW; Range:[-10,30]; QP variance between P frame and virtual I  frame */
} VENC_GOP_SMARTP_S;

/* the attribute of the advsmartp*/
typedef struct _VENC_GOP_ADVSMARTP_S {
	CVI_U32 u32BgInterval; /* RW; Range:[U32Gop,4294967295] ;Interval of the long-term reference frame*/
	CVI_S32 s32BgQpDelta; /* RW; Range:[-10,30]; QP variance between P frame and Bg frame */
	CVI_S32 s32ViQpDelta; /* RW; Range:[-10,30]; QP variance between P frame and virtual I  frame */
} VENC_GOP_ADVSMARTP_S;

/* the attribute of the bipredb*/
typedef struct _VENC_GOP_BIPREDB_S {
	CVI_U32 u32BFrmNum; /* RW; Range:[1,3]; Number of B frames */
	CVI_S32 s32BQpDelta; /* RW; Range:[-10,30]; QP variance between P frame and B frame */
	CVI_S32 s32IPQpDelta; /* RW; Range:[-10,30]; QP variance between P frame and I frame */
} VENC_GOP_BIPREDB_S;

/* the attribute of the gop*/
typedef struct _VENC_GOP_ATTR_S {
	VENC_GOP_MODE_E enGopMode; /* RW; Encoding GOP type */
	union {
		VENC_GOP_NORMALP_S stNormalP; /*attributes of normal P*/
		VENC_GOP_DUALP_S stDualP; /*attributes of dual   P*/
		VENC_GOP_SMARTP_S stSmartP; /*attributes of Smart P*/
		VENC_GOP_ADVSMARTP_S stAdvSmartP; /*attributes of AdvSmart P*/
		VENC_GOP_BIPREDB_S stBipredB; /*attributes of b */
	};

} VENC_GOP_ATTR_S;

/**
 * @brief Define the attributes of the venc channel.
 *
 */
typedef struct _VENC_CHN_ATTR_S {
	VENC_ATTR_S stVencAttr;		///< The attribute of video encoder
	VENC_RC_ATTR_S stRcAttr;	///< The attribute of bitrate control
	VENC_GOP_ATTR_S stGopAttr;	///< TODO VENC
} VENC_CHN_ATTR_S;

#define RC_TEXTURE_THR_SIZE	(16)

/* The param of H264e cbr*/
typedef struct _VENC_PARAM_H264_CBR_S {
	CVI_U32 u32MinIprop; /* RW; Range:[1, 100]; the min ratio of i frame and p frame */
	CVI_U32 u32MaxIprop; /* RW; Range:(u32MinIprop, 100]; the max ratio of i frame and p frame */
	CVI_U32 u32MaxQp; /* RW; Range:(u32MinQp, 51];the max QP value */
	CVI_U32 u32MinQp; /* RW; Range:[0, 51]; the min QP value */
	CVI_U32 u32MaxIQp; /* RW; Range:(u32MinIQp, 51]; max qp for i frame */
	CVI_U32 u32MinIQp; /* RW; Range:[0, 51]; min qp for i frame */
	CVI_S32 s32MaxReEncodeTimes; /* RW; Range:[0, 3]; Range:max number of re-encode times.*/
	CVI_BOOL bQpMapEn; /* RW; Range:[0, 1]; enable qpmap.*/
} VENC_PARAM_H264_CBR_S;

/* The param of H264e vbr*/
typedef struct _VENC_PARAM_H264_VBR_S {
	CVI_S32 s32ChangePos;
	// RW; Range:[50, 100]; Indicates the ratio of the current bit rate to the maximum
	// bit rate when the QP value starts to be adjusted
	CVI_U32 u32MinIprop; /* RW; Range:[1, 100] ; the min ratio of i frame and p frame */
	CVI_U32 u32MaxIprop; /* RW; Range:(u32MinIprop, 100] ; the max ratio of i frame and p frame */
	CVI_S32 s32MaxReEncodeTimes; /* RW; Range:[0, 3]; max number of re-encode times */
	CVI_BOOL bQpMapEn;

	CVI_U32 u32MaxQp; /* RW; Range:(u32MinQp, 51]; the max P B qp */
	CVI_U32 u32MinQp; /* RW; Range:[0, 51]; the min P B qp */
	CVI_U32 u32MaxIQp; /* RW; Range:(u32MinIQp, 51]; the max I qp */
	CVI_U32 u32MinIQp; /* RW; Range:[0, 51]; the min I qp */
} VENC_PARAM_H264_VBR_S;

/* The param of H264e avbr*/
typedef struct _VENC_PARAM_H264_AVBR_S {
	CVI_S32 s32ChangePos;
	// RW; Range:[50, 100]; Indicates the ratio of the current bit rate to the maximum
	// bit rate when the QP value starts to be adjusted
	CVI_U32 u32MinIprop; /* RW; Range:[1, 100] ; the min ratio of i frame and p frame */
	CVI_U32 u32MaxIprop; /* RW; Range:(u32MinIprop, 100] ; the max ratio of i frame and p frame */
	CVI_S32 s32MaxReEncodeTimes; /* RW; Range:[0, 3]; max number of re-encode times */
	CVI_BOOL bQpMapEn;

	CVI_S32 s32MinStillPercent; /* RW; Range:[5, 100]; the min percent of target bitrate for still scene */
	CVI_U32 u32MaxStillQP; /* RW; Range:[u32MinIQp, u32MaxIQp]; the max QP value of I frame for still scene*/
	CVI_U32 u32MinStillPSNR; /* RW; reserved,Invalid member currently */

	CVI_U32 u32MaxQp; /* RW; Range:(u32MinQp, 51]; the max P B qp */
	CVI_U32 u32MinQp; /* RW; Range:[0, 51]; the min P B qp */
	CVI_U32 u32MaxIQp; /* RW; Range:(u32MinIQp, 51]; the max I qp */
	CVI_U32 u32MinIQp; /* RW; Range:[0, 51]; the min I qp */
	CVI_U32 u32MinQpDelta;
	// Difference between FrameLevelMinQp & u32MinQp,FrameLevelMinQp = u32MinQp(or u32MinIQp) + MinQpDelta

	CVI_U32 u32MotionSensitivity; /* RW; Range:[0, 100]; Motion Sensitivity */
	CVI_S32	s32AvbrFrmLostOpen;
	CVI_S32 s32AvbrFrmGap;
	CVI_S32 s32AvbrPureStillThr;
} VENC_PARAM_H264_AVBR_S;

typedef struct _VENC_PARAM_H264_QVBR_S {
	CVI_U32 u32MinIprop; /* the min ratio of i frame and p frame */
	CVI_U32 u32MaxIprop; /* the max ratio of i frame and p frame */
	CVI_S32 s32MaxReEncodeTimes; /* max number of re-encode times [0, 3]*/
	CVI_BOOL bQpMapEn;

	CVI_U32 u32MaxQp; /* the max P B qp */
	CVI_U32 u32MinQp; /* the min P B qp */
	CVI_U32 u32MaxIQp; /* the max I qp */
	CVI_U32 u32MinIQp; /* the min I qp */

	CVI_S32 s32BitPercentUL; /*Indicate the ratio of bitrate  upper limit*/
	CVI_S32 s32BitPercentLL; /*Indicate the ratio of bitrate  lower limit*/
	CVI_S32 s32PsnrFluctuateUL; /*Reduce the target bitrate when the value of psnr approch the upper limit*/
	CVI_S32 s32PsnrFluctuateLL; /*Increase the target bitrate when the value of psnr approch the lower limit */
} VENC_PARAM_H264_QVBR_S;

/* The param of H264e ubr */
typedef struct _VENC_PARAM_H264_UBR_S {
	CVI_U32 u32MinIprop; /* RW; Range:[1, 100]; the min ratio of i frame and p frame */
	CVI_U32 u32MaxIprop; /* RW; Range:(u32MinIprop, 100]; the max ratio of i frame and p frame */
	CVI_U32 u32MaxQp; /* RW; Range:(u32MinQp, 51];the max QP value */
	CVI_U32 u32MinQp; /* RW; Range:[0, 51]; the min QP value */
	CVI_U32 u32MaxIQp; /* RW; Range:(u32MinIQp, 51]; max qp for i frame */
	CVI_U32 u32MinIQp; /* RW; Range:[0, 51]; min qp for i frame */
	CVI_S32 s32MaxReEncodeTimes; /* RW; Range:[0, 3]; Range:max number of re-encode times.*/
	CVI_BOOL bQpMapEn; /* RW; Range:[0, 1]; enable qpmap.*/
} VENC_PARAM_H264_UBR_S;

/* The param of mjpege cbr*/
typedef struct _VENC_PARAM_MJPEG_CBR_S {
	CVI_U32 u32MaxQfactor; /* RW; Range:[MinQfactor, 99]; the max Qfactor value*/
	CVI_U32 u32MinQfactor; /* RW; Range:[1, 99]; the min Qfactor value */
} VENC_PARAM_MJPEG_CBR_S;

/* The param of mjpege vbr*/
typedef struct _VENC_PARAM_MJPEG_VBR_S {
	CVI_S32 s32ChangePos;
	// RW; Range:[50, 100]; Indicates the ratio of the current bit rate to the maximum
	// bit rate when the Qfactor value starts to be adjusted
	CVI_U32 u32MaxQfactor; /* RW; Range:[MinQfactor, 99]; max image quailty allowed */
	CVI_U32 u32MinQfactor; /* RW; Range:[1, 99]; min image quality allowed */
} VENC_PARAM_MJPEG_VBR_S;

/* The param of h265e cbr*/
typedef struct _VENC_PARAM_H265_CBR_S {
	CVI_U32 u32MinIprop; /* RW; Range: [1, 100]; the min ratio of i frame and p frame */
	CVI_U32 u32MaxIprop; /* RW; Range: (u32MinIprop, 100];the max ratio of i frame and p frame */
	CVI_U32 u32MaxQp; /* RW; Range:(u32MinQp, 51];the max QP value */
	CVI_U32 u32MinQp; /* RW; Range:[0, 51];the min QP value */
	CVI_U32 u32MaxIQp; /* RW; Range:(u32MinIQp, 51];max qp for i frame */
	CVI_U32 u32MinIQp; /* RW; Range:[0, 51];min qp for i frame */
	CVI_S32 s32MaxReEncodeTimes; /* RW; Range:[0, 3]; Range:max number of re-encode times.*/
	CVI_BOOL bQpMapEn; /* RW; Range:[0, 1]; enable qpmap.*/
	VENC_RC_QPMAP_MODE_E enQpMapMode; /* RW; Qpmap Mode*/
} VENC_PARAM_H265_CBR_S;

/* The param of h265e vbr*/
typedef struct _VENC_PARAM_H265_VBR_S {
	CVI_S32 s32ChangePos;
	// RW; Range:[50, 100];Indicates the ratio of the current
	// bit rate to the maximum bit rate when the QP value starts to be adjusted
	CVI_U32 u32MinIprop; /* RW; [1, 100] the min ratio of i frame and p frame */
	CVI_U32 u32MaxIprop; /* RW; (u32MinIprop, 100] the max ratio of i frame and p frame */
	CVI_S32 s32MaxReEncodeTimes; /* RW; Range:[0, 3]; Range:max number of re-encode times.*/

	CVI_U32 u32MaxQp; /* RW; Range:(u32MinQp, 51]; the max P B qp */
	CVI_U32 u32MinQp; /* RW; Range:[0, 51]; the min P B qp */
	CVI_U32 u32MaxIQp; /* RW; Range:(u32MinIQp, 51]; the max I qp */
	CVI_U32 u32MinIQp; /* RW; Range:[0, 51]; the min I qp */

	CVI_BOOL bQpMapEn; /* RW; Range:[0, 1]; enable qpmap.*/
	VENC_RC_QPMAP_MODE_E enQpMapMode; /* RW; Qpmap Mode*/
} VENC_PARAM_H265_VBR_S;

/* The param of h265e vbr*/
typedef struct _VENC_PARAM_H265_AVBR_S {
	CVI_S32 s32ChangePos;
	// RW; Range:[50, 100];Indicates the ratio of the current
	// bit rate to the maximum bit rate when the QP value starts to be adjusted
	CVI_U32 u32MinIprop; /* RW; [1, 100]the min ratio of i frame and p frame */
	CVI_U32 u32MaxIprop; /* RW; (u32MinIprop, 100]the max ratio of i frame and p frame */
	CVI_S32 s32MaxReEncodeTimes; /* RW; Range:[0, 3]; Range:max number of re-encode times.*/

	CVI_S32 s32MinStillPercent; /* RW; Range:[5, 100]; the min percent of target bitrate for still scene */
	CVI_U32 u32MaxStillQP; /* RW; Range:[u32MinIQp, u32MaxIQp]; the max QP value of I frame for still scene*/
	CVI_U32 u32MinStillPSNR; /* RW; reserved */

	CVI_U32 u32MaxQp; /* RW; Range:(u32MinQp, 51];the max P B qp */
	CVI_U32 u32MinQp; /* RW; Range:[0, 51];the min P B qp */
	CVI_U32 u32MaxIQp; /* RW; Range:(u32MinIQp, 51];the max I qp */
	CVI_U32 u32MinIQp; /* RW; Range:[0, 51];the min I qp */
	CVI_U32 u32MinQpDelta;
	// Difference between FrameLevelMinQp & u32MinQp, FrameLevelMinQp = u32MinQp(or u32MinIQp) + MinQpDelta
	CVI_U32 u32MotionSensitivity; /* RW; Range:[0, 100]; Motion Sensitivity */
	CVI_S32	s32AvbrFrmLostOpen; /* RW; Range:[0, 1]; Open Frame Lost */
	CVI_S32 s32AvbrFrmGap; /* RW; Range:[0, 100]; Maximim Gap of Frame Lost */
	CVI_S32 s32AvbrPureStillThr;
	CVI_BOOL bQpMapEn; /* RW; Range:[0, 1]; enable qpmap.*/
	VENC_RC_QPMAP_MODE_E enQpMapMode; /* RW; Qpmap Mode*/
} VENC_PARAM_H265_AVBR_S;

typedef struct _VENC_PARAM_H265_QVBR_S {
	CVI_U32 u32MinIprop; /* the min ratio of i frame and p frame */
	CVI_U32 u32MaxIprop; /* the max ratio of i frame and p frame */
	CVI_S32 s32MaxReEncodeTimes; /* max number of re-encode times [0, 3]*/

	CVI_BOOL bQpMapEn;
	VENC_RC_QPMAP_MODE_E enQpMapMode;

	CVI_U32 u32MaxQp; /* the max P B qp */
	CVI_U32 u32MinQp; /* the min P B qp */
	CVI_U32 u32MaxIQp; /* the max I qp */
	CVI_U32 u32MinIQp; /* the min I qp */

	CVI_S32 s32BitPercentUL; /* Indicate the ratio of bitrate  upper limit*/
	CVI_S32 s32BitPercentLL; /* Indicate the ratio of bitrate  lower limit*/
	CVI_S32 s32PsnrFluctuateUL; /* Reduce the target bitrate when the value of psnr approch the upper limit */
	CVI_S32 s32PsnrFluctuateLL; /* Increase the target bitrate when the value of psnr approch the lower limit */
} VENC_PARAM_H265_QVBR_S;

/* The param of h265e ubr*/
typedef struct _VENC_PARAM_H265_UBR_S {
	CVI_U32 u32MinIprop; /* RW; Range: [1, 100]; the min ratio of i frame and p frame */
	CVI_U32 u32MaxIprop; /* RW; Range: (u32MinIprop, 100]; the max ratio of i frame and p frame */
	CVI_U32 u32MaxQp; /* RW; Range:(u32MinQp, 51];the max QP value */
	CVI_U32 u32MinQp; /* RW; Range:[0, 51];the min QP value */
	CVI_U32 u32MaxIQp; /* RW; Range:(u32MinIQp, 51];max qp for i frame */
	CVI_U32 u32MinIQp; /* RW; Range:[0, 51];min qp for i frame */
	CVI_S32 s32MaxReEncodeTimes; /* RW; Range:[0, 3]; Range:max number of re-encode times.*/
	CVI_BOOL bQpMapEn; /* RW; Range:[0, 1]; enable qpmap.*/
	VENC_RC_QPMAP_MODE_E enQpMapMode; /* RW; Qpmap Mode*/
} VENC_PARAM_H265_UBR_S;

/* The param of rc*/
typedef struct _VENC_RC_PARAM_S {
	CVI_U32 u32ThrdI[RC_TEXTURE_THR_SIZE]; // RW; Range:[0, 255]; Mad threshold for
	// controlling the macroblock-level bit rate of I frames
	CVI_U32 u32ThrdP[RC_TEXTURE_THR_SIZE]; // RW; Range:[0, 255]; Mad threshold for
	// controlling the macroblock-level bit rate of P frames
	CVI_U32 u32ThrdB[RC_TEXTURE_THR_SIZE]; // RW; Range:[0, 255]; Mad threshold for
	// controlling the macroblock-level bit rate of B frames
	CVI_U32 u32DirectionThrd; /*RW; Range:[0, 16]; The direction for controlling the macroblock-level bit rate */
	CVI_U32 u32RowQpDelta;
	// RW; Range:[0, 10];the start QP value of each macroblock row relative to the start QP value
	CVI_S32 s32FirstFrameStartQp; /* RW; Range:[-1, 51];Start QP value of the first frame*/
	CVI_S32 s32InitialDelay;	// RW; Range:[10, 3000]; Rate control initial delay (ms).
								// Set 0 to use default value.
	CVI_U32 u32ThrdLv; /*RW; Range:[0, 4]; Mad threshold for controlling the macroblock-level bit rate */
	CVI_BOOL bBgEnhanceEn; /* RW; Range:[0, 1];  Enable background enhancement */
	CVI_S32 s32BgDeltaQp; /* RW; Range:[-51, 51]; Backgournd Qp Delta */
	union {
		VENC_PARAM_H264_CBR_S stParamH264Cbr;
		VENC_PARAM_H264_VBR_S stParamH264Vbr;
		VENC_PARAM_H264_AVBR_S stParamH264AVbr;
		VENC_PARAM_H264_QVBR_S stParamH264QVbr;
		VENC_PARAM_H264_UBR_S stParamH264Ubr;
		VENC_PARAM_H265_CBR_S stParamH265Cbr;
		VENC_PARAM_H265_VBR_S stParamH265Vbr;
		VENC_PARAM_H265_AVBR_S stParamH265AVbr;
		VENC_PARAM_H265_QVBR_S stParamH265QVbr;
		VENC_PARAM_H265_UBR_S stParamH265Ubr;
		VENC_PARAM_MJPEG_CBR_S stParamMjpegCbr;
		VENC_PARAM_MJPEG_VBR_S stParamMjpegVbr;
	};
} VENC_RC_PARAM_S;

/* the param of the roibg frame rate */
typedef struct _VENC_REF_PARAM_S {
	CVI_U32 u32Base; /* RW; Range:[0,4294967295]; Base layer period*/
	CVI_U32 u32Enhance; /* RW; Range:[0,255]; Enhance layer period*/
	CVI_BOOL bEnablePred; // RW; Range:[0, 1]; Whether some frames at the base
	// layer are referenced by other frames at the base layer. When bEnablePred is CVI_FALSE,
	// all frames at the base layer reference IDR frames.
} VENC_REF_PARAM_S;

/* the frame lost mode*/
typedef enum _VENC_FRAMELOST_MODE_E {
	FRMLOST_NORMAL = 0, /*normal mode*/
	FRMLOST_PSKIP, /*pskip*/
	FRMLOST_BUTT,
} VENC_FRAMELOST_MODE_E;

/* The param of the frame lost mode*/
typedef struct _VENC_FRAMELOST_S {
	CVI_BOOL bFrmLostOpen; // RW; Range:[0,1];Indicates whether to discard frames
	// to ensure stable bit rate when the instant bit rate is exceeded
	CVI_U32 u32FrmLostBpsThr; /* RW; Range:[64k, 163840k];the instant bit rate threshold */
	VENC_FRAMELOST_MODE_E enFrmLostMode; /* frame lost strategy*/
	CVI_U32 u32EncFrmGaps; /* RW; Range:[0,65535]; the gap of frame lost*/
} VENC_FRAMELOST_S;

/* the attribute of the roi */
typedef struct _VENC_ROI_ATTR_S {
	CVI_U32 u32Index; /* RW; Range:[0, 7]; Index of an ROI. The system supports indexes ranging from 0 to 7 */
	CVI_BOOL bEnable; /* RW; Range:[0, 1]; Whether to enable this ROI */
	CVI_BOOL bAbsQp; // RW; Range:[0, 1]; QP mode of an ROI. CVI_FALSE: relative QP. CVI_TRUE: absolute QP
	CVI_S32 s32Qp; /* RW; Range:[-51, 51]; QP value,only relative mode can QP value less than 0. */
	RECT_S stRect; /* RW;Region of an ROI*/
} VENC_ROI_ATTR_S;

/*the nalu type of H264E*/
typedef enum _H264E_NALU_TYPE_E {
	H264E_NALU_BSLICE = 0, /*B SLICE types*/
	H264E_NALU_PSLICE = 1, /*P SLICE types*/
	H264E_NALU_ISLICE = 2, /*I SLICE types*/
	H264E_NALU_IDRSLICE = 5, /*IDR SLICE types*/
	H264E_NALU_SEI = 6, /*SEI types*/
	H264E_NALU_SPS = 7, /*SPS types*/
	H264E_NALU_PPS = 8, /*PPS types*/
	H264E_NALU_BUTT
} H264E_NALU_TYPE_E;

/*the nalu type of H265E*/
typedef enum _H265E_NALU_TYPE_E {
	H265E_NALU_BSLICE = 0, /*B SLICE types*/
	H265E_NALU_PSLICE = 1, /*P SLICE types*/
	H265E_NALU_ISLICE = 2, /*I SLICE types*/
	H265E_NALU_IDRSLICE = 19, /*IDR SLICE types*/
	H265E_NALU_VPS = 32, /*VPS types*/
	H265E_NALU_SPS = 33, /*SPS types*/
	H265E_NALU_PPS = 34, /*PPS types*/
	H265E_NALU_SEI = 39, /*SEI types*/

	H265E_NALU_BUTT
} H265E_NALU_TYPE_E;

/*h265 decoding refresh type*/
typedef enum _H265E_REFERSH_TYPE_E {
	H265E_REFRESH_IDR = 0, /*Instantaneous decoding refresh picture*/
	H265E_REFRESH_CRA = 1, /*Clean random access picture*/
	H265E_REFRESH_BUTT
} H265E_REFRESH_TYPE_E;

/*the reference type of H264E slice*/
typedef enum _H264E_REFSLICE_TYPE_E {
	H264E_REFSLICE_FOR_1X = 1, /*Reference slice for H264E_REF_MODE_1X*/
	H264E_REFSLICE_FOR_2X = 2, /*Reference slice for H264E_REF_MODE_2X*/
	H264E_REFSLICE_FOR_4X = 5, /*Reference slice for H264E_REF_MODE_4X*/
	H264E_REFSLICE_FOR_BUTT /* slice not for reference*/
} H264E_REFSLICE_TYPE_E;

/*the pack type of JPEGE*/
typedef enum _JPEGE_PACK_TYPE_E {
	JPEGE_PACK_ECS = 5, /*ECS types*/
	JPEGE_PACK_APP = 6, /*APP types*/
	JPEGE_PACK_VDO = 7, /*VDO types*/
	JPEGE_PACK_PIC = 8, /*PIC types*/
	JPEGE_PACK_DCF = 9, /*DCF types*/
	JPEGE_PACK_DCF_PIC = 10, /*DCF PIC types*/
	JPEGE_PACK_BUTT
} JPEGE_PACK_TYPE_E;

/*the marker type of JPEGE*/
typedef enum _JPEGE_MARKER_TYPE_E {
	JPEGE_MARKER_SOI = 1,           /*SOI*/
	JPEGE_MARKER_DQT = 2,           /*DQT*/
	JPEGE_MARKER_DQT_MERGE = 3,     /*DQT containing multiple tables*/
	JPEGE_MARKER_DHT = 4,           /*DHT*/
	JPEGE_MARKER_DHT_MERGE = 5,     /*DHT containing multiple tables*/
	JPEGE_MARKER_DRI = 6,           /*DRI*/
	JPEGE_MARKER_DRI_OPT = 7,       /*DRI inserted only when restart interval is not 0*/
	JPEGE_MARKER_SOF0 = 8,          /*SOF0*/
	JPEGE_MARKER_JFIF = 9,          /*JFIF tags as APP0*/
	JPEGE_MARKER_FRAME_INDEX = 10,  /*frame index as APP9*/
	JPEGE_MARKER_ADOBE = 11,        /*ADOBE tags as APP14*/
	JPEGE_MARKER_USER_DATA = 12,    /*user data as APP15*/
	JPEGE_MARKER_BUTT
} JPEGE_MARKER_TYPE_E;

typedef enum _JPEGE_FORMAT_E {
	JPEGE_FORMAT_DEFAULT = 0,       /* SOI, FRAME_INDEX, USER_DATA, DRI_OPT, DQT, DHT, SOF0 */
	JPEGE_FORMAT_TYPE_1 = 1,        /* SOI, JFIF, DQT_MERGE, SOF0, DHT_MERGE, DRI */
	JPEGE_FORMAT_CUSTOM = 0xFF,     /* custom marker order specified by JpegMarkerOrder */
} JPEGE_FORMAT_E;

/*the pack type of PRORES*/
typedef enum _PRORES_PACK_TYPE_E {
	PRORES_PACK_PIC = 1, /*PIC types*/
	PRORES_PACK_BUTT
} PRORES_PACK_TYPE_E;

/*the data type of VENC*/
typedef union cviVENC_DATA_TYPE_U {
	H264E_NALU_TYPE_E enH264EType; /* R; H264E NALU types*/
	JPEGE_PACK_TYPE_E enJPEGEType; /* R; JPEGE pack types*/
	H265E_NALU_TYPE_E enH265EType; /* R; H264E NALU types*/
	PRORES_PACK_TYPE_E enPRORESType;
} VENC_DATA_TYPE_U;

/*the pack info of VENC*/
typedef struct _VENC_PACK_INFO_S {
	VENC_DATA_TYPE_U u32PackType; /* R; the pack type*/
	CVI_U32 u32PackOffset;
	CVI_U32 u32PackLength;
} VENC_PACK_INFO_S;

/*Defines a stream packet*/
typedef struct _VENC_PACK_S {
	CVI_U64 u64PhyAddr; /* R; the physics address of stream */

	CVI_U8 ATTRIBUTE *pu8Addr; /* R; the virtual address of stream */
#ifdef __arm__
	__u32 u32AddrPadding; /* padding for keeping same size of this structure */
#endif
	CVI_U32 ATTRIBUTE u32Len; /* R; the length of stream */

	CVI_U64 u64PTS; /* R; PTS */
	CVI_BOOL bFrameEnd; /* R; frame end */

	VENC_DATA_TYPE_U DataType; /* R; the type of stream */
	CVI_U32 u32Offset; /* R; the offset between the Valid data and the start address*/
	CVI_U32 u32DataNum; /* R; the  stream packets num */
	VENC_PACK_INFO_S stPackInfo[8]; /* R; the stream packet Information */
} VENC_PACK_S;

/*Defines the frame type and reference attributes of the H.264 frame skipping reference streams*/
typedef enum _H264E_REF_TYPE_E {
	BASE_IDRSLICE = 0, /* the Idr frame at Base layer*/
	BASE_PSLICE_REFTOIDR, // the P frame at Base layer,
	// referenced by other frames at Base layer and reference to Idr frame
	BASE_PSLICE_REFBYBASE, /* the P frame at Base layer, referenced by other frames at Base layer */
	BASE_PSLICE_REFBYENHANCE, /* the P frame at Base layer, referenced by other frames at Enhance layer */
	ENHANCE_PSLICE_REFBYENHANCE, /* the P frame at Enhance layer, referenced by other frames at Enhance layer */
	ENHANCE_PSLICE_NOTFORREF, /* the P frame at Enhance layer ,not referenced */
	ENHANCE_PSLICE_BUTT
} H264E_REF_TYPE_E;

/*Defines the features of an H.264 stream*/
typedef struct _VENC_STREAM_INFO_H264_S {
	CVI_U32 u32PicBytesNum; /* R; the coded picture stream byte number */
	CVI_U32 u32Inter16x16MbNum; /* R; the inter16x16 macroblock num */
	CVI_U32 u32Inter8x8MbNum; /* R; the inter8x8 macroblock num */
	CVI_U32 u32Intra16MbNum; /* R; the intra16x16 macroblock num */
	CVI_U32 u32Intra8MbNum; /* R; the intra8x8 macroblock num */
	CVI_U32 u32Intra4MbNum; /* R; the inter4x4 macroblock num */

	H264E_REF_TYPE_E enRefType; /* R; Type of encoded frames in advanced frame skipping reference mode */
	CVI_U32 u32UpdateAttrCnt; // R; Number of times that channel attributes
	// or parameters (including RC parameters) are set
	CVI_U32 u32StartQp; /* R; the start Qp of encoded frames*/
	CVI_U32 u32MeanQp; /* R; the mean Qp of encoded frames*/
	CVI_BOOL bPSkip;
} VENC_STREAM_INFO_H264_S;

typedef enum _H264E_REF_TYPE_E H265E_REF_TYPE_E;

/*Defines the features of an H.265 stream*/
typedef struct _VENC_STREAM_INFO_H265_S {
	CVI_U32 u32PicBytesNum; /* R; the coded picture stream byte number */
	CVI_U32 u32Inter64x64CuNum; /* R; the inter64x64 cu num  */
	CVI_U32 u32Inter32x32CuNum; /* R; the inter32x32 cu num  */
	CVI_U32 u32Inter16x16CuNum; /* R; the inter16x16 cu num  */
	CVI_U32 u32Inter8x8CuNum; /* R; the inter8x8   cu num  */
	CVI_U32 u32Intra32x32CuNum; /* R; the Intra32x32 cu num  */
	CVI_U32 u32Intra16x16CuNum; /* R; the Intra16x16 cu num  */
	CVI_U32 u32Intra8x8CuNum; /* R; the Intra8x8   cu num  */
	CVI_U32 u32Intra4x4CuNum; /* R; the Intra4x4   cu num  */
	H265E_REF_TYPE_E enRefType; /* R; Type of encoded frames in advanced frame skipping reference mode*/

	CVI_U32 u32UpdateAttrCnt; // R; Number of times that channel
	// attributes/parameters (including RC parameters) are set
	CVI_U32 u32StartQp; /* R; the start Qp of encoded frames*/
	CVI_U32 u32MeanQp; /* R; the mean Qp of encoded frames*/
	CVI_BOOL bPSkip;
} VENC_STREAM_INFO_H265_S;

/*Defines the features of an jpege stream*/
typedef struct _VENC_STREAM_INFO_PRORES_S {
	CVI_U32 u32PicBytesNum;
	CVI_U32 u32UpdateAttrCnt;
} VENC_STREAM_INFO_PRORES_S;

/*Defines the features of an jpege stream*/
typedef struct _VENC_STREAM_INFO_JPEG_S {
	CVI_U32 u32PicBytesNum; /* R; the coded picture stream byte number */
	CVI_U32 u32UpdateAttrCnt; // R; Number of times that channel attributes
	// or parameters (including RC parameters) are set
	CVI_U32 u32Qfactor; /* R; image quality */
} VENC_STREAM_INFO_JPEG_S;

/* the sse info*/
typedef struct _VENC_SSE_INFO_S {
	CVI_BOOL bSSEEn; /* RW; Range:[0,1]; Region SSE enable */
	CVI_U32 u32SSEVal; /* R; Region SSE value */
} VENC_SSE_INFO_S;

/* the advance information of the h264e */
typedef struct _VENC_STREAM_ADVANCE_INFO_H264_S {
	CVI_U32 u32ResidualBitNum; /* R; the residual num */
	CVI_U32 u32HeadBitNum; /* R; the head bit num */
	CVI_U32 u32MadiVal; /* R; the madi value */
	CVI_U32 u32MadpVal; /* R; the madp value */
	double dPSNRVal; /* R; the PSNR value */
	CVI_U32 u32MseLcuCnt; /* R; the lcu cnt of the mse */
	CVI_U32 u32MseSum; /* R; the sum of the mse */
	VENC_SSE_INFO_S stSSEInfo[8]; /* R; the information of the sse */
	CVI_U32 u32QpHstgrm[52]; /* R; the Qp histogram value */
	CVI_U32 u32MoveScene16x16Num; /* R; the 16x16 cu num of the move scene*/
	CVI_U32 u32MoveSceneBits; /* R; the stream bit num of the move scene */
} VENC_STREAM_ADVANCE_INFO_H264_S;

/* the advance information of the Jpege */
typedef struct _VENC_STREAM_ADVANCE_INFO_JPEG_S {
	// CVI_U32 u32Reserved;
} VENC_STREAM_ADVANCE_INFO_JPEG_S;

/* the advance information of the Prores */
typedef struct _VENC_STREAM_ADVANCE_INFO_PRORES_S {
	// CVI_U32 u32Reserved;
} VENC_STREAM_ADVANCE_INFO_PRORES_S;

/* the advance information of the h265e */
typedef struct _VENC_STREAM_ADVANCE_INFO_H265_S {
	CVI_U32 u32ResidualBitNum; /* R; the residual num */
	CVI_U32 u32HeadBitNum; /* R; the head bit num */
	CVI_U32 u32MadiVal; /* R; the madi value */
	CVI_U32 u32MadpVal; /* R; the madp value */
	double dPSNRVal; /* R; the PSNR value */
	CVI_U32 u32MseLcuCnt; /* R; the lcu cnt of the mse */
	CVI_U32 u32MseSum; /* R; the sum of the mse */
	VENC_SSE_INFO_S stSSEInfo[8]; /* R; the information of the sse */
	CVI_U32 u32QpHstgrm[52]; /* R; the Qp histogram value */
	CVI_U32 u32MoveScene32x32Num; /* R; the 32x32 cu num of the move scene*/
	CVI_U32 u32MoveSceneBits; /* R; the stream bit num of the move scene */
} VENC_STREAM_ADVANCE_INFO_H265_S;

/**
 * @brief Define the attributes of encoded bitstream
 *
 */
typedef struct _VENC_STREAM_S {
	VENC_PACK_S ATTRIBUTE *pstPack;	///< Encoded bitstream packet
#ifdef __arm__
	__u32 u32stPackPadding;
#endif
	CVI_U32 ATTRIBUTE u32PackCount; ///< Number of bitstream packets
	CVI_U32 u32Seq;	///< TODO VENC

	union {
		VENC_STREAM_INFO_H264_S stH264Info;		///< TODO VENC
		VENC_STREAM_INFO_JPEG_S stJpegInfo;		///< The information of JPEG bitstream
		VENC_STREAM_INFO_H265_S stH265Info;		///< TODO VENC
		VENC_STREAM_INFO_PRORES_S stProresInfo;	///< TODO VENC
	};

	union {
		VENC_STREAM_ADVANCE_INFO_H264_S
		stAdvanceH264Info;	///< TODO VENC
		VENC_STREAM_ADVANCE_INFO_JPEG_S
		stAdvanceJpegInfo;	///< TODO VENC
		VENC_STREAM_ADVANCE_INFO_H265_S
		stAdvanceH265Info;	///< TODO VENC
		VENC_STREAM_ADVANCE_INFO_PRORES_S
		stAdvanceProresInfo;///< TODO VENC
	};
} VENC_STREAM_S;

/* the param of the jpege */
typedef struct _VENC_JPEG_PARAM_S {
	CVI_U32 u32Qfactor; /* RW; Range:[1,99]; Qfactor value, 50 = user q-table */
	CVI_U8 u8YQt[64]; /* RW; Range:[1, 255]; Y quantization table */
	CVI_U8 u8CbQt[64]; /* RW; Range:[1, 255]; Cb quantization table */
	CVI_U8 u8CrQt[64]; /* RW; Range:[1, 255]; Cr quantization table */
	CVI_U32 u32MCUPerECS; // RW; Range:[0, (picwidth + 15) >> 4 x (picheight + 15) >> 4 x 2]; MCU number of one ECS
} VENC_JPEG_PARAM_S;

/* the param of the crop */
typedef struct _VENC_CROP_INFO_S {
	CVI_BOOL bEnable; /* RW; Range:[0, 1]; Crop region enable */
	RECT_S stRect; /* RW;  Crop region, note: s32X must be multi of 16 */
} VENC_CROP_INFO_S;

/* the param of the venc frame rate */
typedef struct _VENC_FRAME_RATE_S {
	CVI_S32 s32SrcFrmRate; /* RW; Range:[0, 240]; Input frame rate of a channel*/
	CVI_S32 s32DstFrmRate; /* RW; Range:[0, 240]; Output frame rate of a channel*/
} VENC_FRAME_RATE_S;

/* the param of the venc encode chnl */
typedef struct _VENC_CHN_PARAM_S {
	CVI_BOOL bColor2Grey; /* RW; Range:[0, 1]; Whether to enable Color2Grey.*/
	CVI_U32 u32Priority; /* RW; Range:[0, 1]; The priority of the coding chnl.*/
	CVI_U32 u32MaxStrmCnt; /* RW: Range:[0,4294967295]; Maximum number of frames in a stream buffer*/
	CVI_U32 u32PollWakeUpFrmCnt; /* RW: Range:(0,4294967295]; the frame num needed to wake up  obtaining streams */
	VENC_CROP_INFO_S stCropCfg;
	VENC_FRAME_RATE_S stFrameRate;
} VENC_CHN_PARAM_S;

typedef struct _VENC_STREAM_INFO_S {
	H265E_REF_TYPE_E enRefType; /*Type of encoded frames in advanced frame skipping reference mode */

	CVI_U32 u32PicBytesNum; /* the coded picture stream byte number */
	CVI_U32 u32PicCnt; /*Number of times that channel attributes or parameters (including RC parameters) are set */
	CVI_U32 u32StartQp; /*the start Qp of encoded frames*/
	CVI_U32 u32MeanQp; /*the mean Qp of encoded frames*/
	CVI_BOOL bPSkip;

	CVI_U32 u32ResidualBitNum; // residual
	CVI_U32 u32HeadBitNum; // head information
	CVI_U32 u32MadiVal; // madi
	CVI_U32 u32MadpVal; // madp
	CVI_U32 u32MseSum; /* Sum of MSE value */
	CVI_U32 u32MseLcuCnt; /* Sum of LCU number */
	double dPSNRVal; // PSNR
} VENC_STREAM_INFO_S;

/**
 * @brief Define the current channel status of encoder
 *
 */
typedef struct _VENC_CHN_STATUS_S {
	CVI_U32 u32LeftPics;				///< Number of frames left to be encoded TODO VENC
	CVI_U32 u32LeftStreamBytes;			///< Number of stream bytes left in the stream buffer TODO VENC
	CVI_U32 u32LeftStreamFrames;		///< Number of encoded frame in the stream buffer TODO VENC
	CVI_U32 u32CurPacks;				///< Number of packets in current frame
	CVI_U32 u32LeftRecvPics;			///< Number of frames to be received TODO VENC
	CVI_U32 u32LeftEncPics;				///< Number of frames to be encoded. TODO VENC
	CVI_BOOL bJpegSnapEnd;				///< if the process of JPEG captureThe is finished. TODO VENC
	VENC_STREAM_INFO_S stVencStrmInfo;	///< the stream information of encoder TODO VENC
} VENC_CHN_STATUS_S;

/* venc mode type */
typedef enum _VENC_MODTYPE_E {
	MODTYPE_VENC = 1, /* VENC */
	MODTYPE_H264E, /* H264e */
	MODTYPE_H265E, /* H265e */
	MODTYPE_JPEGE, /* Jpege */
	MODTYPE_RC, /* Rc */
	MODTYPE_BUTT
} VENC_MODTYPE_E;

typedef enum _VB_SOURCE_E {
	VB_SOURCE_COMMON = 0,
	VB_SOURCE_MODULE = 1,
	VB_SOURCE_PRIVATE = 2,
	VB_SOURCE_USER = 3,
	VB_SOURCE_BUTT
} VB_SOURCE_E;

/* the param of the h264e mod */
typedef struct _VENC_MOD_H264E_S {
	CVI_U32 u32OneStreamBuffer; /* RW; Range:[0,1]; one stream buffer*/
	CVI_U32 u32H264eMiniBufMode; /* RW; Range:[0,1]; H264e MiniBufMode*/
	CVI_U32 u32H264ePowerSaveEn; /* RW; Range:[0,1]; H264e PowerSaveEn*/
	VB_SOURCE_E enH264eVBSource; /* RW; Range:VB_SOURCE_PRIVATE,VB_SOURCE_USER; H264e VBSource*/
	CVI_BOOL bQpHstgrmEn; /* RW; Range:[0,1]*/
	CVI_U32 u32UserDataMaxLen;   /* RW; Range:[0,65536]; maximum number of bytes of a user data segment */
	CVI_BOOL bSingleEsBuf;       /* RW; Range[0,1]; use single output es buffer in n-way encode */
	CVI_U32 u32SingleEsBufSize;  /* RW; size of single es buffer in n-way encode */
} VENC_MOD_H264E_S;

/* the param of the h265e mod */
typedef struct _VENC_MOD_H265E_S {
	CVI_U32 u32OneStreamBuffer; /* RW; Range:[0,1]; one stream buffer*/
	CVI_U32 u32H265eMiniBufMode; /* RW; Range:[0,1]; H265e MiniBufMode*/
	CVI_U32 u32H265ePowerSaveEn; /* RW; Range:[0,2]; H265e PowerSaveEn*/
	VB_SOURCE_E enH265eVBSource; /* RW; Range:VB_SOURCE_PRIVATE,VB_SOURCE_USER; H265e VBSource*/
	CVI_BOOL bQpHstgrmEn; /* RW; Range:[0,1]*/
	CVI_U32 u32UserDataMaxLen;   /* RW; Range:[0,65536]; maximum number of bytes of a user data segment */
	CVI_BOOL bSingleEsBuf;       /* RW; Range[0,1]; use single output es buffer in n-way encode */
	CVI_U32 u32SingleEsBufSize;  /* RW; size of single es buffer in n-way encode */
	H265E_REFRESH_TYPE_E enRefreshType; /* RW; Range:H265E_REFRESH_IDR,H265E_REFRESH_CRA; decoding refresh type */
} VENC_MOD_H265E_S;

/* the param of the jpege mod */
typedef struct _VENC_MOD_JPEGE_S {
	CVI_U32 u32OneStreamBuffer; /* RW; Range:[0,1]; one stream buffer*/
	CVI_U32 u32JpegeMiniBufMode; /* RW; Range:[0,1]; Jpege MiniBufMode*/
	CVI_U32 u32JpegClearStreamBuf; /* RW; Range:[0,1]; JpegClearStreamBuf*/
	CVI_BOOL bSingleEsBuf;       /* RW; Range[0,1]; use single output es buffer in n-way encode */
	CVI_U32 u32SingleEsBufSize;  /* RW; size of single es buffer in n-way encode */
	JPEGE_FORMAT_E enJpegeFormat; /* RW; Range[0,255]; Jpege format with different marker order */
	JPEGE_MARKER_TYPE_E JpegMarkerOrder[JPEG_MARKER_ORDER_CNT];  /* RW: array specifying JPEG marker order*/
} VENC_MOD_JPEGE_S;

typedef struct _VENC_MOD_RC_S {
	CVI_U32 u32ClrStatAfterSetBr;
} VENC_MOD_RC_S;
/* the param of the venc mod */
typedef struct _VENC_MOD_VENC_S {
	CVI_U32 u32VencBufferCache; /* RW; Range:[0,1]; VencBufferCache*/
	CVI_U32 u32FrameBufRecycle; /* RW; Range:[0,1]; FrameBufRecycle*/
} VENC_MOD_VENC_S;

/* the param of the mod */
typedef struct _CVI_VENC_PARAM_MOD_S {
	VENC_MOD_VENC_S stVencModParam;
	VENC_MOD_H264E_S stH264eModParam;
	VENC_MOD_H265E_S stH265eModParam;
	VENC_MOD_JPEGE_S stJpegeModParam;
	VENC_MOD_RC_S stRcModParam;
} CVI_VENC_PARAM_MOD_S;

typedef enum _PIXEL_FORMAT_E {
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
} PIXEL_FORMAT_E;

/*
 * VIDEO_FORMAT_LINEAR: nature video line.
 */
typedef enum _VIDEO_FORMAT_E {
	VIDEO_FORMAT_LINEAR = 0,
	VIDEO_FORMAT_MAX
} VIDEO_FORMAT_E;

/*
 * COMPRESS_MODE_NONE: no compress.
 * COMPRESS_MODE_TILE: compress unit is a tile.
 * COMPRESS_MODE_LINE: compress unit is the whole line.
 * COMPRESS_MODE_FRAME: ompress unit is the whole frame.
 */
typedef enum _COMPRESS_MODE_E {
	COMPRESS_MODE_NONE = 0,
	COMPRESS_MODE_TILE,
	COMPRESS_MODE_LINE,
	COMPRESS_MODE_FRAME,
	COMPRESS_MODE_BUTT
} COMPRESS_MODE_E;

typedef enum _BAYER_FORMAT_E {
	BAYER_FORMAT_BG = 0,
	BAYER_FORMAT_GB,
	BAYER_FORMAT_GR,
	BAYER_FORMAT_RG,
	BAYER_FORMAT_MAX
} BAYER_FORMAT_E;

typedef enum _VIDEO_DISPLAY_MODE_E {
	VIDEO_DISPLAY_MODE_PREVIEW = 0x0,
	VIDEO_DISPLAY_MODE_PLAYBACK = 0x1,

	VIDEO_DISPLAY_MODE_MAX
} VIDEO_DISPLAY_MODE_E;

typedef enum _DYNAMIC_RANGE_E {
	DYNAMIC_RANGE_SDR8 = 0,
	DYNAMIC_RANGE_SDR10,
	DYNAMIC_RANGE_HDR10,
	DYNAMIC_RANGE_HLG,
	DYNAMIC_RANGE_SLF,
	DYNAMIC_RANGE_XDR,
	DYNAMIC_RANGE_MAX
} DYNAMIC_RANGE_E;

typedef enum _COLOR_GAMUT_E {
	COLOR_GAMUT_BT601 = 0,
	COLOR_GAMUT_BT709,
	COLOR_GAMUT_BT2020,
	COLOR_GAMUT_USER,
	COLOR_GAMUT_MAX
} COLOR_GAMUT_E;

/**
 * @brief Define video frame
 *
 * s16OffsetTop: top offset of show area
 * s16OffsetBottom: bottom offset of show area
 * s16OffsetLeft: left offset of show area
 * s16OffsetRight: right offset of show area
 * u32FrameFlag: FRAME_FLAG_E, can be OR operation.
 */
typedef struct _VIDEO_FRAME_S {
	CVI_U32 u32Width;
	CVI_U32 u32Height;
	PIXEL_FORMAT_E enPixelFormat;
	BAYER_FORMAT_E enBayerFormat;
	VIDEO_FORMAT_E enVideoFormat;
	COMPRESS_MODE_E enCompressMode;
	DYNAMIC_RANGE_E enDynamicRange;
	COLOR_GAMUT_E enColorGamut;
	CVI_U32 u32Stride[3];

	CVI_U64 u64PhyAddr[3];
	CVI_U8 *pu8VirAddr[3];
#ifdef __arm__
	__u32 u32VirAddrPadding[3];
#endif
	CVI_U32 u32Length[3];

	CVI_S16 s16OffsetTop;
	CVI_S16 s16OffsetBottom;
	CVI_S16 s16OffsetLeft;
	CVI_S16 s16OffsetRight;

	CVI_U32 u32TimeRef;
	CVI_U64 u64PTS;

	void *pPrivateData;
#ifdef __arm__
	__u32 u32PrivateDataPadding;
#endif
	CVI_U32 u32FrameFlag;
} VIDEO_FRAME_S;

typedef enum _VIDEO_MODE_E {
	VIDEO_MODE_STREAM = 0, /* send by stream */
	VIDEO_MODE_FRAME, /* send by frame  */
	VIDEO_MODE_COMPAT, /* One frame supports multiple packets sending. */
	/* The current frame is considered to end when bEndOfFrame is equal to HI_TRUE */
	VIDEO_MODE_BUTT
} VIDEO_MODE_E;

typedef struct _VDEC_ATTR_VIDEO_S {
	CVI_U32 u32RefFrameNum; /* RW, Range: [0, 16]; reference frame num. */
	CVI_BOOL bTemporalMvpEnable; /* RW; */
	/* specifies whether temporal motion vector predictors can be used for inter prediction */
	CVI_U32 u32TmvBufSize; /* RW; tmv buffer size(Byte) */
} VDEC_ATTR_VIDEO_S;

typedef struct _VDEC_CHN_ATTR_S {
	PAYLOAD_TYPE_E enType; /* RW; video type to be decoded   */
	VIDEO_MODE_E enMode; /* RW; send by stream or by frame */
	CVI_U32 u32PicWidth; /* RW; max pic width */
	CVI_U32 u32PicHeight; /* RW; max pic height */
	CVI_U32 u32StreamBufSize; /* RW; stream buffer size(Byte) */
	CVI_U32 u32FrameBufSize; /* RW; frame buffer size(Byte) */
	CVI_U32 u32FrameBufCnt;
	union {
		VDEC_ATTR_VIDEO_S
		stVdecVideoAttr; /* structure with video ( h264/h265) */
	};
} VDEC_CHN_ATTR_S;

typedef enum _VIDEO_DEC_MODE_E {
	VIDEO_DEC_MODE_IPB = 0,
	VIDEO_DEC_MODE_IP,
	VIDEO_DEC_MODE_I,
	VIDEO_DEC_MODE_BUTT
} VIDEO_DEC_MODE_E;

typedef enum _VIDEO_OUTPUT_ORDER_E {
	VIDEO_OUTPUT_ORDER_DISP = 0,
	VIDEO_OUTPUT_ORDER_DEC,
	VIDEO_OUTPUT_ORDER_BUTT
} VIDEO_OUTPUT_ORDER_E;

typedef struct _VDEC_PARAM_VIDEO_S {
	CVI_S32 s32ErrThreshold; /* RW, Range: [0, 100]; */
	/* threshold for stream error process, 0: discard with any error, 100 : keep data with any error */
	VIDEO_DEC_MODE_E enDecMode; /* RW; */
	/* decode mode , 0: deocde IPB frames, 1: only decode I frame & P frame , 2: only decode I frame */
	VIDEO_OUTPUT_ORDER_E enOutputOrder; /* RW; */
	/* frames output order ,0: the same with display order , 1: the same width decoder order */
	COMPRESS_MODE_E enCompressMode; /* RW; compress mode */
	VIDEO_FORMAT_E enVideoFormat; /* RW; video format */
} VDEC_PARAM_VIDEO_S;

typedef struct _VDEC_PARAM_PICTURE_S {
	CVI_U32 u32Alpha; /* RW, Range: [0, 255]; value 0 is transparent. */
	/* [0 ,127]   is deemed to transparent when enPixelFormat is ARGB1555 or
	 * ABGR1555 [128 ,256] is deemed to non-transparent when enPixelFormat is
	 * ARGB1555 or ABGR1555
	 */
} VDEC_PARAM_PICTURE_S;

typedef struct _VDEC_CHN_PARAM_S {
	PAYLOAD_TYPE_E enType; /* RW; video type to be decoded   */
	PIXEL_FORMAT_E enPixelFormat; /* RW; out put pixel format */
	CVI_U32 u32DisplayFrameNum; /* RW, Range: [0, 16]; display frame num */
	union {
		VDEC_PARAM_VIDEO_S
		stVdecVideoParam; /* structure with video ( h265/h264) */
		VDEC_PARAM_PICTURE_S
		stVdecPictureParam; /* structure with picture (jpeg/mjpeg ) */
	};
} VDEC_CHN_PARAM_S;

typedef struct _VDEC_DECODE_ERROR_S {
	CVI_S32 s32FormatErr; /* R; format error. eg: do not support filed */
	CVI_S32 s32PicSizeErrSet; /* R; picture width or height is larger than channel width or height */
	CVI_S32 s32StreamUnsprt; /* R; unsupport the stream specification */
	CVI_S32 s32PackErr; /* R; stream package error */
	CVI_S32 s32PrtclNumErrSet; /* R; protocol num is not enough. eg: slice, pps, sps */
	CVI_S32 s32RefErrSet; /* R; reference num is not enough */
	CVI_S32 s32PicBufSizeErrSet; /* R; the buffer size of picture is not enough */
	CVI_S32 s32StreamSizeOver; /* R; the stream size is too big and force discard stream */
	CVI_S32 s32VdecStreamNotRelease; /* R; the stream not released for too long time */
} VDEC_DECODE_ERROR_S;

typedef struct _VDEC_CHN_STATUS_S {
	PAYLOAD_TYPE_E enType; /* R; video type to be decoded */
	CVI_S32 u32LeftStreamBytes; /* R; left stream bytes waiting for decode */
	CVI_S32 u32LeftStreamFrames; /* R; left frames waiting for decode,only valid for VIDEO_MODE_FRAME */
	CVI_S32 u32LeftPics; /* R; pics waiting for output */
	CVI_BOOL bStartRecvStream; /* R; had started recv stream? */
	CVI_U32 u32RecvStreamFrames; /* R; how many frames of stream has been received. valid when send by frame. */
	CVI_U32 u32DecodeStreamFrames; /* R; how many frames of stream has been decoded. valid when send by frame. */
	VDEC_DECODE_ERROR_S stVdecDecErr; /* R; information about decode error */
	CVI_U32 u32Width; /* R; the width of the currently decoded stream */
	CVI_U32 u32Height; /* R; the height of the currently decoded stream */
} VDEC_CHN_STATUS_S;

typedef struct _VDEC_VIDEO_MOD_PARAM_S {
	CVI_U32 u32MaxPicWidth;
	CVI_U32 u32MaxPicHeight;
	CVI_U32 u32MaxSliceNum;
	CVI_U32 u32VdhMsgNum;
	CVI_U32 u32VdhBinSize;
	CVI_U32 u32VdhExtMemLevel;
} VDEC_VIDEO_MOD_PARAM_S;

typedef enum _VDEC_CAPACITY_STRATEGY_E {
	VDEC_CAPACITY_STRATEGY_BY_MOD = 0,
	VDEC_CAPACITY_STRATEGY_BY_CHN = 1,
	VDEC_CAPACITY_STRATEGY_BUTT
} VDEC_CAPACITY_STRATEGY_E;

typedef struct _VDEC_PICTURE_MOD_PARAM_S {
	CVI_U32 u32MaxPicWidth;
	CVI_U32 u32MaxPicHeight;
	CVI_BOOL bSupportProgressive;
	CVI_BOOL bDynamicAllocate;
	VDEC_CAPACITY_STRATEGY_E enCapStrategy;
} VDEC_PICTURE_MOD_PARAM_S;

typedef struct _VDEC_MOD_PARAM_S {
	VB_SOURCE_E enVdecVBSource; /* RW, Range: [1, 3];  frame buffer mode  */
	CVI_U32 u32MiniBufMode; /* RW, Range: [0, 1];  stream buffer mode */
	CVI_U32 u32ParallelMode; /* RW, Range: [0, 1];  VDH working mode   */
	VDEC_VIDEO_MOD_PARAM_S stVideoModParam;
	VDEC_PICTURE_MOD_PARAM_S stPictureModParam;
} VDEC_MOD_PARAM_S;

/*the super frame mode*/
typedef enum _VENC_SUPERFRM_MODE_E {
	SUPERFRM_NONE = 0, /* sdk don't care super frame */
	SUPERFRM_DISCARD, /* the super frame is discarded */
	SUPERFRM_REENCODE, /* the super frame is re-encode */
	SUPERFRM_REENCODE_IDR, /* the super frame is re-encode to IDR */
	SUPERFRM_BUTT
} VENC_SUPERFRM_MODE_E;

/* the rc priority*/
typedef enum _VENC_RC_PRIORITY_E {
	VENC_RC_PRIORITY_BITRATE_FIRST = 1, /* bitrate first */
	VENC_RC_PRIORITY_FRAMEBITS_FIRST, /* framebits first*/
	VENC_RC_PRIORITY_BUTT,
} VENC_RC_PRIORITY_E;

/* the config of the superframe */
typedef struct _VENC_SUPERFRAME_CFG_S {
	VENC_SUPERFRM_MODE_E enSuperFrmMode;
	/* RW; Indicates the mode of processing the super frame */
	CVI_U32 u32SuperIFrmBitsThr; // RW; Range:[0, 33554432];Indicate the threshold
	// of the super I frame for enabling the super frame processing mode
	CVI_U32 u32SuperPFrmBitsThr; // RW; Range:[0, 33554432];Indicate the threshold
	// of the super P frame for enabling the super frame processing mode
	CVI_U32 u32SuperBFrmBitsThr; // RW; Range:[0, 33554432];Indicate the threshold
	// of the super B frame for enabling the super frame processing mode
	VENC_RC_PRIORITY_E enRcPriority; /* RW; Rc Priority */
} VENC_SUPERFRAME_CFG_S;

typedef struct _VCODEC_PERF_FPS_S {
	CVI_U32 u32InFPS;
	CVI_U32 u32OutFPS;
	CVI_U64 u64HwTime;
} VCODEC_PERF_FPS_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif  /* __VPU_DRV_PROC_INFO_H__ */

