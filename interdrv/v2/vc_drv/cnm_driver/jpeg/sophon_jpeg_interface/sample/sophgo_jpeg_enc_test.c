#include "cvi_vc_getopt.h"
#include "sophgo_jpeg_interface.h"
#include "jpu_helper.h"

#define CFG_DIR                     "./cfg"
#define YUV_DIR                     "./yuv"
#define DEFAULT_OUTPUT_PATH         "output.jpg"


static void Help(
    const char* programName
    )
{
    pr_err("------------------------------------------------------------------------------\n");
    pr_err(" CODAJ12 Encoder \n");
    pr_err("------------------------------------------------------------------------------\n");
    pr_err("%s [option] cfg_file \n", programName);
    pr_err("-h                      help\n");
    pr_err("--output=FILE           output file path\n");
    pr_err("--cfg-dir=DIR           folder that has a huffman table and a quantization table. default: %s\n", CFG_DIR);
    pr_err("--yuv-dir=DIR           folder that has an input source image. default: %s\n", YUV_DIR);
    pr_err("--yuv=FILE              use given yuv file instead of yuv file in cfg file\n");
    pr_err("--slice-height=height   the vertical height of a slice. multiple of 8 alignment. 0 is to set the height of picture\n");
    pr_err("--enable-slice-intr     enable get the interrupt at every slice encoded\n");
    pr_err("--stream-endian=ENDIAN  bitstream endianness. refer to datasheet Chapter 4.\n");
    pr_err("--frame-endian=ENDIAN   pixel endianness of 16bit input source. refer to datasheet Chapter 4.\n");
    pr_err("--bs-size=SIZE          bitstream buffer size in byte\n");
    pr_err("--width=SIZE           yuv width\n");
    pr_err("--height=SIZE          yuv height\n");
    pr_err("--quality=PERCENTAGE    quality factor(0..100)\n");
    pr_err("--pixelj=JUSTIFICATION  16bit-pixel justification. 0(default) - msb justified, 1 - lsb justified in little-endianness\n");
    pr_err("--enable-tiledMode      enable tiled mode (default linear mode)\n");
#if defined(CNM_FPGA_PLATFORM)
    pr_err("--pf                    Enable performance measurement\n");
    pr_err("--aclk=CLOCK            AXI clock, valid range 16 ~ 30\n");
    pr_err("--cclk=CLOCK            CORE clock, valid range 16 ~ 30\n");
    pr_err("--aclk_div=[0..3]       AXI clock division. 0: 1x, 1: 2x, 2: 4x, 3:8x \n");
    pr_err("--cclk_div=[0..3]       CORE clock division. 0: 1x, 1: 2x, 2: 4x, 3:8x\n");
    pr_err("--rdleay=[0..1000]      AXI read delay\n");
    pr_err("--wdleay=[0..1000]      AXI write delay\n");
#endif

    return;
}


static int write_jpg_file(char *name, unsigned char *pDstBuf, int suffix)
{
    // int i;
    int writeLen = 0;
    char writeName[256];
    drv_file_t *fpJpg;
    char *data;
    char *data1;
    SOPHGO_S_BUF *sophgoBuf = (SOPHGO_S_BUF *)pDstBuf;

    sprintf(writeName, "%s%d.jpg", name, suffix);
    fpJpg = drv_fopen(writeName, "wb");
    if (0 == fpJpg) {
        pr_err("Cann't create a file %s to write data\n", writeName);
        return 0;
    }

    data = vmalloc(sophgoBuf->u32Size);
    JLOG(INFO,"write_jpg_file header addr:%lx size:%ld\n", sophgoBuf->phys_addr, sophgoBuf->u32Size);
    jdi_read_memory(sophgoBuf->phys_addr, data, sophgoBuf->u32Size, 0);

    writeLen = drv_fwrite((unsigned char *)data, sizeof(unsigned char),
              sophgoBuf->u32Size, fpJpg);
    JLOG(INFO,"write_jpg_file header done writeLen:%d\n", writeLen);
    sophgoBuf +=1;
    data1 = vmalloc(sophgoBuf->u32Size);
    JLOG(INFO,"write_jpg_file body addr:%lx  size:%ld\n",sophgoBuf->phys_addr, sophgoBuf->u32Size);
    jdi_read_memory(sophgoBuf->phys_addr, data1, sophgoBuf->u32Size, 0);
    writeLen += drv_fwrite((unsigned char *)data1, sizeof(unsigned char),
              sophgoBuf->u32Size, fpJpg);
    JLOG(INFO,"write_jpg_file body done writeLen:%d\n", writeLen);

    drv_fclose(fpJpg);
    vfree(data);
    vfree(data1);
    fpJpg = NULL;

    return writeLen;
}


static int read_yuv_file(char *inFileName, unsigned char *pSrcBuf,
              SOPHGO_E_FRAME_FORMAT fmt)
{
    drv_file_t *fpSrc = NULL;
    int readLen = 0;
    int yDataLen = 0, cbDataLen = 0;
    SOPHGO_S_FRAME_BUF *buffer = (SOPHGO_S_FRAME_BUF *)pSrcBuf;
    if (NULL == buffer) {
        pr_err("alloc enc memory fail\n");
        goto FIND_ERROR;
    }

    if (0 != strcmp(inFileName, "")) {
        fpSrc = drv_fopen(inFileName, "rb");
        if (NULL == fpSrc) {
            pr_err("Cann't open input file %s\n", inFileName);
            goto FIND_ERROR;
        }
        yDataLen = buffer->s32Width * buffer->s32Height;
        #if 0 //only for 420
        if (0 != iPackedFormat) {
            if ((5 == iPackedFormat) || (6 == iPackedFormat))
                datLen *= 3;
            else
                datLen *= 2;
        }
        #endif
        readLen = drv_fread((unsigned char *)(buffer->vbY.virt_addr),
                sizeof(unsigned char), yDataLen,
                fpSrc);

        #if 0
        // [0](4:2:0) [1](4:2:2) [2](2:2:4 4:2:2 rotated) [3](4:4:4)
        // [4](4:0:0)
        dataLen = buffer->width * buffer->height;
        if (CVI_FORMAT_422 == buffer->Format ||
            CVI_FORMAT_224 == buffer->Format) {
            dataLen = dataLen >> 1;
        } else if (CVI_FORMAT_400 == buffer->Format) {
            dataLen = 0;
        } else if (CVI_FORMAT_420 == buffer->Format) {
            dataLen = dataLen >> 2;
        }
        #else
        cbDataLen = (yDataLen >> 2);  //only for 420
        #endif
        readLen += drv_fread((unsigned char *)(buffer->vbCb.virt_addr),
                 sizeof(unsigned char), cbDataLen, fpSrc);
        readLen += drv_fread((unsigned char *)(buffer->vbCr.virt_addr),
                 sizeof(unsigned char), cbDataLen, fpSrc);
    }

FIND_ERROR:
    if (NULL != fpSrc)
        drv_fclose(fpSrc);
    return readLen;
}

int jpeg_enc_main(int argc, char *argv[])
{

    int ret = 1;
    struct option longOpt[] = {
        { "yuv",                required_argument,    NULL, 0 },
        { "height",                required_argument,    NULL, 0 },
        { "width",                required_argument,    NULL, 0 },
        { "stream-endian",        required_argument,    NULL, 0 },
        { "frame-endian",        required_argument,    NULL, 0 },
        { "pixelj",             required_argument,    NULL, 0 },
        { "bs-size",            required_argument,    NULL, 0 },
        { "cfg-dir",            required_argument,    NULL, 0 },
        { "yuv-dir",            required_argument,    NULL, 0 },
        { "output",             required_argument,    NULL, 0 },
        { "input",                required_argument,    NULL, 0 },
        { "slice-height",        required_argument,    NULL, 0 },
        { "enable-slice-intr",    required_argument,    NULL, 0 },
        { "quality",            required_argument,    NULL, 0 },
        { "enable-tiledMode",    required_argument,    NULL, 0 },
        { "12bit",                no_argument,        NULL, 0 },
        { "fpgaReset",            required_argument,    NULL, 0 },
        { "rotation",            required_argument,    NULL, 0 },
        { "mirror",             required_argument,    NULL, 0 },
        { NULL,                 no_argument,        NULL, 0 },
    };
    const char*     shortOpt    = "fh";
    EncConfigParam* config;
    int32_t            c;
    int             l;
    char                yuvPath[MAX_FILE_PATH];
    unsigned char *srcBuf = NULL;
    SOPHGO_S_FRAME_BUF frame_buffer;
    int readLen;
    SOPHGO_S_BUF streamBuf[2];
    SOPHGO_S_JPEG_CONFIG stJpegConfig = {0};
    SOPHGO_JPEG_HANDLE_PTR handle;
    int alloclen;

    config = vzalloc(sizeof(EncConfigParam));
    if (config == NULL)
        return -1;

    /* Default configurations */
    config->bsSize = STREAM_BUF_SIZE;
    strcpy(config->strCfgDir, CFG_DIR);
    strcpy(config->strYuvDir, YUV_DIR);
    strcpy(config->bitstreamFileName, DEFAULT_OUTPUT_PATH);
    getopt_init();
    while ((c=getopt_long(argc, argv, shortOpt, longOpt, &l)) != -1) {
        switch (c) {
        case 0:
            if (ParseEncTestLongArgs((void*)config, longOpt[l].name, optarg) == FALSE) {
                Help(argv[0]);
            }
            break;
        case 'h':
        default:
            Help(argv[0]);
            break;
        }
    }

    ret = sophgo_jpeg_init();
    if(ret != JPG_RET_SUCCESS) {
        pr_err("sophgo_jpeg_init init failed.\n");
        vfree(config);
        return -1;
    }

    memset(&stJpegConfig, 0, sizeof(SOPHGO_S_JPEG_CONFIG));
    config->sourceSubsample = YUV_E_FORMAT_420;
    config->packedFormat = PACKED_FORMAT_NONE;
    //config.packedFormat

    stJpegConfig.eType = JPGCOD_E_ENC;
    stJpegConfig.u.stEncConfig.eSourceFormat = config->sourceSubsample;
    stJpegConfig.u.stEncConfig.s32PicWidth = config->picWidth;
    stJpegConfig.u.stEncConfig.s32PicHeight =  config->picHeight;
    stJpegConfig.u.stEncConfig.s32BitstreamBufSize = 4*1024*1024;
    stJpegConfig.u.stEncConfig.bEnStuffByte = config->bEnStuffByte;
    stJpegConfig.u.stEncConfig.eChromaInterleave = config->chromaInterleave;
    stJpegConfig.u.stEncConfig.encHeaderMode = config->encHeaderMode;
    stJpegConfig.u.stEncConfig.ePackedFormat = config->packedFormat;
    stJpegConfig.u.stEncConfig.RandRotMode = 0;
    stJpegConfig.u.stEncConfig.s32Framerate = 0;
    stJpegConfig.u.stEncConfig.s32MirDir = config->mirror;
    stJpegConfig.u.stEncConfig.s32Quality = 99; //config?
    //stJpegConfig.u.stEncConfig.s32RotAngle = config.
    stJpegConfig.u.stEncConfig.s32SrcType = 3;
    stJpegConfig.u.stEncConfig.s32Bitrate = 10000;
    stJpegConfig.u.stEncConfig.s32OutNum = 1;
    stJpegConfig.u.stEncConfig.s32SrcType = JPEG_MEM_E_MODULE;

    sprintf(yuvPath, "%s/%s", config->strYuvDir, config->yuvFileName);

    handle = sophgo_jpeg_open_instance(stJpegConfig);
    if(handle == NULL) {
        pr_err("sophgo_jpeg_open_instance open failed.\n");
        vfree(config);
        return -1;
    }

    /* read source file data */

    // read yuv data
    frame_buffer.s32Width = stJpegConfig.u.stEncConfig.s32PicWidth;
    frame_buffer.s32Height = stJpegConfig.u.stEncConfig.s32PicHeight;
    frame_buffer.eFormat = config->sourceSubsample;
    frame_buffer.s32StrideY = config->picWidth;
    frame_buffer.s32StrideC = config->picWidth;
    alloclen = config->picWidth * config->picHeight;
    if (0 != config->sourceSubsample) {
        if ((5 == config->sourceSubsample) || (6 == config->sourceSubsample))
            alloclen *= 3;
        else
            alloclen *= 2;
    }

    sophgo_jpeg_get_input_source_data(handle,(void *)&frame_buffer);

    srcBuf = (unsigned char *)&frame_buffer;
    readLen = read_yuv_file(yuvPath,
                 (unsigned char *)&frame_buffer, 0);
    if(readLen <= 0) {
        pr_err("read yuv failed.\n");
        vfree(config);
        return -1;
    }

    // send to jpu
    sophgo_jpeg_send_frame_data(handle, srcBuf, -1, 20000);

    //get jpeg data
    memset(streamBuf, 0, sizeof(SOPHGO_S_BUF)*2);
    ret = sophgo_jpeg_get_frame_data(handle,
                (unsigned char *)streamBuf,
                sizeof(SOPHGO_S_BUF), NULL);

    //save the jpeg
    write_jpg_file(config->bitstreamFileName,
               (unsigned char *)streamBuf, 0);

    //release the frame
    sophgo_jpeg_release_frame_data(handle);

    //close the instance
    ret = sophgo_jpeg_close_instance(handle);

    // deinit the jpu
    sophgo_jpeg_deinit();
    vfree(config);

    return 0;
}

int jpeg_enc_test(u_long arg)
{
#define MAX_ARG_CNT 30
    char buf[512];
    char *pArgv[MAX_ARG_CNT] = {0};
    char *save_ptr;
    unsigned int u32Argc = 0;
    char *pBuf;
    unsigned int __user *argp = (unsigned int __user *)arg;

    memset(buf, 0, 512);

    if (argp != NULL) {
        if (copy_from_user(buf, (char *)argp, 512))
            return -1;
    }

    pBuf = buf;

    while (NULL != (pArgv[u32Argc] = cvi_strtok_r(pBuf, " ", &save_ptr))) {
        u32Argc++;

        if (u32Argc >= MAX_ARG_CNT) {
            break;
        }

        pBuf = NULL;
    }

    return jpeg_enc_main(u32Argc, pArgv);
}


