#ifndef __SOPHGO_JPEG_INTERNAL_H__
#define __SOPHGO_JPEG_INTERNAL_H__

#include "sophgo_jpeg_interface.h"
#include "jpuapi.h"
#include "jpulog.h"
#include "jpuapifunc.h"

#define CHECK_PTR_RET_VOID(ptr)             \
    ({                                      \
        if(ptr == NULL) {                   \
            JLOG(ERR, "NULL pointer\n");    \
            return;                         \
        }                                   \
    })

#define CHECK_PTR_RET_INT(ptr)              \
    ({                                      \
        if(ptr == NULL) {                   \
            JLOG(ERR, "NULL pointer\n");    \
            return -1;                      \
        }                                   \
    })


/* DEC */
int sophgo_jpeg_dec_open_instance(SOPHGO_JPEG_HANDLE_PTR *pvJpgHandle, SOPHGO_S_DEC_CONFIG_PARAM stConfig);
int sophgo_jpeg_dec_close_instance(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle);
int sophgo_jpeg_dec_send_frame_data(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, void *pvData, int s32Length);
int sophgo_jpeg_dec_get_frame_data(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, SOPHGO_S_FRAME_BUF *pstFrameBuf);
int sophgo_jpeg_dec_release_frame_data(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle);
int sophgo_jpeg_dec_get_valid_frame_count(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle);



/* ENC */
int sophgo_jpeg_enc_open_instance(SOPHGO_JPEG_HANDLE_PTR *pvJpgHandle, SOPHGO_S_ENC_CONFIG_PARAM stConfig);
int sophgo_jpeg_enc_close_instance(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle);
int sophgo_jpeg_enc_send_frame_data(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, SOPHGO_S_FRAME_BUF *pstFrame);
int sophgo_jpeg_enc_get_stream(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, void *pvData);
int sophgo_jpeg_enc_get_input_source_data(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, SOPHGO_S_FRAME_BUF *pvData);
int sophgo_jpeg_enc_release_frame(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle);
int sophgo_jpeg_enc_get_valid_bitstream_count(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle);
int sophgo_jpeg_enc_get_quality(SOPHGO_JPEG_HANDLE_PTR *pvJpgHandle, void *pvData);
int sophgo_jpeg_enc_set_quality(SOPHGO_JPEG_HANDLE_PTR *pvJpgHandle, void *pvData);





#endif

