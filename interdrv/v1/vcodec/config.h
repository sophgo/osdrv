/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: config.h
 * Description:
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) ||                \
	defined(WIN32) || defined(__MINGW32__)
#define PLATFORM_WIN32
#elif defined(linux) || defined(__linux) || defined(ANDROID)
#define PLATFORM_LINUX
#elif defined(unix) || defined(__unix)
#define PLATFORM_QNX
#else
#define PLATFORM_NON_OS
#define SUPPORT_DONT_READ_STREAM
#endif

#if defined(_MSC_VER)
#include <windows.h>
#define inline _inline
#elif defined(__GNUC__)
#elif defined(__ARMCC__)
#else
#error "Unknown compiler."
#endif

#define API_VERSION_MAJOR 5
#define API_VERSION_MINOR 5
#define API_VERSION_PATCH 34
#define API_VERSION                                                            \
	((API_VERSION_MAJOR << 16) | (API_VERSION_MINOR << 8) |                \
	 API_VERSION_PATCH)

#if defined(PLATFORM_NON_OS) || defined(ANDROID) || defined(MFHMFT_EXPORTS) || \
	defined(PLATFORM_QNX) || defined(_LINUX_)
//#define SUPPORT_FFMPEG_DEMUX
#else
#define SUPPORT_FFMPEG_DEMUX
#endif

//------------------------------------------------------------------------------
// COMMON
//------------------------------------------------------------------------------

// do not define BIT_CODE_FILE_PATH in case of multiple product support. because wave410 and coda980 has different firmware binary format.
#define CORE_0_BIT_CODE_FILE_PATH "cezzane.bin" // for wave420
#define CORE_1_BIT_CODE_FILE_PATH "coda980.out" // for coda980
#define CORE_2_BIT_CODE_FILE_PATH "pissarro.bin" // for wave412
#define CORE_3_BIT_CODE_FILE_PATH "michelangel.bin" // for wave410
#define CORE_4_BIT_CODE_FILE_PATH "coda960.out" // for coda960
#define CORE_5_BIT_CODE_FILE_PATH "monet.bin" // for wave420L
#define CORE_6_BIT_CODE_FILE_PATH "mondrian.bin" // for wave510
#define CORE_7_BIT_CODE_FILE_PATH "picasso.bin" // for wave510
#define CORE_8_BIT_CODE_FILE_PATH "kepler.bin" // for wave515
#define CORE_9_BIT_CODE_FILE_PATH "millet.bin" // for wave520

//------------------------------------------------------------------------------
// OMX
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// CODA980
//------------------------------------------------------------------------------
//#define SUPPORT_ROI_50
#define SUPPORT_LIB_THEORA
//#define AUTO_FRM_SKIP_DROP
#define CLIP_PIC_DELTA_QP

#define SUPPORT_980_ROI_RC_LIB
#define ROI_MB_RC
#define RC_PIC_PARACHANGE

//------------------------------------------------------------------------------
// WAVE420
//------------------------------------------------------------------------------
//#define SUPPORT_ENCODE_CUSTOM_HEADER    // to make VUI/HRD/SEI data
#ifdef SUPPORT_ENCODE_CUSTOM_HEADER
//#define TEST_ENCODE_CUSTOM_HEADER
#endif

#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_H__ */
