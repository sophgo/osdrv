/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_ive_ioctl.h
 * Description:
 */

#ifndef __CVI_IVE_IOCTL_H__
#define __CVI_IVE_IOCTL_H__

#include "cvi_comm_ive.h"

struct IVE_KEN_TEST_S {
	IVE_IMAGE_TYPE_E enType;

	char *pAddr;
	CVI_U16 u16Width;
	CVI_U16 u16Height;
};

struct cvi_ive_ioctl_add_arg {
	IVE_HANDLE pIveHandle;
	IVE_SRC_IMAGE_S *pstSrc1;
	IVE_SRC_IMAGE_S *pstSrc2;
	IVE_DST_IMAGE_S *pstDst;
	IVE_ADD_CTRL_S *pstAddCtrl;
	bool bInstant;
};

struct cvi_ive_ioctl_and_arg {
	IVE_HANDLE pIveHandle;
	IVE_SRC_IMAGE_S *pstSrc1;
	IVE_SRC_IMAGE_S *pstSrc2;
	IVE_DST_IMAGE_S *pstDst;
	bool bInstant;
};

struct cvi_ive_ioctl_xor_arg {
	IVE_HANDLE pIveHandle;
	IVE_SRC_IMAGE_S *pstSrc1;
	IVE_SRC_IMAGE_S *pstSrc2;
	IVE_DST_IMAGE_S *pstDst;
	bool bInstant;
};

struct cvi_ive_ioctl_or_arg {
	IVE_HANDLE pIveHandle;
	IVE_SRC_IMAGE_S *pstSrc1;
	IVE_SRC_IMAGE_S *pstSrc2;
	IVE_DST_IMAGE_S *pstDst;
	bool bInstant;
};

struct cvi_ive_ioctl_sub_arg {
	IVE_HANDLE pIveHandle;
	IVE_SRC_IMAGE_S *pstSrc1;
	IVE_SRC_IMAGE_S *pstSrc2;
	IVE_DST_IMAGE_S *pstDst;
	IVE_SUB_CTRL_S *pstSubCtrl;
	bool bInstant;
};

struct cvi_ive_ioctl_dilate_erode_arg {
	IVE_HANDLE pIveHandle;
	IVE_SRC_IMAGE_S *pstSrc;
	IVE_DST_IMAGE_S *pstDst;
	IVE_ELEMENT_STRUCTURE_CTRL_S *pstCtrl;
	bool bInstant;
};

struct cvi_ive_ioctl_thresh_arg {
	IVE_HANDLE pIveHandle;
	IVE_SRC_IMAGE_S *pstSrc;
	IVE_DST_IMAGE_S *pstDst;
	IVE_THRESH_CTRL_S *pstCtrl;
	bool bInstant;
};

struct cvi_ive_ioctl_bgmodel_arg {
	IVE_HANDLE pIveHandle;
	IVE_SRC_IMAGE_S *pstCurImg;
	IVE_DATA_S *pstBgModel;
	IVE_IMAGE_S *pstFgFlag;
	IVE_DST_IMAGE_S *pstBgDiffFg;
	IVE_DST_IMAGE_S *pstFrmDiffFg;
	IVE_DST_MEM_INFO_S *pstStatData;
	IVE_MATCH_BG_MODEL_CTRL_S *pstCtrl;
	bool bInstant;
};

struct cvi_ive_ioctl_gmm_arg {
	IVE_HANDLE pIveHandle;
	IVE_SRC_IMAGE_S *pstSrc;
	IVE_DST_IMAGE_S *pstFg;
	IVE_DST_IMAGE_S *pstBg;
	IVE_MEM_INFO_S *pstModel;
	IVE_GMM_CTRL_S *pstCtrl;
	bool bInstant;
};

struct cvi_ive_ioctl_gmm2_arg {
	IVE_HANDLE pIveHandle;
	IVE_SRC_IMAGE_S *pstSrc;
	IVE_SRC_IMAGE_S *pstFactor;
	IVE_DST_IMAGE_S *pstFg;
	IVE_DST_IMAGE_S *pstBg;
	IVE_DST_IMAGE_S *pstMatchModelInfo;
	IVE_MEM_INFO_S *pstModel;
	IVE_GMM2_CTRL_S *pstCtrl;
	bool bInstant;
};

#define CVI_IVE_IOC_MAGIC 'v'
#define CVI_IVE_IOC_TEST _IOWR(CVI_IVE_IOC_MAGIC, 0x00, unsigned long)
#define CVI_IVE_IOC_DMA _IOWR(CVI_IVE_IOC_MAGIC, 0x01, unsigned long)
#define CVI_IVE_IOC_Add _IOWR(CVI_IVE_IOC_MAGIC, 0x02, unsigned long)
#define CVI_IVE_IOC_And _IOWR(CVI_IVE_IOC_MAGIC, 0x03, unsigned long)
#define CVI_IVE_IOC_Or _IOWR(CVI_IVE_IOC_MAGIC, 0x04, unsigned long)
#define CVI_IVE_IOC_Sub _IOWR(CVI_IVE_IOC_MAGIC, 0x05, unsigned long)
#define CVI_IVE_IOC_Xor _IOWR(CVI_IVE_IOC_MAGIC, 0x06, unsigned long)
#define CVI_IVE_IOC_Thresh _IOWR(CVI_IVE_IOC_MAGIC, 0x07, unsigned long)
#define CVI_IVE_IOC_Thresh_S16 _IOWR(CVI_IVE_IOC_MAGIC, 0x08, unsigned long)
#define CVI_IVE_IOC_Thresh_U16 _IOWR(CVI_IVE_IOC_MAGIC, 0x09, unsigned long)
#define CVI_IVE_IOC_16BitTo8Bit _IOWR(CVI_IVE_IOC_MAGIC, 0x0a, unsigned long)
#define CVI_IVE_IOC_CSC _IOWR(CVI_IVE_IOC_MAGIC, 0x0b, unsigned long)
#define CVI_IVE_IOC_GradFg _IOWR(CVI_IVE_IOC_MAGIC, 0x0c, unsigned long)
#define CVI_IVE_IOC_NormGrad _IOWR(CVI_IVE_IOC_MAGIC, 0x0d, unsigned long)
#define CVI_IVE_IOC_Filter _IOWR(CVI_IVE_IOC_MAGIC, 0x0e, unsigned long)
#define CVI_IVE_IOC_FilterAndCSC _IOWR(CVI_IVE_IOC_MAGIC, 0x0f, unsigned long)
#define CVI_IVE_IOC_Hist _IOWR(CVI_IVE_IOC_MAGIC, 0x10, unsigned long)
#define CVI_IVE_IOC_EqualizeHist _IOWR(CVI_IVE_IOC_MAGIC, 0x11, unsigned long)
#define CVI_IVE_IOC_Map _IOWR(CVI_IVE_IOC_MAGIC, 0x12, unsigned long)
#define CVI_IVE_IOC_NCC _IOWR(CVI_IVE_IOC_MAGIC, 0x13, unsigned long)
#define CVI_IVE_IOC_OrdStatFilter _IOWR(CVI_IVE_IOC_MAGIC, 0x14, unsigned long)
#define CVI_IVE_IOC_Resize _IOWR(CVI_IVE_IOC_MAGIC, 0x15, unsigned long)
#define CVI_IVE_IOC_CannyHysEdge _IOWR(CVI_IVE_IOC_MAGIC, 0x16, unsigned long)
#define CVI_IVE_IOC_CannyEdge _IOWR(CVI_IVE_IOC_MAGIC, 0x17, unsigned long)
#define CVI_IVE_IOC_Integ _IOWR(CVI_IVE_IOC_MAGIC, 0x18, unsigned long)
#define CVI_IVE_IOC_LBP _IOWR(CVI_IVE_IOC_MAGIC, 0x19, unsigned long)
#define CVI_IVE_IOC_MagAndAng _IOWR(CVI_IVE_IOC_MAGIC, 0x1a, unsigned long)
#define CVI_IVE_IOC_STCandiCorner _IOWR(CVI_IVE_IOC_MAGIC, 0x1b, unsigned long)
#define CVI_IVE_IOC_STCorner _IOWR(CVI_IVE_IOC_MAGIC, 0x1c, unsigned long)
#define CVI_IVE_IOC_Sobel _IOWR(CVI_IVE_IOC_MAGIC, 0x1d, unsigned long)
#define CVI_IVE_IOC_CCL _IOWR(CVI_IVE_IOC_MAGIC, 0x1e, unsigned long)
#define CVI_IVE_IOC_Dilate _IOWR(CVI_IVE_IOC_MAGIC, 0x1f, unsigned long)
#define CVI_IVE_IOC_Erode _IOWR(CVI_IVE_IOC_MAGIC, 0x20, unsigned long)
#define CVI_IVE_IOC_MatchBgModel _IOWR(CVI_IVE_IOC_MAGIC, 0x21, unsigned long)
#define CVI_IVE_IOC_UpdateBgModel _IOWR(CVI_IVE_IOC_MAGIC, 0x22, unsigned long)
#define CVI_IVE_IOC_GMM _IOWR(CVI_IVE_IOC_MAGIC, 0x23, unsigned long)
#define CVI_IVE_IOC_GMM2 _IOWR(CVI_IVE_IOC_MAGIC, 0x24, unsigned long)
#define CVI_IVE_IOC_LKOpticalFlowPyr                                           \
	_IOWR(CVI_IVE_IOC_MAGIC, 0x25, unsigned long)
#define CVI_IVE_IOC_SAD _IOWR(CVI_IVE_IOC_MAGIC, 0x26, unsigned long)
#define CVI_IVE_IOC_Bernsen _IOWR(CVI_IVE_IOC_MAGIC, 0x27, unsigned long)
#define CVI_IVE_IOC_imgInToOdma _IOWR(CVI_IVE_IOC_MAGIC, 0x28, unsigned long)
#define CVI_IVE_IOC_rgbPToYuvToErodeToDilate                                   \
	_IOWR(CVI_IVE_IOC_MAGIC, 0x29, unsigned long)

#endif /* __CVI_IVE_IOCTL_H__ */
