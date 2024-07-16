#ifndef _U_CVI_VIP_ISP_H_
#define _U_CVI_VIP_ISP_H_

#include <linux/v4l2-subdev.h>
#include "cvi_vip_snsr.h"

enum ISP_SCENE_INFO {
	PRE_OFF_POST_OFF_SC,      //183x
	PRE_OFF_POST_ON_SC,       //183x
	FE_ON_BE_ON_POST_OFF_SC,  //182x
	FE_ON_BE_ON_POST_ON_SC,   //182x
	FE_ON_BE_OFF_POST_OFF_SC, //182x
	FE_ON_BE_OFF_POST_ON_SC,  //182x
	FE_OFF_BE_ON_POST_OFF_SC, //182x
	FE_OFF_BE_ON_POST_ON_SC,  //182x
};

enum IP_INFO_GRP {
	IP_INFO_ID_MIN = 0,
	//Preraw_0
	IP_INFO_ID_PRERAW0 = IP_INFO_ID_MIN,
	IP_INFO_ID_CSIBDG0,
	IP_INFO_ID_CROP0,
	IP_INFO_ID_CROP1,
	IP_INFO_ID_BLC0,
	IP_INFO_ID_BLC1,
	IP_INFO_ID_LSCR0,
	IP_INFO_ID_LSCR1,
	IP_INFO_ID_AEHIST0,
	IP_INFO_ID_AEHIST1,
	IP_INFO_ID_AWB0,
	IP_INFO_ID_AWB1,
	IP_INFO_ID_AF,
	IP_INFO_ID_AF_GAMMA,
	IP_INFO_ID_GMS,
	IP_INFO_ID_WBG0,
	IP_INFO_ID_WBG1,
	IP_INFO_ID_LMP0,
	IP_INFO_ID_RGBMAP0,
	//Preraw_1
	IP_INFO_ID_PRERAW1_R1,
	IP_INFO_ID_CSIBDG1_R1,
	IP_INFO_ID_CROP0_R1,
	IP_INFO_ID_CROP1_R1,
	IP_INFO_ID_BLC0_R1,
	IP_INFO_ID_BLC1_R1,
	IP_INFO_ID_LSCR2_R1,
	IP_INFO_ID_LSCR3_R1,
	IP_INFO_ID_AEHIST0_R1,
	IP_INFO_ID_AEHIST1_R1,
	IP_INFO_ID_AWB0_R1,
	IP_INFO_ID_AWB1_R1,
	IP_INFO_ID_AF_R1,
	IP_INFO_ID_AF_GAMMA_R1,
	IP_INFO_ID_GMS_R1,
	IP_INFO_ID_WBG0_R1,
	IP_INFO_ID_WBG1_R1,
	IP_INFO_ID_LMP2_R1,
	IP_INFO_ID_RGBMAP2_R1,
	//Rawtop
	IP_INFO_ID_RAWTOP,
	IP_INFO_ID_BLC2,
	IP_INFO_ID_BLC3,
	IP_INFO_ID_DPC0,
	IP_INFO_ID_DPC1,
	IP_INFO_ID_WBG2,
	IP_INFO_ID_WBG3,
	IP_INFO_ID_LSCM0,
	IP_INFO_ID_LSCM1,
	IP_INFO_ID_AWB4,
	IP_INFO_ID_HDRFUSION,
	IP_INFO_ID_HDRLTM,
	IP_INFO_ID_BNR,
	IP_INFO_ID_CROP2,
	IP_INFO_ID_CROP3,
	IP_INFO_ID_MANR,
	IP_INFO_ID_FPN0,
	IP_INFO_ID_FPN1,
	IP_INFO_ID_WBG4,
	//Rgbtop
	IP_INFO_ID_RGBTOP,
	IP_INFO_ID_CFA,
	IP_INFO_ID_CCM,
	IP_INFO_ID_GAMMA,
	IP_INFO_ID_HSV,
	IP_INFO_ID_DHZ,
	IP_INFO_ID_R2Y4,
	IP_INFO_ID_RGBDITHER,
	IP_INFO_ID_RGBEE,
	//Yuvtop
	IP_INFO_ID_YUVTOP,
	IP_INFO_ID_444422,
	IP_INFO_ID_UVDITHER,
	IP_INFO_ID_YNR,
	IP_INFO_ID_CNR,
	IP_INFO_ID_EE,
	IP_INFO_ID_YCURVE,
	IP_INFO_ID_CROP4,
	IP_INFO_ID_CROP5,
	IP_INFO_ID_CROP6,
	IP_INFO_ID_DCI,
	IP_INFO_ID_ISPTOP,
	IP_INFO_ID_MAX,
};

struct ip_info {
	__u32 str_addr; //IP start address
	__u32 size; //IP total registers size
};

/* struct cvi_vip_memblock
 * @base: the address of the memory allocated.
 * @size: Size in bytes of the memblock.
 */
struct cvi_vip_memblock {
	__u8  raw_num;
	__u64 phy_addr;
	void *vir_addr;
#ifdef __arm__
	__u32 padding;
#endif
	__u32 size;
};

struct cvi_vip_isp_raw_blk {
	struct cvi_vip_memblock raw_dump;
	__u32 time_out;//msec
	__u16 src_w;
	__u16 src_h;
	__u16 crop_x;
	__u16 crop_y;
	__u8  is_b_not_rls;
	__u8  is_timeout;
	__u8  is_sig_int;
};

struct cvi_isp_sts_mem {
	__u8			raw_num;
	struct cvi_vip_memblock af;
	struct cvi_vip_memblock gms;
	struct cvi_vip_memblock ae_le0;
	struct cvi_vip_memblock ae_le1;
	struct cvi_vip_memblock hist_le;
	struct cvi_vip_memblock awb_le;
	struct cvi_vip_memblock ae_se;
	struct cvi_vip_memblock hist_se;
	struct cvi_vip_memblock awb_se;
	struct cvi_vip_memblock awb_post;
	struct cvi_vip_memblock dci;
	struct cvi_vip_memblock mmap;
};

struct cvi_isp_usr_pic_cfg {
	struct v4l2_mbus_framefmt fmt;
	struct v4l2_rect crop;
};

enum cvi_isp_source {
	CVI_ISP_SOURCE_DEV = 0,
	CVI_ISP_SOURCE_FE,
	CVI_ISP_SOURCE_BE,
	CVI_ISP_SOURCE_MAX,
};

struct cvi_isp_snr_ut_cfg {
	__u8   dual_sensor_on;
	__u8   hdr_on;
	__u32  w;
	__u32  h;
	__u64  phy_addr[4];
};

struct cvi_isp_snr_info {
	__u8   raw_num;
	__u16  color_mode;
	__u32  pixel_rate;
	struct wdr_size_s snr_fmt;
};

struct cvi_isp_snr_update {
	__u8  raw_num;
	struct snsr_cfg_node_s snr_cfg_node;
};

struct cvi_vip_isp_yuv_param {
	__u8   raw_num;
	__u32  yuv_bypass_path;
};

struct cvi_isp_mmap_grid_size {
	__u8  raw_num;
	__u8  grid_size;
};

struct isp_proc_cfg {
	void *buffer;
#ifdef __arm__
	__u32 padding;
#endif
	size_t buffer_size;
};

struct cvi_vi_dma_buf_info {
	__u64  paddr;
	__u32  size;
};

struct cvi_isp_sc_online {
	__u8   raw_num;
	__u8   is_sc_online;
};

#endif // _U_CVI_VIP_ISP_H_
