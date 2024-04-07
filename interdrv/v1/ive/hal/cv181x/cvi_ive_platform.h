/*
 * Copyright (C) Cvitek Co., Ltd. 2021-2022. All rights reserved.
 *
 * File Name: cvi_ive_platform.h
 * Description: cvitek ive driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef _CVI_IVE_PLATFORM_H_
#define _CVI_IVE_PLATFORM_H_

#ifdef __cplusplus
extern "C" {
#endif /* End of #ifdef __cplusplus */

#include "cvi_comm_ive.h"
#include "cvi_ive_interface.h"
#include "cvi_ive_ioctl.h"
#include "cvi_ive_reg.h"
#include "cvi_reg.h"

struct ive_block {
	void __iomem *IVE_TOP; /* NULL if not initialized. */
	void __iomem *IMG_IN; /* NULL if not initialized. */
	void __iomem *RDMA_IMG1; /* NULL if not initialized. */
	void __iomem *MAP; /* NULL if not initialized. */
	void __iomem *HIST; /* NULL if not initialized. */
	void __iomem *INTG; /* NULL if not initialized. */
	void __iomem *SAD; /* NULL if not initialized. */
	void __iomem *SAD_WDMA; /* NULL if not initialized. */
	void __iomem *SAD_WDMA_THR; /* NULL if not initialized. */
	void __iomem *NCC; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_RDMA_0; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_RDMA_1; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_RDMA_2; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_RDMA_3; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_RDMA_4; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_WDMA_0; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_WDMA_1; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_WDMA_2; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_WDMA_3; /* NULL if not initialized. */
	void __iomem *GMM_MODEL_WDMA_4; /* NULL if not initialized. */
	void __iomem *GMM_MATCH_WDMA; /* NULL if not initialized. */
	void __iomem *GMM; /* NULL if not initialized. */
	void __iomem *BG_MATCH_FGFLAG_RDMA; /* NULL if not initialized. */
	void __iomem *BG_MATCH_BGMODEL_0_RDMA; /* NULL if not initialized. */
	void __iomem *BG_MATCH_BGMODEL_1_RDMA; /* NULL if not initialized. */
	void __iomem *BG_MATCH_DIFFFG_WDMA; /* NULL if not initialized. */
	void __iomem *BG_MATCH_IVEMATCH_BG; /* NULL if not initialized. */
	void __iomem *BG_UPDATE; /* NULL if not initialized. */
	void __iomem *FILTEROP_RDMA; /* NULL if not initialized. */
	void __iomem *FILTEROP_WDMA_Y; /* NULL if not initialized. */
	void __iomem *FILTEROP_WDMA_C; /* NULL if not initialized. */
	void __iomem *FILTEROP; /* NULL if not initialized. */

	void __iomem *CCL; /* NULL if not initialized. */
	void __iomem *CCL_SRC_RDMA; /* NULL if not initialized. */
	void __iomem *CCL_DST_WDMA; /* NULL if not initialized. */
	void __iomem *CCL_REGION_WDMA; /* NULL if not initialized. */

	void __iomem *DMAF; /* NULL if not initialized. */
	void __iomem *DMAF_WDMA; /* NULL if not initialized. */
	void __iomem *DMAF_RDMA; /* NULL if not initialized. */
	void __iomem *LK; /* NULL if not initialized. */
	void __iomem *RDMA_EIGVAL; /* NULL if not initialized. */
	void __iomem *WDMA; /* NULL if not initialized. */
	void __iomem *RDMA; /* NULL if not initialized. */
};

CVI_S32 assign_ive_block_addr(struct cvi_ive_device *ndev);

irqreturn_t platform_ive_irq(struct cvi_ive_device *ndev);

CVI_S32 cvi_ive_test(struct cvi_ive_device *ndev, char *addr, CVI_U16 *w,
		     CVI_U16 *h);

CVI_S32 cvi_ive_DMA(struct cvi_ive_device *ndev, IVE_DATA_S *pstSrc,
		    IVE_DST_DATA_S *pstDst, IVE_DMA_CTRL_S *pstDmaCtrl,
		    bool bInstant);

CVI_S32 cvi_ive_And(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
		    IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
		    bool bInstant);

CVI_S32 cvi_ive_Or(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
		   IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
		   bool bInstant);

CVI_S32 cvi_ive_Xor(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
		    IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
		    bool bInstant);

CVI_S32 cvi_ive_Add(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
		    IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
		    IVE_ADD_CTRL_S *pstAddCtrl, bool bInstant);

CVI_S32 cvi_ive_Sub(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
		    IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
		    IVE_SUB_CTRL_S *pstSubCtrl, bool bInstant);

CVI_S32 cvi_ive_Thresh(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		       IVE_DST_IMAGE_S *pstDst, IVE_THRESH_CTRL_S *pstThrCtrl,
		       bool bInstant);

CVI_S32 cvi_ive_Dilate(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		       IVE_DST_IMAGE_S *pstDst,
		       IVE_DILATE_CTRL_S *pstDilateCtrl, bool bInstant);

CVI_S32 cvi_ive_Erode(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		      IVE_DST_IMAGE_S *pstDst, IVE_ERODE_CTRL_S *pstErodeCtrl,
		      bool bInstant);

CVI_S32 cvi_ive_MatchBgModel(struct cvi_ive_device *ndev,
			     IVE_SRC_IMAGE_S *pstCurImg, IVE_DATA_S *pstBgModel,
			     IVE_IMAGE_S *pstFgFlag,
			     IVE_DST_IMAGE_S *pstBgDiffFg,
			     IVE_DST_IMAGE_S *pstFrmDiffFg,
			     IVE_DST_MEM_INFO_S *pstStatData,
			     IVE_MATCH_BG_MODEL_CTRL_S *pstMatchBgModelCtrl,
			     bool bInstant);

CVI_S32 cvi_ive_GMM(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		    IVE_DST_IMAGE_S *pstFg, IVE_DST_IMAGE_S *pstBg,
		    IVE_MEM_INFO_S *pstModel, IVE_GMM_CTRL_S *pstGmmCtrl,
		    bool bInstant);

CVI_S32 cvi_ive_GMM2(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		     IVE_SRC_IMAGE_S *pstFactor, IVE_DST_IMAGE_S *pstFg,
		     IVE_DST_IMAGE_S *pstBg, IVE_DST_IMAGE_S *pstMatchModelInfo,
		     IVE_MEM_INFO_S *pstModel, IVE_GMM2_CTRL_S *pstGmm2Ctrl,
		     bool bInstant);

CVI_S32 cvi_ive_Bernsen(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			IVE_DST_IMAGE_S *pstDst, IVE_BERNSEN_CTRL_S *pstLbpCtrl,
			bool bInstant);

CVI_S32 cvi_ive_Filter(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		       IVE_DST_IMAGE_S *pstDst, IVE_FILTER_CTRL_S *pstFltCtrl,
		       bool bInstant);

CVI_S32 cvi_ive_CSC(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		    IVE_DST_IMAGE_S *pstDst, IVE_CSC_CTRL_S *pstCscCtrl,
		    bool bInstant);

CVI_S32 cvi_ive_FilterAndCSC(struct cvi_ive_device *ndev,
			     IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
			     IVE_FILTER_AND_CSC_CTRL_S *pstFltCscCtrl,
			     bool bInstant);

CVI_S32 cvi_ive_Sobel(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
		      IVE_DST_IMAGE_S *pstDstH, IVE_DST_IMAGE_S *pstDstV,
		      IVE_SOBEL_CTRL_S *pstSobelCtrl, bool bInstant);

CVI_S32 cvi_ive_MagAndAng(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			  IVE_DST_IMAGE_S *pstDstMag,
			  IVE_DST_IMAGE_S *pstDstAng,
			  IVE_MAG_AND_ANG_CTRL_S *pstMagAndAngCtrl,
			  bool bInstant);

CVI_S32 CVI_MPI_IVE_Integ(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			  IVE_DST_IMAGE_S *pstDst,
			  IVE_INTEG_CTRL_S *pstIntegCtrl, bool bInstant);

CVI_S32 CVI_MPI_IVE_Hist(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			 IVE_DST_MEM_INFO_S *pstDst, bool bInstant);

CVI_S32 CVI_MPI_IVE_EqualizeHist(IVE_HANDLE *pIveHandle,
				 IVE_SRC_IMAGE_S *pstSrc,
				 IVE_DST_IMAGE_S *pstDst,
				 IVE_EQUALIZE_HIST_CTRL_S *pstEqualizeHistCtrl,
				 bool bInstant);

CVI_S32 CVI_MPI_IVE_Thresh_S16(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			       IVE_DST_IMAGE_S *pstDst,
			       IVE_THRESH_S16_CTRL_S *pstThrS16Ctrl,
			       bool bInstant);

CVI_S32 CVI_MPI_IVE_Thresh_U16(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			       IVE_DST_IMAGE_S *pstDst,
			       IVE_THRESH_U16_CTRL_S *pstThrU16Ctrl,
			       bool bInstant);

CVI_S32 CVI_MPI_IVE_16BitTo8Bit(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
				IVE_DST_IMAGE_S *pstDst,
				IVE_16BIT_TO_8BIT_CTRL_S *pst16BitTo8BitCtrl,
				bool bInstant);

CVI_S32 CVI_MPI_IVE_OrdStatFilter(IVE_HANDLE *pIveHandle,
				  IVE_SRC_IMAGE_S *pstSrc,
				  IVE_DST_IMAGE_S *pstDst,
				  IVE_ORD_STAT_FILTER_CTRL_S *pstOrdStatFltCtrl,
				  bool bInstant);

CVI_S32 CVI_MPI_IVE_Map(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			IVE_SRC_MEM_INFO_S *pstMap, IVE_DST_IMAGE_S *pstDst,
			IVE_MAP_CTRL_S *pstMapCtrl, bool bInstant);

CVI_S32 CVI_MPI_IVE_NCC(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
			IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_MEM_INFO_S *pstDst,
			bool bInstant);

CVI_S32 CVI_MPI_IVE_CCL(IVE_HANDLE *pIveHandle, IVE_IMAGE_S *pstSrcDst,
			IVE_DST_MEM_INFO_S *pstBlob, IVE_CCL_CTRL_S *pstCclCtrl,
			bool bInstant);

CVI_S32 CVI_MPI_IVE_CannyHysEdge(IVE_HANDLE *pIveHandle,
				 IVE_SRC_IMAGE_S *pstSrc,
				 IVE_DST_IMAGE_S *pstEdge,
				 IVE_DST_MEM_INFO_S *pstStack,
				 IVE_CANNY_HYS_EDGE_CTRL_S *pstCannyHysEdgeCtrl,
				 bool bInstant);

CVI_S32 CVI_MPI_IVE_CannyEdge(IVE_IMAGE_S *pstEdge, IVE_MEM_INFO_S *pstStack);

CVI_S32 CVI_MPI_IVE_LBP(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			IVE_DST_IMAGE_S *pstDst, IVE_LBP_CTRL_S *pstLbpCtrl,
			bool bInstant);

CVI_S32 CVI_MPI_IVE_NormGrad(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			     IVE_DST_IMAGE_S *pstDstH, IVE_DST_IMAGE_S *pstDstV,
			     IVE_DST_IMAGE_S *pstDstHV,
			     IVE_NORM_GRAD_CTRL_S *pstNormGradCtrl,
			     bool bInstant);

CVI_S32 CVI_MPI_IVE_NormGrad(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			     IVE_DST_IMAGE_S *pstDstH, IVE_DST_IMAGE_S *pstDstV,
			     IVE_DST_IMAGE_S *pstDstHV,
			     IVE_NORM_GRAD_CTRL_S *pstNormGradCtrl,
			     bool bInstant);

CVI_S32 CVI_MPI_IVE_LKOpticalFlowPyr(
	IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S astSrcPrevPyr[],
	IVE_SRC_IMAGE_S astSrcNextPyr[], IVE_SRC_MEM_INFO_S *pstPrevPts,
	IVE_MEM_INFO_S *pstNextPts, IVE_DST_MEM_INFO_S *pstStatus,
	IVE_DST_MEM_INFO_S *pstErr,
	IVE_LK_OPTICAL_FLOW_PYR_CTRL_S *pstLkOptiFlowPyrCtrl, bool bInstant);

CVI_S32
CVI_MPI_IVE_STCandiCorner(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			  IVE_DST_IMAGE_S *pstCandiCorner,
			  IVE_ST_CANDI_CORNER_CTRL_S *pstStCandiCornerCtrl,
			  bool bInstant);

CVI_S32 CVI_MPI_IVE_STCorner(IVE_SRC_IMAGE_S *pstCandiCorner,
			     IVE_DST_MEM_INFO_S *pstCorner,
			     IVE_ST_CORNER_CTRL_S *pstStCornerCtrl);

CVI_S32 CVI_MPI_IVE_GradFg(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstBgDiffFg,
			   IVE_SRC_IMAGE_S *pstCurGrad,
			   IVE_SRC_IMAGE_S *pstBgGrad,
			   IVE_DST_IMAGE_S *pstGradFg,
			   IVE_GRAD_FG_CTRL_S *pstGradFgCtrl, bool bInstant);

CVI_S32 CVI_MPI_IVE_UpdateBgModel(
	IVE_HANDLE *pIveHandle, IVE_DATA_S *pstBgModel, IVE_IMAGE_S *pstFgFlag,
	IVE_DST_IMAGE_S *pstBgImg, IVE_DST_IMAGE_S *pstChgStaImg,
	IVE_DST_IMAGE_S *pstChgStaFg, IVE_DST_IMAGE_S *pstChgStaLife,
	IVE_DST_MEM_INFO_S *pstStatData,
	IVE_UPDATE_BG_MODEL_CTRL_S *pstUpdateBgModelCtrl, bool bInstant);

CVI_S32 CVI_MPI_IVE_ANN_MLP_LoadModel(const CVI_CHAR *pchFileName,
				      IVE_ANN_MLP_MODEL_S *pstAnnMlpModel);

CVI_VOID CVI_MPI_IVE_ANN_MLP_UnloadModel(IVE_ANN_MLP_MODEL_S *pstAnnMlpModel);

CVI_S32 CVI_MPI_IVE_ANN_MLP_Predict(IVE_HANDLE *pIveHandle,
				    IVE_SRC_DATA_S *pstSrc,
				    IVE_LOOK_UP_TABLE_S *pstActivFuncTab,
				    IVE_ANN_MLP_MODEL_S *pstAnnMlpModel,
				    IVE_DST_DATA_S *pstDst, bool bInstant);

CVI_S32 CVI_MPI_IVE_SVM_LoadModel(const CVI_CHAR *pchFileName,
				  IVE_SVM_MODEL_S *pstSvmModel);

CVI_VOID CVI_MPI_IVE_SVM_UnloadModel(IVE_SVM_MODEL_S *pstSvmModel);

CVI_S32 CVI_MPI_IVE_SVM_Predict(IVE_HANDLE *pIveHandle, IVE_SRC_DATA_S *pstSrc,
				IVE_LOOK_UP_TABLE_S *pstKernelTab,
				IVE_SVM_MODEL_S *pstSvmModel,
				IVE_DST_DATA_S *pstDstVote, bool bInstant);

CVI_S32 CVI_MPI_IVE_SAD(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
			IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstSad,
			IVE_DST_IMAGE_S *pstThr, IVE_SAD_CTRL_S *pstSadCtrl,
			bool bInstant);

CVI_S32 CVI_MPI_IVE_Resize(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S astSrc[],
			   IVE_DST_IMAGE_S astDst[],
			   IVE_RESIZE_CTRL_S *pstResizeCtrl, bool bInstant);

CVI_S32 CVI_MPI_IVE_CNN_LoadModel(const CVI_CHAR *pchFileName,
				  IVE_CNN_MODEL_S *pstCnnModel);

CVI_VOID CVI_MPI_IVE_CNN_UnloadModel(IVE_CNN_MODEL_S *pstCnnModel);

CVI_S32 CVI_MPI_IVE_CNN_Predict(IVE_HANDLE *pIveHandle,
				IVE_SRC_IMAGE_S astSrc[],
				IVE_CNN_MODEL_S *pstCnnModel,
				IVE_DST_DATA_S *pstDst,
				IVE_CNN_CTRL_S *pstCnnCtrl, bool bInstant);

CVI_S32 CVI_MPI_IVE_CNN_GetResult(IVE_SRC_DATA_S *pstSrc,
				  IVE_DST_MEM_INFO_S *pstDst,
				  IVE_CNN_MODEL_S *pstCnnModel,
				  IVE_CNN_CTRL_S *pstCnnCtrl);

CVI_S32 CVI_MPI_IVE_Query(IVE_HANDLE IveHandle, bool *pbFinish, bool bBlock);

CVI_S32 CVI_MPI_IVE_imgInToOdma(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
				IVE_DST_IMAGE_S *pstDst,
				IVE_FILTER_CTRL_S *pstFltCtrl, bool bInstant);
CVI_S32 CVI_MPI_IVE_rgbPToYuvToErodeToDilate(IVE_HANDLE *pIveHandle,
					     IVE_SRC_IMAGE_S *pstSrc,
					     IVE_DST_IMAGE_S *pstDst,
					     IVE_DST_IMAGE_S *pstDst2,
					     IVE_FILTER_CTRL_S *pstFltCtrl,
					     bool bInstant);
#ifdef __cplusplus
}
#endif
#endif /*_CVI_IVE_PLATFORM_H_*/
