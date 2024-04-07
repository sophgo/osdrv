/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name:stitch_reg_fields.h
 * Description:HW register description
 */

#ifndef _STITCH_REG_FIELDS_H_
#define _STITCH_REG_FIELDS_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_STITCHING_BLENDING_0 {
	uint32_t raw;
	struct {
		uint32_t FRAME_START_W1P                 : 1;
		uint32_t _rsv_1                          : 1;
		uint32_t FRAME_DONE                      : 1;
		uint32_t _rsv_3                          : 1;
		uint32_t AVG_MODE_BLD0                   : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t AVG_MODE_LEFT0                  : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t AVG_MODE_RIGHT0                 : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t BLD_DISABLE0                    : 1;
		uint32_t _rsv_17                         : 3;
		uint32_t DROP_MODE_BLD0                  : 1;
		uint32_t _rsv_21                         : 1;
		uint32_t DROP_MODE_LEFT0                 : 1;
		uint32_t _rsv_23                         : 1;
		uint32_t DROP_MODE_RIGHT0                : 1;
	} bits;
};

union REG_STITCHING_BLENDING_1 {
	uint32_t raw;
	struct {
		uint32_t H_ERROR_LEFT0                   : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t H_ERROR_RIGHT0                  : 1;
		uint32_t _rsv_9                          : 7;
		uint32_t W_ERROR_LEFT0                   : 1;
		uint32_t _rsv_17                         : 7;
		uint32_t W_ERROR_RIGHT0                  : 1;
	} bits;
};

union REG_STITCHING_BLENDING_2 {
	uint32_t raw;
	struct {
		uint32_t BLD_W_STR_LEFT0                 : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t BLD_W_END_LEFT0                 : 14;
	} bits;
};

union REG_STITCHING_BLENDING_3 {
	uint32_t raw;
	struct {
		uint32_t BLD_W_STR_RIGHT0                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t BLD_W_END_RIGHT0                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_4 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_BLDY0                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_BLDY0                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_5 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_R_LEFTY0             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_R_LEFTY0             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_6 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_R_RIGHTY0            : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_R_RIGHTY0            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_7 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_BLDY0                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_BLDY0                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_8 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_R_LEFTY0             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_R_LEFTY0             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_9 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_R_RIGHTY0            : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_R_RIGHTY0            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_10 {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT_BLDY0                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH_BLDY0                 : 14;
	} bits;
};

union REG_STITCHING_BLENDING_11 {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT_R_LEFTY0             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH_R_LEFTY0              : 14;
	} bits;
};

union REG_STITCHING_BLENDING_12 {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT_R_RIGHTY0            : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH_R_RIGHTY0             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_13 {
	uint32_t raw;
	struct {
		uint32_t NBLD_W_END_LEFT0                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t NBLD_W_END_RIGHT0               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_14 {
	uint32_t raw;
	struct {
		uint32_t NBLD_W_STR_LEFT0                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t NBLD_W_STR_RIGHT0               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_15 {
	uint32_t raw;
	struct {
		uint32_t NODELAY_DUPLICATE_MODE_LEFT0    : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t NODELAY_DUPLICATE_MODE_RIGHT0   : 1;
	} bits;
};

union REG_STITCHING_BLENDING_16 {
	uint32_t raw;
	struct {
		uint32_t RDMA_STCH_ALPHA0_ENABLE         : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t RDMA_STCH_BETA0_ENABLE          : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t RDMA_STCH_LEFTY0_ENABLE         : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t RDMA_STCH_LEFTU0_ENABLE         : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t RDMA_STCH_LEFTV0_ENABLE         : 1;
		uint32_t _rsv_17                         : 3;
		uint32_t RDMA_STCH_RIGHTY0_ENABLE        : 1;
		uint32_t _rsv_21                         : 3;
		uint32_t RDMA_STCH_RIGHTU0_ENABLE        : 1;
		uint32_t _rsv_25                         : 3;
		uint32_t RDMA_STCH_RIGHTV0_ENABLE        : 1;
	} bits;
};

union REG_STITCHING_BLENDING_17 {
	uint32_t raw;
	struct {
		uint32_t SCALE_DISABLE_LEFT0             : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t SCALE_DISABLE_RIGHT0            : 1;
	} bits;
};

union REG_STITCHING_BLENDING_18 {
	uint32_t raw;
	struct {
		uint32_t WDMA_STCH_LEFTY0_ENABLE         : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t WDMA_STCH_LEFTU0_ENABLE         : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t WDMA_STCH_LEFTV0_ENABLE         : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t WDMA_STCH_RIGHTY0_ENABLE        : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t WDMA_STCH_RIGHTU0_ENABLE        : 1;
		uint32_t _rsv_17                         : 3;
		uint32_t WDMA_STCH_RIGHTV0_ENABLE        : 1;
		uint32_t _rsv_21                         : 3;
		uint32_t WDMA_STCH_BLDY0_ENABLE          : 1;
		uint32_t _rsv_25                         : 3;
		uint32_t WDMA_STCH_BLDU0_ENABLE          : 1;
		uint32_t _rsv_29                         : 2;
		uint32_t WDMA_STCH_BLDV0_ENABLE          : 1;
	} bits;
};

union REG_STITCHING_BLENDING_19 {
	uint32_t raw;
	struct {
		uint32_t X_0                             : 5;
	} bits;
};

union REG_STITCHING_BLENDING_20 {
	uint32_t raw;
	struct {
		uint32_t BYPASS_H_BLD0                   : 1;
		uint32_t _rsv_1                          : 1;
		uint32_t BYPASS_V_BLD0                   : 1;
		uint32_t _rsv_3                          : 1;
		uint32_t BYPASS_H_LEFTIN0                : 1;
		uint32_t _rsv_5                          : 1;
		uint32_t BYPASS_V_LEFTIN0                : 1;
		uint32_t _rsv_7                          : 1;
		uint32_t BYPASS_H_LEFTOUT0               : 1;
		uint32_t _rsv_9                          : 1;
		uint32_t BYPASS_V_LEFTOUT0               : 1;
		uint32_t _rsv_11                         : 1;
		uint32_t BYPASS_H_RIGHTIN0               : 1;
		uint32_t _rsv_13                         : 1;
		uint32_t BYPASS_V_RIGHTIN0               : 1;
		uint32_t _rsv_15                         : 1;
		uint32_t BYPASS_H_RIGHTOUT0              : 1;
		uint32_t _rsv_17                         : 1;
		uint32_t BYPASS_V_RIGHTOUT0              : 1;
	} bits;
};

union REG_STITCHING_BLENDING_21 {
	uint32_t raw;
	struct {
		uint32_t AVG_MODE_BLD1                   : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t AVG_MODE_LEFT1                  : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t AVG_MODE_RIGHT1                 : 1;
		uint32_t _rsv_9                          : 7;
		uint32_t BLD_DISABLE1                    : 1;
		uint32_t _rsv_17                         : 3;
		uint32_t DROP_MODE_BLD1                  : 1;
		uint32_t _rsv_21                         : 1;
		uint32_t DROP_MODE_LEFT1                 : 1;
		uint32_t _rsv_23                         : 1;
		uint32_t DROP_MODE_RIGHT1                : 1;
	} bits;
};

union REG_STITCHING_BLENDING_22 {
	uint32_t raw;
	struct {
		uint32_t H_ERROR_LEFT1                   : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t H_ERROR_RIGHT1                  : 1;
		uint32_t _rsv_9                          : 7;
		uint32_t W_ERROR_LEFT1                   : 1;
		uint32_t _rsv_17                         : 7;
		uint32_t W_ERROR_RIGHT1                  : 1;
	} bits;
};

union REG_STITCHING_BLENDING_23 {
	uint32_t raw;
	struct {
		uint32_t BLD_W_STR_LEFT1                 : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t BLD_W_END_LEFT1                 : 14;
	} bits;
};

union REG_STITCHING_BLENDING_24 {
	uint32_t raw;
	struct {
		uint32_t BLD_W_STR_RIGHT1                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t BLD_W_END_RIGHT1                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_25 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_BLDY1                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_BLDY1                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_26 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_R_LEFTY1             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_R_LEFTY1             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_27 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_R_RIGHTY1            : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_R_RIGHTY1            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_28 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_BLDY1                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_BLDY1                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_29 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_R_LEFTY1             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_R_LEFTY1             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_30 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_R_RIGHTY1            : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_R_RIGHTY1            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_31 {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT_BLDY1                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH_BLDY1                 : 14;
	} bits;
};

union REG_STITCHING_BLENDING_32 {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT_R_LEFTY1             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH_R_LEFTY1              : 14;
	} bits;
};

union REG_STITCHING_BLENDING_33 {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT_R_RIGHTY1            : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH_R_RIGHTY1             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_34 {
	uint32_t raw;
	struct {
		uint32_t NBLD_W_END_LEFT1                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t NBLD_W_END_RIGHT1               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_35 {
	uint32_t raw;
	struct {
		uint32_t NBLD_W_STR_LEFT1                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t NBLD_W_STR_RIGHT1               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_36 {
	uint32_t raw;
	struct {
		uint32_t NODELAY_DUPLICATE_MODE_LEFT1    : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t NODELAY_DUPLICATE_MODE_RIGHT1   : 1;
	} bits;
};

union REG_STITCHING_BLENDING_37 {
	uint32_t raw;
	struct {
		uint32_t RDMA_STCH_ALPHA1_ENABLE         : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t RDMA_STCH_BETA1_ENABLE          : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t RDMA_STCH_LEFTY1_ENABLE         : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t RDMA_STCH_LEFTU1_ENABLE         : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t RDMA_STCH_LEFTV1_ENABLE         : 1;
		uint32_t _rsv_17                         : 3;
		uint32_t RDMA_STCH_RIGHTY1_ENABLE        : 1;
		uint32_t _rsv_21                         : 3;
		uint32_t RDMA_STCH_RIGHTU1_ENABLE        : 1;
		uint32_t _rsv_25                         : 3;
		uint32_t RDMA_STCH_RIGHTV1_ENABLE        : 1;
	} bits;
};

union REG_STITCHING_BLENDING_38 {
	uint32_t raw;
	struct {
		uint32_t SCALE_DISABLE_LEFT1             : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t SCALE_DISABLE_RIGHT1            : 1;
	} bits;
};

union REG_STITCHING_BLENDING_39 {
	uint32_t raw;
	struct {
		uint32_t WDMA_STCH_LEFTY1_ENABLE         : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t WDMA_STCH_LEFTU1_ENABLE         : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t WDMA_STCH_LEFTV1_ENABLE         : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t WDMA_STCH_RIGHTY1_ENABLE        : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t WDMA_STCH_RIGHTU1_ENABLE        : 1;
		uint32_t _rsv_17                         : 3;
		uint32_t WDMA_STCH_RIGHTV1_ENABLE        : 1;
	} bits;
};

union REG_STITCHING_BLENDING_40 {
	uint32_t raw;
	struct {
		uint32_t _rsv_0                          : 24;
		uint32_t WDMA_STCH_BLDY1_ENABLE          : 1;
		uint32_t _rsv_25                         : 3;
		uint32_t WDMA_STCH_BLDU1_ENABLE          : 1;
		uint32_t _rsv_29                         : 2;
		uint32_t WDMA_STCH_BLDV1_ENABLE          : 1;
	} bits;
};

union REG_STITCHING_BLENDING_41 {
	uint32_t raw;
	struct {
		uint32_t X_1                             : 5;
	} bits;
};

union REG_STITCHING_BLENDING_42 {
	uint32_t raw;
	struct {
		uint32_t BYPASS_H_BLD1                   : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t BYPASS_H_LEFTIN1                : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t BYPASS_H_RIGHTIN1               : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t BYPASS_H_LEFTOUT1               : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t BYPASS_H_RIGHTOUT1              : 1;
	} bits;
};

union REG_STITCHING_BLENDING_43 {
	uint32_t raw;
	struct {
		uint32_t BYPASS_V_BLD1                   : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t BYPASS_V_LEFTIN1                : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t BYPASS_V_RIGHTIN1               : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t BYPASS_V_LEFTOUT1               : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t BYPASS_V_RIGHTOUT1              : 1;
	} bits;
};

union REG_STITCHING_BLENDING_FLOW_CONTROL {
	uint32_t raw;
	struct {
		uint32_t INTERRUPT_CLR_W1P               : 1;
		uint32_t SUBSYS_RESET_W1P                : 1;
		uint32_t STITCH_INTER_FLOW               : 1;
		uint32_t RESET_DONE                      : 2;
		uint32_t SHADOW_DONE                     : 1;
		uint32_t SHADOW_CLR_W1P                  : 1;
		uint32_t SHADOW_INT_MASK                 : 1;
		uint32_t TRIG_CNT                        : 8;
		uint32_t SHADOW_CNT                      : 8;
		uint32_t RESET_CNT                       : 8;
	} bits;
};

union REG_STITCHING_BLENDING_STCH_SHDW_SEL {
	uint32_t raw;
	struct {
		uint32_t STCH_SHDW_SEL                   : 1;
	} bits;
};

union REG_STITCHING_BLENDING_44 {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT_R_LEFTUV0            : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH_R_LEFTUV0             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_45 {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT_R_RIGHTUV0           : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH_R_RIGHTUV0            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_46 {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT_R_LEFTUV1            : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH_R_LEFTUV1             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_47 {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT_R_RIGHTUV1           : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH_R_RIGHTUV1            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_48 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_R_LEFTUV0            : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_R_LEFTUV0            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_49 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_R_RIGHTUV0           : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_R_RIGHTUV0           : 14;
	} bits;
};

union REG_STITCHING_BLENDING_50 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_R_LEFTUV0            : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_R_LEFTUV0            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_51 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_R_RIGHTUV0           : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_R_RIGHTUV0           : 14;
	} bits;
};

union REG_STITCHING_BLENDING_52 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_R_LEFTUV1            : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_R_LEFTUV1            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_53 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_R_RIGHTUV1           : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_R_RIGHTUV1           : 14;
	} bits;
};

union REG_STITCHING_BLENDING_54 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_R_LEFTUV1            : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_R_LEFTUV1            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_55 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_R_RIGHTUV1           : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_R_RIGHTUV1           : 14;
	} bits;
};

union REG_STITCHING_BLENDING_56 {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT_ALPHA0               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH_ALPHA0                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_57 {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT_BETA0                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH_BETA0                 : 14;
	} bits;
};

union REG_STITCHING_BLENDING_58 {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT_ALPHA1               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH_ALPHA1                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_59 {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT_BETA1                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH_BETA1                 : 14;
	} bits;
};

union REG_STITCHING_BLENDING_60 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_ALPHA0               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_ALPHA0               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_61 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_BETA0                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_BETA0                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_62 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_ALPHA0               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_ALPHA0               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_63 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_BETA0                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_BETA0                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_64 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_ALPHA1               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_ALPHA1               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_65 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_BETA1                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_BETA1                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_66 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_ALPHA1               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_ALPHA1               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_67 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_BETA1                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_BETA1                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_68 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_BLDUV0               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_BLDUV0               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_69 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_W_LEFTY0             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_W_LEFTY0             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_70 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_W_RIGHTY0            : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_W_RIGHTY0            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_71 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_BLDUV0               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_BLDUV0               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_72 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_W_LEFTY0             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_W_LEFTY0             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_73 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_W_RIGHTY0            : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_W_RIGHTY0            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_74 {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT_BLDUV0               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH_BLDUV0                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_75 {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT_W_LEFTY0             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH_W_LEFTY0              : 14;
	} bits;
};

union REG_STITCHING_BLENDING_76 {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT_W_RIGHTY0            : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH_W_RIGHTY0             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_77 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_BLDUV1               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_BLDUV1               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_78 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_BLDUV1               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_BLDUV1               : 14;
	} bits;
};

union REG_STITCHING_BLENDING_79 {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT_BLDUV1               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH_BLDUV1                : 14;
	} bits;
};

union REG_STITCHING_BLENDING_80 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_W_LEFTUV0            : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_W_LEFTUV0            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_81 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_W_RIGHTUV0           : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_W_RIGHTUV0           : 14;
	} bits;
};

union REG_STITCHING_BLENDING_82 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_W_LEFTUV0            : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_W_LEFTUV0            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_83 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_W_RIGHTUV0           : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_W_RIGHTUV0           : 14;
	} bits;
};

union REG_STITCHING_BLENDING_84 {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT_W_LEFTUV0            : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH_W_LEFTUV0             : 14;
	} bits;
};

union REG_STITCHING_BLENDING_85 {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT_W_RIGHTUV0           : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH_W_RIGHTUV0            : 14;
	} bits;
};

union REG_STITCHING_BLENDING_86 {
	uint32_t raw;
	struct {
		uint32_t CROP_ENABLE_R_LEFTY0            : 1;
		uint32_t CROP_ENABLE_R_LEFTUV0           : 1;
		uint32_t CROP_ENABLE_R_RIGHTY0           : 1;
		uint32_t CROP_ENABLE_R_RIGHTUV0          : 1;
		uint32_t CROP_ENABLE_ALPHA0              : 1;
		uint32_t CROP_ENABLE_BETA0               : 1;
		uint32_t CROP_ENABLE_W_LEFTY0            : 1;
		uint32_t CROP_ENABLE_W_LEFTUV0           : 1;
		uint32_t CROP_ENABLE_W_RIGHTY0           : 1;
		uint32_t CROP_ENABLE_W_RIGHTUV0          : 1;
		uint32_t CROP_ENABLE_BLDY0               : 1;
		uint32_t CROP_ENABLE_BLDUV0              : 1;
		uint32_t CROP2BJ_LEFT_ENABLE0            : 1;
		uint32_t CROP2BJ_RIGHT_ENABLE0           : 1;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_ENABLE_R_LEFTY1            : 1;
		uint32_t CROP_ENABLE_R_LEFTUV1           : 1;
		uint32_t CROP_ENABLE_R_RIGHTY1           : 1;
		uint32_t CROP_ENABLE_R_RIGHTUV1          : 1;
		uint32_t CROP_ENABLE_ALPHA1              : 1;
		uint32_t CROP_ENABLE_BETA1               : 1;
		uint32_t CROP_ENABLE_BLDY1               : 1;
		uint32_t CROP_ENABLE_BLDUV1              : 1;
		uint32_t CROP2BJ_LEFT_ENABLE1            : 1;
		uint32_t CROP2BJ_RIGHT_ENABLE1           : 1;
	} bits;
};

union REG_STITCHING_BLENDING_MODE_SEL {
	uint32_t raw;
	struct {
		uint32_t MODE_SEL                        : 2;
	} bits;
};

union REG_STITCHING_BLENDING_SRC_SEL {
	uint32_t raw;
	struct {
		uint32_t SRC_SEL_LEFT0                   : 1;
		uint32_t SRC_SEL_RIGHT0                  : 1;
		uint32_t _rsv_2                          : 14;
		uint32_t SRC_SEL_LEFT1                   : 1;
		uint32_t SRC_SEL_RIGHT1                  : 1;
	} bits;
};

union REG_STITCHING_BLENDING_87 {
	uint32_t raw;
	struct {
		uint32_t CROP2BJ_LEFT_IMG_HEIGHT0        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP2BJ_LEFT_IMG_WIDTH0         : 14;
	} bits;
};

union REG_STITCHING_BLENDING_88 {
	uint32_t raw;
	struct {
		uint32_t CROP2BJ_RIGHT_IMG_HEIGHT0       : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP2BJ_RIGHT_IMG_WIDTH0        : 14;
	} bits;
};

union REG_STITCHING_BLENDING_89 {
	uint32_t raw;
	struct {
		uint32_t CROP2BJ_LEFT_CROP_H_STR0        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP2BJ_LEFT_CROP_H_END0        : 14;
	} bits;
};

union REG_STITCHING_BLENDING_90 {
	uint32_t raw;
	struct {
		uint32_t CROP2BJ_RIGHT_CROP_H_STR0       : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP2BJ_RIGHT_CROP_H_END0       : 14;
	} bits;
};

union REG_STITCHING_BLENDING_91 {
	uint32_t raw;
	struct {
		uint32_t CROP2BJ_LEFT_CROP_W_STR0        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP2BJ_LEFT_CROP_W_END0        : 14;
	} bits;
};

union REG_STITCHING_BLENDING_92 {
	uint32_t raw;
	struct {
		uint32_t CROP2BJ_RIGHT_CROP_W_STR0       : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP2BJ_RIGHT_CROP_W_END0       : 14;
	} bits;
};

union REG_STITCHING_BLENDING_93 {
	uint32_t raw;
	struct {
		uint32_t CROP2BJ_LEFT_IMG_HEIGHT1        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP2BJ_LEFT_IMG_WIDTH1         : 14;
	} bits;
};

union REG_STITCHING_BLENDING_94 {
	uint32_t raw;
	struct {
		uint32_t CROP2BJ_RIGHT_IMG_HEIGHT1       : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP2BJ_RIGHT_IMG_WIDTH1        : 14;
	} bits;
};

union REG_STITCHING_BLENDING_95 {
	uint32_t raw;
	struct {
		uint32_t CROP2BJ_LEFT_CROP_H_STR1        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP2BJ_LEFT_CROP_H_END1        : 14;
	} bits;
};

union REG_STITCHING_BLENDING_96 {
	uint32_t raw;
	struct {
		uint32_t CROP2BJ_RIGHT_CROP_H_STR1       : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP2BJ_RIGHT_CROP_H_END1       : 14;
	} bits;
};

union REG_STITCHING_BLENDING_97 {
	uint32_t raw;
	struct {
		uint32_t CROP2BJ_LEFT_CROP_W_STR1        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP2BJ_LEFT_CROP_W_END1        : 14;
	} bits;
};

union REG_STITCHING_BLENDING_98 {
	uint32_t raw;
	struct {
		uint32_t CROP2BJ_RIGHT_CROP_W_STR1       : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP2BJ_RIGHT_CROP_W_END1       : 14;
	} bits;
};

union REG_STITCHING_BLENDING_INT_MASK {
	uint32_t raw;
	struct {
		uint32_t INTERRUPT_MASK                  : 32;
	} bits;
};

union REG_STITCH_DMA_CTL_SYS_CONTROL {
	uint32_t raw;
	struct {
		uint32_t QOS_SEL						 : 1;
		uint32_t SW_QOS 						 : 1;
		uint32_t ENABLE_INV 					 : 1;
		uint32_t _rsv_3 						 : 5;
		uint32_t BASEH							 : 8;
		uint32_t BASE_SEL						 : 1;
		uint32_t STRIDE_SEL 					 : 1;
		uint32_t SEGLEN_SEL 					 : 1;
		uint32_t SEGNUM_SEL 					 : 1;
		uint32_t SLICE_ENABLE					 : 1;
		uint32_t UPDATE_BASE_ADDR				 : 1;
		uint32_t _rsv_22						 : 6;
		uint32_t DBG_SEL						 : 3;
	} bits;
};

union REG_STITCH_DMA_CTL_BASE_ADDR {
	uint32_t raw;
	struct {
		uint32_t BASEL							 : 32;
	} bits;
};

union REG_STITCH_DMA_CTL_DMA_SEGLEN {
	uint32_t raw;
	struct {
		uint32_t SEGLEN 						 : 28;
	} bits;
};

union REG_STITCH_DMA_CTL_DMA_STRIDE {
	uint32_t raw;
	struct {
		uint32_t STRIDE 						 : 28;
	} bits;
};

union REG_STITCH_DMA_CTL_DMA_SEGNUM {
	uint32_t raw;
	struct {
		uint32_t SEGNUM 						 : 14;
	} bits;
};

union REG_STITCH_DMA_CTL_DMA_STATUS {
	uint32_t raw;
	struct {
		uint32_t STATUS 						 : 32;
	} bits;
};

union REG_STITCH_DMA_CTL_DMA_SLICESIZE {
	uint32_t raw;
	struct {
		uint32_t SLICE_SIZE 					 : 6;
	} bits;
};

union REG_STITCH_DMA_CTL_DMA_DUMMY {
	uint32_t raw;
	struct {
		uint32_t DUMMY							 : 16;
		uint32_t PERF_PATCH_ENABLE				 : 1;
		uint32_t SEGLEN_LESS16_ENABLE			 : 1;
		uint32_t SYNC_PATCH_ENABLE				 : 1;
		uint32_t TRIG_PATCH_ENABLE				 : 1;
	} bits;
};

#ifdef __cplusplus
}
#endif

#endif /* _STITCH_REG_FIELDS_H_ */
