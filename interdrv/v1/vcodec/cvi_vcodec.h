/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_vcodec.h
 * Description:
 */

#ifndef __CVI_VCODEC_H__
#define __CVI_VCODEC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/fs.h>
#include <linux/types.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <generated/compile.h>

#include "cvi_vcodec_proc_info.h"

#define USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY

#define VDI_IOCTL_MAGIC 'V'
#define VDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY _IO(VDI_IOCTL_MAGIC, 0)
#define VDI_IOCTL_FREE_PHYSICALMEMORY _IO(VDI_IOCTL_MAGIC, 1)
#define VDI_IOCTL_WAIT_INTERRUPT _IO(VDI_IOCTL_MAGIC, 2)
#define VDI_IOCTL_SET_CLOCK_GATE _IO(VDI_IOCTL_MAGIC, 3)
#define VDI_IOCTL_RESET _IO(VDI_IOCTL_MAGIC, 4)
#define VDI_IOCTL_GET_INSTANCE_POOL _IO(VDI_IOCTL_MAGIC, 5)
#define VDI_IOCTL_GET_COMMON_MEMORY _IO(VDI_IOCTL_MAGIC, 6)
#define VDI_IOCTL_GET_RESERVED_VIDEO_MEMORY_INFO _IO(VDI_IOCTL_MAGIC, 8)
#define VDI_IOCTL_OPEN_INSTANCE _IO(VDI_IOCTL_MAGIC, 9)
#define VDI_IOCTL_CLOSE_INSTANCE _IO(VDI_IOCTL_MAGIC, 10)
#define VDI_IOCTL_GET_INSTANCE_NUM _IO(VDI_IOCTL_MAGIC, 11)
#define VDI_IOCTL_GET_REGISTER_INFO _IO(VDI_IOCTL_MAGIC, 12)
#define VDI_IOCTL_SET_CLOCK_GATE_EXT _IO(VDI_IOCTL_MAGIC, 13)
#define VDI_IOCTL_GET_CHIP_VERSION _IO(VDI_IOCTL_MAGIC, 14)
#define VDI_IOCTL_GET_CLOCK_FREQUENCY _IO(VDI_IOCTL_MAGIC, 15)
#define VDI_IOCTL_RELEASE_COMMON_MEMORY _IO(VDI_IOCTL_MAGIC, 16)
#define VDI_IOCTL_GET_SINGLE_CORE_CONFIG _IO(VDI_IOCTL_MAGIC, 17)

#define VCODEC_GET_REG_BASE             0x8000
#define VCODEC_GET_REG_BASE_CTRL        0x8000
#define VCODEC_GET_REG_BASE_ADDR_REMAP  0x8001
#define VCODEC_GET_REG_BASE_SBM         0x8002

#define VENC_PROC_NAME		"cvitek/venc"
#define H265E_PROC_NAME		"cvitek/h265e"
#define H264E_PROC_NAME		"cvitek/h264e"
#define JPEGE_PROC_NAME		"cvitek/jpege"
#define RC_PROC_NAME		"cvitek/rc"
#define VDEC_PROC_NAME		"cvitek/vdec"
#define VIDEO_PROC_PERMS	(0644)
#define VIDEO_PROC_PARENT	(NULL)
#define MAX_VENC_CHN_NUM	(70)	// h26x(6) + jpeg(64)
#define MAX_VDEC_CHN_NUM	(70)	// h26x(6) + jpeg(64)
#define MAX_PROC_STR_SIZE	(255)
#define MAX_DIR_STR_SIZE	(255)

typedef struct vpudrv_buffer_t {
	__u32 size;
	__u64 phys_addr;
	__u64 base; /* kernel logical address in use kernel */
	__u8 *virt_addr; /* virtual user space address */
#ifdef __arm__
	__u32 padding; /* padding for keeping same size of this structure */
#endif
} vpudrv_buffer_t;

typedef struct vpu_bit_firmware_info_t {
	__u32 size; /* size of this structure*/
	__u32 core_idx;
	__u64 reg_base_offset;
	__u16 bit_code[512];
} vpu_bit_firmware_info_t;

typedef struct vpudrv_inst_info_t {
	unsigned int core_idx;
	unsigned int inst_idx;
	int inst_open_count; /* for output only*/
} vpudrv_inst_info_t;

typedef struct vpudrv_intr_info_t {
	unsigned int timeout;
	int intr_reason;
	int coreIdx;
	__u64 intr_tv_sec;
	__u64 intr_tv_nsec;
} vpudrv_intr_info_t;

#define VCODEC_QUIRK_SUPPORT_CLOCK_CONTROL	(1 << 0)
#define VCODEC_QUIRK_SUPPORT_SWITCH_TO_PLL	(1 << 1)
#define VCODEC_QUIRK_SUPPORT_REMAP_DDR		(1 << 2)
#define VCODEC_QUIRK_SUPPORT_VC_CTRL_REG	(1 << 3)
#define VCODEC_QUIRK_SUPPORT_VC_SBM		(1 << 4)  // slice buffer mode
#define VCODEC_QUIRK_SUPPORT_VC_ADDR_REMAP	(1 << 5)  // address remapping
#define VCDOEC_QUIRK_SUPPORT_FPGA		(1 << 6)  // fpga initialization

struct vpu_pltfm_data {
	const struct vpu_ops *ops;
	unsigned int quirks;
	unsigned int version;
};

struct cvi_vpu_device {
	struct device *dev;
	struct miscdevice miscdev;

	dev_t cdev_id;
	struct cdev cdev;

	int s_vpu_major;
	struct class *vpu_class;

	struct clk *clk_axi_video_codec;
	struct clk *clk_h264c;
	struct clk *clk_apb_h264c;
	struct clk *clk_h265c;
	struct clk *clk_apb_h265c;
	struct clk *clk_vc_src0;
	struct clk *clk_vc_src1;
	struct clk *clk_vc_src2;
	struct clk *clk_cfg_reg_vc;

	struct cvi_vcodec_context *pvctx;

	const struct vpu_pltfm_data *pdata;

	void *vpu_shared_mem;
};

struct vpu_ops {
	void	(*clk_get)(struct cvi_vpu_device *vdev);
	void	(*clk_put)(struct cvi_vpu_device *vdev);
	void	(*clk_enable)(struct cvi_vpu_device *vdev, int mask);
	void	(*clk_disable)(struct cvi_vpu_device *vdev, int mask);
	unsigned long (*clk_get_rate)(struct cvi_vpu_device *vdev);
	void	(*config_ddr)(struct cvi_vpu_device *vdev);
};

typedef struct _venc_proc_info_t {
	VENC_CHN_ATTR_S chnAttr;
	VENC_CHN_PARAM_S stChnParam;
	VENC_CHN_STATUS_S chnStatus;
	VENC_RC_PARAM_S rcParam;
	VENC_REF_PARAM_S refParam;
	VENC_JPEG_PARAM_S stJpegParam;
	VENC_STREAM_S stStream;
	VIDEO_FRAME_S stFrame;
	VENC_FRAMELOST_S frameLost;
	VENC_ROI_ATTR_S stRoiAttr[8];
	VCODEC_PERF_FPS_S stFPS;
	VENC_SUPERFRAME_CFG_S stSuperFrmParam;
	uint16_t u16ChnNo;
	uint8_t u8ChnUsed;
} venc_proc_info_t;

typedef struct _vdec_proc_info_t {
	VDEC_CHN_ATTR_S chnAttr;
	VDEC_CHN_PARAM_S stChnParam;
	VIDEO_FRAME_S stFrame;
	VCODEC_PERF_FPS_S stFPS;
	uint16_t u16ChnNo;
	uint8_t u8ChnUsed;
} vdec_proc_info_t;

typedef struct _proc_debug_config_t {
	uint32_t u32DbgMask;
	uint32_t u32StartFrmIdx;
	uint32_t u32EndFrmIdx;
	char cDumpPath[MAX_DIR_STR_SIZE];
	uint32_t u32NoDataTimeout;
} proc_debug_config_t;

#define VCODEC_VENC_SHARE_MEM_SIZE	(0x30000)	// 192k
#define VCODEC_VDEC_SHARE_MEM_SIZE	(0x8000)	// 32k
#define VCODEC_SHARE_MEM_SIZE		(VCODEC_VENC_SHARE_MEM_SIZE + \
	VCODEC_VDEC_SHARE_MEM_SIZE)

#define VCODEC_DBG_MSG_ENABLE

#define VCODEC_MASK_ERR 0x1
#define VCODEC_MASK_WARN 0x2
#define VCODEC_MASK_INFO 0x4
#define VCODEC_MASK_FLOW 0x8
#define VCODEC_MASK_DBG 0x10
#define VCODEC_MASK_INTR 0x20
#define VCODEC_MASK_MCU 0x40
#define VCODEC_MASK_MEM 0x80
#define VCODEC_MASK_BS 0x100
#define VCODEC_MASK_TRACE 0x1000
#define VCODEC_MASK_REG 0x10000
#define VCODEC_MASK_DISABLE_CLK_GATING 0x20000
#define VCODEC_MASK_ENABLE_DOWNSPEED 0x40000

#define VCODEC_MASK_CURR (0x0 | VCODEC_MASK_ERR)

// Below define must sync with middleware
#define H265_CORE_IDX	0
#define H264_CORE_IDX	1

#ifdef VCODEC_DBG_MSG_ENABLE
#define DPRINTK(msg, ...) VCODEC_DBG_TRACE(msg, ##__VA_ARGS__)
#else
#define DPRINTK(msg, ...)
#endif

#ifdef VCODEC_DBG_MSG_ENABLE
extern int vcodec_mask;
#define VCODEC_DBG_ERR(msg, ...)                                                  \
	do {                                                                   \
		if (vcodec_mask & VCODEC_MASK_ERR)                                \
			pr_info("[ERR] %s = %d, " msg, __func__, __LINE__, \
				##__VA_ARGS__);                                \
	} while (0)
#define VCODEC_DBG_WARN(msg, ...)                                                 \
	do {                                                                   \
		if (vcodec_mask & VCODEC_MASK_WARN)                               \
			pr_info("[WARN] %s = %d, " msg, __func__,          \
				__LINE__, ##__VA_ARGS__);                      \
	} while (0)
#define VCODEC_DBG_INFO(msg, ...)                                                 \
	do {                                                                   \
		if (vcodec_mask & VCODEC_MASK_INFO)                               \
			pr_info("[INFO] %s = %d, " msg, __func__,          \
				__LINE__, ##__VA_ARGS__);                      \
	} while (0)
#define VCODEC_DBG_FLOW(msg, ...)                                                 \
	do {                                                                   \
		if (vcodec_mask & VCODEC_MASK_FLOW)                               \
			pr_info("[FLOW] %s = %d, " msg, __func__,          \
				__LINE__, ##__VA_ARGS__);                      \
	} while (0)
#define VCODEC_DBG_DBG(msg, ...)                                                  \
	do {                                                                   \
		if (vcodec_mask & VCODEC_MASK_DBG)                                \
			pr_info("[DBG] %s = %d, " msg, __func__, __LINE__, \
				##__VA_ARGS__);                                \
	} while (0)
#define VCODEC_DBG_INTR(msg, ...)                                                 \
	do {                                                                   \
		if (vcodec_mask & VCODEC_MASK_INTR)                               \
			pr_info("[INTR] %s = %d, " msg, __func__,          \
				__LINE__, ##__VA_ARGS__);                      \
	} while (0)
#define VCODEC_DBG_MCU(msg, ...)                                                  \
	do {                                                                   \
		if (vcodec_mask & VCODEC_MASK_MCU)                                \
			pr_info("[MCU] %s = %d, " msg, __func__, __LINE__, \
				##__VA_ARGS__);                                \
	} while (0)
#define VCODEC_DBG_MEM(msg, ...)                                                  \
	do {                                                                   \
		if (vcodec_mask & VCODEC_MASK_MEM)                                \
			pr_info("[MEM] %s = %d, " msg, __func__, __LINE__, \
				##__VA_ARGS__);                                \
	} while (0)
#define VCODEC_DBG_BS(msg, ...)                                                   \
	do {                                                                   \
		if (vcodec_mask & VCODEC_MASK_BS)                                 \
			pr_info("[BS] %s = %d, " msg, __func__, __LINE__,  \
				##__VA_ARGS__);                                \
	} while (0)
#define VCODEC_DBG_TRACE(msg, ...)                                                \
	do {                                                                   \
		if (vcodec_mask & VCODEC_MASK_TRACE)                              \
			pr_info("[TRACE] %s = %d, " msg, __func__,         \
				__LINE__, ##__VA_ARGS__);                      \
	} while (0)
#define VCODEC_DBG_REG(msg, ...)                                                  \
	do {                                                                   \
		if (vcodec_mask & VCODEC_MASK_REG)                                \
			pr_info("[REG] %s = %d, " msg, __func__, __LINE__, \
				##__VA_ARGS__);                                \
	} while (0)
#else
#define VCODEC_DBG_ERR(msg, ...)
#define VCODEC_DBG_WARN(msg, ...)
#define VCODEC_DBG_INFO(msg, ...)
#define VCODEC_DBG_FLOW(msg, ...)
#define VCODEC_DBG_DBG(msg, ...)
#define VCODEC_DBG_INTR(msg, ...)
#define VCODEC_DBG_MCU(msg, ...)
#define VCODEC_DBG_MEM(msg, ...)
#define VCODEC_DBG_BS(msg, ...)
#define VCODEC_DBG_TRACE(msg, ...)
#define VCODEC_DBG_REG(msg, ...)
#endif

extern bool __clk_is_enabled(struct clk *clk);

#ifdef __cplusplus
}
#endif

#endif
