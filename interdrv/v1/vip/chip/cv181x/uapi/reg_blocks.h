#ifndef _REG_BLOCKS_H_
#define _REG_BLOCKS_H_

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_PRE_RAW_BE_T {
	union REG_PRE_RAW_BE_TOP_CTRL           TOP_CTRL;
	union REG_PRE_RAW_BE_UP_PQ_EN           UP_PQ_EN;
	union REG_PRE_RAW_BE_RDMI_SIZE          RDMI_SIZE;
	union REG_PRE_RAW_BE_CROP_SIZE_LE       CROP_SIZE_LE;
	union REG_PRE_RAW_BE_RDMI_DPCM          RDMI_DPCM;
	uint32_t                                _resv_0x14[3];
	union REG_PRE_RAW_BE_FLOW_CTRL          FLOW_CTRL;
	uint32_t                                _resv_0x24[3];
	union REG_PRE_RAW_BE_PRE_RAW_DUMMY      PRE_RAW_DUMMY;
	union REG_PRE_RAW_BE_PRE_RAW_POST_NO_RSPD_CYC  PRE_RAW_POST_NO_RSPD_CYC;
	union REG_PRE_RAW_BE_PRE_RAW_DEBUG_STATE  PRE_RAW_DEBUG_STATE;
	union REG_PRE_RAW_BE_DEBUG_INFO         BE_INFO;
	union REG_PRE_RAW_BE_LINE_BALANCE_CTRL  LINE_BALANCE_CTRL;
	union REG_PRE_RAW_BE_IP_INPUT_SEL       IP_INPUT_SEL;
	uint32_t                                _resv_0x48[2];
	union REG_PRE_RAW_BE_RDMI_DEBUG_0       RDMI_DEBUG_0;
	union REG_PRE_RAW_BE_RDMI_DEBUG_1       RDMI_DEBUG_1;
	union REG_PRE_RAW_BE_IP_CONNECTION_DEBUG_0  IP_CONNECTION_DEBUG_0;
	union REG_PRE_RAW_BE_IP_CONNECTION_DEBUG_1  IP_CONNECTION_DEBUG_1;
	union REG_PRE_RAW_BE_IP_CONNECTION_DEBUG_2  IP_CONNECTION_DEBUG_2;
	union REG_PRE_RAW_BE_IP_CONNECTION_DEBUG_3  IP_CONNECTION_DEBUG_3;
	union REG_PRE_RAW_BE_IDLE_INFO          BE_IDLE_INFO;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_FPN_T {
	union REG_ISP_FPN_STATUS                STATUS;
	union REG_ISP_FPN_GRACE_RESET           GRACE_RESET;
	union REG_ISP_FPN_MONITOR               MONITOR;
	union REG_ISP_FPN_ENABLE                ENABLE;
	union REG_ISP_FPN_MAP_ENABLE            MAP_ENABLE;
	union REG_ISP_FPN_FLOW                  FLOW;
	uint32_t                                _resv_0x18[1];
	union REG_ISP_FPN_MONITOR_SELECT        MONITOR_SELECT;
	union REG_ISP_FPN_LOCATION              LOCATION;
	uint32_t                                _resv_0x24[8];
	union REG_ISP_FPN_MEM_SELECT            MEM_SELECT;
	union REG_ISP_FPN_SECTION_INFO_0        SECTION_INFO_0;
	union REG_ISP_FPN_SECTION_INFO_1        SECTION_INFO_1;
	union REG_ISP_FPN_SECTION_INFO_2        SECTION_INFO_2;
	union REG_ISP_FPN_SECTION_INFO_3        SECTION_INFO_3;
	uint32_t                                _resv_0x58[1];
	union REG_ISP_FPN_SECTION_INFO_4        SECTION_INFO_4;
	union REG_ISP_FPN_DEBUG                 DEBUG;
	union REG_ISP_FPN_IMG_BAYERID           IMG_BAYERID;
	union REG_ISP_FPN_DUMMY                 DUMMY;
	uint32_t                                _resv_0x6c[1];
	union REG_ISP_FPN_PROG_DATA             PROG_DATA;
	union REG_ISP_FPN_PROG_CTRL             PROG_CTRL;
	union REG_ISP_FPN_PROG_MAX              PROG_MAX;
	uint32_t                                _resv_0x7c[1];
	union REG_ISP_FPN_MEM_SW_MODE           SW_MODE;
	union REG_ISP_FPN_MEM_SW_RADDR          SW_RADDR;
	union REG_ISP_FPN_MEM_SW_RDATA_OFFSET   SW_RDATA_OFFSET;
	union REG_ISP_FPN_MEM_SW_RDATA_OFFSET_BG  SW_RDATA_OFFSET_BG;
	union REG_ISP_FPN_MEM_SW_RDATA_GAIN     SW_RDATA_GAIN;
	union REG_ISP_FPN_MEM_SW_RDATA_GAIN_BG  SW_RDATA_GAIN_BG;
	uint32_t                                _resv_0x98[2];
	union REG_ISP_FPN_DBG                   DBG;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_PREPROCESS_T {
	union REG_ISP_PREPROCESS_IR_PREPROC_CTRL  IR_PREPROC_CTRL;
	union REG_ISP_PREPROCESS_IR_PREPROC_PROG_CTRL  IR_PREPROC_PROG_CTRL;
	union REG_ISP_PREPROCESS_IR_PREPROC_PROG_ST_ADDR  IR_PREPROC_PROG_ST_ADDR;
	union REG_ISP_PREPROCESS_IR_PREPROC_PROG_DATA  IR_PREPROC_PROG_DATA;
	union REG_ISP_PREPROCESS_IR_PREPROC_MEM_SW_MODE  IR_PREPROC_SW_MODE;
	union REG_ISP_PREPROCESS_IR_PREPROC_MEM_SW_RDATA  IR_PREPROC_SW_RDATA;
	union REG_ISP_PREPROCESS_IR_PREPROC_MEM_RATIO_PROG_DATA  IR_PREPROC_MEM_RATIO_PROG_DATA;
	uint32_t                                _resv_0x1c[2];
	union REG_ISP_PREPROCESS_IR_PREPROC_DBG  IR_PREPROC_DBG;
	union REG_ISP_PREPROCESS_IR_PREPROC_DMY0  IR_PREPROC_DMY0;
	union REG_ISP_PREPROCESS_IR_PREPROC_DMY1  IR_PREPROC_DMY1;
	union REG_ISP_PREPROCESS_IR_PREPROC_DMY_R  IR_PREPROC_DMY_R;
	union REG_ISP_PREPROCESS_IR_PREPROC_RATIO_0  IR_PREPROC_RATIO_0;
	union REG_ISP_PREPROCESS_IR_PREPROC_RATIO_1  IR_PREPROC_RATIO_1;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_IR_WDMA_PROC_T {
	union REG_IR_WDMA_PROC_IR_PROC_CTRL     IR_PROC_CTRL;
	union REG_IR_WDMA_PROC_GAMMA_CURVE_PROG_DATA  GAMMA_CURVE_PROG_DATA;
	union REG_IR_WDMA_PROC_GAMMA_CURVE_PROG_CTRL  GAMMA_CURVE_PROG_CTRL;
	union REG_IR_WDMA_PROC_GAMMA_CURVE_PROG_MAX  GAMMA_CURVE_PROG_MAX;
	union REG_IR_WDMA_PROC_GAMMA_CURVE_CTRL  GAMMA_CURVE_CTRL;
	union REG_IR_WDMA_PROC_GAMMA_CURVE_MEM_SW_MODE  GAMMA_CURVE_SW_MODE;
	union REG_IR_WDMA_PROC_GAMMA_CURVE_MEM_SW_RADDR  GAMMA_CURVE_SW_RADDR;
	union REG_IR_WDMA_PROC_GAMMA_CURVE_MEM_SW_RDATA  GAMMA_CURVE_SW_RDATA;
	uint32_t                                _resv_0x20[4];
	union REG_IR_WDMA_PROC_IR_PROC_DUMMY    IR_PROC_DUMMY;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_AE_HIST_T {
	union REG_ISP_AE_HIST_AE_HIST_STATUS    AE_HIST_STATUS;
	union REG_ISP_AE_HIST_AE_HIST_GRACE_RESET  AE_HIST_GRACE_RESET;
	union REG_ISP_AE_HIST_AE_HIST_MONITOR   AE_HIST_MONITOR;
	union REG_ISP_AE_HIST_AE_HIST_BYPASS    AE_HIST_BYPASS;
	union REG_ISP_AE_HIST_AE_KICKOFF        AE_KICKOFF;
	union REG_ISP_AE_HIST_AE_HIST_ENABLE    AE_HIST_ENABLE;
	union REG_ISP_AE_HIST_STS_AE_OFFSETX    STS_AE_OFFSETX;
	union REG_ISP_AE_HIST_STS_AE_OFFSETY    STS_AE_OFFSETY;
	union REG_ISP_AE_HIST_STS_AE_NUMXM1     STS_AE_NUMXM1;
	union REG_ISP_AE_HIST_STS_AE_NUMYM1     STS_AE_NUMYM1;
	union REG_ISP_AE_HIST_STS_AE_WIDTH      STS_AE_WIDTH;
	union REG_ISP_AE_HIST_STS_AE_HEIGHT     STS_AE_HEIGHT;
	union REG_ISP_AE_HIST_STS_AE_STS_DIV    STS_AE_STS_DIV;
	union REG_ISP_AE_HIST_STS_HIST_MODE     STS_HIST_MODE;
	uint32_t                                _resv_0x38[1];
	union REG_ISP_AE_HIST_AE_HIST_MONITOR_SELECT  AE_HIST_MONITOR_SELECT;
	union REG_ISP_AE_HIST_AE_HIST_LOCATION  AE_HIST_LOCATION;
	uint32_t                                _resv_0x44[1];
	union REG_ISP_AE_HIST_STS_IR_AE_OFFSETX  STS_IR_AE_OFFSETX;
	union REG_ISP_AE_HIST_STS_IR_AE_OFFSETY  STS_IR_AE_OFFSETY;
	union REG_ISP_AE_HIST_STS_IR_AE_NUMXM1  STS_IR_AE_NUMXM1;
	union REG_ISP_AE_HIST_STS_IR_AE_NUMYM1  STS_IR_AE_NUMYM1;
	union REG_ISP_AE_HIST_STS_IR_AE_WIDTH   STS_IR_AE_WIDTH;
	union REG_ISP_AE_HIST_STS_IR_AE_HEIGHT  STS_IR_AE_HEIGHT;
	union REG_ISP_AE_HIST_STS_IR_AE_STS_DIV  STS_IR_AE_STS_DIV;
	uint32_t                                _resv_0x64[1];
	union REG_ISP_AE_HIST_AE_HIST_BAYER_STARTING  AE_HIST_BAYER_STARTING;
	union REG_ISP_AE_HIST_AE_HIST_DUMMY     AE_HIST_DUMMY;
	union REG_ISP_AE_HIST_AE_HIST_CHECKSUM  AE_HIST_CHECKSUM;
	union REG_ISP_AE_HIST_WBG_4             WBG_4;
	union REG_ISP_AE_HIST_WBG_5             WBG_5;
	union REG_ISP_AE_HIST_WBG_6             WBG_6;
	union REG_ISP_AE_HIST_WBG_7             WBG_7;
	uint32_t                                _resv_0x84[7];
	union REG_ISP_AE_HIST_DMI_ENABLE        DMI_ENABLE;
	uint32_t                                _resv_0xa4[3];
	union REG_ISP_AE_HIST_AE_FACE0_LOCATION  AE_FACE0_LOCATION;
	union REG_ISP_AE_HIST_AE_FACE1_LOCATION  AE_FACE1_LOCATION;
	union REG_ISP_AE_HIST_AE_FACE2_LOCATION  AE_FACE2_LOCATION;
	union REG_ISP_AE_HIST_AE_FACE3_LOCATION  AE_FACE3_LOCATION;
	union REG_ISP_AE_HIST_AE_FACE0_SIZE     AE_FACE0_SIZE;
	union REG_ISP_AE_HIST_AE_FACE1_SIZE     AE_FACE1_SIZE;
	union REG_ISP_AE_HIST_AE_FACE2_SIZE     AE_FACE2_SIZE;
	union REG_ISP_AE_HIST_AE_FACE3_SIZE     AE_FACE3_SIZE;
	union REG_ISP_AE_HIST_IR_AE_FACE0_LOCATION  IR_AE_FACE0_LOCATION;
	union REG_ISP_AE_HIST_IR_AE_FACE1_LOCATION  IR_AE_FACE1_LOCATION;
	union REG_ISP_AE_HIST_IR_AE_FACE2_LOCATION  IR_AE_FACE2_LOCATION;
	union REG_ISP_AE_HIST_IR_AE_FACE3_LOCATION  IR_AE_FACE3_LOCATION;
	union REG_ISP_AE_HIST_IR_AE_FACE0_SIZE  IR_AE_FACE0_SIZE;
	union REG_ISP_AE_HIST_IR_AE_FACE1_SIZE  IR_AE_FACE1_SIZE;
	union REG_ISP_AE_HIST_IR_AE_FACE2_SIZE  IR_AE_FACE2_SIZE;
	union REG_ISP_AE_HIST_IR_AE_FACE3_SIZE  IR_AE_FACE3_SIZE;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_AWB_T {
	union REG_ISP_AWB_STATUS                STATUS;
	union REG_ISP_AWB_GRACE_RESET           GRACE_RESET;
	union REG_ISP_AWB_MONITOR               MONITOR;
	union REG_ISP_AWB_SHADOW_SELECT         SHADOW_SELECT;
	union REG_ISP_AWB_KICKOFF               KICKOFF;
	union REG_ISP_AWB_ENABLE                ENABLE;
	uint32_t                                _resv_0x18[1];
	union REG_ISP_AWB_MONITOR_SELECT        MONITOR_SELECT;
	union REG_ISP_AWB_STS_OFFSETX           STS_OFFSETX;
	union REG_ISP_AWB_STS_OFFSETY           STS_OFFSETY;
	union REG_ISP_AWB_STS_NUMXM1            STS_NUMXM1;
	union REG_ISP_AWB_STS_NUMYM1            STS_NUMYM1;
	union REG_ISP_AWB_STS_WIDTH             STS_WIDTH;
	union REG_ISP_AWB_STS_HEIGHT            STS_HEIGHT;
	union REG_ISP_AWB_STS_SKIPX             STS_SKIPX;
	union REG_ISP_AWB_STS_SKIPY             STS_SKIPY;
	union REG_ISP_AWB_STS_CORNER_AVG_EN     STS_CORNER_AVG_EN;
	union REG_ISP_AWB_STS_CORNER_SIZE       STS_CORNER_SIZE;
	union REG_ISP_AWB_STS_STS_DIV           STS_STS_DIV;
	uint32_t                                _resv_0x4c[1];
	union REG_ISP_AWB_STS_R_LOTHD           STS_R_LOTHD;
	union REG_ISP_AWB_STS_R_UPTHD           STS_R_UPTHD;
	union REG_ISP_AWB_STS_G_LOTHD           STS_G_LOTHD;
	union REG_ISP_AWB_STS_G_UPTHD           STS_G_UPTHD;
	union REG_ISP_AWB_STS_B_LOTHD           STS_B_LOTHD;
	union REG_ISP_AWB_STS_B_UPTHD           STS_B_UPTHD;
	uint32_t                                _resv_0x68[3];
	union REG_ISP_AWB_WBG_4                 WBG_4;
	union REG_ISP_AWB_WBG_5                 WBG_5;
	uint32_t                                _resv_0x7c[1];
	union REG_ISP_AWB_MEM_SW_MODE           SW_MODE;
	union REG_ISP_AWB_MEM_SW_RADDR          SW_RADDR;
	union REG_ISP_AWB_LOCATION              LOCATION;
	union REG_ISP_AWB_MEM_SW_RDATA          SW_RDATA;
	union REG_ISP_AWB_BAYER_STARTING        BAYER_STARTING;
	union REG_ISP_AWB_DUMMY                 DUMMY;
	uint32_t                                _resv_0x98[2];
	union REG_ISP_AWB_DMI_ENABLE            DMI_ENABLE;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_GMS_T {
	union REG_ISP_GMS_STATUS                GMS_STATUS;
	union REG_ISP_GMS_GRACE_RESET           GMS_GRACE_RESET;
	union REG_ISP_GMS_MONITOR               GMS_MONITOR;
	union REG_ISP_GMS_ENABLE                GMS_ENABLE;
	union REG_ISP_GMS_IMG_BAYERID           IMG_BAYERID;
	union REG_ISP_GMS_FLOW                  GMS_FLOW;
	union REG_ISP_GMS_START_X               GMS_START_X;
	union REG_ISP_GMS_START_Y               GMS_START_Y;
	union REG_ISP_GMS_LOCATION              GMS_LOCATION;
	uint32_t                                _resv_0x24[1];
	union REG_ISP_GMS_X_SECTION_SIZE        GMS_X_SECTION_SIZE;
	union REG_ISP_GMS_Y_SECTION_SIZE        GMS_Y_SECTION_SIZE;
	union REG_ISP_GMS_X_GAP                 GMS_X_GAP;
	union REG_ISP_GMS_Y_GAP                 GMS_Y_GAP;
	union REG_ISP_GMS_DUMMY                 GMS_DUMMY;
	uint32_t                                _resv_0x3c[1];
	union REG_ISP_GMS_MEM_SW_MODE           GMS_SW_MODE;
	union REG_ISP_GMS_MEM_SW_RADDR          GMS_SW_RADDR;
	union REG_ISP_GMS_MEM_SW_RDATA          GMS_SW_RDATA;
	union REG_ISP_GMS_MONITOR_SELECT        GMS_MONITOR_SELECT;
	uint32_t                                _resv_0x50[20];
	union REG_ISP_GMS_DMI_ENABLE            DMI_ENABLE;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_AF_T {
	union REG_ISP_AF_STATUS                 STATUS;
	union REG_ISP_AF_GRACE_RESET            GRACE_RESET;
	union REG_ISP_AF_MONITOR                MONITOR;
	union REG_ISP_AF_BYPASS                 BYPASS;
	union REG_ISP_AF_KICKOFF                KICKOFF;
	union REG_ISP_AF_ENABLES                ENABLES;
	union REG_ISP_AF_OFFSET_X               OFFSET_X;
	union REG_ISP_AF_MXN_IMAGE_WIDTH_M1     MXN_IMAGE_WIDTH_M1;
	union REG_ISP_AF_BLOCK_WIDTH            BLOCK_WIDTH;
	union REG_ISP_AF_BLOCK_HEIGHT           BLOCK_HEIGHT;
	union REG_ISP_AF_BLOCK_NUM_X            BLOCK_NUM_X;
	union REG_ISP_AF_BLOCK_NUM_Y            BLOCK_NUM_Y;
	union REG_ISP_AF_CROP_BAYERID           CROP_BAYERID;
	union REG_ISP_AF_HOR_LOW_PASS_VALUE_SHIFT  HOR_LOW_PASS_VALUE_SHIFT;
	union REG_ISP_AF_CORNING_OFFSET_HORIZONTAL_0  OFFSET_HORIZONTAL_0;
	union REG_ISP_AF_CORNING_OFFSET_HORIZONTAL_1  OFFSET_HORIZONTAL_1;
	union REG_ISP_AF_CORNING_OFFSET_VERTICAL  OFFSET_VERTICAL;
	union REG_ISP_AF_HIGH_Y_THRE            HIGH_Y_THRE;
	union REG_ISP_AF_LOW_PASS_HORIZON       LOW_PASS_HORIZON;
	union REG_ISP_AF_LOCATION               LOCATION;
	union REG_ISP_AF_HIGH_PASS_HORIZON_0    HIGH_PASS_HORIZON_0;
	union REG_ISP_AF_HIGH_PASS_HORIZON_1    HIGH_PASS_HORIZON_1;
	union REG_ISP_AF_HIGH_PASS_VERTICAL_0   HIGH_PASS_VERTICAL_0;
	union REG_ISP_AF_MEM_SW_MODE            SW_MODE;
	union REG_ISP_AF_MONITOR_SELECT         MONITOR_SELECT;
	uint32_t                                _resv_0x64[2];
	union REG_ISP_AF_IMAGE_WIDTH            IMAGE_WIDTH;
	union REG_ISP_AF_DUMMY                  DUMMY;
	union REG_ISP_AF_MEM_SW_RADDR           SW_RADDR;
	union REG_ISP_AF_MEM_SW_RDATA           SW_RDATA;
	union REG_ISP_AF_MXN_BORDER             MXN_BORDER;
	union REG_ISP_AF_TH_LOW                 TH_LOW;
	union REG_ISP_AF_GAIN_LOW               GAIN_LOW;
	union REG_ISP_AF_SLOP_LOW               SLOP_LOW;
	uint32_t                                _resv_0x8c[5];
	union REG_ISP_AF_DMI_ENABLE             DMI_ENABLE;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_PRE_RAW_FE_T {
	union REG_PRE_RAW_FE_PRE_RAW_CTRL       PRE_RAW_CTRL;
	union REG_PRE_RAW_FE_PRE_RAW_FRAME_SIZE  PRE_RAW_FRAME_SIZE;
	union REG_PRE_RAW_FE_LE_LMAP_GRID_NUMBER  LE_LMAP_GRID_NUMBER;
	union REG_PRE_RAW_FE_SE_LMAP_GRID_NUMBER  SE_LMAP_GRID_NUMBER;
	union REG_PRE_RAW_FE_LE_RGBMAP_GRID_NUMBER  LE_RGBMAP_GRID_NUMBER;
	union REG_PRE_RAW_FE_SE_RGBMAP_GRID_NUMBER  SE_RGBMAP_GRID_NUMBER;
	uint32_t                                _resv_0x18[2];
	union REG_PRE_RAW_FE_PRE_RAW_POST_NO_RSPD_CYC  PRE_RAW_POST_NO_RSPD_CYC;
	union REG_PRE_RAW_FE_PRE_RAW_POST_RGBMAP_NO_RSPD_CYC  PRE_RAW_POST_RGBMAP_NO_RSPD_CYC;
	union REG_PRE_RAW_FE_PRE_RAW_FRAME_VLD  PRE_RAW_FRAME_VLD;
	union REG_PRE_RAW_FE_PRE_RAW_DEBUG_STATE  PRE_RAW_DEBUG_STATE;
	union REG_PRE_RAW_FE_PRE_RAW_DUMMY      PRE_RAW_DUMMY;
	union REG_PRE_RAW_FE_DEBUG_INFO         FE_INFO;
	union REG_PRE_RAW_FE_IP_INPUT_SEL       IP_INPUT_SEL;
	uint32_t                                _resv_0x3c[1];
	union REG_PRE_RAW_FE_IP_CONNECTION_DEBUG_0  IP_CONNECTION_DEBUG_0;
	union REG_PRE_RAW_FE_IP_CONNECTION_DEBUG_1  IP_CONNECTION_DEBUG_1;
	union REG_PRE_RAW_FE_IP_CONNECTION_DEBUG_2  IP_CONNECTION_DEBUG_2;
	union REG_PRE_RAW_FE_IP_CONNECTION_DEBUG_3  IP_CONNECTION_DEBUG_3;
	union REG_PRE_RAW_FE_IDLE_INFO          FE_IDLE_INFO;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_CSI_BDG_T {
	union REG_ISP_CSI_BDG_TOP_CTRL          CSI_BDG_TOP_CTRL;
	union REG_ISP_CSI_BDG_INTERRUPT_CTRL    CSI_BDG_INTERRUPT_CTRL;
	union REG_ISP_CSI_BDG_DMA_DPCM_MODE     CSI_BDG_DMA_DPCM_MODE;
	uint32_t                                _resv_0xc[1];
	union REG_ISP_CSI_BDG_CH0_SIZE          CH0_SIZE;
	union REG_ISP_CSI_BDG_CH1_SIZE          CH1_SIZE;
	union REG_ISP_CSI_BDG_CH2_SIZE          CH2_SIZE;
	union REG_ISP_CSI_BDG_CH3_SIZE          CH3_SIZE;
	union REG_ISP_CSI_BDG_CH0_CROP_EN       CH0_CROP_EN;
	union REG_ISP_CSI_BDG_CH0_HORZ_CROP     CH0_HORZ_CROP;
	union REG_ISP_CSI_BDG_CH0_VERT_CROP     CH0_VERT_CROP;
	union REG_ISP_CSI_BDG_CH0_BLC_SUM       CH0_BLC_SUM;
	union REG_ISP_CSI_BDG_CH1_CROP_EN       CH1_CROP_EN;
	union REG_ISP_CSI_BDG_CH1_HORZ_CROP     CH1_HORZ_CROP;
	union REG_ISP_CSI_BDG_CH1_VERT_CROP     CH1_VERT_CROP;
	union REG_ISP_CSI_BDG_CH1_BLC_SUM       CH1_BLC_SUM;
	union REG_ISP_CSI_BDG_CH2_CROP_EN       CH2_CROP_EN;
	union REG_ISP_CSI_BDG_CH2_HORZ_CROP     CH2_HORZ_CROP;
	union REG_ISP_CSI_BDG_CH2_VERT_CROP     CH2_VERT_CROP;
	union REG_ISP_CSI_BDG_CH2_BLC_SUM       CH2_BLC_SUM;
	union REG_ISP_CSI_BDG_CH3_CROP_EN       CH3_CROP_EN;
	union REG_ISP_CSI_BDG_CH3_HORZ_CROP     CH3_HORZ_CROP;
	union REG_ISP_CSI_BDG_CH3_VERT_CROP     CH3_VERT_CROP;
	union REG_ISP_CSI_BDG_CH3_BLC_SUM       CH3_BLC_SUM;
	union REG_ISP_CSI_BDG_PAT_GEN_CTRL      CSI_PAT_GEN_CTRL;
	union REG_ISP_CSI_BDG_PAT_IDX_CTRL      CSI_PAT_IDX_CTRL;
	union REG_ISP_CSI_BDG_PAT_COLOR_0       CSI_PAT_COLOR_0;
	union REG_ISP_CSI_BDG_PAT_COLOR_1       CSI_PAT_COLOR_1;
	union REG_ISP_CSI_BDG_BACKGROUND_COLOR_0  CSI_BACKGROUND_COLOR_0;
	union REG_ISP_CSI_BDG_BACKGROUND_COLOR_1  CSI_BACKGROUND_COLOR_1;
	union REG_ISP_CSI_BDG_FIX_COLOR_0       CSI_FIX_COLOR_0;
	union REG_ISP_CSI_BDG_FIX_COLOR_1       CSI_FIX_COLOR_1;
	union REG_ISP_CSI_BDG_MDE_V_SIZE        CSI_MDE_V_SIZE;
	union REG_ISP_CSI_BDG_MDE_H_SIZE        CSI_MDE_H_SIZE;
	union REG_ISP_CSI_BDG_FDE_V_SIZE        CSI_FDE_V_SIZE;
	union REG_ISP_CSI_BDG_FDE_H_SIZE        CSI_FDE_H_SIZE;
	union REG_ISP_CSI_BDG_HSYNC_CTRL        CSI_HSYNC_CTRL;
	union REG_ISP_CSI_BDG_VSYNC_CTRL        CSI_VSYNC_CTRL;
	union REG_ISP_CSI_BDG_TGEN_TT_SIZE      CSI_TGEN_TT_SIZE;
	union REG_ISP_CSI_BDG_LINE_INTP_HEIGHT_0  LINE_INTP_HEIGHT_0;
	union REG_ISP_CSI_BDG_CH0_DEBUG_0       CH0_DEBUG_0;
	union REG_ISP_CSI_BDG_CH0_DEBUG_1       CH0_DEBUG_1;
	union REG_ISP_CSI_BDG_CH0_DEBUG_2       CH0_DEBUG_2;
	union REG_ISP_CSI_BDG_CH0_DEBUG_3       CH0_DEBUG_3;
	union REG_ISP_CSI_BDG_CH1_DEBUG_0       CH1_DEBUG_0;
	union REG_ISP_CSI_BDG_CH1_DEBUG_1       CH1_DEBUG_1;
	union REG_ISP_CSI_BDG_CH1_DEBUG_2       CH1_DEBUG_2;
	union REG_ISP_CSI_BDG_CH1_DEBUG_3       CH1_DEBUG_3;
	union REG_ISP_CSI_BDG_CH2_DEBUG_0       CH2_DEBUG_0;
	union REG_ISP_CSI_BDG_CH2_DEBUG_1       CH2_DEBUG_1;
	union REG_ISP_CSI_BDG_CH2_DEBUG_2       CH2_DEBUG_2;
	union REG_ISP_CSI_BDG_CH2_DEBUG_3       CH2_DEBUG_3;
	union REG_ISP_CSI_BDG_CH3_DEBUG_0       CH3_DEBUG_0;
	union REG_ISP_CSI_BDG_CH3_DEBUG_1       CH3_DEBUG_1;
	union REG_ISP_CSI_BDG_CH3_DEBUG_2       CH3_DEBUG_2;
	union REG_ISP_CSI_BDG_CH3_DEBUG_3       CH3_DEBUG_3;
	union REG_ISP_CSI_BDG_INTERRUPT_STATUS_0  INTERRUPT_STATUS_0;
	union REG_ISP_CSI_BDG_INTERRUPT_STATUS_1  INTERRUPT_STATUS_1;
	union REG_ISP_CSI_BDG_DEBUG             BDG_DEBUG;
	union REG_ISP_CSI_BDG_OUT_VSYNC_LINE_DELAY  CSI_OUT_VSYNC_LINE_DELAY;
	union REG_ISP_CSI_BDG_WR_URGENT_CTRL    CSI_WR_URGENT_CTRL;
	union REG_ISP_CSI_BDG_RD_URGENT_CTRL    CSI_RD_URGENT_CTRL;
	union REG_ISP_CSI_BDG_DUMMY             CSI_DUMMY;
	union REG_ISP_CSI_BDG_LINE_INTP_HEIGHT_1  LINE_INTP_HEIGHT_1;
	union REG_ISP_CSI_BDG_SLICE_LINE_INTP_HEIGHT_0  SLICE_LINE_INTP_HEIGHT_0;
	union REG_ISP_CSI_BDG_SLICE_LINE_INTP_HEIGHT_1  SLICE_LINE_INTP_HEIGHT_1;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_CROP_T {
	union REG_ISP_CROP_0                    CROP_0;
	union REG_ISP_CROP_1                    CROP_1;
	union REG_ISP_CROP_2                    CROP_2;
	union REG_ISP_CROP_3                    CROP_3;
	union REG_ISP_CROP_DUMMY                DUMMY;
	union REG_ISP_CROP_DEBUG                CROP_DEBUG;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_BLC_T {
	union REG_ISP_BLC_0                     BLC_0;
	union REG_ISP_BLC_1                     BLC_1;
	union REG_ISP_BLC_2                     BLC_2;
	union REG_ISP_BLC_3                     BLC_3;
	union REG_ISP_BLC_4                     BLC_4;
	union REG_ISP_BLC_5                     BLC_5;
	union REG_ISP_BLC_6                     BLC_6;
	union REG_ISP_BLC_7                     BLC_7;
	union REG_ISP_BLC_8                     BLC_8;
	union REG_ISP_BLC_IMG_BAYERID           IMG_BAYERID;
	union REG_ISP_BLC_DUMMY                 BLC_DUMMY;
	uint32_t                                _resv_0x2c[1];
	union REG_ISP_BLC_LOCATION              BLC_LOCATION;
	union REG_ISP_BLC_9                     BLC_9;
	union REG_ISP_BLC_A                     BLC_A;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_LMAP_T {
	union REG_ISP_LMAP_LMP_0                LMP_0;
	union REG_ISP_LMAP_LMP_1                LMP_1;
	union REG_ISP_LMAP_LMP_2                LMP_2;
	union REG_ISP_LMAP_LMP_DEBUG_0          LMP_DEBUG_0;
	union REG_ISP_LMAP_LMP_DEBUG_1          LMP_DEBUG_1;
	union REG_ISP_LMAP_DUMMY                DUMMY;
	union REG_ISP_LMAP_LMP_DEBUG_2          LMP_DEBUG_2;
	uint32_t                                _resv_0x1c[1];
	union REG_ISP_LMAP_LMP_3                LMP_3;
	union REG_ISP_LMAP_LMP_4                LMP_4;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_RGBMAP_T {
	union REG_ISP_RGBMAP_0                  RGBMAP_0;
	union REG_ISP_RGBMAP_1                  RGBMAP_1;
	union REG_ISP_RGBMAP_DEBUG_0            RGBMAP_DEBUG_0;
	union REG_ISP_RGBMAP_DEBUG_1            RGBMAP_DEBUG_1;
	union REG_ISP_RGBMAP_DUMMY              DUMMY;
	union REG_ISP_RGBMAP_2                  RGBMAP_2;
	union REG_ISP_RGBMAP_DEBUG_2            RGBMAP_DEBUG_2;
	union REG_ISP_RGBMAP_3                  RGBMAP_3;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_PCHK_T {
	union REG_ISP_PCHK_SHADOW_RD_SEL        SHADOW_RD_SEL;
	union REG_ISP_PCHK_IN_SEL               IN_SEL;
	union REG_ISP_PCHK_RULE_EN              RULE_EN;
	uint32_t                                _resv_0xc[1];
	union REG_ISP_PCHK_HSIZE                HSIZE;
	union REG_ISP_PCHK_VSIZE                VSIZE;
	union REG_ISP_PCHK_NRDY_LIMIT           NRDY_LIMIT;
	union REG_ISP_PCHK_NREQ_LIMIT           NREQ_LIMIT;
	union REG_ISP_PCHK_PFREQ_LIMIT          PFREQ_LIMIT;
	uint32_t                                _resv_0x24[55];
	union REG_ISP_PCHK_ERR_BUS              ERR_BUS;
	union REG_ISP_PCHK_ERR_XY               ERR_XY;
	uint32_t                                _resv_0x108[62];
	union REG_ISP_PCHK_ERR_CLR              ERR_CLR;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_LSCR_T {
	union REG_ISP_LSCR_ENABLE               LSCR_ENABLE;
	union REG_ISP_LSCR_DELAY                LSCR_DELAY;
	union REG_ISP_LSCR_OUT_SEL              LSCR_OUT_SEL;
	union REG_ISP_LSCR_SHDW_READ_SEL        LSCR_SHDW_READ_SEL;
	union REG_ISP_LSCR_CENTERX              LSCR_CENTERX;
	union REG_ISP_LSCR_CENTERY              LSCR_CENTERY;
	union REG_ISP_LSCR_NORM                 LSCR_NORM;
	union REG_ISP_LSCR_STRNTH               LSCR_STRNTH;
	union REG_ISP_LSCR_NORM_IR              LSCR_NORM_IR;
	union REG_ISP_LSCR_STRNTH_IR            LSCR_STRNTH_IR;
	union REG_ISP_LSCR_DEBUG                LSCR_DEBUG;
	union REG_ISP_LSCR_DUMMY                LSCR_DUMMY;
	union REG_ISP_LSCR_GAIN_LUT             LSCR_GAIN_LUT;
	union REG_ISP_LSCR_GAIN_LUT_IR          LSCR_GAIN_LUT_IR;
	union REG_ISP_LSCR_INDEX_CLR            LSCR_INDEX_CLR;
	union REG_ISP_LSCR_FORCE_CLK_EN         LSCR_FORCE_CLK_EN;
	union REG_ISP_LSCR_DEBUG_2              LSCR_DEBUG_2;
	union REG_ISP_LSCR_DEBUG_3              LSCR_DEBUG_3;
	union REG_ISP_LSCR_DEBUG_4              LSCR_DEBUG_4;
	union REG_ISP_LSCR_GAIN_LUT_G           LSCR_GAIN_LUT_G;
	union REG_ISP_LSCR_GAIN_LUT_B           LSCR_GAIN_LUT_B;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_WBG_T {
	union REG_ISP_WBG_0                     WBG_0;
	union REG_ISP_WBG_1                     WBG_1;
	union REG_ISP_WBG_2                     WBG_2;
	uint32_t                                _resv_0xc[1];
	union REG_ISP_WBG_4                     WBG_4;
	union REG_ISP_WBG_5                     WBG_5;
	union REG_ISP_WBG_6                     WBG_6;
	union REG_ISP_WBG_7                     WBG_7;
	uint32_t                                _resv_0x20[1];
	union REG_ISP_WBG_IMG_BAYERID           IMG_BAYERID;
	union REG_ISP_WBG_DUMMY                 WBG_DUMMY;
	uint32_t                                _resv_0x2c[1];
	union REG_ISP_WBG_LOCATION              WBG_LOCATION;
	union REG_ISP_WBG_34                    WBG_34;
	union REG_ISP_WBG_38                    WBG_38;
	union REG_ISP_WBG_3C                    WBG_3C;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_DPC_T {
	union REG_ISP_DPC_0                     DPC_0;
	union REG_ISP_DPC_1                     DPC_1;
	union REG_ISP_DPC_2                     DPC_2;
	union REG_ISP_DPC_3                     DPC_3;
	union REG_ISP_DPC_4                     DPC_4;
	union REG_ISP_DPC_5                     DPC_5;
	union REG_ISP_DPC_6                     DPC_6;
	union REG_ISP_DPC_7                     DPC_7;
	union REG_ISP_DPC_8                     DPC_8;
	union REG_ISP_DPC_9                     DPC_9;
	union REG_ISP_DPC_10                    DPC_10;
	union REG_ISP_DPC_11                    DPC_11;
	union REG_ISP_DPC_12                    DPC_12;
	union REG_ISP_DPC_13                    DPC_13;
	union REG_ISP_DPC_14                    DPC_14;
	union REG_ISP_DPC_15                    DPC_15;
	union REG_ISP_DPC_16                    DPC_16;
	union REG_ISP_DPC_17                    DPC_17;
	union REG_ISP_DPC_18                    DPC_18;
	union REG_ISP_DPC_19                    DPC_19;
	union REG_ISP_DPC_MEM_W0                DPC_MEM_W0;
	union REG_ISP_DPC_WINDOW                DPC_WINDOW;
	union REG_ISP_DPC_MEM_ST_ADDR           DPC_MEM_ST_ADDR;
	uint32_t                                _resv_0x5c[1];
	union REG_ISP_DPC_CHECKSUM              DPC_CHECKSUM;
	union REG_ISP_DPC_INT                   DPC_INT;
	uint32_t                                _resv_0x68[2];
	union REG_ISP_DPC_20                    DPC_20;
	union REG_ISP_DPC_21                    DPC_21;
	union REG_ISP_DPC_22                    DPC_22;
	union REG_ISP_DPC_23                    DPC_23;
	union REG_ISP_DPC_24                    DPC_24;
	union REG_ISP_DPC_25                    DPC_25;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_RAW_TOP_T {
	union REG_RAW_TOP_RAW_0                 RAW_0;
	union REG_RAW_TOP_READ_SEL              READ_SEL;
	union REG_RAW_TOP_RAW_1                 RAW_1;
	uint32_t                                _resv_0xc[1];
	union REG_RAW_TOP_CTRL                  CTRL;
	union REG_RAW_TOP_UP_PQ_EN              UP_PQ_EN;
	union REG_RAW_TOP_RAW_2                 RAW_2;
	union REG_RAW_TOP_DUMMY                 DUMMY;
	union REG_RAW_TOP_RAW_4                 RAW_4;
	union REG_RAW_TOP_STATUS                STATUS;
	union REG_RAW_TOP_DEBUG                 DEBUG;
	union REG_RAW_TOP_DEBUG_SELECT          DEBUG_SELECT;
	union REG_RAW_TOP_RAW_BAYER_TYPE_TOPLEFT  RAW_BAYER_TYPE_TOPLEFT;
	union REG_RAW_TOP_RDMI_ENBALE           RDMI_ENBALE;
	union REG_RAW_TOP_RDMA_SIZE             RDMA_SIZE;
	union REG_RAW_TOP_DPCM_MODE             DPCM_MODE;
	union REG_RAW_TOP_STVALID_STATUS        STVALID_STATUS;
	union REG_RAW_TOP_STREADY_STATUS        STREADY_STATUS;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_CFA_T {
	union REG_ISP_CFA_0                     REG_0;
	union REG_ISP_CFA_1                     REG_1;
	union REG_ISP_CFA_2                     REG_2;
	union REG_ISP_CFA_3                     REG_3;
	union REG_ISP_CFA_4                     REG_4;
	union REG_ISP_CFA_4_1                   REG_4_1;
	union REG_ISP_CFA_5                     REG_5;
	uint32_t                                _resv_0x1c[1];
	union REG_ISP_CFA_6                     REG_6;
	union REG_ISP_CFA_7                     REG_7;
	union REG_ISP_CFA_8                     REG_8;
	union REG_ISP_CFA_9                     REG_9;
	union REG_ISP_CFA_GHP_LUT_0             GHP_LUT_0;
	union REG_ISP_CFA_GHP_LUT_1             GHP_LUT_1;
	union REG_ISP_CFA_GHP_LUT_2             GHP_LUT_2;
	union REG_ISP_CFA_GHP_LUT_3             GHP_LUT_3;
	union REG_ISP_CFA_GHP_LUT_4             GHP_LUT_4;
	union REG_ISP_CFA_GHP_LUT_5             GHP_LUT_5;
	union REG_ISP_CFA_GHP_LUT_6             GHP_LUT_6;
	union REG_ISP_CFA_GHP_LUT_7             GHP_LUT_7;
	uint32_t                                _resv_0x50[1];
	union REG_ISP_CFA_10                    REG_10;
	union REG_ISP_CFA_11                    REG_11;
	union REG_ISP_CFA_12                    REG_12;
	union REG_ISP_CFA_13                    REG_13;
	union REG_ISP_CFA_14                    REG_14;
	union REG_ISP_CFA_15                    REG_15;
	union REG_ISP_CFA_16                    REG_16;
	union REG_ISP_CFA_17                    REG_17;
	union REG_ISP_CFA_18                    REG_18;
	union REG_ISP_CFA_19                    REG_19;
	union REG_ISP_CFA_20                    REG_20;
	union REG_ISP_CFA_21                    REG_21;
	union REG_ISP_CFA_22                    REG_22;
	union REG_ISP_CFA_23                    REG_23;
	union REG_ISP_CFA_24                    REG_24;
	union REG_ISP_CFA_25                    REG_25;
	union REG_ISP_CFA_26                    REG_26;
	union REG_ISP_CFA_27                    REG_27;
	uint32_t                                _resv_0x9c[25];
	union REG_ISP_CFA_28                    REG_28;
	union REG_ISP_CFA_29                    REG_29;
	union REG_ISP_CFA_30                    REG_30;
	union REG_ISP_CFA_31                    REG_31;
	union REG_ISP_CFA_32                    REG_32;
	union REG_ISP_CFA_33                    REG_33;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_BNR_T {
	union REG_ISP_BNR_SHADOW_RD_SEL         SHADOW_RD_SEL;
	union REG_ISP_BNR_OUT_SEL               OUT_SEL;
	union REG_ISP_BNR_INDEX_CLR             INDEX_CLR;
	uint32_t                                _resv_0xc[61];
	union REG_ISP_BNR_NS_LUMA_TH_R          NS_LUMA_TH_R;
	union REG_ISP_BNR_NS_SLOPE_R            NS_SLOPE_R;
	union REG_ISP_BNR_NS_OFFSET0_R          NS_OFFSET0_R;
	union REG_ISP_BNR_NS_OFFSET1_R          NS_OFFSET1_R;
	union REG_ISP_BNR_NS_LUMA_TH_GR         NS_LUMA_TH_GR;
	union REG_ISP_BNR_NS_SLOPE_GR           NS_SLOPE_GR;
	union REG_ISP_BNR_NS_OFFSET0_GR         NS_OFFSET0_GR;
	union REG_ISP_BNR_NS_OFFSET1_GR         NS_OFFSET1_GR;
	union REG_ISP_BNR_NS_LUMA_TH_GB         NS_LUMA_TH_GB;
	union REG_ISP_BNR_NS_SLOPE_GB           NS_SLOPE_GB;
	union REG_ISP_BNR_NS_OFFSET0_GB         NS_OFFSET0_GB;
	union REG_ISP_BNR_NS_OFFSET1_GB         NS_OFFSET1_GB;
	union REG_ISP_BNR_NS_LUMA_TH_B          NS_LUMA_TH_B;
	union REG_ISP_BNR_NS_SLOPE_B            NS_SLOPE_B;
	union REG_ISP_BNR_NS_OFFSET0_B          NS_OFFSET0_B;
	union REG_ISP_BNR_NS_OFFSET1_B          NS_OFFSET1_B;
	union REG_ISP_BNR_NS_GAIN               NS_GAIN;
	union REG_ISP_BNR_STRENGTH_MODE         STRENGTH_MODE;
	union REG_ISP_BNR_INTENSITY_SEL         INTENSITY_SEL;
	uint32_t                                _resv_0x14c[45];
	union REG_ISP_BNR_WEIGHT_INTRA_0        WEIGHT_INTRA_0;
	union REG_ISP_BNR_WEIGHT_INTRA_1        WEIGHT_INTRA_1;
	union REG_ISP_BNR_WEIGHT_INTRA_2        WEIGHT_INTRA_2;
	uint32_t                                _resv_0x20c[1];
	union REG_ISP_BNR_WEIGHT_NORM_1         WEIGHT_NORM_1;
	union REG_ISP_BNR_WEIGHT_NORM_2         WEIGHT_NORM_2;
	uint32_t                                _resv_0x218[2];
	union REG_ISP_BNR_LSC_RATIO             LSC_RATIO;
	union REG_ISP_BNR_VAR_TH                VAR_TH;
	union REG_ISP_BNR_WEIGHT_LUT            WEIGHT_LUT;
	union REG_ISP_BNR_WEIGHT_SM             WEIGHT_SM;
	union REG_ISP_BNR_WEIGHT_V              WEIGHT_V;
	union REG_ISP_BNR_WEIGHT_H              WEIGHT_H;
	union REG_ISP_BNR_WEIGHT_D45            WEIGHT_D45;
	union REG_ISP_BNR_WEIGHT_D135           WEIGHT_D135;
	union REG_ISP_BNR_NEIGHBOR_MAX          NEIGHBOR_MAX;
	uint32_t                                _resv_0x244[3];
	union REG_ISP_BNR_RES_K_SMOOTH          RES_K_SMOOTH;
	union REG_ISP_BNR_RES_K_TEXTURE         RES_K_TEXTURE;
	uint32_t                                _resv_0x258[106];
	union REG_ISP_BNR_LSC_EN                LSC_EN;
	uint32_t                                _resv_0x404[2];
	union REG_ISP_BNR_HSTR                  HSTR;
	union REG_ISP_BNR_HSIZE                 HSIZE;
	union REG_ISP_BNR_VSIZE                 VSIZE;
	union REG_ISP_BNR_X_CENTER              X_CENTER;
	union REG_ISP_BNR_Y_CENTER              Y_CENTER;
	union REG_ISP_BNR_NORM_FACTOR           NORM_FACTOR;
	union REG_ISP_BNR_LSC_LUT               LSC_LUT;
	union REG_ISP_BNR_LSC_STRENTH           LSC_STRENTH;
	uint32_t                                _resv_0x42c[756];
	union REG_ISP_BNR_DUMMY                 DUMMY;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_RGB_T {
	union REG_ISP_RGB_0                     REG_0;
	union REG_ISP_RGB_1                     REG_1;
	union REG_ISP_RGB_2                     REG_2;
	union REG_ISP_RGB_3                     REG_3;
	union REG_ISP_RGB_4                     REG_4;
	union REG_ISP_RGB_5                     REG_5;
	union REG_ISP_RGB_6                     REG_6;
	union REG_ISP_RGB_7                     REG_7;
	union REG_ISP_RGB_8                     REG_8;
	union REG_ISP_RGB_9                     REG_9;
	uint32_t                                _resv_0x28[2];
	union REG_ISP_RGB_10                    REG_10;
	union REG_ISP_RGB_11                    REG_11;
	union REG_ISP_RGB_12                    REG_12;
	union REG_ISP_RGB_13                    REG_13;
	union REG_ISP_RGB_14                    REG_14;
	uint32_t                                _resv_0x44[3];
	union REG_ISP_RGB_15                    REG_15;
	union REG_ISP_RGB_16                    REG_16;
	union REG_ISP_RGB_17                    REG_17;
	union REG_ISP_RGB_18                    REG_18;
	union REG_ISP_RGB_19                    REG_19;
	uint32_t                                _resv_0x64[7];
	union REG_ISP_RGB_DBG_IP_S_VLD          DBG_IP_S_VLD;
	union REG_ISP_RGB_DBG_IP_S_RDY          DBG_IP_S_RDY;
	union REG_ISP_RGB_DBG_DMI_VLD           DBG_DMI_VLD;
	union REG_ISP_RGB_DBG_DMI_RDY           DBG_DMI_RDY;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_LSC_T {
	union REG_ISP_LSC_STATUS                LSC_STATUS;
	union REG_ISP_LSC_GRACE_RESET           LSC_GRACE_RESET;
	union REG_ISP_LSC_MONITOR               LSC_MONITOR;
	union REG_ISP_LSC_ENABLE                LSC_ENABLE;
	union REG_ISP_LSC_KICKOFF               LSC_KICKOFF;
	union REG_ISP_LSC_STRENGTH              LSC_STRENGTH;
	union REG_ISP_LSC_IMG_BAYERID           IMG_BAYERID;
	union REG_ISP_LSC_MONITOR_SELECT        LSC_MONITOR_SELECT;
	union REG_ISP_LSC_BLK_NUM_SELECT        LSC_BLK_NUM_SELECT;
	uint32_t                                _resv_0x24[1];
	union REG_ISP_LSC_DMI_WIDTHM1           LSC_DMI_WIDTHM1;
	union REG_ISP_LSC_DMI_HEIGHTM1          LSC_DMI_HEIGHTM1;
	uint32_t                                _resv_0x30[3];
	union REG_ISP_LSC_GAIN_BASE             LSC_GAIN_BASE;
	union REG_ISP_LSC_XSTEP                 LSC_XSTEP;
	union REG_ISP_LSC_YSTEP                 LSC_YSTEP;
	union REG_ISP_LSC_IMGX0                 LSC_IMGX0;
	union REG_ISP_LSC_IMGY0                 LSC_IMGY0;
	uint32_t                                _resv_0x50[2];
	union REG_ISP_LSC_INITX0                LSC_INITX0;
	union REG_ISP_LSC_INITY0                LSC_INITY0;
	union REG_ISP_LSC_KERNEL_TABLE_WRITE    LSC_KERNEL_TABLE_WRITE;
	union REG_ISP_LSC_KERNEL_TABLE_DATA     LSC_KERNEL_TABLE_DATA;
	union REG_ISP_LSC_KERNEL_TABLE_CTRL     LSC_KERNEL_TABLE_CTRL;
	union REG_ISP_LSC_DUMMY                 LSC_DUMMY;
	union REG_ISP_LSC_LOCATION              LSC_LOCATION;
	union REG_ISP_LSC_1ST_RUNHIT            LSC_1ST_RUNHIT;
	union REG_ISP_LSC_COMPARE_VALUE         LSC_COMPARE_VALUE;
	uint32_t                                _resv_0x7c[1];
	union REG_ISP_LSC_MEM_SW_MODE           LSC_SW_MODE;
	union REG_ISP_LSC_MEM_SW_RADDR          LSC_SW_RADDR;
	uint32_t                                _resv_0x88[1];
	union REG_ISP_LSC_MEM_SW_RDATA          LSC_SW_RDATA;
	union REG_ISP_LSC_INTERPOLATION         INTERPOLATION;
	uint32_t                                _resv_0x94[3];
	union REG_ISP_LSC_DMI_ENABLE            DMI_ENABLE;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_CCM_T {
	union REG_ISP_CCM_00                    CCM_00;
	union REG_ISP_CCM_01                    CCM_01;
	union REG_ISP_CCM_02                    CCM_02;
	union REG_ISP_CCM_10                    CCM_10;
	union REG_ISP_CCM_11                    CCM_11;
	union REG_ISP_CCM_12                    CCM_12;
	union REG_ISP_CCM_20                    CCM_20;
	union REG_ISP_CCM_21                    CCM_21;
	union REG_ISP_CCM_22                    CCM_22;
	union REG_ISP_CCM_CTRL                  CCM_CTRL;
	union REG_ISP_CCM_DBG                   CCM_DBG;
	uint32_t                                _resv_0x2c[1];
	union REG_ISP_CCM_DMY0                  DMY0;
	union REG_ISP_CCM_DMY1                  DMY1;
	union REG_ISP_CCM_DMY_R                 DMY_R;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_MMAP_T {
	union REG_ISP_MMAP_00                   REG_00;
	union REG_ISP_MMAP_04                   REG_04;
	union REG_ISP_MMAP_08                   REG_08;
	union REG_ISP_MMAP_0C                   REG_0C;
	union REG_ISP_MMAP_10                   REG_10;
	union REG_ISP_MMAP_14                   REG_14;
	union REG_ISP_MMAP_18                   REG_18;
	union REG_ISP_MMAP_1C                   REG_1C;
	union REG_ISP_MMAP_20                   REG_20;
	union REG_ISP_MMAP_24                   REG_24;
	union REG_ISP_MMAP_28                   REG_28;
	union REG_ISP_MMAP_2C                   REG_2C;
	union REG_ISP_MMAP_30                   REG_30;
	union REG_ISP_MMAP_34                   REG_34;
	union REG_ISP_MMAP_38                   REG_38;
	union REG_ISP_MMAP_3C                   REG_3C;
	union REG_ISP_MMAP_40                   REG_40;
	union REG_ISP_MMAP_44                   REG_44;
	uint32_t                                _resv_0x48[1];
	union REG_ISP_MMAP_4C                   REG_4C;
	union REG_ISP_MMAP_50                   REG_50;
	union REG_ISP_MMAP_54                   REG_54;
	union REG_ISP_MMAP_58                   REG_58;
	union REG_ISP_MMAP_5C                   REG_5C;
	union REG_ISP_MMAP_60                   REG_60;
	union REG_ISP_MMAP_64                   REG_64;
	union REG_ISP_MMAP_68                   REG_68;
	union REG_ISP_MMAP_6C                   REG_6C;
	union REG_ISP_MMAP_70                   REG_70;
	union REG_ISP_MMAP_74                   REG_74;
	union REG_ISP_MMAP_78                   REG_78;
	union REG_ISP_MMAP_7C                   REG_7C;
	union REG_ISP_MMAP_80                   REG_80;
	union REG_ISP_MMAP_84                   REG_84;
	union REG_ISP_MMAP_88                   REG_88;
	union REG_ISP_MMAP_8C                   REG_8C;
	union REG_ISP_MMAP_90                   REG_90;
	uint32_t                                _resv_0x94[3];
	union REG_ISP_MMAP_A0                   REG_A0;
	union REG_ISP_MMAP_A4                   REG_A4;
	union REG_ISP_MMAP_A8                   REG_A8;
	union REG_ISP_MMAP_AC                   REG_AC;
	union REG_ISP_MMAP_B0                   REG_B0;
	union REG_ISP_MMAP_B4                   REG_B4;
	union REG_ISP_MMAP_B8                   REG_B8;
	union REG_ISP_MMAP_BC                   REG_BC;
	union REG_ISP_MMAP_C0                   REG_C0;
	union REG_ISP_MMAP_C4                   REG_C4;
	union REG_ISP_MMAP_C8                   REG_C8;
	union REG_ISP_MMAP_CC                   REG_CC;
	union REG_ISP_MMAP_D0                   REG_D0;
	union REG_ISP_MMAP_D4                   REG_D4;
	union REG_ISP_MMAP_D8                   REG_D8;
	union REG_ISP_MMAP_DC                   REG_DC;
	union REG_ISP_MMAP_E0                   REG_E0;
	union REG_ISP_MMAP_E4                   REG_E4;
	union REG_ISP_MMAP_E8                   REG_E8;
	union REG_ISP_MMAP_EC                   REG_EC;
	union REG_ISP_MMAP_F0                   REG_F0;
	union REG_ISP_MMAP_F4                   REG_F4;
	union REG_ISP_MMAP_F8                   REG_F8;
	union REG_ISP_MMAP_FC                   REG_FC;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_GAMMA_T {
	union REG_ISP_GAMMA_CTRL                GAMMA_CTRL;
	union REG_ISP_GAMMA_PROG_CTRL           GAMMA_PROG_CTRL;
	union REG_ISP_GAMMA_PROG_ST_ADDR        GAMMA_PROG_ST_ADDR;
	union REG_ISP_GAMMA_PROG_DATA           GAMMA_PROG_DATA;
	union REG_ISP_GAMMA_PROG_MAX            GAMMA_PROG_MAX;
	union REG_ISP_GAMMA_MEM_SW_RADDR        GAMMA_SW_RADDR;
	union REG_ISP_GAMMA_MEM_SW_RDATA        GAMMA_SW_RDATA;
	union REG_ISP_GAMMA_MEM_SW_RDATA_BG     GAMMA_SW_RDATA_BG;
	union REG_ISP_GAMMA_DBG                 GAMMA_DBG;
	union REG_ISP_GAMMA_DMY0                GAMMA_DMY0;
	union REG_ISP_GAMMA_DMY1                GAMMA_DMY1;
	union REG_ISP_GAMMA_DMY_R               GAMMA_DMY_R;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_CLUT_T {
	union REG_ISP_CLUT_CTRL                 CLUT_CTRL;
	union REG_ISP_CLUT_PROG_ADDR            CLUT_PROG_ADDR;
	union REG_ISP_CLUT_PROG_DATA            CLUT_PROG_DATA;
	union REG_ISP_CLUT_PROG_RDATA           CLUT_PROG_RDATA;
	uint32_t                                _resv_0x10[4];
	union REG_ISP_CLUT_DBG                  CLUT_DBG;
	union REG_ISP_CLUT_DMY0                 CLUT_DMY0;
	union REG_ISP_CLUT_DMY1                 CLUT_DMY1;
	union REG_ISP_CLUT_DMY_R                CLUT_DMY_R;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_DHZ_T {
	union REG_ISP_DHZ_SMOOTH                DHZ_SMOOTH;
	union REG_ISP_DHZ_SKIN                  DHZ_SKIN;
	uint32_t                                _resv_0x8[3];
	union REG_ISP_DHZ_BYPASS                DHZ_BYPASS;
	union REG_ISP_DHZ_0                     REG_0;
	uint32_t                                _resv_0x1c[1];
	union REG_ISP_DHZ_1                     REG_1;
	union REG_ISP_DHZ_2                     REG_2;
	union REG_ISP_DHZ_3                     REG_3;
	uint32_t                                _resv_0x2c[1];
	union REG_ISP_DHZ_4                     REG_4;
	union REG_ISP_DHZ_5                     REG_5;
	union REG_ISP_DHZ_6                     REG_6;
	union REG_ISP_DHZ_7                     REG_7;
	union REG_ISP_DHZ_8                     REG_8;
	uint32_t                                _resv_0x44[3];
	union REG_ISP_DHZ_9                     REG_9;
	union REG_ISP_DHZ_10                    REG_10;
	union REG_ISP_DHZ_11                    REG_11;
	union REG_ISP_DHZ_12                    REG_12;
	union REG_ISP_DHZ_13                    REG_13;
	union REG_ISP_DHZ_14                    REG_14;
	union REG_ISP_DHZ_15                    REG_15;
	union REG_ISP_DHZ_16                    REG_16;
	union REG_ISP_DHZ_17                    REG_17;
	union REG_ISP_DHZ_18                    REG_18;
	union REG_ISP_DHZ_19                    REG_19;
	union REG_ISP_DHZ_20                    REG_20;
	union REG_ISP_DHZ_21                    REG_21;
	union REG_ISP_DHZ_22                    REG_22;
	union REG_ISP_DHZ_23                    REG_23;
	union REG_ISP_DHZ_24                    REG_24;
	union REG_ISP_DHZ_25                    REG_25;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_CSC_T {
	union REG_ISP_CSC_0                     REG_0;
	union REG_ISP_CSC_1                     REG_1;
	union REG_ISP_CSC_2                     REG_2;
	union REG_ISP_CSC_3                     REG_3;
	union REG_ISP_CSC_4                     REG_4;
	union REG_ISP_CSC_5                     REG_5;
	union REG_ISP_CSC_6                     REG_6;
	union REG_ISP_CSC_7                     REG_7;
	union REG_ISP_CSC_8                     REG_8;
	union REG_ISP_CSC_9                     REG_9;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_RGBDITHER_T {
	union REG_ISP_RGBDITHER_RGB_DITHER      RGB_DITHER;
	union REG_ISP_RGBDITHER_RGB_DITHER_DEBUG0  RGB_DITHER_DEBUG0;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_DCI_T {
	union REG_ISP_DCI_STATUS                DCI_STATUS;
	union REG_ISP_DCI_GRACE_RESET           DCI_GRACE_RESET;
	union REG_ISP_DCI_MONITOR               DCI_MONITOR;
	union REG_ISP_DCI_ENABLE                DCI_ENABLE;
	union REG_ISP_DCI_MAP_ENABLE            DCI_MAP_ENABLE;
	union REG_ISP_DCI_FLOW                  DCI_FLOW;
	union REG_ISP_DCI_DEMO_MODE             DCI_DEMO_MODE;
	union REG_ISP_DCI_MONITOR_SELECT        DCI_MONITOR_SELECT;
	union REG_ISP_DCI_LOCATION              DCI_LOCATION;
	uint32_t                                _resv_0x24[1];
	union REG_ISP_DCI_PROG_DATA             DCI_PROG_DATA;
	union REG_ISP_DCI_PROG_CTRL             DCI_PROG_CTRL;
	union REG_ISP_DCI_PROG_MAX              DCI_PROG_MAX;
	union REG_ISP_DCI_CTRL                  DCI_CTRL;
	union REG_ISP_DCI_MEM_SW_MODE           DCI_SW_MODE;
	union REG_ISP_DCI_MEM_RADDR             DCI_MEM_RADDR;
	union REG_ISP_DCI_MEM_RDATA             DCI_MEM_RDATA;
	union REG_ISP_DCI_DEBUG                 DCI_DEBUG;
	union REG_ISP_DCI_DUMMY                 DCI_DUMMY;
	union REG_ISP_DCI_IMG_WIDTHM1           IMG_WIDTHM1;
	union REG_ISP_DCI_LUT_ORDER_SELECT      DCI_LUT_ORDER_SELECT;
	union REG_ISP_DCI_ROI_START             DCI_ROI_START;
	union REG_ISP_DCI_ROI_GEO               DCI_ROI_GEO;
	uint32_t                                _resv_0x5c[9];
	union REG_ISP_DCI_MAP_DBG               DCI_MAP_DBG;
	uint32_t                                _resv_0x84[1];
	union REG_ISP_DCI_BAYER_STARTING        DCI_BAYER_STARTING;
	uint32_t                                _resv_0x8c[5];
	union REG_ISP_DCI_DMI_ENABLE            DMI_ENABLE;
	uint32_t                                _resv_0xa4[23];
	union REG_ISP_DCI_MAPPED_LUT            DCI_MAPPED_LUT;
	uint32_t                                _resv_0x104[64];
	union REG_ISP_DCI_GAMMA_PROG_CTRL       GAMMA_PROG_CTRL;
	uint32_t                                _resv_0x208[1];
	union REG_ISP_DCI_GAMMA_PROG_DATA       GAMMA_PROG_DATA;
	union REG_ISP_DCI_GAMMA_PROG_MAX        GAMMA_PROG_MAX;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_HIST_EDGE_V_T {
	union REG_HIST_EDGE_V_STATUS            STATUS;
	union REG_HIST_EDGE_V_SW_CTL            SW_CTL;
	union REG_HIST_EDGE_V_BYPASS            BYPASS;
	union REG_HIST_EDGE_V_IP_CONFIG         IP_CONFIG;
	union REG_HIST_EDGE_V_OFFSETX           OFFSETX;
	union REG_HIST_EDGE_V_OFFSETY           OFFSETY;
	union REG_HIST_EDGE_V_MONITOR           MONITOR;
	union REG_HIST_EDGE_V_MONITOR_SELECT    MONITOR_SELECT;
	union REG_HIST_EDGE_V_LOCATION          LOCATION;
	union REG_HIST_EDGE_V_DUMMY             DUMMY;
	union REG_HIST_EDGE_V_DMI_ENABLE        DMI_ENABLE;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_FUSION_T {
	union REG_ISP_FUSION_FS_CTRL_0          FS_CTRL_0;
	union REG_ISP_FUSION_FS_FRAME_SIZE      FS_FRAME_SIZE;
	union REG_ISP_FUSION_FS_SE_GAIN         FS_SE_GAIN;
	union REG_ISP_FUSION_FS_LUMA_THD        FS_LUMA_THD;
	union REG_ISP_FUSION_FS_WGT             FS_WGT;
	union REG_ISP_FUSION_FS_WGT_SLOPE       FS_WGT_SLOPE;
	union REG_ISP_FUSION_FS_SHDW_READ_SEL   FS_SHDW_READ_SEL;
	union REG_ISP_FUSION_FS_UP              FS_UP;
	union REG_ISP_FUSION_FS_MOTION_LUT_IN   FS_MOTION_LUT_IN;
	union REG_ISP_FUSION_FS_MOTION_LUT_OUT_0  FS_MOTION_LUT_OUT_0;
	union REG_ISP_FUSION_FS_MOTION_LUT_OUT_1  FS_MOTION_LUT_OUT_1;
	union REG_ISP_FUSION_FS_MOTION_LUT_SLOPE_0  FS_MOTION_LUT_SLOPE_0;
	union REG_ISP_FUSION_FS_MOTION_LUT_SLOPE_1  FS_MOTION_LUT_SLOPE_1;
	union REG_ISP_FUSION_FS_CTRL_1          FS_CTRL_1;
	union REG_ISP_FUSION_FS_CTRL_2          FS_CTRL_2;
	uint32_t                                _resv_0x3c[1];
	union REG_ISP_FUSION_FS_LUMA_THD_SE     FS_LUMA_THD_SE;
	union REG_ISP_FUSION_FS_WGT_SE          FS_WGT_SE;
	union REG_ISP_FUSION_FS_WGT_SLOPE_SE    FS_WGT_SLOPE_SE;
	union REG_ISP_FUSION_FS_DITHER          FS_DITHER;
	union REG_ISP_FUSION_FS_CALIB_CTRL_0    FS_CALIB_CTRL_0;
	union REG_ISP_FUSION_FS_CALIB_CTRL_1    FS_CALIB_CTRL_1;
	union REG_ISP_FUSION_FS_SE_FIX_OFFSET_0  FS_SE_FIX_OFFSET_0;
	union REG_ISP_FUSION_FS_SE_FIX_OFFSET_1  FS_SE_FIX_OFFSET_1;
	union REG_ISP_FUSION_FS_SE_FIX_OFFSET_2  FS_SE_FIX_OFFSET_2;
	union REG_ISP_FUSION_FS_CALIB_OUT_0     FS_CALIB_OUT_0;
	union REG_ISP_FUSION_FS_CALIB_OUT_1     FS_CALIB_OUT_1;
	union REG_ISP_FUSION_FS_CALIB_OUT_2     FS_CALIB_OUT_2;
	union REG_ISP_FUSION_FS_CALIB_OUT_3     FS_CALIB_OUT_3;
	union REG_ISP_FUSION_FS_CALIB_CTRL_2    FS_CALIB_CTRL_2;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_LTM_T {
	union REG_ISP_LTM_TOP_CTRL              LTM_TOP_CTRL;
	union REG_ISP_LTM_BLK_SIZE              LTM_BLK_SIZE;
	union REG_ISP_LTM_FRAME_SIZE            LTM_FRAME_SIZE;
	union REG_ISP_LTM_BE_STRTH_CTRL         LTM_BE_STRTH_CTRL;
	union REG_ISP_LTM_DE_STRTH_CTRL         LTM_DE_STRTH_CTRL;
	union REG_ISP_LTM_FILTER_WIN_SIZE_CTRL  LTM_FILTER_WIN_SIZE_CTRL;
	union REG_ISP_LTM_BGAIN_CTRL_0          LTM_BGAIN_CTRL_0;
	union REG_ISP_LTM_BGAIN_CTRL_1          LTM_BGAIN_CTRL_1;
	union REG_ISP_LTM_DGAIN_CTRL_0          LTM_DGAIN_CTRL_0;
	union REG_ISP_LTM_DGAIN_CTRL_1          LTM_DGAIN_CTRL_1;
	union REG_ISP_LTM_BRI_LCE_CTRL_0        LTM_BRI_LCE_CTRL_0;
	union REG_ISP_LTM_BRI_LCE_CTRL_1        LTM_BRI_LCE_CTRL_1;
	union REG_ISP_LTM_BRI_LCE_CTRL_2        LTM_BRI_LCE_CTRL_2;
	union REG_ISP_LTM_BRI_LCE_CTRL_3        LTM_BRI_LCE_CTRL_3;
	union REG_ISP_LTM_BRI_LCE_CTRL_4        LTM_BRI_LCE_CTRL_4;
	union REG_ISP_LTM_BRI_LCE_CTRL_5        LTM_BRI_LCE_CTRL_5;
	union REG_ISP_LTM_BRI_LCE_CTRL_6        LTM_BRI_LCE_CTRL_6;
	union REG_ISP_LTM_BRI_LCE_CTRL_7        LTM_BRI_LCE_CTRL_7;
	union REG_ISP_LTM_BRI_LCE_CTRL_8        LTM_BRI_LCE_CTRL_8;
	union REG_ISP_LTM_BRI_LCE_CTRL_9        LTM_BRI_LCE_CTRL_9;
	union REG_ISP_LTM_BRI_LCE_CTRL_10       LTM_BRI_LCE_CTRL_10;
	union REG_ISP_LTM_BRI_LCE_CTRL_11       LTM_BRI_LCE_CTRL_11;
	union REG_ISP_LTM_BRI_LCE_CTRL_12       LTM_BRI_LCE_CTRL_12;
	union REG_ISP_LTM_BRI_LCE_CTRL_13       LTM_BRI_LCE_CTRL_13;
	union REG_ISP_LTM_DAR_LCE_CTRL_0        LTM_DAR_LCE_CTRL_0;
	union REG_ISP_LTM_DAR_LCE_CTRL_1        LTM_DAR_LCE_CTRL_1;
	union REG_ISP_LTM_DAR_LCE_CTRL_2        LTM_DAR_LCE_CTRL_2;
	union REG_ISP_LTM_DAR_LCE_CTRL_3        LTM_DAR_LCE_CTRL_3;
	union REG_ISP_LTM_DAR_LCE_CTRL_4        LTM_DAR_LCE_CTRL_4;
	union REG_ISP_LTM_DAR_LCE_CTRL_5        LTM_DAR_LCE_CTRL_5;
	union REG_ISP_LTM_DAR_LCE_CTRL_6        LTM_DAR_LCE_CTRL_6;
	union REG_ISP_LTM_CURVE_QUAN_BIT        LTM_CURVE_QUAN_BIT;
	union REG_ISP_LTM_LMAP0_LP_DIST_WGT_0   LTM_LMAP0_LP_DIST_WGT_0;
	union REG_ISP_LTM_LMAP0_LP_DIST_WGT_1   LTM_LMAP0_LP_DIST_WGT_1;
	union REG_ISP_LTM_LMAP0_LP_DIFF_WGT_0   LTM_LMAP0_LP_DIFF_WGT_0;
	union REG_ISP_LTM_LMAP0_LP_DIFF_WGT_1   LTM_LMAP0_LP_DIFF_WGT_1;
	union REG_ISP_LTM_LMAP0_LP_DIFF_WGT_2   LTM_LMAP0_LP_DIFF_WGT_2;
	union REG_ISP_LTM_LMAP0_LP_DIFF_WGT_3   LTM_LMAP0_LP_DIFF_WGT_3;
	union REG_ISP_LTM_LMAP0_LP_DIFF_WGT_4   LTM_LMAP0_LP_DIFF_WGT_4;
	union REG_ISP_LTM_LMAP1_LP_DIST_WGT_0   LTM_LMAP1_LP_DIST_WGT_0;
	union REG_ISP_LTM_LMAP1_LP_DIST_WGT_1   LTM_LMAP1_LP_DIST_WGT_1;
	union REG_ISP_LTM_LMAP1_LP_DIFF_WGT_0   LTM_LMAP1_LP_DIFF_WGT_0;
	union REG_ISP_LTM_LMAP1_LP_DIFF_WGT_1   LTM_LMAP1_LP_DIFF_WGT_1;
	union REG_ISP_LTM_LMAP1_LP_DIFF_WGT_2   LTM_LMAP1_LP_DIFF_WGT_2;
	union REG_ISP_LTM_LMAP1_LP_DIFF_WGT_3   LTM_LMAP1_LP_DIFF_WGT_3;
	union REG_ISP_LTM_LMAP1_LP_DIFF_WGT_4   LTM_LMAP1_LP_DIFF_WGT_4;
	union REG_ISP_LTM_BE_DIST_WGT_0         LTM_BE_DIST_WGT_0;
	union REG_ISP_LTM_BE_DIST_WGT_1         LTM_BE_DIST_WGT_1;
	union REG_ISP_LTM_DE_DIST_WGT_0         LTM_DE_DIST_WGT_0;
	union REG_ISP_LTM_DE_DIST_WGT_1         LTM_DE_DIST_WGT_1;
	union REG_ISP_LTM_DE_LUMA_WGT_0         LTM_DE_LUMA_WGT_0;
	union REG_ISP_LTM_DE_LUMA_WGT_1         LTM_DE_LUMA_WGT_1;
	union REG_ISP_LTM_DE_LUMA_WGT_2         LTM_DE_LUMA_WGT_2;
	union REG_ISP_LTM_DE_LUMA_WGT_3         LTM_DE_LUMA_WGT_3;
	union REG_ISP_LTM_DE_LUMA_WGT_4         LTM_DE_LUMA_WGT_4;
	uint32_t                                _resv_0xdc[1];
	union REG_ISP_LTM_DTONE_CURVE_PROG_DATA  DTONE_CURVE_PROG_DATA;
	union REG_ISP_LTM_DTONE_CURVE_PROG_CTRL  DTONE_CURVE_PROG_CTRL;
	union REG_ISP_LTM_DTONE_CURVE_PROG_MAX  DTONE_CURVE_PROG_MAX;
	union REG_ISP_LTM_DTONE_CURVE_CTRL      DTONE_CURVE_CTRL;
	union REG_ISP_LTM_DTONE_CURVE_MEM_SW_MODE  DTONE_CURVE_SW_MODE;
	union REG_ISP_LTM_DTONE_CURVE_MEM_SW_RADDR  DTONE_CURVE_SW_RADDR;
	union REG_ISP_LTM_DTONE_CURVE_MEM_SW_RDATA  DTONE_CURVE_SW_RDATA;
	uint32_t                                _resv_0xfc[1];
	union REG_ISP_LTM_BTONE_CURVE_PROG_DATA  BTONE_CURVE_PROG_DATA;
	union REG_ISP_LTM_BTONE_CURVE_PROG_CTRL  BTONE_CURVE_PROG_CTRL;
	union REG_ISP_LTM_BTONE_CURVE_PROG_MAX  BTONE_CURVE_PROG_MAX;
	union REG_ISP_LTM_BTONE_CURVE_CTRL      BTONE_CURVE_CTRL;
	union REG_ISP_LTM_BTONE_CURVE_MEM_SW_MODE  BTONE_CURVE_SW_MODE;
	union REG_ISP_LTM_BTONE_CURVE_MEM_SW_RADDR  BTONE_CURVE_SW_RADDR;
	union REG_ISP_LTM_BTONE_CURVE_MEM_SW_RDATA  BTONE_CURVE_SW_RDATA;
	uint32_t                                _resv_0x11c[1];
	union REG_ISP_LTM_GLOBAL_CURVE_PROG_DATA  GLOBAL_CURVE_PROG_DATA;
	union REG_ISP_LTM_GLOBAL_CURVE_PROG_CTRL  GLOBAL_CURVE_PROG_CTRL;
	union REG_ISP_LTM_GLOBAL_CURVE_PROG_MAX  GLOBAL_CURVE_PROG_MAX;
	union REG_ISP_LTM_GLOBAL_CURVE_CTRL     GLOBAL_CURVE_CTRL;
	union REG_ISP_LTM_GLOBAL_CURVE_MEM_SW_MODE  GLOBAL_CURVE_SW_MODE;
	union REG_ISP_LTM_GLOBAL_CURVE_MEM_SW_RADDR  GLOBAL_CURVE_SW_RADDR;
	union REG_ISP_LTM_GLOBAL_CURVE_MEM_SW_RDATA  GLOBAL_CURVE_SW_RDATA;
	uint32_t                                _resv_0x13c[1];
	union REG_ISP_LTM_RESIZE_COEFF_PROG_CTRL  RESIZE_COEFF_PROG_CTRL;
	union REG_ISP_LTM_RESIZE_COEFF_WDATA_0  RESIZE_COEFF_WDATA_0;
	union REG_ISP_LTM_RESIZE_COEFF_WDATA_1  RESIZE_COEFF_WDATA_1;
	uint32_t                                _resv_0x14c[9];
	union REG_ISP_LTM_TILE_MODE_CTRL_0      TILE_MODE_CTRL_0;
	union REG_ISP_LTM_TILE_MODE_CTRL_1      TILE_MODE_CTRL_1;
	union REG_ISP_LTM_TILE_MODE_CTRL_2      TILE_MODE_CTRL_2;
	union REG_ISP_LTM_DUMMY                 DUMMY;
	union REG_ISP_LTM_LMAP_COMPUTE_CTRL_0   LMAP_COMPUTE_CTRL_0;
	union REG_ISP_LTM_EE_CTRL_0             LTM_EE_CTRL_0;
	union REG_ISP_LTM_EE_CTRL_1             LTM_EE_CTRL_1;
	union REG_ISP_LTM_EE_DETAIL_LUT_00      EE_DETAIL_LUT_00;
	union REG_ISP_LTM_EE_DETAIL_LUT_01      EE_DETAIL_LUT_01;
	union REG_ISP_LTM_EE_DETAIL_LUT_02      EE_DETAIL_LUT_02;
	union REG_ISP_LTM_EE_DETAIL_LUT_03      EE_DETAIL_LUT_03;
	union REG_ISP_LTM_EE_DETAIL_LUT_04      EE_DETAIL_LUT_04;
	union REG_ISP_LTM_EE_DETAIL_LUT_05      EE_DETAIL_LUT_05;
	union REG_ISP_LTM_EE_DETAIL_LUT_06      EE_DETAIL_LUT_06;
	union REG_ISP_LTM_EE_DETAIL_LUT_07      EE_DETAIL_LUT_07;
	union REG_ISP_LTM_EE_DETAIL_LUT_08      EE_DETAIL_LUT_08;
	uint32_t                                _resv_0x1b0[8];
	union REG_ISP_LTM_EE_GAIN_LUT_00        EE_GAIN_LUT_00;
	union REG_ISP_LTM_EE_GAIN_LUT_01        EE_GAIN_LUT_01;
	union REG_ISP_LTM_EE_GAIN_LUT_02        EE_GAIN_LUT_02;
	union REG_ISP_LTM_EE_GAIN_LUT_03        EE_GAIN_LUT_03;
	union REG_ISP_LTM_EE_GAIN_LUT_04        EE_GAIN_LUT_04;
	union REG_ISP_LTM_EE_GAIN_LUT_05        EE_GAIN_LUT_05;
	union REG_ISP_LTM_EE_GAIN_LUT_06        EE_GAIN_LUT_06;
	union REG_ISP_LTM_EE_GAIN_LUT_07        EE_GAIN_LUT_07;
	union REG_ISP_LTM_EE_GAIN_LUT_08        EE_GAIN_LUT_08;
	union REG_ISP_LTM_SAT_ADJ               SAT_ADJ;
	union REG_ISP_LTM_HSV_S_BY_V_0          HSV_S_BY_V_0;
	union REG_ISP_LTM_HSV_S_BY_V_1          HSV_S_BY_V_1;
	union REG_ISP_LTM_HSV_S_BY_V_2          HSV_S_BY_V_2;
	union REG_ISP_LTM_HSV_S_BY_V_3          HSV_S_BY_V_3;
	union REG_ISP_LTM_HSV_S_BY_V_4          HSV_S_BY_V_4;
	union REG_ISP_LTM_HSV_S_BY_V_5          HSV_S_BY_V_5;
	union REG_ISP_LTM_STR_CTRL_0            STR_CTRL_0;
	union REG_ISP_LTM_STR_CTRL_1            STR_CTRL_1;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_YUV_TOP_T {
	union REG_YUV_TOP_YUV_0                 YUV_0;
	union REG_YUV_TOP_YUV_1                 YUV_1;
	union REG_YUV_TOP_YUV_2                 YUV_2;
	union REG_YUV_TOP_YUV_3                 YUV_3;
	union REG_YUV_TOP_YUV_DEBUG_0           YUV_DEBUG_0;
	union REG_YUV_TOP_YUV_4                 YUV_4;
	union REG_YUV_TOP_YUV_DEBUG_STATE       YUV_DEBUG_STATE;
	uint32_t                                _resv_0x1c[1];
	union REG_YUV_TOP_YUV_5                 YUV_5;
	uint32_t                                _resv_0x24[7];
	union REG_YUV_TOP_HSV_LUT_PROG_SRAM0    HSV_LUT_PROG_SRAM0;
	union REG_YUV_TOP_HSV_LUT_PROG_SRAM1    HSV_LUT_PROG_SRAM1;
	union REG_YUV_TOP_HSV_LUT_READ_SRAM0    HSV_LUT_READ_SRAM0;
	union REG_YUV_TOP_HSV_LUT_READ_SRAM1    HSV_LUT_READ_SRAM1;
	union REG_YUV_TOP_HSV_LUT_CONTROL       HSV_LUT_CONTROL;
	uint32_t                                _resv_0x54[2];
	union REG_YUV_TOP_HSV_LUT_STATUS        HSV_LUT_STATUS;
	union REG_YUV_TOP_HSV_ENABLE            HSV_ENABLE;
	union REG_YUV_TOP_IMGW_M1               IMGW_M1;
	union REG_YUV_TOP_IMGW_M1_CROP          IMGW_M1_CROP;
	union REG_YUV_TOP_STVALID_STATUS        STVALID_STATUS;
	union REG_YUV_TOP_STREADY_STATUS        STREADY_STATUS;
	union REG_YUV_TOP_CA_LITE_ENABLE        CA_LITE_ENABLE;
	union REG_YUV_TOP_CA_LITE_LUT_IN        CA_LITE_LUT_IN;
	union REG_YUV_TOP_CA_LITE_LUT_OUT       CA_LITE_LUT_OUT;
	union REG_YUV_TOP_CA_LITE_LUT_SLP       CA_LITE_LUT_SLP;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_444_422_T {
	union REG_ISP_444_422_0                 REG_0;
	union REG_ISP_444_422_1                 REG_1;
	union REG_ISP_444_422_2                 REG_2;
	union REG_ISP_444_422_3                 REG_3;
	union REG_ISP_444_422_4                 REG_4;
	union REG_ISP_444_422_5                 REG_5;
	union REG_ISP_444_422_6                 REG_6;
	uint32_t                                _resv_0x1c[1];
	union REG_ISP_444_422_8                 REG_8;
	union REG_ISP_444_422_9                 REG_9;
	union REG_ISP_444_422_10                REG_10;
	union REG_ISP_444_422_11                REG_11;
	union REG_ISP_444_422_12                REG_12;
	union REG_ISP_444_422_13                REG_13;
	union REG_ISP_444_422_14                REG_14;
	union REG_ISP_444_422_15                REG_15;
	union REG_ISP_444_422_16                REG_16;
	union REG_ISP_444_422_17                REG_17;
	union REG_ISP_444_422_18                REG_18;
	union REG_ISP_444_422_19                REG_19;
	union REG_ISP_444_422_20                REG_20;
	union REG_ISP_444_422_21                REG_21;
	union REG_ISP_444_422_22                REG_22;
	union REG_ISP_444_422_23                REG_23;
	union REG_ISP_444_422_24                REG_24;
	union REG_ISP_444_422_25                REG_25;
	union REG_ISP_444_422_26                REG_26;
	union REG_ISP_444_422_27                REG_27;
	union REG_ISP_444_422_28                REG_28;
	union REG_ISP_444_422_29                REG_29;
	union REG_ISP_444_422_30                REG_30;
	union REG_ISP_444_422_31                REG_31;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_FBCD_T {
	union REG_FBCD_0                        REG_0;
	uint32_t                                _resv_0x4[2];
	union REG_FBCD_3                        REG_3;
	union REG_FBCD_4                        REG_4;
	uint32_t                                _resv_0x14[3];
	union REG_FBCD_FBCE_7                   FBCE_7;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_YUVDITHER_T {
	union REG_ISP_YUVDITHER_Y_DITHER        Y_DITHER;
	union REG_ISP_YUVDITHER_UV_DITHER       UV_DITHER;
	union REG_ISP_YUVDITHER_DEBUG_00        DEBUG_00;
	union REG_ISP_YUVDITHER_DEBUG_01        DEBUG_01;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_YNR_T {
	union REG_ISP_YNR_SHADOW_RD_SEL         SHADOW_RD_SEL;
	union REG_ISP_YNR_OUT_SEL               OUT_SEL;
	union REG_ISP_YNR_INDEX_CLR             INDEX_CLR;
	uint32_t                                _resv_0xc[61];
	union REG_ISP_YNR_NS0_LUMA_TH           NS0_LUMA_TH;
	union REG_ISP_YNR_NS0_SLOPE             NS0_SLOPE;
	union REG_ISP_YNR_NS0_OFFSET            NS0_OFFSET;
	uint32_t                                _resv_0x10c[1];
	union REG_ISP_YNR_NS1_LUMA_TH           NS1_LUMA_TH;
	union REG_ISP_YNR_NS1_SLOPE             NS1_SLOPE;
	union REG_ISP_YNR_NS1_OFFSET            NS1_OFFSET;
	uint32_t                                _resv_0x11c[1];
	union REG_ISP_YNR_MOTION_NS_TH          MOTION_NS_TH;
	union REG_ISP_YNR_MOTION_POS_GAIN       MOTION_POS_GAIN;
	union REG_ISP_YNR_MOTION_NEG_GAIN       MOTION_NEG_GAIN;
	union REG_ISP_YNR_NS_GAIN               NS_GAIN;
	union REG_ISP_YNR_STRENGTH_MODE         STRENGTH_MODE;
	union REG_ISP_YNR_INTENSITY_SEL         INTENSITY_SEL;
	union REG_ISP_YNR_MOTION_LUT            MOTION_LUT;
	uint32_t                                _resv_0x13c[49];
	union REG_ISP_YNR_WEIGHT_INTRA_0        WEIGHT_INTRA_0;
	union REG_ISP_YNR_WEIGHT_INTRA_1        WEIGHT_INTRA_1;
	union REG_ISP_YNR_WEIGHT_INTRA_2        WEIGHT_INTRA_2;
	uint32_t                                _resv_0x20c[1];
	union REG_ISP_YNR_WEIGHT_NORM_1         WEIGHT_NORM_1;
	union REG_ISP_YNR_WEIGHT_NORM_2         WEIGHT_NORM_2;
	uint32_t                                _resv_0x218[2];
	union REG_ISP_YNR_ALPHA_GAIN            ALPHA_GAIN;
	union REG_ISP_YNR_VAR_TH                VAR_TH;
	union REG_ISP_YNR_WEIGHT_LUT            WEIGHT_LUT;
	union REG_ISP_YNR_WEIGHT_SM             WEIGHT_SM;
	union REG_ISP_YNR_WEIGHT_V              WEIGHT_V;
	union REG_ISP_YNR_WEIGHT_H              WEIGHT_H;
	union REG_ISP_YNR_WEIGHT_D45            WEIGHT_D45;
	union REG_ISP_YNR_WEIGHT_D135           WEIGHT_D135;
	union REG_ISP_YNR_NEIGHBOR_MAX          NEIGHBOR_MAX;
	uint32_t                                _resv_0x244[3];
	union REG_ISP_YNR_RES_K_SMOOTH          RES_K_SMOOTH;
	union REG_ISP_YNR_RES_K_TEXTURE         RES_K_TEXTURE;
	union REG_ISP_YNR_FILTER_MODE_ENABLE    FILTER_MODE_ENABLE;
	union REG_ISP_YNR_FILTER_MODE_ALPHA     FILTER_MODE_ALPHA;
	union REG_ISP_YNR_RES_MOT_LUT           RES_MOT_LUT;
	union REG_ISP_YNR_RES_MAX               RES_MAX;
	union REG_ISP_YNR_RES_MOTION_MAX        RES_MOTION_MAX;
	union REG_ISP_YNR_MOTION_NS_CLIP_MAX    MOTION_NS_CLIP_MAX;
	uint32_t                                _resv_0x270[867];
	union REG_ISP_YNR_DUMMY                 DUMMY;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_CNR_T {
	union REG_ISP_CNR_ENABLE                CNR_ENABLE;
	union REG_ISP_CNR_STRENGTH_MODE         CNR_STRENGTH_MODE;
	union REG_ISP_CNR_PURPLE_TH             CNR_PURPLE_TH;
	union REG_ISP_CNR_PURPLE_CR             CNR_PURPLE_CR;
	union REG_ISP_CNR_GREEN_CR              CNR_GREEN_CR;
	union REG_ISP_CNR_WEIGHT_LUT_INTER_CNR_00  WEIGHT_LUT_INTER_CNR_00;
	union REG_ISP_CNR_WEIGHT_LUT_INTER_CNR_04  WEIGHT_LUT_INTER_CNR_04;
	union REG_ISP_CNR_WEIGHT_LUT_INTER_CNR_08  WEIGHT_LUT_INTER_CNR_08;
	union REG_ISP_CNR_WEIGHT_LUT_INTER_CNR_12  WEIGHT_LUT_INTER_CNR_12;
	union REG_ISP_CNR_INTENSITY_SEL_0       CNR_INTENSITY_SEL_0;
	union REG_ISP_CNR_INTENSITY_SEL_4       CNR_INTENSITY_SEL_4;
	union REG_ISP_CNR_MOTION_LUT_0          CNR_MOTION_LUT_0;
	union REG_ISP_CNR_MOTION_LUT_4          CNR_MOTION_LUT_4;
	union REG_ISP_CNR_MOTION_LUT_8          CNR_MOTION_LUT_8;
	union REG_ISP_CNR_MOTION_LUT_12         CNR_MOTION_LUT_12;
	union REG_ISP_CNR_PURPLE_CB2            CNR_PURPLE_CB2;
	union REG_ISP_CNR_PURPLE_W1             CNR_PURPLE_W1;
	union REG_ISP_CNR_DUMMY                 CNR_DUMMY;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_EE_T {
	union REG_ISP_EE_00                     REG_00;
	union REG_ISP_EE_04                     REG_04;
	union REG_ISP_EE_08                     REG_08;
	union REG_ISP_EE_0C                     REG_0C;
	union REG_ISP_EE_10                     REG_10;
	union REG_ISP_EE_14                     REG_14;
	union REG_ISP_EE_18                     REG_18;
	union REG_ISP_EE_1C                     REG_1C;
	union REG_ISP_EE_20                     REG_20;
	union REG_ISP_EE_24                     REG_24;
	union REG_ISP_EE_28                     REG_28;
	union REG_ISP_EE_2C                     REG_2C;
	union REG_ISP_EE_30                     REG_30;
	union REG_ISP_EE_34                     REG_34;
	union REG_ISP_EE_38                     REG_38;
	union REG_ISP_EE_3C                     REG_3C;
	union REG_ISP_EE_40                     REG_40;
	uint32_t                                _resv_0x44[5];
	union REG_ISP_EE_58                     REG_58;
	union REG_ISP_EE_5C                     REG_5C;
	union REG_ISP_EE_60                     REG_60;
	union REG_ISP_EE_64                     REG_64;
	union REG_ISP_EE_68                     REG_68;
	union REG_ISP_EE_6C                     REG_6C;
	union REG_ISP_EE_70                     REG_70;
	union REG_ISP_EE_74                     REG_74;
	union REG_ISP_EE_78                     REG_78;
	union REG_ISP_EE_7C                     REG_7C;
	union REG_ISP_EE_80                     REG_80;
	union REG_ISP_EE_84                     REG_84;
	union REG_ISP_EE_88                     REG_88;
	union REG_ISP_EE_8C                     REG_8C;
	union REG_ISP_EE_90                     REG_90;
	union REG_ISP_EE_94                     REG_94;
	union REG_ISP_EE_98                     REG_98;
	union REG_ISP_EE_9C                     REG_9C;
	uint32_t                                _resv_0xa0[1];
	union REG_ISP_EE_A4                     REG_A4;
	union REG_ISP_EE_A8                     REG_A8;
	union REG_ISP_EE_AC                     REG_AC;
	union REG_ISP_EE_B0                     REG_B0;
	union REG_ISP_EE_B4                     REG_B4;
	union REG_ISP_EE_B8                     REG_B8;
	union REG_ISP_EE_BC                     REG_BC;
	union REG_ISP_EE_C0                     REG_C0;
	union REG_ISP_EE_C4                     REG_C4;
	union REG_ISP_EE_C8                     REG_C8;
	union REG_ISP_EE_CC                     REG_CC;
	union REG_ISP_EE_D0                     REG_D0;
	union REG_ISP_EE_D4                     REG_D4;
	union REG_ISP_EE_D8                     REG_D8;
	union REG_ISP_EE_DC                     REG_DC;
	union REG_ISP_EE_E0                     REG_E0;
	union REG_ISP_EE_E4                     REG_E4;
	union REG_ISP_EE_E8                     REG_E8;
	union REG_ISP_EE_EC                     REG_EC;
	union REG_ISP_EE_F0                     REG_F0;
	union REG_ISP_EE_F4                     REG_F4;
	union REG_ISP_EE_F8                     REG_F8;
	union REG_ISP_EE_FC                     REG_FC;
	union REG_ISP_EE_100                    REG_100;
	union REG_ISP_EE_104                    REG_104;
	union REG_ISP_EE_108                    REG_108;
	union REG_ISP_EE_10C                    REG_10C;
	union REG_ISP_EE_110                    REG_110;
	union REG_ISP_EE_114                    REG_114;
	union REG_ISP_EE_118                    REG_118;
	union REG_ISP_EE_11C                    REG_11C;
	union REG_ISP_EE_120                    REG_120;
	union REG_ISP_EE_124                    REG_124;
	union REG_ISP_EE_128                    REG_128;
	union REG_ISP_EE_12C                    REG_12C;
	union REG_ISP_EE_130                    REG_130;
	union REG_ISP_EE_134                    REG_134;
	union REG_ISP_EE_138                    REG_138;
	union REG_ISP_EE_13C                    REG_13C;
	union REG_ISP_EE_140                    REG_140;
	union REG_ISP_EE_144                    REG_144;
	union REG_ISP_EE_148                    REG_148;
	union REG_ISP_EE_14C                    REG_14C;
	union REG_ISP_EE_150                    REG_150;
	union REG_ISP_EE_154                    REG_154;
	union REG_ISP_EE_158                    REG_158;
	union REG_ISP_EE_15C                    REG_15C;
	union REG_ISP_EE_160                    REG_160;
	union REG_ISP_EE_164                    REG_164;
	union REG_ISP_EE_168                    REG_168;
	union REG_ISP_EE_16C                    REG_16C;
	union REG_ISP_EE_170                    REG_170;
	union REG_ISP_EE_174                    REG_174;
	union REG_ISP_EE_178                    REG_178;
	union REG_ISP_EE_17C                    REG_17C;
	union REG_ISP_EE_180                    REG_180;
	union REG_ISP_EE_184                    REG_184;
	union REG_ISP_EE_188                    REG_188;
	union REG_ISP_EE_18C                    REG_18C;
	union REG_ISP_EE_190                    REG_190;
	union REG_ISP_EE_194                    REG_194;
	union REG_ISP_EE_198                    REG_198;
	union REG_ISP_EE_19C                    REG_19C;
	union REG_ISP_EE_1A0                    REG_1A0;
	union REG_ISP_EE_1A4                    REG_1A4;
	union REG_ISP_EE_1A8                    REG_1A8;
	union REG_ISP_EE_1AC                    REG_1AC;
	union REG_ISP_EE_1B0                    REG_1B0;
	union REG_ISP_EE_1B4                    REG_1B4;
	union REG_ISP_EE_1B8                    REG_1B8;
	union REG_ISP_EE_1BC                    REG_1BC;
	union REG_ISP_EE_1C0                    REG_1C0;
	union REG_ISP_EE_1C4                    REG_1C4;
	union REG_ISP_EE_1C8                    REG_1C8;
	union REG_ISP_EE_1CC                    REG_1CC;
	union REG_ISP_EE_1D0                    REG_1D0;
	union REG_ISP_EE_1D4                    REG_1D4;
	union REG_ISP_EE_1D8                    REG_1D8;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_YCURVE_T {
	union REG_ISP_YCURVE_YCUR_CTRL          YCUR_CTRL;
	union REG_ISP_YCURVE_YCUR_PROG_CTRL     YCUR_PROG_CTRL;
	union REG_ISP_YCURVE_YCUR_PROG_ST_ADDR  YCUR_PROG_ST_ADDR;
	union REG_ISP_YCURVE_YCUR_PROG_DATA     YCUR_PROG_DATA;
	union REG_ISP_YCURVE_YCUR_PROG_MAX      YCUR_PROG_MAX;
	union REG_ISP_YCURVE_YCUR_MEM_SW_MODE   YCUR_SW_MODE;
	union REG_ISP_YCURVE_YCUR_MEM_SW_RDATA  YCUR_SW_RDATA;
	uint32_t                                _resv_0x1c[1];
	union REG_ISP_YCURVE_YCUR_DBG           YCUR_DBG;
	union REG_ISP_YCURVE_YCUR_DMY0          YCUR_DMY0;
	union REG_ISP_YCURVE_YCUR_DMY1          YCUR_DMY1;
	union REG_ISP_YCURVE_YCUR_DMY_R         YCUR_DMY_R;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_TOP_T {
	union REG_ISP_TOP_0                     REG_0;
	union REG_ISP_TOP_1                     REG_1;
	union REG_ISP_TOP_2                     REG_2;
	union REG_ISP_TOP_3                     REG_3;
	union REG_ISP_TOP_4                     REG_4;
	union REG_ISP_TOP_5                     REG_5;
	union REG_ISP_TOP_6                     REG_6;
	union REG_ISP_TOP_7                     REG_7;
	union REG_ISP_TOP_8                     REG_8;
	union REG_ISP_TOP_9                     REG_9;
	union REG_ISP_TOP_A                     REG_A;
	union REG_ISP_TOP_B                     REG_B;
	union REG_ISP_TOP_C                     REG_C;
	union REG_ISP_TOP_D                     REG_D;
	uint32_t                                _resv_0x38[2];
	union REG_ISP_TOP_10                    REG_10;
	union REG_ISP_TOP_11                    REG_11;
	union REG_ISP_TOP_12                    REG_12;
	union REG_ISP_TOP_13                    REG_13;
	union REG_ISP_TOP_14                    REG_14;
	union REG_ISP_TOP_15                    REG_15;
	union REG_ISP_TOP_16                    REG_16;
	union REG_ISP_TOP_17                    REG_17;
	union REG_ISP_TOP_18                    REG_18;
	union REG_ISP_TOP_19                    REG_19;
	union REG_ISP_TOP_1A                    REG_1A;
	union REG_ISP_TOP_1B                    REG_1B;
	union REG_ISP_TOP_1C                    REG_1C;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_WDMA_COM_T {
	union REG_ISP_WDMA_COM_SHADOW_RD_SEL    SHADOW_RD_SEL;
	uint32_t                                _resv_0x4[3];
	union REG_ISP_WDMA_COM_NORM_STATUS0     NORM_STATUS0;
	union REG_ISP_WDMA_COM_NORM_STATUS1     NORM_STATUS1;
	uint32_t                                _resv_0x18[2];
	union REG_ISP_WDMA_COM_NORM_PERF        NORM_PERF;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_RDMA_COM_T {
	union REG_ISP_RDMA_COM_SHADOW_RD_SEL    SHADOW_RD_SEL;
	uint32_t                                _resv_0x4[3];
	union REG_ISP_RDMA_COM_NORM_STATUS0     NORM_STATUS0;
	union REG_ISP_RDMA_COM_NORM_STATUS1     NORM_STATUS1;
	uint32_t                                _resv_0x18[2];
	union REG_ISP_RDMA_COM_NORM_PERF        NORM_PERF;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_CSI_BDG_LITE_T {
	union REG_ISP_CSI_BDG_LITE_BDG_TOP_CTRL  CSI_BDG_TOP_CTRL;
	union REG_ISP_CSI_BDG_LITE_BDG_INTERRUPT_CTRL_0  CSI_BDG_INTERRUPT_CTRL_0;
	union REG_ISP_CSI_BDG_LITE_BDG_INTERRUPT_CTRL_1  CSI_BDG_INTERRUPT_CTRL_1;
	union REG_ISP_CSI_BDG_LITE_FRAME_VLD    FRAME_VLD;
	union REG_ISP_CSI_BDG_LITE_CH0_SIZE     CH0_SIZE;
	union REG_ISP_CSI_BDG_LITE_CH1_SIZE     CH1_SIZE;
	union REG_ISP_CSI_BDG_LITE_CH2_SIZE     CH2_SIZE;
	union REG_ISP_CSI_BDG_LITE_CH3_SIZE     CH3_SIZE;
	union REG_ISP_CSI_BDG_LITE_CH0_CROP_EN  CH0_CROP_EN;
	union REG_ISP_CSI_BDG_LITE_CH0_HORZ_CROP  CH0_HORZ_CROP;
	union REG_ISP_CSI_BDG_LITE_CH0_VERT_CROP  CH0_VERT_CROP;
	uint32_t                                _resv_0x2c[1];
	union REG_ISP_CSI_BDG_LITE_CH1_CROP_EN  CH1_CROP_EN;
	union REG_ISP_CSI_BDG_LITE_CH1_HORZ_CROP  CH1_HORZ_CROP;
	union REG_ISP_CSI_BDG_LITE_CH1_VERT_CROP  CH1_VERT_CROP;
	uint32_t                                _resv_0x3c[1];
	union REG_ISP_CSI_BDG_LITE_CH2_CROP_EN  CH2_CROP_EN;
	union REG_ISP_CSI_BDG_LITE_CH2_HORZ_CROP  CH2_HORZ_CROP;
	union REG_ISP_CSI_BDG_LITE_CH2_VERT_CROP  CH2_VERT_CROP;
	uint32_t                                _resv_0x4c[1];
	union REG_ISP_CSI_BDG_LITE_CH3_CROP_EN  CH3_CROP_EN;
	union REG_ISP_CSI_BDG_LITE_CH3_HORZ_CROP  CH3_HORZ_CROP;
	union REG_ISP_CSI_BDG_LITE_CH3_VERT_CROP  CH3_VERT_CROP;
	uint32_t                                _resv_0x5c[16];
	union REG_ISP_CSI_BDG_LITE_LINE_INTP_HEIGHT  LINE_INTP_HEIGHT;
	union REG_ISP_CSI_BDG_LITE_CH0_DEBUG_0  CH0_DEBUG_0;
	union REG_ISP_CSI_BDG_LITE_CH0_DEBUG_1  CH0_DEBUG_1;
	union REG_ISP_CSI_BDG_LITE_CH0_DEBUG_2  CH0_DEBUG_2;
	union REG_ISP_CSI_BDG_LITE_CH0_DEBUG_3  CH0_DEBUG_3;
	union REG_ISP_CSI_BDG_LITE_CH1_DEBUG_0  CH1_DEBUG_0;
	union REG_ISP_CSI_BDG_LITE_CH1_DEBUG_1  CH1_DEBUG_1;
	union REG_ISP_CSI_BDG_LITE_CH1_DEBUG_2  CH1_DEBUG_2;
	union REG_ISP_CSI_BDG_LITE_CH1_DEBUG_3  CH1_DEBUG_3;
	union REG_ISP_CSI_BDG_LITE_CH2_DEBUG_0  CH2_DEBUG_0;
	union REG_ISP_CSI_BDG_LITE_CH2_DEBUG_1  CH2_DEBUG_1;
	union REG_ISP_CSI_BDG_LITE_CH2_DEBUG_2  CH2_DEBUG_2;
	union REG_ISP_CSI_BDG_LITE_CH2_DEBUG_3  CH2_DEBUG_3;
	union REG_ISP_CSI_BDG_LITE_CH3_DEBUG_0  CH3_DEBUG_0;
	union REG_ISP_CSI_BDG_LITE_CH3_DEBUG_1  CH3_DEBUG_1;
	union REG_ISP_CSI_BDG_LITE_CH3_DEBUG_2  CH3_DEBUG_2;
	union REG_ISP_CSI_BDG_LITE_CH3_DEBUG_3  CH3_DEBUG_3;
	union REG_ISP_CSI_BDG_LITE_INTERRUPT_STATUS_0  INTERRUPT_STATUS_0;
	union REG_ISP_CSI_BDG_LITE_INTERRUPT_STATUS_1  INTERRUPT_STATUS_1;
	union REG_ISP_CSI_BDG_LITE_BDG_DEBUG    BDG_DEBUG;
	uint32_t                                _resv_0xec[1];
	union REG_ISP_CSI_BDG_LITE_WR_URGENT_CTRL  CSI_WR_URGENT_CTRL;
	union REG_ISP_CSI_BDG_LITE_RD_URGENT_CTRL  CSI_RD_URGENT_CTRL;
	union REG_ISP_CSI_BDG_LITE_DUMMY        CSI_DUMMY;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_ISP_DMA_T {
	union REG_ISP_DMA_ARR_SYSTEM            ARR_SYSTEM;
	union REG_ISP_DMA_ARR_BASE              ARR_BASE;
	union REG_ISP_DMA_ARR_SEGLEN            ARR_SEGLEN;
	union REG_ISP_DMA_ARR_STRIDE            ARR_STRIDE;
	union REG_ISP_DMA_ARR_SEGNUM            ARR_SEGNUM;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_IR_PROC_T {
	union REG_IR_PROC_CTRL                  CTRL;
	union REG_IR_PROC_DUMMY                 DUMMY;
};

/******************************************/
/*           Module Definition            */
/******************************************/
struct REG_FBCE_T {
	union REG_FBCE_0                        REG_0;
	union REG_FBCE_1                        REG_1;
	union REG_FBCE_2                        REG_2;
	union REG_FBCE_3                        REG_3;
	union REG_FBCE_4                        REG_4;
	union REG_FBCE_5                        REG_5;
	union REG_FBCE_6                        REG_6;
};

#endif // _REG_BLOCKS_H_
