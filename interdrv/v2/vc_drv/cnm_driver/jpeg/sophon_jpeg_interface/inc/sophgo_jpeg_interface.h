#ifndef __SOPHGO_JPEG_INTERFACE_H__
#define __SOPHGO_JPEG_INTERFACE_H__


/* enum define */
typedef enum {
    JPGCOD_E_UNKNOWN = 0,
    JPGCOD_E_DEC = 1,
    JPGCOD_E_ENC = 2,
    JPGCOD_E_BUTT
} SOPHGO_E_JPEG_CODEC_TYPE;

typedef enum {
    YUV_E_FORMAT_420 = 0,
    YUV_E_FORMAT_422 = 1,
    YUV_E_FORMAT_440 = 2,
    YUV_E_FORMAT_444 = 3,
    YUV_E_FORMAT_400 = 4,
    YUV_E_FORMAT_BUTT
} SOPHGO_E_FRAME_FORMAT;


typedef enum {
    PACKED_FORMAT_E_NONE,
    PACKED_FORMAT_E_422_YUYV,
    PACKED_FORMAT_E_422_UYVY,
    PACKED_FORMAT_E_422_YVYU,
    PACKED_FORMAT_E_422_VYUY,
    PACKED_FORMAT_E_444,
    PACKED_FORMAT_E_444_RGB,
    PACKED_FORMAT_E_BUTT
} SOPHGO_E_PACKED_FORAMT;

typedef enum {
    CBCR_E_SEPARATED = 0,
    CBCR_E_INTERLEAVE,
    CRCB_E_INTERLEAVE,
    CRCB_E_BUTT
} SOPHGO_E_CBCR_INTERLEAVE;

typedef enum {
    JPEG_MEM_E_MODULE = 2,
    JPEG_MEM_E_EXTERNAL = 3,
} SOPHGO_E_JPEG_MEM_TYPE;


typedef struct {
    unsigned long size;
    unsigned long phys_addr;
    unsigned long base;
    char *virt_addr;
} SOPHGO_S_USER_BUF;

/* Frame Buffer */

typedef struct {
    unsigned long u32Size;
    unsigned long phys_addr;
    unsigned long base;
    unsigned long virt_addr;
} SOPHGO_S_BUF,*SOPHGO_S_BUF_PTR;

typedef struct {
    SOPHGO_E_FRAME_FORMAT eFormat;
    SOPHGO_E_PACKED_FORAMT ePackedFormat;
    SOPHGO_E_CBCR_INTERLEAVE eCbcrInterleave;
    SOPHGO_S_BUF vbY;
    SOPHGO_S_BUF vbCb;
    SOPHGO_S_BUF vbCr;
    int s32Width;
    int s32Height;
    int s32StrideY;
    int s32StrideC;
} SOPHGO_S_FRAME_BUF,*SOPHGO_S_FRAME_BUF_PTR;

/* encode config param */
typedef struct {
    int s32PicWidth;
    int s32PicHeight;
    int s32RotAngle;
    int s32MirDir;
    SOPHGO_E_FRAME_FORMAT eSourceFormat;
    int s32OutNum;
    SOPHGO_E_CBCR_INTERLEAVE eChromaInterleave;
    int bEnStuffByte;
    int encHeaderMode;
    int RandRotMode;
    SOPHGO_E_PACKED_FORAMT ePackedFormat;
    int s32Quality;
    int s32Bitrate;
    int s32Framerate;
    int s32SrcType;
    int s32BitstreamBufSize;
}SOPHGO_S_ENC_CONFIG_PARAM,*SOPHGO_S_ENC_CONFIG_PARAM_PTR;

/* decode config param */
typedef struct {
    unsigned int              StreamEndian;
    unsigned int              FrameEndian;
    unsigned int             bitstreamBuffer;
    unsigned int              bitstreamBufferSize;
    unsigned char               *pBitStream;
    //ROI
    unsigned int              roiEnable;
    unsigned int              roiWidth;
    unsigned int              roiHeight;
    unsigned int              roiOffsetX;
    unsigned int              roiOffsetY;
    unsigned int              roiWidthInMcu;
    unsigned int              roiHeightInMcu;
    unsigned int              roiOffsetXInMcu;
    unsigned int              roiOffsetYInMcu;
    unsigned int              rotation;
    unsigned int             mirror;
    unsigned int             subsample;
    unsigned int            packedFormat;
    unsigned int              cbcrInterleave;
    unsigned int              bsSize;
    unsigned int              pixelJustification;
    unsigned int               feedingMode;
    unsigned int              iHorScaleMode;
    unsigned int              iVerScaleMode;
    int                    pf_en;
    SOPHGO_S_FRAME_BUF     stDecBuf;
} SOPHGO_S_DEC_CONFIG_PARAM,*SOPHGO_S_DEC_CONFIG_PARAM_PTR;


/* jpu config param */
typedef struct {
    SOPHGO_E_JPEG_CODEC_TYPE eType;
    unsigned int             s32Chn;
    union {
        SOPHGO_S_ENC_CONFIG_PARAM stEncConfig;
        SOPHGO_S_DEC_CONFIG_PARAM stDecConfig;
    } u;
} SOPHGO_S_JPEG_CONFIG,*SOPHGO_S_JPEG_CONFIG_PTR;


/* JPU CODEC HANDLE */
typedef void *SOPHGO_JPEG_HANDLE_PTR;

int sophgo_jpeg_init(void);

void sophgo_jpeg_deinit(void);

SOPHGO_JPEG_HANDLE_PTR sophgo_jpeg_open_instance(SOPHGO_S_JPEG_CONFIG stJpegConfig);
/* close and free alloced jpu handle */
int sophgo_jpeg_close_instance(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle);
/* send jpu data to decode or encode */
int sophgo_jpeg_send_frame_data(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, void *pvData, int s32Length, int s32TimeOut);
/* after decoded or encoded, get data from jpu */
int sophgo_jpeg_get_frame_data(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, void *pvData, int s32Length, unsigned long int *pu64HwTime);
/* get jpu encoder input data buffer,check if the source framebuffer set or not */
int sophgo_jpeg_get_input_source_data(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, void *pvData);
/* release stream buffer */
int sophgo_jpeg_release_frame_data(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle);
int sophgo_jpeg_set_quality_params(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, void *pvData);
int sophgo_jpeg_get_quality_params(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, void *pvData);
int sophgo_jpeg_get_output_count(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle);


#endif
