#ifndef __ENC_CTX_H__
#define __ENC_CTX_H__

#include <linux/cvi_comm_venc.h>
#include "cvi_jpg_interface.h"
#include "sophgo_jpeg_interface.h"
#include "cvi_h265_interface.h"


typedef struct _venc_jpeg_ctx {
    CVIJpgHandle *handle;
} venc_jpeg_ctx;

typedef struct _venc_vid_ctx {
    CVI_VOID (*setInitCfgFixQp)
    (cviInitEncConfig *pInitEncCfg, CVI_VOID *pchnctx);
    CVI_VOID (*setInitCfgCbr)
    (cviInitEncConfig *pInitEncCfg, CVI_VOID *pchnctx);
    CVI_VOID (*setInitCfgVbr)
    (cviInitEncConfig *pInitEncCfg, CVI_VOID *pchnctx);
    CVI_VOID (*setInitCfgAVbr)
    (cviInitEncConfig *pInitEncCfg, CVI_VOID *pchnctx);
    CVI_VOID (*setInitCfgQpMap)
    (cviInitEncConfig *pInitEncCfg, CVI_VOID *pchnctx);
    CVI_VOID (*setInitCfgUbr)
    (cviInitEncConfig *pInitEncCfg, CVI_VOID *pchnctx);
    CVI_VOID (*setInitCfgRc)
    (cviInitEncConfig *pInitEncCfg, CVI_VOID *pchnctx);
    CVI_S32 (*mapNaluType)(VENC_PACK_S *ppack, CVI_S32 naluType);
    CVI_VOID *pHandle;
    cviVEncStreamInfo streamInfo;
} venc_vid_ctx;

typedef struct _venc_h264_ctx {
} venc_h264_ctx;

typedef struct _venc_h265_ctx {
} venc_h265_ctx;

typedef struct _venc_enc_ctx_base {
    CVI_S32 x;
    CVI_S32 y;
    CVI_S32 width;
    CVI_S32 height;
    CVI_S32 u32Profile;
    CVI_S32 rcMode;
    CVI_BOOL bVariFpsEn;
    CVI_U64 u64PTS;
    CVI_U64 u64EncHwTime;
    CVI_S32 (*init)(CVI_VOID);
    CVI_S32 (*open)(CVI_VOID *handle, CVI_VOID *pchnctx);
    CVI_S32 (*close)(CVI_VOID *ctx);
    CVI_S32 (*encOnePic)
    (CVI_VOID *ctx, const VIDEO_FRAME_INFO_S *pstFrame,
     CVI_S32 s32MilliSec);
    CVI_S32 (*getStream)
    (CVI_VOID *ctx, VENC_STREAM_S *pstStream, CVI_S32 s32MIlliSec);
    CVI_S32 (*releaseStream)(CVI_VOID *ctx, VENC_STREAM_S *pstStream);
    CVI_S32 (*ioctl)(CVI_VOID *ctx, CVI_S32 op, CVI_VOID *arg);
} venc_enc_ctx_base;

typedef struct _venc_enc_ctx {
    venc_enc_ctx_base base;
    union {
        venc_jpeg_ctx jpeg;
        struct {
            venc_vid_ctx vid;
            union {
                venc_h264_ctx h264;
                venc_h265_ctx h265;
            };
        };
    } ext;
} venc_enc_ctx;


#define H265_NALU_TYPE_W_RADL 19
#define H265_NALU_TYPE_N_LP 20
#define H264_NALU_TYPE_IDR 5

#ifndef UNREFERENCED_PARAM
#define UNREFERENCED_PARAM(x) ((void)(x))
#endif

#define DEF_IQP 32
#define DEF_PQP 32
#define ENC_TIMEOUT (-2)
#define DEAULT_QP_MAP_BITRATE 6000

#define MAX_VENC_QUEUE_DEPTH 4



CVI_S32 venc_create_enc_ctx(venc_enc_ctx *pEncCtx, CVI_VOID *pchnctx);
#if 0
CVI_S32 jpege_init(CVI_VOID);
CVI_S32 jpege_open(CVI_VOID *handle, CVI_VOID *pchnctx);
CVI_S32 jpege_close(CVI_VOID *ctx);
CVI_S32 jpege_enc_one_pic(CVI_VOID *ctx,
                 const VIDEO_FRAME_INFO_S *pstFrame,
                 int s32MilliSec);
CVI_VOID setSrcInfo(CVIFRAMEBUF *psi, venc_enc_ctx *pEncCtx,
               const VIDEO_FRAME_INFO_S *pstFrame);
CVI_S32 jpege_get_stream(CVI_VOID *ctx, VENC_STREAM_S *pstStream,
                CVI_S32 s32MilliSec);
CVI_S32 jpege_release_stream(CVI_VOID *ctx, VENC_STREAM_S *pstStream);
CVI_S32 jpege_ioctl(CVI_VOID *ctx, CVI_S32 op, CVI_VOID *arg);

CVI_S32 vidEnc_init(CVI_VOID);
CVI_S32 vidEnc_open(CVI_VOID *handle, CVI_VOID *pchnctx);
CVI_S32 vidEnc_close(CVI_VOID *ctx);
CVI_S32 vidEnc_enc_one_pic(CVI_VOID *ctx,
                  const VIDEO_FRAME_INFO_S *pstFrame,
                  CVI_S32 s32MilliSec);
CVI_S32 vidEnc_get_stream(CVI_VOID *ctx, VENC_STREAM_S *pstStream,
                 CVI_S32 s32MilliSec);
CVI_S32 vidEnc_release_stream(CVI_VOID *ctx, VENC_STREAM_S *pstStream);
CVI_S32 vidEnc_ioctl(CVI_VOID *ctx, CVI_S32 op, CVI_VOID *arg);

CVI_VOID h265e_setInitCfgFixQp(cviInitEncConfig *pInitEncCfg,
                      CVI_VOID *pchnctx);
CVI_VOID h265e_setInitCfgCbr(cviInitEncConfig *pInitEncCfg,
                    CVI_VOID *pchnctx);
CVI_VOID h265e_setInitCfgVbr(cviInitEncConfig *pInitEncCfg,
                    CVI_VOID *pchnctx);
CVI_VOID h265e_setInitCfgAVbr(cviInitEncConfig *pInitEncCfg,
                     CVI_VOID *pchnctx);
CVI_VOID h265e_setInitCfgQpMap(cviInitEncConfig *pInitEncCfg,
                      CVI_VOID *pchnctx);
CVI_VOID h265e_setInitCfgUbr(cviInitEncConfig *pInitEncCfg,
                    CVI_VOID *pchnctx);
CVI_S32 h265e_mapNaluType(VENC_PACK_S *ppack, CVI_S32 naluType);


CVI_VOID cviSetInitCfgGop(cviInitEncConfig *pInitEncCfg,
                 venc_chn_context *pChnHandle);
CVI_VOID h264e_setInitCfgFixQp(cviInitEncConfig *pInitEncCfg,
                      CVI_VOID *pchnctx);
CVI_VOID h264e_setInitCfgCbr(cviInitEncConfig *pInitEncCfg,
                    CVI_VOID *pchnctx);
CVI_VOID h264e_setInitCfgVbr(cviInitEncConfig *pInitEncCfg,
                    CVI_VOID *pchnctx);
CVI_VOID h264e_setInitCfgAVbr(cviInitEncConfig *pInitEncCfg,
                     CVI_VOID *pchnctx);
CVI_VOID h264e_setInitCfgUbr(cviInitEncConfig *pInitEncCfg,
                    CVI_VOID *pchnctx);
CVI_S32 h264e_mapNaluType(VENC_PACK_S *ppack, CVI_S32 naluType);
#endif

#endif
