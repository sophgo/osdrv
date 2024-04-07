/*
 * Copyright (c) 2018, Chips&Media
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "jpuapi.h"
#include "regdefine.h"
#include "jpulog.h"
#include "jpuapifunc.h"
#include "jpu_helper.h"
#include "sophgo_jpeg_interface.h"
#include "cvi_vc_getopt.h"

#define NUM_FRAME_BUF               MAX_FRAME
#define MAX_ROT_BUF_NUM             1

#ifdef SUPPORT_MULTI_INSTANCE_TEST
#else
static void Help(
    const char* programName
    )
{
    pr_err( "------------------------------------------------------------------------------\n");
    pr_err( " CODAJ12 Decoder\n");
    pr_err( "------------------------------------------------------------------------------\n");
    pr_err( "%s [options] --input=jpg_file_path\n", programName);
    pr_err( "-h                      help\n");
    pr_err( "--input=FILE            jpeg filepath\n");
    pr_err( "--output=FILE           output file path\n");
    pr_err( "--stream-endian=ENDIAN  bitstream endianness. refer to datasheet Chapter 4.\n");
    pr_err( "--frame-endian=ENDIAN   pixel endianness of 16bit input source. refer to datasheet Chapter 4.\n");
    pr_err( "--pixelj=JUSTIFICATION  16bit-pixel justification. 0(default) - msb justified, 1 - lsb justified in little-endianness\n");
    pr_err( "--bs-size=SIZE          bitstream buffer size in byte\n");
    pr_err( "--roi=x,y,w,h           ROI region\n");
    pr_err( "--subsample             conversion sub-sample(ignore case): NONE, 420, 422, 444\n");
    pr_err( "--ordering              conversion ordering(ingore-case): NONE, NV12, NV21, YUYV, YVYU, UYVY, VYUY, AYUV\n");
    pr_err( "                        NONE - planar format\n");
    pr_err( "                        NV12, NV21 - semi-planar format for all the subsamples.\n");
    pr_err( "                                     If subsample isn't defined or is none, the sub-sample depends on jpeg information\n");
    pr_err( "                                     The subsample 440 can be converted to the semi-planar format. It means that the encoded sub-sample should be 440.\n");
    pr_err( "                        YUVV..VYUY - packed format. subsample be ignored.\n");
    pr_err( "                        AYUV       - packed format. subsample be ignored.\n");
    pr_err( "--rotation              0, 90, 180, 270\n");
    pr_err( "--mirror                0(none), 1(V), 2(H), 3(VH)\n");
    pr_err( "--scaleH                Horizontal downscale: 0(none), 1(1/2), 2(1/4), 3(1/8)\n");
    pr_err( "--scaleV                Vertical downscale  : 0(none), 1(1/2), 2(1/4), 3(1/8)\n");

    return;
}
#endif /* SUPPORT_MULTI_INSTANCE_TEST */


static int write_yuv_file(char *name, unsigned char *pDstBuf, int suffix)
{
    int i;
    int writeLen = 0;
    int writeTotalLen = 0;
    char writeName[256];
    drv_file_t *fpYuv;
    SOPHGO_S_FRAME_BUF *pstFrameBuf = (SOPHGO_S_FRAME_BUF *)pDstBuf;
    unsigned char *addrY_v = (unsigned char *)(pstFrameBuf->vbY.virt_addr);
    unsigned char *addrCb_v = (unsigned char *)(pstFrameBuf->vbCb.virt_addr);
    unsigned char *addrCr_v = (unsigned char *)(pstFrameBuf->vbCr.virt_addr);

    unsigned char *address = addrY_v;
    int datLen = pstFrameBuf->s32Width;
    int iChromaHeight;
    int iChromaWidth;

    sprintf(writeName, "%s%d.yuv", name, suffix);
    fpYuv = drv_fopen(writeName, "wb");
    if (0 == fpYuv) {
        pr_err("Cann't create a file to write data\n");
        return 0;
    }

#ifdef OUTPUT_FOR_SCALER
    pr_err("s32StrideY = %d\n", pstFrameBuf->s32StrideY);
    writeLen = drv_fwrite((unsigned char *)address, sizeof(unsigned char),
              pstFrameBuf->s32Height * pstFrameBuf->s32StrideY, fpYuv);
#else
    for (i = 0; i < pstFrameBuf->s32Height; i++) {
        writeLen = drv_fwrite((unsigned char *)address,
                  sizeof(unsigned char), datLen, fpYuv);
        writeTotalLen += writeLen;
        address = address + pstFrameBuf->s32StrideY;
    }
#endif
    //only for yuv420

    iChromaHeight = pstFrameBuf->s32Height;
    iChromaWidth = pstFrameBuf->s32Width;
    switch (pstFrameBuf->eFormat) {
    case FORMAT_422:
        iChromaHeight = pstFrameBuf->s32Height;
        iChromaWidth = pstFrameBuf->s32Width >> 1;
        break;
    case FORMAT_444:
        iChromaHeight = pstFrameBuf->s32Height;
        iChromaWidth = pstFrameBuf->s32Width;
        break;
    case FORMAT_400:
        iChromaHeight = 0;
        iChromaWidth = 0;
        break;
    case FORMAT_420:
    default:
        iChromaHeight = pstFrameBuf->s32Height >> 1;
        iChromaWidth = pstFrameBuf->s32Width >> 1;
        break;
    }

#if 1
    address = addrCb_v;

    pr_err("iChromaHeight = %d, iChromaWidth = %d, s32StrideC = %d\n",
           iChromaHeight,iChromaWidth, pstFrameBuf->s32StrideC);

    writeLen = drv_fwrite((unsigned char *)address, sizeof(unsigned char),
              iChromaHeight * pstFrameBuf->s32StrideC, fpYuv);

    address = addrCr_v;

    writeLen = drv_fwrite((unsigned char *)address, sizeof(unsigned char),
              iChromaHeight * pstFrameBuf->s32StrideC, fpYuv);
#else
    address = addrCb_v;
    if (0 == iChromaInterLeave) {
        for (i = 0; i < iChromaHeight; i++) {
            writeLen = fwrite((unsigned char *)address,
                      sizeof(unsigned char), iChromaWidth,
                      fpYuv);
            writeTotalLen += writeLen;
            address = address + pstFrameBuf->s32StrideC;
        }

        address = addrCr_v;
        for (i = 0; i < iChromaHeight; i++) {
            writeLen = fwrite((unsigned char *)address,
                      sizeof(unsigned char), iChromaWidth,
                      fpYuv);
            writeTotalLen += writeLen;
            address = address + pstFrameBuf->s32StrideC;
        }
    } else {
        for (i = 0; i < iChromaHeight; i++) {
            writeLen = fwrite((unsigned char *)address,
                      sizeof(unsigned char),
                      pstFrameBuf->s32Width, fpYuv);
            writeTotalLen += writeLen;
            address = address + pstFrameBuf->s32StrideC;
        }
    }
#endif

//ERROR_WRITE_YUV:
    if (0 != fpYuv) {
        drv_fclose(fpYuv);
        fpYuv = NULL;
    }
    return writeTotalLen;
}


static int read_jpeg_file(char *inFileName, unsigned char **pSrcBuf)
{
    int srcLen = 0;
    int readLen = 0;
    if (0 != strcmp(inFileName, "")) {
        drv_file_t *fpSrc = drv_fopen(inFileName, "rb");
        if (NULL == fpSrc) {
            pr_err("Cann't open input file %s\n", inFileName);
            goto FIND_ERROR;
        }
        /* get file size */
        drv_fseek(fpSrc, 0, SEEK_END);
        srcLen = drv_ftell(fpSrc);
        drv_fseek(fpSrc, 0, SEEK_SET);
        *pSrcBuf = (unsigned char *)vmalloc(srcLen + 16);
        readLen = drv_fread(*pSrcBuf, sizeof(unsigned char), srcLen, fpSrc);
        if (srcLen != readLen) {
            pr_err("Some Wrong happend at read jpeg file??, readLen = %d, request = %d\n",
                   readLen, srcLen);
        }
        pr_err("readLen = 0x%X\n", readLen);
        drv_fclose(fpSrc);
    }

FIND_ERROR:
    return readLen;
}



static BOOL dec_TestDecoder(
    DecConfigParam *param
    )
{
    JpgDecHandle        handle        = {0};
    JpgRet              ret = JPG_RET_SUCCESS;
    BOOL                suc = FALSE;
    DecConfigParam      decConfig;
    unsigned long int cost_time;
    SOPHGO_S_JPEG_CONFIG stJpegConfig;
    unsigned char *srcBuf = NULL;
    SOPHGO_S_FRAME_BUF frame_buffer;
    int readLen = 0;

    memcpy(&decConfig, param, sizeof(DecConfigParam));

    ret = sophgo_jpeg_init();
    if (ret != JPG_RET_SUCCESS && ret != JPG_RET_CALLED_BEFORE) {
        suc = 0;
        JLOG(ERR, "JPU_Init failed Error code is 0x%x \n", ret );
        return -1;
    }

    memset(&stJpegConfig, 0 ,sizeof(SOPHGO_S_JPEG_CONFIG));

    stJpegConfig.eType = JPGCOD_E_DEC;
    stJpegConfig.u.stDecConfig.StreamEndian          = decConfig.StreamEndian;
    stJpegConfig.u.stDecConfig.FrameEndian           = decConfig.FrameEndian;
    stJpegConfig.u.stDecConfig.bitstreamBufferSize   = param->bsSize;
    stJpegConfig.u.stDecConfig.cbcrInterleave      = decConfig.cbcrInterleave;
    stJpegConfig.u.stDecConfig.packedFormat          = decConfig.packedFormat;
    stJpegConfig.u.stDecConfig.roiEnable             = decConfig.roiEnable;
    stJpegConfig.u.stDecConfig.roiOffsetX            = decConfig.roiOffsetX;
    stJpegConfig.u.stDecConfig.roiOffsetY            = decConfig.roiOffsetY;
    stJpegConfig.u.stDecConfig.roiWidth              = decConfig.roiWidth;
    stJpegConfig.u.stDecConfig.roiHeight             = decConfig.roiHeight;
    stJpegConfig.u.stDecConfig.rotation              = decConfig.rotation;
    stJpegConfig.u.stDecConfig.mirror                = decConfig.mirror;
    stJpegConfig.u.stDecConfig.iHorScaleMode         = 0;
    stJpegConfig.u.stDecConfig.iVerScaleMode         = 0;
    stJpegConfig.u.stDecConfig.pixelJustification    = decConfig.pixelJustification;
    stJpegConfig.u.stDecConfig.subsample          = decConfig.subsample;
    handle = sophgo_jpeg_open_instance( stJpegConfig);
    if( ret != JPG_RET_SUCCESS ) {
        JLOG(ERR, "JPU_DecOpen failed Error code is 0x%x \n", ret );
        goto ERROR_JPEG_DEC;
    }

    //read jpeg data from file
    /* read source file data */
    memset(&frame_buffer, 0, sizeof(SOPHGO_S_FRAME_BUF));

    readLen = read_jpeg_file(param->bitstreamFileName , &srcBuf);
    if (0 == readLen)
        goto ERROR_JPEG_DEC;

    //send jpeg data to jpu
    ret = sophgo_jpeg_send_frame_data(handle, srcBuf, readLen, 2000);


    //get the yuv data from jpu
    ret = sophgo_jpeg_get_frame_data(handle, (char *)&frame_buffer, -1, &cost_time);

    //save the yuv
    write_yuv_file(param->yuvFileName, (unsigned char *)&frame_buffer, 0);

    //release
    sophgo_jpeg_release_frame_data(handle);

    //close the instance
    sophgo_jpeg_close_instance(handle);

    //deinit

ERROR_JPEG_DEC:
    vfree(srcBuf);
    return suc;
}


int jpeg_dec_main(int argc, char** argv)
{
    Int32   ret = 1;
    struct option longOpt[] = {
        { "stream-endian",      required_argument, NULL, 0 },
        { "frame-endian",       required_argument, NULL, 0 },
        { "output",             required_argument, NULL, 0 },
        { "input",              required_argument, NULL, 0 },
        { "pixelj",             required_argument, NULL, 0 },
        { "bs-size",            required_argument, NULL, 0 },
        { "roi",                required_argument, NULL, 0 },
        { "subsample",          required_argument, NULL, 0 },
        { "ordering",           required_argument, NULL, 0 },
        { "rotation",           required_argument, NULL, 0 },
        { "mirror",             required_argument, NULL, 0 },
        { "scaleH",             required_argument, NULL, 0 },
        { "scaleV",             required_argument, NULL, 0 },
        { "fpgaReset",          required_argument, NULL, 0 },
        { NULL,                 no_argument,       NULL, 0 },
    };
    Int32           c;
    int             l;
    const char*     shortOpt    = "fh";
    DecConfigParam  config;
    TestDevConfig   devConfig   = { ACLK_MIN, CCLK_MIN, TRUE };

    memset((void*)&config, 0x00, sizeof(DecConfigParam));
    config.subsample = FORMAT_MAX;

    getopt_init();
    while ((c=getopt_long(argc, argv, shortOpt, longOpt, &l)) != -1) {
        switch (c) {
        case 'h':
            Help(argv[0]);
            break;
        case 'f':
            devConfig.reset = FALSE;
            break;
        case 0:
            if (strcmp(longOpt[l].name, "aclk") == 0) {
                devConfig.aclk=atoi(optarg);
                if (devConfig.aclk < ACLK_MIN || devConfig.aclk > ACLK_MAX) {
                    JLOG(ERR, "Invalid ACLK(%d) valid range(%d ~ %d)\n", devConfig.aclk, ACLK_MIN, ACLK_MAX);
                    Help(argv[0]);
                }
            }
            else if (strcmp(longOpt[l].name, "cclk") == 0) {
                devConfig.cclk=atoi(optarg);
                if (devConfig.cclk < CCLK_MIN || devConfig.cclk > CCLK_MAX) {
                    JLOG(ERR, "Invalid CCLK(%d) valid range(%d ~ %d)\n", devConfig.cclk, CCLK_MIN, CCLK_MAX);
                    Help(argv[0]);
                }
            }
            else if (strcmp(longOpt[l].name, "aclk_div") == 0) {
                devConfig.aclk_div = atoi(optarg);
            }
            else if (strcmp(longOpt[l].name, "cclk_div") == 0) {
                devConfig.cclk_div = atoi(optarg);
            }
            else if (strcmp(longOpt[l].name, "pf") == 0) {
                config.pf_en = 1;
            }
            else {
                if (ParseDecTestLongArgs((void*)&config, longOpt[l].name, optarg) == FALSE) {
                    Help(argv[0]);
                }
            }
            break;
        default:
            Help(argv[0]);
            break;
        }
    }

    /* CHECK PARAMETERS */
    if ((config.iHorScaleMode || config.iVerScaleMode) && config.roiEnable) {
        JLOG(ERR, "Invalid operation mode : ROI cannot work with the scaler\n");
        return 1;
    }
    if(config.packedFormat && config.roiEnable) {
        JLOG(ERR, "Invalid operation mode : ROI cannot work with the packed format conversion\n");
        return 1;
    }
    if (config.roiEnable && (config.rotation || config.mirror)) {
        JLOG(ERR, "Invalid operation mode : ROI cannot work with the PPU.\n");
    }

    if (config.packedFormat == PACKED_FORMAT_422_YUYV ||
        config.packedFormat == PACKED_FORMAT_422_UYVY ||
        config.packedFormat == PACKED_FORMAT_422_YVYU ||
        config.packedFormat == PACKED_FORMAT_422_VYUY ||
        config.packedFormat == PACKED_FORMAT_444) {
        if (config.subsample != FORMAT_MAX) {
            JLOG(ERR, "Invalid operation mode : subsample cannot be enabled if ordering is YUYV or UYVY or YVYU or VYUY or AYUV\n");
            return 1;
        }
    }

    ret = dec_TestDecoder(&config);

    return ret == TRUE ? 0 : 1;
}

int jpeg_dec_test(u_long arg)
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

    return jpeg_dec_main(u32Argc, pArgv);
}



