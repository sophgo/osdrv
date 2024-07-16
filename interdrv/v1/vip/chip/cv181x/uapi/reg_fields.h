#ifndef _REG_FIELDS_H_
#define _REG_FIELDS_H_

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_PRE_RAW_BE_TOP_CTRL {
	uint32_t raw;
	struct {
		uint32_t BAYER_TYPE                      : 4;
		uint32_t RGBIR_EN                        : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t PCHK0_TRIG_SEL                  : 1;
		uint32_t PCHK1_TRIG_SEL                  : 1;
		uint32_t _rsv_10                         : 2;
		uint32_t BAYER_TYPE_AFTER_PREPROC        : 4;
		uint32_t INPUT_SEL                       : 1;
		uint32_t _rsv_17                         : 3;
		uint32_t WDMI_EN_LE                      : 1;
		uint32_t WDMI_EN_SE                      : 1;
		uint32_t DMA_WR_MODE                     : 1;
		uint32_t DMA_WR_MSB                      : 1;
		uint32_t CH_NUM                          : 1;
		uint32_t _rsv_25                         : 3;
		uint32_t GMS_IN_SEL                      : 1;
		uint32_t _rsv_29                         : 2;
		uint32_t SHDW_READ_SEL                   : 1;
	} bits;
};

union REG_PRE_RAW_BE_UP_PQ_EN {
	uint32_t raw;
	struct {
		uint32_t UP_PQ_EN                        : 1;
	} bits;
};

union REG_PRE_RAW_BE_RDMI_SIZE {
	uint32_t raw;
	struct {
		uint32_t RDMI_WIDTHM1                    : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t RDMI_HEIGHTM1                   : 13;
	} bits;
};

union REG_PRE_RAW_BE_CROP_SIZE_LE {
	uint32_t raw;
	struct {
		uint32_t CROP_WIDTHM1                    : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CROP_HEIGHTM1                   : 13;
	} bits;
};

union REG_PRE_RAW_BE_RDMI_DPCM {
	uint32_t raw;
	struct {
		uint32_t DPCM_MODE                       : 3;
		uint32_t _rsv_3                          : 13;
		uint32_t DPCM_XSTR                       : 13;
	} bits;
};

union REG_PRE_RAW_BE_FLOW_CTRL {
	uint32_t raw;
	struct {
		uint32_t _rsv_0                          : 8;
		uint32_t AWB_OUTPUT_IS_SE                : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t DEBUG_AWB_SRC_SEL               : 1;
	} bits;
};

union REG_PRE_RAW_BE_PRE_RAW_DUMMY {
	uint32_t raw;
	struct {
		uint32_t DUMMY_RW                        : 16;
		uint32_t DUMMY_RO                        : 16;
	} bits;
};

union REG_PRE_RAW_BE_PRE_RAW_POST_NO_RSPD_CYC {
	uint32_t raw;
	struct {
		uint32_t POST_NO_RSPD_CYC                : 32;
	} bits;
};

union REG_PRE_RAW_BE_PRE_RAW_DEBUG_STATE {
	uint32_t raw;
	struct {
		uint32_t LE_LINE_BUFF_EMPTY              : 1;
		uint32_t SE_LINE_BUFF_EMPTY              : 1;
	} bits;
};

union REG_PRE_RAW_BE_DEBUG_INFO {
	uint32_t raw;
	struct {
		uint32_t CH0_DMA_DONE                    : 1;
		uint32_t CH0_OUT_DONE                    : 1;
		uint32_t CH0_WDMI_CTRL_DONE              : 1;
		uint32_t CH0_AWB_DMA_DONE                : 1;
		uint32_t CH0_HIST_DONE                   : 1;
		uint32_t CH0_AE_DONE                     : 1;
		uint32_t CH0_IR_AE_DONE                  : 1;
		uint32_t CH0_IR_PROC_DONE                : 1;
		uint32_t CH0_GMS_DONE                    : 1;
		uint32_t CH0_AF_DONE                     : 1;
		uint32_t _rsv_10                         : 6;
		uint32_t CH1_DMA_DONE                    : 1;
		uint32_t CH1_OUT_DONE                    : 1;
		uint32_t CH1_WDMI_CTRL_DONE              : 1;
		uint32_t CH1_AWB_DMA_DONE                : 1;
		uint32_t CH1_HIST_DONE                   : 1;
		uint32_t CH1_AE_DONE                     : 1;
		uint32_t CH1_IR_AE_DONE                  : 1;
		uint32_t CH1_IR_PROC_DONE                : 1;
		uint32_t CH1_GMS_DONE                    : 1;
	} bits;
};

union REG_PRE_RAW_BE_LINE_BALANCE_CTRL {
	uint32_t raw;
	struct {
		uint32_t PASS_SEL                        : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t PASS_CNT_M1                     : 8;
	} bits;
};

union REG_PRE_RAW_BE_IP_INPUT_SEL {
	uint32_t raw;
	struct {
		uint32_t IR_WDMA_PROC_LSCR_ENABLE        : 1;
		uint32_t IR_AE_LSCR_ENABLE               : 1;
		uint32_t WBG_INV_LSCR_ENABLE             : 1;
		uint32_t _rsv_3                          : 1;
		uint32_t LE_AE_MERGER_ENABLE             : 1;
		uint32_t LE_IR_AE_MERGER_ENABLE          : 1;
		uint32_t SE_AE_MERGER_ENABLE             : 1;
		uint32_t SE_IR_AE_MERGER_ENABLE          : 1;
	} bits;
};

union REG_PRE_RAW_BE_RDMI_DEBUG_0 {
	uint32_t raw;
	struct {
		uint32_t RDMI_CH0_PXL_HCNT               : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t RDMI_CH0_PXL_VCNT               : 13;
	} bits;
};

union REG_PRE_RAW_BE_RDMI_DEBUG_1 {
	uint32_t raw;
	struct {
		uint32_t RDMI_CH1_PXL_HCNT               : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t RDMI_CH1_PXL_VCNT               : 13;
	} bits;
};

union REG_PRE_RAW_BE_IP_CONNECTION_DEBUG_0 {
	uint32_t raw;
	struct {
		uint32_t PCHK0_VALID                     : 32;
	} bits;
};

union REG_PRE_RAW_BE_IP_CONNECTION_DEBUG_1 {
	uint32_t raw;
	struct {
		uint32_t PCHK0_READY                     : 32;
	} bits;
};

union REG_PRE_RAW_BE_IP_CONNECTION_DEBUG_2 {
	uint32_t raw;
	struct {
		uint32_t PCHK1_VALID                     : 32;
	} bits;
};

union REG_PRE_RAW_BE_IP_CONNECTION_DEBUG_3 {
	uint32_t raw;
	struct {
		uint32_t PCHK1_READY                     : 32;
	} bits;
};

union REG_PRE_RAW_BE_IDLE_INFO {
	uint32_t raw;
	struct {
		uint32_t CH0_DMA_IDLE                    : 1;
		uint32_t CH0_OUT_IDLE                    : 1;
		uint32_t CH0_WDMI_CTRL_IDLE              : 1;
		uint32_t CH0_AWB_DMA_IDLE                : 1;
		uint32_t CH0_HIST_IDLE                   : 1;
		uint32_t CH0_AE_IDLE                     : 1;
		uint32_t CH0_IR_AE_IDLE                  : 1;
		uint32_t CH0_IR_PROC_IDLE                : 1;
		uint32_t CH0_GMS_IDLE                    : 1;
		uint32_t CH0_AF_IDLE                     : 1;
		uint32_t _rsv_10                         : 6;
		uint32_t CH1_DMA_IDLE                    : 1;
		uint32_t CH1_OUT_IDLE                    : 1;
		uint32_t CH1_WDMI_CTRL_IDLE              : 1;
		uint32_t CH1_AWB_DMA_IDLE                : 1;
		uint32_t CH1_HIST_IDLE                   : 1;
		uint32_t CH1_AE_IDLE                     : 1;
		uint32_t CH1_IR_AE_IDLE                  : 1;
		uint32_t CH1_IR_PROC_IDLE                : 1;
		uint32_t CH1_GMS_IDLE                    : 1;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_FPN_STATUS {
	uint32_t raw;
	struct {
		uint32_t FPN_STATUS                      : 32;
	} bits;
};

union REG_ISP_FPN_GRACE_RESET {
	uint32_t raw;
	struct {
		uint32_t FPN_GRACE_RESET                 : 1;
	} bits;
};

union REG_ISP_FPN_MONITOR {
	uint32_t raw;
	struct {
		uint32_t FPN_MONITOR                     : 32;
	} bits;
};

union REG_ISP_FPN_ENABLE {
	uint32_t raw;
	struct {
		uint32_t FPN_ENABLE                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t FPN_ENABLE_SHORTEXP             : 1;
		uint32_t _rsv_5                          : 11;
		uint32_t FPN_BYPASS                      : 1;
		uint32_t _rsv_17                         : 11;
		uint32_t FORCE_CLK_ENABLE                : 1;
	} bits;
};

union REG_ISP_FPN_MAP_ENABLE {
	uint32_t raw;
	struct {
		uint32_t FPN_MAP_ENABLE                  : 1;
	} bits;
};

union REG_ISP_FPN_FLOW {
	uint32_t raw;
	struct {
		uint32_t FPN_ZEROFPNOGRAM                : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t FPN_SHADOW_SELECT               : 1;
	} bits;
};

union REG_ISP_FPN_MONITOR_SELECT {
	uint32_t raw;
	struct {
		uint32_t FPN_MONITOR_SELECT              : 32;
	} bits;
};

union REG_ISP_FPN_LOCATION {
	uint32_t raw;
	struct {
		uint32_t FPN_LOCATION                    : 32;
	} bits;
};

union REG_ISP_FPN_MEM_SELECT {
	uint32_t raw;
	struct {
		uint32_t FPN_MEM_SELECT                  : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t FPN_MEM_PINGPONG_SELECT         : 1;
		uint32_t _rsv_17                         : 7;
		uint32_t FPN_MEM_ORDER_SELECT            : 1;
	} bits;
};

union REG_ISP_FPN_SECTION_INFO_0 {
	uint32_t raw;
	struct {
		uint32_t FPN_STARTPIXEL0                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t FPN_WIDTH0                      : 10;
	} bits;
};

union REG_ISP_FPN_SECTION_INFO_1 {
	uint32_t raw;
	struct {
		uint32_t FPN_STARTPIXEL1                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t FPN_WIDTH1                      : 10;
	} bits;
};

union REG_ISP_FPN_SECTION_INFO_2 {
	uint32_t raw;
	struct {
		uint32_t FPN_STARTPIXEL2                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t FPN_WIDTH2                      : 10;
	} bits;
};

union REG_ISP_FPN_SECTION_INFO_3 {
	uint32_t raw;
	struct {
		uint32_t FPN_STARTPIXEL3                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t FPN_WIDTH3                      : 10;
	} bits;
};

union REG_ISP_FPN_SECTION_INFO_4 {
	uint32_t raw;
	struct {
		uint32_t FPN_STARTPIXEL4                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t FPN_WIDTH4                      : 10;
	} bits;
};

union REG_ISP_FPN_DEBUG {
	uint32_t raw;
	struct {
		uint32_t FPN_DEBUG                       : 32;
	} bits;
};

union REG_ISP_FPN_IMG_BAYERID {
	uint32_t raw;
	struct {
		uint32_t IMG_BAYERID                     : 4;
		uint32_t _rsv_4                          : 12;
		uint32_t FORCE_BAYER_ENABLE              : 1;
	} bits;
};

union REG_ISP_FPN_DUMMY {
	uint32_t raw;
	struct {
		uint32_t FPN_DUMMY                       : 16;
	} bits;
};

union REG_ISP_FPN_PROG_DATA {
	uint32_t raw;
	struct {
		uint32_t FPN_DATA_OFFSET                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t FPN_DATA_GAIN                   : 12;
		uint32_t _rsv_28                         : 3;
		uint32_t FPN_W                           : 1;
	} bits;
};

union REG_ISP_FPN_PROG_CTRL {
	uint32_t raw;
	struct {
		uint32_t FPN_WSEL                        : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t FPN_RSEL                        : 1;
	} bits;
};

union REG_ISP_FPN_PROG_MAX {
	uint32_t raw;
	struct {
		uint32_t FPN_MAX                         : 12;
	} bits;
};

union REG_ISP_FPN_MEM_SW_MODE {
	uint32_t raw;
	struct {
		uint32_t FPN_MEM_SW_MODE                 : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t FPN_MEM_SEL                     : 4;
	} bits;
};

union REG_ISP_FPN_MEM_SW_RADDR {
	uint32_t raw;
	struct {
		uint32_t FPN_SW_RADDR                    : 8;
	} bits;
};

union REG_ISP_FPN_MEM_SW_RDATA_OFFSET {
	uint32_t raw;
	struct {
		uint32_t FPN_RDATA_OFFSET_R              : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t FPN_RDATA_OFFSET_GR             : 12;
		uint32_t _rsv_28                         : 3;
		uint32_t FPN_SW_R                        : 1;
	} bits;
};

union REG_ISP_FPN_MEM_SW_RDATA_OFFSET_BG {
	uint32_t raw;
	struct {
		uint32_t FPN_RDATA_OFFSET_GB             : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t FPN_RDATA_OFFSET_B              : 12;
	} bits;
};

union REG_ISP_FPN_MEM_SW_RDATA_GAIN {
	uint32_t raw;
	struct {
		uint32_t FPN_RDATA_GAIN_R                : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t FPN_RDATA_GAIN_GR               : 12;
	} bits;
};

union REG_ISP_FPN_MEM_SW_RDATA_GAIN_BG {
	uint32_t raw;
	struct {
		uint32_t FPN_RDATA_GAIN_GB               : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t FPN_RDATA_GAIN_B                : 12;
	} bits;
};

union REG_ISP_FPN_DBG {
	uint32_t raw;
	struct {
		uint32_t PROG_HDK_DIS                    : 1;
		uint32_t CONT_EN                         : 1;
		uint32_t RESERVED                        : 1;
		uint32_t DBG_EN                          : 1;
		uint32_t FPN_PROG_1TO4_EN                : 1;
		uint32_t _rsv_5                          : 11;
		uint32_t CHECK_SUM                       : 16;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_PREPROCESS_IR_PREPROC_CTRL {
	uint32_t raw;
	struct {
		uint32_t PREPROCESS_ENABLE               : 1;
		uint32_t IR_PREPROC_SHDW_SEL             : 1;
		uint32_t FORCE_CLK_ENABLE                : 1;
		uint32_t DELAY                           : 1;
		uint32_t CH_NM                           : 1;
	} bits;
};

union REG_ISP_PREPROCESS_IR_PREPROC_PROG_CTRL {
	uint32_t raw;
	struct {
		uint32_t MEM_WSEL                        : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t MEM_RSEL                        : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t MEM_PROG_EN                     : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t MEM_RATIO_RSEL                  : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t MEM_PROG_IDX                    : 2;
	} bits;
};

union REG_ISP_PREPROCESS_IR_PREPROC_PROG_ST_ADDR {
	uint32_t raw;
	struct {
		uint32_t MEM_ST_ADDR                     : 7;
		uint32_t _rsv_7                          : 24;
		uint32_t MEM_ST_W                        : 1;
	} bits;
};

union REG_ISP_PREPROCESS_IR_PREPROC_PROG_DATA {
	uint32_t raw;
	struct {
		uint32_t MEM_WDATA                       : 7;
		uint32_t _rsv_7                          : 24;
		uint32_t MEM_W                           : 1;
	} bits;
};

union REG_ISP_PREPROCESS_IR_PREPROC_MEM_SW_MODE {
	uint32_t raw;
	struct {
		uint32_t MEM_SW_RADDR                    : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t MEM_SW_RSEL                     : 1;
	} bits;
};

union REG_ISP_PREPROCESS_IR_PREPROC_MEM_SW_RDATA {
	uint32_t raw;
	struct {
		uint32_t MEM_RDATA                       : 14;
		uint32_t _rsv_14                         : 17;
		uint32_t MEM_SW_R                        : 1;
	} bits;
};

union REG_ISP_PREPROCESS_IR_PREPROC_MEM_RATIO_PROG_DATA {
	uint32_t raw;
	struct {
		uint32_t IR_RATIO_WDATA_E                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IR_RATIO_WDATA_O                : 14;
		uint32_t _rsv_30                         : 1;
		uint32_t IR_RATIO_WDATA_W                : 1;
	} bits;
};

union REG_ISP_PREPROCESS_IR_PREPROC_DBG {
	uint32_t raw;
	struct {
		uint32_t PROG_HDK_DIS                    : 1;
		uint32_t SOFTRST                         : 1;
	} bits;
};

union REG_ISP_PREPROCESS_IR_PREPROC_DMY0 {
	uint32_t raw;
	struct {
		uint32_t DMY_DEF0                        : 32;
	} bits;
};

union REG_ISP_PREPROCESS_IR_PREPROC_DMY1 {
	uint32_t raw;
	struct {
		uint32_t DMY_DEF1                        : 32;
	} bits;
};

union REG_ISP_PREPROCESS_IR_PREPROC_DMY_R {
	uint32_t raw;
	struct {
		uint32_t DMY_RO                          : 32;
	} bits;
};

union REG_ISP_PREPROCESS_IR_PREPROC_RATIO_0 {
	uint32_t raw;
	struct {
		uint32_t PREPROCESS_R_IR_RATIO           : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t PREPROCESS_G_IR_RATIO           : 14;
	} bits;
};

union REG_ISP_PREPROCESS_IR_PREPROC_RATIO_1 {
	uint32_t raw;
	struct {
		uint32_t PREPROCESS_B_IR_RATIO           : 14;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_IR_WDMA_PROC_IR_PROC_CTRL {
	uint32_t raw;
	struct {
		uint32_t IR_WDMA_PROC_EN                 : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t WDMI_EN_LE                      : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t WDMI_EN_SE                      : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t IR_DATA_SEL                     : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t IR_BIT_MODE                     : 1;
		uint32_t _rsv_17                         : 3;
		uint32_t IS_RGBIR                        : 1;
		uint32_t _rsv_21                         : 3;
		uint32_t FORCE_CLK_ENABLE                : 1;
		uint32_t FORCE_PCLK_ENABLE               : 1;
		uint32_t _rsv_26                         : 5;
		uint32_t SHDW_READ_SEL                   : 1;
	} bits;
};

union REG_IR_WDMA_PROC_GAMMA_CURVE_PROG_DATA {
	uint32_t raw;
	struct {
		uint32_t GAMMA_CURVE_DATA_E              : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t GAMMA_CURVE_DATA_O              : 8;
		uint32_t _rsv_24                         : 7;
		uint32_t GAMMA_CURVE_W                   : 1;
	} bits;
};

union REG_IR_WDMA_PROC_GAMMA_CURVE_PROG_CTRL {
	uint32_t raw;
	struct {
		uint32_t GAMMA_CURVE_WSEL                : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t GAMMA_CURVE_RSEL_LE             : 1;
		uint32_t GAMMA_CURVE_RSEL_SE             : 1;
	} bits;
};

union REG_IR_WDMA_PROC_GAMMA_CURVE_PROG_MAX {
	uint32_t raw;
	struct {
		uint32_t GAMMA_CURVE_MAX                 : 8;
	} bits;
};

union REG_IR_WDMA_PROC_GAMMA_CURVE_CTRL {
	uint32_t raw;
	struct {
		uint32_t GAMMA_CURVE_ADDR_RST            : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t GAMMA_CURVE_PROG_HDK_DIS        : 1;
	} bits;
};

union REG_IR_WDMA_PROC_GAMMA_CURVE_MEM_SW_MODE {
	uint32_t raw;
	struct {
		uint32_t GAMMA_CURVE_MEM_SW_MODE         : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t GAMMA_CURVE_MEM_SEL             : 1;
	} bits;
};

union REG_IR_WDMA_PROC_GAMMA_CURVE_MEM_SW_RADDR {
	uint32_t raw;
	struct {
		uint32_t GAMMA_CURVE_SW_RADDR            : 9;
	} bits;
};

union REG_IR_WDMA_PROC_GAMMA_CURVE_MEM_SW_RDATA {
	uint32_t raw;
	struct {
		uint32_t GAMMA_CURVE_RDATA               : 8;
		uint32_t _rsv_8                          : 23;
		uint32_t GAMMA_CURVE_SW_RD               : 1;
	} bits;
};

union REG_IR_WDMA_PROC_IR_PROC_DUMMY {
	uint32_t raw;
	struct {
		uint32_t DUMMY_RW                        : 16;
		uint32_t DUMMY_RO                        : 16;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_AE_HIST_AE_HIST_STATUS {
	uint32_t raw;
	struct {
		uint32_t AE_HIST_STATUS                  : 32;
	} bits;
};

union REG_ISP_AE_HIST_AE_HIST_GRACE_RESET {
	uint32_t raw;
	struct {
		uint32_t AE_HIST_GRACE_RESET             : 1;
	} bits;
};

union REG_ISP_AE_HIST_AE_HIST_MONITOR {
	uint32_t raw;
	struct {
		uint32_t AE_HIST_MONITOR                 : 32;
	} bits;
};

union REG_ISP_AE_HIST_AE_HIST_BYPASS {
	uint32_t raw;
	struct {
		uint32_t AE_HIST_BYPASS                  : 1;
		uint32_t _rsv_1                          : 19;
		uint32_t HIST_ZEROING_ENABLE             : 1;
		uint32_t _rsv_21                         : 7;
		uint32_t FORCE_CLK_ENABLE                : 1;
	} bits;
};

union REG_ISP_AE_HIST_AE_KICKOFF {
	uint32_t raw;
	struct {
		uint32_t AE_ZERO_AE_SUM                  : 1;
		uint32_t _rsv_1                          : 1;
		uint32_t AE_WBGAIN_APPLY                 : 1;
		uint32_t _rsv_3                          : 1;
		uint32_t LOADSHADOWREG                   : 1;
		uint32_t _rsv_5                          : 1;
		uint32_t HIST_ZEROHISTOGRAM              : 1;
		uint32_t _rsv_7                          : 1;
		uint32_t HIST_WBGAIN_APPLY               : 1;
		uint32_t _rsv_9                          : 1;
		uint32_t AE_HIST_SHADOW_SELECT           : 1;
		uint32_t _rsv_11                         : 5;
		uint32_t AE_FACE_ENABLE                  : 4;
	} bits;
};

union REG_ISP_AE_HIST_AE_HIST_ENABLE {
	uint32_t raw;
	struct {
		uint32_t AE0_ENABLE                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t AE0_GAIN_ENABLE                 : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t HIST0_ENABLE                    : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t HIST0_GAIN_ENABLE               : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t IR_AE_ENABLE                    : 1;
		uint32_t _rsv_17                         : 3;
		uint32_t IR_AE_GAIN_ENABLE               : 1;
		uint32_t _rsv_21                         : 3;
		uint32_t IR_HIST_ENABLE                  : 1;
		uint32_t _rsv_25                         : 3;
		uint32_t IR_HIST_GAIN_ENABLE             : 1;
	} bits;
};

union REG_ISP_AE_HIST_STS_AE_OFFSETX {
	uint32_t raw;
	struct {
		uint32_t STS_AE0_OFFSETX                 : 13;
	} bits;
};

union REG_ISP_AE_HIST_STS_AE_OFFSETY {
	uint32_t raw;
	struct {
		uint32_t STS_AE0_OFFSETY                 : 13;
	} bits;
};

union REG_ISP_AE_HIST_STS_AE_NUMXM1 {
	uint32_t raw;
	struct {
		uint32_t STS_AE0_NUMXM1                  : 5;
	} bits;
};

union REG_ISP_AE_HIST_STS_AE_NUMYM1 {
	uint32_t raw;
	struct {
		uint32_t STS_AE0_NUMYM1                  : 5;
	} bits;
};

union REG_ISP_AE_HIST_STS_AE_WIDTH {
	uint32_t raw;
	struct {
		uint32_t STS_AE0_WIDTH                   : 10;
	} bits;
};

union REG_ISP_AE_HIST_STS_AE_HEIGHT {
	uint32_t raw;
	struct {
		uint32_t STS_AE0_HEIGHT                  : 10;
	} bits;
};

union REG_ISP_AE_HIST_STS_AE_STS_DIV {
	uint32_t raw;
	struct {
		uint32_t STS_AE0_STS_DIV                 : 3;
	} bits;
};

union REG_ISP_AE_HIST_STS_HIST_MODE {
	uint32_t raw;
	struct {
		uint32_t STS_HIST0_MODE                  : 2;
	} bits;
};

union REG_ISP_AE_HIST_AE_HIST_MONITOR_SELECT {
	uint32_t raw;
	struct {
		uint32_t AE_HIST_MONITOR_SELECT          : 32;
	} bits;
};

union REG_ISP_AE_HIST_AE_HIST_LOCATION {
	uint32_t raw;
	struct {
		uint32_t AE_HIST_LOCATION                : 32;
	} bits;
};

union REG_ISP_AE_HIST_STS_IR_AE_OFFSETX {
	uint32_t raw;
	struct {
		uint32_t STS_IR_AE_OFFSETX               : 13;
	} bits;
};

union REG_ISP_AE_HIST_STS_IR_AE_OFFSETY {
	uint32_t raw;
	struct {
		uint32_t STS_IR_AE_OFFSETY               : 13;
	} bits;
};

union REG_ISP_AE_HIST_STS_IR_AE_NUMXM1 {
	uint32_t raw;
	struct {
		uint32_t STS_IR_AE_NUMXM1                : 5;
	} bits;
};

union REG_ISP_AE_HIST_STS_IR_AE_NUMYM1 {
	uint32_t raw;
	struct {
		uint32_t STS_IR_AE_NUMYM1                : 5;
	} bits;
};

union REG_ISP_AE_HIST_STS_IR_AE_WIDTH {
	uint32_t raw;
	struct {
		uint32_t STS_IR_AE_WIDTH                 : 10;
	} bits;
};

union REG_ISP_AE_HIST_STS_IR_AE_HEIGHT {
	uint32_t raw;
	struct {
		uint32_t STS_IR_AE_HEIGHT                : 10;
	} bits;
};

union REG_ISP_AE_HIST_STS_IR_AE_STS_DIV {
	uint32_t raw;
	struct {
		uint32_t STS_IR_AE_STS_DIV               : 3;
	} bits;
};

union REG_ISP_AE_HIST_AE_HIST_BAYER_STARTING {
	uint32_t raw;
	struct {
		uint32_t AE_HIST_BAYER_STARTING          : 4;
		uint32_t _rsv_4                          : 12;
		uint32_t FORCE_BAYER_ENABLE              : 1;
	} bits;
};

union REG_ISP_AE_HIST_AE_HIST_DUMMY {
	uint32_t raw;
	struct {
		uint32_t AE_HIST_DUMMY                   : 16;
	} bits;
};

union REG_ISP_AE_HIST_AE_HIST_CHECKSUM {
	uint32_t raw;
	struct {
		uint32_t AE_HIST_CHECKSUM                : 32;
	} bits;
};

union REG_ISP_AE_HIST_WBG_4 {
	uint32_t raw;
	struct {
		uint32_t AE0_WBG_RGAIN                   : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t AE0_WBG_GGAIN                   : 14;
	} bits;
};

union REG_ISP_AE_HIST_WBG_5 {
	uint32_t raw;
	struct {
		uint32_t AE0_WBG_BGAIN                   : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t AE1_WBG_BGAIN                   : 14;
	} bits;
};

union REG_ISP_AE_HIST_WBG_6 {
	uint32_t raw;
	struct {
		uint32_t AE1_WBG_RGAIN                   : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t AE1_WBG_GGAIN                   : 14;
	} bits;
};

union REG_ISP_AE_HIST_WBG_7 {
	uint32_t raw;
	struct {
		uint32_t AE0_WBG_VGAIN                   : 14;
	} bits;
};

union REG_ISP_AE_HIST_DMI_ENABLE {
	uint32_t raw;
	struct {
		uint32_t DMI_ENABLE                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t DMI_QOS                         : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t FORCE_DMA_DISABLE               : 1;
		uint32_t _rsv_9                          : 7;
		uint32_t IR_DMI_ENABLE                   : 1;
		uint32_t _rsv_17                         : 3;
		uint32_t IR_DMI_QOS                      : 1;
		uint32_t _rsv_21                         : 3;
		uint32_t IR_FORCE_DMA_DISABLE            : 1;
	} bits;
};

union REG_ISP_AE_HIST_AE_FACE0_LOCATION {
	uint32_t raw;
	struct {
		uint32_t AE_FACE0_OFFSET_X               : 16;
		uint32_t AE_FACE0_OFFSET_Y               : 16;
	} bits;
};

union REG_ISP_AE_HIST_AE_FACE1_LOCATION {
	uint32_t raw;
	struct {
		uint32_t AE_FACE1_OFFSET_X               : 16;
		uint32_t AE_FACE1_OFFSET_Y               : 16;
	} bits;
};

union REG_ISP_AE_HIST_AE_FACE2_LOCATION {
	uint32_t raw;
	struct {
		uint32_t AE_FACE2_OFFSET_X               : 16;
		uint32_t AE_FACE2_OFFSET_Y               : 16;
	} bits;
};

union REG_ISP_AE_HIST_AE_FACE3_LOCATION {
	uint32_t raw;
	struct {
		uint32_t AE_FACE3_OFFSET_X               : 16;
		uint32_t AE_FACE3_OFFSET_Y               : 16;
	} bits;
};

union REG_ISP_AE_HIST_AE_FACE0_SIZE {
	uint32_t raw;
	struct {
		uint32_t AE_FACE0_SIZE_MINUS1_X          : 7;
		uint32_t _rsv_7                          : 9;
		uint32_t AE_FACE0_SIZE_MINUS1_Y          : 7;
	} bits;
};

union REG_ISP_AE_HIST_AE_FACE1_SIZE {
	uint32_t raw;
	struct {
		uint32_t AE_FACE1_SIZE_MINUS1_X          : 7;
		uint32_t _rsv_7                          : 9;
		uint32_t AE_FACE1_SIZE_MINUS1_Y          : 7;
	} bits;
};

union REG_ISP_AE_HIST_AE_FACE2_SIZE {
	uint32_t raw;
	struct {
		uint32_t AE_FACE2_SIZE_MINUS1_X          : 7;
		uint32_t _rsv_7                          : 9;
		uint32_t AE_FACE2_SIZE_MINUS1_Y          : 7;
	} bits;
};

union REG_ISP_AE_HIST_AE_FACE3_SIZE {
	uint32_t raw;
	struct {
		uint32_t AE_FACE3_SIZE_MINUS1_X          : 7;
		uint32_t _rsv_7                          : 9;
		uint32_t AE_FACE3_SIZE_MINUS1_Y          : 7;
	} bits;
};

union REG_ISP_AE_HIST_IR_AE_FACE0_LOCATION {
	uint32_t raw;
	struct {
		uint32_t IR_AE_FACE0_OFFSET_X            : 16;
		uint32_t IR_AE_FACE0_OFFSET_Y            : 16;
	} bits;
};

union REG_ISP_AE_HIST_IR_AE_FACE1_LOCATION {
	uint32_t raw;
	struct {
		uint32_t IR_AE_FACE1_OFFSET_X            : 16;
		uint32_t IR_AE_FACE1_OFFSET_Y            : 16;
	} bits;
};

union REG_ISP_AE_HIST_IR_AE_FACE2_LOCATION {
	uint32_t raw;
	struct {
		uint32_t IR_AE_FACE2_OFFSET_X            : 16;
		uint32_t IR_AE_FACE2_OFFSET_Y            : 16;
	} bits;
};

union REG_ISP_AE_HIST_IR_AE_FACE3_LOCATION {
	uint32_t raw;
	struct {
		uint32_t IR_AE_FACE3_OFFSET_X            : 16;
		uint32_t IR_AE_FACE3_OFFSET_Y            : 16;
	} bits;
};

union REG_ISP_AE_HIST_IR_AE_FACE0_SIZE {
	uint32_t raw;
	struct {
		uint32_t IR_AE_FACE0_SIZE_MINUS1_X       : 7;
		uint32_t _rsv_7                          : 9;
		uint32_t IR_AE_FACE0_SIZE_MINUS1_Y       : 7;
	} bits;
};

union REG_ISP_AE_HIST_IR_AE_FACE1_SIZE {
	uint32_t raw;
	struct {
		uint32_t IR_AE_FACE1_SIZE_MINUS1_X       : 7;
		uint32_t _rsv_7                          : 9;
		uint32_t IR_AE_FACE1_SIZE_MINUS1_Y       : 7;
	} bits;
};

union REG_ISP_AE_HIST_IR_AE_FACE2_SIZE {
	uint32_t raw;
	struct {
		uint32_t IR_AE_FACE2_SIZE_MINUS1_X       : 7;
		uint32_t _rsv_7                          : 9;
		uint32_t IR_AE_FACE2_SIZE_MINUS1_Y       : 7;
	} bits;
};

union REG_ISP_AE_HIST_IR_AE_FACE3_SIZE {
	uint32_t raw;
	struct {
		uint32_t IR_AE_FACE3_SIZE_MINUS1_X       : 7;
		uint32_t _rsv_7                          : 9;
		uint32_t IR_AE_FACE3_SIZE_MINUS1_Y       : 7;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_AWB_STATUS {
	uint32_t raw;
	struct {
		uint32_t AWB_STATUS                      : 32;
	} bits;
};

union REG_ISP_AWB_GRACE_RESET {
	uint32_t raw;
	struct {
		uint32_t AWB_GRACE_RESET                 : 1;
	} bits;
};

union REG_ISP_AWB_MONITOR {
	uint32_t raw;
	struct {
		uint32_t AWB_MONITOR                     : 32;
	} bits;
};

union REG_ISP_AWB_SHADOW_SELECT {
	uint32_t raw;
	struct {
		uint32_t AWB_SHADOW_SELECT               : 1;
	} bits;
};

union REG_ISP_AWB_KICKOFF {
	uint32_t raw;
	struct {
		uint32_t AWB_ZERO_AWB_SUM                : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t AWB_WBGAIN_APPLY                : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t AWB_LOADSHADOWREG               : 1;
	} bits;
};

union REG_ISP_AWB_ENABLE {
	uint32_t raw;
	struct {
		uint32_t AWB_ENABLE                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t AWB_GAIN_ENABLE                 : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t AWB_SOURCE                      : 2;
		uint32_t _rsv_10                         : 6;
		uint32_t AWB_EOWYWJP_FOOTPRINT           : 1;
		uint32_t _rsv_17                         : 3;
		uint32_t AWB_ENDIANNESS_SELECT           : 1;
		uint32_t _rsv_21                         : 7;
		uint32_t FORCE_CLK_ENABLE                : 1;
	} bits;
};

union REG_ISP_AWB_MONITOR_SELECT {
	uint32_t raw;
	struct {
		uint32_t AWB_MONITOR_SELECT              : 32;
	} bits;
};

union REG_ISP_AWB_STS_OFFSETX {
	uint32_t raw;
	struct {
		uint32_t STS_AWB_OFFSETX                 : 13;
	} bits;
};

union REG_ISP_AWB_STS_OFFSETY {
	uint32_t raw;
	struct {
		uint32_t STS_AWB_OFFSETY                 : 13;
	} bits;
};

union REG_ISP_AWB_STS_NUMXM1 {
	uint32_t raw;
	struct {
		uint32_t STS_AWB_NUMXM1                  : 7;
	} bits;
};

union REG_ISP_AWB_STS_NUMYM1 {
	uint32_t raw;
	struct {
		uint32_t STS_AWB_NUMYM1                  : 7;
	} bits;
};

union REG_ISP_AWB_STS_WIDTH {
	uint32_t raw;
	struct {
		uint32_t STS_AWB_WIDTH                   : 7;
	} bits;
};

union REG_ISP_AWB_STS_HEIGHT {
	uint32_t raw;
	struct {
		uint32_t STS_AWB_HEIGHT                  : 7;
	} bits;
};

union REG_ISP_AWB_STS_SKIPX {
	uint32_t raw;
	struct {
		uint32_t STS_AWB_SKIPX                   : 3;
	} bits;
};

union REG_ISP_AWB_STS_SKIPY {
	uint32_t raw;
	struct {
		uint32_t STS_AWB_SKIPY                   : 3;
	} bits;
};

union REG_ISP_AWB_STS_CORNER_AVG_EN {
	uint32_t raw;
	struct {
		uint32_t STS_AWB_CORNER_AVG_EN           : 1;
	} bits;
};

union REG_ISP_AWB_STS_CORNER_SIZE {
	uint32_t raw;
	struct {
		uint32_t STS_AWB_CORNER_SIZE             : 3;
	} bits;
};

union REG_ISP_AWB_STS_STS_DIV {
	uint32_t raw;
	struct {
		uint32_t STS_AWB_STS_DIV                 : 3;
	} bits;
};

union REG_ISP_AWB_STS_R_LOTHD {
	uint32_t raw;
	struct {
		uint32_t STS_AWB_R_LOTHD                 : 12;
	} bits;
};

union REG_ISP_AWB_STS_R_UPTHD {
	uint32_t raw;
	struct {
		uint32_t STS_AWB_R_UPTHD                 : 12;
	} bits;
};

union REG_ISP_AWB_STS_G_LOTHD {
	uint32_t raw;
	struct {
		uint32_t STS_AWB_G_LOTHD                 : 12;
	} bits;
};

union REG_ISP_AWB_STS_G_UPTHD {
	uint32_t raw;
	struct {
		uint32_t STS_AWB_G_UPTHD                 : 12;
	} bits;
};

union REG_ISP_AWB_STS_B_LOTHD {
	uint32_t raw;
	struct {
		uint32_t STS_AWB_B_LOTHD                 : 12;
	} bits;
};

union REG_ISP_AWB_STS_B_UPTHD {
	uint32_t raw;
	struct {
		uint32_t STS_AWB_B_UPTHD                 : 12;
	} bits;
};

union REG_ISP_AWB_WBG_4 {
	uint32_t raw;
	struct {
		uint32_t AWB_WBG_RGAIN                   : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t AWB_WBG_GGAIN                   : 14;
	} bits;
};

union REG_ISP_AWB_WBG_5 {
	uint32_t raw;
	struct {
		uint32_t AWB_WBG_BGAIN                   : 14;
	} bits;
};

union REG_ISP_AWB_MEM_SW_MODE {
	uint32_t raw;
	struct {
		uint32_t AWB_MEM_SW_MODE                 : 1;
	} bits;
};

union REG_ISP_AWB_MEM_SW_RADDR {
	uint32_t raw;
	struct {
		uint32_t AWB_SW_RADDR                    : 7;
	} bits;
};

union REG_ISP_AWB_LOCATION {
	uint32_t raw;
	struct {
		uint32_t AWB_LOCATION                    : 32;
	} bits;
};

union REG_ISP_AWB_MEM_SW_RDATA {
	uint32_t raw;
	struct {
		uint32_t AWB_RDATA                       : 31;
		uint32_t AWB_SW_READ                     : 1;
	} bits;
};

union REG_ISP_AWB_BAYER_STARTING {
	uint32_t raw;
	struct {
		uint32_t AWB_BAYER_STARTING              : 4;
		uint32_t _rsv_4                          : 12;
		uint32_t FORCE_BAYER_ENABLE              : 1;
	} bits;
};

union REG_ISP_AWB_DUMMY {
	uint32_t raw;
	struct {
		uint32_t AWB_DUMMY                       : 16;
	} bits;
};

union REG_ISP_AWB_DMI_ENABLE {
	uint32_t raw;
	struct {
		uint32_t DMI_ENABLE                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t DMI_QOS                         : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t FORCE_DMA_DISABLE               : 1;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_GMS_STATUS {
	uint32_t raw;
	struct {
		uint32_t GMS_STATUS                      : 32;
	} bits;
};

union REG_ISP_GMS_GRACE_RESET {
	uint32_t raw;
	struct {
		uint32_t GMS_GRACE_RESET                 : 1;
	} bits;
};

union REG_ISP_GMS_MONITOR {
	uint32_t raw;
	struct {
		uint32_t GMS_MONITOR                     : 32;
	} bits;
};

union REG_ISP_GMS_ENABLE {
	uint32_t raw;
	struct {
		uint32_t GMS_ENABLE                      : 1;
		uint32_t _rsv_1                          : 27;
		uint32_t FORCE_CLK_ENABLE                : 1;
	} bits;
};

union REG_ISP_GMS_IMG_BAYERID {
	uint32_t raw;
	struct {
		uint32_t IMG_BAYERID                     : 4;
		uint32_t _rsv_4                          : 12;
		uint32_t FORCE_BAYER_ENABLE              : 1;
	} bits;
};

union REG_ISP_GMS_FLOW {
	uint32_t raw;
	struct {
		uint32_t GMS_ZEROGMSOGRAM                : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t GMS_SHADOW_SELECT               : 1;
	} bits;
};

union REG_ISP_GMS_START_X {
	uint32_t raw;
	struct {
		uint32_t GMS_START_X                     : 13;
	} bits;
};

union REG_ISP_GMS_START_Y {
	uint32_t raw;
	struct {
		uint32_t GMS_START_Y                     : 13;
	} bits;
};

union REG_ISP_GMS_LOCATION {
	uint32_t raw;
	struct {
		uint32_t GMS_LOCATION                    : 32;
	} bits;
};

union REG_ISP_GMS_X_SECTION_SIZE {
	uint32_t raw;
	struct {
		uint32_t GMS_X_SECTION_SIZE              : 10;
	} bits;
};

union REG_ISP_GMS_Y_SECTION_SIZE {
	uint32_t raw;
	struct {
		uint32_t GMS_Y_SECTION_SIZE              : 10;
	} bits;
};

union REG_ISP_GMS_X_GAP {
	uint32_t raw;
	struct {
		uint32_t GMS_X_GAP                       : 10;
	} bits;
};

union REG_ISP_GMS_Y_GAP {
	uint32_t raw;
	struct {
		uint32_t GMS_Y_GAP                       : 10;
	} bits;
};

union REG_ISP_GMS_DUMMY {
	uint32_t raw;
	struct {
		uint32_t GMS_DUMMY                       : 16;
	} bits;
};

union REG_ISP_GMS_MEM_SW_MODE {
	uint32_t raw;
	struct {
		uint32_t GMS_MEM_SW_MODE                 : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t GMS_MEM_SEL                     : 6;
	} bits;
};

union REG_ISP_GMS_MEM_SW_RADDR {
	uint32_t raw;
	struct {
		uint32_t GMS_SW_RADDR                    : 11;
	} bits;
};

union REG_ISP_GMS_MEM_SW_RDATA {
	uint32_t raw;
	struct {
		uint32_t GMS_RDATA_R                     : 31;
		uint32_t GMS_SW_R                        : 1;
	} bits;
};

union REG_ISP_GMS_MONITOR_SELECT {
	uint32_t raw;
	struct {
		uint32_t GMS_MONITOR_SELECT              : 32;
	} bits;
};

union REG_ISP_GMS_DMI_ENABLE {
	uint32_t raw;
	struct {
		uint32_t DMI_ENABLE                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t DMI_QOS                         : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t FORCE_DMA_DISABLE               : 1;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_AF_STATUS {
	uint32_t raw;
	struct {
		uint32_t AF_STATUS                       : 32;
	} bits;
};

union REG_ISP_AF_GRACE_RESET {
	uint32_t raw;
	struct {
		uint32_t AF_GRACE_RESET                  : 1;
	} bits;
};

union REG_ISP_AF_MONITOR {
	uint32_t raw;
	struct {
		uint32_t AF_MONITOR                      : 32;
	} bits;
};

union REG_ISP_AF_BYPASS {
	uint32_t raw;
	struct {
		uint32_t AF_BYPASS                       : 1;
	} bits;
};

union REG_ISP_AF_KICKOFF {
	uint32_t raw;
	struct {
		uint32_t AF_ENABLE                       : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t AF_WBGAIN_APPLY                 : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t AF_REVERT_EXPOSURE              : 1;
		uint32_t _rsv_9                          : 7;
		uint32_t AF_GAIN_ENABLE                  : 1;
	} bits;
};

union REG_ISP_AF_ENABLES {
	uint32_t raw;
	struct {
		uint32_t AF_HORIZON_0_ENABLE             : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t AF_HORIZON_1_ENABLE             : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t AF_VERTICAL_0_ENABLE            : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t AF_GAMMA_ENABLE                 : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t AF_DPC_ENABLE                   : 1;
		uint32_t _rsv_17                         : 3;
		uint32_t AF_HLC_ENABLE                   : 1;
		uint32_t _rsv_21                         : 3;
		uint32_t AF_SHADOW_SELECT                : 1;
		uint32_t _rsv_25                         : 3;
		uint32_t FORCE_CLK_ENABLE                : 1;
	} bits;
};

union REG_ISP_AF_OFFSET_X {
	uint32_t raw;
	struct {
		uint32_t AF_OFFSET_X                     : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t AF_OFFSET_Y                     : 13;
	} bits;
};

union REG_ISP_AF_MXN_IMAGE_WIDTH_M1 {
	uint32_t raw;
	struct {
		uint32_t AF_MXN_IMAGE_WIDTH              : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t AF_MXN_IMAGE_HEIGHT             : 13;
	} bits;
};

union REG_ISP_AF_BLOCK_WIDTH {
	uint32_t raw;
	struct {
		uint32_t AF_BLOCK_WIDTH                  : 8;
	} bits;
};

union REG_ISP_AF_BLOCK_HEIGHT {
	uint32_t raw;
	struct {
		uint32_t AF_BLOCK_HEIGHT                 : 8;
	} bits;
};

union REG_ISP_AF_BLOCK_NUM_X {
	uint32_t raw;
	struct {
		uint32_t AF_BLOCK_NUM_X                  : 5;
	} bits;
};

union REG_ISP_AF_BLOCK_NUM_Y {
	uint32_t raw;
	struct {
		uint32_t AF_BLOCK_NUM_Y                  : 5;
	} bits;
};

union REG_ISP_AF_CROP_BAYERID {
	uint32_t raw;
	struct {
		uint32_t AF_CROP_BAYERID                 : 4;
		uint32_t _rsv_4                          : 12;
		uint32_t FORCE_BAYER_ENABLE              : 1;
	} bits;
};

union REG_ISP_AF_HOR_LOW_PASS_VALUE_SHIFT {
	uint32_t raw;
	struct {
		uint32_t AF_HOR_LOW_PASS_VALUE_SHIFT     : 4;
	} bits;
};

union REG_ISP_AF_CORNING_OFFSET_HORIZONTAL_0 {
	uint32_t raw;
	struct {
		uint32_t AF_CORNING_OFFSET_HORIZONTAL_0  : 18;
	} bits;
};

union REG_ISP_AF_CORNING_OFFSET_HORIZONTAL_1 {
	uint32_t raw;
	struct {
		uint32_t AF_CORNING_OFFSET_HORIZONTAL_1  : 18;
	} bits;
};

union REG_ISP_AF_CORNING_OFFSET_VERTICAL {
	uint32_t raw;
	struct {
		uint32_t AF_CORNING_OFFSET_VERTICAL      : 13;
	} bits;
};

union REG_ISP_AF_HIGH_Y_THRE {
	uint32_t raw;
	struct {
		uint32_t AF_HIGH_Y_THRE                  : 12;
	} bits;
};

union REG_ISP_AF_LOW_PASS_HORIZON {
	uint32_t raw;
	struct {
		uint32_t AF_LOW_PASS_HORIZON_0           : 6;
		uint32_t AF_LOW_PASS_HORIZON_1           : 6;
		uint32_t AF_LOW_PASS_HORIZON_2           : 6;
		uint32_t AF_LOW_PASS_HORIZON_3           : 6;
		uint32_t AF_LOW_PASS_HORIZON_4           : 6;
	} bits;
};

union REG_ISP_AF_LOCATION {
	uint32_t raw;
	struct {
		uint32_t AF_LOCATION                     : 32;
	} bits;
};

union REG_ISP_AF_HIGH_PASS_HORIZON_0 {
	uint32_t raw;
	struct {
		uint32_t AF_HIGH_PASS_HORIZON_0_0        : 6;
		uint32_t AF_HIGH_PASS_HORIZON_0_1        : 6;
		uint32_t AF_HIGH_PASS_HORIZON_0_2        : 6;
		uint32_t AF_HIGH_PASS_HORIZON_0_3        : 6;
		uint32_t AF_HIGH_PASS_HORIZON_0_4        : 6;
	} bits;
};

union REG_ISP_AF_HIGH_PASS_HORIZON_1 {
	uint32_t raw;
	struct {
		uint32_t AF_HIGH_PASS_HORIZON_1_0        : 6;
		uint32_t AF_HIGH_PASS_HORIZON_1_1        : 6;
		uint32_t AF_HIGH_PASS_HORIZON_1_2        : 6;
		uint32_t AF_HIGH_PASS_HORIZON_1_3        : 6;
		uint32_t AF_HIGH_PASS_HORIZON_1_4        : 6;
	} bits;
};

union REG_ISP_AF_HIGH_PASS_VERTICAL_0 {
	uint32_t raw;
	struct {
		uint32_t AF_HIGH_PASS_VERTICAL_0_0       : 6;
		uint32_t AF_HIGH_PASS_VERTICAL_0_1       : 6;
		uint32_t AF_HIGH_PASS_VERTICAL_0_2       : 6;
	} bits;
};

union REG_ISP_AF_MEM_SW_MODE {
	uint32_t raw;
	struct {
		uint32_t AF_MEM_SW_MODE                  : 1;
		uint32_t AF_R_MEM_SEL                    : 1;
		uint32_t AF_G_MEM_SEL                    : 1;
		uint32_t AF_B_MEM_SEL                    : 1;
		uint32_t AF_BLK_DIV_MEM_SEL              : 1;
		uint32_t AF_GAMMA_G_MEM_SEL              : 1;
		uint32_t AF_MAGFACTOR_MEM_SEL            : 1;
		uint32_t AF_BLK_DIV_DFF_SEL              : 1;
		uint32_t AF_GAMMA_G_DFF_SEL              : 1;
		uint32_t AF_MAGFACTOR_DFF_SEL            : 1;
	} bits;
};

union REG_ISP_AF_MONITOR_SELECT {
	uint32_t raw;
	struct {
		uint32_t AF_MONITOR_SELECT               : 32;
	} bits;
};

union REG_ISP_AF_IMAGE_WIDTH {
	uint32_t raw;
	struct {
		uint32_t AF_IMAGE_WIDTH                  : 16;
	} bits;
};

union REG_ISP_AF_DUMMY {
	uint32_t raw;
	struct {
		uint32_t AF_DUMMY                        : 16;
	} bits;
};

union REG_ISP_AF_MEM_SW_RADDR {
	uint32_t raw;
	struct {
		uint32_t AF_SW_RADDR                     : 7;
	} bits;
};

union REG_ISP_AF_MEM_SW_RDATA {
	uint32_t raw;
	struct {
		uint32_t AF_RDATA                        : 31;
		uint32_t AF_SW_READ                      : 1;
	} bits;
};

union REG_ISP_AF_MXN_BORDER {
	uint32_t raw;
	struct {
		uint32_t AF_MXN_BORDER                   : 2;
	} bits;
};

union REG_ISP_AF_TH_LOW    {
	uint32_t raw;
	struct {
		uint32_t AF_TH_LOW                       : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t AF_TH_HIGH                      : 8;
	} bits;
};

union REG_ISP_AF_GAIN_LOW  {
	uint32_t raw;
	struct {
		uint32_t AF_GAIN_LOW                     : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t AF_GAIN_HIGH                    : 8;
	} bits;
};

union REG_ISP_AF_SLOP_LOW {
	uint32_t raw;
	struct {
		uint32_t AF_SLOP_LOW                     : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t AF_SLOP_HIGH                    : 8;
	} bits;
};

union REG_ISP_AF_DMI_ENABLE {
	uint32_t raw;
	struct {
		uint32_t DMI_ENABLE                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t DMI_QOS                         : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t FORCE_DMA_DISABLE               : 1;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_PRE_RAW_FE_PRE_RAW_CTRL {
	uint32_t raw;
	struct {
		uint32_t BAYER_FRONT_TYPE                : 4;
		uint32_t BAYER_TYPE                      : 4;
		uint32_t BAYER_FRONT_TYPE_SE             : 4;
		uint32_t BAYER_TYPE_SE                   : 4;
		uint32_t UP_PQ_EN                        : 1;
		uint32_t _rsv_17                         : 3;
		uint32_t RGBIR_EN                        : 1;
		uint32_t _rsv_21                         : 3;
		uint32_t PCHK0_TRIG_SEL                  : 1;
		uint32_t PCHK1_TRIG_SEL                  : 1;
		uint32_t _rsv_26                         : 5;
		uint32_t SHDW_READ_SEL                   : 1;
	} bits;
};

union REG_PRE_RAW_FE_PRE_RAW_FRAME_SIZE {
	uint32_t raw;
	struct {
		uint32_t FRAME_WIDTHM1                   : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t FRAME_HEIGHTM1                  : 13;
	} bits;
};

union REG_PRE_RAW_FE_LE_LMAP_GRID_NUMBER {
	uint32_t raw;
	struct {
		uint32_t LE_LMP_H_GRID_NUMM1             : 10;
		uint32_t _rsv_10                         : 2;
		uint32_t LE_LMP_H_GRID_SIZE              : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t LE_LMP_V_GRID_NUMM1             : 10;
		uint32_t _rsv_26                         : 2;
		uint32_t LE_LMP_V_GRID_SIZE              : 3;
	} bits;
};

union REG_PRE_RAW_FE_SE_LMAP_GRID_NUMBER {
	uint32_t raw;
	struct {
		uint32_t SE_LMP_H_GRID_NUMM1             : 10;
		uint32_t _rsv_10                         : 2;
		uint32_t SE_LMP_H_GRID_SIZE              : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t SE_LMP_V_GRID_NUMM1             : 10;
		uint32_t _rsv_26                         : 2;
		uint32_t SE_LMP_V_GRID_SIZE              : 3;
	} bits;
};

union REG_PRE_RAW_FE_LE_RGBMAP_GRID_NUMBER {
	uint32_t raw;
	struct {
		uint32_t LE_RGBMP_H_GRID_NUMM1           : 10;
		uint32_t _rsv_10                         : 2;
		uint32_t LE_RGBMP_H_GRID_SIZE            : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t LE_RGBMP_V_GRID_NUMM1           : 10;
		uint32_t _rsv_26                         : 2;
		uint32_t LE_RGBMP_V_GRID_SIZE            : 3;
	} bits;
};

union REG_PRE_RAW_FE_SE_RGBMAP_GRID_NUMBER {
	uint32_t raw;
	struct {
		uint32_t SE_RGBMP_H_GRID_NUMM1           : 10;
		uint32_t _rsv_10                         : 2;
		uint32_t SE_RGBMP_H_GRID_SIZE            : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t SE_RGBMP_V_GRID_NUMM1           : 10;
		uint32_t _rsv_26                         : 2;
		uint32_t SE_RGBMP_V_GRID_SIZE            : 3;
	} bits;
};

union REG_PRE_RAW_FE_PRE_RAW_POST_NO_RSPD_CYC {
	uint32_t raw;
	struct {
		uint32_t POST_NO_RSPD_CYC                : 32;
	} bits;
};

union REG_PRE_RAW_FE_PRE_RAW_POST_RGBMAP_NO_RSPD_CYC {
	uint32_t raw;
	struct {
		uint32_t POST_RGBMAP_NO_RSPD_CYC         : 32;
	} bits;
};

union REG_PRE_RAW_FE_PRE_RAW_FRAME_VLD {
	uint32_t raw;
	struct {
		uint32_t PRE_RAW_FRAME_VLD_CH0           : 1;
		uint32_t PRE_RAW_FRAME_VLD_CH1           : 1;
		uint32_t PRE_RAW_FRAME_VLD_CH2           : 1;
		uint32_t PRE_RAW_FRAME_VLD_CH3           : 1;
		uint32_t PRE_RAW_PQ_VLD_CH0              : 1;
		uint32_t PRE_RAW_PQ_VLD_CH1              : 1;
		uint32_t PRE_RAW_PQ_VLD_CH2              : 1;
		uint32_t PRE_RAW_PQ_VLD_CH3              : 1;
	} bits;
};

union REG_PRE_RAW_FE_PRE_RAW_DEBUG_STATE {
	uint32_t raw;
	struct {
		uint32_t POST_RAW_IDLE                   : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t PRE_RAW_FE_IDLE                 : 4;
	} bits;
};

union REG_PRE_RAW_FE_PRE_RAW_DUMMY {
	uint32_t raw;
	struct {
		uint32_t DUMMY_RW                        : 16;
		uint32_t DUMMY_RO                        : 16;
	} bits;
};

union REG_PRE_RAW_FE_DEBUG_INFO {
	uint32_t raw;
	struct {
		uint32_t CH0_DMA_DONE                    : 1;
		uint32_t CH0_CSI_BDG_DONE                : 1;
		uint32_t CH0_RGBMP_ASYNC_DONE            : 1;
		uint32_t CH0_YUV_ASYNC_DONE              : 1;
		uint32_t CH0_BYR_ASYNC_DONE              : 1;
		uint32_t CH0_RGBMP_DONE                  : 1;
		uint32_t CH0_LMP_DONE                    : 1;
		uint32_t CH1_DMA_DONE                    : 1;
		uint32_t CH1_CSI_BDG_DONE                : 1;
		uint32_t CH1_BYR_ASYNC_DONE              : 1;
		uint32_t CH1_RGBMP_DONE                  : 1;
		uint32_t CH1_LMP_DONE                    : 1;
		uint32_t CH2_DMA_DONE                    : 1;
		uint32_t CH2_CSI_BDG_DONE                : 1;
		uint32_t CH3_DMA_DONE                    : 1;
		uint32_t CH3_CSI_BDG_DONE                : 1;
	} bits;
};

union REG_PRE_RAW_FE_IP_INPUT_SEL {
	uint32_t raw;
	struct {
		uint32_t LMP_LSCR_ENABLE                 : 1;
		uint32_t RGBMP_LSCR_ENABLE               : 1;
	} bits;
};

union REG_PRE_RAW_FE_IP_CONNECTION_DEBUG_0 {
	uint32_t raw;
	struct {
		uint32_t PCHK0_VALID                     : 32;
	} bits;
};

union REG_PRE_RAW_FE_IP_CONNECTION_DEBUG_1 {
	uint32_t raw;
	struct {
		uint32_t PCHK0_READY                     : 32;
	} bits;
};

union REG_PRE_RAW_FE_IP_CONNECTION_DEBUG_2 {
	uint32_t raw;
	struct {
		uint32_t PCHK1_VALID                     : 32;
	} bits;
};

union REG_PRE_RAW_FE_IP_CONNECTION_DEBUG_3 {
	uint32_t raw;
	struct {
		uint32_t PCHK1_READY                     : 32;
	} bits;
};

union REG_PRE_RAW_FE_IDLE_INFO {
	uint32_t raw;
	struct {
		uint32_t CH0_DMA_IDLE                    : 1;
		uint32_t CH0_CSI_BDG_IDLE                : 1;
		uint32_t CH0_RGBMP_ASYNC_IDLE            : 1;
		uint32_t CH0_YUV_ASYNC_IDLE              : 1;
		uint32_t CH0_BYR_ASYNC_IDLE              : 1;
		uint32_t CH0_RGBMP_IDLE                  : 1;
		uint32_t CH0_LMP_IDLE                    : 1;
		uint32_t CH1_DMA_IDLE                    : 1;
		uint32_t CH1_CSI_BDG_IDLE                : 1;
		uint32_t CH1_BYR_ASYNC_IDLE              : 1;
		uint32_t CH1_RGBMP_IDLE                  : 1;
		uint32_t CH1_LMP_IDLE                    : 1;
		uint32_t CH2_DMA_IDLE                    : 1;
		uint32_t CH2_CSI_BDG_IDLE                : 1;
		uint32_t CH3_DMA_IDLE                    : 1;
		uint32_t CH3_CSI_BDG_IDLE                : 1;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_CSI_BDG_TOP_CTRL {
	uint32_t raw;
	struct {
		uint32_t CSI_MODE                        : 2;
		uint32_t CSI_IN_FORMAT                   : 1;
		uint32_t CSI_IN_YUV_FORMAT               : 1;
		uint32_t CH_NUM                          : 2;
		uint32_t CH0_DMA_WR_ENABLE               : 1;
		uint32_t CH1_DMA_WR_ENABLE               : 1;
		uint32_t CH2_DMA_WR_ENABLE               : 1;
		uint32_t Y_ONLY                          : 1;
		uint32_t PXL_DATA_SEL                    : 1;
		uint32_t VS_POL                          : 1;
		uint32_t HS_POL                          : 1;
		uint32_t RESET_MODE                      : 1;
		uint32_t VS_MODE                         : 1;
		uint32_t ABORT_MODE                      : 1;
		uint32_t RESET                           : 1;
		uint32_t ABORT                           : 1;
		uint32_t CH3_DMA_WR_ENABLE               : 1;
		uint32_t _rsv_19                         : 1;
		uint32_t YUV_PACK_MODE                   : 1;
		uint32_t MULTI_CH_FRAME_SYNC_EN          : 1;
		uint32_t _rsv_22                         : 2;
		uint32_t CSI_ENABLE                      : 1;
		uint32_t TGEN_ENABLE                     : 1;
		uint32_t YUV2BAY_ENABLE                  : 1;
		uint32_t _rsv_27                         : 1;
		uint32_t SHDW_READ_SEL                   : 1;
		uint32_t _rsv_29                         : 2;
		uint32_t CSI_UP_REG                      : 1;
	} bits;
};

union REG_ISP_CSI_BDG_INTERRUPT_CTRL {
	uint32_t raw;
	struct {
		uint32_t CH0_VS_INT_EN                   : 1;
		uint32_t CH0_TRIG_INT_EN                 : 1;
		uint32_t CH0_DROP_INT_EN                 : 1;
		uint32_t CH0_SIZE_ERROR_INT_EN           : 1;
		uint32_t CH1_VS_INT_EN                   : 1;
		uint32_t CH1_TRIG_INT_EN                 : 1;
		uint32_t CH1_DROP_INT_EN                 : 1;
		uint32_t CH1_SIZE_ERROR_INT_EN           : 1;
		uint32_t CH2_VS_INT_EN                   : 1;
		uint32_t CH2_TRIG_INT_EN                 : 1;
		uint32_t CH2_DROP_INT_EN                 : 1;
		uint32_t CH2_SIZE_ERROR_INT_EN           : 1;
		uint32_t CH3_VS_INT_EN                   : 1;
		uint32_t CH3_TRIG_INT_EN                 : 1;
		uint32_t CH3_DROP_INT_EN                 : 1;
		uint32_t CH3_SIZE_ERROR_INT_EN           : 1;
		uint32_t _rsv_16                         : 12;
		uint32_t SLICE_LINE_INTP_EN              : 1;
		uint32_t DMA_ERROR_INTP_EN               : 1;
		uint32_t LINE_INTP_EN                    : 1;
		uint32_t FIFO_OVERFLOW_INT_EN            : 1;
	} bits;
};

union REG_ISP_CSI_BDG_DMA_DPCM_MODE {
	uint32_t raw;
	struct {
		uint32_t DMA_ST_DPCM_MODE                : 3;
		uint32_t _rsv_3                          : 13;
		uint32_t DPCM_XSTR                       : 13;
	} bits;
};

union REG_ISP_CSI_BDG_CH0_SIZE {
	uint32_t raw;
	struct {
		uint32_t CH0_FRAME_WIDTHM1               : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH0_FRAME_HEIGHTM1              : 13;
	} bits;
};

union REG_ISP_CSI_BDG_CH1_SIZE {
	uint32_t raw;
	struct {
		uint32_t CH1_FRAME_WIDTHM1               : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH1_FRAME_HEIGHTM1              : 13;
	} bits;
};

union REG_ISP_CSI_BDG_CH2_SIZE {
	uint32_t raw;
	struct {
		uint32_t CH2_FRAME_WIDTHM1               : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH2_FRAME_HEIGHTM1              : 13;
	} bits;
};

union REG_ISP_CSI_BDG_CH3_SIZE {
	uint32_t raw;
	struct {
		uint32_t CH3_FRAME_WIDTHM1               : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH3_FRAME_HEIGHTM1              : 13;
	} bits;
};

union REG_ISP_CSI_BDG_CH0_CROP_EN {
	uint32_t raw;
	struct {
		uint32_t CH0_CROP_EN                     : 1;
	} bits;
};

union REG_ISP_CSI_BDG_CH0_HORZ_CROP {
	uint32_t raw;
	struct {
		uint32_t CH0_HORZ_CROP_START             : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH0_HORZ_CROP_END               : 13;
	} bits;
};

union REG_ISP_CSI_BDG_CH0_VERT_CROP {
	uint32_t raw;
	struct {
		uint32_t CH0_VERT_CROP_START             : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH0_VERT_CROP_END               : 13;
	} bits;
};

union REG_ISP_CSI_BDG_CH0_BLC_SUM {
	uint32_t raw;
	struct {
		uint32_t CH0_BLC_SUM                     : 32;
	} bits;
};

union REG_ISP_CSI_BDG_CH1_CROP_EN {
	uint32_t raw;
	struct {
		uint32_t CH1_CROP_EN                     : 1;
	} bits;
};

union REG_ISP_CSI_BDG_CH1_HORZ_CROP {
	uint32_t raw;
	struct {
		uint32_t CH1_HORZ_CROP_START             : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH1_HORZ_CROP_END               : 13;
	} bits;
};

union REG_ISP_CSI_BDG_CH1_VERT_CROP {
	uint32_t raw;
	struct {
		uint32_t CH1_VERT_CROP_START             : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH1_VERT_CROP_END               : 13;
	} bits;
};

union REG_ISP_CSI_BDG_CH1_BLC_SUM {
	uint32_t raw;
	struct {
		uint32_t CH1_BLC_SUM                     : 32;
	} bits;
};

union REG_ISP_CSI_BDG_CH2_CROP_EN {
	uint32_t raw;
	struct {
		uint32_t CH2_CROP_EN                     : 1;
	} bits;
};

union REG_ISP_CSI_BDG_CH2_HORZ_CROP {
	uint32_t raw;
	struct {
		uint32_t CH2_HORZ_CROP_START             : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH2_HORZ_CROP_END               : 13;
	} bits;
};

union REG_ISP_CSI_BDG_CH2_VERT_CROP {
	uint32_t raw;
	struct {
		uint32_t CH2_VERT_CROP_START             : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH2_VERT_CROP_END               : 13;
	} bits;
};

union REG_ISP_CSI_BDG_CH2_BLC_SUM {
	uint32_t raw;
	struct {
		uint32_t CH2_BLC_SUM                     : 32;
	} bits;
};

union REG_ISP_CSI_BDG_CH3_CROP_EN {
	uint32_t raw;
	struct {
		uint32_t CH3_CROP_EN                     : 1;
	} bits;
};

union REG_ISP_CSI_BDG_CH3_HORZ_CROP {
	uint32_t raw;
	struct {
		uint32_t CH3_HORZ_CROP_START             : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH3_HORZ_CROP_END               : 13;
	} bits;
};

union REG_ISP_CSI_BDG_CH3_VERT_CROP {
	uint32_t raw;
	struct {
		uint32_t CH3_VERT_CROP_START             : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH3_VERT_CROP_END               : 13;
	} bits;
};

union REG_ISP_CSI_BDG_CH3_BLC_SUM {
	uint32_t raw;
	struct {
		uint32_t CH3_BLC_SUM                     : 32;
	} bits;
};

union REG_ISP_CSI_BDG_PAT_GEN_CTRL {
	uint32_t raw;
	struct {
		uint32_t PAT_EN                          : 1;
		uint32_t GRA_INV                         : 1;
		uint32_t AUTO_EN                         : 1;
		uint32_t DITH_EN                         : 1;
		uint32_t SNOW_EN                         : 1;
		uint32_t FIX_MC                          : 1;
		uint32_t _rsv_6                          : 2;
		uint32_t DITH_MD                         : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t BAYER_ID                        : 2;
	} bits;
};

union REG_ISP_CSI_BDG_PAT_IDX_CTRL {
	uint32_t raw;
	struct {
		uint32_t PAT_PRD                         : 8;
		uint32_t PAT_IDX                         : 5;
	} bits;
};

union REG_ISP_CSI_BDG_PAT_COLOR_0 {
	uint32_t raw;
	struct {
		uint32_t PAT_R                           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t PAT_G                           : 12;
	} bits;
};

union REG_ISP_CSI_BDG_PAT_COLOR_1 {
	uint32_t raw;
	struct {
		uint32_t PAT_B                           : 12;
	} bits;
};

union REG_ISP_CSI_BDG_BACKGROUND_COLOR_0 {
	uint32_t raw;
	struct {
		uint32_t FDE_R                           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t FDE_G                           : 12;
	} bits;
};

union REG_ISP_CSI_BDG_BACKGROUND_COLOR_1 {
	uint32_t raw;
	struct {
		uint32_t FDE_B                           : 12;
	} bits;
};

union REG_ISP_CSI_BDG_FIX_COLOR_0 {
	uint32_t raw;
	struct {
		uint32_t MDE_R                           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t MDE_G                           : 12;
	} bits;
};

union REG_ISP_CSI_BDG_FIX_COLOR_1 {
	uint32_t raw;
	struct {
		uint32_t MDE_B                           : 12;
	} bits;
};

union REG_ISP_CSI_BDG_MDE_V_SIZE {
	uint32_t raw;
	struct {
		uint32_t VMDE_STR                        : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t VMDE_STP                        : 13;
	} bits;
};

union REG_ISP_CSI_BDG_MDE_H_SIZE {
	uint32_t raw;
	struct {
		uint32_t HMDE_STR                        : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t HMDE_STP                        : 13;
	} bits;
};

union REG_ISP_CSI_BDG_FDE_V_SIZE {
	uint32_t raw;
	struct {
		uint32_t VFDE_STR                        : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t VFDE_STP                        : 13;
	} bits;
};

union REG_ISP_CSI_BDG_FDE_H_SIZE {
	uint32_t raw;
	struct {
		uint32_t HFDE_STR                        : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t HFDE_STP                        : 13;
	} bits;
};

union REG_ISP_CSI_BDG_HSYNC_CTRL {
	uint32_t raw;
	struct {
		uint32_t HS_STR                          : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t HS_STP                          : 13;
	} bits;
};

union REG_ISP_CSI_BDG_VSYNC_CTRL {
	uint32_t raw;
	struct {
		uint32_t VS_STR                          : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t VS_STP                          : 13;
	} bits;
};

union REG_ISP_CSI_BDG_TGEN_TT_SIZE {
	uint32_t raw;
	struct {
		uint32_t HTT                             : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t VTT                             : 13;
	} bits;
};

union REG_ISP_CSI_BDG_LINE_INTP_HEIGHT_0 {
	uint32_t raw;
	struct {
		uint32_t CH0_LINE_INTP_HEIGHTM1          : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH1_LINE_INTP_HEIGHTM1          : 13;
	} bits;
};

union REG_ISP_CSI_BDG_CH0_DEBUG_0 {
	uint32_t raw;
	struct {
		uint32_t CH0_PXL_CNT                     : 32;
	} bits;
};

union REG_ISP_CSI_BDG_CH0_DEBUG_1 {
	uint32_t raw;
	struct {
		uint32_t CH0_LINE_CNT                    : 16;
	} bits;
};

union REG_ISP_CSI_BDG_CH0_DEBUG_2 {
	uint32_t raw;
	struct {
		uint32_t CH0_VS_CNT                      : 16;
		uint32_t CH0_TRIG_CNT                    : 16;
	} bits;
};

union REG_ISP_CSI_BDG_CH0_DEBUG_3 {
	uint32_t raw;
	struct {
		uint32_t CH0_TOT_BLK_IDLE                : 1;
		uint32_t CH0_TOT_DMA_IDLE                : 1;
		uint32_t CH0_BDG_DMA_IDLE                : 1;
	} bits;
};

union REG_ISP_CSI_BDG_CH1_DEBUG_0 {
	uint32_t raw;
	struct {
		uint32_t CH1_PXL_CNT                     : 32;
	} bits;
};

union REG_ISP_CSI_BDG_CH1_DEBUG_1 {
	uint32_t raw;
	struct {
		uint32_t CH1_LINE_CNT                    : 16;
	} bits;
};

union REG_ISP_CSI_BDG_CH1_DEBUG_2 {
	uint32_t raw;
	struct {
		uint32_t CH1_VS_CNT                      : 16;
		uint32_t CH1_TRIG_CNT                    : 16;
	} bits;
};

union REG_ISP_CSI_BDG_CH1_DEBUG_3 {
	uint32_t raw;
	struct {
		uint32_t CH1_TOT_BLK_IDLE                : 1;
		uint32_t CH1_TOT_DMA_IDLE                : 1;
		uint32_t CH1_BDG_DMA_IDLE                : 1;
	} bits;
};

union REG_ISP_CSI_BDG_CH2_DEBUG_0 {
	uint32_t raw;
	struct {
		uint32_t CH2_PXL_CNT                     : 32;
	} bits;
};

union REG_ISP_CSI_BDG_CH2_DEBUG_1 {
	uint32_t raw;
	struct {
		uint32_t CH2_LINE_CNT                    : 16;
	} bits;
};

union REG_ISP_CSI_BDG_CH2_DEBUG_2 {
	uint32_t raw;
	struct {
		uint32_t CH2_VS_CNT                      : 16;
		uint32_t CH2_TRIG_CNT                    : 16;
	} bits;
};

union REG_ISP_CSI_BDG_CH2_DEBUG_3 {
	uint32_t raw;
	struct {
		uint32_t CH2_TOT_BLK_IDLE                : 1;
		uint32_t CH2_TOT_DMA_IDLE                : 1;
		uint32_t CH2_BDG_DMA_IDLE                : 1;
	} bits;
};

union REG_ISP_CSI_BDG_CH3_DEBUG_0 {
	uint32_t raw;
	struct {
		uint32_t CH3_PXL_CNT                     : 32;
	} bits;
};

union REG_ISP_CSI_BDG_CH3_DEBUG_1 {
	uint32_t raw;
	struct {
		uint32_t CH3_LINE_CNT                    : 16;
	} bits;
};

union REG_ISP_CSI_BDG_CH3_DEBUG_2 {
	uint32_t raw;
	struct {
		uint32_t CH3_VS_CNT                      : 16;
		uint32_t CH3_TRIG_CNT                    : 16;
	} bits;
};

union REG_ISP_CSI_BDG_CH3_DEBUG_3 {
	uint32_t raw;
	struct {
		uint32_t CH3_TOT_BLK_IDLE                : 1;
		uint32_t CH3_TOT_DMA_IDLE                : 1;
		uint32_t CH3_BDG_DMA_IDLE                : 1;
	} bits;
};

union REG_ISP_CSI_BDG_INTERRUPT_STATUS_0 {
	uint32_t raw;
	struct {
		uint32_t CH0_FRAME_DROP_INT              : 1;
		uint32_t CH0_VS_INT                      : 1;
		uint32_t CH0_TRIG_INT                    : 1;
		uint32_t _rsv_3                          : 1;
		uint32_t CH0_FRAME_WIDTH_GT_INT          : 1;
		uint32_t CH0_FRAME_WIDTH_LS_INT          : 1;
		uint32_t CH0_FRAME_HEIGHT_GT_INT         : 1;
		uint32_t CH0_FRAME_HEIGHT_LS_INT         : 1;
		uint32_t CH1_FRAME_DROP_INT              : 1;
		uint32_t CH1_VS_INT                      : 1;
		uint32_t CH1_TRIG_INT                    : 1;
		uint32_t _rsv_11                         : 1;
		uint32_t CH1_FRAME_WIDTH_GT_INT          : 1;
		uint32_t CH1_FRAME_WIDTH_LS_INT          : 1;
		uint32_t CH1_FRAME_HEIGHT_GT_INT         : 1;
		uint32_t CH1_FRAME_HEIGHT_LS_INT         : 1;
		uint32_t CH2_FRAME_DROP_INT              : 1;
		uint32_t CH2_VS_INT                      : 1;
		uint32_t CH2_TRIG_INT                    : 1;
		uint32_t _rsv_19                         : 1;
		uint32_t CH2_FRAME_WIDTH_GT_INT          : 1;
		uint32_t CH2_FRAME_WIDTH_LS_INT          : 1;
		uint32_t CH2_FRAME_HEIGHT_GT_INT         : 1;
		uint32_t CH2_FRAME_HEIGHT_LS_INT         : 1;
		uint32_t CH3_FRAME_DROP_INT              : 1;
		uint32_t CH3_VS_INT                      : 1;
		uint32_t CH3_TRIG_INT                    : 1;
		uint32_t _rsv_27                         : 1;
		uint32_t CH3_FRAME_WIDTH_GT_INT          : 1;
		uint32_t CH3_FRAME_WIDTH_LS_INT          : 1;
		uint32_t CH3_FRAME_HEIGHT_GT_INT         : 1;
		uint32_t CH3_FRAME_HEIGHT_LS_INT         : 1;
	} bits;
};

union REG_ISP_CSI_BDG_INTERRUPT_STATUS_1 {
	uint32_t raw;
	struct {
		uint32_t FIFO_OVERFLOW_INT               : 1;
		uint32_t FRAME_RESOLUTION_OVER_MAX_INT   : 1;
		uint32_t _rsv_2                          : 2;
		uint32_t DMA_ERROR_INT                   : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t CH0_LINE_INTP_INT               : 1;
		uint32_t CH1_LINE_INTP_INT               : 1;
		uint32_t CH2_LINE_INTP_INT               : 1;
		uint32_t CH3_LINE_INTP_INT               : 1;
		uint32_t CH0_SLICE_LINE_INTP_INT         : 1;
		uint32_t CH1_SLICE_LINE_INTP_INT         : 1;
		uint32_t CH2_SLICE_LINE_INTP_INT         : 1;
		uint32_t CH3_SLICE_LINE_INTP_INT         : 1;
	} bits;
};

union REG_ISP_CSI_BDG_DEBUG {
	uint32_t raw;
	struct {
		uint32_t RING_BUFF_IDLE                  : 1;
	} bits;
};

union REG_ISP_CSI_BDG_OUT_VSYNC_LINE_DELAY {
	uint32_t raw;
	struct {
		uint32_t OUT_VSYNC_LINE_DELAY            : 12;
	} bits;
};

union REG_ISP_CSI_BDG_WR_URGENT_CTRL {
	uint32_t raw;
	struct {
		uint32_t WR_NEAR_OVERFLOW_THRESHOLD      : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t WR_SAFE_THRESHOLD               : 13;
	} bits;
};

union REG_ISP_CSI_BDG_RD_URGENT_CTRL {
	uint32_t raw;
	struct {
		uint32_t RD_NEAR_OVERFLOW_THRESHOLD      : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t RD_SAFE_THRESHOLD               : 13;
	} bits;
};

union REG_ISP_CSI_BDG_DUMMY {
	uint32_t raw;
	struct {
		uint32_t DUMMY_IN                        : 16;
		uint32_t DUMMY_OUT                       : 16;
	} bits;
};

union REG_ISP_CSI_BDG_LINE_INTP_HEIGHT_1 {
	uint32_t raw;
	struct {
		uint32_t CH2_LINE_INTP_HEIGHTM1          : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH3_LINE_INTP_HEIGHTM1          : 13;
	} bits;
};

union REG_ISP_CSI_BDG_SLICE_LINE_INTP_HEIGHT_0 {
	uint32_t raw;
	struct {
		uint32_t CH0_SLICE_LINE_INTP_HEIGHTM1    : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH1_SLICE_LINE_INTP_HEIGHTM1    : 13;
	} bits;
};

union REG_ISP_CSI_BDG_SLICE_LINE_INTP_HEIGHT_1 {
	uint32_t raw;
	struct {
		uint32_t CH2_SLICE_LINE_INTP_HEIGHTM1    : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH3_SLICE_LINE_INTP_HEIGHTM1    : 13;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_CROP_0 {
	uint32_t raw;
	struct {
		uint32_t CROP_ENABLE                     : 1;
		uint32_t DMA_ENABLE                      : 1;
		uint32_t SHAW_READ_SEL                   : 1;
		uint32_t DMI_QOS                         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t DPCM_MODE                       : 3;
		uint32_t _rsv_11                         : 5;
		uint32_t DPCM_XSTR                       : 13;
		uint32_t _rsv_29                         : 2;
		uint32_t DMI16B_EN                       : 1;
	} bits;
};

union REG_ISP_CROP_1 {
	uint32_t raw;
	struct {
		uint32_t CROP_START_Y                    : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CROP_END_Y                      : 13;
	} bits;
};

union REG_ISP_CROP_2 {
	uint32_t raw;
	struct {
		uint32_t CROP_START_X                    : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CROP_END_X                      : 13;
	} bits;
};

union REG_ISP_CROP_3 {
	uint32_t raw;
	struct {
		uint32_t IN_WIDTHM1                      : 16;
		uint32_t IN_HEIGHTM1                     : 16;
	} bits;
};

union REG_ISP_CROP_DUMMY {
	uint32_t raw;
	struct {
		uint32_t DUMMY                           : 32;
	} bits;
};

union REG_ISP_CROP_DEBUG {
	uint32_t raw;
	struct {
		uint32_t FORCE_CLK_ENABLE                : 1;
		uint32_t FORCE_DMA_DISABLE               : 1;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_BLC_0 {
	uint32_t raw;
	struct {
		uint32_t BLC_BYPASS                      : 1;
	} bits;
};

union REG_ISP_BLC_1 {
	uint32_t raw;
	struct {
		uint32_t SHDW_READ_SEL                   : 1;
	} bits;
};

union REG_ISP_BLC_2 {
	uint32_t raw;
	struct {
		uint32_t BLC_ENABLE                      : 1;
		uint32_t _rsv_1                          : 27;
		uint32_t FORCE_CLK_ENABLE                : 1;
	} bits;
};

union REG_ISP_BLC_3 {
	uint32_t raw;
	struct {
		uint32_t BLC_OFFSET_R                    : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t BLC_OFFSET_GR                   : 12;
	} bits;
};

union REG_ISP_BLC_4 {
	uint32_t raw;
	struct {
		uint32_t BLC_OFFSET_GB                   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t BLC_OFFSET_B                    : 12;
	} bits;
};

union REG_ISP_BLC_5 {
	uint32_t raw;
	struct {
		uint32_t BLC_GAIN_R                      : 16;
		uint32_t BLC_GAIN_GR                     : 16;
	} bits;
};

union REG_ISP_BLC_6 {
	uint32_t raw;
	struct {
		uint32_t BLC_GAIN_GB                     : 16;
		uint32_t BLC_GAIN_B                      : 16;
	} bits;
};

union REG_ISP_BLC_7 {
	uint32_t raw;
	struct {
		uint32_t BLC_CHECKSUM                    : 32;
	} bits;
};

union REG_ISP_BLC_8 {
	uint32_t raw;
	struct {
		uint32_t BLC_INT                         : 1;
	} bits;
};

union REG_ISP_BLC_IMG_BAYERID {
	uint32_t raw;
	struct {
		uint32_t IMG_BAYERID                     : 4;
		uint32_t _rsv_4                          : 12;
		uint32_t FORCE_BAYER_ENABLE              : 1;
	} bits;
};

union REG_ISP_BLC_DUMMY {
	uint32_t raw;
	struct {
		uint32_t BLC_DUMMY                       : 16;
	} bits;
};

union REG_ISP_BLC_LOCATION {
	uint32_t raw;
	struct {
		uint32_t BLC_LOCATION                    : 32;
	} bits;
};

union REG_ISP_BLC_9 {
	uint32_t raw;
	struct {
		uint32_t BLC_2NDOFFSET_R                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t BLC_2NDOFFSET_GR                : 12;
	} bits;
};

union REG_ISP_BLC_A {
	uint32_t raw;
	struct {
		uint32_t BLC_2NDOFFSET_GB                : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t BLC_2NDOFFSET_B                 : 12;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_LMAP_LMP_0 {
	uint32_t raw;
	struct {
		uint32_t LMAP_ENABLE                     : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t LMAP_Y_MODE                     : 2;
		uint32_t LMAP_THD_L                      : 12;
		uint32_t LMAP_THD_H                      : 12;
		uint32_t LMAP_SOFTRST                    : 1;
		uint32_t FORCE_DMA_DISABLE               : 1;
	} bits;
};

union REG_ISP_LMAP_LMP_1 {
	uint32_t raw;
	struct {
		uint32_t LMAP_CROP_WIDTHM1               : 13;
		uint32_t LMAP_CROP_HEIGHTM1              : 13;
		uint32_t _rsv_26                         : 1;
		uint32_t LMAP_BAYER_ID                   : 4;
		uint32_t LMAP_SHDW_SEL                   : 1;
	} bits;
};

union REG_ISP_LMAP_LMP_2 {
	uint32_t raw;
	struct {
		uint32_t LMAP_W_GRID_NUM                 : 10;
		uint32_t _rsv_10                         : 2;
		uint32_t LMAP_W_BIT                      : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t LMAP_H_GRID_NUM                 : 10;
		uint32_t _rsv_26                         : 2;
		uint32_t LMAP_H_BIT                      : 3;
		uint32_t LMAP_OUT_SEL                    : 1;
	} bits;
};

union REG_ISP_LMAP_LMP_DEBUG_0 {
	uint32_t raw;
	struct {
		uint32_t LMAP_DEBUG_0                    : 32;
	} bits;
};

union REG_ISP_LMAP_LMP_DEBUG_1 {
	uint32_t raw;
	struct {
		uint32_t LMAP_DEBUG_1                    : 32;
	} bits;
};

union REG_ISP_LMAP_DUMMY {
	uint32_t raw;
	struct {
		uint32_t LMAP_DUMMY                      : 32;
	} bits;
};

union REG_ISP_LMAP_LMP_DEBUG_2 {
	uint32_t raw;
	struct {
		uint32_t LMAP_DEBUG_2                    : 32;
	} bits;
};

union REG_ISP_LMAP_LMP_3 {
	uint32_t raw;
	struct {
		uint32_t DEBUG_DATA_SEL                  : 2;
	} bits;
};

union REG_ISP_LMAP_LMP_4 {
	uint32_t raw;
	struct {
		uint32_t FORCE_CLK_ENABLE                : 1;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_RGBMAP_0 {
	uint32_t raw;
	struct {
		uint32_t RGBMAP_ENABLE                   : 1;
		uint32_t RGBMAP_W_BIT                    : 3;
		uint32_t RGBMAP_H_BIT                    : 3;
		uint32_t IMG_BAYERID                     : 4;
		uint32_t RGBMAP_W_GRID_NUM               : 10;
		uint32_t RGBMAP_H_GRID_NUM               : 10;
		uint32_t RGBMAP_SOFTRST                  : 1;
	} bits;
};

union REG_ISP_RGBMAP_1 {
	uint32_t raw;
	struct {
		uint32_t IMG_WIDTHM1                     : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t IMG_HEIGHTM1                    : 13;
		uint32_t RGBMAP_SHDW_SEL                 : 1;
	} bits;
};

union REG_ISP_RGBMAP_DEBUG_0 {
	uint32_t raw;
	struct {
		uint32_t RGBMAP_DEBUG_0                  : 32;
	} bits;
};

union REG_ISP_RGBMAP_DEBUG_1 {
	uint32_t raw;
	struct {
		uint32_t RGBMAP_DEBUG_1                  : 32;
	} bits;
};

union REG_ISP_RGBMAP_DUMMY {
	uint32_t raw;
	struct {
		uint32_t RGBMAP_DUMMY                    : 32;
	} bits;
};

union REG_ISP_RGBMAP_2 {
	uint32_t raw;
	struct {
		uint32_t FORCE_DMA_DISABLE               : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t DEBUG_DATA_SEL                  : 2;
	} bits;
};

union REG_ISP_RGBMAP_DEBUG_2 {
	uint32_t raw;
	struct {
		uint32_t RGBMAP_DEBUG_2                  : 32;
	} bits;
};

union REG_ISP_RGBMAP_3 {
	uint32_t raw;
	struct {
		uint32_t FORCE_CLK_ENABLE                : 1;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_PCHK_SHADOW_RD_SEL {
	uint32_t raw;
	struct {
		uint32_t SHADOW_RD_SEL                   : 1;
	} bits;
};

union REG_ISP_PCHK_IN_SEL        {
	uint32_t raw;
	struct {
		uint32_t PCHK_IN_SEL                     : 6;
	} bits;
};

union REG_ISP_PCHK_RULE_EN       {
	uint32_t raw;
	struct {
		uint32_t PCHK_RULE_EN                    : 12;
	} bits;
};

union REG_ISP_PCHK_HSIZE         {
	uint32_t raw;
	struct {
		uint32_t PCHK_HSIZE                      : 16;
	} bits;
};

union REG_ISP_PCHK_VSIZE         {
	uint32_t raw;
	struct {
		uint32_t PCHK_VSIZE                      : 16;
	} bits;
};

union REG_ISP_PCHK_NRDY_LIMIT    {
	uint32_t raw;
	struct {
		uint32_t PCHK_NRDY_LIMIT                 : 24;
	} bits;
};

union REG_ISP_PCHK_NREQ_LIMIT    {
	uint32_t raw;
	struct {
		uint32_t PCHK_NREQ_LIMIT                 : 24;
	} bits;
};

union REG_ISP_PCHK_PFREQ_LIMIT   {
	uint32_t raw;
	struct {
		uint32_t PCHK_PFREQ_LIMIT                : 10;
	} bits;
};

union REG_ISP_PCHK_ERR_BUS       {
	uint32_t raw;
	struct {
		uint32_t PCHK_ERR_BUS                    : 12;
	} bits;
};

union REG_ISP_PCHK_ERR_XY        {
	uint32_t raw;
	struct {
		uint32_t X                               : 16;
		uint32_t Y                               : 16;
	} bits;
};

union REG_ISP_PCHK_ERR_CLR       {
	uint32_t raw;
	struct {
		uint32_t CLEAR                           : 16;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_LSCR_ENABLE {
	uint32_t raw;
	struct {
		uint32_t LSCR_ENABLE                     : 1;
	} bits;
};

union REG_ISP_LSCR_DELAY {
	uint32_t raw;
	struct {
		uint32_t LSCR_DELAY                      : 1;
	} bits;
};

union REG_ISP_LSCR_OUT_SEL {
	uint32_t raw;
	struct {
		uint32_t LSCR_OUT_SEL                    : 2;
	} bits;
};

union REG_ISP_LSCR_SHDW_READ_SEL {
	uint32_t raw;
	struct {
		uint32_t SHDW_READ_SEL                   : 1;
	} bits;
};

union REG_ISP_LSCR_CENTERX {
	uint32_t raw;
	struct {
		uint32_t LSCR_CENTERX                    : 13;
	} bits;
};

union REG_ISP_LSCR_CENTERY {
	uint32_t raw;
	struct {
		uint32_t LSCR_CENTERY                    : 13;
	} bits;
};

union REG_ISP_LSCR_NORM {
	uint32_t raw;
	struct {
		uint32_t LSCR_NORM                       : 15;
	} bits;
};

union REG_ISP_LSCR_STRNTH {
	uint32_t raw;
	struct {
		uint32_t LSCR_STRNTH                     : 12;
	} bits;
};

union REG_ISP_LSCR_NORM_IR {
	uint32_t raw;
	struct {
		uint32_t LSCR_NORM_IR                    : 15;
	} bits;
};

union REG_ISP_LSCR_STRNTH_IR {
	uint32_t raw;
	struct {
		uint32_t LSCR_STRNTH_IR                  : 12;
	} bits;
};

union REG_ISP_LSCR_DEBUG {
	uint32_t raw;
	struct {
		uint32_t LSCR_DEBUG                      : 32;
	} bits;
};

union REG_ISP_LSCR_DUMMY {
	uint32_t raw;
	struct {
		uint32_t LSCR_DUMMY                      : 32;
	} bits;
};

union REG_ISP_LSCR_GAIN_LUT {
	uint32_t raw;
	struct {
		uint32_t LSCR_GAIN_LUT                   : 12;
	} bits;
};

union REG_ISP_LSCR_GAIN_LUT_IR {
	uint32_t raw;
	struct {
		uint32_t LSCR_GAIN_LUT_IR                : 12;
	} bits;
};

union REG_ISP_LSCR_INDEX_CLR {
	uint32_t raw;
	struct {
		uint32_t LSCR_INDEX_CLR                  : 1;
	} bits;
};

union REG_ISP_LSCR_FORCE_CLK_EN {
	uint32_t raw;
	struct {
		uint32_t FORCE_CLK_ENABLE                : 1;
	} bits;
};

union REG_ISP_LSCR_DEBUG_2 {
	uint32_t raw;
	struct {
		uint32_t LSCR_DEBUG_2                    : 32;
	} bits;
};

union REG_ISP_LSCR_DEBUG_3 {
	uint32_t raw;
	struct {
		uint32_t LSCR_DEBUG_3                    : 32;
	} bits;
};

union REG_ISP_LSCR_DEBUG_4 {
	uint32_t raw;
	struct {
		uint32_t LSCR_DEBUG_4                    : 32;
	} bits;
};

union REG_ISP_LSCR_GAIN_LUT_G {
	uint32_t raw;
	struct {
		uint32_t LSCR_GAIN_LUT_G                 : 12;
	} bits;
};

union REG_ISP_LSCR_GAIN_LUT_B {
	uint32_t raw;
	struct {
		uint32_t LSCR_GAIN_LUT_B                 : 12;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_WBG_0 {
	uint32_t raw;
	struct {
		uint32_t WBG_BYPASS                      : 1;
	} bits;
};

union REG_ISP_WBG_1 {
	uint32_t raw;
	struct {
		uint32_t SHDW_READ_SEL                   : 1;
	} bits;
};

union REG_ISP_WBG_2 {
	uint32_t raw;
	struct {
		uint32_t WBG_ENABLE                      : 1;
		uint32_t _rsv_1                          : 27;
		uint32_t FORCE_CLK_ENABLE                : 1;
	} bits;
};

union REG_ISP_WBG_4 {
	uint32_t raw;
	struct {
		uint32_t WBG_RGAIN                       : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t WBG_GGAIN                       : 14;
	} bits;
};

union REG_ISP_WBG_5 {
	uint32_t raw;
	struct {
		uint32_t WBG_BGAIN                       : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t WBG_IRGAIN                      : 14;
	} bits;
};

union REG_ISP_WBG_6 {
	uint32_t raw;
	struct {
		uint32_t WBG_CHECKSUM                    : 32;
	} bits;
};

union REG_ISP_WBG_7 {
	uint32_t raw;
	struct {
		uint32_t WBG_INT                         : 1;
	} bits;
};

union REG_ISP_WBG_IMG_BAYERID {
	uint32_t raw;
	struct {
		uint32_t IMG_BAYERID                     : 4;
		uint32_t _rsv_4                          : 12;
		uint32_t FORCE_BAYER_ENABLE              : 1;
	} bits;
};

union REG_ISP_WBG_DUMMY {
	uint32_t raw;
	struct {
		uint32_t WBG_DUMMY                       : 32;
	} bits;
};

union REG_ISP_WBG_LOCATION {
	uint32_t raw;
	struct {
		uint32_t WBG_LOCATION                    : 32;
	} bits;
};

union REG_ISP_WBG_34 {
	uint32_t raw;
	struct {
		uint32_t RGAIN_FRACTION                  : 24;
	} bits;
};

union REG_ISP_WBG_38 {
	uint32_t raw;
	struct {
		uint32_t GGAIN_FRACTION                  : 24;
	} bits;
};

union REG_ISP_WBG_3C {
	uint32_t raw;
	struct {
		uint32_t BGAIN_FRACTION                  : 24;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_DPC_0 {
	uint32_t raw;
	struct {
		uint32_t PROG_HDK_DIS                    : 1;
		uint32_t CONT_EN                         : 1;
		uint32_t SOFTRST                         : 1;
		uint32_t DBG_EN                          : 1;
		uint32_t CH_NM                           : 1;
	} bits;
};

union REG_ISP_DPC_1 {
	uint32_t raw;
	struct {
		uint32_t SHDW_READ_SEL                   : 1;
	} bits;
};

union REG_ISP_DPC_2 {
	uint32_t raw;
	struct {
		uint32_t DPC_ENABLE                      : 1;
		uint32_t GE_ENABLE                       : 1;
		uint32_t DPC_DYNAMICBPC_ENABLE           : 1;
		uint32_t DPC_STATICBPC_ENABLE            : 1;
		uint32_t DELAY                           : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t FORCE_CLK_ENABLE                : 1;
		uint32_t _rsv_9                          : 7;
		uint32_t DPC_CLUSTER_SIZE                : 2;
	} bits;
};

union REG_ISP_DPC_3 {
	uint32_t raw;
	struct {
		uint32_t DPC_R_BRIGHT_PIXEL_RATIO        : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t DPC_G_BRIGHT_PIXEL_RATIO        : 10;
	} bits;
};

union REG_ISP_DPC_4 {
	uint32_t raw;
	struct {
		uint32_t DPC_B_BRIGHT_PIXEL_RATIO        : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t DPC_R_DARK_PIXEL_RATIO          : 10;
	} bits;
};

union REG_ISP_DPC_5 {
	uint32_t raw;
	struct {
		uint32_t DPC_G_DARK_PIXEL_RATIO          : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t DPC_B_DARK_PIXEL_RATIO          : 10;
	} bits;
};

union REG_ISP_DPC_6 {
	uint32_t raw;
	struct {
		uint32_t DPC_R_DARK_PIXEL_MINDIFF        : 8;
		uint32_t DPC_G_DARK_PIXEL_MINDIFF        : 8;
		uint32_t DPC_B_DARK_PIXEL_MINDIFF        : 8;
	} bits;
};

union REG_ISP_DPC_7 {
	uint32_t raw;
	struct {
		uint32_t DPC_R_BRIGHT_PIXEL_UPBOUD_RATIO : 8;
		uint32_t DPC_G_BRIGHT_PIXEL_UPBOUD_RATIO : 8;
		uint32_t DPC_B_BRIGHT_PIXEL_UPBOUD_RATIO : 8;
	} bits;
};

union REG_ISP_DPC_8 {
	uint32_t raw;
	struct {
		uint32_t DPC_FLAT_THRE_MIN_RB            : 8;
		uint32_t DPC_FLAT_THRE_MIN_G             : 8;
	} bits;
};

union REG_ISP_DPC_9 {
	uint32_t raw;
	struct {
		uint32_t DPC_FLAT_THRE_R                 : 8;
		uint32_t DPC_FLAT_THRE_G                 : 8;
		uint32_t DPC_FLAT_THRE_B                 : 8;
	} bits;
};

union REG_ISP_DPC_10 {
	uint32_t raw;
	struct {
		uint32_t GE_STRENGTH                     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t GE_COMBINEWEIGHT                : 4;
	} bits;
};

union REG_ISP_DPC_11 {
	uint32_t raw;
	struct {
		uint32_t GE_THRE1                        : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t GE_THRE2                        : 12;
	} bits;
};

union REG_ISP_DPC_12 {
	uint32_t raw;
	struct {
		uint32_t GE_THRE3                        : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t GE_THRE4                        : 12;
	} bits;
};

union REG_ISP_DPC_13 {
	uint32_t raw;
	struct {
		uint32_t GE_THRE11                       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t GE_THRE21                       : 12;
	} bits;
};

union REG_ISP_DPC_14 {
	uint32_t raw;
	struct {
		uint32_t GE_THRE31                       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t GE_THRE41                       : 12;
	} bits;
};

union REG_ISP_DPC_15 {
	uint32_t raw;
	struct {
		uint32_t GE_THRE12                       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t GE_THRE22                       : 12;
	} bits;
};

union REG_ISP_DPC_16 {
	uint32_t raw;
	struct {
		uint32_t GE_THRE32                       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t GE_THRE42                       : 12;
	} bits;
};

union REG_ISP_DPC_17 {
	uint32_t raw;
	struct {
		uint32_t DPC_MEM0_IMG0_ADDR              : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t DPC_MEM0_IMG1_ADDR              : 11;
		uint32_t _rsv_27                         : 3;
		uint32_t DPC_MEM0_IMG_ADDR_SEL           : 1;
		uint32_t DPC_MEM_PROG_MODE               : 1;
	} bits;
};

union REG_ISP_DPC_18 {
	uint32_t raw;
	struct {
		uint32_t DPC_SW_RADDR                    : 12;
	} bits;
};

union REG_ISP_DPC_19 {
	uint32_t raw;
	struct {
		uint32_t DPC_RDATA_R                     : 24;
		uint32_t _rsv_24                         : 7;
		uint32_t DPC_SW_R                        : 1;
	} bits;
};

union REG_ISP_DPC_MEM_W0 {
	uint32_t raw;
	struct {
		uint32_t DPC_BP_MEM_D                    : 24;
		uint32_t _rsv_24                         : 7;
		uint32_t DPC_BP_MEM_W                    : 1;
	} bits;
};

union REG_ISP_DPC_WINDOW {
	uint32_t raw;
	struct {
		uint32_t IMG_WD                          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t IMG_HT                          : 12;
	} bits;
};

union REG_ISP_DPC_MEM_ST_ADDR {
	uint32_t raw;
	struct {
		uint32_t DPC_BP_MEM_ST_ADDR              : 11;
		uint32_t _rsv_11                         : 20;
		uint32_t DPC_BP_MEM_ST_ADDR_W            : 1;
	} bits;
};

union REG_ISP_DPC_CHECKSUM {
	uint32_t raw;
	struct {
		uint32_t DPC_CHECKSUM                    : 32;
	} bits;
};

union REG_ISP_DPC_INT {
	uint32_t raw;
	struct {
		uint32_t DPC_INT                         : 1;
	} bits;
};

union REG_ISP_DPC_20 {
	uint32_t raw;
	struct {
		uint32_t PROB_OUT_SEL                    : 4;
		uint32_t PROB_PERFMT                     : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t PROB_FMT                        : 6;
	} bits;
};

union REG_ISP_DPC_21 {
	uint32_t raw;
	struct {
		uint32_t PROB_LINE                       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t PROB_PIX                        : 12;
	} bits;
};

union REG_ISP_DPC_22 {
	uint32_t raw;
	struct {
		uint32_t DPC_DBG0                        : 32;
	} bits;
};

union REG_ISP_DPC_23 {
	uint32_t raw;
	struct {
		uint32_t DPC_DBG1                        : 32;
	} bits;
};

union REG_ISP_DPC_24 {
	uint32_t raw;
	struct {
		uint32_t DPC_IR_BRIGHT_PIXEL_RATIO       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t DPC_IR_DARK_PIXEL_RATIO         : 10;
	} bits;
};

union REG_ISP_DPC_25 {
	uint32_t raw;
	struct {
		uint32_t DPC_IR_DARK_PIXEL_MINDIFF       : 8;
		uint32_t DPC_IR_BRIGHT_PIXEL_UPBOUD_RATIO: 8;
		uint32_t DPC_FLAT_THRE_MIN_IR            : 8;
		uint32_t DPC_FLAT_THRE_IR                : 8;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_RAW_TOP_RAW_0 {
	uint32_t raw;
	struct {
		uint32_t SVN_VERSION                     : 32;
	} bits;
};

union REG_RAW_TOP_READ_SEL {
	uint32_t raw;
	struct {
		uint32_t SHDW_READ_SEL                   : 1;
	} bits;
};

union REG_RAW_TOP_RAW_1 {
	uint32_t raw;
	struct {
		uint32_t TIMESTAMP                       : 32;
	} bits;
};

union REG_RAW_TOP_CTRL {
	uint32_t raw;
	struct {
		uint32_t LS_CROP_DST_SEL                 : 1;
	} bits;
};

union REG_RAW_TOP_UP_PQ_EN {
	uint32_t raw;
	struct {
		uint32_t UP_PQ_EN                        : 1;
	} bits;
};

union REG_RAW_TOP_RAW_2 {
	uint32_t raw;
	struct {
		uint32_t IMG_WIDTHM1                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_HEIGHTM1                    : 14;
	} bits;
};

union REG_RAW_TOP_DUMMY {
	uint32_t raw;
	struct {
		uint32_t RAW_TOP_DUMMY                   : 32;
	} bits;
};

union REG_RAW_TOP_RAW_4 {
	uint32_t raw;
	struct {
		uint32_t YUV_IN_MODE                     : 1;
	} bits;
};

union REG_RAW_TOP_STATUS {
	uint32_t raw;
	struct {
		uint32_t RAW_TOP_STATUS                  : 32;
	} bits;
};

union REG_RAW_TOP_DEBUG {
	uint32_t raw;
	struct {
		uint32_t RAW_TOP_DEBUG                   : 32;
	} bits;
};

union REG_RAW_TOP_DEBUG_SELECT {
	uint32_t raw;
	struct {
		uint32_t RAW_TOP_DEBUG_SELECT            : 32;
	} bits;
};

union REG_RAW_TOP_RAW_BAYER_TYPE_TOPLEFT {
	uint32_t raw;
	struct {
		uint32_t BAYER_TYPE_TOPLEFT              : 4;
		uint32_t _rsv_4                          : 12;
		uint32_t RGBIR_ENABLE                    : 1;
	} bits;
};

union REG_RAW_TOP_RDMI_ENBALE {
	uint32_t raw;
	struct {
		uint32_t RDMI_EN                         : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t PASS_SEL                        : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t PASS_CNT_M1                     : 8;
		uint32_t CH_NUM                          : 1;
	} bits;
};

union REG_RAW_TOP_RDMA_SIZE {
	uint32_t raw;
	struct {
		uint32_t RDMI_WIDTHM1                    : 16;
		uint32_t RDMI_HEIGHTM1                   : 16;
	} bits;
};

union REG_RAW_TOP_DPCM_MODE {
	uint32_t raw;
	struct {
		uint32_t DPCM_MODE                       : 3;
		uint32_t _rsv_3                          : 5;
		uint32_t DPCM_XSTR                       : 13;
	} bits;
};

union REG_RAW_TOP_STVALID_STATUS {
	uint32_t raw;
	struct {
		uint32_t STVALID_STATUS                  : 11;
	} bits;
};

union REG_RAW_TOP_STREADY_STATUS {
	uint32_t raw;
	struct {
		uint32_t STREADY_STATUS                  : 11;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_CFA_0 {
	uint32_t raw;
	struct {
		uint32_t CFA_SHDW_SEL                    : 1;
		uint32_t CFA_ENABLE                      : 1;
		uint32_t CFA_FCR_ENABLE                  : 1;
		uint32_t CFA_MOIRE_ENABLE                : 1;
		uint32_t DELAY                           : 1;
		uint32_t FORCE_CLK_ENABLE                : 1;
	} bits;
};

union REG_ISP_CFA_1 {
	uint32_t raw;
	struct {
		uint32_t CFA_OUT_SEL                     : 1;
		uint32_t CONT_EN                         : 1;
		uint32_t _rsv_2                          : 1;
		uint32_t SOFTRST                         : 1;
		uint32_t _rsv_4                          : 1;
		uint32_t DBG_EN                          : 1;
	} bits;
};

union REG_ISP_CFA_2 {
	uint32_t raw;
	struct {
		uint32_t IMG_WD                          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t IMG_HT                          : 12;
	} bits;
};

union REG_ISP_CFA_3 {
	uint32_t raw;
	struct {
		uint32_t CFA_EDGEE_THD                   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t CFA_SIGE_THD                    : 12;
	} bits;
};

union REG_ISP_CFA_4 {
	uint32_t raw;
	struct {
		uint32_t CFA_GSIG_TOL                    : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t CFA_RBSIG_TOL                   : 12;
	} bits;
};

union REG_ISP_CFA_4_1 {
	uint32_t raw;
	struct {
		uint32_t CFA_EDGE_TOL                    : 12;
	} bits;
};

union REG_ISP_CFA_5 {
	uint32_t raw;
	struct {
		uint32_t CFA_GHP_THD                     : 16;
	} bits;
};

union REG_ISP_CFA_6 {
	uint32_t raw;
	struct {
		uint32_t CFA_FCR_HV_LOWERBND             : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t CFA_FCR_HV_UPPERBND             : 12;
	} bits;
};

union REG_ISP_CFA_7 {
	uint32_t raw;
	struct {
		uint32_t CFA_FCR_SAT_LOWERBND            : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t CFA_FCR_SAT_UPPERBND            : 12;
	} bits;
};

union REG_ISP_CFA_8 {
	uint32_t raw;
	struct {
		uint32_t CFA_FCR_W3_LOWERBND             : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t CFA_FCR_W3_UPPERBND             : 12;
	} bits;
};

union REG_ISP_CFA_9 {
	uint32_t raw;
	struct {
		uint32_t CFA_FCR_W3MAX_WEIGHT            : 5;
		uint32_t _rsv_5                          : 11;
		uint32_t CFA_FCR_STRENGTH                : 10;
	} bits;
};

union REG_ISP_CFA_GHP_LUT_0 {
	uint32_t raw;
	struct {
		uint32_t CFA_GHP_LUT00                   : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t CFA_GHP_LUT01                   : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t CFA_GHP_LUT02                   : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t CFA_GHP_LUT03                   : 6;
	} bits;
};

union REG_ISP_CFA_GHP_LUT_1 {
	uint32_t raw;
	struct {
		uint32_t CFA_GHP_LUT04                   : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t CFA_GHP_LUT05                   : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t CFA_GHP_LUT06                   : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t CFA_GHP_LUT07                   : 6;
	} bits;
};

union REG_ISP_CFA_GHP_LUT_2 {
	uint32_t raw;
	struct {
		uint32_t CFA_GHP_LUT08                   : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t CFA_GHP_LUT09                   : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t CFA_GHP_LUT10                   : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t CFA_GHP_LUT11                   : 6;
	} bits;
};

union REG_ISP_CFA_GHP_LUT_3 {
	uint32_t raw;
	struct {
		uint32_t CFA_GHP_LUT12                   : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t CFA_GHP_LUT13                   : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t CFA_GHP_LUT14                   : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t CFA_GHP_LUT15                   : 6;
	} bits;
};

union REG_ISP_CFA_GHP_LUT_4 {
	uint32_t raw;
	struct {
		uint32_t CFA_GHP_LUT16                   : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t CFA_GHP_LUT17                   : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t CFA_GHP_LUT18                   : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t CFA_GHP_LUT19                   : 6;
	} bits;
};

union REG_ISP_CFA_GHP_LUT_5 {
	uint32_t raw;
	struct {
		uint32_t CFA_GHP_LUT20                   : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t CFA_GHP_LUT21                   : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t CFA_GHP_LUT22                   : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t CFA_GHP_LUT23                   : 6;
	} bits;
};

union REG_ISP_CFA_GHP_LUT_6 {
	uint32_t raw;
	struct {
		uint32_t CFA_GHP_LUT24                   : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t CFA_GHP_LUT25                   : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t CFA_GHP_LUT26                   : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t CFA_GHP_LUT27                   : 6;
	} bits;
};

union REG_ISP_CFA_GHP_LUT_7 {
	uint32_t raw;
	struct {
		uint32_t CFA_GHP_LUT28                   : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t CFA_GHP_LUT29                   : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t CFA_GHP_LUT30                   : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t CFA_GHP_LUT31                   : 6;
	} bits;
};

union REG_ISP_CFA_10 {
	uint32_t raw;
	struct {
		uint32_t CFA_MOIRE_STRTH                 : 8;
		uint32_t CFA_MOIRE_WGHT_GAIN             : 8;
		uint32_t CFA_MOIRE_NP_YSLOPE             : 8;
	} bits;
};

union REG_ISP_CFA_11 {
	uint32_t raw;
	struct {
		uint32_t CFA_MOIRE_NP_YMIN               : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t CFA_MOIRE_NP_LOW                : 8;
		uint32_t CFA_MOIRE_NP_HIGH               : 8;
	} bits;
};

union REG_ISP_CFA_12 {
	uint32_t raw;
	struct {
		uint32_t CFA_MOIRE_DIFFTHD_MIN           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t CFA_MOIRE_DIFFTHD_SLOPE         : 8;
	} bits;
};

union REG_ISP_CFA_13 {
	uint32_t raw;
	struct {
		uint32_t CFA_MOIRE_DIFFW_LOW             : 8;
		uint32_t CFA_MOIRE_DIFFW_HIGH            : 8;
		uint32_t CFA_MOIRE_SADTHD_MIN            : 12;
	} bits;
};

union REG_ISP_CFA_14 {
	uint32_t raw;
	struct {
		uint32_t CFA_MOIRE_SADTHD_SLOPE          : 8;
		uint32_t CFA_MOIRE_SADW_LOW              : 8;
		uint32_t CFA_MOIRE_SADW_HIGH             : 8;
	} bits;
};

union REG_ISP_CFA_15 {
	uint32_t raw;
	struct {
		uint32_t CFA_MOIRE_LUMAW_LOW             : 8;
		uint32_t CFA_MOIRE_LUMAW_HIGH            : 8;
	} bits;
};

union REG_ISP_CFA_16 {
	uint32_t raw;
	struct {
		uint32_t CFA_MOIRE_LUMATHD_MIN           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t CFA_MOIRE_LUMATHD_SLOPE         : 8;
	} bits;
};

union REG_ISP_CFA_17 {
	uint32_t raw;
	struct {
		uint32_t DIR_READCNT_FROM_LINE0          : 5;
	} bits;
};

union REG_ISP_CFA_18 {
	uint32_t raw;
	struct {
		uint32_t PROB_OUT_SEL                    : 4;
		uint32_t PROB_PERFMT                     : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t PROB_FMT                        : 6;
	} bits;
};

union REG_ISP_CFA_19 {
	uint32_t raw;
	struct {
		uint32_t PROB_LINE                       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t PROB_PIX                        : 12;
	} bits;
};

union REG_ISP_CFA_20 {
	uint32_t raw;
	struct {
		uint32_t CFA_DBG0                        : 32;
	} bits;
};

union REG_ISP_CFA_21 {
	uint32_t raw;
	struct {
		uint32_t CFA_DBG1                        : 32;
	} bits;
};

union REG_ISP_CFA_22 {
	uint32_t raw;
	struct {
		uint32_t CFA_FILTER_MODE                 : 2;
	} bits;
};

union REG_ISP_CFA_23 {
	uint32_t raw;
	struct {
		uint32_t CFA_NORM_R0                     : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CFA_NORM_G0                     : 13;
	} bits;
};

union REG_ISP_CFA_24 {
	uint32_t raw;
	struct {
		uint32_t CFA_NORM_B0                     : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CFA_NORM_R1                     : 13;
	} bits;
};

union REG_ISP_CFA_25 {
	uint32_t raw;
	struct {
		uint32_t CFA_NORM_G1                     : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CFA_NORM_B1                     : 13;
	} bits;
};

union REG_ISP_CFA_26 {
	uint32_t raw;
	struct {
		uint32_t CFA_NORM_R2                     : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CFA_NORM_G2                     : 13;
	} bits;
};

union REG_ISP_CFA_27 {
	uint32_t raw;
	struct {
		uint32_t CFA_NORM_B2                     : 13;
	} bits;
};

union REG_ISP_CFA_28 {
	uint32_t raw;
	struct {
		uint32_t RGBCAC_ENABLE                   : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t RGBCAC_OUT_SEL                  : 2;
	} bits;
};

union REG_ISP_CFA_29 {
	uint32_t raw;
	struct {
		uint32_t RGBCAC_VAR_TH                   : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t RGBCAC_PURPLE_TH                : 8;
		uint32_t RGBCAC_CORRECT_STRENGTH         : 8;
	} bits;
};

union REG_ISP_CFA_30 {
	uint32_t raw;
	struct {
		uint32_t RGBCAC_PURPLE_CB                : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t RGBCAC_PURPLE_CR                : 12;
	} bits;
};

union REG_ISP_CFA_31 {
	uint32_t raw;
	struct {
		uint32_t RGBCAC_PURPLE_CB2               : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t RGBCAC_PURPLE_CR2               : 12;
	} bits;
};

union REG_ISP_CFA_32 {
	uint32_t raw;
	struct {
		uint32_t RGBCAC_PURPLE_CB3               : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t RGBCAC_PURPLE_CR3               : 12;
	} bits;
};

union REG_ISP_CFA_33 {
	uint32_t raw;
	struct {
		uint32_t RGBCAC_GREEN_CB                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t RGBCAC_GREEN_CR                 : 12;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_BNR_SHADOW_RD_SEL {
	uint32_t raw;
	struct {
		uint32_t SHADOW_RD_SEL                   : 1;
	} bits;
};

union REG_ISP_BNR_OUT_SEL {
	uint32_t raw;
	struct {
		uint32_t BNR_OUT_SEL                     : 4;
	} bits;
};

union REG_ISP_BNR_INDEX_CLR {
	uint32_t raw;
	struct {
		uint32_t BNR_INDEX_CLR                   : 1;
	} bits;
};

union REG_ISP_BNR_NS_LUMA_TH_R    {
	uint32_t raw;
	struct {
		uint32_t BNR_NS_LUMA_TH_R                : 10;
	} bits;
};

union REG_ISP_BNR_NS_SLOPE_R      {
	uint32_t raw;
	struct {
		uint32_t BNR_NS_SLOPE_R                  : 10;
	} bits;
};

union REG_ISP_BNR_NS_OFFSET0_R    {
	uint32_t raw;
	struct {
		uint32_t BNR_NS_OFFSET0_R                : 10;
	} bits;
};

union REG_ISP_BNR_NS_OFFSET1_R    {
	uint32_t raw;
	struct {
		uint32_t BNR_NS_OFFSET1_R                : 10;
	} bits;
};

union REG_ISP_BNR_NS_LUMA_TH_GR   {
	uint32_t raw;
	struct {
		uint32_t BNR_NS_LUMA_TH_GR               : 10;
	} bits;
};

union REG_ISP_BNR_NS_SLOPE_GR     {
	uint32_t raw;
	struct {
		uint32_t BNR_NS_SLOPE_GR                 : 10;
	} bits;
};

union REG_ISP_BNR_NS_OFFSET0_GR   {
	uint32_t raw;
	struct {
		uint32_t BNR_NS_OFFSET0_GR               : 10;
	} bits;
};

union REG_ISP_BNR_NS_OFFSET1_GR   {
	uint32_t raw;
	struct {
		uint32_t BNR_NS_OFFSET1_GR               : 10;
	} bits;
};

union REG_ISP_BNR_NS_LUMA_TH_GB   {
	uint32_t raw;
	struct {
		uint32_t BNR_NS_LUMA_TH_GB               : 10;
	} bits;
};

union REG_ISP_BNR_NS_SLOPE_GB     {
	uint32_t raw;
	struct {
		uint32_t BNR_NS_SLOPE_GB                 : 10;
	} bits;
};

union REG_ISP_BNR_NS_OFFSET0_GB   {
	uint32_t raw;
	struct {
		uint32_t BNR_NS_OFFSET0_GB               : 10;
	} bits;
};

union REG_ISP_BNR_NS_OFFSET1_GB   {
	uint32_t raw;
	struct {
		uint32_t BNR_NS_OFFSET1_GB               : 10;
	} bits;
};

union REG_ISP_BNR_NS_LUMA_TH_B    {
	uint32_t raw;
	struct {
		uint32_t BNR_NS_LUMA_TH_B                : 10;
	} bits;
};

union REG_ISP_BNR_NS_SLOPE_B      {
	uint32_t raw;
	struct {
		uint32_t BNR_NS_SLOPE_B                  : 10;
	} bits;
};

union REG_ISP_BNR_NS_OFFSET0_B    {
	uint32_t raw;
	struct {
		uint32_t BNR_NS_OFFSET0_B                : 10;
	} bits;
};

union REG_ISP_BNR_NS_OFFSET1_B    {
	uint32_t raw;
	struct {
		uint32_t BNR_NS_OFFSET1_B                : 10;
	} bits;
};

union REG_ISP_BNR_NS_GAIN         {
	uint32_t raw;
	struct {
		uint32_t BNR_NS_GAIN                     : 8;
	} bits;
};

union REG_ISP_BNR_STRENGTH_MODE   {
	uint32_t raw;
	struct {
		uint32_t BNR_STRENGTH_MODE               : 8;
	} bits;
};

union REG_ISP_BNR_INTENSITY_SEL   {
	uint32_t raw;
	struct {
		uint32_t BNR_INTENSITY_SEL               : 5;
	} bits;
};

union REG_ISP_BNR_WEIGHT_INTRA_0  {
	uint32_t raw;
	struct {
		uint32_t BNR_WEIGHT_INTRA_0              : 3;
	} bits;
};

union REG_ISP_BNR_WEIGHT_INTRA_1  {
	uint32_t raw;
	struct {
		uint32_t BNR_WEIGHT_INTRA_1              : 3;
	} bits;
};

union REG_ISP_BNR_WEIGHT_INTRA_2  {
	uint32_t raw;
	struct {
		uint32_t BNR_WEIGHT_INTRA_2              : 3;
	} bits;
};

union REG_ISP_BNR_WEIGHT_NORM_1   {
	uint32_t raw;
	struct {
		uint32_t BNR_WEIGHT_NORM_1               : 8;
	} bits;
};

union REG_ISP_BNR_WEIGHT_NORM_2   {
	uint32_t raw;
	struct {
		uint32_t BNR_WEIGHT_NORM_2               : 7;
	} bits;
};

union REG_ISP_BNR_LSC_RATIO       {
	uint32_t raw;
	struct {
		uint32_t BNR_LSC_RATIO                   : 5;
	} bits;
};

union REG_ISP_BNR_VAR_TH          {
	uint32_t raw;
	struct {
		uint32_t BNR_VAR_TH                      : 8;
	} bits;
};

union REG_ISP_BNR_WEIGHT_LUT      {
	uint32_t raw;
	struct {
		uint32_t BNR_WEIGHT_LUT                  : 5;
	} bits;
};

union REG_ISP_BNR_WEIGHT_SM       {
	uint32_t raw;
	struct {
		uint32_t BNR_WEIGHT_SM                   : 5;
	} bits;
};

union REG_ISP_BNR_WEIGHT_V        {
	uint32_t raw;
	struct {
		uint32_t BNR_WEIGHT_V                    : 5;
	} bits;
};

union REG_ISP_BNR_WEIGHT_H        {
	uint32_t raw;
	struct {
		uint32_t BNR_WEIGHT_H                    : 5;
	} bits;
};

union REG_ISP_BNR_WEIGHT_D45      {
	uint32_t raw;
	struct {
		uint32_t BNR_WEIGHT_D45                  : 5;
	} bits;
};

union REG_ISP_BNR_WEIGHT_D135     {
	uint32_t raw;
	struct {
		uint32_t BNR_WEIGHT_D135                 : 5;
	} bits;
};

union REG_ISP_BNR_NEIGHBOR_MAX    {
	uint32_t raw;
	struct {
		uint32_t BNR_NEIGHBOR_MAX                : 1;
	} bits;
};

union REG_ISP_BNR_RES_K_SMOOTH    {
	uint32_t raw;
	struct {
		uint32_t BNR_RES_K_SMOOTH                : 9;
	} bits;
};

union REG_ISP_BNR_RES_K_TEXTURE   {
	uint32_t raw;
	struct {
		uint32_t BNR_RES_K_TEXTURE               : 9;
	} bits;
};

union REG_ISP_BNR_LSC_EN {
	uint32_t raw;
	struct {
		uint32_t BNR_LSC_EN                      : 1;
	} bits;
};

union REG_ISP_BNR_HSTR    {
	uint32_t raw;
	struct {
		uint32_t BNR_HSTR                        : 12;
	} bits;
};

union REG_ISP_BNR_HSIZE           {
	uint32_t raw;
	struct {
		uint32_t BNR_HSIZE                       : 12;
	} bits;
};

union REG_ISP_BNR_VSIZE           {
	uint32_t raw;
	struct {
		uint32_t BNR_VSIZE                       : 12;
	} bits;
};

union REG_ISP_BNR_X_CENTER        {
	uint32_t raw;
	struct {
		uint32_t BNR_X_CENTER                    : 12;
	} bits;
};

union REG_ISP_BNR_Y_CENTER        {
	uint32_t raw;
	struct {
		uint32_t BNR_Y_CENTER                    : 12;
	} bits;
};

union REG_ISP_BNR_NORM_FACTOR     {
	uint32_t raw;
	struct {
		uint32_t BNR_NORM_FACTOR                 : 15;
	} bits;
};

union REG_ISP_BNR_LSC_LUT         {
	uint32_t raw;
	struct {
		uint32_t BNR_LSC_LUT                     : 8;
	} bits;
};

union REG_ISP_BNR_LSC_STRENTH     {
	uint32_t raw;
	struct {
		uint32_t BNR_LSC_STRENTH                 : 12;
	} bits;
};

union REG_ISP_BNR_DUMMY           {
	uint32_t raw;
	struct {
		uint32_t BNR_DUMMY                       : 16;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_RGB_0 {
	uint32_t raw;
	struct {
		uint32_t RGBTOP_BAYER_TYPE               : 4;
		uint32_t RGBTOP_RGBIR_ENABLE             : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t UP_PQ_EN                        : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t PROG_HDK_DIS                    : 1;
	} bits;
};

union REG_ISP_RGB_1 {
	uint32_t raw;
	struct {
		uint32_t RGB_SHDW_SEL                    : 1;
	} bits;
};

union REG_ISP_RGB_2 {
	uint32_t raw;
	struct {
		uint32_t SHDW_DMY                        : 32;
	} bits;
};

union REG_ISP_RGB_3 {
	uint32_t raw;
	struct {
		uint32_t DMY                             : 32;
	} bits;
};

union REG_ISP_RGB_4 {
	uint32_t raw;
	struct {
		uint32_t PROB_OUT_SEL                    : 5;
		uint32_t PROB_PERFMT                     : 1;
		uint32_t _rsv_6                          : 2;
		uint32_t PROB_FMT                        : 6;
	} bits;
};

union REG_ISP_RGB_5 {
	uint32_t raw;
	struct {
		uint32_t PROB_LINE                       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t PROB_PIX                        : 12;
	} bits;
};

union REG_ISP_RGB_6 {
	uint32_t raw;
	struct {
		uint32_t PROB_R                          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t PROB_G                          : 12;
	} bits;
};

union REG_ISP_RGB_7 {
	uint32_t raw;
	struct {
		uint32_t PROB_B                          : 12;
	} bits;
};

union REG_ISP_RGB_8 {
	uint32_t raw;
	struct {
		uint32_t FORCE_CLK_ENABLE                : 1;
		uint32_t DBG_EN                          : 1;
	} bits;
};

union REG_ISP_RGB_9 {
	uint32_t raw;
	struct {
		uint32_t RGBTOP_IMGW_M1                  : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t RGBTOP_IMGH_M1                  : 13;
	} bits;
};

union REG_ISP_RGB_10 {
	uint32_t raw;
	struct {
		uint32_t IR_BIT_MODE                     : 1;
		uint32_t IR_SW_MODE                      : 1;
		uint32_t IR_DMI_ENABLE                   : 1;
		uint32_t IR_CROP_ENABLE                  : 1;
		uint32_t IR_DMI_NUM_SW                   : 14;
	} bits;
};

union REG_ISP_RGB_11 {
	uint32_t raw;
	struct {
		uint32_t IR_IMG_WIDTH                    : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IR_IMG_HEIGHT                   : 14;
	} bits;
};

union REG_ISP_RGB_12 {
	uint32_t raw;
	struct {
		uint32_t IR_CROP_W_STR                   : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IR_CROP_W_END                   : 14;
	} bits;
};

union REG_ISP_RGB_13 {
	uint32_t raw;
	struct {
		uint32_t IR_CROP_H_STR                   : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IR_CROP_H_END                   : 14;
	} bits;
};

union REG_ISP_RGB_14 {
	uint32_t raw;
	struct {
		uint32_t IRM_ENABLE                      : 1;
		uint32_t IRM_HW_RQOS                     : 1;
		uint32_t _rsv_2                          : 2;
		uint32_t IR_BLENDING_WGT                 : 9;
		uint32_t _rsv_13                         : 3;
		uint32_t IR_DMI_NUM                      : 14;
	} bits;
};

union REG_ISP_RGB_15 {
	uint32_t raw;
	struct {
		uint32_t CACP_MEM_D                      : 24;
		uint32_t _rsv_24                         : 7;
		uint32_t CACP_MEM_W                      : 1;
	} bits;
};

union REG_ISP_RGB_16 {
	uint32_t raw;
	struct {
		uint32_t CACP_MEM_ST_ADDR                : 8;
		uint32_t _rsv_8                          : 23;
		uint32_t CACP_MEM_ST_ADDR_W              : 1;
	} bits;
};

union REG_ISP_RGB_17 {
	uint32_t raw;
	struct {
		uint32_t CACP_MEM_SW_RADDR               : 8;
		uint32_t _rsv_8                          : 23;
		uint32_t CACP_MEM_SW_R                   : 1;
	} bits;
};

union REG_ISP_RGB_18 {
	uint32_t raw;
	struct {
		uint32_t CACP_MEM_SW_RDATA_R             : 24;
	} bits;
};

union REG_ISP_RGB_19 {
	uint32_t raw;
	struct {
		uint32_t CACP_ENABLE                     : 1;
		uint32_t CACP_MODE                       : 1;
		uint32_t CACP_DBG_MODE                   : 1;
		uint32_t CACP_MEM_SW_MODE                : 1;
		uint32_t _rsv_4                          : 12;
		uint32_t CACP_ISO_RATIO                  : 11;
	} bits;
};

union REG_ISP_RGB_DBG_IP_S_VLD {
	uint32_t raw;
	struct {
		uint32_t IP_S_TVALID                     : 31;
		uint32_t IP_DBG_EN                       : 1;
	} bits;
};

union REG_ISP_RGB_DBG_IP_S_RDY {
	uint32_t raw;
	struct {
		uint32_t IP_S_TREADY                     : 31;
	} bits;
};

union REG_ISP_RGB_DBG_DMI_VLD {
	uint32_t raw;
	struct {
		uint32_t IP_DMI_VALID                    : 16;
	} bits;
};

union REG_ISP_RGB_DBG_DMI_RDY {
	uint32_t raw;
	struct {
		uint32_t IP_DMI_READY                    : 16;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_LSC_STATUS {
	uint32_t raw;
	struct {
		uint32_t LSC_STATUS                      : 32;
	} bits;
};

union REG_ISP_LSC_GRACE_RESET {
	uint32_t raw;
	struct {
		uint32_t LSC_GRACE_RESET                 : 1;
	} bits;
};

union REG_ISP_LSC_MONITOR {
	uint32_t raw;
	struct {
		uint32_t LSC_MONITOR                     : 32;
	} bits;
};

union REG_ISP_LSC_ENABLE {
	uint32_t raw;
	struct {
		uint32_t LSC_ENABLE                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t LSC_GAIN_3P9_0_4P8_1            : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t LSC_GAIN_BICUBIC_0_BILINEAR_1   : 1;
		uint32_t LSC_BOUNDARY_INTERPOLATION_MODE : 1;
		uint32_t _rsv_10                         : 2;
		uint32_t LSC_RENORMALIZE_ENABLE          : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t LSC_HDR_ENABLE                  : 1;
		uint32_t _rsv_17                         : 3;
		uint32_t LSC_BLOCKING_GAIN_UPDATE_ENABLE : 1;
		uint32_t _rsv_21                         : 3;
		uint32_t LSC_35TILE_ENABLE               : 1;
		uint32_t _rsv_25                         : 3;
		uint32_t FORCE_CLK_ENABLE                : 1;
	} bits;
};

union REG_ISP_LSC_KICKOFF {
	uint32_t raw;
	struct {
		uint32_t LSC_KICKOFF                     : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t LSC_GAINMOVER_ENABLE            : 1;
		uint32_t _rsv_5                          : 7;
		uint32_t LSC_SHADOW_SELECT               : 1;
	} bits;
};

union REG_ISP_LSC_STRENGTH {
	uint32_t raw;
	struct {
		uint32_t LSC_STRENGTH                    : 12;
	} bits;
};

union REG_ISP_LSC_IMG_BAYERID {
	uint32_t raw;
	struct {
		uint32_t IMG_BAYERID                     : 4;
		uint32_t _rsv_4                          : 12;
		uint32_t FORCE_BAYER_ENABLE              : 1;
	} bits;
};

union REG_ISP_LSC_MONITOR_SELECT {
	uint32_t raw;
	struct {
		uint32_t LSC_MONITOR_SELECT              : 32;
	} bits;
};

union REG_ISP_LSC_BLK_NUM_SELECT {
	uint32_t raw;
	struct {
		uint32_t LSC_BLK_NUM_SELECT              : 1;
	} bits;
};

union REG_ISP_LSC_DMI_WIDTHM1 {
	uint32_t raw;
	struct {
		uint32_t LSC_DMI_WIDTHM1                 : 13;
	} bits;
};

union REG_ISP_LSC_DMI_HEIGHTM1 {
	uint32_t raw;
	struct {
		uint32_t LSC_DMI_HEIGHTM1                : 13;
	} bits;
};

union REG_ISP_LSC_GAIN_BASE {
	uint32_t raw;
	struct {
		uint32_t LSC_GAIN_BASE                   : 2;
	} bits;
};

union REG_ISP_LSC_XSTEP {
	uint32_t raw;
	struct {
		uint32_t LSC_XSTEP                       : 15;
	} bits;
};

union REG_ISP_LSC_YSTEP {
	uint32_t raw;
	struct {
		uint32_t LSC_YSTEP                       : 15;
	} bits;
};

union REG_ISP_LSC_IMGX0 {
	uint32_t raw;
	struct {
		uint32_t LSC_IMGX0                       : 22;
	} bits;
};

union REG_ISP_LSC_IMGY0 {
	uint32_t raw;
	struct {
		uint32_t LSC_IMGY0                       : 22;
	} bits;
};

union REG_ISP_LSC_INITX0 {
	uint32_t raw;
	struct {
		uint32_t LSC_INITX0                      : 22;
	} bits;
};

union REG_ISP_LSC_INITY0 {
	uint32_t raw;
	struct {
		uint32_t LSC_INITY0                      : 22;
	} bits;
};

union REG_ISP_LSC_KERNEL_TABLE_WRITE {
	uint32_t raw;
	struct {
		uint32_t LSC_KERNEL_TABLE_WRITE          : 1;
	} bits;
};

union REG_ISP_LSC_KERNEL_TABLE_DATA {
	uint32_t raw;
	struct {
		uint32_t LSC_KERNEL_TABLE_DATA           : 32;
	} bits;
};

union REG_ISP_LSC_KERNEL_TABLE_CTRL {
	uint32_t raw;
	struct {
		uint32_t LSC_KERNEL_TABLE_START          : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t LSC_KERNEL_TABLE_W              : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t LSC_KERNEL_TABLE_DONE           : 1;
	} bits;
};

union REG_ISP_LSC_DUMMY {
	uint32_t raw;
	struct {
		uint32_t LSC_DUMMY                       : 16;
		uint32_t LSC_DEBUG                       : 16;
	} bits;
};

union REG_ISP_LSC_LOCATION {
	uint32_t raw;
	struct {
		uint32_t LSC_LOCATION                    : 32;
	} bits;
};

union REG_ISP_LSC_1ST_RUNHIT {
	uint32_t raw;
	struct {
		uint32_t LSC_1ST_RUNHIT                  : 32;
	} bits;
};

union REG_ISP_LSC_COMPARE_VALUE {
	uint32_t raw;
	struct {
		uint32_t LSC_COMPARE_VALUE               : 32;
	} bits;
};

union REG_ISP_LSC_MEM_SW_MODE {
	uint32_t raw;
	struct {
		uint32_t LSC_MEM_SW_MODE                 : 1;
		uint32_t _rsv_1                          : 4;
		uint32_t LSC_CUBIC_KERNEL_MEM_SEL        : 1;
	} bits;
};

union REG_ISP_LSC_MEM_SW_RADDR {
	uint32_t raw;
	struct {
		uint32_t LSC_SW_RADDR                    : 7;
	} bits;
};

union REG_ISP_LSC_MEM_SW_RDATA {
	uint32_t raw;
	struct {
		uint32_t LSC_RDATA                       : 31;
		uint32_t LSC_SW_READ                     : 1;
	} bits;
};

union REG_ISP_LSC_INTERPOLATION {
	uint32_t raw;
	struct {
		uint32_t LSC_BOUNDARY_INTERPOLATION_LF_RANGE: 6;
		uint32_t _rsv_6                          : 2;
		uint32_t LSC_BOUNDARY_INTERPOLATION_UP_RANGE: 6;
		uint32_t _rsv_14                         : 2;
		uint32_t LSC_BOUNDARY_INTERPOLATION_RT_RANGE: 6;
		uint32_t _rsv_22                         : 2;
		uint32_t LSC_BOUNDARY_INTERPOLATION_DN_RANGE: 6;
	} bits;
};

union REG_ISP_LSC_DMI_ENABLE {
	uint32_t raw;
	struct {
		uint32_t DMI_ENABLE                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t DMI_QOS                         : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t FORCE_DMA_DISABLE               : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t DMI_PULL_AFTER_DONE             : 1;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_CCM_00 {
	uint32_t raw;
	struct {
		uint32_t CCM_00                          : 14;
	} bits;
};

union REG_ISP_CCM_01 {
	uint32_t raw;
	struct {
		uint32_t CCM_01                          : 14;
	} bits;
};

union REG_ISP_CCM_02 {
	uint32_t raw;
	struct {
		uint32_t CCM_02                          : 14;
	} bits;
};

union REG_ISP_CCM_10 {
	uint32_t raw;
	struct {
		uint32_t CCM_10                          : 14;
	} bits;
};

union REG_ISP_CCM_11 {
	uint32_t raw;
	struct {
		uint32_t CCM_11                          : 14;
	} bits;
};

union REG_ISP_CCM_12 {
	uint32_t raw;
	struct {
		uint32_t CCM_12                          : 14;
	} bits;
};

union REG_ISP_CCM_20 {
	uint32_t raw;
	struct {
		uint32_t CCM_20                          : 14;
	} bits;
};

union REG_ISP_CCM_21 {
	uint32_t raw;
	struct {
		uint32_t CCM_21                          : 14;
	} bits;
};

union REG_ISP_CCM_22 {
	uint32_t raw;
	struct {
		uint32_t CCM_22                          : 14;
	} bits;
};

union REG_ISP_CCM_CTRL {
	uint32_t raw;
	struct {
		uint32_t CCM_SHDW_SEL                    : 1;
		uint32_t _rsv_1                          : 1;
		uint32_t CCM_ENABLE                      : 1;
	} bits;
};

union REG_ISP_CCM_DBG {
	uint32_t raw;
	struct {
		uint32_t FORCE_CLK_ENABLE                : 1;
		uint32_t SOFTRST                         : 1;
	} bits;
};

union REG_ISP_CCM_DMY0 {
	uint32_t raw;
	struct {
		uint32_t DMY_DEF0                        : 32;
	} bits;
};

union REG_ISP_CCM_DMY1 {
	uint32_t raw;
	struct {
		uint32_t DMY_DEF1                        : 32;
	} bits;
};

union REG_ISP_CCM_DMY_R {
	uint32_t raw;
	struct {
		uint32_t DMY_RO                          : 32;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_MMAP_00 {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_ENABLE                   : 1;
		uint32_t MMAP_1_ENABLE                   : 1;
		uint32_t MMAP_MRG_MODE                   : 1;
		uint32_t MM_DOUBLE_BUF_SEL               : 1;
		uint32_t ON_THE_FLY                      : 1;
		uint32_t FIRST_FRAME_RESET               : 1;
		uint32_t REG_2_TAP_EN                    : 1;
		uint32_t MIRROR_MODE_EN                  : 1;
		uint32_t MMAP_MRG_ALPH                   : 8;
		uint32_t GUARD_CNT                       : 8;
		uint32_t BYPASS                          : 1;
		uint32_t INTER_1_EN                      : 1;
		uint32_t INTER_2_EN                      : 1;
		uint32_t INTER_3_EN                      : 1;
		uint32_t INTER_4_EN                      : 1;
		uint32_t DMA_SEL                         : 1;
		uint32_t RGBMAP_SW_CROP                  : 1;
	} bits;
};

union REG_ISP_MMAP_04 {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_LPF_00                   : 3;
		uint32_t MMAP_0_LPF_01                   : 3;
		uint32_t MMAP_0_LPF_02                   : 3;
		uint32_t MMAP_0_LPF_10                   : 3;
		uint32_t MMAP_0_LPF_11                   : 3;
		uint32_t MMAP_0_LPF_12                   : 3;
		uint32_t MMAP_0_LPF_20                   : 3;
		uint32_t MMAP_0_LPF_21                   : 3;
		uint32_t MMAP_0_LPF_22                   : 3;
		uint32_t _rsv_27                         : 2;
		uint32_t FORCE_CLK_EN                    : 1;
		uint32_t REG_8BIT_RGBMAP_MODE            : 1;
		uint32_t WH_SW_MODE                      : 1;
	} bits;
};

union REG_ISP_MMAP_08 {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_MAP_CORING               : 8;
		uint32_t MMAP_0_MAP_GAIN                 : 8;
		uint32_t MMAP_0_MAP_THD_L                : 8;
		uint32_t MMAP_0_MAP_THD_H                : 8;
	} bits;
};

union REG_ISP_MMAP_0C {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_LUMA_ADAPT_LUT_IN_0      : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t MMAP_0_LUMA_ADAPT_LUT_IN_1      : 12;
	} bits;
};

union REG_ISP_MMAP_10 {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_LUMA_ADAPT_LUT_IN_2      : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t MMAP_0_LUMA_ADAPT_LUT_IN_3      : 12;
	} bits;
};

union REG_ISP_MMAP_14 {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_LUMA_ADAPT_LUT_OUT_0     : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t MMAP_0_LUMA_ADAPT_LUT_OUT_1     : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t MMAP_0_LUMA_ADAPT_LUT_OUT_2     : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t MMAP_0_LUMA_ADAPT_LUT_OUT_3     : 6;
	} bits;
};

union REG_ISP_MMAP_18 {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_LUMA_ADAPT_LUT_SLOPE_0   : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t MMAP_0_LUMA_ADAPT_LUT_SLOPE_1   : 11;
	} bits;
};

union REG_ISP_MMAP_1C {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_LUMA_ADAPT_LUT_SLOPE_2   : 11;
		uint32_t _rsv_11                         : 1;
		uint32_t MMAP_0_MAP_DSHIFT_BIT           : 3;
	} bits;
};

union REG_ISP_MMAP_20 {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_IIR_PRTCT_LUT_IN_0       : 8;
		uint32_t MMAP_0_IIR_PRTCT_LUT_IN_1       : 8;
		uint32_t MMAP_0_IIR_PRTCT_LUT_IN_2       : 8;
		uint32_t MMAP_0_IIR_PRTCT_LUT_IN_3       : 8;
	} bits;
};

union REG_ISP_MMAP_24 {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_IIR_PRTCT_LUT_OUT_0      : 4;
		uint32_t MMAP_0_IIR_PRTCT_LUT_OUT_1      : 4;
		uint32_t MMAP_0_IIR_PRTCT_LUT_OUT_2      : 4;
		uint32_t MMAP_0_IIR_PRTCT_LUT_OUT_3      : 4;
	} bits;
};

union REG_ISP_MMAP_28 {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_IIR_PRTCT_LUT_SLOPE_0    : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t MMAP_0_IIR_PRTCT_LUT_SLOPE_1    : 9;
	} bits;
};

union REG_ISP_MMAP_2C {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_IIR_PRTCT_LUT_SLOPE_2    : 9;
	} bits;
};

union REG_ISP_MMAP_30 {
	uint32_t raw;
	struct {
		uint32_t IMG_WIDTHM1_SW                  : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_HEIGHTM1_SW                 : 14;
	} bits;
};

union REG_ISP_MMAP_34 {
	uint32_t raw;
	struct {
		uint32_t V_THD_L                         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t V_THD_H                         : 12;
	} bits;
};

union REG_ISP_MMAP_38 {
	uint32_t raw;
	struct {
		uint32_t MMAP_1_MAP_CORING               : 8;
	} bits;
};

union REG_ISP_MMAP_3C {
	uint32_t raw;
	struct {
		uint32_t V_WGT_SLP                       : 19;
		uint32_t _rsv_19                         : 5;
		uint32_t MOTION_LS_MODE                  : 1;
		uint32_t MOTION_LS_SEL                   : 1;
		uint32_t MOTION_YV_LS_MODE               : 1;
		uint32_t MOTION_YV_LS_SEL                : 1;
	} bits;
};

union REG_ISP_MMAP_40 {
	uint32_t raw;
	struct {
		uint32_t V_WGT_MAX                       : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t V_WGT_MIN                       : 9;
	} bits;
};

union REG_ISP_MMAP_44 {
	uint32_t raw;
	struct {
		uint32_t MMAP_MED_WGT                    : 9;
		uint32_t _rsv_9                          : 6;
		uint32_t MMAP_MED_ENABLE                 : 1;
	} bits;
};

union REG_ISP_MMAP_4C {
	uint32_t raw;
	struct {
		uint32_t MMAP_1_LUMA_ADAPT_LUT_IN_0      : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t MMAP_1_LUMA_ADAPT_LUT_IN_1      : 12;
	} bits;
};

union REG_ISP_MMAP_50 {
	uint32_t raw;
	struct {
		uint32_t MMAP_1_LUMA_ADAPT_LUT_IN_2      : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t MMAP_1_LUMA_ADAPT_LUT_IN_3      : 12;
	} bits;
};

union REG_ISP_MMAP_54 {
	uint32_t raw;
	struct {
		uint32_t MMAP_1_LUMA_ADAPT_LUT_OUT_0     : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t MMAP_1_LUMA_ADAPT_LUT_OUT_1     : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t MMAP_1_LUMA_ADAPT_LUT_OUT_2     : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t MMAP_1_LUMA_ADAPT_LUT_OUT_3     : 6;
	} bits;
};

union REG_ISP_MMAP_58 {
	uint32_t raw;
	struct {
		uint32_t MMAP_1_LUMA_ADAPT_LUT_SLOPE_0   : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t MMAP_1_LUMA_ADAPT_LUT_SLOPE_1   : 11;
	} bits;
};

union REG_ISP_MMAP_5C {
	uint32_t raw;
	struct {
		uint32_t MMAP_1_LUMA_ADAPT_LUT_SLOPE_2   : 11;
	} bits;
};

union REG_ISP_MMAP_60 {
	uint32_t raw;
	struct {
		uint32_t RGBMAP_W_BIT                    : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t RGBMAP_H_BIT                    : 3;
	} bits;
};

union REG_ISP_MMAP_64 {
	uint32_t raw;
	struct {
		uint32_t SRAM_WDATA_0                    : 32;
	} bits;
};

union REG_ISP_MMAP_68 {
	uint32_t raw;
	struct {
		uint32_t SRAM_WDATA_1                    : 32;
	} bits;
};

union REG_ISP_MMAP_6C {
	uint32_t raw;
	struct {
		uint32_t SRAM_WADD                       : 7;
		uint32_t SRAM_WEN                        : 1;
		uint32_t FORCE_DMA_DISABLE               : 8;
		uint32_t MANR_DEBUG                      : 16;
	} bits;
};

union REG_ISP_MMAP_70 {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_GAIN_RATIO_R             : 16;
		uint32_t MMAP_0_GAIN_RATIO_G             : 16;
	} bits;
};

union REG_ISP_MMAP_74 {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_GAIN_RATIO_B             : 16;
	} bits;
};

union REG_ISP_MMAP_78 {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_NS_SLOPE_R               : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t MMAP_0_NS_SLOPE_G               : 10;
	} bits;
};

union REG_ISP_MMAP_7C {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_NS_SLOPE_B               : 10;
	} bits;
};

union REG_ISP_MMAP_80 {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_NS_LUMA_TH0_R            : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t MMAP_0_NS_LUMA_TH0_G            : 12;
	} bits;
};

union REG_ISP_MMAP_84 {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_NS_LUMA_TH0_B            : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t MMAP_0_NS_LOW_OFFSET_R          : 12;
	} bits;
};

union REG_ISP_MMAP_88 {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_NS_LOW_OFFSET_G          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t MMAP_0_NS_LOW_OFFSET_B          : 12;
	} bits;
};

union REG_ISP_MMAP_8C {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_NS_HIGH_OFFSET_R         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t MMAP_0_NS_HIGH_OFFSET_G         : 12;
	} bits;
};

union REG_ISP_MMAP_90 {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_NS_HIGH_OFFSET_B         : 12;
	} bits;
};

union REG_ISP_MMAP_A0 {
	uint32_t raw;
	struct {
		uint32_t MMAP_1_GAIN_RATIO_R             : 16;
		uint32_t MMAP_1_GAIN_RATIO_G             : 16;
	} bits;
};

union REG_ISP_MMAP_A4 {
	uint32_t raw;
	struct {
		uint32_t MMAP_1_GAIN_RATIO_B             : 16;
	} bits;
};

union REG_ISP_MMAP_A8 {
	uint32_t raw;
	struct {
		uint32_t MMAP_1_NS_SLOPE_R               : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t MMAP_1_NS_SLOPE_G               : 10;
	} bits;
};

union REG_ISP_MMAP_AC {
	uint32_t raw;
	struct {
		uint32_t MMAP_1_NS_SLOPE_B               : 10;
	} bits;
};

union REG_ISP_MMAP_B0 {
	uint32_t raw;
	struct {
		uint32_t MMAP_1_NS_LUMA_TH0_R            : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t MMAP_1_NS_LUMA_TH0_G            : 12;
	} bits;
};

union REG_ISP_MMAP_B4 {
	uint32_t raw;
	struct {
		uint32_t MMAP_1_NS_LUMA_TH0_B            : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t MMAP_1_NS_LOW_OFFSET_R          : 12;
	} bits;
};

union REG_ISP_MMAP_B8 {
	uint32_t raw;
	struct {
		uint32_t MMAP_1_NS_LOW_OFFSET_G          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t MMAP_1_NS_LOW_OFFSET_B          : 12;
	} bits;
};

union REG_ISP_MMAP_BC {
	uint32_t raw;
	struct {
		uint32_t MMAP_1_NS_HIGH_OFFSET_R         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t MMAP_1_NS_HIGH_OFFSET_G         : 12;
	} bits;
};

union REG_ISP_MMAP_C0 {
	uint32_t raw;
	struct {
		uint32_t MMAP_1_NS_HIGH_OFFSET_B         : 12;
	} bits;
};

union REG_ISP_MMAP_C4 {
	uint32_t raw;
	struct {
		uint32_t IMG_WIDTH_CROP                  : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_HEIGHT_CROP                 : 14;
		uint32_t _rsv_30                         : 1;
		uint32_t CROP_ENABLE                     : 1;
	} bits;
};

union REG_ISP_MMAP_C8 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR                      : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END                      : 14;
	} bits;
};

union REG_ISP_MMAP_CC {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR                      : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END                      : 14;
	} bits;
};

union REG_ISP_MMAP_D0 {
	uint32_t raw;
	struct {
		uint32_t IMG_WIDTH_CROP_SCALAR           : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_HEIGHT_CROP_SCALAR          : 14;
		uint32_t _rsv_30                         : 1;
		uint32_t CROP_ENABLE_SCALAR              : 1;
	} bits;
};

union REG_ISP_MMAP_D4 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR_SCALAR               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END_SCALAR               : 14;
	} bits;
};

union REG_ISP_MMAP_D8 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR_SCALAR               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END_SCALAR               : 14;
	} bits;
};

union REG_ISP_MMAP_DC {
	uint32_t raw;
	struct {
		uint32_t COEF_R                          : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t COEF_G                          : 11;
	} bits;
};

union REG_ISP_MMAP_E0 {
	uint32_t raw;
	struct {
		uint32_t COEF_B                          : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t COEF_I                          : 11;
	} bits;
};

union REG_ISP_MMAP_E4 {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_GAIN_RATIO_I             : 16;
		uint32_t MMAP_0_NS_SLOPE_I               : 10;
	} bits;
};

union REG_ISP_MMAP_E8 {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_NS_LUMA_TH0_I            : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t MMAP_0_NS_LOW_OFFSET_I          : 12;
	} bits;
};

union REG_ISP_MMAP_EC {
	uint32_t raw;
	struct {
		uint32_t MMAP_0_NS_HIGH_OFFSET_I         : 12;
	} bits;
};

union REG_ISP_MMAP_F0 {
	uint32_t raw;
	struct {
		uint32_t MMAP_1_GAIN_RATIO_I             : 16;
		uint32_t MMAP_1_NS_SLOPE_I               : 10;
	} bits;
};

union REG_ISP_MMAP_F4 {
	uint32_t raw;
	struct {
		uint32_t MMAP_1_NS_LUMA_TH0_I            : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t MMAP_1_NS_LOW_OFFSET_I          : 12;
	} bits;
};

union REG_ISP_MMAP_F8 {
	uint32_t raw;
	struct {
		uint32_t MMAP_1_NS_HIGH_OFFSET_I         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t HISTORY_SEL_0                   : 1;
		uint32_t HISTORY_SEL_1                   : 1;
		uint32_t HISTORY_SEL_2                   : 1;
		uint32_t HISTORY_SEL_3                   : 1;
		uint32_t _rsv_20                         : 4;
		uint32_t MMAP_0_MH_WGT                   : 4;
	} bits;
};

union REG_ISP_MMAP_FC {
	uint32_t raw;
	struct {
		uint32_t MANR_STATUS                     : 28;
		uint32_t MANR_STATUS_MUX                 : 4;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_GAMMA_CTRL {
	uint32_t raw;
	struct {
		uint32_t GAMMA_ENABLE                    : 1;
		uint32_t GAMMA_SHDW_SEL                  : 1;
		uint32_t FORCE_CLK_ENABLE                : 1;
	} bits;
};

union REG_ISP_GAMMA_PROG_CTRL {
	uint32_t raw;
	struct {
		uint32_t GAMMA_WSEL                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t GAMMA_RSEL                      : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t GAMMA_PROG_EN                   : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t GAMMA_PROG_1TO3_EN              : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t GAMMA_PROG_MODE                 : 2;
	} bits;
};

union REG_ISP_GAMMA_PROG_ST_ADDR {
	uint32_t raw;
	struct {
		uint32_t GAMMA_ST_ADDR                   : 8;
		uint32_t _rsv_8                          : 23;
		uint32_t GAMMA_ST_W                      : 1;
	} bits;
};

union REG_ISP_GAMMA_PROG_DATA {
	uint32_t raw;
	struct {
		uint32_t GAMMA_DATA_E                    : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t GAMMA_DATA_O                    : 12;
		uint32_t _rsv_28                         : 3;
		uint32_t GAMMA_W                         : 1;
	} bits;
};

union REG_ISP_GAMMA_PROG_MAX {
	uint32_t raw;
	struct {
		uint32_t GAMMA_MAX                       : 12;
	} bits;
};

union REG_ISP_GAMMA_MEM_SW_RADDR {
	uint32_t raw;
	struct {
		uint32_t GAMMA_SW_RADDR                  : 8;
		uint32_t _rsv_8                          : 4;
		uint32_t GAMMA_SW_R_MEM_SEL              : 1;
	} bits;
};

union REG_ISP_GAMMA_MEM_SW_RDATA {
	uint32_t raw;
	struct {
		uint32_t GAMMA_RDATA_R                   : 12;
		uint32_t _rsv_12                         : 19;
		uint32_t GAMMA_SW_R                      : 1;
	} bits;
};

union REG_ISP_GAMMA_MEM_SW_RDATA_BG {
	uint32_t raw;
	struct {
		uint32_t GAMMA_RDATA_G                   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t GAMMA_RDATA_B                   : 12;
	} bits;
};

union REG_ISP_GAMMA_DBG {
	uint32_t raw;
	struct {
		uint32_t PROG_HDK_DIS                    : 1;
		uint32_t SOFTRST                         : 1;
	} bits;
};

union REG_ISP_GAMMA_DMY0 {
	uint32_t raw;
	struct {
		uint32_t DMY_DEF0                        : 32;
	} bits;
};

union REG_ISP_GAMMA_DMY1 {
	uint32_t raw;
	struct {
		uint32_t DMY_DEF1                        : 32;
	} bits;
};

union REG_ISP_GAMMA_DMY_R {
	uint32_t raw;
	struct {
		uint32_t DMY_RO                          : 32;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_CLUT_CTRL {
	uint32_t raw;
	struct {
		uint32_t CLUT_ENABLE                     : 1;
		uint32_t CLUT_SHDW_SEL                   : 1;
		uint32_t FORCE_CLK_ENABLE                : 1;
		uint32_t PROG_EN                         : 1;
	} bits;
};

union REG_ISP_CLUT_PROG_ADDR {
	uint32_t raw;
	struct {
		uint32_t SRAM_R_IDX                      : 5;
		uint32_t _rsv_5                          : 3;
		uint32_t SRAM_G_IDX                      : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t SRAM_B_IDX                      : 5;
	} bits;
};

union REG_ISP_CLUT_PROG_DATA {
	uint32_t raw;
	struct {
		uint32_t SRAM_WDATA                      : 30;
		uint32_t _rsv_30                         : 1;
		uint32_t SRAM_WR                         : 1;
	} bits;
};

union REG_ISP_CLUT_PROG_RDATA {
	uint32_t raw;
	struct {
		uint32_t SRAM_RDATA                      : 30;
		uint32_t _rsv_30                         : 1;
		uint32_t SRAM_RD                         : 1;
	} bits;
};

union REG_ISP_CLUT_DBG {
	uint32_t raw;
	struct {
		uint32_t PROG_HDK_DIS                    : 1;
	} bits;
};

union REG_ISP_CLUT_DMY0 {
	uint32_t raw;
	struct {
		uint32_t DMY_DEF0                        : 32;
	} bits;
};

union REG_ISP_CLUT_DMY1 {
	uint32_t raw;
	struct {
		uint32_t DMY_DEF1                        : 32;
	} bits;
};

union REG_ISP_CLUT_DMY_R {
	uint32_t raw;
	struct {
		uint32_t DMY_RO                          : 32;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_DHZ_SMOOTH {
	uint32_t raw;
	struct {
		uint32_t DEHAZE_W                        : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t DEHAZE_TH_SMOOTH                : 10;
	} bits;
};

union REG_ISP_DHZ_SKIN {
	uint32_t raw;
	struct {
		uint32_t DEHAZE_SKIN_CB                  : 8;
		uint32_t DEHAZE_SKIN_CR                  : 8;
	} bits;
};

union REG_ISP_DHZ_BYPASS {
	uint32_t raw;
	struct {
		uint32_t DEHAZE_ENABLE                   : 1;
		uint32_t DEHAZE_LUMA_LUT_ENABLE          : 1;
		uint32_t DEHAZE_SKIN_LUT_ENABLE          : 1;
		uint32_t DEHAZE_SHDW_SEL                 : 1;
		uint32_t FORCE_CLK_ENABLE                : 1;
	} bits;
};

union REG_ISP_DHZ_0 {
	uint32_t raw;
	struct {
		uint32_t SOFTRST                         : 1;
		uint32_t _rsv_1                          : 4;
		uint32_t DBG_EN                          : 1;
		uint32_t _rsv_6                          : 10;
		uint32_t CHECK_SUM                       : 16;
	} bits;
};

union REG_ISP_DHZ_1 {
	uint32_t raw;
	struct {
		uint32_t DEHAZE_CUM_TH                   : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t DEHAZE_HIST_TH                  : 14;
	} bits;
};

union REG_ISP_DHZ_2 {
	uint32_t raw;
	struct {
		uint32_t DEHAZE_SW_DC_TH                 : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t DEHAZE_SW_AGLOBAL               : 12;
		uint32_t _rsv_28                         : 3;
		uint32_t DEHAZE_SW_DC_AGLOBAL_TRIG       : 1;
	} bits;
};

union REG_ISP_DHZ_3 {
	uint32_t raw;
	struct {
		uint32_t DEHAZE_TMAP_MIN                 : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t DEHAZE_TMAP_MAX                 : 13;
	} bits;
};

union REG_ISP_DHZ_4 {
	uint32_t raw;
	struct {
		uint32_t IMG_WD                          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t IMG_HT                          : 12;
		uint32_t BORDER                          : 2;
	} bits;
};

union REG_ISP_DHZ_5 {
	uint32_t raw;
	struct {
		uint32_t FMT_ST                          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t FMT_END                         : 12;
		uint32_t TILE_NM                         : 4;
	} bits;
};

union REG_ISP_DHZ_6 {
	uint32_t raw;
	struct {
		uint32_t DBG_SEL                         : 3;
	} bits;
};

union REG_ISP_DHZ_7 {
	uint32_t raw;
	struct {
		uint32_t DHZ_DBG0                        : 32;
	} bits;
};

union REG_ISP_DHZ_8 {
	uint32_t raw;
	struct {
		uint32_t DHZ_DBG1                        : 32;
	} bits;
};

union REG_ISP_DHZ_9 {
	uint32_t raw;
	struct {
		uint32_t _DEHAZE_LUMA_LUT00              : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t _DEHAZE_LUMA_LUT01              : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t _DEHAZE_LUMA_LUT02              : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t _DEHAZE_LUMA_LUT03              : 7;
	} bits;
};

union REG_ISP_DHZ_10 {
	uint32_t raw;
	struct {
		uint32_t _DEHAZE_LUMA_LUT04              : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t _DEHAZE_LUMA_LUT05              : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t _DEHAZE_LUMA_LUT06              : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t _DEHAZE_LUMA_LUT07              : 7;
	} bits;
};

union REG_ISP_DHZ_11 {
	uint32_t raw;
	struct {
		uint32_t _DEHAZE_LUMA_LUT08              : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t _DEHAZE_LUMA_LUT09              : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t _DEHAZE_LUMA_LUT10              : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t _DEHAZE_LUMA_LUT11              : 7;
	} bits;
};

union REG_ISP_DHZ_12 {
	uint32_t raw;
	struct {
		uint32_t _DEHAZE_LUMA_LUT12              : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t _DEHAZE_LUMA_LUT13              : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t _DEHAZE_LUMA_LUT14              : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t _DEHAZE_LUMA_LUT15              : 7;
	} bits;
};

union REG_ISP_DHZ_13 {
	uint32_t raw;
	struct {
		uint32_t _DEHAZE_LUMA_LUT16              : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t _DEHAZE_LUMA_LUT17              : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t _DEHAZE_LUMA_LUT18              : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t _DEHAZE_LUMA_LUT19              : 7;
	} bits;
};

union REG_ISP_DHZ_14 {
	uint32_t raw;
	struct {
		uint32_t _DEHAZE_LUMA_LUT20              : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t _DEHAZE_LUMA_LUT21              : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t _DEHAZE_LUMA_LUT22              : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t _DEHAZE_LUMA_LUT23              : 7;
	} bits;
};

union REG_ISP_DHZ_15 {
	uint32_t raw;
	struct {
		uint32_t _DEHAZE_LUMA_LUT24              : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t _DEHAZE_LUMA_LUT25              : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t _DEHAZE_LUMA_LUT26              : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t _DEHAZE_LUMA_LUT27              : 7;
	} bits;
};

union REG_ISP_DHZ_16 {
	uint32_t raw;
	struct {
		uint32_t _DEHAZE_LUMA_LUT28              : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t _DEHAZE_LUMA_LUT29              : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t _DEHAZE_LUMA_LUT30              : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t _DEHAZE_LUMA_LUT31              : 7;
	} bits;
};

union REG_ISP_DHZ_17 {
	uint32_t raw;
	struct {
		uint32_t _DEHAZE_SKIN_LUT00              : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t _DEHAZE_SKIN_LUT01              : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t _DEHAZE_SKIN_LUT02              : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t _DEHAZE_SKIN_LUT03              : 7;
	} bits;
};

union REG_ISP_DHZ_18 {
	uint32_t raw;
	struct {
		uint32_t _DEHAZE_SKIN_LUT04              : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t _DEHAZE_SKIN_LUT05              : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t _DEHAZE_SKIN_LUT06              : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t _DEHAZE_SKIN_LUT07              : 7;
	} bits;
};

union REG_ISP_DHZ_19 {
	uint32_t raw;
	struct {
		uint32_t _DEHAZE_SKIN_LUT08              : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t _DEHAZE_SKIN_LUT09              : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t _DEHAZE_SKIN_LUT10              : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t _DEHAZE_SKIN_LUT11              : 7;
	} bits;
};

union REG_ISP_DHZ_20 {
	uint32_t raw;
	struct {
		uint32_t _DEHAZE_SKIN_LUT12              : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t _DEHAZE_SKIN_LUT13              : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t _DEHAZE_SKIN_LUT14              : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t _DEHAZE_SKIN_LUT15              : 7;
	} bits;
};

union REG_ISP_DHZ_21 {
	uint32_t raw;
	struct {
		uint32_t _DEHAZE_SKIN_LUT16              : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t _DEHAZE_SKIN_LUT17              : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t _DEHAZE_SKIN_LUT18              : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t _DEHAZE_SKIN_LUT19              : 7;
	} bits;
};

union REG_ISP_DHZ_22 {
	uint32_t raw;
	struct {
		uint32_t _DEHAZE_SKIN_LUT20              : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t _DEHAZE_SKIN_LUT21              : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t _DEHAZE_SKIN_LUT22              : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t _DEHAZE_SKIN_LUT23              : 7;
	} bits;
};

union REG_ISP_DHZ_23 {
	uint32_t raw;
	struct {
		uint32_t _DEHAZE_SKIN_LUT24              : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t _DEHAZE_SKIN_LUT25              : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t _DEHAZE_SKIN_LUT26              : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t _DEHAZE_SKIN_LUT27              : 7;
	} bits;
};

union REG_ISP_DHZ_24 {
	uint32_t raw;
	struct {
		uint32_t _DEHAZE_SKIN_LUT28              : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t _DEHAZE_SKIN_LUT29              : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t _DEHAZE_SKIN_LUT30              : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t _DEHAZE_SKIN_LUT31              : 7;
	} bits;
};

union REG_ISP_DHZ_25 {
	uint32_t raw;
	struct {
		uint32_t AGLOBAL                         : 12;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_CSC_0 {
	uint32_t raw;
	struct {
		uint32_t CSC_ENABLE                      : 1;
		uint32_t R2Y4_SHDW_SEL                   : 1;
	} bits;
};

union REG_ISP_CSC_1 {
	uint32_t raw;
	struct {
		uint32_t OP_START                        : 1;
		uint32_t CONT_EN                         : 1;
		uint32_t R2Y4_BYPASS                     : 1;
		uint32_t SOFTRST                         : 1;
		uint32_t AUTO_UPDATE_EN                  : 1;
		uint32_t DBG_EN                          : 1;
		uint32_t _rsv_6                          : 10;
		uint32_t CHECK_SUM                       : 16;
	} bits;
};

union REG_ISP_CSC_2 {
	uint32_t raw;
	struct {
		uint32_t SHDW_UPDATE_REQ                 : 1;
	} bits;
};

union REG_ISP_CSC_3 {
	uint32_t raw;
	struct {
		uint32_t DMY0                            : 32;
	} bits;
};

union REG_ISP_CSC_4 {
	uint32_t raw;
	struct {
		uint32_t COEFF_00                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t COEFF_01                        : 14;
	} bits;
};

union REG_ISP_CSC_5 {
	uint32_t raw;
	struct {
		uint32_t COEFF_02                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t COEFF_10                        : 14;
	} bits;
};

union REG_ISP_CSC_6 {
	uint32_t raw;
	struct {
		uint32_t COEFF_11                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t COEFF_12                        : 14;
	} bits;
};

union REG_ISP_CSC_7 {
	uint32_t raw;
	struct {
		uint32_t COEFF_20                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t COEFF_21                        : 14;
	} bits;
};

union REG_ISP_CSC_8 {
	uint32_t raw;
	struct {
		uint32_t COEFF_22                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t OFFSET_0                        : 11;
	} bits;
};

union REG_ISP_CSC_9 {
	uint32_t raw;
	struct {
		uint32_t OFFSET_1                        : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t OFFSET_2                        : 11;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_RGBDITHER_RGB_DITHER {
	uint32_t raw;
	struct {
		uint32_t RGB_DITHER_ENABLE               : 1;
		uint32_t RGB_DITHER_MOD_EN               : 1;
		uint32_t RGB_DITHER_HISTIDX_EN           : 1;
		uint32_t RGB_DITHER_FMNUM_EN             : 1;
		uint32_t RGB_DITHER_SHDW_SEL             : 1;
		uint32_t RGB_DITHER_SOFTRST              : 1;
		uint32_t _rsv_6                          : 2;
		uint32_t CROP_WIDTHM1                    : 12;
		uint32_t CROP_HEIGHTM1                   : 12;
	} bits;
};

union REG_ISP_RGBDITHER_RGB_DITHER_DEBUG0 {
	uint32_t raw;
	struct {
		uint32_t RGB_DITHER_DEBUG0               : 32;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_DCI_STATUS {
	uint32_t raw;
	struct {
		uint32_t DCI_STATUS                      : 32;
	} bits;
};

union REG_ISP_DCI_GRACE_RESET {
	uint32_t raw;
	struct {
		uint32_t DCI_GRACE_RESET                 : 1;
	} bits;
};

union REG_ISP_DCI_MONITOR {
	uint32_t raw;
	struct {
		uint32_t DCI_MONITOR                     : 32;
	} bits;
};

union REG_ISP_DCI_ENABLE {
	uint32_t raw;
	struct {
		uint32_t DCI_ENABLE                      : 1;
		uint32_t _rsv_1                          : 15;
		uint32_t DCI_HIST_ENABLE                 : 1;
		uint32_t _rsv_17                         : 11;
		uint32_t FORCE_CLK_ENABLE                : 1;
	} bits;
};

union REG_ISP_DCI_MAP_ENABLE {
	uint32_t raw;
	struct {
		uint32_t DCI_MAP_ENABLE                  : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t DCI_PER1SAMPLE_ENABLE           : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t DCI_HISTO_BIG_ENDIAN            : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t DCI_ROI_ENABLE                  : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t DCI_DITHER_ENABLE               : 1;
		uint32_t _rsv_17                         : 3;
		uint32_t DCI_ZEROING_ENABLE              : 1;
		uint32_t _rsv_21                         : 3;
		uint32_t DCI_SHIFT_ENABLE                : 1;
		uint32_t _rsv_25                         : 3;
		uint32_t DCI_INDEX_ENABLE                : 1;
	} bits;
};

union REG_ISP_DCI_FLOW {
	uint32_t raw;
	struct {
		uint32_t DCI_ZERODCIOGRAM                : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t DCI_SHADOW_SELECT               : 1;
	} bits;
};

union REG_ISP_DCI_DEMO_MODE {
	uint32_t raw;
	struct {
		uint32_t DCI_DEMO_MODE                   : 1;
	} bits;
};

union REG_ISP_DCI_MONITOR_SELECT {
	uint32_t raw;
	struct {
		uint32_t DCI_MONITOR_SELECT              : 32;
	} bits;
};

union REG_ISP_DCI_LOCATION {
	uint32_t raw;
	struct {
		uint32_t DCI_LOCATION                    : 32;
	} bits;
};

union REG_ISP_DCI_PROG_DATA {
	uint32_t raw;
	struct {
		uint32_t DCI_PROG_DATA                   : 32;
	} bits;
};

union REG_ISP_DCI_PROG_CTRL {
	uint32_t raw;
	struct {
		uint32_t DCI_PROG_CTRL                   : 32;
	} bits;
};

union REG_ISP_DCI_PROG_MAX {
	uint32_t raw;
	struct {
		uint32_t DCI_PROG_MAX                    : 32;
	} bits;
};

union REG_ISP_DCI_CTRL {
	uint32_t raw;
	struct {
		uint32_t DCI_CTRL                        : 32;
	} bits;
};

union REG_ISP_DCI_MEM_SW_MODE {
	uint32_t raw;
	struct {
		uint32_t DCI_MEM_SW_MODE                 : 32;
	} bits;
};

union REG_ISP_DCI_MEM_RADDR {
	uint32_t raw;
	struct {
		uint32_t DCI_MEM_RADDR                   : 32;
	} bits;
};

union REG_ISP_DCI_MEM_RDATA {
	uint32_t raw;
	struct {
		uint32_t DCI_MEM_RDATA                   : 32;
	} bits;
};

union REG_ISP_DCI_DEBUG {
	uint32_t raw;
	struct {
		uint32_t DCI_DEBUG                       : 32;
	} bits;
};

union REG_ISP_DCI_DUMMY {
	uint32_t raw;
	struct {
		uint32_t DCI_DUMMY                       : 32;
	} bits;
};

union REG_ISP_DCI_IMG_WIDTHM1 {
	uint32_t raw;
	struct {
		uint32_t IMG_WIDTHM1                     : 16;
	} bits;
};

union REG_ISP_DCI_LUT_ORDER_SELECT {
	uint32_t raw;
	struct {
		uint32_t DCI_LUT_ORDER_SELECT            : 1;
	} bits;
};

union REG_ISP_DCI_ROI_START {
	uint32_t raw;
	struct {
		uint32_t DCI_ROI_START_X                 : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t DCI_ROI_START_Y                 : 13;
	} bits;
};

union REG_ISP_DCI_ROI_GEO {
	uint32_t raw;
	struct {
		uint32_t DCI_ROI_WIDTHM1                 : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t DCI_ROI_HEIGHTM1                : 13;
	} bits;
};

union REG_ISP_DCI_MAP_DBG {
	uint32_t raw;
	struct {
		uint32_t PROG_HDK_DIS                    : 1;
		uint32_t CONT_EN                         : 1;
		uint32_t SOFTRST                         : 1;
		uint32_t DBG_EN                          : 1;
		uint32_t _rsv_4                          : 12;
		uint32_t CHECK_SUM                       : 16;
	} bits;
};

union REG_ISP_DCI_BAYER_STARTING {
	uint32_t raw;
	struct {
		uint32_t DCI_BAYER_STARTING              : 4;
		uint32_t _rsv_4                          : 12;
		uint32_t FORCE_BAYER_ENABLE              : 1;
	} bits;
};

union REG_ISP_DCI_DMI_ENABLE {
	uint32_t raw;
	struct {
		uint32_t DMI_ENABLE                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t DMI_QOS                         : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t FORCE_DMA_DISABLE               : 1;
	} bits;
};

union REG_ISP_DCI_MAPPED_LUT {
	uint32_t raw;
	struct {
		uint32_t DCI_MAPPED_LUT_LSB              : 10;
	} bits;
};

union REG_ISP_DCI_GAMMA_PROG_CTRL {
	uint32_t raw;
	struct {
		uint32_t GAMMA_WSEL                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t GAMMA_RSEL                      : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t GAMMA_PROG_EN                   : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t GAMMA_PROG_1TO3_EN              : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t GAMMA_PROG_MODE                 : 2;
	} bits;
};

union REG_ISP_DCI_GAMMA_PROG_DATA {
	uint32_t raw;
	struct {
		uint32_t GAMMA_DATA_E                    : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t GAMMA_DATA_O                    : 12;
		uint32_t _rsv_28                         : 3;
		uint32_t GAMMA_W                         : 1;
	} bits;
};

union REG_ISP_DCI_GAMMA_PROG_MAX {
	uint32_t raw;
	struct {
		uint32_t GAMMA_MAX                       : 12;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_HIST_EDGE_V_STATUS {
	uint32_t raw;
	struct {
		uint32_t STATUS                          : 32;
	} bits;
};

union REG_HIST_EDGE_V_SW_CTL {
	uint32_t raw;
	struct {
		uint32_t SW_RESET                        : 1;
		uint32_t CLR_SRAM                        : 1;
		uint32_t _rsv_2                          : 1;
		uint32_t SHAW_SEL                        : 1;
		uint32_t TILE_NM                         : 4;
	} bits;
};

union REG_HIST_EDGE_V_BYPASS {
	uint32_t raw;
	struct {
		uint32_t BYPASS                          : 1;
		uint32_t FORCE_CLK_ENABLE                : 1;
	} bits;
};

union REG_HIST_EDGE_V_IP_CONFIG {
	uint32_t raw;
	struct {
		uint32_t HIST_EDGE_V_ENABLE              : 1;
		uint32_t HIST_EDGE_V_LUMA_MODE           : 1;
	} bits;
};

union REG_HIST_EDGE_V_OFFSETX {
	uint32_t raw;
	struct {
		uint32_t HIST_EDGE_V_OFFSETX             : 13;
	} bits;
};

union REG_HIST_EDGE_V_OFFSETY {
	uint32_t raw;
	struct {
		uint32_t HIST_EDGE_V_OFFSETY             : 13;
	} bits;
};

union REG_HIST_EDGE_V_MONITOR {
	uint32_t raw;
	struct {
		uint32_t MONITOR                         : 32;
	} bits;
};

union REG_HIST_EDGE_V_MONITOR_SELECT {
	uint32_t raw;
	struct {
		uint32_t MONITOR_SEL                     : 32;
	} bits;
};

union REG_HIST_EDGE_V_LOCATION {
	uint32_t raw;
	struct {
		uint32_t LOCATION                        : 32;
	} bits;
};

union REG_HIST_EDGE_V_DUMMY {
	uint32_t raw;
	struct {
		uint32_t DUMMY                           : 16;
	} bits;
};

union REG_HIST_EDGE_V_DMI_ENABLE {
	uint32_t raw;
	struct {
		uint32_t DMI_ENABLE                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t DMI_QOS                         : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t FORCE_DMA_DISABLE               : 1;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_FUSION_FS_CTRL_0 {
	uint32_t raw;
	struct {
		uint32_t FS_ENABLE                       : 1;
		uint32_t FS_DELAY                        : 1;
		uint32_t FORCE_CLK_ENABLE                : 1;
		uint32_t SE_IN_SEL                       : 1;
		uint32_t FS_MC_ENABLE                    : 1;
		uint32_t FS_LUMA_MODE                    : 1;
		uint32_t FORCE_PCLK_ENABLE               : 1;
		uint32_t _rsv_7                          : 1;
		uint32_t FS_OUT_SEL                      : 4;
		uint32_t FS_S_MAX                        : 20;
	} bits;
};

union REG_ISP_FUSION_FS_FRAME_SIZE {
	uint32_t raw;
	struct {
		uint32_t FS_WIDTHM1                      : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t FS_HEIGHTM1                     : 13;
	} bits;
};

union REG_ISP_FUSION_FS_SE_GAIN {
	uint32_t raw;
	struct {
		uint32_t FS_LS_GAIN                      : 14;
	} bits;
};

union REG_ISP_FUSION_FS_LUMA_THD {
	uint32_t raw;
	struct {
		uint32_t FS_LUMA_THD_L                   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t FS_LUMA_THD_H                   : 12;
	} bits;
};

union REG_ISP_FUSION_FS_WGT {
	uint32_t raw;
	struct {
		uint32_t FS_WGT_MAX                      : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t FS_WGT_MIN                      : 9;
	} bits;
};

union REG_ISP_FUSION_FS_WGT_SLOPE {
	uint32_t raw;
	struct {
		uint32_t FS_WGT_SLP                      : 19;
	} bits;
};

union REG_ISP_FUSION_FS_SHDW_READ_SEL {
	uint32_t raw;
	struct {
		uint32_t SHDW_READ_SEL                   : 1;
	} bits;
};

union REG_ISP_FUSION_FS_UP {
	uint32_t raw;
	struct {
		uint32_t FS_UP_REG                       : 1;
	} bits;
};

union REG_ISP_FUSION_FS_MOTION_LUT_IN {
	uint32_t raw;
	struct {
		uint32_t FS_MOTION_LUT_IN_0              : 8;
		uint32_t FS_MOTION_LUT_IN_1              : 8;
		uint32_t FS_MOTION_LUT_IN_2              : 8;
		uint32_t FS_MOTION_LUT_IN_3              : 8;
	} bits;
};

union REG_ISP_FUSION_FS_MOTION_LUT_OUT_0 {
	uint32_t raw;
	struct {
		uint32_t FS_MOTION_LUT_OUT_0             : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t FS_MOTION_LUT_OUT_1             : 9;
	} bits;
};

union REG_ISP_FUSION_FS_MOTION_LUT_OUT_1 {
	uint32_t raw;
	struct {
		uint32_t FS_MOTION_LUT_OUT_2             : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t FS_MOTION_LUT_OUT_3             : 9;
	} bits;
};

union REG_ISP_FUSION_FS_MOTION_LUT_SLOPE_0 {
	uint32_t raw;
	struct {
		uint32_t FS_MOTION_LUT_SLOPE_0           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t FS_MOTION_LUT_SLOPE_1           : 12;
	} bits;
};

union REG_ISP_FUSION_FS_MOTION_LUT_SLOPE_1 {
	uint32_t raw;
	struct {
		uint32_t FS_MOTION_LUT_SLOPE_2           : 12;
	} bits;
};

union REG_ISP_FUSION_FS_CTRL_1 {
	uint32_t raw;
	struct {
		uint32_t FS_REF_LE                       : 1;
		uint32_t _rsv_1                          : 5;
		uint32_t FS_FUSION_TYPE                  : 2;
		uint32_t _rsv_8                          : 8;
		uint32_t FS_FUSION_LWGT                  : 9;
	} bits;
};

union REG_ISP_FUSION_FS_CTRL_2 {
	uint32_t raw;
	struct {
		uint32_t FS_LUMA_WGT                     : 9;
	} bits;
};

union REG_ISP_FUSION_FS_LUMA_THD_SE {
	uint32_t raw;
	struct {
		uint32_t FS_LUMA_THD_L2                  : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t FS_LUMA_THD_H2                  : 12;
	} bits;
};

union REG_ISP_FUSION_FS_WGT_SE {
	uint32_t raw;
	struct {
		uint32_t FS_WGT_MAX2                     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t FS_WGT_MIN2                     : 9;
	} bits;
};

union REG_ISP_FUSION_FS_WGT_SLOPE_SE {
	uint32_t raw;
	struct {
		uint32_t FS_WGT_SLP2                     : 19;
	} bits;
};

union REG_ISP_FUSION_FS_DITHER {
	uint32_t raw;
	struct {
		uint32_t FS_DITHER_ENABLE                : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t FS_DITHER_NBITS                 : 5;
		uint32_t _rsv_9                          : 7;
		uint32_t FS_DITHER_DELTA                 : 12;
	} bits;
};

union REG_ISP_FUSION_FS_CALIB_CTRL_0 {
	uint32_t raw;
	struct {
		uint32_t FS_CALIB_LUMA_LOW_TH            : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t FS_CALIB_LUMA_HIGH_TH           : 12;
	} bits;
};

union REG_ISP_FUSION_FS_CALIB_CTRL_1 {
	uint32_t raw;
	struct {
		uint32_t FS_CALIB_DIF_TH                 : 11;
	} bits;
};

union REG_ISP_FUSION_FS_SE_FIX_OFFSET_0 {
	uint32_t raw;
	struct {
		uint32_t FS_SE_FIX_OFFSET_R              : 21;
	} bits;
};

union REG_ISP_FUSION_FS_SE_FIX_OFFSET_1 {
	uint32_t raw;
	struct {
		uint32_t FS_SE_FIX_OFFSET_G              : 21;
	} bits;
};

union REG_ISP_FUSION_FS_SE_FIX_OFFSET_2 {
	uint32_t raw;
	struct {
		uint32_t FS_SE_FIX_OFFSET_B              : 21;
	} bits;
};

union REG_ISP_FUSION_FS_CALIB_OUT_0 {
	uint32_t raw;
	struct {
		uint32_t FS_CAL_PXL_NUM                  : 20;
	} bits;
};

union REG_ISP_FUSION_FS_CALIB_OUT_1 {
	uint32_t raw;
	struct {
		uint32_t FS_PXL_DIFF_SUM_R               : 32;
	} bits;
};

union REG_ISP_FUSION_FS_CALIB_OUT_2 {
	uint32_t raw;
	struct {
		uint32_t FS_PXL_DIFF_SUM_G               : 32;
	} bits;
};

union REG_ISP_FUSION_FS_CALIB_OUT_3 {
	uint32_t raw;
	struct {
		uint32_t FS_PXL_DIFF_SUM_B               : 32;
	} bits;
};

union REG_ISP_FUSION_FS_CALIB_CTRL_2 {
	uint32_t raw;
	struct {
		uint32_t FS_CALIB_CROP_STR               : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t FS_CALIB_CROP_END               : 12;
		uint32_t FS_CALIB_CROP_EN                : 1;
		uint32_t FS_CALIB_RESET                  : 1;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_LTM_TOP_CTRL {
	uint32_t raw;
	struct {
		uint32_t LTM_ENABLE                      : 1;
		uint32_t DTONE_EHN_EN                    : 1;
		uint32_t BTONE_EHN_EN                    : 1;
		uint32_t DARK_LCE_EN                     : 1;
		uint32_t BRIT_LCE_EN                     : 1;
		uint32_t SHDW_READ_SEL                   : 1;
		uint32_t _rsv_6                          : 2;
		uint32_t BAYER_ID                        : 2;
		uint32_t FORCE_PCLK_ENABLE               : 1;
		uint32_t _rsv_11                         : 1;
		uint32_t DBG_ENABLE                      : 1;
		uint32_t DBG_MODE                        : 3;
		uint32_t DE_MAX_THR                      : 12;
		uint32_t FORCE_DMA_DISABLE               : 1;
		uint32_t LTM_DE_MAX_THR_ENABLE           : 1;
		uint32_t LTM_UP_REG                      : 1;
		uint32_t FORCE_CLK_ENABLE                : 1;
	} bits;
};

union REG_ISP_LTM_BLK_SIZE {
	uint32_t raw;
	struct {
		uint32_t HORZ_BLK_SIZE                   : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t BLK_WIDTHM1                     : 9;
		uint32_t _rsv_13                         : 3;
		uint32_t BLK_HEIGHTM1                    : 9;
		uint32_t _rsv_25                         : 3;
		uint32_t VERT_BLK_SIZE                   : 3;
	} bits;
};

union REG_ISP_LTM_FRAME_SIZE {
	uint32_t raw;
	struct {
		uint32_t FRAME_WIDTHM1                   : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t FRAME_HEIGHTM1                  : 13;
	} bits;
};

union REG_ISP_LTM_BE_STRTH_CTRL {
	uint32_t raw;
	struct {
		uint32_t BE_LMAP_THR                     : 8;
		uint32_t BE_STRTH_DSHFT                  : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t BE_STRTH_GAIN                   : 11;
	} bits;
};

union REG_ISP_LTM_DE_STRTH_CTRL {
	uint32_t raw;
	struct {
		uint32_t DE_LMAP_THR                     : 8;
		uint32_t DE_STRTH_DSHFT                  : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t DE_STRTH_GAIN                   : 11;
	} bits;
};

union REG_ISP_LTM_FILTER_WIN_SIZE_CTRL {
	uint32_t raw;
	struct {
		uint32_t LMAP0_LP_RNG                    : 3;
		uint32_t _rsv_3                          : 5;
		uint32_t LMAP1_LP_RNG                    : 3;
		uint32_t _rsv_11                         : 5;
		uint32_t BE_RNG                          : 4;
		uint32_t _rsv_20                         : 4;
		uint32_t DE_RNG                          : 4;
	} bits;
};

union REG_ISP_LTM_BGAIN_CTRL_0 {
	uint32_t raw;
	struct {
		uint32_t BRI_IN_THD_L                    : 10;
		uint32_t _rsv_10                         : 2;
		uint32_t BRI_IN_THD_H                    : 10;
		uint32_t _rsv_22                         : 2;
		uint32_t BRI_OUT_THD_L                   : 8;
	} bits;
};

union REG_ISP_LTM_BGAIN_CTRL_1 {
	uint32_t raw;
	struct {
		uint32_t BRI_OUT_THD_H                   : 8;
		uint32_t BRI_IN_GAIN_SLOP                : 21;
	} bits;
};

union REG_ISP_LTM_DGAIN_CTRL_0 {
	uint32_t raw;
	struct {
		uint32_t DAR_IN_THD_L                    : 10;
		uint32_t _rsv_10                         : 2;
		uint32_t DAR_IN_THD_H                    : 10;
		uint32_t _rsv_22                         : 2;
		uint32_t DAR_OUT_THD_L                   : 8;
	} bits;
};

union REG_ISP_LTM_DGAIN_CTRL_1 {
	uint32_t raw;
	struct {
		uint32_t DAR_OUT_THD_H                   : 8;
		uint32_t DAR_IN_GAIN_SLOP                : 21;
	} bits;
};

union REG_ISP_LTM_BRI_LCE_CTRL_0 {
	uint32_t raw;
	struct {
		uint32_t BRI_LCE_UP_GAIN_0               : 12;
		uint32_t BRI_LCE_UP_THD_0                : 20;
	} bits;
};

union REG_ISP_LTM_BRI_LCE_CTRL_1 {
	uint32_t raw;
	struct {
		uint32_t BRI_LCE_UP_GAIN_1               : 12;
		uint32_t BRI_LCE_UP_THD_1                : 20;
	} bits;
};

union REG_ISP_LTM_BRI_LCE_CTRL_2 {
	uint32_t raw;
	struct {
		uint32_t BRI_LCE_UP_GAIN_2               : 12;
		uint32_t BRI_LCE_UP_THD_2                : 20;
	} bits;
};

union REG_ISP_LTM_BRI_LCE_CTRL_3 {
	uint32_t raw;
	struct {
		uint32_t BRI_LCE_UP_GAIN_3               : 12;
		uint32_t BRI_LCE_UP_THD_3                : 20;
	} bits;
};

union REG_ISP_LTM_BRI_LCE_CTRL_4 {
	uint32_t raw;
	struct {
		uint32_t BRI_LCE_UP_GAIN_SLOPE_0         : 25;
	} bits;
};

union REG_ISP_LTM_BRI_LCE_CTRL_5 {
	uint32_t raw;
	struct {
		uint32_t BRI_LCE_UP_GAIN_SLOPE_1         : 25;
	} bits;
};

union REG_ISP_LTM_BRI_LCE_CTRL_6 {
	uint32_t raw;
	struct {
		uint32_t BRI_LCE_UP_GAIN_SLOPE_2         : 25;
	} bits;
};

union REG_ISP_LTM_BRI_LCE_CTRL_7 {
	uint32_t raw;
	struct {
		uint32_t BRI_LCE_DOWN_GAIN_0             : 12;
		uint32_t BRI_LCE_DOWN_THD_0              : 20;
	} bits;
};

union REG_ISP_LTM_BRI_LCE_CTRL_8 {
	uint32_t raw;
	struct {
		uint32_t BRI_LCE_DOWN_GAIN_1             : 12;
		uint32_t BRI_LCE_DOWN_THD_1              : 20;
	} bits;
};

union REG_ISP_LTM_BRI_LCE_CTRL_9 {
	uint32_t raw;
	struct {
		uint32_t BRI_LCE_DOWN_GAIN_2             : 12;
		uint32_t BRI_LCE_DOWN_THD_2              : 20;
	} bits;
};

union REG_ISP_LTM_BRI_LCE_CTRL_10 {
	uint32_t raw;
	struct {
		uint32_t BRI_LCE_DOWN_GAIN_3             : 12;
		uint32_t BRI_LCE_DOWN_THD_3              : 20;
	} bits;
};

union REG_ISP_LTM_BRI_LCE_CTRL_11 {
	uint32_t raw;
	struct {
		uint32_t BRI_LCE_DOWN_GAIN_SLOPE_0       : 25;
	} bits;
};

union REG_ISP_LTM_BRI_LCE_CTRL_12 {
	uint32_t raw;
	struct {
		uint32_t BRI_LCE_DOWN_GAIN_SLOPE_1       : 25;
	} bits;
};

union REG_ISP_LTM_BRI_LCE_CTRL_13 {
	uint32_t raw;
	struct {
		uint32_t BRI_LCE_DOWN_GAIN_SLOPE_2       : 25;
	} bits;
};

union REG_ISP_LTM_DAR_LCE_CTRL_0 {
	uint32_t raw;
	struct {
		uint32_t DAR_LCE_GAIN_0                  : 12;
		uint32_t DAR_LCE_DIFF_THD_0              : 20;
	} bits;
};

union REG_ISP_LTM_DAR_LCE_CTRL_1 {
	uint32_t raw;
	struct {
		uint32_t DAR_LCE_GAIN_1                  : 12;
		uint32_t DAR_LCE_DIFF_THD_1              : 20;
	} bits;
};

union REG_ISP_LTM_DAR_LCE_CTRL_2 {
	uint32_t raw;
	struct {
		uint32_t DAR_LCE_GAIN_2                  : 12;
		uint32_t DAR_LCE_DIFF_THD_2              : 20;
	} bits;
};

union REG_ISP_LTM_DAR_LCE_CTRL_3 {
	uint32_t raw;
	struct {
		uint32_t DAR_LCE_GAIN_3                  : 12;
		uint32_t DAR_LCE_DIFF_THD_3              : 20;
	} bits;
};

union REG_ISP_LTM_DAR_LCE_CTRL_4 {
	uint32_t raw;
	struct {
		uint32_t DAR_LCE_GAIN_SLOPE_0            : 17;
	} bits;
};

union REG_ISP_LTM_DAR_LCE_CTRL_5 {
	uint32_t raw;
	struct {
		uint32_t DAR_LCE_GAIN_SLOPE_1            : 17;
	} bits;
};

union REG_ISP_LTM_DAR_LCE_CTRL_6 {
	uint32_t raw;
	struct {
		uint32_t DAR_LCE_GAIN_SLOPE_2            : 17;
	} bits;
};

union REG_ISP_LTM_CURVE_QUAN_BIT {
	uint32_t raw;
	struct {
		uint32_t DCRV_QUAN_BIT                   : 4;
		uint32_t BCRV_QUAN_BIT                   : 4;
		uint32_t GCRV_QUAN_BIT_0                 : 4;
		uint32_t GCRV_QUAN_BIT_1                 : 4;
	} bits;
};

union REG_ISP_LTM_LMAP0_LP_DIST_WGT_0 {
	uint32_t raw;
	struct {
		uint32_t DARK_LPF_DIST_WGT_00            : 5;
		uint32_t DARK_LPF_DIST_WGT_01            : 5;
		uint32_t DARK_LPF_DIST_WGT_02            : 5;
		uint32_t DARK_LPF_DIST_WGT_03            : 5;
		uint32_t DARK_LPF_DIST_WGT_04            : 5;
		uint32_t DARK_LPF_DIST_WGT_05            : 5;
	} bits;
};

union REG_ISP_LTM_LMAP0_LP_DIST_WGT_1 {
	uint32_t raw;
	struct {
		uint32_t DARK_LPF_DIST_WGT_06            : 5;
		uint32_t DARK_LPF_DIST_WGT_07            : 5;
		uint32_t DARK_LPF_DIST_WGT_08            : 5;
		uint32_t DARK_LPF_DIST_WGT_09            : 5;
		uint32_t DARK_LPF_DIST_WGT_10            : 5;
	} bits;
};

union REG_ISP_LTM_LMAP0_LP_DIFF_WGT_0 {
	uint32_t raw;
	struct {
		uint32_t DARK_LPF_DIFF_WGT_00            : 5;
		uint32_t DARK_LPF_DIFF_WGT_01            : 5;
		uint32_t DARK_LPF_DIFF_WGT_02            : 5;
		uint32_t DARK_LPF_DIFF_WGT_03            : 5;
		uint32_t DARK_LPF_DIFF_WGT_04            : 5;
		uint32_t DARK_LPF_DIFF_WGT_05            : 5;
	} bits;
};

union REG_ISP_LTM_LMAP0_LP_DIFF_WGT_1 {
	uint32_t raw;
	struct {
		uint32_t DARK_LPF_DIFF_WGT_06            : 5;
		uint32_t DARK_LPF_DIFF_WGT_07            : 5;
		uint32_t DARK_LPF_DIFF_WGT_08            : 5;
		uint32_t DARK_LPF_DIFF_WGT_09            : 5;
		uint32_t DARK_LPF_DIFF_WGT_10            : 5;
		uint32_t DARK_LPF_DIFF_WGT_11            : 5;
	} bits;
};

union REG_ISP_LTM_LMAP0_LP_DIFF_WGT_2 {
	uint32_t raw;
	struct {
		uint32_t DARK_LPF_DIFF_WGT_12            : 5;
		uint32_t DARK_LPF_DIFF_WGT_13            : 5;
		uint32_t DARK_LPF_DIFF_WGT_14            : 5;
		uint32_t DARK_LPF_DIFF_WGT_15            : 5;
		uint32_t DARK_LPF_DIFF_WGT_16            : 5;
		uint32_t DARK_LPF_DIFF_WGT_17            : 5;
	} bits;
};

union REG_ISP_LTM_LMAP0_LP_DIFF_WGT_3 {
	uint32_t raw;
	struct {
		uint32_t DARK_LPF_DIFF_WGT_18            : 5;
		uint32_t DARK_LPF_DIFF_WGT_19            : 5;
		uint32_t DARK_LPF_DIFF_WGT_20            : 5;
		uint32_t DARK_LPF_DIFF_WGT_21            : 5;
		uint32_t DARK_LPF_DIFF_WGT_22            : 5;
		uint32_t DARK_LPF_DIFF_WGT_23            : 5;
	} bits;
};

union REG_ISP_LTM_LMAP0_LP_DIFF_WGT_4 {
	uint32_t raw;
	struct {
		uint32_t DARK_LPF_DIFF_WGT_24            : 5;
		uint32_t DARK_LPF_DIFF_WGT_25            : 5;
		uint32_t DARK_LPF_DIFF_WGT_26            : 5;
		uint32_t DARK_LPF_DIFF_WGT_27            : 5;
		uint32_t DARK_LPF_DIFF_WGT_28            : 5;
		uint32_t DARK_LPF_DIFF_WGT_29            : 5;
	} bits;
};

union REG_ISP_LTM_LMAP1_LP_DIST_WGT_0 {
	uint32_t raw;
	struct {
		uint32_t BRIT_LPF_DIST_WGT_00            : 5;
		uint32_t BRIT_LPF_DIST_WGT_01            : 5;
		uint32_t BRIT_LPF_DIST_WGT_02            : 5;
		uint32_t BRIT_LPF_DIST_WGT_03            : 5;
		uint32_t BRIT_LPF_DIST_WGT_04            : 5;
		uint32_t BRIT_LPF_DIST_WGT_05            : 5;
	} bits;
};

union REG_ISP_LTM_LMAP1_LP_DIST_WGT_1 {
	uint32_t raw;
	struct {
		uint32_t BRIT_LPF_DIST_WGT_06            : 5;
		uint32_t BRIT_LPF_DIST_WGT_07            : 5;
		uint32_t BRIT_LPF_DIST_WGT_08            : 5;
		uint32_t BRIT_LPF_DIST_WGT_09            : 5;
		uint32_t BRIT_LPF_DIST_WGT_10            : 5;
	} bits;
};

union REG_ISP_LTM_LMAP1_LP_DIFF_WGT_0 {
	uint32_t raw;
	struct {
		uint32_t BRIT_LPF_DIFF_WGT_00            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_01            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_02            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_03            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_04            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_05            : 5;
	} bits;
};

union REG_ISP_LTM_LMAP1_LP_DIFF_WGT_1 {
	uint32_t raw;
	struct {
		uint32_t BRIT_LPF_DIFF_WGT_06            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_07            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_08            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_09            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_10            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_11            : 5;
	} bits;
};

union REG_ISP_LTM_LMAP1_LP_DIFF_WGT_2 {
	uint32_t raw;
	struct {
		uint32_t BRIT_LPF_DIFF_WGT_12            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_13            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_14            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_15            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_16            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_17            : 5;
	} bits;
};

union REG_ISP_LTM_LMAP1_LP_DIFF_WGT_3 {
	uint32_t raw;
	struct {
		uint32_t BRIT_LPF_DIFF_WGT_18            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_19            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_20            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_21            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_22            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_23            : 5;
	} bits;
};

union REG_ISP_LTM_LMAP1_LP_DIFF_WGT_4 {
	uint32_t raw;
	struct {
		uint32_t BRIT_LPF_DIFF_WGT_24            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_25            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_26            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_27            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_28            : 5;
		uint32_t BRIT_LPF_DIFF_WGT_29            : 5;
	} bits;
};

union REG_ISP_LTM_BE_DIST_WGT_0 {
	uint32_t raw;
	struct {
		uint32_t BE_DIST_WGT_00                  : 5;
		uint32_t BE_DIST_WGT_01                  : 5;
		uint32_t BE_DIST_WGT_02                  : 5;
		uint32_t BE_DIST_WGT_03                  : 5;
		uint32_t BE_DIST_WGT_04                  : 5;
		uint32_t BE_DIST_WGT_05                  : 5;
	} bits;
};

union REG_ISP_LTM_BE_DIST_WGT_1 {
	uint32_t raw;
	struct {
		uint32_t BE_DIST_WGT_06                  : 5;
		uint32_t BE_DIST_WGT_07                  : 5;
		uint32_t BE_DIST_WGT_08                  : 5;
		uint32_t BE_DIST_WGT_09                  : 5;
		uint32_t BE_DIST_WGT_10                  : 5;
	} bits;
};

union REG_ISP_LTM_DE_DIST_WGT_0 {
	uint32_t raw;
	struct {
		uint32_t DE_DIST_WGT_00                  : 5;
		uint32_t DE_DIST_WGT_01                  : 5;
		uint32_t DE_DIST_WGT_02                  : 5;
		uint32_t DE_DIST_WGT_03                  : 5;
		uint32_t DE_DIST_WGT_04                  : 5;
		uint32_t DE_DIST_WGT_05                  : 5;
	} bits;
};

union REG_ISP_LTM_DE_DIST_WGT_1 {
	uint32_t raw;
	struct {
		uint32_t DE_DIST_WGT_06                  : 5;
		uint32_t DE_DIST_WGT_07                  : 5;
		uint32_t DE_DIST_WGT_08                  : 5;
		uint32_t DE_DIST_WGT_09                  : 5;
		uint32_t DE_DIST_WGT_10                  : 5;
	} bits;
};

union REG_ISP_LTM_DE_LUMA_WGT_0 {
	uint32_t raw;
	struct {
		uint32_t DE_LUMA_WGT_00                  : 5;
		uint32_t DE_LUMA_WGT_01                  : 5;
		uint32_t DE_LUMA_WGT_02                  : 5;
		uint32_t DE_LUMA_WGT_03                  : 5;
		uint32_t DE_LUMA_WGT_04                  : 5;
		uint32_t DE_LUMA_WGT_05                  : 5;
	} bits;
};

union REG_ISP_LTM_DE_LUMA_WGT_1 {
	uint32_t raw;
	struct {
		uint32_t DE_LUMA_WGT_06                  : 5;
		uint32_t DE_LUMA_WGT_07                  : 5;
		uint32_t DE_LUMA_WGT_08                  : 5;
		uint32_t DE_LUMA_WGT_09                  : 5;
		uint32_t DE_LUMA_WGT_10                  : 5;
		uint32_t DE_LUMA_WGT_11                  : 5;
	} bits;
};

union REG_ISP_LTM_DE_LUMA_WGT_2 {
	uint32_t raw;
	struct {
		uint32_t DE_LUMA_WGT_12                  : 5;
		uint32_t DE_LUMA_WGT_13                  : 5;
		uint32_t DE_LUMA_WGT_14                  : 5;
		uint32_t DE_LUMA_WGT_15                  : 5;
		uint32_t DE_LUMA_WGT_16                  : 5;
		uint32_t DE_LUMA_WGT_17                  : 5;
	} bits;
};

union REG_ISP_LTM_DE_LUMA_WGT_3 {
	uint32_t raw;
	struct {
		uint32_t DE_LUMA_WGT_18                  : 5;
		uint32_t DE_LUMA_WGT_19                  : 5;
		uint32_t DE_LUMA_WGT_20                  : 5;
		uint32_t DE_LUMA_WGT_21                  : 5;
		uint32_t DE_LUMA_WGT_22                  : 5;
		uint32_t DE_LUMA_WGT_23                  : 5;
	} bits;
};

union REG_ISP_LTM_DE_LUMA_WGT_4 {
	uint32_t raw;
	struct {
		uint32_t DE_LUMA_WGT_24                  : 5;
		uint32_t DE_LUMA_WGT_25                  : 5;
		uint32_t DE_LUMA_WGT_26                  : 5;
		uint32_t DE_LUMA_WGT_27                  : 5;
		uint32_t DE_LUMA_WGT_28                  : 5;
		uint32_t DE_LUMA_WGT_29                  : 5;
	} bits;
};

union REG_ISP_LTM_DTONE_CURVE_PROG_DATA {
	uint32_t raw;
	struct {
		uint32_t DTONE_CURVE_DATA_E              : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t DTONE_CURVE_DATA_O              : 12;
		uint32_t _rsv_28                         : 3;
		uint32_t DTONE_CURVE_W                   : 1;
	} bits;
};

union REG_ISP_LTM_DTONE_CURVE_PROG_CTRL {
	uint32_t raw;
	struct {
		uint32_t DTONE_CURVE_WSEL                : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t DTONE_CURVE_RSEL                : 1;
	} bits;
};

union REG_ISP_LTM_DTONE_CURVE_PROG_MAX {
	uint32_t raw;
	struct {
		uint32_t DTONE_CURVE_MAX                 : 12;
	} bits;
};

union REG_ISP_LTM_DTONE_CURVE_CTRL {
	uint32_t raw;
	struct {
		uint32_t DTONE_CURVE_ADDR_RST            : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t DTONE_CURVE_PROG_HDK_DIS        : 1;
	} bits;
};

union REG_ISP_LTM_DTONE_CURVE_MEM_SW_MODE {
	uint32_t raw;
	struct {
		uint32_t DTONE_CURVE_MEM_SW_MODE         : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t DTONE_CURVE_MEM_SEL             : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t DTONE_CURVE_INS_MEM_SEL         : 1;
	} bits;
};

union REG_ISP_LTM_DTONE_CURVE_MEM_SW_RADDR {
	uint32_t raw;
	struct {
		uint32_t DTONE_CURVE_SW_RADDR            : 8;
	} bits;
};

union REG_ISP_LTM_DTONE_CURVE_MEM_SW_RDATA {
	uint32_t raw;
	struct {
		uint32_t DTONE_CURVE_RDATA               : 12;
		uint32_t _rsv_12                         : 19;
		uint32_t DTONE_CURVE_SW_RD               : 1;
	} bits;
};

union REG_ISP_LTM_BTONE_CURVE_PROG_DATA {
	uint32_t raw;
	struct {
		uint32_t BTONE_CURVE_DATA_E              : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t BTONE_CURVE_DATA_O              : 12;
		uint32_t _rsv_28                         : 3;
		uint32_t BTONE_CURVE_W                   : 1;
	} bits;
};

union REG_ISP_LTM_BTONE_CURVE_PROG_CTRL {
	uint32_t raw;
	struct {
		uint32_t BTONE_CURVE_WSEL                : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t BTONE_CURVE_RSEL                : 1;
	} bits;
};

union REG_ISP_LTM_BTONE_CURVE_PROG_MAX {
	uint32_t raw;
	struct {
		uint32_t BTONE_CURVE_MAX                 : 12;
	} bits;
};

union REG_ISP_LTM_BTONE_CURVE_CTRL {
	uint32_t raw;
	struct {
		uint32_t BTONE_CURVE_ADDR_RST            : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t BTONE_CURVE_PROG_HDK_DIS        : 1;
	} bits;
};

union REG_ISP_LTM_BTONE_CURVE_MEM_SW_MODE {
	uint32_t raw;
	struct {
		uint32_t BTONE_CURVE_MEM_SW_MODE         : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t BTONE_CURVE_MEM_SEL             : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t BTONE_CURVE_INS_MEM_SEL         : 1;
	} bits;
};

union REG_ISP_LTM_BTONE_CURVE_MEM_SW_RADDR {
	uint32_t raw;
	struct {
		uint32_t BTONE_CURVE_SW_RADDR            : 9;
	} bits;
};

union REG_ISP_LTM_BTONE_CURVE_MEM_SW_RDATA {
	uint32_t raw;
	struct {
		uint32_t BTONE_CURVE_RDATA               : 12;
		uint32_t _rsv_12                         : 19;
		uint32_t BTONE_CURVE_SW_RD               : 1;
	} bits;
};

union REG_ISP_LTM_GLOBAL_CURVE_PROG_DATA {
	uint32_t raw;
	struct {
		uint32_t GLOBAL_CURVE_DATA_E             : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t GLOBAL_CURVE_DATA_O             : 12;
		uint32_t _rsv_28                         : 3;
		uint32_t GLOBAL_CURVE_W                  : 1;
	} bits;
};

union REG_ISP_LTM_GLOBAL_CURVE_PROG_CTRL {
	uint32_t raw;
	struct {
		uint32_t GLOBAL_CURVE_WSEL               : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t GLOBAL_CURVE_RSEL               : 1;
	} bits;
};

union REG_ISP_LTM_GLOBAL_CURVE_PROG_MAX {
	uint32_t raw;
	struct {
		uint32_t GLOBAL_CURVE_MAX                : 12;
	} bits;
};

union REG_ISP_LTM_GLOBAL_CURVE_CTRL {
	uint32_t raw;
	struct {
		uint32_t GLOBAL_CURVE_ADDR_RST           : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t GLOBAL_CURVE_PROG_HDK_DIS       : 1;
	} bits;
};

union REG_ISP_LTM_GLOBAL_CURVE_MEM_SW_MODE {
	uint32_t raw;
	struct {
		uint32_t GLOBAL_CURVE_MEM_SW_MODE        : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t GLOBAL_CURVE_MEM_SEL            : 1;
	} bits;
};

union REG_ISP_LTM_GLOBAL_CURVE_MEM_SW_RADDR {
	uint32_t raw;
	struct {
		uint32_t GLOBAL_CURVE_SW_RADDR           : 10;
	} bits;
};

union REG_ISP_LTM_GLOBAL_CURVE_MEM_SW_RDATA {
	uint32_t raw;
	struct {
		uint32_t GLOBAL_CURVE_RDATA              : 12;
		uint32_t _rsv_12                         : 19;
		uint32_t GLOBAL_CURVE_SW_RD              : 1;
	} bits;
};

union REG_ISP_LTM_RESIZE_COEFF_PROG_CTRL {
	uint32_t raw;
	struct {
		uint32_t RESIZE_COEFF_BUFF_ADDR          : 7;
		uint32_t _rsv_7                          : 24;
		uint32_t RESIZE_COEFF_BUFF_WEN           : 1;
	} bits;
};

union REG_ISP_LTM_RESIZE_COEFF_WDATA_0 {
	uint32_t raw;
	struct {
		uint32_t RESIZE_COEFF_BUFF_WDATA_0       : 26;
	} bits;
};

union REG_ISP_LTM_RESIZE_COEFF_WDATA_1 {
	uint32_t raw;
	struct {
		uint32_t RESIZE_COEFF_BUFF_WDATA_1       : 26;
	} bits;
};

union REG_ISP_LTM_TILE_MODE_CTRL_0 {
	uint32_t raw;
	struct {
		uint32_t DMA_BLK_CROP_STR                : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t DMA_BLK_CROP_END                : 9;
		uint32_t _rsv_25                         : 6;
		uint32_t DMA_BLK_CROP_EN                 : 1;
	} bits;
};

union REG_ISP_LTM_TILE_MODE_CTRL_1 {
	uint32_t raw;
	struct {
		uint32_t BLK_WIN_CROP_STR                : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t BLK_WIN_CROP_END                : 9;
		uint32_t _rsv_25                         : 6;
		uint32_t BLK_WIN_CROP_EN                 : 1;
	} bits;
};

union REG_ISP_LTM_TILE_MODE_CTRL_2 {
	uint32_t raw;
	struct {
		uint32_t RS_OUT_CROP_STR                 : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t RS_OUT_CROP_END                 : 13;
		uint32_t _rsv_29                         : 2;
		uint32_t RS_OUT_CROP_EN                  : 1;
	} bits;
};

union REG_ISP_LTM_DUMMY {
	uint32_t raw;
	struct {
		uint32_t DUMMY_RW                        : 16;
		uint32_t DUMMY_RO                        : 16;
	} bits;
};

union REG_ISP_LTM_LMAP_COMPUTE_CTRL_0 {
	uint32_t raw;
	struct {
		uint32_t LTM_LMAP_LUMA_LE_SEL            : 1;
		uint32_t LTM_LMAP_LUMA_SE_SEL            : 1;
		uint32_t LTM_LMAP_LUMA_MODE              : 1;
		uint32_t _rsv_3                          : 1;
		uint32_t LTM_TONE_STR_LE_SEL             : 1;
		uint32_t LTM_TONE_STR_SE_SEL             : 1;
		uint32_t LTM_TONE_STR_MODE               : 1;
		uint32_t _rsv_7                          : 1;
		uint32_t LTM_LMAP_ALPHA                  : 9;
	} bits;
};

union REG_ISP_LTM_EE_CTRL_0 {
	uint32_t raw;
	struct {
		uint32_t LTM_EE_ENABLE                   : 1;
		uint32_t LTM_EE_DETAIL_MODE              : 1;
		uint32_t LTM_GTM_MODE                    : 1;
	} bits;
};

union REG_ISP_LTM_EE_CTRL_1 {
	uint32_t raw;
	struct {
		uint32_t LTM_EE_CLIP_MIN                 : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t LTM_EE_CLIP_MAX                 : 13;
	} bits;
};

union REG_ISP_LTM_EE_DETAIL_LUT_00 {
	uint32_t raw;
	struct {
		uint32_t LTM_EE_DETAIL_GAIN_LUT_00       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_01       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_02       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_03       : 8;
	} bits;
};

union REG_ISP_LTM_EE_DETAIL_LUT_01 {
	uint32_t raw;
	struct {
		uint32_t LTM_EE_DETAIL_GAIN_LUT_04       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_05       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_06       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_07       : 8;
	} bits;
};

union REG_ISP_LTM_EE_DETAIL_LUT_02 {
	uint32_t raw;
	struct {
		uint32_t LTM_EE_DETAIL_GAIN_LUT_08       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_09       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_10       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_11       : 8;
	} bits;
};

union REG_ISP_LTM_EE_DETAIL_LUT_03 {
	uint32_t raw;
	struct {
		uint32_t LTM_EE_DETAIL_GAIN_LUT_12       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_13       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_14       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_15       : 8;
	} bits;
};

union REG_ISP_LTM_EE_DETAIL_LUT_04 {
	uint32_t raw;
	struct {
		uint32_t LTM_EE_DETAIL_GAIN_LUT_16       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_17       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_18       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_19       : 8;
	} bits;
};

union REG_ISP_LTM_EE_DETAIL_LUT_05 {
	uint32_t raw;
	struct {
		uint32_t LTM_EE_DETAIL_GAIN_LUT_20       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_21       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_22       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_23       : 8;
	} bits;
};

union REG_ISP_LTM_EE_DETAIL_LUT_06 {
	uint32_t raw;
	struct {
		uint32_t LTM_EE_DETAIL_GAIN_LUT_24       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_25       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_26       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_27       : 8;
	} bits;
};

union REG_ISP_LTM_EE_DETAIL_LUT_07 {
	uint32_t raw;
	struct {
		uint32_t LTM_EE_DETAIL_GAIN_LUT_28       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_29       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_30       : 8;
		uint32_t LTM_EE_DETAIL_GAIN_LUT_31       : 8;
	} bits;
};

union REG_ISP_LTM_EE_DETAIL_LUT_08 {
	uint32_t raw;
	struct {
		uint32_t LTM_EE_DETAIL_GAIN_LUT_32       : 8;
	} bits;
};

union REG_ISP_LTM_EE_GAIN_LUT_00 {
	uint32_t raw;
	struct {
		uint32_t LTM_EE_LUMA_GAIN_LUT_00         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_01         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_02         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_03         : 8;
	} bits;
};

union REG_ISP_LTM_EE_GAIN_LUT_01 {
	uint32_t raw;
	struct {
		uint32_t LTM_EE_LUMA_GAIN_LUT_04         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_05         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_06         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_07         : 8;
	} bits;
};

union REG_ISP_LTM_EE_GAIN_LUT_02 {
	uint32_t raw;
	struct {
		uint32_t LTM_EE_LUMA_GAIN_LUT_08         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_09         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_10         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_11         : 8;
	} bits;
};

union REG_ISP_LTM_EE_GAIN_LUT_03 {
	uint32_t raw;
	struct {
		uint32_t LTM_EE_LUMA_GAIN_LUT_12         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_13         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_14         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_15         : 8;
	} bits;
};

union REG_ISP_LTM_EE_GAIN_LUT_04 {
	uint32_t raw;
	struct {
		uint32_t LTM_EE_LUMA_GAIN_LUT_16         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_17         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_18         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_19         : 8;
	} bits;
};

union REG_ISP_LTM_EE_GAIN_LUT_05 {
	uint32_t raw;
	struct {
		uint32_t LTM_EE_LUMA_GAIN_LUT_20         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_21         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_22         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_23         : 8;
	} bits;
};

union REG_ISP_LTM_EE_GAIN_LUT_06 {
	uint32_t raw;
	struct {
		uint32_t LTM_EE_LUMA_GAIN_LUT_24         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_25         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_26         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_27         : 8;
	} bits;
};

union REG_ISP_LTM_EE_GAIN_LUT_07 {
	uint32_t raw;
	struct {
		uint32_t LTM_EE_LUMA_GAIN_LUT_28         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_29         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_30         : 8;
		uint32_t LTM_EE_LUMA_GAIN_LUT_31         : 8;
	} bits;
};

union REG_ISP_LTM_EE_GAIN_LUT_08 {
	uint32_t raw;
	struct {
		uint32_t LTM_EE_LUMA_GAIN_LUT_32         : 8;
	} bits;
};

union REG_ISP_LTM_SAT_ADJ {
	uint32_t raw;
	struct {
		uint32_t LTM_HSV_STUNE_ENABLE            : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t LTM_HSV_SAT_STRENGTH            : 8;
	} bits;
};

union REG_ISP_LTM_HSV_S_BY_V_0 {
	uint32_t raw;
	struct {
		uint32_t LTM_HSV_S_BY_V_LUT_IN_0         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t LTM_HSV_S_BY_V_LUT_IN_1         : 12;
	} bits;
};

union REG_ISP_LTM_HSV_S_BY_V_1 {
	uint32_t raw;
	struct {
		uint32_t LTM_HSV_S_BY_V_LUT_IN_2         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t LTM_HSV_S_BY_V_LUT_IN_3         : 12;
	} bits;
};

union REG_ISP_LTM_HSV_S_BY_V_2 {
	uint32_t raw;
	struct {
		uint32_t LTM_HSV_S_BY_V_LUT_OUT_0        : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t LTM_HSV_S_BY_V_LUT_OUT_1        : 11;
	} bits;
};

union REG_ISP_LTM_HSV_S_BY_V_3 {
	uint32_t raw;
	struct {
		uint32_t LTM_HSV_S_BY_V_LUT_OUT_2        : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t LTM_HSV_S_BY_V_LUT_OUT_3        : 11;
	} bits;
};

union REG_ISP_LTM_HSV_S_BY_V_4 {
	uint32_t raw;
	struct {
		uint32_t LTM_HSV_S_BY_V_LUT_SLOPE_0      : 16;
		uint32_t LTM_HSV_S_BY_V_LUT_SLOPE_1      : 16;
	} bits;
};

union REG_ISP_LTM_HSV_S_BY_V_5 {
	uint32_t raw;
	struct {
		uint32_t LTM_HSV_S_BY_V_LUT_SLOPE_2      : 16;
	} bits;
};

union REG_ISP_LTM_STR_CTRL_0 {
	uint32_t raw;
	struct {
		uint32_t LTM_DARK_TONE_STR_GAIN          : 10;
		uint32_t LTM_DARK_TONE_STR_SHFT          : 5;
		uint32_t _rsv_15                         : 1;
		uint32_t LTM_BRIT_TONE_STR_GAIN          : 10;
		uint32_t LTM_BRIT_TONE_STR_SHFT          : 5;
	} bits;
};

union REG_ISP_LTM_STR_CTRL_1 {
	uint32_t raw;
	struct {
		uint32_t LTM_TONE_STR_ALPHA              : 9;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_YUV_TOP_YUV_0 {
	uint32_t raw;
	struct {
		uint32_t OP_START                        : 1;
		uint32_t CONT_EN                         : 1;
		uint32_t _rsv_2                          : 1;
		uint32_t SOFTRST                         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t YUV_TOP_SEL                     : 1;
	} bits;
};

union REG_YUV_TOP_YUV_1 {
	uint32_t raw;
	struct {
		uint32_t CG_444_422_EN                   : 1;
		uint32_t CS_EN                           : 1;
	} bits;
};

union REG_YUV_TOP_YUV_2 {
	uint32_t raw;
	struct {
		uint32_t FD_INT                          : 1;
		uint32_t DMA_INT                         : 1;
		uint32_t FRAME_OVERFLOW                  : 1;
	} bits;
};

union REG_YUV_TOP_YUV_3 {
	uint32_t raw;
	struct {
		uint32_t DISABLE_DO                      : 1;
		uint32_t DISABLE_DMAO                    : 1;
		uint32_t Y42_SEL                         : 1;
	} bits;
};

union REG_YUV_TOP_YUV_DEBUG_0 {
	uint32_t raw;
	struct {
		uint32_t DEBUG_BUS                       : 32;
	} bits;
};

union REG_YUV_TOP_YUV_4 {
	uint32_t raw;
	struct {
		uint32_t DUMMY                           : 32;
	} bits;
};

union REG_YUV_TOP_YUV_DEBUG_STATE {
	uint32_t raw;
	struct {
		uint32_t _IP_DMA_IDLE                    : 1;
		uint32_t MA_IDLE_YCROP                   : 1;
		uint32_t MA_IDLE_UCROP                   : 1;
		uint32_t MA_IDLE_VCROP                   : 1;
		uint32_t IP_DMA_IDLE                     : 1;
		uint32_t OR_IP_DMA_IDLE                  : 1;
		uint32_t IDLE                            : 1;
	} bits;
};

union REG_YUV_TOP_YUV_5 {
	uint32_t raw;
	struct {
		uint32_t DIS_UV2DRAM                     : 1;
		uint32_t LINE_THRES_EN                   : 1;
		uint32_t _rsv_2                          : 6;
		uint32_t LINE_THRES                      : 12;
		uint32_t UP_PQ_EN                        : 1;
		uint32_t UV_ORDER                        : 1;
	} bits;
};

union REG_YUV_TOP_HSV_LUT_PROG_SRAM0 {
	uint32_t raw;
	struct {
		uint32_t SRAM_WDATA_LSB                  : 32;
	} bits;
};

union REG_YUV_TOP_HSV_LUT_PROG_SRAM1 {
	uint32_t raw;
	struct {
		uint32_t SRAM_WDATA_MSB                  : 2;
		uint32_t SRAM_H_IDX                      : 5;
		uint32_t SRAM_S_IDX                      : 4;
		uint32_t SRAM_V_IDX                      : 4;
		uint32_t SRAM_WR                         : 1;
		uint32_t SRAM_RD                         : 1;
	} bits;
};

union REG_YUV_TOP_HSV_LUT_READ_SRAM0 {
	uint32_t raw;
	struct {
		uint32_t SRAM_RDATA_LSB                  : 32;
	} bits;
};

union REG_YUV_TOP_HSV_LUT_READ_SRAM1 {
	uint32_t raw;
	struct {
		uint32_t SRAM_RDATA_MSB                  : 2;
	} bits;
};

union REG_YUV_TOP_HSV_LUT_CONTROL {
	uint32_t raw;
	struct {
		uint32_t HSV3DLUT_ENABLE                 : 1;
		uint32_t HSV3DLUT_H_CLAMP_WRAP_OPT       : 1;
	} bits;
};

union REG_YUV_TOP_HSV_LUT_STATUS {
	uint32_t raw;
	struct {
		uint32_t CLR_STATUS_HSV_LUT_WR_ERR       : 1;
		uint32_t STATUS_HSV_LUT_WR_ERR           : 1;
	} bits;
};

union REG_YUV_TOP_HSV_ENABLE {
	uint32_t raw;
	struct {
		uint32_t HSV_ENABLE                      : 1;
		uint32_t SC_DMA_SWITCH                   : 1;
		uint32_t AVG_MODE                        : 1;
		uint32_t BYPASS_H                        : 1;
		uint32_t BYPASS_V                        : 1;
		uint32_t DROP_MODE                       : 1;
		uint32_t _rsv_6                          : 2;
		uint32_t GUARD_CNT                       : 8;
		uint32_t DITH_EN                         : 1;
		uint32_t DITH_MD                         : 3;
	} bits;
};

union REG_YUV_TOP_IMGW_M1 {
	uint32_t raw;
	struct {
		uint32_t YUV_TOP_IMGW_M1                 : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t YUV_TOP_IMGH_M1                 : 13;
	} bits;
};

union REG_YUV_TOP_IMGW_M1_CROP {
	uint32_t raw;
	struct {
		uint32_t YUV_TOP_IMGW_M1_CROP            : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t YUV_TOP_IMGH_M1_CROP            : 13;
	} bits;
};

union REG_YUV_TOP_STVALID_STATUS {
	uint32_t raw;
	struct {
		uint32_t STVALID_STATUS                  : 14;
	} bits;
};

union REG_YUV_TOP_STREADY_STATUS {
	uint32_t raw;
	struct {
		uint32_t STREADY_STATUS                  : 14;
	} bits;
};

union REG_YUV_TOP_CA_LITE_ENABLE {
	uint32_t raw;
	struct {
		uint32_t CA_LITE_ENABLE                  : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t CA_LITE_LUT_IN_0                : 9;
		uint32_t _rsv_13                         : 3;
		uint32_t CA_LITE_LUT_IN_1                : 9;
	} bits;
};

union REG_YUV_TOP_CA_LITE_LUT_IN {
	uint32_t raw;
	struct {
		uint32_t CA_LITE_LUT_IN_2                : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t CA_LITE_LUT_OUT_0               : 11;
	} bits;
};

union REG_YUV_TOP_CA_LITE_LUT_OUT {
	uint32_t raw;
	struct {
		uint32_t CA_LITE_LUT_OUT_1               : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t CA_LITE_LUT_OUT_2               : 11;
	} bits;
};

union REG_YUV_TOP_CA_LITE_LUT_SLP {
	uint32_t raw;
	struct {
		uint32_t CA_LITE_LUT_SLP_0               : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t CA_LITE_LUT_SLP_1               : 12;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_444_422_0 {
	uint32_t raw;
	struct {
		uint32_t OP_START                        : 1;
		uint32_t CONT_EN                         : 1;
		uint32_t BYPASS_EN                       : 1;
		uint32_t SOFTRST                         : 1;
		uint32_t AUTO_UPDATE_EN                  : 1;
		uint32_t DBG_EN                          : 1;
		uint32_t _rsv_6                          : 1;
		uint32_t FORCE_CLK_EN                    : 1;
	} bits;
};

union REG_ISP_444_422_1 {
	uint32_t raw;
	struct {
		uint32_t SHDW_UPDATE_REQ                 : 1;
		uint32_t SHDW_READ_SEL                   : 1;
	} bits;
};

union REG_ISP_444_422_2 {
	uint32_t raw;
	struct {
		uint32_t FD_INT                          : 1;
		uint32_t _rsv_1                          : 1;
		uint32_t FRAME_OVERFLOW                  : 1;
	} bits;
};

union REG_ISP_444_422_3 {
	uint32_t raw;
	struct {
		uint32_t CHECKSUM                        : 32;
	} bits;
};

union REG_ISP_444_422_4 {
	uint32_t raw;
	struct {
		uint32_t REG_422_444                     : 1;
		uint32_t SWAP                            : 2;
	} bits;
};

union REG_ISP_444_422_5 {
	uint32_t raw;
	struct {
		uint32_t FIRST_FRAME_RESET               : 1;
		uint32_t TDNR_DISABLE                    : 1;
		uint32_t DMA_CROP_ENABLE                 : 1;
		uint32_t FORCE_MONO_ENABLE               : 1;
		uint32_t DEBUG_STATUS_EN                 : 1;
	} bits;
};

union REG_ISP_444_422_6 {
	uint32_t raw;
	struct {
		uint32_t TDNR_DEBUG_STATUS               : 32;
	} bits;
};

union REG_ISP_444_422_8 {
	uint32_t raw;
	struct {
		uint32_t GUARD_CNT                       : 8;
		uint32_t FORCE_DMA_DISABLE               : 6;
		uint32_t UV_ROUNDING_TYPE_SEL            : 1;
		uint32_t TDNR_PIXEL_LP                   : 1;
		uint32_t TDNR_DEBUG_SEL                  : 16;
	} bits;
};

union REG_ISP_444_422_9 {
	uint32_t raw;
	struct {
		uint32_t DMA_WRITE_SEL_Y                 : 1;
		uint32_t DMA_WRITE_SEL_C                 : 1;
		uint32_t DMA_SEL                         : 1;
		uint32_t _rsv_3                          : 2;
		uint32_t AVG_MODE_READ                   : 1;
		uint32_t AVG_MODE_WRITE                  : 1;
		uint32_t DROP_MODE_WRITE                 : 1;
	} bits;
};

union REG_ISP_444_422_10 {
	uint32_t raw;
	struct {
		uint32_t IMG_WIDTH_CROP                  : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_HEIGHT_CROP                 : 14;
		uint32_t _rsv_30                         : 1;
		uint32_t CROP_ENABLE                     : 1;
	} bits;
};

union REG_ISP_444_422_11 {
	uint32_t raw;
	struct {
		uint32_t CROP_W_STR                      : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_W_END                      : 14;
	} bits;
};

union REG_ISP_444_422_12 {
	uint32_t raw;
	struct {
		uint32_t CROP_H_STR                      : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t CROP_H_END                      : 14;
	} bits;
};

union REG_ISP_444_422_13 {
	uint32_t raw;
	struct {
		uint32_t REG_3DNR_Y_LUT_IN_0             : 8;
		uint32_t REG_3DNR_Y_LUT_IN_1             : 8;
		uint32_t REG_3DNR_Y_LUT_IN_2             : 8;
		uint32_t REG_3DNR_Y_LUT_IN_3             : 8;
	} bits;
};

union REG_ISP_444_422_14 {
	uint32_t raw;
	struct {
		uint32_t REG_3DNR_Y_LUT_OUT_0            : 8;
		uint32_t REG_3DNR_Y_LUT_OUT_1            : 8;
		uint32_t REG_3DNR_Y_LUT_OUT_2            : 8;
		uint32_t REG_3DNR_Y_LUT_OUT_3            : 8;
	} bits;
};

union REG_ISP_444_422_15 {
	uint32_t raw;
	struct {
		uint32_t REG_3DNR_Y_LUT_SLOPE_0          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t REG_3DNR_Y_LUT_SLOPE_1          : 12;
	} bits;
};

union REG_ISP_444_422_16 {
	uint32_t raw;
	struct {
		uint32_t REG_3DNR_Y_LUT_SLOPE_2          : 12;
		uint32_t _rsv_12                         : 3;
		uint32_t MOTION_SEL                      : 1;
		uint32_t REG_3DNR_Y_BETA_MAX             : 8;
	} bits;
};

union REG_ISP_444_422_17 {
	uint32_t raw;
	struct {
		uint32_t REG_3DNR_U_LUT_IN_0             : 8;
		uint32_t REG_3DNR_U_LUT_IN_1             : 8;
		uint32_t REG_3DNR_U_LUT_IN_2             : 8;
		uint32_t REG_3DNR_U_LUT_IN_3             : 8;
	} bits;
};

union REG_ISP_444_422_18 {
	uint32_t raw;
	struct {
		uint32_t REG_3DNR_U_LUT_OUT_0            : 8;
		uint32_t REG_3DNR_U_LUT_OUT_1            : 8;
		uint32_t REG_3DNR_U_LUT_OUT_2            : 8;
		uint32_t REG_3DNR_U_LUT_OUT_3            : 8;
	} bits;
};

union REG_ISP_444_422_19 {
	uint32_t raw;
	struct {
		uint32_t REG_3DNR_U_LUT_SLOPE_0          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t REG_3DNR_U_LUT_SLOPE_1          : 12;
	} bits;
};

union REG_ISP_444_422_20 {
	uint32_t raw;
	struct {
		uint32_t REG_3DNR_U_LUT_SLOPE_2          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t REG_3DNR_U_BETA_MAX             : 8;
	} bits;
};

union REG_ISP_444_422_21 {
	uint32_t raw;
	struct {
		uint32_t REG_3DNR_V_LUT_IN_0             : 8;
		uint32_t REG_3DNR_V_LUT_IN_1             : 8;
		uint32_t REG_3DNR_V_LUT_IN_2             : 8;
		uint32_t REG_3DNR_V_LUT_IN_3             : 8;
	} bits;
};

union REG_ISP_444_422_22 {
	uint32_t raw;
	struct {
		uint32_t REG_3DNR_V_LUT_OUT_0            : 8;
		uint32_t REG_3DNR_V_LUT_OUT_1            : 8;
		uint32_t REG_3DNR_V_LUT_OUT_2            : 8;
		uint32_t REG_3DNR_V_LUT_OUT_3            : 8;
	} bits;
};

union REG_ISP_444_422_23 {
	uint32_t raw;
	struct {
		uint32_t REG_3DNR_V_LUT_SLOPE_0          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t REG_3DNR_V_LUT_SLOPE_1          : 12;
	} bits;
};

union REG_ISP_444_422_24 {
	uint32_t raw;
	struct {
		uint32_t REG_3DNR_V_LUT_SLOPE_2          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t REG_3DNR_V_BETA_MAX             : 8;
	} bits;
};

union REG_ISP_444_422_25 {
	uint32_t raw;
	struct {
		uint32_t REG_3DNR_MOTION_Y_LUT_IN_0      : 8;
		uint32_t REG_3DNR_MOTION_Y_LUT_IN_1      : 8;
		uint32_t REG_3DNR_MOTION_Y_LUT_IN_2      : 8;
		uint32_t REG_3DNR_MOTION_Y_LUT_IN_3      : 8;
	} bits;
};

union REG_ISP_444_422_26 {
	uint32_t raw;
	struct {
		uint32_t REG_3DNR_MOTION_Y_LUT_OUT_0     : 8;
		uint32_t REG_3DNR_MOTION_Y_LUT_OUT_1     : 8;
		uint32_t REG_3DNR_MOTION_Y_LUT_OUT_2     : 8;
		uint32_t REG_3DNR_MOTION_Y_LUT_OUT_3     : 8;
	} bits;
};

union REG_ISP_444_422_27 {
	uint32_t raw;
	struct {
		uint32_t REG_3DNR_MOTION_Y_LUT_SLOPE_0   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t REG_3DNR_MOTION_Y_LUT_SLOPE_1   : 12;
	} bits;
};

union REG_ISP_444_422_28 {
	uint32_t raw;
	struct {
		uint32_t REG_3DNR_MOTION_Y_LUT_SLOPE_2   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t REG_3DNR_MOTION_C_LUT_SLOPE_0   : 12;
	} bits;
};

union REG_ISP_444_422_29 {
	uint32_t raw;
	struct {
		uint32_t REG_3DNR_MOTION_C_LUT_SLOPE_1   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t REG_3DNR_MOTION_C_LUT_SLOPE_2   : 12;
	} bits;
};

union REG_ISP_444_422_30 {
	uint32_t raw;
	struct {
		uint32_t REG_3DNR_MOTION_C_LUT_IN_0      : 8;
		uint32_t REG_3DNR_MOTION_C_LUT_IN_1      : 8;
		uint32_t REG_3DNR_MOTION_C_LUT_IN_2      : 8;
		uint32_t REG_3DNR_MOTION_C_LUT_IN_3      : 8;
	} bits;
};

union REG_ISP_444_422_31 {
	uint32_t raw;
	struct {
		uint32_t REG_3DNR_MOTION_C_LUT_OUT_0     : 8;
		uint32_t REG_3DNR_MOTION_C_LUT_OUT_1     : 8;
		uint32_t REG_3DNR_MOTION_C_LUT_OUT_2     : 8;
		uint32_t REG_3DNR_MOTION_C_LUT_OUT_3     : 8;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_FBCD_0 {
	uint32_t raw;
	struct {
		uint32_t FBCD_EN                         : 1;
		uint32_t FF_DROP                         : 1;
		uint32_t FORCE_RUN                       : 1;
	} bits;
};

union REG_FBCD_3 {
	uint32_t raw;
	struct {
		uint32_t Y_BIT_STREAM_SIZE               : 22;
		uint32_t _rsv_22                         : 3;
		uint32_t Y_STREAM_TAG                    : 3;
	} bits;
};

union REG_FBCD_4 {
	uint32_t raw;
	struct {
		uint32_t C_BIT_STREAM_SIZE               : 22;
		uint32_t _rsv_22                         : 3;
		uint32_t C_STREAM_TAG                    : 3;
	} bits;
};

union REG_FBCD_FBCE_7 {
	uint32_t raw;
	struct {
		uint32_t DUMMY                           : 8;
		uint32_t SHD_RD                          : 1;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_YUVDITHER_Y_DITHER {
	uint32_t raw;
	struct {
		uint32_t Y_DITHER_EN                     : 1;
		uint32_t Y_DITHER_MOD_EN                 : 1;
		uint32_t Y_DITHER_HISTIDX_EN             : 1;
		uint32_t Y_DITHER_FMNUM_EN               : 1;
		uint32_t Y_DITHER_SHDW_SEL               : 1;
		uint32_t Y_DITHER_SOFTRST                : 1;
		uint32_t _rsv_6                          : 2;
		uint32_t Y_DITHER_HEIGHTM1               : 12;
		uint32_t Y_DITHER_WIDTHM1                : 12;
	} bits;
};

union REG_ISP_YUVDITHER_UV_DITHER {
	uint32_t raw;
	struct {
		uint32_t UV_DITHER_EN                    : 1;
		uint32_t UV_DITHER_MOD_EN                : 1;
		uint32_t UV_DITHER_HISTIDX_EN            : 1;
		uint32_t UV_DITHER_FMNUM_EN              : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t UV_DITHER_HEIGHTM1              : 12;
		uint32_t UV_DITHER_WIDTHM1               : 12;
	} bits;
};

union REG_ISP_YUVDITHER_DEBUG_00 {
	uint32_t raw;
	struct {
		uint32_t UV_DITHER_DEBUG0                : 32;
	} bits;
};

union REG_ISP_YUVDITHER_DEBUG_01 {
	uint32_t raw;
	struct {
		uint32_t Y_DITHER_DEBUG0                 : 32;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_YNR_SHADOW_RD_SEL   {
	uint32_t raw;
	struct {
		uint32_t SHADOW_RD_SEL                   : 1;
	} bits;
};

union REG_ISP_YNR_OUT_SEL         {
	uint32_t raw;
	struct {
		uint32_t YNR_OUT_SEL                     : 4;
	} bits;
};

union REG_ISP_YNR_INDEX_CLR {
	uint32_t raw;
	struct {
		uint32_t YNR_INDEX_CLR                   : 1;
	} bits;
};

union REG_ISP_YNR_NS0_LUMA_TH     {
	uint32_t raw;
	struct {
		uint32_t YNR_NS0_LUMA_TH                 : 8;
	} bits;
};

union REG_ISP_YNR_NS0_SLOPE       {
	uint32_t raw;
	struct {
		uint32_t YNR_NS0_SLOPE                   : 11;
	} bits;
};

union REG_ISP_YNR_NS0_OFFSET      {
	uint32_t raw;
	struct {
		uint32_t YNR_NS0_OFFSET                  : 8;
	} bits;
};

union REG_ISP_YNR_NS1_LUMA_TH     {
	uint32_t raw;
	struct {
		uint32_t YNR_NS1_LUMA_TH                 : 8;
	} bits;
};

union REG_ISP_YNR_NS1_SLOPE       {
	uint32_t raw;
	struct {
		uint32_t YNR_NS1_SLOPE                   : 11;
	} bits;
};

union REG_ISP_YNR_NS1_OFFSET      {
	uint32_t raw;
	struct {
		uint32_t YNR_NS1_OFFSET                  : 8;
	} bits;
};

union REG_ISP_YNR_MOTION_NS_TH    {
	uint32_t raw;
	struct {
		uint32_t YNR_MOTION_NS_TH                : 4;
	} bits;
};

union REG_ISP_YNR_MOTION_POS_GAIN {
	uint32_t raw;
	struct {
		uint32_t YNR_MOTION_POS_GAIN             : 7;
	} bits;
};

union REG_ISP_YNR_MOTION_NEG_GAIN {
	uint32_t raw;
	struct {
		uint32_t YNR_MOTION_NEG_GAIN             : 7;
	} bits;
};

union REG_ISP_YNR_NS_GAIN         {
	uint32_t raw;
	struct {
		uint32_t YNR_NS_GAIN                     : 9;
	} bits;
};

union REG_ISP_YNR_STRENGTH_MODE   {
	uint32_t raw;
	struct {
		uint32_t YNR_STRENGTH_MODE               : 8;
	} bits;
};

union REG_ISP_YNR_INTENSITY_SEL   {
	uint32_t raw;
	struct {
		uint32_t YNR_INTENSITY_SEL               : 5;
	} bits;
};

union REG_ISP_YNR_MOTION_LUT {
	uint32_t raw;
	struct {
		uint32_t YNR_MOTION_LUT                  : 8;
	} bits;
};

union REG_ISP_YNR_WEIGHT_INTRA_0  {
	uint32_t raw;
	struct {
		uint32_t YNR_WEIGHT_INTRA_0              : 3;
	} bits;
};

union REG_ISP_YNR_WEIGHT_INTRA_1  {
	uint32_t raw;
	struct {
		uint32_t YNR_WEIGHT_INTRA_1              : 3;
	} bits;
};

union REG_ISP_YNR_WEIGHT_INTRA_2  {
	uint32_t raw;
	struct {
		uint32_t YNR_WEIGHT_INTRA_2              : 3;
	} bits;
};

union REG_ISP_YNR_WEIGHT_NORM_1   {
	uint32_t raw;
	struct {
		uint32_t YNR_WEIGHT_NORM_1               : 7;
	} bits;
};

union REG_ISP_YNR_WEIGHT_NORM_2   {
	uint32_t raw;
	struct {
		uint32_t YNR_WEIGHT_NORM_2               : 6;
	} bits;
};

union REG_ISP_YNR_ALPHA_GAIN      {
	uint32_t raw;
	struct {
		uint32_t YNR_ALPHA_GAIN                  : 10;
	} bits;
};

union REG_ISP_YNR_VAR_TH          {
	uint32_t raw;
	struct {
		uint32_t YNR_VAR_TH                      : 8;
	} bits;
};

union REG_ISP_YNR_WEIGHT_LUT      {
	uint32_t raw;
	struct {
		uint32_t YNR_WEIGHT_LUT                  : 5;
	} bits;
};

union REG_ISP_YNR_WEIGHT_SM       {
	uint32_t raw;
	struct {
		uint32_t YNR_WEIGHT_SMOOTH               : 5;
	} bits;
};

union REG_ISP_YNR_WEIGHT_V        {
	uint32_t raw;
	struct {
		uint32_t YNR_WEIGHT_V                    : 5;
	} bits;
};

union REG_ISP_YNR_WEIGHT_H        {
	uint32_t raw;
	struct {
		uint32_t YNR_WEIGHT_H                    : 5;
	} bits;
};

union REG_ISP_YNR_WEIGHT_D45      {
	uint32_t raw;
	struct {
		uint32_t YNR_WEIGHT_D45                  : 5;
	} bits;
};

union REG_ISP_YNR_WEIGHT_D135     {
	uint32_t raw;
	struct {
		uint32_t YNR_WEIGHT_D135                 : 5;
	} bits;
};

union REG_ISP_YNR_NEIGHBOR_MAX    {
	uint32_t raw;
	struct {
		uint32_t YNR_FLAG_NEIGHBOR_MAX_WEIGHT    : 1;
	} bits;
};

union REG_ISP_YNR_RES_K_SMOOTH    {
	uint32_t raw;
	struct {
		uint32_t YNR_RES_RATIO_K_SMOOTH          : 9;
	} bits;
};

union REG_ISP_YNR_RES_K_TEXTURE   {
	uint32_t raw;
	struct {
		uint32_t YNR_RES_RATIO_K_TEXTURE         : 9;
	} bits;
};

union REG_ISP_YNR_FILTER_MODE_ENABLE {
	uint32_t raw;
	struct {
		uint32_t YNR_FILTER_MODE_ENABLE          : 1;
	} bits;
};

union REG_ISP_YNR_FILTER_MODE_ALPHA {
	uint32_t raw;
	struct {
		uint32_t YNR_FILTER_MODE_ALPHA           : 9;
	} bits;
};

union REG_ISP_YNR_RES_MOT_LUT {
	uint32_t raw;
	struct {
		uint32_t YNR_RES_MOT_LUT                 : 9;
	} bits;
};

union REG_ISP_YNR_RES_MAX {
	uint32_t raw;
	struct {
		uint32_t YNR_RES_MAX                     : 8;
	} bits;
};

union REG_ISP_YNR_RES_MOTION_MAX {
	uint32_t raw;
	struct {
		uint32_t YNR_RES_MOTION_MAX              : 8;
	} bits;
};

union REG_ISP_YNR_MOTION_NS_CLIP_MAX {
	uint32_t raw;
	struct {
		uint32_t YNR_MOTION_NS_CLIP_MAX          : 8;
	} bits;
};

union REG_ISP_YNR_DUMMY           {
	uint32_t raw;
	struct {
		uint32_t YNR_DUMMY                       : 16;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_CNR_ENABLE {
	uint32_t raw;
	struct {
		uint32_t CNR_ENABLE                      : 1;
		uint32_t PFC_ENABLE                      : 1;
		uint32_t _rsv_2                          : 6;
		uint32_t CNR_DIFF_SHIFT_VAL              : 8;
		uint32_t CNR_RATIO                       : 8;
	} bits;
};

union REG_ISP_CNR_STRENGTH_MODE {
	uint32_t raw;
	struct {
		uint32_t CNR_STRENGTH_MODE               : 8;
		uint32_t CNR_FUSION_INTENSITY_WEIGHT     : 4;
		uint32_t _rsv_12                         : 1;
		uint32_t CNR_WEIGHT_INTER_SEL            : 4;
		uint32_t CNR_VAR_TH                      : 9;
		uint32_t CNR_FLAG_NEIGHBOR_MAX_WEIGHT    : 1;
		uint32_t CNR_SHDW_SEL                    : 1;
		uint32_t CNR_OUT_SEL                     : 2;
	} bits;
};

union REG_ISP_CNR_PURPLE_TH {
	uint32_t raw;
	struct {
		uint32_t CNR_PURPLE_TH                   : 8;
		uint32_t CNR_CORRECT_STRENGTH            : 8;
		uint32_t CNR_DIFF_GAIN                   : 4;
		uint32_t _rsv_20                         : 4;
		uint32_t CNR_MOTION_ENABLE               : 1;
		uint32_t _rsv_25                         : 3;
		uint32_t CNR_PURPLE_MODE                 : 1;
	} bits;
};

union REG_ISP_CNR_PURPLE_CR {
	uint32_t raw;
	struct {
		uint32_t CNR_PURPLE_CR                   : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t CNR_GREEN_CB                    : 8;
	} bits;
};

union REG_ISP_CNR_GREEN_CR {
	uint32_t raw;
	struct {
		uint32_t CNR_GREEN_CR                    : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t CNR_PURPLE_CB                   : 8;
	} bits;
};

union REG_ISP_CNR_WEIGHT_LUT_INTER_CNR_00 {
	uint32_t raw;
	struct {
		uint32_t WEIGHT_LUT_INTER_CNR_00         : 5;
		uint32_t _rsv_5                          : 3;
		uint32_t WEIGHT_LUT_INTER_CNR_01         : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t WEIGHT_LUT_INTER_CNR_02         : 5;
		uint32_t _rsv_21                         : 3;
		uint32_t WEIGHT_LUT_INTER_CNR_03         : 5;
	} bits;
};

union REG_ISP_CNR_WEIGHT_LUT_INTER_CNR_04 {
	uint32_t raw;
	struct {
		uint32_t WEIGHT_LUT_INTER_CNR_04         : 5;
		uint32_t _rsv_5                          : 3;
		uint32_t WEIGHT_LUT_INTER_CNR_05         : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t WEIGHT_LUT_INTER_CNR_06         : 5;
		uint32_t _rsv_21                         : 3;
		uint32_t WEIGHT_LUT_INTER_CNR_07         : 5;
	} bits;
};

union REG_ISP_CNR_WEIGHT_LUT_INTER_CNR_08 {
	uint32_t raw;
	struct {
		uint32_t WEIGHT_LUT_INTER_CNR_08         : 5;
		uint32_t _rsv_5                          : 3;
		uint32_t WEIGHT_LUT_INTER_CNR_09         : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t WEIGHT_LUT_INTER_CNR_10         : 5;
		uint32_t _rsv_21                         : 3;
		uint32_t WEIGHT_LUT_INTER_CNR_11         : 5;
	} bits;
};

union REG_ISP_CNR_WEIGHT_LUT_INTER_CNR_12 {
	uint32_t raw;
	struct {
		uint32_t WEIGHT_LUT_INTER_CNR_12         : 5;
		uint32_t _rsv_5                          : 3;
		uint32_t WEIGHT_LUT_INTER_CNR_13         : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t WEIGHT_LUT_INTER_CNR_14         : 5;
		uint32_t _rsv_21                         : 3;
		uint32_t WEIGHT_LUT_INTER_CNR_15         : 5;
	} bits;
};

union REG_ISP_CNR_INTENSITY_SEL_0 {
	uint32_t raw;
	struct {
		uint32_t CNR_INTENSITY_SEL_0             : 5;
		uint32_t _rsv_5                          : 3;
		uint32_t CNR_INTENSITY_SEL_1             : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t CNR_INTENSITY_SEL_2             : 5;
		uint32_t _rsv_21                         : 3;
		uint32_t CNR_INTENSITY_SEL_3             : 5;
	} bits;
};

union REG_ISP_CNR_INTENSITY_SEL_4 {
	uint32_t raw;
	struct {
		uint32_t CNR_INTENSITY_SEL_4             : 5;
		uint32_t _rsv_5                          : 3;
		uint32_t CNR_INTENSITY_SEL_5             : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t CNR_INTENSITY_SEL_6             : 5;
		uint32_t _rsv_21                         : 3;
		uint32_t CNR_INTENSITY_SEL_7             : 5;
	} bits;
};

union REG_ISP_CNR_MOTION_LUT_0 {
	uint32_t raw;
	struct {
		uint32_t CNR_MOTION_LUT_0                : 8;
		uint32_t CNR_MOTION_LUT_1                : 8;
		uint32_t CNR_MOTION_LUT_2                : 8;
		uint32_t CNR_MOTION_LUT_3                : 8;
	} bits;
};

union REG_ISP_CNR_MOTION_LUT_4 {
	uint32_t raw;
	struct {
		uint32_t CNR_MOTION_LUT_4                : 8;
		uint32_t CNR_MOTION_LUT_5                : 8;
		uint32_t CNR_MOTION_LUT_6                : 8;
		uint32_t CNR_MOTION_LUT_7                : 8;
	} bits;
};

union REG_ISP_CNR_MOTION_LUT_8 {
	uint32_t raw;
	struct {
		uint32_t CNR_MOTION_LUT_8                : 8;
		uint32_t CNR_MOTION_LUT_9                : 8;
		uint32_t CNR_MOTION_LUT_10               : 8;
		uint32_t CNR_MOTION_LUT_11               : 8;
	} bits;
};

union REG_ISP_CNR_MOTION_LUT_12 {
	uint32_t raw;
	struct {
		uint32_t CNR_MOTION_LUT_12               : 8;
		uint32_t CNR_MOTION_LUT_13               : 8;
		uint32_t CNR_MOTION_LUT_14               : 8;
		uint32_t CNR_MOTION_LUT_15               : 8;
	} bits;
};

union REG_ISP_CNR_PURPLE_CB2 {
	uint32_t raw;
	struct {
		uint32_t CNR_PURPLE_CB2                  : 8;
		uint32_t CNR_PURPLE_CR2                  : 8;
		uint32_t CNR_PURPLE_CB3                  : 8;
		uint32_t CNR_PURPLE_CR3                  : 8;
	} bits;
};

union REG_ISP_CNR_PURPLE_W1 {
	uint32_t raw;
	struct {
		uint32_t CNR_PURPLE_W1                   : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t CNR_PURPLE_W2                   : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t CNR_PURPLE_W3                   : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t CNR_MASK                        : 8;
		uint32_t CNR_PURPLE_NORM                 : 9;
	} bits;
};

union REG_ISP_CNR_DUMMY {
	uint32_t raw;
	struct {
		uint32_t CNR_DUMMY                       : 32;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_EE_00 {
	uint32_t raw;
	struct {
		uint32_t EE_ENABLE                       : 1;
		uint32_t FORCE_CLK_EN                    : 1;
		uint32_t MIRROR_MODE_EN                  : 1;
		uint32_t EE_DOUBLE_BUF_SEL               : 1;
		uint32_t EE_DEBUG_MODE                   : 4;
		uint32_t EE_TOTAL_CORING                 : 10;
		uint32_t _rsv_18                         : 6;
		uint32_t EE_TOTAL_GAIN                   : 8;
	} bits;
};

union REG_ISP_EE_04 {
	uint32_t raw;
	struct {
		uint32_t EE_TOTAL_OSHTTHRD               : 8;
		uint32_t EE_TOTAL_USHTTHRD               : 8;
		uint32_t EE_DEBUG_SHIFT_BIT              : 3;
		uint32_t _rsv_19                         : 3;
		uint32_t EE_HANDSHAKE_BYPASS             : 1;
		uint32_t EE_PRE_PROC_BLENDING            : 1;
		uint32_t EE_PRE_PROC_GAIN                : 6;
		uint32_t EE_PRE_PROC_MODE                : 1;
		uint32_t EE_PRE_PROC_ENABLE              : 1;
	} bits;
};

union REG_ISP_EE_08 {
	uint32_t raw;
	struct {
		uint32_t EE_STD_SHTCTRL_EN               : 1;
		uint32_t EE_STD_SHTCTRL_MAX3X3_EN        : 1;
		uint32_t EE_STD_SHTCTRL_MIN3X3_EN        : 1;
		uint32_t _rsv_3                          : 5;
		uint32_t EE_STD_SHTCTRL_MIN_GAIN         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_STD_SHTCTRL_DIFF_GAIN        : 6;
		uint32_t _rsv_22                         : 1;
		uint32_t EE_STD_SHTCTRL_DIFF_THRD        : 9;
	} bits;
};

union REG_ISP_EE_0C {
	uint32_t raw;
	struct {
		uint32_t EE_LUMAREF_LPF_EN               : 1;
		uint32_t EE_LUMA_CORING_EN               : 1;
		uint32_t EE_LUMA_SHTCTRL_EN              : 1;
		uint32_t EE_DELTA_SHTCTRL_EN             : 1;
		uint32_t EE_DELTA_SHTCTRL_SHIFT          : 2;
		uint32_t EE_LUMA_ADPTCTRL_EN             : 1;
		uint32_t EE_DELTA_ADPTCTRL_EN            : 1;
		uint32_t EE_DELTA_ADPTCTRL_SHIFT         : 2;
	} bits;
};

union REG_ISP_EE_10 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRDET_LPF_BLDWGT            : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_DIRDET_MIN_GAIN              : 6;
	} bits;
};

union REG_ISP_EE_14 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRDET_DIRGAIN               : 8;
		uint32_t EE_DIRDET_DIRTHRD               : 8;
		uint32_t EE_DIRDET_SUBGAIN               : 8;
		uint32_t EE_DIRDET_SUBTHRD               : 8;
	} bits;
};

union REG_ISP_EE_18 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRDET_SADNORM_TGT           : 8;
		uint32_t EE_DIRDET_DGRADPT_TRANS_SLOP    : 4;
		uint32_t EE_DIRDET_MIN_GAIN_ADPT_EN      : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t EE_DIRDET_MIDLEDIR_GAIN         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_DIRDET_ALIGNDIR_GAIN         : 6;
	} bits;
};

union REG_ISP_EE_1C {
	uint32_t raw;
	struct {
		uint32_t EE_DIRSUB_WGT_CORING_THRD       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t EE_DIRDET_WGT_CORING_THRD       : 12;
	} bits;
};

union REG_ISP_EE_20 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_000_NORGAIN      : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t EE_DIRCAL_DGR4_045_NORGAIN      : 13;
	} bits;
};

union REG_ISP_EE_24 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_090_NORGAIN      : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t EE_DIRCAL_DGR4_135_NORGAIN      : 13;
	} bits;
};

union REG_ISP_EE_28 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_NOD_NORGAIN      : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t EE_DIRCAL_DGR4_DIR_ADJGAIN      : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_DIRCAL_DGR4_NOD_CORINGTH     : 8;
	} bits;
};

union REG_ISP_EE_2C {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_000_CORINGTH     : 8;
		uint32_t EE_DIRCAL_DGR4_045_CORINGTH     : 8;
		uint32_t EE_DIRCAL_DGR4_090_CORINGTH     : 8;
		uint32_t EE_DIRCAL_DGR4_135_CORINGTH     : 8;
	} bits;
};

union REG_ISP_EE_30 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTER000_W1     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t EE_DIRCAL_DGR4_FILTER000_W2     : 9;
	} bits;
};

union REG_ISP_EE_34 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTER000_W3     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t EE_DIRCAL_DGR4_FILTER000_W4     : 9;
	} bits;
};

union REG_ISP_EE_38 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTER000_W5     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t EE_DIRCAL_DGR4_FILTER000_W6     : 9;
	} bits;
};

union REG_ISP_EE_3C {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTER000_W7     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t EE_DIRCAL_DGR4_FILTER000_W8     : 9;
	} bits;
};

union REG_ISP_EE_40 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTER000_W9     : 9;
	} bits;
};

union REG_ISP_EE_58 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTER045_W1     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t EE_DIRCAL_DGR4_FILTER045_W2     : 9;
	} bits;
};

union REG_ISP_EE_5C {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTER045_W3     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t EE_DIRCAL_DGR4_FILTER045_W4     : 9;
	} bits;
};

union REG_ISP_EE_60 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTER045_W5     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t EE_DIRCAL_DGR4_FILTER045_W6     : 9;
	} bits;
};

union REG_ISP_EE_64 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTER045_W7     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t EE_DIRCAL_DGR4_FILTER045_W8     : 9;
	} bits;
};

union REG_ISP_EE_68 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTER045_W9     : 9;
	} bits;
};

union REG_ISP_EE_6C {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTER090_W1     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t EE_DIRCAL_DGR4_FILTER090_W2     : 9;
	} bits;
};

union REG_ISP_EE_70 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTER090_W3     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t EE_DIRCAL_DGR4_FILTER090_W4     : 9;
	} bits;
};

union REG_ISP_EE_74 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTER090_W5     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t EE_DIRCAL_DGR4_FILTER090_W6     : 9;
	} bits;
};

union REG_ISP_EE_78 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTER090_W7     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t EE_DIRCAL_DGR4_FILTER090_W8     : 9;
	} bits;
};

union REG_ISP_EE_7C {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTER090_W9     : 9;
	} bits;
};

union REG_ISP_EE_80 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTER135_W1     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t EE_DIRCAL_DGR4_FILTER135_W2     : 9;
	} bits;
};

union REG_ISP_EE_84 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTER135_W3     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t EE_DIRCAL_DGR4_FILTER135_W4     : 9;
	} bits;
};

union REG_ISP_EE_88 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTER135_W5     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t EE_DIRCAL_DGR4_FILTER135_W6     : 9;
	} bits;
};

union REG_ISP_EE_8C {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTER135_W7     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t EE_DIRCAL_DGR4_FILTER135_W8     : 9;
	} bits;
};

union REG_ISP_EE_90 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTER135_W9     : 9;
	} bits;
};

union REG_ISP_EE_94 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTERNOD_W1     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t EE_DIRCAL_DGR4_FILTERNOD_W2     : 9;
	} bits;
};

union REG_ISP_EE_98 {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTERNOD_W3     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t EE_DIRCAL_DGR4_FILTERNOD_W4     : 9;
	} bits;
};

union REG_ISP_EE_9C {
	uint32_t raw;
	struct {
		uint32_t EE_DIRCAL_DGR4_FILTERNOD_W5     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t EE_DIRCAL_DGR4_FILTERNOD_W6     : 9;
	} bits;
};

union REG_ISP_EE_A4 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_CORING_LUT_00           : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t EE_LUMA_CORING_LUT_01           : 10;
	} bits;
};

union REG_ISP_EE_A8 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_CORING_LUT_02           : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t EE_LUMA_CORING_LUT_03           : 10;
	} bits;
};

union REG_ISP_EE_AC {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_CORING_LUT_04           : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t EE_LUMA_CORING_LUT_05           : 10;
	} bits;
};

union REG_ISP_EE_B0 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_CORING_LUT_06           : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t EE_LUMA_CORING_LUT_07           : 10;
	} bits;
};

union REG_ISP_EE_B4 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_CORING_LUT_08           : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t EE_LUMA_CORING_LUT_09           : 10;
	} bits;
};

union REG_ISP_EE_B8 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_CORING_LUT_10           : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t EE_LUMA_CORING_LUT_11           : 10;
	} bits;
};

union REG_ISP_EE_BC {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_CORING_LUT_12           : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t EE_LUMA_CORING_LUT_13           : 10;
	} bits;
};

union REG_ISP_EE_C0 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_CORING_LUT_14           : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t EE_LUMA_CORING_LUT_15           : 10;
	} bits;
};

union REG_ISP_EE_C4 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_CORING_LUT_16           : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t EE_LUMA_CORING_LUT_17           : 10;
	} bits;
};

union REG_ISP_EE_C8 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_CORING_LUT_18           : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t EE_LUMA_CORING_LUT_19           : 10;
	} bits;
};

union REG_ISP_EE_CC {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_CORING_LUT_20           : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t EE_LUMA_CORING_LUT_21           : 10;
	} bits;
};

union REG_ISP_EE_D0 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_CORING_LUT_22           : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t EE_LUMA_CORING_LUT_23           : 10;
	} bits;
};

union REG_ISP_EE_D4 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_CORING_LUT_24           : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t EE_LUMA_CORING_LUT_25           : 10;
	} bits;
};

union REG_ISP_EE_D8 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_CORING_LUT_26           : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t EE_LUMA_CORING_LUT_27           : 10;
	} bits;
};

union REG_ISP_EE_DC {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_CORING_LUT_28           : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t EE_LUMA_CORING_LUT_29           : 10;
	} bits;
};

union REG_ISP_EE_E0 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_CORING_LUT_30           : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t EE_LUMA_CORING_LUT_31           : 10;
	} bits;
};

union REG_ISP_EE_E4 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_CORING_LUT_32           : 10;
	} bits;
};

union REG_ISP_EE_E8 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_SHTCTRL_LUT_00          : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_01          : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_02          : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_03          : 6;
	} bits;
};

union REG_ISP_EE_EC {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_SHTCTRL_LUT_04          : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_05          : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_06          : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_07          : 6;
	} bits;
};

union REG_ISP_EE_F0 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_SHTCTRL_LUT_08          : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_09          : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_10          : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_11          : 6;
	} bits;
};

union REG_ISP_EE_F4 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_SHTCTRL_LUT_12          : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_13          : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_14          : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_15          : 6;
	} bits;
};

union REG_ISP_EE_F8 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_SHTCTRL_LUT_16          : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_17          : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_18          : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_19          : 6;
	} bits;
};

union REG_ISP_EE_FC {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_SHTCTRL_LUT_20          : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_21          : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_22          : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_23          : 6;
	} bits;
};

union REG_ISP_EE_100 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_SHTCTRL_LUT_24          : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_25          : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_26          : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_27          : 6;
	} bits;
};

union REG_ISP_EE_104 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_SHTCTRL_LUT_28          : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_29          : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_30          : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_LUMA_SHTCTRL_LUT_31          : 6;
	} bits;
};

union REG_ISP_EE_108 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_SHTCTRL_LUT_32          : 6;
	} bits;
};

union REG_ISP_EE_10C {
	uint32_t raw;
	struct {
		uint32_t EE_DELTA_SHTCTRL_LUT_00         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_01         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_02         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_03         : 6;
	} bits;
};

union REG_ISP_EE_110 {
	uint32_t raw;
	struct {
		uint32_t EE_DELTA_SHTCTRL_LUT_04         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_05         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_06         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_07         : 6;
	} bits;
};

union REG_ISP_EE_114 {
	uint32_t raw;
	struct {
		uint32_t EE_DELTA_SHTCTRL_LUT_08         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_09         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_10         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_11         : 6;
	} bits;
};

union REG_ISP_EE_118 {
	uint32_t raw;
	struct {
		uint32_t EE_DELTA_SHTCTRL_LUT_12         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_13         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_14         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_15         : 6;
	} bits;
};

union REG_ISP_EE_11C {
	uint32_t raw;
	struct {
		uint32_t EE_DELTA_SHTCTRL_LUT_16         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_17         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_18         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_19         : 6;
	} bits;
};

union REG_ISP_EE_120 {
	uint32_t raw;
	struct {
		uint32_t EE_DELTA_SHTCTRL_LUT_20         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_21         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_22         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_23         : 6;
	} bits;
};

union REG_ISP_EE_124 {
	uint32_t raw;
	struct {
		uint32_t EE_DELTA_SHTCTRL_LUT_24         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_25         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_26         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_27         : 6;
	} bits;
};

union REG_ISP_EE_128 {
	uint32_t raw;
	struct {
		uint32_t EE_DELTA_SHTCTRL_LUT_28         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_29         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_30         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_DELTA_SHTCTRL_LUT_31         : 6;
	} bits;
};

union REG_ISP_EE_12C {
	uint32_t raw;
	struct {
		uint32_t EE_DELTA_SHTCTRL_LUT_32         : 6;
	} bits;
};

union REG_ISP_EE_130 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_ADPTCTRL_LUT_00         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_01         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_02         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_03         : 6;
	} bits;
};

union REG_ISP_EE_134 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_ADPTCTRL_LUT_04         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_05         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_06         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_07         : 6;
	} bits;
};

union REG_ISP_EE_138 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_ADPTCTRL_LUT_08         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_09         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_10         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_11         : 6;
	} bits;
};

union REG_ISP_EE_13C {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_ADPTCTRL_LUT_12         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_13         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_14         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_15         : 6;
	} bits;
};

union REG_ISP_EE_140 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_ADPTCTRL_LUT_16         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_17         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_18         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_19         : 6;
	} bits;
};

union REG_ISP_EE_144 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_ADPTCTRL_LUT_20         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_21         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_22         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_23         : 6;
	} bits;
};

union REG_ISP_EE_148 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_ADPTCTRL_LUT_24         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_25         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_26         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_27         : 6;
	} bits;
};

union REG_ISP_EE_14C {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_ADPTCTRL_LUT_28         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_29         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_30         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_LUMA_ADPTCTRL_LUT_31         : 6;
	} bits;
};

union REG_ISP_EE_150 {
	uint32_t raw;
	struct {
		uint32_t EE_LUMA_ADPTCTRL_LUT_32         : 6;
	} bits;
};

union REG_ISP_EE_154 {
	uint32_t raw;
	struct {
		uint32_t EE_DELTA_ADPTCTRL_LUT_00        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_01        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_02        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_03        : 6;
	} bits;
};

union REG_ISP_EE_158 {
	uint32_t raw;
	struct {
		uint32_t EE_DELTA_ADPTCTRL_LUT_04        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_05        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_06        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_07        : 6;
	} bits;
};

union REG_ISP_EE_15C {
	uint32_t raw;
	struct {
		uint32_t EE_DELTA_ADPTCTRL_LUT_08        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_09        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_10        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_11        : 6;
	} bits;
};

union REG_ISP_EE_160 {
	uint32_t raw;
	struct {
		uint32_t EE_DELTA_ADPTCTRL_LUT_12        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_13        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_14        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_15        : 6;
	} bits;
};

union REG_ISP_EE_164 {
	uint32_t raw;
	struct {
		uint32_t EE_DELTA_ADPTCTRL_LUT_16        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_17        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_18        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_19        : 6;
	} bits;
};

union REG_ISP_EE_168 {
	uint32_t raw;
	struct {
		uint32_t EE_DELTA_ADPTCTRL_LUT_20        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_21        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_22        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_23        : 6;
	} bits;
};

union REG_ISP_EE_16C {
	uint32_t raw;
	struct {
		uint32_t EE_DELTA_ADPTCTRL_LUT_24        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_25        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_26        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_27        : 6;
	} bits;
};

union REG_ISP_EE_170 {
	uint32_t raw;
	struct {
		uint32_t EE_DELTA_ADPTCTRL_LUT_28        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_29        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_30        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_DELTA_ADPTCTRL_LUT_31        : 6;
	} bits;
};

union REG_ISP_EE_174 {
	uint32_t raw;
	struct {
		uint32_t EE_DELTA_ADPTCTRL_LUT_32        : 6;
	} bits;
};

union REG_ISP_EE_178 {
	uint32_t raw;
	struct {
		uint32_t SAD_NORM_MUL_00                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t SAD_NORM_MUL_01                 : 12;
	} bits;
};

union REG_ISP_EE_17C {
	uint32_t raw;
	struct {
		uint32_t SAD_NORM_MUL_02                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t SAD_NORM_MUL_03                 : 12;
	} bits;
};

union REG_ISP_EE_180 {
	uint32_t raw;
	struct {
		uint32_t SAD_NORM_MUL_04                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t SAD_NORM_MUL_05                 : 12;
	} bits;
};

union REG_ISP_EE_184 {
	uint32_t raw;
	struct {
		uint32_t SAD_NORM_MUL_06                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t SAD_NORM_MUL_07                 : 12;
	} bits;
};

union REG_ISP_EE_188 {
	uint32_t raw;
	struct {
		uint32_t SAD_NORM_MUL_08                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t SAD_NORM_MUL_09                 : 12;
	} bits;
};

union REG_ISP_EE_18C {
	uint32_t raw;
	struct {
		uint32_t SAD_NORM_MUL_10                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t SAD_NORM_MUL_11                 : 12;
	} bits;
};

union REG_ISP_EE_190 {
	uint32_t raw;
	struct {
		uint32_t SAD_NORM_MUL_12                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t SAD_NORM_MUL_13                 : 12;
	} bits;
};

union REG_ISP_EE_194 {
	uint32_t raw;
	struct {
		uint32_t SAD_NORM_MUL_14                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t SAD_NORM_MUL_15                 : 12;
	} bits;
};

union REG_ISP_EE_198 {
	uint32_t raw;
	struct {
		uint32_t SAD_NORM_MUL_16                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t SAD_NORM_MUL_17                 : 12;
	} bits;
};

union REG_ISP_EE_19C {
	uint32_t raw;
	struct {
		uint32_t SAD_NORM_MUL_18                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t SAD_NORM_MUL_19                 : 12;
	} bits;
};

union REG_ISP_EE_1A0 {
	uint32_t raw;
	struct {
		uint32_t SAD_NORM_MUL_20                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t SAD_NORM_MUL_21                 : 12;
	} bits;
};

union REG_ISP_EE_1A4 {
	uint32_t raw;
	struct {
		uint32_t SAD_NORM_MUL_22                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t SAD_NORM_MUL_23                 : 12;
	} bits;
};

union REG_ISP_EE_1A8 {
	uint32_t raw;
	struct {
		uint32_t SAD_NORM_MUL_24                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t SAD_NORM_MUL_25                 : 12;
	} bits;
};

union REG_ISP_EE_1AC {
	uint32_t raw;
	struct {
		uint32_t SAD_NORM_MUL_26                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t SAD_NORM_MUL_27                 : 12;
	} bits;
};

union REG_ISP_EE_1B0 {
	uint32_t raw;
	struct {
		uint32_t SAD_NORM_MUL_28                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t SAD_NORM_MUL_29                 : 12;
	} bits;
};

union REG_ISP_EE_1B4 {
	uint32_t raw;
	struct {
		uint32_t SAD_NORM_MUL_30                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t SAD_NORM_MUL_31                 : 12;
	} bits;
};

union REG_ISP_EE_1B8 {
	uint32_t raw;
	struct {
		uint32_t EE_DEBUG_RESERVE                : 32;
	} bits;
};

union REG_ISP_EE_1BC {
	uint32_t raw;
	struct {
		uint32_t IMG_HEIGHT                      : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t IMG_WIDTH                       : 14;
	} bits;
};

union REG_ISP_EE_1C0 {
	uint32_t raw;
	struct {
		uint32_t GUARD_CNT                       : 8;
	} bits;
};

union REG_ISP_EE_1C4 {
	uint32_t raw;
	struct {
		uint32_t GUARD_CNT_UV                    : 8;
		uint32_t UV_BYPASS                       : 1;
		uint32_t UV_BYPASS_SEL                   : 1;
		uint32_t _rsv_10                         : 6;
		uint32_t EE_SHTCTRL_OSHTGAIN             : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t EE_SHTCTRL_USHTGAIN             : 6;
	} bits;
};

union REG_ISP_EE_1C8 {
	uint32_t raw;
	struct {
		uint32_t EE_TOTAL_OSHTTHRD_CLP           : 8;
		uint32_t EE_TOTAL_USHTTHRD_CLP           : 8;
	} bits;
};

union REG_ISP_EE_1CC {
	uint32_t raw;
	struct {
		uint32_t EE_MOTION_LUT_IN_0              : 8;
		uint32_t EE_MOTION_LUT_IN_1              : 8;
		uint32_t EE_MOTION_LUT_IN_2              : 8;
		uint32_t EE_MOTION_LUT_IN_3              : 8;
	} bits;
};

union REG_ISP_EE_1D0 {
	uint32_t raw;
	struct {
		uint32_t EE_MOTION_LUT_OUT_0             : 8;
		uint32_t EE_MOTION_LUT_OUT_1             : 8;
		uint32_t EE_MOTION_LUT_OUT_2             : 8;
		uint32_t EE_MOTION_LUT_OUT_3             : 8;
	} bits;
};

union REG_ISP_EE_1D4 {
	uint32_t raw;
	struct {
		uint32_t EE_MOTION_LUT_SLOPE_0           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t EE_MOTION_LUT_SLOPE_1           : 12;
	} bits;
};

union REG_ISP_EE_1D8 {
	uint32_t raw;
	struct {
		uint32_t EE_MOTION_LUT_SLOPE_2           : 12;
		uint32_t _rsv_12                         : 1;
		uint32_t EE_DEBUG_MMAP_MODE              : 1;
		uint32_t EE_DEBUG_MOTION                 : 1;
		uint32_t EE_DEBUG_DELTA                  : 1;
		uint32_t EE_DEBUG_MMAP_THD0              : 8;
		uint32_t EE_DEBUG_MMAP_THD1              : 8;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_YCURVE_YCUR_CTRL {
	uint32_t raw;
	struct {
		uint32_t YCURVE_ENABLE                   : 1;
		uint32_t YCUR_SHDW_SEL                   : 1;
		uint32_t FORCE_CLK_ENABLE                : 1;
	} bits;
};

union REG_ISP_YCURVE_YCUR_PROG_CTRL {
	uint32_t raw;
	struct {
		uint32_t YCUR_WSEL                       : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t YCUR_RSEL                       : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t YCUR_PROG_EN                    : 1;
	} bits;
};

union REG_ISP_YCURVE_YCUR_PROG_ST_ADDR {
	uint32_t raw;
	struct {
		uint32_t YCUR_ST_ADDR                    : 6;
		uint32_t _rsv_6                          : 25;
		uint32_t YCUR_ST_W                       : 1;
	} bits;
};

union REG_ISP_YCURVE_YCUR_PROG_DATA {
	uint32_t raw;
	struct {
		uint32_t YCUR_DATA_E                     : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t YCUR_DATA_O                     : 8;
		uint32_t _rsv_24                         : 7;
		uint32_t YCUR_W                          : 1;
	} bits;
};

union REG_ISP_YCURVE_YCUR_PROG_MAX {
	uint32_t raw;
	struct {
		uint32_t YCURVE_LUT_256                  : 9;
	} bits;
};

union REG_ISP_YCURVE_YCUR_MEM_SW_MODE {
	uint32_t raw;
	struct {
		uint32_t YCUR_SW_RADDR                   : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t YCUR_SW_R_MEM_SEL               : 1;
	} bits;
};

union REG_ISP_YCURVE_YCUR_MEM_SW_RDATA {
	uint32_t raw;
	struct {
		uint32_t YCUR_RDATA_R                    : 8;
		uint32_t _rsv_8                          : 23;
		uint32_t YCUR_SW_R                       : 1;
	} bits;
};

union REG_ISP_YCURVE_YCUR_DBG {
	uint32_t raw;
	struct {
		uint32_t PROG_HDK_DIS                    : 1;
		uint32_t SOFTRST                         : 1;
	} bits;
};

union REG_ISP_YCURVE_YCUR_DMY0 {
	uint32_t raw;
	struct {
		uint32_t DMY_DEF0                        : 32;
	} bits;
};

union REG_ISP_YCURVE_YCUR_DMY1 {
	uint32_t raw;
	struct {
		uint32_t DMY_DEF1                        : 32;
	} bits;
};

union REG_ISP_YCURVE_YCUR_DMY_R {
	uint32_t raw;
	struct {
		uint32_t DMY_RO                          : 32;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_TOP_0 {
	uint32_t raw;
	struct {
		uint32_t FRAME_DONE_FE0                  : 4;
		uint32_t FRAME_DONE_FE1                  : 2;
		uint32_t FRAME_DONE_BE                   : 2;
		uint32_t FRAME_DONE_POST                 : 1;
		uint32_t SHAW_DONE_FE0                   : 4;
		uint32_t SHAW_DONE_FE1                   : 2;
		uint32_t SHAW_DONE_BE                    : 2;
		uint32_t SHAW_DONE_POST                  : 1;
		uint32_t FRAME_START_FE0                 : 4;
		uint32_t FRAME_START_FE1                 : 2;
		uint32_t FRAME_ERR                       : 1;
		uint32_t PCHK_ERR                        : 1;
		uint32_t CMDQ_INT                        : 1;
		uint32_t LINE_INTP_FE0                   : 1;
		uint32_t LINE_INTP_FE1                   : 1;
		uint32_t LINE_INTP_POST                  : 1;
		uint32_t INT_BRIDGE_LITE                 : 1;
		uint32_t INT_DMA_ERR                     : 1;
	} bits;
};

union REG_ISP_TOP_1 {
	uint32_t raw;
	struct {
		uint32_t PCHK0_ERR_FE0                   : 1;
		uint32_t PCHK0_ERR_FE1                   : 1;
		uint32_t PCHK0_ERR_BE                    : 1;
		uint32_t PCHK0_ERR_RAW                   : 1;
		uint32_t PCHK0_ERR_RGB                   : 1;
		uint32_t PCHK0_ERR_YUV                   : 1;
		uint32_t PCHK1_ERR_FE0                   : 1;
		uint32_t PCHK1_ERR_FE1                   : 1;
		uint32_t PCHK1_ERR_BE                    : 1;
		uint32_t PCHK1_ERR_RAW                   : 1;
		uint32_t PCHK1_ERR_RGB                   : 1;
		uint32_t PCHK1_ERR_YUV                   : 1;
	} bits;
};

union REG_ISP_TOP_2 {
	uint32_t raw;
	struct {
		uint32_t FRAME_DONE_ENABLE_FE0           : 4;
		uint32_t FRAME_DONE_ENABLE_FE1           : 2;
		uint32_t FRAME_DONE_ENABLE_BE            : 2;
		uint32_t FRAME_DONE_ENABLE_POST          : 1;
		uint32_t SHAW_DONE_ENABLE_FE0            : 4;
		uint32_t SHAW_DONE_ENABLE_FE1            : 2;
		uint32_t SHAW_DONE_ENABLE_BE             : 2;
		uint32_t SHAW_DONE_ENABLE_POST           : 1;
		uint32_t FRAME_START_ENABLE_FE0          : 4;
		uint32_t FRAME_START_ENABLE_FE1          : 2;
		uint32_t FRAME_ERR_ENABLE                : 1;
		uint32_t PCHK_ERR_ENABLE                 : 1;
		uint32_t CMDQ_INT_ENABLE                 : 1;
		uint32_t LINE_INTP_ENABLE_FE0            : 1;
		uint32_t LINE_INTP_ENABLE_FE1            : 1;
		uint32_t LINE_INTP_ENABLE_POST           : 1;
		uint32_t INT_BRIDGE_LITE_ENABLE          : 1;
		uint32_t INT_DMA_ERR_ENABLE              : 1;
	} bits;
};

union REG_ISP_TOP_3 {
	uint32_t raw;
	struct {
		uint32_t TRIG_STR_FE0                    : 4;
		uint32_t TRIG_STR_FE1                    : 2;
		uint32_t TRIG_STR_BE                     : 2;
		uint32_t TRIG_STR_POST                   : 1;
		uint32_t PQ_UP_FE0                       : 4;
		uint32_t PQ_UP_FE1                       : 2;
		uint32_t _rsv_15                         : 1;
		uint32_t SHAW_UP_FE0                     : 4;
		uint32_t SHAW_UP_FE1                     : 2;
		uint32_t SHAW_UP_BE                      : 2;
		uint32_t SHAW_UP_POST                    : 1;
		uint32_t PQ_UP_BE                        : 2;
		uint32_t PQ_UP_POST                      : 1;
	} bits;
};

union REG_ISP_TOP_4 {
	uint32_t raw;
	struct {
		uint32_t TRIG_STR_SEL_FE0                : 4;
		uint32_t TRIG_STR_SEL_FE1                : 2;
		uint32_t TRIG_STR_SEL_BE                 : 2;
		uint32_t TRIG_STR_SEL_POST               : 1;
		uint32_t PQ_UP_SEL_FE0                   : 4;
		uint32_t PQ_UP_SEL_FE1                   : 2;
		uint32_t _rsv_15                         : 1;
		uint32_t SHAW_UP_SEL_FE0                 : 4;
		uint32_t SHAW_UP_SEL_FE1                 : 2;
		uint32_t SHAW_UP_SEL_BE                  : 2;
		uint32_t SHAW_UP_SEL_POST                : 1;
		uint32_t PQ_UP_SEL_BE                    : 2;
		uint32_t PQ_UP_SEL_POST                  : 1;
	} bits;
};

union REG_ISP_TOP_5 {
	uint32_t raw;
	struct {
		uint32_t DST2SC                          : 1;
		uint32_t DST2DMA                         : 1;
		uint32_t POST_OFFLINE                    : 1;
		uint32_t BE2DMA_ENABLE                   : 1;
		uint32_t FE02BE_ENABLE                   : 1;
		uint32_t FE12BE_ENABLE                   : 1;
		uint32_t BE2RAW_ENABLE                   : 1;
		uint32_t BE2YUV_422_ENABLE               : 1;
		uint32_t RAW2YUV_422_ENABLE              : 1;
	} bits;
};

union REG_ISP_TOP_6 {
	uint32_t raw;
	struct {
		uint32_t IMG_WIDTHM1                     : 16;
		uint32_t IMG_HEIGHTM1                    : 16;
	} bits;
};

union REG_ISP_TOP_7 {
	uint32_t raw;
	struct {
		uint32_t ISP_RST                         : 1;
		uint32_t CSI0_RST                        : 1;
		uint32_t CSI1_RST                        : 1;
		uint32_t CSI_BE_RST                      : 1;
		uint32_t CSI2_RST                        : 1;
		uint32_t AXI_RST                         : 1;
		uint32_t CMDQ_RST                        : 1;
		uint32_t APB_RST                         : 1;
	} bits;
};

union REG_ISP_TOP_8 {
	uint32_t raw;
	struct {
		uint32_t FE0_BLK_IDLE                    : 1;
		uint32_t FE1_BLK_IDLE                    : 1;
		uint32_t BE_BLK_IDLE                     : 1;
		uint32_t RAW_BLK_IDLE                    : 1;
		uint32_t RGB_BLK_IDLE                    : 1;
		uint32_t YUV_BLK_IDLE                    : 1;
		uint32_t RDMA_IDLE                       : 1;
		uint32_t WDMA_IDLE                       : 1;
	} bits;
};

union REG_ISP_TOP_9 {
	uint32_t raw;
	struct {
		uint32_t PQ_DONE_FE0                     : 4;
		uint32_t PQ_DONE_FE1                     : 2;
		uint32_t PQ_DONE_BE                      : 2;
		uint32_t PQ_DONE_POST                    : 1;
		uint32_t SHAW_DONE_AWB                   : 1;
		uint32_t FRAME_DONE_AWB                  : 1;
	} bits;
};

union REG_ISP_TOP_A {
	uint32_t raw;
	struct {
		uint32_t PQ_DONE_ENABLE_FE0              : 4;
		uint32_t PQ_DONE_ENABLE_FE1              : 2;
		uint32_t PQ_DONE_ENABLE_BE               : 2;
		uint32_t PQ_DONE_ENABLE_POST             : 1;
		uint32_t SHAW_DONE_ENABLE_AWB            : 1;
		uint32_t FRAME_DONE_ENABLE_AWB           : 1;
	} bits;
};

union REG_ISP_TOP_B {
	uint32_t raw;
	struct {
		uint32_t BLK_IDLE_CSI0_EN                : 1;
		uint32_t BLK_IDLE_CSI1_EN                : 1;
		uint32_t BLK_IDLE_CSI2_EN                : 1;
		uint32_t BLK_IDLE_BE_EN                  : 1;
		uint32_t BLK_IDLE_POST_EN                : 1;
		uint32_t BLK_IDLE_APB_EN                 : 1;
		uint32_t BLK_IDLE_AXI_EN                 : 1;
		uint32_t BLK_IDLE_CMDQ_EN                : 1;
	} bits;
};

union REG_ISP_TOP_C {
	uint32_t raw;
	struct {
		uint32_t SVN_REVISION                    : 32;
	} bits;
};

union REG_ISP_TOP_D {
	uint32_t raw;
	struct {
		uint32_t UNIX_TIMESTAMP                  : 32;
	} bits;
};

union REG_ISP_TOP_10 {
	uint32_t raw;
	struct {
		uint32_t DBUS0                           : 32;
	} bits;
};

union REG_ISP_TOP_11 {
	uint32_t raw;
	struct {
		uint32_t DBUS1                           : 32;
	} bits;
};

union REG_ISP_TOP_12 {
	uint32_t raw;
	struct {
		uint32_t DBUS2                           : 32;
	} bits;
};

union REG_ISP_TOP_13 {
	uint32_t raw;
	struct {
		uint32_t DBUS3                           : 32;
	} bits;
};

union REG_ISP_TOP_14 {
	uint32_t raw;
	struct {
		uint32_t FORCE_ISP_INT                   : 1;
		uint32_t FORCE_ISP_INT_EN                : 1;
	} bits;
};

union REG_ISP_TOP_15 {
	uint32_t raw;
	struct {
		uint32_t DUMMY                           : 28;
		uint32_t DBUS_SEL                        : 4;
	} bits;
};

union REG_ISP_TOP_16 {
	uint32_t raw;
	struct {
		uint32_t FE1_RGBMAP_L_ENABLE             : 1;
		uint32_t FE1_LMAP_L_ENABLE               : 1;
		uint32_t FE1_RGBMAP_S_ENABLE             : 1;
		uint32_t FE1_LMAP_S_ENABLE               : 1;
		uint32_t FE1_BLC_L_ENABLE                : 1;
		uint32_t FE1_BLC_S_ENABLE                : 1;
		uint32_t FE1_CROP_L_ENABLE               : 1;
		uint32_t FE1_CROP_S_ENABLE               : 1;
		uint32_t FE1_LSCR_L_ENABLE               : 1;
		uint32_t FE1_LSCR_S_ENABLE               : 1;
		uint32_t FE0_RGBMAP_L_ENABLE             : 1;
		uint32_t FE0_LMAP_L_ENABLE               : 1;
		uint32_t FE0_RGBMAP_S_ENABLE             : 1;
		uint32_t FE0_LMAP_S_ENABLE               : 1;
		uint32_t FE0_BLC_L_ENABLE                : 1;
		uint32_t FE0_BLC_S_ENABLE                : 1;
		uint32_t FE0_CROP_L_ENABLE               : 1;
		uint32_t FE0_CROP_S_ENABLE               : 1;
		uint32_t FE0_LSCR_L_ENABLE               : 1;
		uint32_t FE0_LSCR_S_ENABLE               : 1;
	} bits;
};

union REG_ISP_TOP_17 {
	uint32_t raw;
	struct {
		uint32_t RAW_CROP_L_ENABLE               : 1;
		uint32_t RAW_CROP_S_ENABLE               : 1;
		uint32_t RAW_BNR_ENABLE                  : 1;
		uint32_t RAW_CFA_ENABLE                  : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t RGB_LSCM_ENABLE                 : 1;
		uint32_t RGB_FUSION_ENABLE               : 1;
		uint32_t RGB_LTM_ENABLE                  : 1;
		uint32_t RGB_AWB_ENABLE                  : 1;
		uint32_t RGB_MANR_ENABLE                 : 1;
		uint32_t RGB_HISTV_ENABLE                : 1;
		uint32_t RGB_GAMMA_ENABLE                : 1;
		uint32_t RGB_DHZ_ENABLE                  : 1;
		uint32_t RGB_RGBDITHER_ENABLE            : 1;
		uint32_t RGB_CLUT_ENABLE                 : 1;
		uint32_t RGB_DCI_ENABLE                  : 1;
		uint32_t RGB_PREYEE_ENABLE               : 1;
		uint32_t RGB_R2Y4_ENABLE                 : 1;
		uint32_t RGB_IRM_ENABLE                  : 1;
		uint32_t RGB_CCM_0_ENABLE                : 1;
		uint32_t RGB_CCM_1_ENABLE                : 1;
		uint32_t RGB_CCM_2_ENABLE                : 1;
		uint32_t RGB_CCM_3_ENABLE                : 1;
		uint32_t RGB_CCM_4_ENABLE                : 1;
		uint32_t RGB_CACP_ENABLE                 : 1;
	} bits;
};

union REG_ISP_TOP_18 {
	uint32_t raw;
	struct {
		uint32_t YUV_3DNR_ENABLE                 : 1;
		uint32_t YUV_YNR_ENABLE                  : 1;
		uint32_t YUV_CNR_ENABLE                  : 1;
		uint32_t YUV_EE_ENABLE                   : 1;
		uint32_t YUV_CROP_Y_ENABLE               : 1;
		uint32_t YUV_CROP_C_ENABLE               : 1;
		uint32_t YUV_YCURVE_ENABLE               : 1;
		uint32_t YUV_CA2_ENABLE                  : 1;
	} bits;
};

union REG_ISP_TOP_19 {
	uint32_t raw;
	struct {
		uint32_t BE_BLC_L_ENABLE                 : 1;
		uint32_t BE_FPN_L_ENABLE                 : 1;
		uint32_t BE_DPC_L_ENABLE                 : 1;
		uint32_t BE_WBG_L_ENABLE                 : 1;
		uint32_t BE_IR_PRE_PROC_L_ENABLE         : 1;
		uint32_t BE_IRAE_L_ENABLE                : 1;
		uint32_t BE_BLC_S_ENABLE                 : 1;
		uint32_t BE_FPN_S_ENABLE                 : 1;
		uint32_t BE_DPC_S_ENABLE                 : 1;
		uint32_t BE_WBG_S_ENABLE                 : 1;
		uint32_t BE_IR_PRE_PROC_S_ENABLE         : 1;
		uint32_t BE_IRAE_S_ENABLE                : 1;
		uint32_t BE_AF_ENABLE                    : 1;
		uint32_t BE_AE_L_ENABLE                  : 1;
		uint32_t BE_AWB_L_ENABLE                 : 1;
		uint32_t BE_HIST_L_ENABLE                : 1;
		uint32_t BE_AE_S_ENABLE                  : 1;
		uint32_t BE_AWB_S_ENABLE                 : 1;
		uint32_t BE_HIST_S_ENABLE                : 1;
		uint32_t BE_GMS_ENABLE                   : 1;
		uint32_t BE_CROP_L_ENABLE                : 1;
		uint32_t BE_CROP_S_ENABLE                : 1;
		uint32_t BE_LSCR_L_ENABLE                : 1;
		uint32_t BE_LSCR_S_ENABLE                : 1;
	} bits;
};

union REG_ISP_TOP_1A {
	uint32_t raw;
	struct {
		uint32_t CMDQ_TSK_EN                     : 8;
		uint32_t CMDQ_FLAG_SEL                   : 2;
		uint32_t CMDQ_TASK_SEL                   : 2;
	} bits;
};

union REG_ISP_TOP_1B {
	uint32_t raw;
	struct {
		uint32_t CMDQ_TSK_TRIG                   : 8;
	} bits;
};

union REG_ISP_TOP_1C {
	uint32_t raw;
	struct {
		uint32_t TRIG_STR_CNT                    : 4;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_WDMA_COM_SHADOW_RD_SEL   {
	uint32_t raw;
	struct {
		uint32_t SHADOW_RD_SEL                   : 1;
	} bits;
};

union REG_ISP_WDMA_COM_NORM_STATUS0 {
	uint32_t raw;
	struct {
		uint32_t ABORT_DONE                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t ERROR_AXI                       : 1;
		uint32_t ERROR_DMI                       : 1;
		uint32_t ERROR_FULL                      : 1;
		uint32_t _rsv_7                          : 1;
		uint32_t ERROR_ID                        : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t DMA_VERSION                     : 16;
	} bits;
};

union REG_ISP_WDMA_COM_NORM_STATUS1 {
	uint32_t raw;
	struct {
		uint32_t ID_DONE                         : 32;
	} bits;
};

union REG_ISP_WDMA_COM_NORM_PERF  {
	uint32_t raw;
	struct {
		uint32_t BWLWIN                          : 10;
		uint32_t BWLTXN                          : 6;
		uint32_t QOSO_TH                         : 4;
		uint32_t QOSO_EN                         : 1;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_RDMA_COM_SHADOW_RD_SEL   {
	uint32_t raw;
	struct {
		uint32_t SHADOW_RD_SEL                   : 1;
	} bits;
};

union REG_ISP_RDMA_COM_NORM_STATUS0 {
	uint32_t raw;
	struct {
		uint32_t ABORT_DONE                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t ERROR_AXI                       : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t ERROR_ID                        : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t DMA_VERSION                     : 16;
	} bits;
};

union REG_ISP_RDMA_COM_NORM_STATUS1 {
	uint32_t raw;
	struct {
		uint32_t ID_DONE                         : 32;
	} bits;
};

union REG_ISP_RDMA_COM_NORM_PERF  {
	uint32_t raw;
	struct {
		uint32_t BWLWIN                          : 10;
		uint32_t BWLTXN                          : 6;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_CSI_BDG_LITE_BDG_TOP_CTRL {
	uint32_t raw;
	struct {
		uint32_t CSI_MODE                        : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t CH_NUM                          : 2;
		uint32_t CH0_DMA_WR_ENABLE               : 1;
		uint32_t CH1_DMA_WR_ENABLE               : 1;
		uint32_t CH2_DMA_WR_ENABLE               : 1;
		uint32_t Y_ONLY                          : 1;
		uint32_t _rsv_10                         : 1;
		uint32_t VS_POL                          : 1;
		uint32_t HS_POL                          : 1;
		uint32_t RESET_MODE                      : 1;
		uint32_t VS_MODE                         : 1;
		uint32_t ABORT_MODE                      : 1;
		uint32_t RESET                           : 1;
		uint32_t ABORT                           : 1;
		uint32_t CH3_DMA_WR_ENABLE               : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t CSI_ENABLE                      : 1;
		uint32_t _rsv_25                         : 3;
		uint32_t SHDW_READ_SEL                   : 1;
		uint32_t _rsv_29                         : 2;
		uint32_t CSI_UP_REG                      : 1;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_BDG_INTERRUPT_CTRL_0 {
	uint32_t raw;
	struct {
		uint32_t CH0_VS_INT_EN                   : 1;
		uint32_t CH0_TRIG_INT_EN                 : 1;
		uint32_t CH0_DROP_INT_EN                 : 1;
		uint32_t CH0_SIZE_ERROR_INT_EN           : 1;
		uint32_t CH0_FRAME_DONE_EN               : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t CH1_VS_INT_EN                   : 1;
		uint32_t CH1_TRIG_INT_EN                 : 1;
		uint32_t CH1_DROP_INT_EN                 : 1;
		uint32_t CH1_SIZE_ERROR_INT_EN           : 1;
		uint32_t CH1_FRAME_DONE_EN               : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t CH2_VS_INT_EN                   : 1;
		uint32_t CH2_TRIG_INT_EN                 : 1;
		uint32_t CH2_DROP_INT_EN                 : 1;
		uint32_t CH2_SIZE_ERROR_INT_EN           : 1;
		uint32_t CH2_FRAME_DONE_EN               : 1;
		uint32_t _rsv_21                         : 3;
		uint32_t CH3_VS_INT_EN                   : 1;
		uint32_t CH3_TRIG_INT_EN                 : 1;
		uint32_t CH3_DROP_INT_EN                 : 1;
		uint32_t CH3_SIZE_ERROR_INT_EN           : 1;
		uint32_t CH3_FRAME_DONE_EN               : 1;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_BDG_INTERRUPT_CTRL_1 {
	uint32_t raw;
	struct {
		uint32_t LINE_INTP_EN                    : 1;
		uint32_t FIFO_OVERFLOW_INT_EN            : 1;
		uint32_t DMA_ERROR_INTP_EN               : 1;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_FRAME_VLD {
	uint32_t raw;
	struct {
		uint32_t FRAME_VLD_CH0                   : 1;
		uint32_t FRAME_VLD_CH1                   : 1;
		uint32_t FRAME_VLD_CH2                   : 1;
		uint32_t FRAME_VLD_CH3                   : 1;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH0_SIZE {
	uint32_t raw;
	struct {
		uint32_t CH0_FRAME_WIDTHM1               : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH0_FRAME_HEIGHTM1              : 13;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH1_SIZE {
	uint32_t raw;
	struct {
		uint32_t CH1_FRAME_WIDTHM1               : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH1_FRAME_HEIGHTM1              : 13;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH2_SIZE {
	uint32_t raw;
	struct {
		uint32_t CH2_FRAME_WIDTHM1               : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH2_FRAME_HEIGHTM1              : 13;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH3_SIZE {
	uint32_t raw;
	struct {
		uint32_t CH3_FRAME_WIDTHM1               : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH3_FRAME_HEIGHTM1              : 13;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH0_CROP_EN {
	uint32_t raw;
	struct {
		uint32_t CH0_CROP_EN                     : 1;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH0_HORZ_CROP {
	uint32_t raw;
	struct {
		uint32_t CH0_HORZ_CROP_START             : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH0_HORZ_CROP_END               : 13;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH0_VERT_CROP {
	uint32_t raw;
	struct {
		uint32_t CH0_VERT_CROP_START             : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH0_VERT_CROP_END               : 13;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH1_CROP_EN {
	uint32_t raw;
	struct {
		uint32_t CH1_CROP_EN                     : 1;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH1_HORZ_CROP {
	uint32_t raw;
	struct {
		uint32_t CH1_HORZ_CROP_START             : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH1_HORZ_CROP_END               : 13;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH1_VERT_CROP {
	uint32_t raw;
	struct {
		uint32_t CH1_VERT_CROP_START             : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH1_VERT_CROP_END               : 13;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH2_CROP_EN {
	uint32_t raw;
	struct {
		uint32_t CH2_CROP_EN                     : 1;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH2_HORZ_CROP {
	uint32_t raw;
	struct {
		uint32_t CH2_HORZ_CROP_START             : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH2_HORZ_CROP_END               : 13;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH2_VERT_CROP {
	uint32_t raw;
	struct {
		uint32_t CH2_VERT_CROP_START             : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH2_VERT_CROP_END               : 13;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH3_CROP_EN {
	uint32_t raw;
	struct {
		uint32_t CH3_CROP_EN                     : 1;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH3_HORZ_CROP {
	uint32_t raw;
	struct {
		uint32_t CH3_HORZ_CROP_START             : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH3_HORZ_CROP_END               : 13;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH3_VERT_CROP {
	uint32_t raw;
	struct {
		uint32_t CH3_VERT_CROP_START             : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t CH3_VERT_CROP_END               : 13;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_LINE_INTP_HEIGHT {
	uint32_t raw;
	struct {
		uint32_t LINE_INTP_HEIGHTM1              : 13;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH0_DEBUG_0 {
	uint32_t raw;
	struct {
		uint32_t CH0_PXL_CNT                     : 32;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH0_DEBUG_1 {
	uint32_t raw;
	struct {
		uint32_t CH0_LINE_CNT                    : 16;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH0_DEBUG_2 {
	uint32_t raw;
	struct {
		uint32_t CH0_VS_CNT                      : 16;
		uint32_t CH0_TRIG_CNT                    : 16;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH0_DEBUG_3 {
	uint32_t raw;
	struct {
		uint32_t CH0_TOT_BLK_IDLE                : 1;
		uint32_t CH0_TOT_DMA_IDLE                : 1;
		uint32_t CH0_BDG_DMA_IDLE                : 1;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH1_DEBUG_0 {
	uint32_t raw;
	struct {
		uint32_t CH1_PXL_CNT                     : 32;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH1_DEBUG_1 {
	uint32_t raw;
	struct {
		uint32_t CH1_LINE_CNT                    : 16;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH1_DEBUG_2 {
	uint32_t raw;
	struct {
		uint32_t CH1_VS_CNT                      : 16;
		uint32_t CH1_TRIG_CNT                    : 16;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH1_DEBUG_3 {
	uint32_t raw;
	struct {
		uint32_t CH1_TOT_BLK_IDLE                : 1;
		uint32_t CH1_TOT_DMA_IDLE                : 1;
		uint32_t CH1_BDG_DMA_IDLE                : 1;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH2_DEBUG_0 {
	uint32_t raw;
	struct {
		uint32_t CH2_PXL_CNT                     : 32;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH2_DEBUG_1 {
	uint32_t raw;
	struct {
		uint32_t CH2_LINE_CNT                    : 16;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH2_DEBUG_2 {
	uint32_t raw;
	struct {
		uint32_t CH2_VS_CNT                      : 16;
		uint32_t CH2_TRIG_CNT                    : 16;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH2_DEBUG_3 {
	uint32_t raw;
	struct {
		uint32_t CH2_TOT_BLK_IDLE                : 1;
		uint32_t CH2_TOT_DMA_IDLE                : 1;
		uint32_t CH2_BDG_DMA_IDLE                : 1;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH3_DEBUG_0 {
	uint32_t raw;
	struct {
		uint32_t CH3_PXL_CNT                     : 32;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH3_DEBUG_1 {
	uint32_t raw;
	struct {
		uint32_t CH3_LINE_CNT                    : 16;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH3_DEBUG_2 {
	uint32_t raw;
	struct {
		uint32_t CH3_VS_CNT                      : 16;
		uint32_t CH3_TRIG_CNT                    : 16;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_CH3_DEBUG_3 {
	uint32_t raw;
	struct {
		uint32_t CH3_TOT_BLK_IDLE                : 1;
		uint32_t CH3_TOT_DMA_IDLE                : 1;
		uint32_t CH3_BDG_DMA_IDLE                : 1;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_INTERRUPT_STATUS_0 {
	uint32_t raw;
	struct {
		uint32_t CH0_FRAME_DROP_INT              : 1;
		uint32_t CH0_VS_INT                      : 1;
		uint32_t CH0_TRIG_INT                    : 1;
		uint32_t CH0_FRAME_DONE                  : 1;
		uint32_t CH0_FRAME_WIDTH_GT_INT          : 1;
		uint32_t CH0_FRAME_WIDTH_LS_INT          : 1;
		uint32_t CH0_FRAME_HEIGHT_GT_INT         : 1;
		uint32_t CH0_FRAME_HEIGHT_LS_INT         : 1;
		uint32_t CH1_FRAME_DROP_INT              : 1;
		uint32_t CH1_VS_INT                      : 1;
		uint32_t CH1_TRIG_INT                    : 1;
		uint32_t CH1_FRAME_DONE                  : 1;
		uint32_t CH1_FRAME_WIDTH_GT_INT          : 1;
		uint32_t CH1_FRAME_WIDTH_LS_INT          : 1;
		uint32_t CH1_FRAME_HEIGHT_GT_INT         : 1;
		uint32_t CH1_FRAME_HEIGHT_LS_INT         : 1;
		uint32_t CH2_FRAME_DROP_INT              : 1;
		uint32_t CH2_VS_INT                      : 1;
		uint32_t CH2_TRIG_INT                    : 1;
		uint32_t CH2_FRAME_DONE                  : 1;
		uint32_t CH2_FRAME_WIDTH_GT_INT          : 1;
		uint32_t CH2_FRAME_WIDTH_LS_INT          : 1;
		uint32_t CH2_FRAME_HEIGHT_GT_INT         : 1;
		uint32_t CH2_FRAME_HEIGHT_LS_INT         : 1;
		uint32_t CH3_FRAME_DROP_INT              : 1;
		uint32_t CH3_VS_INT                      : 1;
		uint32_t CH3_TRIG_INT                    : 1;
		uint32_t CH3_FRAME_DONE                  : 1;
		uint32_t CH3_FRAME_WIDTH_GT_INT          : 1;
		uint32_t CH3_FRAME_WIDTH_LS_INT          : 1;
		uint32_t CH3_FRAME_HEIGHT_GT_INT         : 1;
		uint32_t CH3_FRAME_HEIGHT_LS_INT         : 1;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_INTERRUPT_STATUS_1 {
	uint32_t raw;
	struct {
		uint32_t FIFO_OVERFLOW_INT               : 1;
		uint32_t FRAME_RESOLUTION_OVER_MAX_INT   : 1;
		uint32_t _rsv_2                          : 1;
		uint32_t LINE_INTP_INT                   : 1;
		uint32_t DMA_ERROR_INT                   : 1;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_BDG_DEBUG {
	uint32_t raw;
	struct {
		uint32_t RING_BUFF_IDLE                  : 1;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_WR_URGENT_CTRL {
	uint32_t raw;
	struct {
		uint32_t WR_NEAR_OVERFLOW_THRESHOLD      : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t WR_SAFE_THRESHOLD               : 13;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_RD_URGENT_CTRL {
	uint32_t raw;
	struct {
		uint32_t RD_NEAR_OVERFLOW_THRESHOLD      : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t RD_SAFE_THRESHOLD               : 13;
	} bits;
};

union REG_ISP_CSI_BDG_LITE_DUMMY {
	uint32_t raw;
	struct {
		uint32_t DUMMY_IN                        : 16;
		uint32_t DUMMY_OUT                       : 16;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_ISP_DMA_ARR_SYSTEM {
	uint32_t raw;
	struct {
		uint32_t QOS_SEL                         : 1;
		uint32_t SW_QOS                          : 1;
		uint32_t ENABLE_SEL                      : 1;
		uint32_t SW_ENABLE                       : 1;
		uint32_t GROUP_SEL                       : 4;
		uint32_t BASEH                           : 1;
	} bits;
};

union REG_ISP_DMA_ARR_BASE {
	uint32_t raw;
	struct {
		uint32_t BASEL                           : 32;
	} bits;
};

union REG_ISP_DMA_ARR_SEGLEN {
	uint32_t raw;
	struct {
		uint32_t SEGLEN                          : 20;
	} bits;
};

union REG_ISP_DMA_ARR_STRIDE {
	uint32_t raw;
	struct {
		uint32_t STRIDE                          : 20;
	} bits;
};

union REG_ISP_DMA_ARR_SEGNUM {
	uint32_t raw;
	struct {
		uint32_t SEGNUM                          : 13;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_IR_PROC_CTRL {
	uint32_t raw;
	struct {
		uint32_t _rsv_0                          : 4;
		uint32_t WDMI_EN_LE                      : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t WDMI_EN_SE                      : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t INPUT_FORMAT                    : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t DMA_WR_MODE                     : 1;
		uint32_t _rsv_17                         : 3;
		uint32_t DMA_WR_MSB                      : 1;
		uint32_t _rsv_21                         : 10;
		uint32_t SHDW_READ_SEL                   : 1;
	} bits;
};

union REG_IR_PROC_DUMMY {
	uint32_t raw;
	struct {
		uint32_t DUMMY_RW                        : 16;
		uint32_t DUMMY_RO                        : 16;
	} bits;
};

/******************************************/
/*           Module Definition            */
/******************************************/
union REG_FBCE_0 {
	uint32_t raw;
	struct {
		uint32_t FBCE_EN                         : 1;
		uint32_t FLOOR_MD_EN                     : 1;
		uint32_t CPLX_SHIFT                      : 2;
		uint32_t DC_CPLX_GAIN                    : 4;
		uint32_t PEN_POS_SHIFT                   : 2;
		uint32_t RC_TYPE                         : 1;
		uint32_t _rsv_11                         : 1;
		uint32_t LINE_BIT_GUARD                  : 8;
		uint32_t _rsv_20                         : 4;
		uint32_t RC_STRICT_CU_IDX                : 7;
		uint32_t RC_STRICT_RC_EN                 : 1;
	} bits;
};

union REG_FBCE_1 {
	uint32_t raw;
	struct {
		uint32_t Y_TOTAL_LINE_BIT_BUDGET         : 15;
		uint32_t _rsv_15                         : 1;
		uint32_t C_TOTAL_LINE_BIT_BUDGET         : 15;
	} bits;
};

union REG_FBCE_2 {
	uint32_t raw;
	struct {
		uint32_t AMPLE_STATE_THR                 : 9;
		uint32_t _rsv_9                          : 3;
		uint32_t AVG_SHARE_SHIFT                 : 4;
		uint32_t MAX_Q_STEP                      : 9;
	} bits;
};

union REG_FBCE_3 {
	uint32_t raw;
	struct {
		uint32_t Y_BIT_STREAM_SIZE               : 22;
		uint32_t _rsv_22                         : 3;
		uint32_t Y_STREAM_TAG                    : 3;
	} bits;
};

union REG_FBCE_4 {
	uint32_t raw;
	struct {
		uint32_t C_BIT_STREAM_SIZE               : 22;
		uint32_t _rsv_22                         : 3;
		uint32_t C_STREAM_TAG                    : 3;
	} bits;
};

union REG_FBCE_5 {
	uint32_t raw;
	struct {
		uint32_t Y_DMA_SIZE                      : 22;
	} bits;
};

union REG_FBCE_6 {
	uint32_t raw;
	struct {
		uint32_t C_DMA_SIZE                      : 22;
	} bits;
};

#endif // _REG_FIELDS_H_
