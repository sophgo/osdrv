#ifndef _VPSS_CORE_H_
#define _VPSS_CORE_H_

#include "vpss_define.h"
#include "comm_sys.h"
#include "comm_vpss.h"
#include "vpss_hal.h"
#include "vpss_ctx.h"

#define VIP_MAX_PLANES 3


struct vpss_core {
	enum vpss_dev vpss_type;
	unsigned int irq_num;
	osal_clk *clk;
	osal_atomic state;
	osal_spinlock core_lock;
	struct vpss_interrupter_status intr_status;
	u8 is_master;
	u8 is_sbm;
	u8 reset_sbm;
	u8 vc_ready;
	u8 chn_idx;
	u8 tile_mode;
	u32 checksum;
	u32 start_cnt;
	u32 int_cnt;
	void *device;
};


struct vpss_cores {
	osal_spinlock lock;
	osal_timer timer;
	struct vpss_core core[VPSS_MAX];
	struct vpss_device device[VPSS_DEVICE_NUM];
	struct vpss_ctx ctx;
	struct vpss_hal_ctx hal_ctx;
	vpss_mode_s vpss_mode;
	vi_vpss_mode_s vi_vpss_mode;
};

void vpss_core_init(struct vpss_cores *cores);
void vpss_core_deinit(struct vpss_cores *cores);
void vpss_core_open(struct vpss_cores *cores);
void vpss_core_release(struct vpss_cores *cores);
void vpss_core_stop(struct vpss_cores *cores);

void vpss_core_isr(int irq, void *data);
void vpss_core_set_mode(struct vpss_cores *cores, const vpss_mode_s *vpss_mode);

int vpss_core_suspend(struct vpss_cores *cores);
int vpss_core_resume(struct vpss_cores *cores);

#endif /* _VPSS_CORE_H_ */
