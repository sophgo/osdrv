// $Module: reg_img_in $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Wed, 03 Nov 2021 05:07:20 PM $
//

//GEN REG ADDR/OFFSET/MASK
#define  IMG_IN_REG_00  0x0
#define  IMG_IN_REG_01  0x4
#define  IMG_IN_REG_02  0x8
#define  IMG_IN_REG_03  0xc
#define  IMG_IN_REG_04  0x10
#define  IMG_IN_REG_05  0x14
#define  IMG_IN_REG_06  0x18
#define  IMG_IN_REG_07  0x1c
#define  IMG_IN_REG_08  0x20
#define  IMG_IN_REG_Y_BASE_0  0x24
#define  IMG_IN_REG_Y_BASE_1  0x28
#define  IMG_IN_REG_U_BASE_0  0x2c
#define  IMG_IN_REG_U_BASE_1  0x30
#define  IMG_IN_REG_V_BASE_0  0x34
#define  IMG_IN_REG_V_BASE_1  0x38
#define  IMG_IN_REG_040  0x40
#define  IMG_IN_REG_044  0x44
#define  IMG_IN_REG_048  0x48
#define  IMG_IN_REG_04C  0x4c
#define  IMG_IN_REG_050  0x50
#define  IMG_IN_REG_054  0x54
#define  IMG_IN_REG_058  0x58
#define  IMG_IN_REG_05C  0x5c
#define  IMG_IN_REG_060  0x60
#define  IMG_IN_REG_064  0x64
#define  IMG_IN_REG_068  0x68
#define  IMG_IN_REG_AXI_ST  0x70
#define  IMG_IN_REG_BW_LIMIT  0x74
#define  IMG_IN_REG_CATCH  0x80
#define  IMG_IN_REG_CHK_CTRL  0x84
#define  img_in_cHKSUM_AXI_RD  0x88
#define  IMG_IN_SB_REG_CTRL  0x90
#define  IMG_IN_SB_REG_C_STAT  0x94
#define  IMG_IN_SB_REG_Y_STAT  0x98
#define  IMG_IN_REG_SRC_SEL   0x0
#define  IMG_IN_REG_SRC_SEL_OFFSET 0
#define  IMG_IN_REG_SRC_SEL_MASK   0x3
#define  IMG_IN_REG_SRC_SEL_BITS   0x2
#define  IMG_IN_REG_FMT_SEL   0x0
#define  IMG_IN_REG_FMT_SEL_OFFSET 4
#define  IMG_IN_REG_FMT_SEL_MASK   0xf0
#define  IMG_IN_REG_FMT_SEL_BITS   0x4
#define  IMG_IN_REG_BURST_LN   0x0
#define  IMG_IN_REG_BURST_LN_OFFSET 8
#define  IMG_IN_REG_BURST_LN_MASK   0xf00
#define  IMG_IN_REG_BURST_LN_BITS   0x4
#define  IMG_IN_REG_IMG_CSC_EN   0x0
#define  IMG_IN_REG_IMG_CSC_EN_OFFSET 12
#define  IMG_IN_REG_IMG_CSC_EN_MASK   0x1000
#define  IMG_IN_REG_IMG_CSC_EN_BITS   0x1
#define  IMG_IN_REG_AUTO_CSC_EN   0x0
#define  IMG_IN_REG_AUTO_CSC_EN_OFFSET 13
#define  IMG_IN_REG_AUTO_CSC_EN_MASK   0x2000
#define  IMG_IN_REG_AUTO_CSC_EN_BITS   0x1
#define  IMG_IN_REG_64B_ALIGN   0x0
#define  IMG_IN_REG_64B_ALIGN_OFFSET 16
#define  IMG_IN_REG_64B_ALIGN_MASK   0x10000
#define  IMG_IN_REG_64B_ALIGN_BITS   0x1
#define  IMG_IN_REG_FORCE_CLK_ENABLE   0x0
#define  IMG_IN_REG_FORCE_CLK_ENABLE_OFFSET 31
#define  IMG_IN_REG_FORCE_CLK_ENABLE_MASK   0x80000000
#define  IMG_IN_REG_FORCE_CLK_ENABLE_BITS   0x1
#define  IMG_IN_REG_SRC_X_STR   0x4
#define  IMG_IN_REG_SRC_X_STR_OFFSET 0
#define  IMG_IN_REG_SRC_X_STR_MASK   0xffff
#define  IMG_IN_REG_SRC_X_STR_BITS   0x10
#define  IMG_IN_REG_SRC_Y_STR   0x4
#define  IMG_IN_REG_SRC_Y_STR_OFFSET 16
#define  IMG_IN_REG_SRC_Y_STR_MASK   0xffff0000
#define  IMG_IN_REG_SRC_Y_STR_BITS   0x10
#define  IMG_IN_REG_SRC_WD   0x8
#define  IMG_IN_REG_SRC_WD_OFFSET 0
#define  IMG_IN_REG_SRC_WD_MASK   0xffff
#define  IMG_IN_REG_SRC_WD_BITS   0x10
#define  IMG_IN_REG_SRC_HT   0x8
#define  IMG_IN_REG_SRC_HT_OFFSET 16
#define  IMG_IN_REG_SRC_HT_MASK   0xffff0000
#define  IMG_IN_REG_SRC_HT_BITS   0x10
#define  IMG_IN_REG_SRC_Y_PITCH   0xc
#define  IMG_IN_REG_SRC_Y_PITCH_OFFSET 0
#define  IMG_IN_REG_SRC_Y_PITCH_MASK   0xffffff
#define  IMG_IN_REG_SRC_Y_PITCH_BITS   0x18
#define  IMG_IN_REG_SRC_C_PITCH   0x10
#define  IMG_IN_REG_SRC_C_PITCH_OFFSET 0
#define  IMG_IN_REG_SRC_C_PITCH_MASK   0xffffff
#define  IMG_IN_REG_SRC_C_PITCH_BITS   0x18
#define  IMG_IN_REG_SW_FORCE_UP   0x14
#define  IMG_IN_REG_SW_FORCE_UP_OFFSET 0
#define  IMG_IN_REG_SW_FORCE_UP_MASK   0x1
#define  IMG_IN_REG_SW_FORCE_UP_BITS   0x1
#define  IMG_IN_REG_SW_MASK_UP   0x14
#define  IMG_IN_REG_SW_MASK_UP_OFFSET 1
#define  IMG_IN_REG_SW_MASK_UP_MASK   0x2
#define  IMG_IN_REG_SW_MASK_UP_BITS   0x1
#define  IMG_IN_REG_SHRD_SEL   0x14
#define  IMG_IN_REG_SHRD_SEL_OFFSET 2
#define  IMG_IN_REG_SHRD_SEL_MASK   0x4
#define  IMG_IN_REG_SHRD_SEL_BITS   0x1
#define  IMG_IN_REG_DUMMY_RO   0x18
#define  IMG_IN_REG_DUMMY_RO_OFFSET 0
#define  IMG_IN_REG_DUMMY_RO_MASK   0xffffffff
#define  IMG_IN_REG_DUMMY_RO_BITS   0x20
#define  IMG_IN_REG_DUMMY_0   0x1c
#define  IMG_IN_REG_DUMMY_0_OFFSET 0
#define  IMG_IN_REG_DUMMY_0_MASK   0xffffffff
#define  IMG_IN_REG_DUMMY_0_BITS   0x20
#define  IMG_IN_REG_DUMMY_1   0x20
#define  IMG_IN_REG_DUMMY_1_OFFSET 0
#define  IMG_IN_REG_DUMMY_1_MASK   0xffffffff
#define  IMG_IN_REG_DUMMY_1_BITS   0x20
#define  IMG_IN_REG_SRC_Y_BASE_B0   0x24
#define  IMG_IN_REG_SRC_Y_BASE_B0_OFFSET 0
#define  IMG_IN_REG_SRC_Y_BASE_B0_MASK   0xffffffff
#define  IMG_IN_REG_SRC_Y_BASE_B0_BITS   0x20
#define  IMG_IN_REG_SRC_Y_BASE_B1   0x28
#define  IMG_IN_REG_SRC_Y_BASE_B1_OFFSET 0
#define  IMG_IN_REG_SRC_Y_BASE_B1_MASK   0xff
#define  IMG_IN_REG_SRC_Y_BASE_B1_BITS   0x8
#define  IMG_IN_REG_SRC_U_BASE_B0   0x2c
#define  IMG_IN_REG_SRC_U_BASE_B0_OFFSET 0
#define  IMG_IN_REG_SRC_U_BASE_B0_MASK   0xffffffff
#define  IMG_IN_REG_SRC_U_BASE_B0_BITS   0x20
#define  IMG_IN_REG_SRC_U_BASE_B1   0x30
#define  IMG_IN_REG_SRC_U_BASE_B1_OFFSET 0
#define  IMG_IN_REG_SRC_U_BASE_B1_MASK   0xff
#define  IMG_IN_REG_SRC_U_BASE_B1_BITS   0x8
#define  IMG_IN_REG_SRC_V_BASE_B0   0x34
#define  IMG_IN_REG_SRC_V_BASE_B0_OFFSET 0
#define  IMG_IN_REG_SRC_V_BASE_B0_MASK   0xffffffff
#define  IMG_IN_REG_SRC_V_BASE_B0_BITS   0x20
#define  IMG_IN_REG_SRC_V_BASE_B1   0x38
#define  IMG_IN_REG_SRC_V_BASE_B1_OFFSET 0
#define  IMG_IN_REG_SRC_V_BASE_B1_MASK   0xff
#define  IMG_IN_REG_SRC_V_BASE_B1_BITS   0x8
#define  IMG_IN_REG_C00   0x40
#define  IMG_IN_REG_C00_OFFSET 0
#define  IMG_IN_REG_C00_MASK   0x3fff
#define  IMG_IN_REG_C00_BITS   0xe
#define  IMG_IN_REG_C01   0x40
#define  IMG_IN_REG_C01_OFFSET 16
#define  IMG_IN_REG_C01_MASK   0x3fff0000
#define  IMG_IN_REG_C01_BITS   0xe
#define  IMG_IN_REG_C02   0x44
#define  IMG_IN_REG_C02_OFFSET 0
#define  IMG_IN_REG_C02_MASK   0x3fff
#define  IMG_IN_REG_C02_BITS   0xe
#define  IMG_IN_REG_C10   0x48
#define  IMG_IN_REG_C10_OFFSET 0
#define  IMG_IN_REG_C10_MASK   0x3fff
#define  IMG_IN_REG_C10_BITS   0xe
#define  IMG_IN_REG_C11   0x48
#define  IMG_IN_REG_C11_OFFSET 16
#define  IMG_IN_REG_C11_MASK   0x3fff0000
#define  IMG_IN_REG_C11_BITS   0xe
#define  IMG_IN_REG_C12   0x4c
#define  IMG_IN_REG_C12_OFFSET 0
#define  IMG_IN_REG_C12_MASK   0x3fff
#define  IMG_IN_REG_C12_BITS   0xe
#define  IMG_IN_REG_C20   0x50
#define  IMG_IN_REG_C20_OFFSET 0
#define  IMG_IN_REG_C20_MASK   0x3fff
#define  IMG_IN_REG_C20_BITS   0xe
#define  IMG_IN_REG_C21   0x50
#define  IMG_IN_REG_C21_OFFSET 16
#define  IMG_IN_REG_C21_MASK   0x3fff0000
#define  IMG_IN_REG_C21_BITS   0xe
#define  IMG_IN_REG_C22   0x54
#define  IMG_IN_REG_C22_OFFSET 0
#define  IMG_IN_REG_C22_MASK   0x3fff
#define  IMG_IN_REG_C22_BITS   0xe
#define  IMG_IN_REG_SUB_0   0x58
#define  IMG_IN_REG_SUB_0_OFFSET 0
#define  IMG_IN_REG_SUB_0_MASK   0xff
#define  IMG_IN_REG_SUB_0_BITS   0x8
#define  IMG_IN_REG_SUB_1   0x58
#define  IMG_IN_REG_SUB_1_OFFSET 8
#define  IMG_IN_REG_SUB_1_MASK   0xff00
#define  IMG_IN_REG_SUB_1_BITS   0x8
#define  IMG_IN_REG_SUB_2   0x58
#define  IMG_IN_REG_SUB_2_OFFSET 16
#define  IMG_IN_REG_SUB_2_MASK   0xff0000
#define  IMG_IN_REG_SUB_2_BITS   0x8
#define  IMG_IN_REG_ADD_0   0x5c
#define  IMG_IN_REG_ADD_0_OFFSET 0
#define  IMG_IN_REG_ADD_0_MASK   0xff
#define  IMG_IN_REG_ADD_0_BITS   0x8
#define  IMG_IN_REG_ADD_1   0x5c
#define  IMG_IN_REG_ADD_1_OFFSET 8
#define  IMG_IN_REG_ADD_1_MASK   0xff00
#define  IMG_IN_REG_ADD_1_BITS   0x8
#define  IMG_IN_REG_ADD_2   0x5c
#define  IMG_IN_REG_ADD_2_OFFSET 16
#define  IMG_IN_REG_ADD_2_MASK   0xff0000
#define  IMG_IN_REG_ADD_2_BITS   0x8
#define  IMG_IN_REG_FIFO_RD_TH_Y   0x60
#define  IMG_IN_REG_FIFO_RD_TH_Y_OFFSET 0
#define  IMG_IN_REG_FIFO_RD_TH_Y_MASK   0xff
#define  IMG_IN_REG_FIFO_RD_TH_Y_BITS   0x8
#define  IMG_IN_REG_FIFO_PR_TH_Y   0x60
#define  IMG_IN_REG_FIFO_PR_TH_Y_OFFSET 8
#define  IMG_IN_REG_FIFO_PR_TH_Y_MASK   0xff00
#define  IMG_IN_REG_FIFO_PR_TH_Y_BITS   0x8
#define  IMG_IN_REG_FIFO_RD_TH_C   0x60
#define  IMG_IN_REG_FIFO_RD_TH_C_OFFSET 16
#define  IMG_IN_REG_FIFO_RD_TH_C_MASK   0xff0000
#define  IMG_IN_REG_FIFO_RD_TH_C_BITS   0x8
#define  IMG_IN_REG_FIFO_PR_TH_C   0x60
#define  IMG_IN_REG_FIFO_PR_TH_C_OFFSET 24
#define  IMG_IN_REG_FIFO_PR_TH_C_MASK   0xff000000
#define  IMG_IN_REG_FIFO_PR_TH_C_BITS   0x8
#define  IMG_IN_REG_OS_MAX   0x64
#define  IMG_IN_REG_OS_MAX_OFFSET 0
#define  IMG_IN_REG_OS_MAX_MASK   0xf
#define  IMG_IN_REG_OS_MAX_BITS   0x4
#define  IMG_IN_REG_ERR_FWR_Y   0x68
#define  IMG_IN_REG_ERR_FWR_Y_OFFSET 0
#define  IMG_IN_REG_ERR_FWR_Y_MASK   0x1
#define  IMG_IN_REG_ERR_FWR_Y_BITS   0x1
#define  IMG_IN_REG_ERR_FWR_U   0x68
#define  IMG_IN_REG_ERR_FWR_U_OFFSET 1
#define  IMG_IN_REG_ERR_FWR_U_MASK   0x2
#define  IMG_IN_REG_ERR_FWR_U_BITS   0x1
#define  IMG_IN_REG_ERR_FWR_V   0x68
#define  IMG_IN_REG_ERR_FWR_V_OFFSET 2
#define  IMG_IN_REG_ERR_FWR_V_MASK   0x4
#define  IMG_IN_REG_ERR_FWR_V_BITS   0x1
#define  IMG_IN_REG_CLR_FWR_W1T   0x68
#define  IMG_IN_REG_CLR_FWR_W1T_OFFSET 3
#define  IMG_IN_REG_CLR_FWR_W1T_MASK   0x8
#define  IMG_IN_REG_CLR_FWR_W1T_BITS   0x1
#define  IMG_IN_REG_ERR_ERD_Y   0x68
#define  IMG_IN_REG_ERR_ERD_Y_OFFSET 4
#define  IMG_IN_REG_ERR_ERD_Y_MASK   0x10
#define  IMG_IN_REG_ERR_ERD_Y_BITS   0x1
#define  IMG_IN_REG_ERR_ERD_U   0x68
#define  IMG_IN_REG_ERR_ERD_U_OFFSET 5
#define  IMG_IN_REG_ERR_ERD_U_MASK   0x20
#define  IMG_IN_REG_ERR_ERD_U_BITS   0x1
#define  IMG_IN_REG_ERR_ERD_V   0x68
#define  IMG_IN_REG_ERR_ERD_V_OFFSET 6
#define  IMG_IN_REG_ERR_ERD_V_MASK   0x40
#define  IMG_IN_REG_ERR_ERD_V_BITS   0x1
#define  IMG_IN_REG_CLR_ERD_W1T   0x68
#define  IMG_IN_REG_CLR_ERD_W1T_OFFSET 7
#define  IMG_IN_REG_CLR_ERD_W1T_MASK   0x80
#define  IMG_IN_REG_CLR_ERD_W1T_BITS   0x1
#define  IMG_IN_REG_LB_FULL_Y   0x68
#define  IMG_IN_REG_LB_FULL_Y_OFFSET 8
#define  IMG_IN_REG_LB_FULL_Y_MASK   0x100
#define  IMG_IN_REG_LB_FULL_Y_BITS   0x1
#define  IMG_IN_REG_LB_FULL_U   0x68
#define  IMG_IN_REG_LB_FULL_U_OFFSET 9
#define  IMG_IN_REG_LB_FULL_U_MASK   0x200
#define  IMG_IN_REG_LB_FULL_U_BITS   0x1
#define  IMG_IN_REG_LB_FULL_V   0x68
#define  IMG_IN_REG_LB_FULL_V_OFFSET 10
#define  IMG_IN_REG_LB_FULL_V_MASK   0x400
#define  IMG_IN_REG_LB_FULL_V_BITS   0x1
#define  IMG_IN_REG_LB_EMPTY_Y   0x68
#define  IMG_IN_REG_LB_EMPTY_Y_OFFSET 12
#define  IMG_IN_REG_LB_EMPTY_Y_MASK   0x1000
#define  IMG_IN_REG_LB_EMPTY_Y_BITS   0x1
#define  IMG_IN_REG_LB_EMPTY_U   0x68
#define  IMG_IN_REG_LB_EMPTY_U_OFFSET 13
#define  IMG_IN_REG_LB_EMPTY_U_MASK   0x2000
#define  IMG_IN_REG_LB_EMPTY_U_BITS   0x1
#define  IMG_IN_REG_LB_EMPTY_V   0x68
#define  IMG_IN_REG_LB_EMPTY_V_OFFSET 14
#define  IMG_IN_REG_LB_EMPTY_V_MASK   0x4000
#define  IMG_IN_REG_LB_EMPTY_V_BITS   0x1
#define  IMG_IN_REG_IP_IDLE   0x68
#define  IMG_IN_REG_IP_IDLE_OFFSET 16
#define  IMG_IN_REG_IP_IDLE_MASK   0x10000
#define  IMG_IN_REG_IP_IDLE_BITS   0x1
#define  IMG_IN_REG_IP_INT   0x68
#define  IMG_IN_REG_IP_INT_OFFSET 17
#define  IMG_IN_REG_IP_INT_MASK   0x20000
#define  IMG_IN_REG_IP_INT_BITS   0x1
#define  IMG_IN_REG_IP_CLR_W1T   0x68
#define  IMG_IN_REG_IP_CLR_W1T_OFFSET 18
#define  IMG_IN_REG_IP_CLR_W1T_MASK   0x40000
#define  IMG_IN_REG_IP_CLR_W1T_BITS   0x1
#define  IMG_IN_REG_CLR_INT_W1T   0x68
#define  IMG_IN_REG_CLR_INT_W1T_OFFSET 19
#define  IMG_IN_REG_CLR_INT_W1T_MASK   0x80000
#define  IMG_IN_REG_CLR_INT_W1T_BITS   0x1
#define  IMG_IN_REG_AXI_IDLE   0x70
#define  IMG_IN_REG_AXI_IDLE_OFFSET 0
#define  IMG_IN_REG_AXI_IDLE_MASK   0x1
#define  IMG_IN_REG_AXI_IDLE_BITS   0x1
#define  IMG_IN_REG_AXI_STATUS   0x70
#define  IMG_IN_REG_AXI_STATUS_OFFSET 8
#define  IMG_IN_REG_AXI_STATUS_MASK   0xff00
#define  IMG_IN_REG_AXI_STATUS_BITS   0x8
#define  IMG_IN_REG_BWL_WIN   0x74
#define  IMG_IN_REG_BWL_WIN_OFFSET 0
#define  IMG_IN_REG_BWL_WIN_MASK   0x3ff
#define  IMG_IN_REG_BWL_WIN_BITS   0xa
#define  IMG_IN_REG_BWL_VLD   0x74
#define  IMG_IN_REG_BWL_VLD_OFFSET 16
#define  IMG_IN_REG_BWL_VLD_MASK   0x3ff0000
#define  IMG_IN_REG_BWL_VLD_BITS   0xa
#define  IMG_IN_REG_BWL_EN   0x74
#define  IMG_IN_REG_BWL_EN_OFFSET 31
#define  IMG_IN_REG_BWL_EN_MASK   0x80000000
#define  IMG_IN_REG_BWL_EN_BITS   0x1
#define  IMG_IN_REG_CATCH_MODE   0x80
#define  IMG_IN_REG_CATCH_MODE_OFFSET 0
#define  IMG_IN_REG_CATCH_MODE_MASK   0x1
#define  IMG_IN_REG_CATCH_MODE_BITS   0x1
#define  IMG_IN_REG_DMA_URGENT_EN   0x80
#define  IMG_IN_REG_DMA_URGENT_EN_OFFSET 1
#define  IMG_IN_REG_DMA_URGENT_EN_MASK   0x2
#define  IMG_IN_REG_DMA_URGENT_EN_BITS   0x1
#define  IMG_IN_REG_QOS_SEL_RR   0x80
#define  IMG_IN_REG_QOS_SEL_RR_OFFSET 2
#define  IMG_IN_REG_QOS_SEL_RR_MASK   0x4
#define  IMG_IN_REG_QOS_SEL_RR_BITS   0x1
#define  IMG_IN_REG_CATCH_ACT_Y   0x80
#define  IMG_IN_REG_CATCH_ACT_Y_OFFSET 4
#define  IMG_IN_REG_CATCH_ACT_Y_MASK   0x10
#define  IMG_IN_REG_CATCH_ACT_Y_BITS   0x1
#define  IMG_IN_REG_CATCH_ACT_U   0x80
#define  IMG_IN_REG_CATCH_ACT_U_OFFSET 5
#define  IMG_IN_REG_CATCH_ACT_U_MASK   0x20
#define  IMG_IN_REG_CATCH_ACT_U_BITS   0x1
#define  IMG_IN_REG_CATCH_ACT_V   0x80
#define  IMG_IN_REG_CATCH_ACT_V_OFFSET 6
#define  IMG_IN_REG_CATCH_ACT_V_MASK   0x40
#define  IMG_IN_REG_CATCH_ACT_V_BITS   0x1
#define  IMG_IN_REG_CATCH_FAIL_Y   0x80
#define  IMG_IN_REG_CATCH_FAIL_Y_OFFSET 8
#define  IMG_IN_REG_CATCH_FAIL_Y_MASK   0x100
#define  IMG_IN_REG_CATCH_FAIL_Y_BITS   0x1
#define  IMG_IN_REG_CATCH_FAIL_U   0x80
#define  IMG_IN_REG_CATCH_FAIL_U_OFFSET 9
#define  IMG_IN_REG_CATCH_FAIL_U_MASK   0x200
#define  IMG_IN_REG_CATCH_FAIL_U_BITS   0x1
#define  IMG_IN_REG_CATCH_FAIL_V   0x80
#define  IMG_IN_REG_CATCH_FAIL_V_OFFSET 10
#define  IMG_IN_REG_CATCH_FAIL_V_MASK   0x400
#define  IMG_IN_REG_CATCH_FAIL_V_BITS   0x1
#define  IMG_IN_REG_CHKSUM_DAT_OUT   0x84
#define  IMG_IN_REG_CHKSUM_DAT_OUT_OFFSET 0
#define  IMG_IN_REG_CHKSUM_DAT_OUT_MASK   0xff
#define  IMG_IN_REG_CHKSUM_DAT_OUT_BITS   0x8
#define  IMG_IN_REG_CHECKSUM_EN   0x84
#define  IMG_IN_REG_CHECKSUM_EN_OFFSET 31
#define  IMG_IN_REG_CHECKSUM_EN_MASK   0x80000000
#define  IMG_IN_REG_CHECKSUM_EN_BITS   0x1
#define  IMG_IN_REG_CHKSUM_AXI_RD   0x88
#define  IMG_IN_REG_CHKSUM_AXI_RD_OFFSET 0
#define  IMG_IN_REG_CHKSUM_AXI_RD_MASK   0xffffffff
#define  IMG_IN_REG_CHKSUM_AXI_RD_BITS   0x20
#define  IMG_IN_REG_SB_MODE   0x90
#define  IMG_IN_REG_SB_MODE_OFFSET 0
#define  IMG_IN_REG_SB_MODE_MASK   0x3
#define  IMG_IN_REG_SB_MODE_BITS   0x2
#define  IMG_IN_REG_SB_SIZE   0x90
#define  IMG_IN_REG_SB_SIZE_OFFSET 2
#define  IMG_IN_REG_SB_SIZE_MASK   0xc
#define  IMG_IN_REG_SB_SIZE_BITS   0x2
#define  IMG_IN_REG_SB_NB   0x90
#define  IMG_IN_REG_SB_NB_OFFSET 8
#define  IMG_IN_REG_SB_NB_MASK   0xf00
#define  IMG_IN_REG_SB_NB_BITS   0x4
#define  IMG_IN_REG_SB_SW_RPTR   0x90
#define  IMG_IN_REG_SB_SW_RPTR_OFFSET 24
#define  IMG_IN_REG_SB_SW_RPTR_MASK   0xf000000
#define  IMG_IN_REG_SB_SW_RPTR_BITS   0x4
#define  IMG_IN_REG_SB_SET_STR   0x90
#define  IMG_IN_REG_SB_SET_STR_OFFSET 30
#define  IMG_IN_REG_SB_SET_STR_MASK   0x40000000
#define  IMG_IN_REG_SB_SET_STR_BITS   0x1
#define  IMG_IN_REG_SB_SW_CLR   0x90
#define  IMG_IN_REG_SB_SW_CLR_OFFSET 31
#define  IMG_IN_REG_SB_SW_CLR_MASK   0x80000000
#define  IMG_IN_REG_SB_SW_CLR_BITS   0x1
#define  IMG_IN_REG_U_SB_RPTR_RO   0x94
#define  IMG_IN_REG_U_SB_RPTR_RO_OFFSET 0
#define  IMG_IN_REG_U_SB_RPTR_RO_MASK   0xf
#define  IMG_IN_REG_U_SB_RPTR_RO_BITS   0x4
#define  IMG_IN_REG_U_SB_FULL   0x94
#define  IMG_IN_REG_U_SB_FULL_OFFSET 6
#define  IMG_IN_REG_U_SB_FULL_MASK   0x40
#define  IMG_IN_REG_U_SB_FULL_BITS   0x1
#define  IMG_IN_REG_U_SB_EMPTY   0x94
#define  IMG_IN_REG_U_SB_EMPTY_OFFSET 7
#define  IMG_IN_REG_U_SB_EMPTY_MASK   0x80
#define  IMG_IN_REG_U_SB_EMPTY_BITS   0x1
#define  IMG_IN_REG_U_SB_DPTR_RO   0x94
#define  IMG_IN_REG_U_SB_DPTR_RO_OFFSET 8
#define  IMG_IN_REG_U_SB_DPTR_RO_MASK   0x1f00
#define  IMG_IN_REG_U_SB_DPTR_RO_BITS   0x5
#define  IMG_IN_REG_V_SB_RPTR_RO   0x94
#define  IMG_IN_REG_V_SB_RPTR_RO_OFFSET 16
#define  IMG_IN_REG_V_SB_RPTR_RO_MASK   0xf0000
#define  IMG_IN_REG_V_SB_RPTR_RO_BITS   0x4
#define  IMG_IN_REG_V_SB_FULL   0x94
#define  IMG_IN_REG_V_SB_FULL_OFFSET 22
#define  IMG_IN_REG_V_SB_FULL_MASK   0x400000
#define  IMG_IN_REG_V_SB_FULL_BITS   0x1
#define  IMG_IN_REG_V_SB_EMPTY   0x94
#define  IMG_IN_REG_V_SB_EMPTY_OFFSET 23
#define  IMG_IN_REG_V_SB_EMPTY_MASK   0x800000
#define  IMG_IN_REG_V_SB_EMPTY_BITS   0x1
#define  IMG_IN_REG_V_SB_DPTR_RO   0x94
#define  IMG_IN_REG_V_SB_DPTR_RO_OFFSET 24
#define  IMG_IN_REG_V_SB_DPTR_RO_MASK   0x1f000000
#define  IMG_IN_REG_V_SB_DPTR_RO_BITS   0x5
#define  IMG_IN_REG_Y_SB_RPTR_RO   0x98
#define  IMG_IN_REG_Y_SB_RPTR_RO_OFFSET 0
#define  IMG_IN_REG_Y_SB_RPTR_RO_MASK   0xf
#define  IMG_IN_REG_Y_SB_RPTR_RO_BITS   0x4
#define  IMG_IN_REG_Y_SB_FULL   0x98
#define  IMG_IN_REG_Y_SB_FULL_OFFSET 5
#define  IMG_IN_REG_Y_SB_FULL_MASK   0x20
#define  IMG_IN_REG_Y_SB_FULL_BITS   0x1
#define  IMG_IN_REG_Y_SB_EMPTY   0x98
#define  IMG_IN_REG_Y_SB_EMPTY_OFFSET 6
#define  IMG_IN_REG_Y_SB_EMPTY_MASK   0x40
#define  IMG_IN_REG_Y_SB_EMPTY_BITS   0x1
#define  IMG_IN_REG_Y_SB_DPTR_RO   0x98
#define  IMG_IN_REG_Y_SB_DPTR_RO_OFFSET 8
#define  IMG_IN_REG_Y_SB_DPTR_RO_MASK   0x1f00
#define  IMG_IN_REG_Y_SB_DPTR_RO_BITS   0x5
#define  IMG_IN_REG_SB_EMPTY   0x98
#define  IMG_IN_REG_SB_EMPTY_OFFSET 15
#define  IMG_IN_REG_SB_EMPTY_MASK   0x8000
#define  IMG_IN_REG_SB_EMPTY_BITS   0x1
