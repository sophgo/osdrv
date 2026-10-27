#ifndef __CVI_MON_INTERFACE_H__
#define __CVI_MON_INTERFACE_H__

#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/dma-direction.h>
#include <linux/version.h>



struct cvi_mon_work {
	struct task_struct *work_thread;
	wait_queue_head_t task_wait_queue;
	wait_queue_head_t done_wait_queue;
	struct list_head task_list;
	spinlock_t task_list_lock;
	struct list_head done_list;
	spinlock_t done_list_lock;
};

struct cvi_mon_device {
	struct device *dev;
	dev_t cdev_id;
	struct cdev cdev;
	struct completion aximon_completion;
	uint8_t __iomem *pcmon_vaddr;
	uint8_t __iomem *ddr_ctrl_vaddr;		//0x08004000
	uint8_t __iomem *ddr_phyd_vaddr;		//0x08006000
#ifndef CONFIG_ARCH_CV186X
	uint8_t __iomem *ddr_aximon_vaddr;	//0x08008000
#else
	uint8_t __iomem *ddr_aximon_real_vaddr;	//0x08008000
	uint8_t __iomem *ddr_aximon_offline_vaddr;	//0x08008000
	uint8_t __iomem *ddr_aximon_bulk_vaddr;	//0x08008000
	uint8_t __iomem *ddr_ddrmon_sys1_vaddr;
	uint8_t __iomem *ddr_ddrmon_sys2_vaddr;
#endif
	uint8_t __iomem *ddr_top_vaddr;			//0x0800A000
#if defined(CONFIG_CHIP_CPU_CV84X6)
	void __iomem *mon_f0s1_vaddr;
	void __iomem *mon_f1s1_vaddr;
	void __iomem *mon_f2s1_vaddr;
	void __iomem *mon_f3s1_vaddr;
	void __iomem *mon_f5s1_vaddr;
	void __iomem *mon_vd0_mon0_vaddr;
	void __iomem *mon_vd0_mon1_vaddr;
	void __iomem *mon_vd1_mon0_vaddr;
	void __iomem *mon_vd1_mon1_vaddr;
	void __iomem *mon_vd2_mon0_vaddr;
	void __iomem *mon_vd2_mon1_vaddr;
	void __iomem *mon_vd3_mon0_vaddr;
	void __iomem *mon_vd3_mon1_vaddr;
	void __iomem *mon_ve_vaddr;
	/* VO monitors (no IRQ) */
	void __iomem *mon_vo_iga_vaddr;
	void __iomem *mon_vo_off_vaddr;
	void __iomem *mon_vo_rt_vaddr;
	/* TPU monitors (no IRQ) */
	void __iomem *mon_tpu_mon0_vaddr;
	void __iomem *mon_tpu_mon1_vaddr;
	void __iomem *mon_tpu_mon2_vaddr;
	void __iomem *mon_tpu_mon3_vaddr;
	void __iomem *mon_tpu_mon4_vaddr;
	void __iomem *mon_tpu_mon5_vaddr;
	/* DDR monitors MON0 (no IRQ) - 0x70c06000, 0x74c06000, 0x78c06000, 0x7cc06000 */
	void __iomem *mon_ddr0_mon0_vaddr;
	void __iomem *mon_ddr1_mon0_vaddr;
	void __iomem *mon_ddr2_mon0_vaddr;
	void __iomem *mon_ddr3_mon0_vaddr;
	/* DDR monitors MON1 (no IRQ) - 0x70c06100, 0x74c06100, 0x78c06100, 0x7cc06100 */
	void __iomem *mon_ddr0_mon1_vaddr;
	void __iomem *mon_ddr1_mon1_vaddr;
	void __iomem *mon_ddr2_mon1_vaddr;
	void __iomem *mon_ddr3_mon1_vaddr;
	/* Physical addresses for debugging */
	phys_addr_t mon_paddr[31];
#endif
	int aximon_irq;
	struct mutex dev_lock;
	spinlock_t close_lock;
	int use_count;
	void *private_data;
	struct cvi_mon_work mon_work;
};

#endif

