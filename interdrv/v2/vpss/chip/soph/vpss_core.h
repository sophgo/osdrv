#ifndef _VPSS_CORE_H_
#define _VPSS_CORE_H_

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/pm_qos.h>
#include <linux/miscdevice.h>

#include <linux/iommu.h>

#include <linux/cvi_defines.h>
#include <linux/vpss_uapi.h>

#include "scaler.h"
#include "vpss_common.h"

#include <vpss_cb.h>

#define DEVICE_FROM_DTS 1

#define VIP_MAX_PLANES 3

#define INTR_IMG_IN (1 << 0)
#define INTR_SC     (1 << 1)


#if 0
#define VIP_CLK_RATIO_MASK(CLK_NAME) VIP_SYS_REG_CK_COEF_##CLK_NAME##_MASK
#define VIP_CLK_RATIO_OFFSET(CLK_NAME) VIP_SYS_REG_CK_COEF_##CLK_NAME##_OFFSET
#define VIP_CLK_RATIO_CONFIG(CLK_NAME, RATIO) \
	vip_sys_reg_write_mask(VIP_SYS_REG_CK_COEF_##CLK_NAME, \
		VIP_CLK_RATIO_MASK(CLK_NAME), \
		RATIO << VIP_CLK_RATIO_OFFSET(CLK_NAME))
#endif

enum cvi_vpss_type {
	CVI_VPSS_V0 = 0,
	CVI_VPSS_V1,
	CVI_VPSS_V2,
	CVI_VPSS_V3,
	CVI_VPSS_T0,
	CVI_VPSS_T1,
	CVI_VPSS_T2,
	CVI_VPSS_T3,
	CVI_VPSS_D0,
	CVI_VPSS_D1,
	CVI_VPSS_MAX,
};

enum cvi_vip_state {
	CVI_VIP_IDLE,
	CVI_VIP_RUNNING,
	CVI_VIP_END,
	CVI_VIP_ONLINE,
};

struct vpss_core {
	enum cvi_vpss_type enVpssType;
	unsigned int irq_num;
	atomic_t state;
	spinlock_t vpss_lock;
	struct clk *clk_src;
	struct clk *clk_apb;
	struct clk *clk_vpss;
	struct timespec64 ts_start;
	struct timespec64 ts_end;
	u32 u32HwDuration;
	u32 u32HwDurationTotal;
	u32 u32DutyRatio;
	u8 intr_status;
	u8 tile_mode;
	u8 map_chn;
	u8 isOnline;
	u32 checksum;
	u32 u32StartCnt;
	u32 u32IntCnt;
	void *job;
};

struct cvi_vpss_device {
	struct miscdevice miscdev;
	spinlock_t lock;
	struct vpss_core vpss_cores[CVI_VPSS_MAX];
};


#endif /* _VPSS_CORE_H_ */
