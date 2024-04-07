/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_vcodec.c
 * Description:
 */

#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_graph.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/wait.h>
#include <linux/streamline_annotate.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/signal.h>
#endif

#include "vpuconfig.h"
#include "cvi_vcodec.h"
#include "vcodec_common.h"

/* definitions to be changed as customer  configuration */
/* if you want to have clock gating scheme frame by frame */
//#define VPU_SUPPORT_CLOCK_CONTROL

/* if the driver want to use interrupt service from kernel ISR */
//#define VPU_SUPPORT_ISR

/* if the platform driver knows the name of this driver */
/* VPU_PLATFORM_DEVICE_NAME */
//#define VPU_SUPPORT_PLATFORM_DRIVER_REGISTER

/* if this driver knows the dedicated video memory address */
#define VPU_SUPPORT_RESERVED_VIDEO_MEMORY

/* global device context to avoid kernal config mismatch in filp->private_data */
#define VPU_SUPPORT_GLOBAL_DEVICE_CONTEXT

#define VPU_PLATFORM_DEVICE_NAME	"vcodec"
#define VPU_CLK_NAME			"vcodec"
#define VPU_CLASS_NAME			"vcodec"
#define VPU_DEV_NAME			"vcodec"


/* if the platform driver knows this driver */
/* the definition of VPU_REG_BASE_ADDR and VPU_REG_SIZE are not meaningful */

#define VPU_REG_BASE_ADDR 0xb020000
#define VPU_REG_SIZE (0x4000 * MAX_NUM_VPU_CORE)

#define VPU_IRQ_NUM (77 + 32)

/* this definition is only for chipsnmedia FPGA board env */
/* so for SOC env of customers can be ignored */

#ifndef VM_RESERVED /*for kernel up to 3.7.0 version*/
#define VM_RESERVED (VM_DONTEXPAND | VM_DONTDUMP)
#endif

/* To track the allocated memory buffer */
typedef struct vpudrv_buffer_pool_t {
	struct list_head list;
	struct vpudrv_buffer_t vb;
	struct file *filp;
} vpudrv_buffer_pool_t;

/* To track the instance index and buffer in instance pool */
typedef struct vpudrv_instanace_list_t {
	struct list_head list;
	unsigned long inst_idx;
	unsigned long core_idx;
	struct file *filp;
} vpudrv_instanace_list_t;

typedef struct vpudrv_instance_pool_t {
	unsigned char codecInstPool[MAX_NUM_INSTANCE][MAX_INST_HANDLE_SIZE];
} vpudrv_instance_pool_t;

struct clk_ctrl_info {
	int core_idx;
	int enable;
};

#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
#define VPU_INIT_VIDEO_MEMORY_SIZE_IN_BYTE (62 * 1024 * 1024)
#define VPU_DRAM_PHYSICAL_BASE 0x86C00000
#include "vmm.h"
static video_mm_t s_vmem;
static vpudrv_buffer_t s_video_memory = { 0 };
#endif /*VPU_SUPPORT_RESERVED_VIDEO_MEMORY*/

static void *pVencDbgShm;
static void *pVencShm;
static void *pVdecDbgShm;
static void *pVdecShm;

#ifdef VPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
// global variable to avoid kernal config mismatch in filp->private_data
static void *pCviVpuDevice;
#endif

static int vpu_hw_reset(void);

static int s_vpu_open_ref_count;

static wait_queue_head_t hasBsWaitQ;

static spinlock_t s_vpu_lock = __SPIN_LOCK_UNLOCKED(s_vpu_lock);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
static DECLARE_MUTEX(s_vpu_sem);
#else
static DEFINE_SEMAPHORE(s_vpu_sem);
#endif
static struct list_head s_vbp_head = LIST_HEAD_INIT(s_vbp_head);
static struct list_head s_inst_list_head = LIST_HEAD_INIT(s_inst_list_head);

struct cvi_vcodec_device vcodec_dev;

/* implement to power management functions */
#define BIT_BASE 0x0000
#define BIT_CODE_RUN (BIT_BASE + 0x000)
#define BIT_CODE_DOWN (BIT_BASE + 0x004)
#define BIT_INT_CLEAR (BIT_BASE + 0x00C)
#define BIT_INT_STS (BIT_BASE + 0x010)
#define BIT_CODE_RESET (BIT_BASE + 0x014)
#define BIT_INT_REASON (BIT_BASE + 0x174)
#define BIT_BUSY_FLAG (BIT_BASE + 0x160)
#define BIT_RUN_COMMAND (BIT_BASE + 0x164)
#define BIT_RUN_INDEX (BIT_BASE + 0x168)
#define BIT_RUN_COD_STD (BIT_BASE + 0x16C)

/* WAVE4 registers */
#define W4_REG_BASE 0x0000
#define W4_VPU_BUSY_STATUS (W4_REG_BASE + 0x0070)
#define W4_VPU_INT_REASON_CLEAR (W4_REG_BASE + 0x0034)
#define W4_VPU_VINT_CLEAR (W4_REG_BASE + 0x003C)
#define W4_VPU_VPU_INT_STS (W4_REG_BASE + 0x0044)
#define W4_VPU_INT_REASON (W4_REG_BASE + 0x004c)

#define W4_RET_SUCCESS (W4_REG_BASE + 0x0110)
#define W4_RET_FAIL_REASON (W4_REG_BASE + 0x0114)

/* WAVE4 INIT, WAKEUP */
#define W4_PO_CONF (W4_REG_BASE + 0x0000)
#define W4_VCPU_CUR_PC (W4_REG_BASE + 0x0004)

#define W4_VPU_VINT_ENABLE (W4_REG_BASE + 0x0048)

#define W4_VPU_RESET_REQ (W4_REG_BASE + 0x0050)
#define W4_VPU_RESET_STATUS (W4_REG_BASE + 0x0054)

#define W4_VPU_REMAP_CTRL (W4_REG_BASE + 0x0060)
#define W4_VPU_REMAP_VADDR (W4_REG_BASE + 0x0064)
#define W4_VPU_REMAP_PADDR (W4_REG_BASE + 0x0068)
#define W4_VPU_REMAP_CORE_START (W4_REG_BASE + 0x006C)
#define W4_VPU_BUSY_STATUS (W4_REG_BASE + 0x0070)

#define W4_REMAP_CODE_INDEX 0
enum {
	W4_INT_INIT_VPU = 0,
	W4_INT_DEC_PIC_HDR = 1,
	W4_INT_FINI_SEQ = 2,
	W4_INT_DEC_PIC = 3,
	W4_INT_SET_FRAMEBUF = 4,
	W4_INT_FLUSH_DEC = 5,
	W4_INT_GET_FW_VERSION = 9,
	W4_INT_QUERY_DEC = 10,
	W4_INT_SLEEP_VPU = 11,
	W4_INT_WAKEUP_VPU = 12,
	W4_INT_CHANGE_INT = 13,
	W4_INT_CREATE_INSTANCE = 14,
	W4_INT_BSBUF_EMPTY = 15, /*!<< Bitstream buffer empty */
	W4_INT_ENC_SLICE_INT = 15,
};

enum {
	W5_INT_INIT_VPU = 0,
	W5_INT_WAKEUP_VPU = 1,
	W5_INT_SLEEP_VPU = 2,
	W5_INT_CREATE_INSTANCE = 3,
	W5_INT_FLUSH_INSTANCE = 4,
	W5_INT_DESTROY_INSTANCE = 5,
	W5_INT_INIT_SEQ = 6,
	W5_INT_SET_FRAMEBUF = 7,
	W5_INT_DEC_PIC = 8,
	W5_INT_ENC_PIC = 8,
	W5_INT_ENC_SET_PARAM = 9,
	W5_INT_DEC_QUERY = 14,
	W5_INT_BSBUF_EMPTY = 15,
};

#define W4_HW_OPTION (W4_REG_BASE + 0x0124)
#define W4_CODE_SIZE (W4_REG_BASE + 0x011C)
/* Note: W4_INIT_CODE_BASE_ADDR should be aligned to 4KB */
#define W4_ADDR_CODE_BASE (W4_REG_BASE + 0x0118)
#define W4_CODE_PARAM (W4_REG_BASE + 0x0120)
#define W4_INIT_VPU_TIME_OUT_CNT (W4_REG_BASE + 0x0134)

/************************************************************************/
/* DECODER - DEC_PIC_HDR/DEC_PIC                                        */
/************************************************************************/
#define W4_BS_PARAM (W4_REG_BASE + 0x0128)
#define W4_BS_RD_PTR (W4_REG_BASE + 0x0130)
#define W4_BS_WR_PTR (W4_REG_BASE + 0x0134)

/* WAVE5 registers */
#define W5_ADDR_CODE_BASE (W4_REG_BASE + 0x0110)
#define W5_CODE_SIZE (W4_REG_BASE + 0x0114)
#define W5_CODE_PARAM (W4_REG_BASE + 0x0128)
#define W5_INIT_VPU_TIME_OUT_CNT (W4_REG_BASE + 0x0130)

#define W5_HW_OPTION (W4_REG_BASE + 0x012C)

#define W5_RET_SUCCESS (W4_REG_BASE + 0x0108)

/* WAVE4 Wave4BitIssueCommand */
#define W4_CORE_INDEX (W4_REG_BASE + 0x0104)
#define W4_INST_INDEX (W4_REG_BASE + 0x0108)
#define W4_COMMAND (W4_REG_BASE + 0x0100)
#define W4_VPU_HOST_INT_REQ (W4_REG_BASE + 0x0038)

/* Product register */
#define VPU_PRODUCT_CODE_REGISTER (BIT_BASE + 0x1044)

/* Clock enable/disable */
#define VCODEC_CLK_ENABLE 1
#define VCODEC_CLK_DISABLE 0

static u32 s_vpu_reg_store[MAX_NUM_VPU_CORE][64];

#define ReadVpuRegister(addr)	\
	(readl(pvctx->s_vpu_register.virt_addr + \
			pvctx->s_bit_firmware_info.reg_base_offset + \
			(addr)))
#define WriteVpuRegister(addr, val)	\
	(writel((val), pvctx->s_vpu_register.virt_addr + \
			pvctx->s_bit_firmware_info.reg_base_offset + \
			(addr)))
#define WriteVpu(addr, val)	\
	(writel((val), (addr)))

static int bSingleCore;
module_param(bSingleCore, int, 0644);

static int cviVpuHasBs(struct cvi_vcodec_context *pvctx, int intr);
static int cviIoctlGetInstPool(u_long arg);
static int cviGetRegResource(struct cvi_vpu_device *vdev, struct platform_device *pdev);
static int cvi_vcodec_register_cdev(struct cvi_vpu_device *vdev);
static int cviCfgIrq(struct platform_device *pdev);
static void cviFreeIrq(void);
static void cviReleaseRegResource(struct cvi_vpu_device *vdev);
static void cviUnmapReg(vpudrv_buffer_t *pReg);
static int cvi_vcodec_allocate_memory(struct platform_device *pdev);

static void set_clock_enable(struct cvi_vpu_device *vdev, int enable, int mask)
{
	if (vdev->pdata->quirks & (VCODEC_QUIRK_SUPPORT_CLOCK_CONTROL | VCDOEC_QUIRK_SUPPORT_FPGA)) {
		if (enable) {
			if (vdev->pdata->ops->clk_enable)
				vdev->pdata->ops->clk_enable(vdev, mask);
		} else {
			if (vdev->pdata->ops->clk_disable)
				vdev->pdata->ops->clk_disable(vdev, mask);
		}
	}
}

static int vpu_alloc_dma_buffer(vpudrv_buffer_t *vb)
{
	if (!vb)
		return -1;

	VCODEC_DBG_TRACE("size = 0x%X\n", vb->size);
#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
	vb->phys_addr = (unsigned long)vmem_alloc(&s_vmem, vb->size, 0);
	if ((unsigned long)vb->phys_addr == (unsigned long)-1) {
		VCODEC_DBG_ERR("Physical memory allocation error size=%d\n",
		       vb->size);
		return -1;
	}

	vb->base = (unsigned long)(s_video_memory.base +
				   (vb->phys_addr - s_video_memory.phys_addr));
#else
	vb->base = (unsigned long)dma_alloc_coherent(
		NULL, PAGE_ALIGN(vb->size), (dma_addr_t *)(&vb->phys_addr),
		GFP_DMA | GFP_KERNEL);
	if ((void *)(vb->base) == NULL) {
		VCODEC_DBG_ERR("Physical memory allocation error size=%d\n",
		       vb->size);
		return -1;
	}
#endif
	return 0;
}

static void vpu_free_dma_buffer(vpudrv_buffer_t *vb)
{
	if (!vb)
		return;

#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
	if (vb->base)
		vmem_free(&s_vmem, vb->phys_addr, 0);
#else
	if (vb->base)
		dma_free_coherent(0, PAGE_ALIGN(vb->size), (void *)vb->base,
				  vb->phys_addr);
#endif
}

static int vpu_free_instances(struct file *filp)
{
	vpudrv_instanace_list_t *vil, *n;
	vpudrv_instance_pool_t *vip;
	void *vip_base;
	int instance_pool_size_per_core;
	void *vdi_mutexes_base;
	const int PTHREAD_MUTEX_T_DESTROY_VALUE = 0xdead10cc;

	instance_pool_size_per_core =
		(vcodec_dev.s_instance_pool.size /
		 MAX_NUM_VPU_CORE);
	/* s_instance_pool.size assigned to the size of all core */
	/* once call VDI_IOCTL_GET_INSTANCE_POOL by user. */

	list_for_each_entry_safe(vil, n, &s_inst_list_head, list) {
		if (vil->filp == filp) {
			vip_base = (void *)(vcodec_dev.s_instance_pool.base +
					    (instance_pool_size_per_core *
					     vil->core_idx));
			VCODEC_DBG_INFO("detect instance crash instIdx=%d, coreIdx=%d\n",
				(int)vil->inst_idx, (int)vil->core_idx);
			vip = (vpudrv_instance_pool_t *)vip_base;
			if (vip) {
				memset(&vip->codecInstPool[vil->inst_idx], 0x00, 4);
				/* only first 4 byte is key point		*/
				/* (in Use of CodecInst in vpuapi)		*/
				/* to free the corresponding instance.	*/

#define PTHREAD_MUTEX_T_HANDLE_SIZE 4
				vdi_mutexes_base =
					(vip_base +
					 (instance_pool_size_per_core -
					  PTHREAD_MUTEX_T_HANDLE_SIZE * 4));
				VCODEC_DBG_INFO("force to destroy mutex = %p in userspace\n",
					vdi_mutexes_base);
				if (vdi_mutexes_base) {
					int i;

					for (i = 0; i < 4; i++) {
						memcpy(vdi_mutexes_base,
						       &PTHREAD_MUTEX_T_DESTROY_VALUE,
						       PTHREAD_MUTEX_T_HANDLE_SIZE);
						vdi_mutexes_base +=
							PTHREAD_MUTEX_T_HANDLE_SIZE;
					}
				}
			}
			s_vpu_open_ref_count--;
			list_del(&vil->list);
			kfree(vil);
		}
	}
	return 1;
}

static int vpu_free_buffers(struct file *filp)
{
	vpudrv_buffer_pool_t *pool, *n;
	vpudrv_buffer_t vb;

	list_for_each_entry_safe(pool, n, &s_vbp_head, list) {
		if (pool->filp == filp) {
			vb = pool->vb;
			if (vb.base) {
				vpu_free_dma_buffer(&vb);
				list_del(&pool->list);
				kfree(pool);
			}
		}
	}

	return 0;
}

static irqreturn_t vpu_irq_handler(int irq, void *dev_id)
{
	vpu_drv_context_t *dev = &vcodec_dev.s_vpu_drv_context;
	struct cvi_vcodec_context *pvctx = (struct cvi_vcodec_context *) dev_id;

	/* this can be removed. it also work in VPU_WaitInterrupt of API function */
	int product_code;

	VCODEC_DBG_INTR("[+]%s\n", __func__);

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
#else
	disable_irq_nosync(pvctx->s_vcodec_irq);
#endif

	if (pvctx->s_bit_firmware_info.size ==
			0) {
		/* it means that we didn't get an information		  */
		/* the current core from API layer. No core activated.*/
		VCODEC_DBG_ERR("s_bit_firmware_info.size is zero\n");
		return IRQ_HANDLED;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	ktime_get_ts64(&pvctx->irq_timestamp);
#else
	ktime_get_ts(&pvctx->irq_timestamp);
#endif

	product_code = ReadVpuRegister(VPU_PRODUCT_CODE_REGISTER);

	VCODEC_DBG_TRACE("product_code = 0x%X\n", product_code);

	if (PRODUCT_CODE_W_SERIES(product_code)) {
		if (ReadVpuRegister(W4_VPU_VPU_INT_STS)) {
			pvctx->interrupt_reason =
				ReadVpuRegister(W4_VPU_INT_REASON);
			WriteVpuRegister(W4_VPU_INT_REASON_CLEAR,
					pvctx->interrupt_reason);
			WriteVpuRegister(W4_VPU_VINT_CLEAR, 0x1);
		}

		if (cviVpuHasBs(pvctx, pvctx->interrupt_reason)) {
			wake_up_interruptible(&hasBsWaitQ);
		}
	} else if (PRODUCT_CODE_NOT_W_SERIES(product_code)) {
		if (ReadVpuRegister(BIT_INT_STS)) {
			pvctx->interrupt_reason =
				ReadVpuRegister(BIT_INT_REASON);
			WriteVpuRegister(BIT_INT_CLEAR, 0x1);
		}
	} else {
		VCODEC_DBG_ERR("Unknown product id : %08x\n",
				product_code);
		return IRQ_HANDLED;
	}

	VCODEC_DBG_INTR("product: 0x%08x intr_reason: 0x%08lx\n",
			product_code, pvctx->interrupt_reason);
	VCODEC_DBG_INTR("intr_reason: 0x%lX\n",
			pvctx->interrupt_reason);

	if (dev->async_queue)
		kill_fasync(&dev->async_queue, SIGIO,
				POLL_IN); /* notify the interrupt to user space */

	pvctx->s_interrupt_flag = 1;

	wake_up_interruptible(&pvctx->s_interrupt_wait_q);
	VCODEC_DBG_INTR("[-]%s\n", __func__);

	return IRQ_HANDLED;
}

static int cviVpuHasBs(struct cvi_vcodec_context *pvctx, int intr)
{
	u32 ringBufferEnable, rptr, wptr;
	int ret = 0;

	if (intr & (1 << W4_INT_DEC_PIC)) {
		ringBufferEnable = ReadVpuRegister(W4_BS_PARAM) & (1 << 4);

		if (ringBufferEnable == 0) {
			rptr = ReadVpuRegister(W4_BS_RD_PTR);
			wptr = ReadVpuRegister(W4_BS_WR_PTR);

			VCODEC_DBG_TRACE("rptr = 0x%X, wptr = 0x%X\n", rptr, wptr);

			if (wptr > rptr) {
				ret = 1;
			} else {
				VCODEC_DBG_INFO("no bs\n");
			}
		} else {
			VCODEC_DBG_INFO("ringBufferEnable = 1\n");
		}
	}

	return ret;
}

static int vpu_open(struct inode *inode, struct file *filp)
{
	#ifndef VPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_vpu_device *vdev =
		container_of(inode->i_cdev, struct cvi_vpu_device, cdev);
	#endif

	spin_lock(&s_vpu_lock);

	vcodec_dev.s_vpu_drv_context.open_count++;

	spin_unlock(&s_vpu_lock);

	#ifndef VPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	filp->private_data = vdev;
	#endif

	return 0;
}

/*static int vpu_ioctl(struct inode *inode, struct file *filp, u_int cmd, u_long arg) // for kernel 2.6.9 of C&M*/
static long vpu_ioctl(struct file *filp, u_int cmd, u_long arg)
{
	#ifdef VPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_vpu_device *vdev = (struct cvi_vpu_device *)pCviVpuDevice;
	#else
	struct cvi_vpu_device *vdev = filp->private_data;
	#endif
	struct cvi_vcodec_context *pvctx = NULL;
	int ret = 0;

	VCODEC_DBG_INFO("cmd %d, filp %p\n", cmd, filp);

	switch (cmd) {
	case VDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY: {
		vpudrv_buffer_pool_t *vbp;

		VCODEC_DBG_TRACE("[+]VDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY\n");

		ret = down_interruptible(&s_vpu_sem);
		if (ret == 0) {
			vbp = kzalloc(sizeof(*vbp), GFP_KERNEL);
			if (!vbp) {
				up(&s_vpu_sem);
				return -ENOMEM;
			}

			ret = copy_from_user(&(vbp->vb), (vpudrv_buffer_t *)arg,
					     sizeof(vpudrv_buffer_t));
			if (ret) {
				kfree(vbp);
				up(&s_vpu_sem);
				return -EFAULT;
			}

			ret = vpu_alloc_dma_buffer(&(vbp->vb));
			if (ret == -1) {
				ret = -ENOMEM;
				kfree(vbp);
				up(&s_vpu_sem);
				break;
			}
			ret = copy_to_user((void __user *)arg, &(vbp->vb),
					   sizeof(vpudrv_buffer_t));
			if (ret) {
				kfree(vbp);
				ret = -EFAULT;
				up(&s_vpu_sem);
				break;
			}

			vbp->filp = filp;
			spin_lock(&s_vpu_lock);
			list_add(&vbp->list, &s_vbp_head);
			spin_unlock(&s_vpu_lock);

			up(&s_vpu_sem);
		}
		VCODEC_DBG_TRACE("[-]VDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY\n");
	} break;
	case VDI_IOCTL_FREE_PHYSICALMEMORY: {
		vpudrv_buffer_pool_t *vbp, *n;
		vpudrv_buffer_t vb;

		VCODEC_DBG_INFO("[+]VDI_IOCTL_FREE_PHYSICALMEMORY\n");

		ret = down_interruptible(&s_vpu_sem);
		if (ret == 0) {
			ret = copy_from_user(&vb, (vpudrv_buffer_t *)arg,
					     sizeof(vpudrv_buffer_t));
			if (ret) {
				up(&s_vpu_sem);
				return -EACCES;
			}

			if (vb.base)
				vpu_free_dma_buffer(&vb);

			spin_lock(&s_vpu_lock);
			list_for_each_entry_safe(vbp, n, &s_vbp_head, list) {
				if (vbp->vb.base == vb.base) {
					list_del(&vbp->list);
					kfree(vbp);
					break;
				}
			}
			spin_unlock(&s_vpu_lock);

			up(&s_vpu_sem);
		}
		VCODEC_DBG_INFO("[-]VDI_IOCTL_FREE_PHYSICALMEMORY\n");

	} break;
	case VDI_IOCTL_GET_RESERVED_VIDEO_MEMORY_INFO: {
#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
		if (s_video_memory.base != 0) {
			ret = copy_to_user((void __user *)arg, &s_video_memory,
					   sizeof(vpudrv_buffer_t));
			if (ret != 0)
				ret = -EFAULT;
		} else {
			ret = -EFAULT;
		}
#endif
	} break;

	case VDI_IOCTL_WAIT_INTERRUPT: {
		vpudrv_intr_info_t info;

		VCODEC_DBG_INTR("[+]VDI_IOCTL_WAIT_INTERRUPT\n");
		ret = copy_from_user(&info, (vpudrv_intr_info_t *)arg,
				     sizeof(vpudrv_intr_info_t));
		if (ret != 0 || info.coreIdx < 0 || info.coreIdx >= MAX_NUM_VPU_CORE)
			return -EFAULT;

		pvctx = &vcodec_dev.vcodec_ctx[info.coreIdx];

		VCODEC_DBG_TRACE("coreIdx = %d, s_interrupt_flag = 0x%X, timeout = %d\n",
				info.coreIdx, pvctx->s_interrupt_flag, info.timeout);

		ret = wait_event_interruptible_timeout(
			pvctx->s_interrupt_wait_q, pvctx->s_interrupt_flag != 0,
			msecs_to_jiffies(info.timeout));
		if (!ret) {
			ret = -ETIME;
			break;
		}

		ANNOTATE_CHANNEL_COLOR(1, ANNOTATE_GREEN, "vcodec end");
		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			break;
		}

		VCODEC_DBG_INTR("s_interrupt_flag(%d), reason(0x%08lx)\n",
			pvctx->s_interrupt_flag, pvctx->interrupt_reason);

		info.intr_reason = pvctx->interrupt_reason;
		info.intr_tv_sec = pvctx->irq_timestamp.tv_sec;
		info.intr_tv_nsec = pvctx->irq_timestamp.tv_nsec;
		pvctx->s_interrupt_flag = 0;
		pvctx->interrupt_reason = 0;
		ret = copy_to_user((void __user *)arg, &info,
				   sizeof(vpudrv_intr_info_t));
		VCODEC_DBG_INTR("[-]VDI_IOCTL_WAIT_INTERRUPT\n");

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
#else
		enable_irq(pvctx->s_vcodec_irq);
#endif

		if (ret != 0)
			return -EFAULT;
		ANNOTATE_CHANNEL_END(1);
		ANNOTATE_NAME_CHANNEL(1, 1, "vcodec end");
	} break;

	case VDI_IOCTL_SET_CLOCK_GATE: {
		u32 clkgate;

		if (get_user(clkgate, (u32 __user *)arg))
			return -EFAULT;

		VCODEC_DBG_INFO("[+]VDI_IOCTL_SET_CLOCK_GATE %d\n", clkgate);

		if (clkgate)
			set_clock_enable(vdev, VCODEC_CLK_ENABLE, BIT(H264_CORE_IDX) | BIT(H265_CORE_IDX));
		else
			set_clock_enable(vdev, VCODEC_CLK_DISABLE, BIT(H264_CORE_IDX) | BIT(H265_CORE_IDX));

	} break;
	case VDI_IOCTL_SET_CLOCK_GATE_EXT: {
		struct clk_ctrl_info info;

		ret = copy_from_user(&info, (struct clk_ctrl_info *)arg, sizeof(struct clk_ctrl_info));
		if (ret != 0 || info.core_idx < 0 || info.core_idx >= MAX_NUM_VPU_CORE)
			return -EFAULT;

		VCODEC_DBG_INFO("[+]VDI_IOCTL_SET_CLOCK_GATE_EXT core %d en %d\n", info.core_idx, info.enable);
		VCODEC_DBG_INFO("vdev %p\n", vdev);
		VCODEC_DBG_INFO("vdev->pdata->quirks %d\n", vdev->pdata->quirks);

		set_clock_enable(vdev, info.enable, 1 << info.core_idx);
	} break;

	case VDI_IOCTL_GET_INSTANCE_POOL: {
		VCODEC_DBG_TRACE("[+]VDI_IOCTL_GET_INSTANCE_POOL\n");

		ret = cviIoctlGetInstPool(arg);

		VCODEC_DBG_TRACE("[-]VDI_IOCTL_GET_INSTANCE_POOL\n");
	} break;
	case VDI_IOCTL_GET_COMMON_MEMORY: {
		VCODEC_DBG_TRACE("[+]VDI_IOCTL_GET_COMMON_MEMORY\n");
		if (vcodec_dev.s_common_memory.base != 0) {
			ret = copy_to_user((void __user *)arg, &vcodec_dev.s_common_memory,
					   sizeof(vpudrv_buffer_t));
			if (ret != 0)
				ret = -EFAULT;
		} else {
			ret = copy_from_user(&vcodec_dev.s_common_memory,
					     (vpudrv_buffer_t *)arg,
					     sizeof(vpudrv_buffer_t));
			if (ret == 0) {
				if (vpu_alloc_dma_buffer(&vcodec_dev.s_common_memory) !=
				    -1) {
					ret = copy_to_user(
						(void __user *)arg,
						&vcodec_dev.s_common_memory,
						sizeof(vpudrv_buffer_t));
					if (ret == 0) {
						/* success to get memory for common memory */
						break;
					}
				}
			}

			ret = -EFAULT;
		}
		VCODEC_DBG_TRACE("[-]VDI_IOCTL_GET_COMMON_MEMORY\n");
	} break;
	case VDI_IOCTL_OPEN_INSTANCE: {
		vpudrv_inst_info_t inst_info;
		vpudrv_instanace_list_t *vil, *n;

		vil = kzalloc(sizeof(*vil), GFP_KERNEL);
		if (!vil)
			return -ENOMEM;

		if (copy_from_user(&inst_info, (vpudrv_inst_info_t *)arg,
				   sizeof(vpudrv_inst_info_t)))
			return -EFAULT;

		vil->inst_idx = inst_info.inst_idx;
		vil->core_idx = inst_info.core_idx;
		vil->filp = filp;

		spin_lock(&s_vpu_lock);
		list_add(&vil->list, &s_inst_list_head);

		inst_info.inst_open_count =
			0; /* counting the current open instance number */
		list_for_each_entry_safe(vil, n, &s_inst_list_head, list) {
			if (vil->core_idx == inst_info.core_idx)
				inst_info.inst_open_count++;
		}
		spin_unlock(&s_vpu_lock);

		s_vpu_open_ref_count++; /* flag just for that vpu is in opened or closed */

		if (copy_to_user((void __user *)arg, &inst_info,
				 sizeof(vpudrv_inst_info_t))) {
			kfree(vil);
			return -EFAULT;
		}

		VCODEC_DBG_TRACE("core_idx=%d, inst_idx=%d, ref cnt=%d, inst cnt=%d\n",
			(int)inst_info.core_idx, (int)inst_info.inst_idx,
			s_vpu_open_ref_count, inst_info.inst_open_count);
	} break;
	case VDI_IOCTL_CLOSE_INSTANCE: {
		vpudrv_inst_info_t inst_info;
		vpudrv_instanace_list_t *vil, *n;

		VCODEC_DBG_TRACE("[+]VDI_IOCTL_CLOSE_INSTANCE\n");
		if (copy_from_user(&inst_info, (vpudrv_inst_info_t *)arg,
				   sizeof(vpudrv_inst_info_t)))
			return -EFAULT;

		spin_lock(&s_vpu_lock);
		list_for_each_entry_safe(vil, n, &s_inst_list_head, list) {
			if (vil->inst_idx == inst_info.inst_idx &&
			    vil->core_idx == inst_info.core_idx) {
				list_del(&vil->list);
				kfree(vil);
				break;
			}
		}

		inst_info.inst_open_count =
			0; /* counting the current open instance number */
		list_for_each_entry_safe(vil, n, &s_inst_list_head, list) {
			if (vil->core_idx == inst_info.core_idx)
				inst_info.inst_open_count++;
		}
		spin_unlock(&s_vpu_lock);

		s_vpu_open_ref_count--; /* flag just for that vpu is in opened or closed */

		if (copy_to_user((void __user *)arg, &inst_info,
				 sizeof(vpudrv_inst_info_t)))
			return -EFAULT;

		VCODEC_DBG_TRACE("core_idx=%d, inst_idx=%d, ref cnt=%d, inst cnt=%d\n",
			(int)inst_info.core_idx, (int)inst_info.inst_idx,
			s_vpu_open_ref_count, inst_info.inst_open_count);
	} break;
	case VDI_IOCTL_GET_INSTANCE_NUM: {
		vpudrv_inst_info_t inst_info;
		vpudrv_instanace_list_t *vil, *n;

		VCODEC_DBG_TRACE("[+]VDI_IOCTL_GET_INSTANCE_NUM\n");

		ret = copy_from_user(&inst_info, (vpudrv_inst_info_t *)arg,
				     sizeof(vpudrv_inst_info_t));
		if (ret != 0)
			break;

		inst_info.inst_open_count = 0;

		spin_lock(&s_vpu_lock);
		list_for_each_entry_safe(vil, n, &s_inst_list_head, list) {
			if (vil->core_idx == inst_info.core_idx)
				inst_info.inst_open_count++;
		}
		spin_unlock(&s_vpu_lock);

		ret = copy_to_user((void __user *)arg, &inst_info,
				   sizeof(vpudrv_inst_info_t));
		if (ret != 0)
			ret = -EFAULT;

		VCODEC_DBG_TRACE("core_idx=%d, inst_idx=%d, open_count=%d\n",
			(int)inst_info.core_idx, (int)inst_info.inst_idx,
			inst_info.inst_open_count);

	} break;
	case VDI_IOCTL_RESET: {
		vpu_hw_reset();
	} break;
	case VDI_IOCTL_GET_REGISTER_INFO: {
		vpudrv_buffer_t info;
		int core_idx;

		VCODEC_DBG_TRACE("[+]VDI_IOCTL_GET_REGISTER_INFO\n");

		ret = copy_from_user(&info, (vpudrv_buffer_t *)arg, sizeof(vpudrv_buffer_t));
		if (ret != 0)
			return -EFAULT;

		if (info.size >= VCODEC_GET_REG_BASE) {
			if (info.size == VCODEC_GET_REG_BASE_CTRL) {
				ret = copy_to_user((void __user *)arg, &vcodec_dev.ctrl_register,
						   sizeof(vpudrv_buffer_t));
				if (ret != 0)
					ret = -EFAULT;

				VCODEC_DBG_TRACE("[-]VC_CTRL, pa=0x%llx, va=0x%lx, size=%d\n",
					vcodec_dev.ctrl_register.phys_addr,
					(unsigned long int)vcodec_dev.ctrl_register.virt_addr,
					vcodec_dev.ctrl_register.size);
			} else if (info.size == VCODEC_GET_REG_BASE_ADDR_REMAP) {
				ret = copy_to_user((void __user *)arg, &vcodec_dev.remap_register,
						   sizeof(vpudrv_buffer_t));
				if (ret != 0)
					ret = -EFAULT;

				VCODEC_DBG_TRACE("[-]VC_ADDR_REMAP, pa=0x%llx, va=0x%lx, size=%d\n",
					vcodec_dev.remap_register.phys_addr,
					(unsigned long int)vcodec_dev.remap_register.virt_addr,
					vcodec_dev.remap_register.size);
			} else if (info.size == VCODEC_GET_REG_BASE_SBM) {
				ret = copy_to_user((void __user *)arg, &vcodec_dev.sbm_register,
						   sizeof(vpudrv_buffer_t));
				if (ret != 0)
					ret = -EFAULT;

				VCODEC_DBG_TRACE("[-]VC_SBM, pa=0x%llx, va=0x%lx, size=%d\n",
					vcodec_dev.sbm_register.phys_addr,
					(unsigned long int)vcodec_dev.sbm_register.virt_addr,
					vcodec_dev.sbm_register.size);
			} else {
				VCODEC_DBG_ERR("[-]No such register 0x%x\n", info.size);
				ret = -EFAULT;
			}
		} else {
			core_idx = info.size;
			pvctx = &vcodec_dev.vcodec_ctx[core_idx];
			vdev->pvctx = pvctx;

			VCODEC_DBG_TRACE("core_idx = %d\n", core_idx);

			ret = copy_to_user((void __user *)arg, &pvctx->s_vpu_register,
					   sizeof(vpudrv_buffer_t));
			if (ret != 0)
				ret = -EFAULT;

			VCODEC_DBG_TRACE("[-]pa=0x%llx, va=0x%lx, size=%d\n",
				pvctx->s_vpu_register.phys_addr,
				(unsigned long int)pvctx->s_vpu_register.virt_addr,
				pvctx->s_vpu_register.size);
		}
	} break;

	case VDI_IOCTL_GET_CHIP_VERSION: {
		ret = copy_to_user((void __user *)arg, &vdev->pdata->version,
				   sizeof(unsigned int));

		if (ret != 0)
			ret = -EFAULT;

		VCODEC_DBG_TRACE("VDI_IOCTL_GET_CHIP_VERSION chip_ver=0x%x\n", vdev->pdata->version);

	} break;

	case VDI_IOCTL_GET_CLOCK_FREQUENCY: {
		unsigned long clk_rate = 0;

		if (vdev->pdata->ops->clk_get_rate)
			clk_rate = vdev->pdata->ops->clk_get_rate(vdev);

		VCODEC_DBG_TRACE("VDI_IOCTL_GET_CLOCK_FREQENCY %lu\n", clk_rate);

		ret = copy_to_user((void __user *)arg, &clk_rate,
				   sizeof(unsigned long));

		if (ret != 0)
			ret = -EFAULT;

	} break;

	case VDI_IOCTL_RELEASE_COMMON_MEMORY: {
		vpudrv_buffer_t vb;

		VCODEC_DBG_INFO("[+]VDI_IOCTL_RELEASE_COMMON_MEMORY\n");

		ret = down_interruptible(&s_vpu_sem);
		if (ret == 0) {
			ret = copy_from_user(&vb, (vpudrv_buffer_t *)arg,
					     sizeof(vpudrv_buffer_t));
			if (ret) {
				up(&s_vpu_sem);
				return -EACCES;
			}

			if (vcodec_dev.s_common_memory.base == vb.base) {
				vpu_free_dma_buffer(&vcodec_dev.s_common_memory);
				vcodec_dev.s_common_memory.base = 0;
			} else {
				VCODEC_DBG_ERR("common memory addr mismatch, driver: 0x%llx user: 0x%llx\n",
					vcodec_dev.s_common_memory.base, vb.base);
				ret = -EFAULT;
			}
			up(&s_vpu_sem);
		}
		VCODEC_DBG_INFO("[-]VDI_IOCTL_RELEASE_COMMON_MEMORY\n");

	} break;

	case VDI_IOCTL_GET_SINGLE_CORE_CONFIG: {

		VCODEC_DBG_INFO("[+]VDI_IOCTL_GET_SINGLE_CORE_CONFIG\n");

		ret = copy_to_user((void __user *)arg, &bSingleCore,
				   sizeof(int));

		if (ret != 0) {
			VCODEC_DBG_ERR("copy_to_user fail with ret %d\n", ret);
			ret = -EFAULT;
		}

		VCODEC_DBG_INFO("[-]VDI_IOCTL_GET_SINGLE_CORE_CONFIG\n");

	} break;

	default: {
		VCODEC_DBG_ERR("No such IOCTL, cmd is %d\n", cmd);
	} break;
	}
	return ret;
}

static int cviIoctlGetInstPool(u_long arg)
{
	int ret = 0;

	ret = down_interruptible(&s_vpu_sem);
	if (ret == 0) {
		if (vcodec_dev.s_instance_pool.base != 0) {
			ret = copy_to_user((void __user *)arg,
					&vcodec_dev.s_instance_pool,
					sizeof(vpudrv_buffer_t));
			if (ret != 0)
				ret = -EFAULT;
		} else {
			ret = copy_from_user(&vcodec_dev.s_instance_pool,
					(vpudrv_buffer_t *)arg,
					sizeof(vpudrv_buffer_t));
			if (ret == 0) {
#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
				vcodec_dev.s_instance_pool.size = PAGE_ALIGN(
						vcodec_dev.s_instance_pool.size);
				vcodec_dev.s_instance_pool.base =
					(unsigned long)vmalloc(
							vcodec_dev.s_instance_pool.size);
				vcodec_dev.s_instance_pool.phys_addr =
					vcodec_dev.s_instance_pool.base;

				if (vcodec_dev.s_instance_pool.base != 0)
#else
				if (vpu_alloc_dma_buffer(
							&vcodec_dev.s_instance_pool) != -1)
#endif
				{
					memset((void *)vcodec_dev.s_instance_pool.base,
							0x0,
							vcodec_dev.s_instance_pool.size);
					ret = copy_to_user((void __user *)arg,
							&vcodec_dev.s_instance_pool,
							sizeof(vpudrv_buffer_t));

					if (ret == 0) {
						/* success to get memory for instance pool */
						up(&s_vpu_sem);
						return ret;
					}
				}
			}
			ret = -EFAULT;
		}

		up(&s_vpu_sem);
	}

	return ret;
}

static ssize_t vpu_write(struct file *filp, const char __user *buf, size_t len,
			 loff_t *ppos)
{
	VCODEC_DBG_TRACE("vpu_write len=%d\n", (int)len);

	if (!buf) {
		VCODEC_DBG_ERR("vpu_write buf = NULL error\n");
		return -EFAULT;
	}

	if (len == sizeof(vpu_bit_firmware_info_t)) {
		vpu_bit_firmware_info_t *bit_firmware_info;
		struct cvi_vcodec_context *pvctx;

		bit_firmware_info =
			kmalloc(sizeof(vpu_bit_firmware_info_t), GFP_KERNEL);
		if (!bit_firmware_info) {
			VCODEC_DBG_ERR("bit_firmware_info allocation error\n");
			return -EFAULT;
		}

		if (copy_from_user(bit_firmware_info, buf, len)) {
			VCODEC_DBG_ERR("copy_from_user error\n");
			return -EFAULT;
		}

		VCODEC_DBG_INFO("bit_firmware_info->size=%d %d\n",
				bit_firmware_info->size,
				(int)sizeof(vpu_bit_firmware_info_t));

		if (bit_firmware_info->size ==
		    sizeof(vpu_bit_firmware_info_t)) {
			VCODEC_DBG_INFO("bit_firmware_info size\n");

			if (bit_firmware_info->core_idx > MAX_NUM_VPU_CORE) {
				VCODEC_DBG_ERR("core_idx[%d] > MAX_NUM_VPU_CORE[%d]\n",
				       bit_firmware_info->core_idx,
				       MAX_NUM_VPU_CORE);
				return -ENODEV;
			}

			pvctx = &vcodec_dev.vcodec_ctx[bit_firmware_info->core_idx];
			memcpy((void *)&pvctx->s_bit_firmware_info,
			       bit_firmware_info,
			       sizeof(vpu_bit_firmware_info_t));
			kfree(bit_firmware_info);

			return len;
		}

		kfree(bit_firmware_info);
	}

	return -1;
}

static int vpu_release(struct inode *inode, struct file *filp)
{
	int ret = 0;

	ret = down_interruptible(&s_vpu_sem);
	if (ret == 0) {
		/* found and free the not handled buffer by user applications */
		vpu_free_buffers(filp);

		/* found and free the not closed instance by user applications */
		vpu_free_instances(filp);
		vcodec_dev.s_vpu_drv_context.open_count--;
		if (vcodec_dev.s_vpu_drv_context.open_count == 0) {
			if (vcodec_dev.s_instance_pool.base) {
				VCODEC_DBG_INFO("free instance pool\n");
#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
				vfree((const void *)vcodec_dev.s_instance_pool.base);
#else
				vpu_free_dma_buffer(&vcodec_dev.s_instance_pool);
#endif
				vcodec_dev.s_instance_pool.base = 0;
			}

			if (vcodec_dev.s_common_memory.base) {
				VCODEC_DBG_INFO("free common memory\n");
				vpu_free_dma_buffer(&vcodec_dev.s_common_memory);
				vcodec_dev.s_common_memory.base = 0;
			}
		}
	}
	up(&s_vpu_sem);

	return 0;
}

static int vpu_fasync(int fd, struct file *filp, int mode)
{
	struct vpu_drv_context_t *dev =
		(struct vpu_drv_context_t *)&vcodec_dev.s_vpu_drv_context;
	return fasync_helper(fd, filp, mode, &dev->async_queue);
}

static int vpu_map_to_register(struct file *filp, struct vm_area_struct *vm)
{
	#ifdef VPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_vpu_device *vdev = (struct cvi_vpu_device *)pCviVpuDevice;
	#else
	struct cvi_vpu_device *vdev = (struct cvi_vpu_device *)filp->private_data;
	#endif
	struct cvi_vcodec_context *pvctx =
		(struct cvi_vcodec_context *) vdev->pvctx;

	unsigned long pfn;

	vm->vm_flags |= VM_IO | VM_RESERVED;
	vm->vm_page_prot = pgprot_noncached(vm->vm_page_prot);
	pfn = pvctx->s_vpu_register.phys_addr >> PAGE_SHIFT;

	return remap_pfn_range(vm, vm->vm_start, pfn, vm->vm_end - vm->vm_start,
			       vm->vm_page_prot) ?
		       -EAGAIN :
		       0;
}

static int vpu_map_to_physical_memory(struct file *flip,
				      struct vm_area_struct *vm)
{
	vm->vm_flags |= VM_IO | VM_RESERVED;
	vm->vm_page_prot = pgprot_noncached(vm->vm_page_prot);

	return remap_pfn_range(vm, vm->vm_start, vm->vm_pgoff,
			       vm->vm_end - vm->vm_start, vm->vm_page_prot) ?
		       -EAGAIN :
		       0;
}

static int vpu_map_to_instance_pool_memory(struct file *filp,
					   struct vm_area_struct *vm)
{
#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
	int ret;
	long length = vm->vm_end - vm->vm_start;
	unsigned long start = vm->vm_start;
	char *vmalloc_area_ptr = (char *)vcodec_dev.s_instance_pool.base;
	unsigned long pfn;

	vm->vm_flags |= VM_RESERVED;

	/* loop over all pages, map it page individually */
	while (length > 0) {
		pfn = vmalloc_to_pfn(vmalloc_area_ptr);
		ret = remap_pfn_range(vm, start, pfn, PAGE_SIZE, PAGE_SHARED);
		if (ret < 0) {
			return ret;
		}
		start += PAGE_SIZE;
		vmalloc_area_ptr += PAGE_SIZE;
		length -= PAGE_SIZE;
	}

	return 0;
#else
	vm->vm_flags |= VM_RESERVED;
	return remap_pfn_range(vm, vm->vm_start, vm->vm_pgoff,
			       vm->vm_end - vm->vm_start, vm->vm_page_prot) ?
		       -EAGAIN :
		       0;
#endif
}

/*!
 * @brief memory map interface for vpu file operation
 * @return  0 on success or negative error code on error
 */
static int vpu_mmap(struct file *filp, struct vm_area_struct *vm)
{
#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
	#ifdef VPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_vpu_device *vdev = (struct cvi_vpu_device *)pCviVpuDevice;
	#else
	struct cvi_vpu_device *vdev = (struct cvi_vpu_device *)filp->private_data;
	#endif
	struct cvi_vcodec_context *pvctx =
		(struct cvi_vcodec_context *) vdev->pvctx;

	if (vm->vm_pgoff == 0)
		return vpu_map_to_instance_pool_memory(filp, vm);

	if (vm->vm_pgoff == (pvctx->s_vpu_register.phys_addr >> PAGE_SHIFT))
		return vpu_map_to_register(filp, vm);

	return vpu_map_to_physical_memory(filp, vm);
#else
	if (vm->vm_pgoff) {
		if (vm->vm_pgoff == (vcodec_dev.s_instance_pool.phys_addr >> PAGE_SHIFT))
			return vpu_map_to_instance_pool_memory(filp, vm);

		return vpu_map_to_physical_memory(filp, vm);
	} else {
		return vpu_map_to_register(filp, vm);
	}
#endif
}

static unsigned int vpu_poll(struct file *filp, struct poll_table_struct *wait)
{
	unsigned int mask = 0;
	int ret = 0;

	ret = down_interruptible(&s_vpu_sem);
	if (ret != 0) {
		up(&s_vpu_sem);

		VCODEC_DBG_WARN("down_interruptible\n");

		return mask;
	}

	poll_wait(filp, &hasBsWaitQ, wait);

	mask |= POLLIN | POLLRDNORM;

	VCODEC_DBG_WARN("mask = 0x%X\n", mask);

	up(&s_vpu_sem);

	return mask;
}

const struct file_operations vpu_fops = {
	.owner = THIS_MODULE,
	.open = vpu_open,
	.write = vpu_write,
	/*.ioctl = vpu_ioctl, // for kernel 2.6.9 of C&M*/
	.unlocked_ioctl = vpu_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = vpu_ioctl,
#endif
	.release = vpu_release,
	.fasync = vpu_fasync,
	.mmap = vpu_mmap,
	.poll = vpu_poll,
};

static void getCodecTypeStr(PAYLOAD_TYPE_E enType, char *pcCodecType)
{
	switch (enType) {
	case PT_JPEG:
		strcpy(pcCodecType, "JPEG");
		break;
	case PT_MJPEG:
		strcpy(pcCodecType, "MJPEG");
		break;
	case PT_H264:
		strcpy(pcCodecType, "H264");
		break;
	case PT_H265:
		strcpy(pcCodecType, "H265");
		break;
	default:
		strcpy(pcCodecType, "N/A");
		break;
	}
}

static void getRcModeStr(VENC_RC_MODE_E enRcMode, char *pRcMode)
{
	switch (enRcMode) {
	case VENC_RC_MODE_H264CBR:
	case VENC_RC_MODE_H265CBR:
	case VENC_RC_MODE_MJPEGCBR:
		strcpy(pRcMode, "CBR");
		break;
	case VENC_RC_MODE_H264VBR:
	case VENC_RC_MODE_H265VBR:
	case VENC_RC_MODE_MJPEGVBR:
		strcpy(pRcMode, "VBR");
		break;
	case VENC_RC_MODE_H264AVBR:
	case VENC_RC_MODE_H265AVBR:
		strcpy(pRcMode, "AVBR");
		break;
	case VENC_RC_MODE_H264QVBR:
	case VENC_RC_MODE_H265QVBR:
		strcpy(pRcMode, "QVBR");
		break;
	case VENC_RC_MODE_H264FIXQP:
	case VENC_RC_MODE_H265FIXQP:
	case VENC_RC_MODE_MJPEGFIXQP:
		strcpy(pRcMode, "FIXQP");
		break;
	case VENC_RC_MODE_H264QPMAP:
	case VENC_RC_MODE_H265QPMAP:
		strcpy(pRcMode, "QPMAP");
		break;
	default:
		strcpy(pRcMode, "N/A");
		break;
	}
}

static void getGopModeStr(VENC_GOP_MODE_E enGopMode, char *pcGopMode)
{
	switch (enGopMode) {
	case VENC_GOPMODE_NORMALP:
		strcpy(pcGopMode, "NORMALP");
		break;
	case VENC_GOPMODE_DUALP:
		strcpy(pcGopMode, "DUALP");
		break;
	case VENC_GOPMODE_SMARTP:
		strcpy(pcGopMode, "SMARTP");
		break;
	case VENC_GOPMODE_ADVSMARTP:
		strcpy(pcGopMode, "ADVSMARTP");
		break;
	case VENC_GOPMODE_BIPREDB:
		strcpy(pcGopMode, "BIPREDB");
		break;
	case VENC_GOPMODE_LOWDELAYB:
		strcpy(pcGopMode, "LOWDELAYB");
		break;
	case VENC_GOPMODE_BUTT:
		strcpy(pcGopMode, "BUTT");
		break;
	default:
		strcpy(pcGopMode, "N/A");
		break;
	}
}

static void getPixelFormatStr(PIXEL_FORMAT_E enPixelFormat, char *pcPixelFormat)
{
	switch (enPixelFormat) {
	case PIXEL_FORMAT_YUV_PLANAR_422:
		strcpy(pcPixelFormat, "YUV422");
		break;
	case PIXEL_FORMAT_YUV_PLANAR_420:
		strcpy(pcPixelFormat, "YUV420");
		break;
	case PIXEL_FORMAT_YUV_PLANAR_444:
		strcpy(pcPixelFormat, "YUV444");
		break;
	case PIXEL_FORMAT_NV12:
		strcpy(pcPixelFormat, "NV12");
		break;
	case PIXEL_FORMAT_NV21:
		strcpy(pcPixelFormat, "NV21");
		break;
	default:
		strcpy(pcPixelFormat, "N/A");
		break;
	}
}

static void getFrameRate(venc_proc_info_t *ptVencProcInfo,
	CVI_U32 *pu32SrcFrameRate, CVI_FR32 *pfr32DstFrameRate)
{
	switch (ptVencProcInfo->chnAttr.stRcAttr.enRcMode) {
	case VENC_RC_MODE_H264CBR:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264Cbr.u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264Cbr.fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H265CBR:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265Cbr.u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265Cbr.fr32DstFrameRate;
		break;
	case VENC_RC_MODE_MJPEGCBR:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stMjpegCbr.u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stMjpegCbr.fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H264VBR:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264Vbr.u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264Vbr.fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H265VBR:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265Vbr.u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265Vbr.fr32DstFrameRate;
		break;
	case VENC_RC_MODE_MJPEGVBR:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stMjpegVbr.u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stMjpegVbr.fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H264FIXQP:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264FixQp.u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264FixQp.fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H265FIXQP:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265FixQp.u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265FixQp.fr32DstFrameRate;
		break;
	case VENC_RC_MODE_MJPEGFIXQP:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stMjpegFixQp.u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stMjpegFixQp.fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H264AVBR:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264AVbr.u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264AVbr.fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H265AVBR:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265AVbr.u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265AVbr.fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H264QVBR:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264QVbr.u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264QVbr.fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H265QVBR:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265QVbr.u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265QVbr.fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H264QPMAP:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264QpMap.u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264QpMap.fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H265QPMAP:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265QpMap.u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265QpMap.fr32DstFrameRate;
		break;
	default:
		break;
	}
}

static int venc_proc_show(struct seq_file *m, void *v)
{
	int idx = 0;
	venc_proc_info_t *pProcInfoShareMem = (venc_proc_info_t *)pVencShm;
	proc_debug_config_t *pVencDebugConfig = (proc_debug_config_t *)pVencDbgShm;
	CVI_VENC_PARAM_MOD_S *pVencModParam = (CVI_VENC_PARAM_MOD_S *)(pVencDbgShm + sizeof(proc_debug_config_t));

	seq_printf(m, "Module: [VENC] System Build Time [%s]\n", UTS_VERSION);
	seq_puts(m, "-----MODULE PARAM---------------------------------------------\n");
	seq_printf(m, "VencBufferCache: %u\t FrameBufRecycle: %d\t VencMaxChnNum: %u\n",
		pVencModParam->stVencModParam.u32VencBufferCache,
		pVencModParam->stVencModParam.u32FrameBufRecycle,
		MAX_VENC_CHN_NUM);
#ifdef CONFIG_ARCH_CV182X
	seq_printf(m, "H264/H265 share singleESBuf %d, bSingleCore %d\n",
		pVencModParam->stH264eModParam.bSingleEsBuf && pVencModParam->stH265eModParam.bSingleEsBuf &&
		pVencModParam->stH264eModParam.u32SingleEsBufSize == pVencModParam->stH265eModParam.u32SingleEsBufSize,
		bSingleCore);
#endif

	for (idx = 0; idx < MAX_VENC_CHN_NUM; idx++) {
		int roiIdx = 0;
		venc_proc_info_t *ptVencProcInfo = (pProcInfoShareMem + idx);

		if (ptVencProcInfo->u8ChnUsed == 1) {
			char cCodecType[6] = {'\0'};
			char cRcMode[6] = {'\0'};
			char cGopMode[10] = {'\0'};
			char cPixelFormat[8] = {'\0'};
			CVI_U32 u32SrcFrameRate = 0;
			CVI_FR32 fr32DstFrameRate = 0;

			getCodecTypeStr(ptVencProcInfo->chnAttr.stVencAttr.enType, cCodecType);
			getRcModeStr(ptVencProcInfo->chnAttr.stRcAttr.enRcMode, cRcMode);
			getGopModeStr(ptVencProcInfo->chnAttr.stGopAttr.enGopMode, cGopMode);
			getPixelFormatStr(ptVencProcInfo->stFrame.enPixelFormat, cPixelFormat);
			getFrameRate(ptVencProcInfo, &u32SrcFrameRate, &fr32DstFrameRate);

			seq_puts(m, "-----VENC CHN ATTR 1---------------------------------------------\n");
			seq_printf(m, "ID: %d\t Width: %u\t Height: %u\t Type: %s\t RcMode: %s",
				idx,
				ptVencProcInfo->chnAttr.stVencAttr.u32PicWidth,
				ptVencProcInfo->chnAttr.stVencAttr.u32PicHeight,
				cCodecType,
				cRcMode);
			seq_printf(m, "\t EsBufQueueEn: %d\t bIsoSendFrmEn: %d",
				ptVencProcInfo->chnAttr.stVencAttr.bEsBufQueueEn,
				ptVencProcInfo->chnAttr.stVencAttr.bIsoSendFrmEn);
			seq_printf(m, "\t ByFrame: %s\t Sequence: %u\t LeftBytes: %u\t LeftFrm: %u",
				ptVencProcInfo->chnAttr.stVencAttr.bByFrame ? "Y" : "N",
				ptVencProcInfo->stStream.u32Seq,
				ptVencProcInfo->chnStatus.u32LeftStreamBytes,
				ptVencProcInfo->chnStatus.u32LeftStreamFrames);
			seq_printf(m, "\t CurPacks: %u\t GopMode: %s\t Prio: %d\n",
				ptVencProcInfo->chnStatus.u32CurPacks,
				cGopMode,
				ptVencProcInfo->stChnParam.u32Priority);

			seq_puts(m, "-----VENC CHN ATTR 2-----------------------------------------------\n");
			seq_printf(m, "VeStr: Y\t SrcFr: %u\t TarFr: %u\t Timeref: %u\t PixFmt: %s",
				u32SrcFrameRate,
				fr32DstFrameRate,
				ptVencProcInfo->stFrame.u32TimeRef,
				cPixelFormat);
			seq_printf(m, "\t PicAddr: 0x%llx\t WakeUpFrmCnt: %u\n",
				ptVencProcInfo->stFrame.u64PhyAddr[0],
				ptVencProcInfo->stChnParam.u32PollWakeUpFrmCnt);

			// TODO: following info should be amended later
			#if 0
			seq_puts(m, "-----VENC JPEGE ATTR ----------------------------------------------\n");
			seq_printf(m, "ID: %d\t RcvMode: %d\t MpfCnt: %d\t Mpf0Width: %d\t Mpf0Height: %d",
				idx, 0, 0, 0, 0);
			seq_printf(m, "\t Mpf1Width: %d\t Mpf1Height: %d\n", 0, 0);

			seq_puts(m, "-----VENC CHN RECEIVE STAT-----------------------------------------\n");
			seq_printf(m, "ID: %d\t Start: %d\t StartEx: %d\t RecvLeft: %d\t EncLeft: %d",
				idx, 0, 0, 0, 0);
			seq_puts(m, "\t JpegEncodeMode: NA\n");

			seq_puts(m, "-----VENC VPSS QUERY----------------------------------------------\n");
			seq_printf(m, "ID: %d\t Query: %d\t QueryOk: %d\t QueryFR: %d\t Invld: %d",
				idx, 0, 0, 0, 0);
			seq_printf(m, "\t Full: %d\t VbFail: %d\t QueryFail: %d\t InfoErr: %d\t Stop: %d\n",
				0, 0, 0, 0, 0);

			seq_puts(m, "-----VENC SEND1---------------------------------------------------\n");
			seq_printf(m, "ID: %d\t VpssSnd: %d\t VInfErr: %d\t OthrSnd: %d\t OInfErr: %d\t Send: %d",
				idx, 0, 0, 0, 0, 0);
			seq_printf(m, "\t Stop: %d\t Full: %d\t CropErr: %d\t DrectSnd: %d\t SizeErr: %d\n",
				0, 0, 0, 0, 0);

			seq_puts(m, "-----VENC SEND2--------------------------------------------------\n");
			seq_printf(m, "ID: %d\t SendVgs: %d\t StartOk: %d\t StartFail: %d\t IntOk: %d",
				idx, 0, 0, 0, 0);
			seq_printf(m, "\t IntFail: %d\t SrcAdd: %d\t SrcSub: %d\t DestAdd: %d\t DestSub: %d\n",
				0, 0, 0, 0, 0);

			seq_puts(m, "-----VENC PIC QUEUE STATE-----------------------------------------\n");
			seq_printf(m, "ID: %d\t Free: %d\t Busy: %d\t Vgs: %d\t BFrame: %d\n",
				idx, 0, 0, 0, 0);

			seq_puts(m, "-----VENC DCF/MPF QUEUE STATE-----------------------------------------\n");
			seq_printf(m, "ID: %d\t ThumbFree: %d\t ThumbBusy: %d",
				idx, 0, 0);
			seq_printf(m, "\t Mpf0Free: %d\t Mpf0Busy: %d\t Mpf1Free: %d\t Mpf1Busy: %d\n",
				0, 0, 0, 0);

			seq_puts(m, "-----VENC CHNL INFO------------------------------------------------\n");
			seq_printf(m, "ID: %d\t Inq: %d\t InqOk: %d\t Start: %d\t StartOk: %d\t Config: %d",
				idx, 0, 0, 0, 0, 0);
			seq_printf(m, "\t VencInt: %d\t ChaResLost: %d\t OverLoad: %d\t RingSkip: %d\t RcSkip: %d\n",
				0, 0, 0, 0, 0);
			#endif

			seq_puts(m, "-----VENC CROP INFO------------------------------------------------\n");
			seq_printf(m, "ID: %d\t CropEn: %s\t StartX: %d\t StartY: %d\t Width: %u\t Height: %u\n",
				idx,
				ptVencProcInfo->stChnParam.stCropCfg.bEnable ? "Y" : "N",
				ptVencProcInfo->stChnParam.stCropCfg.stRect.s32X,
				ptVencProcInfo->stChnParam.stCropCfg.stRect.s32Y,
				ptVencProcInfo->stChnParam.stCropCfg.stRect.u32Width,
				ptVencProcInfo->stChnParam.stCropCfg.stRect.u32Height);


			seq_puts(m, "-----ROI INFO-----------------------------------------------------\n");
			for (roiIdx = 0; roiIdx < 8; roiIdx++) {
				if (ptVencProcInfo->stRoiAttr[roiIdx].bEnable) {
					seq_printf(m, "ID: %d\t Index: %u\t bRoiEn: %s\t bAbsQp: %s\t Qp: %d",
						idx,
						ptVencProcInfo->stRoiAttr[roiIdx].u32Index,
						ptVencProcInfo->stRoiAttr[roiIdx].bEnable ? "Y" : "N",
						ptVencProcInfo->stRoiAttr[roiIdx].bAbsQp ? "Y" : "N",
						ptVencProcInfo->stRoiAttr[roiIdx].s32Qp);
					seq_printf(m, "\t Width: %u\t Height: %u\t StartX: %d\t StartY: %d\n",
						ptVencProcInfo->stRoiAttr[roiIdx].stRect.u32Width,
						ptVencProcInfo->stRoiAttr[roiIdx].stRect.u32Height,
						ptVencProcInfo->stRoiAttr[roiIdx].stRect.s32X,
						ptVencProcInfo->stRoiAttr[roiIdx].stRect.s32Y);
				}
			}

			#if 0
			seq_puts(m, "-----VENC STREAM STATE---------------------------------------------\n");
			seq_printf(m, "ID: %d\t FreeCnt: %d\t BusyCnt: %d\t UserCnt: %d\t UserGet: %d",
				idx, 0, 0, 0, 0);
			seq_printf(m, "\t UserRls: %d\t GetTimes: %d\t Interval: %d\t FrameRate: %d\n",
				0, 0, 0, 0);
			#endif

			seq_puts(m, "-----VENC PTS STATE------------------------------------------------\n");
			seq_printf(m, "ID: %d\t RcvFirstFrmPts: %llu\t RcvFrmPts: %llu\n",
				idx, 0LL, ptVencProcInfo->stFrame.u64PTS);

			seq_puts(m, "-----VENC CHN PERFORMANCE------------------------------------------------\n");
			seq_printf(m, "ID: %d\t No.SendFramePerSec: %u\t No.EncFramePerSec: %u\t HwEncTime: %llu us\n\n",
				idx,
				ptVencProcInfo->stFPS.u32InFPS, ptVencProcInfo->stFPS.u32OutFPS,
				ptVencProcInfo->stFPS.u64HwTime);
		}
	}

	seq_puts(m, "\n----- CVITEK Debug Level STATE ----------------------------------------\n");
	seq_printf(m, "VencDebugMask: 0x%X\t VencStartFrmIdx: %u\t VencEndFrmIdx: %u\t VencDumpPath: %s\t ",
		pVencDebugConfig->u32DbgMask, pVencDebugConfig->u32StartFrmIdx,
		pVencDebugConfig->u32EndFrmIdx, pVencDebugConfig->cDumpPath);
	seq_printf(m, "VencNoDataTimeout: %u\n", pVencDebugConfig->u32NoDataTimeout);

	return 0;
}

static int venc_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, venc_proc_show, PDE_DATA(inode));
}

static ssize_t vcodec_proc_write_helper(
		const char __user *user_buf, size_t count,
		proc_debug_config_t *ptDebugConfig, char *pcProcInputdata,
		char *pcDbgPrefix, char *pcDbgStartFrmPrefix,
		char *pcDbgEndFrmPrefix, char *pcDbgDirPrefix, char *pcNoDataTimeoutPrefix)
{
	uint8_t u8DgbPrefixLen;
	uint8_t u8DgbStartFrmPrefixLen;
	uint8_t u8DgbEndFrmPrefixLen;
	uint8_t u8DgbDirPrefixLen;
	uint8_t u8NoDataTimeoutPrefixLen;

	if (!user_buf) {
		VCODEC_DBG_ERR("no user buf input\n");
		return -EFAULT;
	}

	u8DgbPrefixLen = strlen(pcDbgPrefix);
	u8DgbStartFrmPrefixLen = strlen(pcDbgStartFrmPrefix);
	u8DgbEndFrmPrefixLen = strlen(pcDbgEndFrmPrefix);
	u8DgbDirPrefixLen = strlen(pcDbgDirPrefix);
	if (pcNoDataTimeoutPrefix)
		u8NoDataTimeoutPrefixLen = strlen(pcNoDataTimeoutPrefix);

	if (copy_from_user(pcProcInputdata, user_buf, count)) {
		VCODEC_DBG_ERR("copy_from_user fail\n");
		return -EFAULT;
	}

	if (strncmp(pcProcInputdata, pcDbgPrefix, u8DgbPrefixLen) == 0) {
		if (kstrtouint(pcProcInputdata + u8DgbPrefixLen, 16, &ptDebugConfig->u32DbgMask) != 0) {
			VCODEC_DBG_ERR("fail to set debug level: 0x%s\n",
					pcProcInputdata + u8DgbPrefixLen);
			return -EFAULT;
		}
	} else if (strncmp(pcProcInputdata, pcDbgStartFrmPrefix, u8DgbStartFrmPrefixLen) == 0) {
		if (kstrtouint(pcProcInputdata + u8DgbStartFrmPrefixLen, 10, &ptDebugConfig->u32StartFrmIdx) != 0) {
			VCODEC_DBG_ERR("fail to set start frame index: 0x%s\n",
					pcProcInputdata + u8DgbStartFrmPrefixLen);
			return -EFAULT;
		}
	} else if (strncmp(pcProcInputdata, pcDbgEndFrmPrefix, u8DgbEndFrmPrefixLen) == 0) {
		if (kstrtouint(pcProcInputdata + u8DgbEndFrmPrefixLen, 10, &ptDebugConfig->u32EndFrmIdx) != 0) {
			VCODEC_DBG_ERR("fail to set end frame index: 0x%s\n",
					pcProcInputdata + u8DgbEndFrmPrefixLen);
			return -EFAULT;
		}
	} else if (strncmp(pcProcInputdata, pcDbgDirPrefix, u8DgbDirPrefixLen) == 0) {
		if (strcpy(ptDebugConfig->cDumpPath, pcProcInputdata + u8DgbDirPrefixLen) == NULL) {
			VCODEC_DBG_ERR("fail to set debug folder: 0x%s\n",
					pcProcInputdata + u8DgbDirPrefixLen);
			return -EFAULT;
		}
	} else if (pcNoDataTimeoutPrefix &&
				strncmp(pcProcInputdata, pcNoDataTimeoutPrefix, u8NoDataTimeoutPrefixLen) == 0) {
		if (kstrtouint(pcProcInputdata + u8NoDataTimeoutPrefixLen, 10, &ptDebugConfig->u32NoDataTimeout) != 0) {
			VCODEC_DBG_ERR("fail to set no data threshold: 0x%s\n",
					pcProcInputdata + u8NoDataTimeoutPrefixLen);
			return -EFAULT;
		}
	} else {
		VCODEC_DBG_ERR("can't handle user input %s\n", pcProcInputdata);
		return -EFAULT;
	}

	return count;
}

static ssize_t venc_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
    /* debug_level list, please refer to: cvi_comm_venc.h
     * CVI_VENC_MASK_ERR	0x1
     * CVI_VENC_MASK_WARN	0x2
     * CVI_VENC_MASK_INFO	0x4
     * CVI_VENC_MASK_FLOW	0x8
     * CVI_VENC_MASK_DBG	0x10
     * CVI_VENC_MASK_BS		0x100
     * CVI_VENC_MASK_SRC	0x200
     * CVI_VENC_MASK_API	0x400
     * CVI_VENC_MASK_SYNC	0x800
     * CVI_VENC_MASK_PERF	0x1000
     * CVI_VENC_MASK_CFG	0x2000
     * CVI_VENC_MASK_FRC	0x4000
     * CVI_VENC_MASK_BIND	0x8000
     * CVI_VENC_MASK_TRACE	0x10000
     * CVI_VENC_MASK_DUMP_YUV	0x100000
     * CVI_VENC_MASK_DUMP_BS	0x200000
     */
	char cProcInputdata[MAX_PROC_STR_SIZE] = {'\0'};
	char cVencDbgPrefix[] = "venc=0x";	// venc=debug_levle
	char cVencDbgStartFrmPrefix[] = "venc_sfn=";	// venc_sfn=frame_idx_begin
	char cVencDbgEndFrmPrefix[] = "venc_efn=";	// venc_efn=frame_idx_end
	char cVencDbgDirPrefix[] = "venc_dir=";	// venc_dir=your_preference_directory
	char cVencDbgNoDataTimeoutPrefix[] = "venc_no_inputdata_timeout=";	// venc_no_inputdata_timeout=timeout
	proc_debug_config_t *pVencDebugConfig = (proc_debug_config_t *)pVencDbgShm;

	return vcodec_proc_write_helper(user_buf, count, pVencDebugConfig,
		cProcInputdata, cVencDbgPrefix, cVencDbgStartFrmPrefix, cVencDbgEndFrmPrefix, cVencDbgDirPrefix,
		cVencDbgNoDataTimeoutPrefix);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops venc_proc_fops = {
	.proc_open = venc_proc_open,
	.proc_read = seq_read,
	.proc_write = venc_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations venc_proc_fops = {
	.owner = THIS_MODULE,
	.open = venc_proc_open,
	.read = seq_read,
	.write = venc_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static int h265e_proc_show(struct seq_file *m, void *v)
{
	int idx = 0;
	venc_proc_info_t *pProcInfoShareMem = (venc_proc_info_t *)pVencShm;
	CVI_VENC_PARAM_MOD_S *pVencModParam = (CVI_VENC_PARAM_MOD_S *)(pVencDbgShm + sizeof(proc_debug_config_t));

	seq_printf(m, "Module: [H265E] System Build Time [%s]\n", UTS_VERSION);

	seq_puts(m, "-----MODULE PARAM-------------------------------------------------\n");
	seq_printf(m, "OnePack: %u\t H265eVBSource: %d\t PowerSaveEn: %u",
			pVencModParam->stH265eModParam.u32OneStreamBuffer,
			pVencModParam->stH265eModParam.enH265eVBSource,
			pVencModParam->stH265eModParam.u32H265ePowerSaveEn);
	seq_printf(m, "\t MiniBufMode: %u\t bQpHstgrmEn: %u\t UserDataMaxLen: %u\n",
			pVencModParam->stH265eModParam.u32H265eMiniBufMode,
			pVencModParam->stH265eModParam.bQpHstgrmEn,
			pVencModParam->stH265eModParam.u32UserDataMaxLen);
	seq_printf(m, "SingleEsBuf: %u\t SingleEsBufSize: %u\t RefreshType: %u\n",
			pVencModParam->stH265eModParam.bSingleEsBuf,
			pVencModParam->stH265eModParam.u32SingleEsBufSize,
			pVencModParam->stH265eModParam.enRefreshType);

	for (idx = 0; idx < MAX_VENC_CHN_NUM; idx++) {
		venc_proc_info_t *ptVencProcInfo = (pProcInfoShareMem + idx);

		if (ptVencProcInfo->u8ChnUsed == 1 &&
			ptVencProcInfo->chnAttr.stVencAttr.enType == PT_H265
			/* ptVencProcInfo->modParam.enVencModType == MODTYPE_H265E */) {
			char cGopMode[10] = {'\0'};

			getGopModeStr(ptVencProcInfo->chnAttr.stGopAttr.enGopMode, cGopMode);

			seq_puts(m, "-----CHN ATTR-----------------------------------------------------\n");
			seq_printf(m, "ID: %d\t MaxWidth: %u\t MaxHeight: %u\t Width: %u\t Height: %u",
				idx,
				ptVencProcInfo->chnAttr.stVencAttr.u32MaxPicWidth,
				ptVencProcInfo->chnAttr.stVencAttr.u32MaxPicHeight,
				ptVencProcInfo->chnAttr.stVencAttr.u32PicWidth,
				ptVencProcInfo->chnAttr.stVencAttr.u32PicHeight);
			seq_printf(m, "\t C2GEn: %d\t BufSize: %u\t ByFrame: %d\t GopMode: %s\t MaxStrCnt: %u\n",
				ptVencProcInfo->stChnParam.bColor2Grey,
				ptVencProcInfo->chnAttr.stVencAttr.u32BufSize,
				ptVencProcInfo->chnAttr.stVencAttr.bByFrame,
				cGopMode,
				ptVencProcInfo->stChnParam.u32MaxStrmCnt);

			seq_puts(m, "-----RefParam INFO---------------------------------------------\n");
			seq_printf(m, "ID: %d\t EnPred: %s\t Base: %u\t Enhance: %u\t RcnRefShareBuf: %u\n",
				idx,
				ptVencProcInfo->refParam.bEnablePred ? "Y" : "N",
				ptVencProcInfo->refParam.u32Base,
				ptVencProcInfo->refParam.u32Enhance,
				ptVencProcInfo->chnAttr.stVencAttr.stAttrH265e.bRcnRefShareBuf);

			seq_puts(m, "-----Syntax INFO---------------------------------------------\n");
			seq_printf(m, "ID: %d\t Profile: Main\n",
				idx);
		}
	}
	return 0;
}

static int h265e_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, h265e_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops h265e_proc_fops = {
	.proc_open = h265e_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations h265e_proc_fops = {
	.owner = THIS_MODULE,
	.open = h265e_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static int h264e_proc_show(struct seq_file *m, void *v)
{
	static const char * const prifle[] = { "Base", "Main", "High", "Svc-t", "Err" };
	int idx = 0;
	venc_proc_info_t *pProcInfoShareMem = (venc_proc_info_t *)pVencShm;
	CVI_VENC_PARAM_MOD_S *pVencModParam = (CVI_VENC_PARAM_MOD_S *)(pVencDbgShm + sizeof(proc_debug_config_t));
	CVI_U32 u32Profile;

	seq_printf(m, "Module: [H264E] System Build Time [%s]\n", UTS_VERSION);

	seq_puts(m, "-----MODULE PARAM-------------------------------------------------\n");
	seq_printf(m, "OnePack: %u\t H264eVBSource: %d\t PowerSaveEn: %u",
			pVencModParam->stH264eModParam.u32OneStreamBuffer,
			pVencModParam->stH264eModParam.enH264eVBSource,
			pVencModParam->stH264eModParam.u32H264ePowerSaveEn);
	seq_printf(m, "\t MiniBufMode: %u\t QpHstgrmEn: %u\t UserDataMaxLen: %u\n",
			pVencModParam->stH264eModParam.u32H264eMiniBufMode,
			pVencModParam->stH264eModParam.bQpHstgrmEn,
			pVencModParam->stH264eModParam.u32UserDataMaxLen);
	seq_printf(m, "SingleEsBuf: %u\t SingleEsBufSize: %u\n",
			pVencModParam->stH264eModParam.bSingleEsBuf,
			pVencModParam->stH264eModParam.u32SingleEsBufSize);

	for (idx = 0; idx < MAX_VENC_CHN_NUM; idx++) {
		venc_proc_info_t *ptVencProcInfo = (pProcInfoShareMem + idx);

		if (ptVencProcInfo->u8ChnUsed == 1 &&
			ptVencProcInfo->chnAttr.stVencAttr.enType == PT_H264
			/* ptVencProcInfo->modParam.enVencModType == MODTYPE_H264E */) {
			char cGopMode[10] = {'\0'};

			getGopModeStr(ptVencProcInfo->chnAttr.stGopAttr.enGopMode, cGopMode);

			seq_puts(m, "-----CHN ATTR-----------------------------------------------------\n");
			seq_printf(m, "ID: %d\t MaxWidth: %u\t MaxHeight: %u\t Width: %u\t Height: %u",
				idx,
				ptVencProcInfo->chnAttr.stVencAttr.u32MaxPicWidth,
				ptVencProcInfo->chnAttr.stVencAttr.u32MaxPicHeight,
				ptVencProcInfo->chnAttr.stVencAttr.u32PicWidth,
				ptVencProcInfo->chnAttr.stVencAttr.u32PicHeight);
			seq_printf(m, "\t C2GEn: %d\t BufSize: %u\t ByFrame: %d\t GopMode: %s\t MaxStrCnt: %u\n",
				ptVencProcInfo->stChnParam.bColor2Grey,
				ptVencProcInfo->chnAttr.stVencAttr.u32BufSize,
				ptVencProcInfo->chnAttr.stVencAttr.bByFrame,
				cGopMode,
				ptVencProcInfo->stChnParam.u32MaxStrmCnt);

			seq_puts(m, "-----RefParam INFO---------------------------------------------\n");
			seq_printf(m, "ID: %d\t EnPred: %s\t Base: %u\t Enhance: %u\t RcnRefShareBuf: %u\n",
				idx,
				ptVencProcInfo->refParam.bEnablePred ? "Y" : "N",
				ptVencProcInfo->refParam.u32Base,
				ptVencProcInfo->refParam.u32Enhance,
				ptVencProcInfo->chnAttr.stVencAttr.stAttrH264e.bRcnRefShareBuf);

			seq_puts(m, "-----Syntax INFO---------------------------------------------\n");
			u32Profile = ptVencProcInfo->chnAttr.stVencAttr.u32Profile;
			if (u32Profile >= 4)
				u32Profile = 4;

			seq_printf(m, "ID: %d\t Profile: %s\n",
				idx,
				prifle[u32Profile]);
		}
	}
	return 0;
}

static int h264e_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, h264e_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops h264e_proc_fops = {
	.proc_open = h264e_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations h264e_proc_fops = {
	.owner = THIS_MODULE,
	.open = h264e_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static int jpege_proc_show(struct seq_file *m, void *v)
{
	int idx = 0;
	venc_proc_info_t *pProcInfoShareMem = (venc_proc_info_t *)pVencShm;
	CVI_VENC_PARAM_MOD_S *pVencModParam = (CVI_VENC_PARAM_MOD_S *)(pVencDbgShm + sizeof(proc_debug_config_t));

	seq_printf(m, "Module: [JPEGE] System Build Time [%s]\n", UTS_VERSION);

	seq_puts(m, "-----MODULE PARAM-------------------------------------------------\n");
	seq_printf(m, "OnePack: %u\t JpegeMiniBufMode: %d",
			pVencModParam->stJpegeModParam.u32OneStreamBuffer,
			pVencModParam->stJpegeModParam.u32JpegeMiniBufMode);
	seq_printf(m, "\t JpegClearStreamBuf: %u\t JpegeDeringMode: %u\n",
			pVencModParam->stJpegeModParam.u32JpegClearStreamBuf, 0);
	seq_printf(m, "SingleEsBuf: %u\t SingleEsBufSize: %u\t JpegeFormat: %u\n",
			pVencModParam->stJpegeModParam.bSingleEsBuf,
			pVencModParam->stJpegeModParam.u32SingleEsBufSize,
			pVencModParam->stJpegeModParam.enJpegeFormat);
	seq_printf(m, "JpegMarkerOrder: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
			pVencModParam->stJpegeModParam.JpegMarkerOrder[0],
			pVencModParam->stJpegeModParam.JpegMarkerOrder[1],
			pVencModParam->stJpegeModParam.JpegMarkerOrder[2],
			pVencModParam->stJpegeModParam.JpegMarkerOrder[3],
			pVencModParam->stJpegeModParam.JpegMarkerOrder[4],
			pVencModParam->stJpegeModParam.JpegMarkerOrder[5],
			pVencModParam->stJpegeModParam.JpegMarkerOrder[6],
			pVencModParam->stJpegeModParam.JpegMarkerOrder[7],
			pVencModParam->stJpegeModParam.JpegMarkerOrder[8],
			pVencModParam->stJpegeModParam.JpegMarkerOrder[9],
			pVencModParam->stJpegeModParam.JpegMarkerOrder[10],
			pVencModParam->stJpegeModParam.JpegMarkerOrder[11],
			pVencModParam->stJpegeModParam.JpegMarkerOrder[12],
			pVencModParam->stJpegeModParam.JpegMarkerOrder[13],
			pVencModParam->stJpegeModParam.JpegMarkerOrder[14],
			pVencModParam->stJpegeModParam.JpegMarkerOrder[15]);

	for (idx = 0; idx < MAX_VENC_CHN_NUM; idx++) {
		venc_proc_info_t *ptVencProcInfo = (pProcInfoShareMem + idx);

		if (ptVencProcInfo->u8ChnUsed == 1 &&
			(ptVencProcInfo->chnAttr.stVencAttr.enType == PT_JPEG ||
			 ptVencProcInfo->chnAttr.stVencAttr.enType == PT_MJPEG)
			/* ptVencProcInfo->modParam.enVencModType == MODTYPE_JPEGE */) {
			char cGopMode[10] = {'\0'};
			char cPicType[8] = {'\0'};
			CVI_U32 u32Qfactor = 0;

			getGopModeStr(ptVencProcInfo->chnAttr.stGopAttr.enGopMode, cGopMode);
			getPixelFormatStr(ptVencProcInfo->stFrame.enPixelFormat, cPicType);

			if (ptVencProcInfo->chnAttr.stVencAttr.enType == PT_JPEG) {
				u32Qfactor = ptVencProcInfo->stJpegParam.u32Qfactor;
			} else if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_MJPEGFIXQP) {
				u32Qfactor = ptVencProcInfo->chnAttr.stRcAttr.stMjpegFixQp.u32Qfactor;
			}

			seq_puts(m, "-----CHN ATTR-----------------------------------------------------\n");
			seq_printf(m, "ID: %d\t bMjpeg: %s\t PicType: %s\t MaxWidth: %u\t MaxHeight: %u",
				idx,
				ptVencProcInfo->chnAttr.stVencAttr.enType == PT_MJPEG ? "Y" : "N",
				cPicType,
				ptVencProcInfo->chnAttr.stVencAttr.u32MaxPicWidth,
				ptVencProcInfo->chnAttr.stVencAttr.u32MaxPicHeight);
			seq_printf(m, "\t Width: %u\t Height: %u\t BufSize: %u\t ByFrm: %d",
				ptVencProcInfo->chnAttr.stVencAttr.u32PicWidth,
				ptVencProcInfo->chnAttr.stVencAttr.u32PicHeight,
				ptVencProcInfo->chnAttr.stVencAttr.u32BufSize,
				ptVencProcInfo->chnAttr.stVencAttr.bByFrame);
			seq_printf(m, "\t MCU: %d\t Qfactor: %u\t C2GEn: %d\t DcfEn: %d\n",
				1,
				u32Qfactor,
				ptVencProcInfo->stChnParam.bColor2Grey,
				0);
		}
	}
	return 0;
}

static int jpege_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, jpege_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops jpege_proc_fops = {
	.proc_open = venc_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations jpege_proc_fops = {
	.owner = THIS_MODULE,
	.open = jpege_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static int rc_proc_show(struct seq_file *m, void *v)
{
	int idx = 0;
	venc_proc_info_t *pProcInfoShareMem = (venc_proc_info_t *)pVencShm;

	seq_printf(m, "Module: [RC] System Build Time [%s]\n", UTS_VERSION);

	for (idx = 0; idx < MAX_VENC_CHN_NUM; idx++) {
		venc_proc_info_t *ptVencProcInfo = (pProcInfoShareMem + idx);

		if (ptVencProcInfo->u8ChnUsed == 1) {
			char cCodecType[16] = {'\0'};
			char cRcMode[8] = {'\0'};
			char cQpMapMode[8] = {'\0'};
			char cGopMode[10] = {'\0'};
			CVI_U32 u32Gop = 0;
			CVI_U32 u32StatTime = 0;
			CVI_U32 u32SrcFrameRate = 0;
			CVI_FR32 fr32DstFrameRate = 0;
			CVI_U32 u32BitRate = 0;
			CVI_U32 u32IQp = 0;
			CVI_U32 u32PQp = 0;
			CVI_U32 u32BQp = 0;
			CVI_U32 u32Qfactor = 0;
			CVI_U32 u32MinIprop = 0;
			CVI_U32 u32MaxIprop = 0;
			CVI_U32 u32MaxQp = 0;
			CVI_U32 u32MinQp = 0;
			CVI_U32 u32MaxIQp = 0;
			CVI_U32 u32MinIQp = 0;
			CVI_BOOL bQpMapEn = CVI_FALSE;
			CVI_BOOL bVariFpsEn = CVI_FALSE;
			CVI_S32 s32IPQpDelta = 0;
			CVI_U32 u32SPInterval = 0;
			CVI_S32 s32SPQpDelta = 0;
			CVI_U32 u32BgInterval = 0;
			CVI_S32 s32BgQpDelta = 0;
			CVI_S32 s32ViQpDelta = 0;
			CVI_U32 u32BFrmNum = 0;
			CVI_S32 s32BQpDelta = 0;
			CVI_S32 s32MaxReEncodeTimes = 0;
			CVI_S32 s32ChangePos = 0;
			CVI_U32 u32MaxQfactor = 0;
			CVI_U32 u32MinQfactor = 0;
			VENC_RC_QPMAP_MODE_E enQpMapMode = VENC_RC_QPMAP_MODE_BUTT + 1;
			CVI_S32 s32MinStillPercent = 0;
			CVI_U32 u32MaxStillQP = 0;
			CVI_U32 u32MinStillPSNR = 0;
			CVI_U32 u32MinQpDelta = 0;
			CVI_U32 u32MotionSensitivity = 0;
			CVI_S32 s32AvbrFrmLostOpen = 0;
			CVI_S32 s32AvbrFrmGap = 0;
			CVI_S32 s32AvbrPureStillThr = 0;

			getCodecTypeStr(ptVencProcInfo->chnAttr.stVencAttr.enType, cCodecType);
			getGopModeStr(ptVencProcInfo->chnAttr.stGopAttr.enGopMode, cGopMode);
			getFrameRate(ptVencProcInfo, &u32SrcFrameRate, &fr32DstFrameRate);

			switch (ptVencProcInfo->chnAttr.stGopAttr.enGopMode) {
			case VENC_GOPMODE_NORMALP:
				s32IPQpDelta = ptVencProcInfo->chnAttr.stGopAttr.stNormalP.s32IPQpDelta;
				break;
			case VENC_GOPMODE_DUALP:
				u32SPInterval = ptVencProcInfo->chnAttr.stGopAttr.stDualP.u32SPInterval;
				s32SPQpDelta = ptVencProcInfo->chnAttr.stGopAttr.stDualP.s32SPQpDelta;
				s32IPQpDelta = ptVencProcInfo->chnAttr.stGopAttr.stDualP.s32IPQpDelta;
				break;
			case VENC_GOPMODE_SMARTP:
				u32BgInterval = ptVencProcInfo->chnAttr.stGopAttr.stSmartP.u32BgInterval;
				s32BgQpDelta = ptVencProcInfo->chnAttr.stGopAttr.stSmartP.s32BgQpDelta;
				s32ViQpDelta = ptVencProcInfo->chnAttr.stGopAttr.stSmartP.s32ViQpDelta;
				break;
			case VENC_GOPMODE_ADVSMARTP:
				u32BgInterval = ptVencProcInfo->chnAttr.stGopAttr.stAdvSmartP.u32BgInterval;
				s32BgQpDelta = ptVencProcInfo->chnAttr.stGopAttr.stAdvSmartP.s32BgQpDelta;
				s32ViQpDelta = ptVencProcInfo->chnAttr.stGopAttr.stAdvSmartP.s32ViQpDelta;
				break;
			case VENC_GOPMODE_BIPREDB:
				u32BFrmNum = ptVencProcInfo->chnAttr.stGopAttr.stBipredB.u32BFrmNum;
				s32BQpDelta = ptVencProcInfo->chnAttr.stGopAttr.stBipredB.s32BQpDelta;
				s32IPQpDelta = ptVencProcInfo->chnAttr.stGopAttr.stBipredB.s32IPQpDelta;
				break;
			default:
				break;
			}

			if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H264CBR) {
				strcpy(cRcMode, "CBR");
				u32Gop = ptVencProcInfo->chnAttr.stRcAttr.stH264Cbr.u32Gop;
				u32StatTime = ptVencProcInfo->chnAttr.stRcAttr.stH264Cbr.u32StatTime;
				u32BitRate = ptVencProcInfo->chnAttr.stRcAttr.stH264Cbr.u32BitRate;
				bVariFpsEn = ptVencProcInfo->chnAttr.stRcAttr.stH264Cbr.bVariFpsEn;

				u32MinIprop = ptVencProcInfo->rcParam.stParamH264Cbr.u32MinIprop;
				u32MaxIprop = ptVencProcInfo->rcParam.stParamH264Cbr.u32MaxIprop;
				u32MaxQp = ptVencProcInfo->rcParam.stParamH264Cbr.u32MaxQp;
				u32MinQp = ptVencProcInfo->rcParam.stParamH264Cbr.u32MinQp;
				u32MaxIQp = ptVencProcInfo->rcParam.stParamH264Cbr.u32MaxIQp;
				u32MinIQp = ptVencProcInfo->rcParam.stParamH264Cbr.u32MinIQp;
				s32MaxReEncodeTimes = ptVencProcInfo->rcParam.stParamH264Cbr.s32MaxReEncodeTimes;
				bQpMapEn = ptVencProcInfo->rcParam.stParamH264Cbr.bQpMapEn;
			} else if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H265CBR) {
				strcpy(cRcMode, "CBR");
				u32Gop = ptVencProcInfo->chnAttr.stRcAttr.stH265Cbr.u32Gop;
				u32StatTime = ptVencProcInfo->chnAttr.stRcAttr.stH265Cbr.u32StatTime;
				u32BitRate = ptVencProcInfo->chnAttr.stRcAttr.stH265Cbr.u32BitRate;
				bVariFpsEn = ptVencProcInfo->chnAttr.stRcAttr.stH265Cbr.bVariFpsEn;

				u32MinIprop = ptVencProcInfo->rcParam.stParamH265Cbr.u32MinIprop;
				u32MaxIprop = ptVencProcInfo->rcParam.stParamH265Cbr.u32MaxIprop;
				u32MaxQp = ptVencProcInfo->rcParam.stParamH265Cbr.u32MaxQp;
				u32MinQp = ptVencProcInfo->rcParam.stParamH265Cbr.u32MinQp;
				u32MaxIQp = ptVencProcInfo->rcParam.stParamH265Cbr.u32MaxIQp;
				u32MinIQp = ptVencProcInfo->rcParam.stParamH265Cbr.u32MinIQp;
				s32MaxReEncodeTimes = ptVencProcInfo->rcParam.stParamH265Cbr.s32MaxReEncodeTimes;
				bQpMapEn = ptVencProcInfo->rcParam.stParamH265Cbr.bQpMapEn;
				enQpMapMode = ptVencProcInfo->rcParam.stParamH265Cbr.enQpMapMode;
			} else if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_MJPEGCBR) {
				strcpy(cRcMode, "CBR");
				u32StatTime = ptVencProcInfo->chnAttr.stRcAttr.stMjpegCbr.u32StatTime;
				u32BitRate = ptVencProcInfo->chnAttr.stRcAttr.stMjpegCbr.u32BitRate;
				bVariFpsEn = ptVencProcInfo->chnAttr.stRcAttr.stMjpegCbr.bVariFpsEn;

				u32MaxQfactor = ptVencProcInfo->rcParam.stParamMjpegCbr.u32MaxQfactor;
				u32MinQfactor = ptVencProcInfo->rcParam.stParamMjpegCbr.u32MinQfactor;
			} else if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H264VBR) {
				strcpy(cRcMode, "VBR");
				u32Gop = ptVencProcInfo->chnAttr.stRcAttr.stH264Vbr.u32Gop;
				u32StatTime = ptVencProcInfo->chnAttr.stRcAttr.stH264Vbr.u32StatTime;
				u32BitRate = ptVencProcInfo->chnAttr.stRcAttr.stH264Vbr.u32MaxBitRate;
				bVariFpsEn = ptVencProcInfo->chnAttr.stRcAttr.stH264Vbr.bVariFpsEn;

				s32ChangePos = ptVencProcInfo->rcParam.stParamH264Vbr.s32ChangePos;
				u32MinIprop = ptVencProcInfo->rcParam.stParamH264Vbr.u32MinIprop;
				u32MaxIprop = ptVencProcInfo->rcParam.stParamH264Vbr.u32MaxIprop;
				u32MaxQp = ptVencProcInfo->rcParam.stParamH264Vbr.u32MaxQp;
				u32MinQp = ptVencProcInfo->rcParam.stParamH264Vbr.u32MinQp;
				u32MaxIQp = ptVencProcInfo->rcParam.stParamH264Vbr.u32MaxIQp;
				u32MinIQp = ptVencProcInfo->rcParam.stParamH264Vbr.u32MinIQp;
				bQpMapEn = ptVencProcInfo->rcParam.stParamH264Vbr.bQpMapEn;
				s32MaxReEncodeTimes = ptVencProcInfo->rcParam.stParamH264Vbr.s32MaxReEncodeTimes;
			} else if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H265VBR) {
				strcpy(cRcMode, "VBR");
				u32Gop = ptVencProcInfo->chnAttr.stRcAttr.stH265Vbr.u32Gop;
				u32StatTime = ptVencProcInfo->chnAttr.stRcAttr.stH265Vbr.u32StatTime;
				u32BitRate = ptVencProcInfo->chnAttr.stRcAttr.stH265Vbr.u32MaxBitRate;
				bVariFpsEn = ptVencProcInfo->chnAttr.stRcAttr.stH265Vbr.bVariFpsEn;

				s32ChangePos = ptVencProcInfo->rcParam.stParamH265Vbr.s32ChangePos;
				u32MinIprop = ptVencProcInfo->rcParam.stParamH265Vbr.u32MinIprop;
				u32MaxIprop = ptVencProcInfo->rcParam.stParamH265Vbr.u32MaxIprop;
				u32MaxQp = ptVencProcInfo->rcParam.stParamH265Vbr.u32MaxQp;
				u32MinQp = ptVencProcInfo->rcParam.stParamH265Vbr.u32MinQp;
				u32MaxIQp = ptVencProcInfo->rcParam.stParamH265Vbr.u32MaxIQp;
				u32MinIQp = ptVencProcInfo->rcParam.stParamH265Vbr.u32MinIQp;
				bQpMapEn = ptVencProcInfo->rcParam.stParamH265Vbr.bQpMapEn;
				s32MaxReEncodeTimes = ptVencProcInfo->rcParam.stParamH265Vbr.s32MaxReEncodeTimes;
			} else if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_MJPEGVBR) {
				strcpy(cRcMode, "VBR");
				u32StatTime = ptVencProcInfo->chnAttr.stRcAttr.stMjpegVbr.u32StatTime;
				u32BitRate = ptVencProcInfo->chnAttr.stRcAttr.stMjpegVbr.u32MaxBitRate;
				bVariFpsEn = ptVencProcInfo->chnAttr.stRcAttr.stMjpegVbr.bVariFpsEn;

				s32ChangePos = ptVencProcInfo->rcParam.stParamMjpegVbr.s32ChangePos;
				u32MaxQfactor = ptVencProcInfo->rcParam.stParamMjpegVbr.u32MaxQfactor;
				u32MinQfactor = ptVencProcInfo->rcParam.stParamMjpegVbr.u32MinQfactor;
			} else if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H264FIXQP) {
				strcpy(cRcMode, "FIXQP");
				u32Gop = ptVencProcInfo->chnAttr.stRcAttr.stH264FixQp.u32Gop;
				u32IQp = ptVencProcInfo->chnAttr.stRcAttr.stH264FixQp.u32IQp;
				u32PQp = ptVencProcInfo->chnAttr.stRcAttr.stH264FixQp.u32PQp;
				u32BQp = ptVencProcInfo->chnAttr.stRcAttr.stH264FixQp.u32BQp;
				bVariFpsEn = ptVencProcInfo->chnAttr.stRcAttr.stH264FixQp.bVariFpsEn;
			} else if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H265FIXQP) {
				strcpy(cRcMode, "FIXQP");
				u32Gop = ptVencProcInfo->chnAttr.stRcAttr.stH265FixQp.u32Gop;
				u32IQp = ptVencProcInfo->chnAttr.stRcAttr.stH265FixQp.u32IQp;
				u32PQp = ptVencProcInfo->chnAttr.stRcAttr.stH265FixQp.u32PQp;
				u32BQp = ptVencProcInfo->chnAttr.stRcAttr.stH265FixQp.u32BQp;
				bVariFpsEn = ptVencProcInfo->chnAttr.stRcAttr.stH265FixQp.bVariFpsEn;
			} else if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_MJPEGFIXQP) {
				strcpy(cRcMode, "FIXQP");
				u32Qfactor = ptVencProcInfo->chnAttr.stRcAttr.stMjpegFixQp.u32Qfactor;
				bVariFpsEn = ptVencProcInfo->chnAttr.stRcAttr.stMjpegFixQp.bVariFpsEn;
			} else if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H264AVBR) {
				strcpy(cRcMode, "AVBR");
				u32Gop = ptVencProcInfo->chnAttr.stRcAttr.stH264AVbr.u32Gop;
				u32StatTime = ptVencProcInfo->chnAttr.stRcAttr.stH264AVbr.u32StatTime;
				u32BitRate = ptVencProcInfo->chnAttr.stRcAttr.stH264AVbr.u32MaxBitRate;
				bVariFpsEn = ptVencProcInfo->chnAttr.stRcAttr.stH264AVbr.bVariFpsEn;

				s32ChangePos = ptVencProcInfo->rcParam.stParamH264AVbr.s32ChangePos;
				u32MinIprop = ptVencProcInfo->rcParam.stParamH264AVbr.u32MinIprop;
				u32MaxIprop = ptVencProcInfo->rcParam.stParamH264AVbr.u32MaxIprop;
				s32MinStillPercent = ptVencProcInfo->rcParam.stParamH264AVbr.s32MinStillPercent;
				u32MaxStillQP = ptVencProcInfo->rcParam.stParamH264AVbr.u32MaxStillQP;
				u32MinStillPSNR = ptVencProcInfo->rcParam.stParamH264AVbr.u32MinStillPSNR;
				u32MaxQp = ptVencProcInfo->rcParam.stParamH264AVbr.u32MaxQp;
				u32MinQp = ptVencProcInfo->rcParam.stParamH264AVbr.u32MinQp;
				u32MaxIQp = ptVencProcInfo->rcParam.stParamH264AVbr.u32MaxIQp;
				u32MinIQp = ptVencProcInfo->rcParam.stParamH264AVbr.u32MinIQp;
				u32MinQpDelta = ptVencProcInfo->rcParam.stParamH264AVbr.u32MinQpDelta;
				u32MotionSensitivity = ptVencProcInfo->rcParam.stParamH264AVbr.u32MotionSensitivity;
				s32AvbrFrmLostOpen = ptVencProcInfo->rcParam.stParamH264AVbr.s32AvbrFrmLostOpen;
				s32AvbrFrmGap = ptVencProcInfo->rcParam.stParamH264AVbr.s32AvbrFrmGap;
				u32MinStillPSNR = ptVencProcInfo->rcParam.stParamH264AVbr.s32AvbrPureStillThr;
				bQpMapEn = ptVencProcInfo->rcParam.stParamH264AVbr.bQpMapEn;
				s32MaxReEncodeTimes = ptVencProcInfo->rcParam.stParamH264AVbr.s32MaxReEncodeTimes;
			} else if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H265AVBR) {
				strcpy(cRcMode, "AVBR");
				u32Gop = ptVencProcInfo->chnAttr.stRcAttr.stH265AVbr.u32Gop;
				u32StatTime = ptVencProcInfo->chnAttr.stRcAttr.stH265AVbr.u32StatTime;
				u32BitRate = ptVencProcInfo->chnAttr.stRcAttr.stH265AVbr.u32MaxBitRate;
				bVariFpsEn = ptVencProcInfo->chnAttr.stRcAttr.stH265AVbr.bVariFpsEn;

				s32ChangePos = ptVencProcInfo->rcParam.stParamH265AVbr.s32ChangePos;
				u32MinIprop = ptVencProcInfo->rcParam.stParamH265AVbr.u32MinIprop;
				u32MaxIprop = ptVencProcInfo->rcParam.stParamH265AVbr.u32MaxIprop;
				s32MinStillPercent = ptVencProcInfo->rcParam.stParamH265AVbr.s32MinStillPercent;
				u32MaxStillQP = ptVencProcInfo->rcParam.stParamH265AVbr.u32MaxStillQP;
				u32MinStillPSNR = ptVencProcInfo->rcParam.stParamH265AVbr.u32MinStillPSNR;
				u32MaxQp = ptVencProcInfo->rcParam.stParamH265AVbr.u32MaxQp;
				u32MinQp = ptVencProcInfo->rcParam.stParamH265AVbr.u32MinQp;
				u32MaxIQp = ptVencProcInfo->rcParam.stParamH265AVbr.u32MaxIQp;
				u32MinIQp = ptVencProcInfo->rcParam.stParamH265AVbr.u32MinIQp;
				u32MinQpDelta = ptVencProcInfo->rcParam.stParamH265AVbr.u32MinQpDelta;
				u32MotionSensitivity = ptVencProcInfo->rcParam.stParamH265AVbr.u32MotionSensitivity;
				s32AvbrFrmLostOpen = ptVencProcInfo->rcParam.stParamH265AVbr.s32AvbrFrmLostOpen;
				s32AvbrFrmGap = ptVencProcInfo->rcParam.stParamH265AVbr.s32AvbrFrmGap;
				u32MinStillPSNR = ptVencProcInfo->rcParam.stParamH265AVbr.s32AvbrPureStillThr;
				bQpMapEn = ptVencProcInfo->rcParam.stParamH265AVbr.bQpMapEn;
				s32MaxReEncodeTimes = ptVencProcInfo->rcParam.stParamH265AVbr.s32MaxReEncodeTimes;
			}  else if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H264QVBR) {
				strcpy(cRcMode, "QVBR");
				u32Gop = ptVencProcInfo->chnAttr.stRcAttr.stH264QVbr.u32Gop;
				u32StatTime = ptVencProcInfo->chnAttr.stRcAttr.stH264QVbr.u32StatTime;
				u32BitRate = ptVencProcInfo->chnAttr.stRcAttr.stH264QVbr.u32TargetBitRate;
			} else if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H265QVBR) {
				strcpy(cRcMode, "QVBR");
				u32Gop = ptVencProcInfo->chnAttr.stRcAttr.stH265QVbr.u32Gop;
				u32StatTime = ptVencProcInfo->chnAttr.stRcAttr.stH265QVbr.u32StatTime;
				u32BitRate = ptVencProcInfo->chnAttr.stRcAttr.stH265QVbr.u32TargetBitRate;
			} else if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H264QPMAP) {
				strcpy(cRcMode, "QPMAP");
				u32Gop = ptVencProcInfo->chnAttr.stRcAttr.stH264QpMap.u32Gop;
				u32StatTime = ptVencProcInfo->chnAttr.stRcAttr.stH264QpMap.u32StatTime;
			} else if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H265QPMAP) {
				strcpy(cRcMode, "QPMAP");
				u32Gop = ptVencProcInfo->chnAttr.stRcAttr.stH265QpMap.u32Gop;
				u32StatTime = ptVencProcInfo->chnAttr.stRcAttr.stH265QpMap.u32StatTime;
			} else if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H264UBR) {
				strcpy(cRcMode, "UBR");
				u32Gop = ptVencProcInfo->chnAttr.stRcAttr.stH264Ubr.u32Gop;
				u32StatTime = ptVencProcInfo->chnAttr.stRcAttr.stH264Ubr.u32StatTime;
				u32BitRate = ptVencProcInfo->chnAttr.stRcAttr.stH264Ubr.u32BitRate;
				bVariFpsEn = ptVencProcInfo->chnAttr.stRcAttr.stH264Ubr.bVariFpsEn;

				u32MinIprop = ptVencProcInfo->rcParam.stParamH264Ubr.u32MinIprop;
				u32MaxIprop = ptVencProcInfo->rcParam.stParamH264Ubr.u32MaxIprop;
				u32MaxQp = ptVencProcInfo->rcParam.stParamH264Ubr.u32MaxQp;
				u32MinQp = ptVencProcInfo->rcParam.stParamH264Ubr.u32MinQp;
				u32MaxIQp = ptVencProcInfo->rcParam.stParamH264Ubr.u32MaxIQp;
				u32MinIQp = ptVencProcInfo->rcParam.stParamH264Ubr.u32MinIQp;
				s32MaxReEncodeTimes = ptVencProcInfo->rcParam.stParamH264Ubr.s32MaxReEncodeTimes;
				bQpMapEn = ptVencProcInfo->rcParam.stParamH264Ubr.bQpMapEn;
			} else if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H265UBR) {
				strcpy(cRcMode, "UBR");
				u32Gop = ptVencProcInfo->chnAttr.stRcAttr.stH265Ubr.u32Gop;
				u32StatTime = ptVencProcInfo->chnAttr.stRcAttr.stH265Ubr.u32StatTime;
				u32BitRate = ptVencProcInfo->chnAttr.stRcAttr.stH265Ubr.u32BitRate;
				bVariFpsEn = ptVencProcInfo->chnAttr.stRcAttr.stH265Ubr.bVariFpsEn;

				u32MinIprop = ptVencProcInfo->rcParam.stParamH265Ubr.u32MinIprop;
				u32MaxIprop = ptVencProcInfo->rcParam.stParamH265Ubr.u32MaxIprop;
				u32MaxQp = ptVencProcInfo->rcParam.stParamH265Ubr.u32MaxQp;
				u32MinQp = ptVencProcInfo->rcParam.stParamH265Ubr.u32MinQp;
				u32MaxIQp = ptVencProcInfo->rcParam.stParamH265Ubr.u32MaxIQp;
				u32MinIQp = ptVencProcInfo->rcParam.stParamH265Ubr.u32MinIQp;
				s32MaxReEncodeTimes = ptVencProcInfo->rcParam.stParamH265Ubr.s32MaxReEncodeTimes;
				bQpMapEn = ptVencProcInfo->rcParam.stParamH265Ubr.bQpMapEn;
				enQpMapMode = ptVencProcInfo->rcParam.stParamH265Ubr.enQpMapMode;
			} else {
				strcpy(cRcMode, "N/A");
			}

			switch (enQpMapMode) {
			case VENC_RC_QPMAP_MODE_MEANQP:
				strcpy(cQpMapMode, "MEANQP");
				break;
			case VENC_RC_QPMAP_MODE_MINQP:
				strcpy(cQpMapMode, "MINQP");
				break;
			case VENC_RC_QPMAP_MODE_MAXQP:
				strcpy(cQpMapMode, "MAXQP");
				break;
			case VENC_RC_QPMAP_MODE_BUTT:
				strcpy(cQpMapMode, "BUTT");
				break;
			default:
				strcpy(cQpMapMode, "N/A");
				break;
			}

			seq_puts(m, "------BASE PARAMS 1------------------------------------------------------\n");
			seq_printf(m, "ChnId: %d\t Gop: %u\t StatTm: %u\t ViFr: %u\t TrgFr: %u\t ProType: %s",
				idx,
				u32Gop,
				u32StatTime,
				u32SrcFrameRate,
				fr32DstFrameRate,
				cCodecType);
			seq_printf(m, "\t RcMode: %s\t Br(kbps): %u\t FluLev: %d\t IQp: %u\t PQp: %u\t BQp: %u\n",
				cRcMode,
				u32BitRate,
				0,
				u32IQp,
				u32PQp,
				u32BQp);

			seq_puts(m, "------BASE PARAMS 2------------------------------------------------------\n");
			seq_printf(m, "ChnId: %d\t MinQp: %u\t MaxQp: %u\t MinIQp: %u\t MaxIQp: %u",
				idx,
				u32MinQp, u32MaxQp,
				u32MinIQp, u32MaxIQp);
			seq_printf(m, "\t EnableIdr: %d\t bQpMapEn: %d\t QpMapMode: %s\n",
				0, bQpMapEn, cQpMapMode);
			seq_printf(m, "u32RowQpDelta: %d\t",
					ptVencProcInfo->rcParam.u32RowQpDelta);
			seq_printf(m, "InitialDelay: %d\t VariFpsEn: %d\t ThrdLv: %d\t BgEnhanceEn: %d\t BgDeltaQp: %d\n",
					ptVencProcInfo->rcParam.s32InitialDelay,
					bVariFpsEn,
					ptVencProcInfo->rcParam.u32ThrdLv,
					ptVencProcInfo->rcParam.bBgEnhanceEn,
					ptVencProcInfo->rcParam.s32BgDeltaQp);

			seq_puts(m, "-----GOP MODE ATTR-------------------------------------------------------\n");
			seq_printf(m, "ChnId: %d\t GopMode: %s\t IpQpDelta: %d\t SPInterval: %u\t SPQpDelta: %d",
				idx,
				cGopMode,
				s32IPQpDelta,
				u32SPInterval,
				s32SPQpDelta);
			seq_printf(m, "\t BFrmNum: %u\t BQpDelta: %d\t BgInterval: %u\t ViQpDelta: %d\n",
				u32BFrmNum,
				s32BQpDelta,
				u32BgInterval,
				s32ViQpDelta);

			if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H264CBR ||
				ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H265CBR) {
				seq_puts(m, "-----SUPER FRAME PARAM -------------------------------------------\n");
				seq_printf(m, "ChnId: %d\t FrmMode: %d\t IFrmBitsThr: %d\t PFrmBitsThr: %d\n",
					idx,
					ptVencProcInfo->stSuperFrmParam.enSuperFrmMode,
					ptVencProcInfo->stSuperFrmParam.u32SuperIFrmBitsThr,
					ptVencProcInfo->stSuperFrmParam.u32SuperPFrmBitsThr);

				seq_puts(m, "-----RUN CBR PARAM -------------------------------------------\n");
				seq_printf(m, "ChnId: %d\t MinIprop: %u\t MaxIprop: %u\t MaxQp: %u\t MinQp: %u",
					idx,
					u32MinIprop,
					u32MaxIprop,
					u32MaxQp,
					u32MinQp);
				seq_printf(m, "\t MaxIQp: %u\t MinIQp: %u\t MaxReEncTimes: %d\n",
					u32MaxIQp,
					u32MinIQp,
					s32MaxReEncodeTimes);
			} else if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H264VBR ||
				ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H265VBR) {
				seq_puts(m, "-----RUN VBR PARAM -------------------------------------------\n");
				seq_printf(m, "ChnId: %d\t ChgPs: %d\t MinIprop: %u\t MaxIprop: %u\t MaxQp: %u",
					idx,
					s32ChangePos,
					u32MinIprop,
					u32MaxIprop,
					u32MaxQp);
				seq_printf(m, "\t MinQp: %u\t MaxIQp: %u\t MinIQp: %u\t MaxReEncTimes: %d\n",
					u32MinQp,
					u32MaxIQp,
					u32MinIQp,
					s32MaxReEncodeTimes);
			} else if (ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H264AVBR ||
				ptVencProcInfo->chnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H265AVBR) {
				seq_puts(m, "-----RUN AVBR PARAM -------------------------------------------\n");
				seq_printf(m, "ChnId: %d\t ChgPs: %d\t MinIprop: %u\t MaxIprop: %u\t MaxQp: %u",
					idx,
					s32ChangePos,
					u32MinIprop,
					u32MaxIprop,
					u32MaxQp);
				seq_printf(m, "\t MinQp: %u\t MaxIQp: %u\t MinIQp: %u\t MaxReEncTimes: %d\n",
					u32MinQp,
					u32MaxIQp,
					u32MinIQp,
					s32MaxReEncodeTimes);
				seq_printf(m, "MinStillPercent: %d\t MaxStillQP: %u\t MinStillPSNR: %u\t MinQpDelta: %u\n",
					s32MinStillPercent,
					u32MaxStillQP,
					u32MinStillPSNR,
					u32MinQpDelta);
				seq_printf(m, "MotionSensitivity: %u\t AvbrFrmLostOpen: %d\t AvbrFrmGap: %d\t bQpMapEn: %d\n",
					u32MotionSensitivity,
					s32AvbrFrmLostOpen,
					s32AvbrFrmGap,
					bQpMapEn);
			}
		}
	}

	return 0;
}

static int rc_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, rc_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops rc_proc_fops = {
	.proc_open = rc_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations rc_proc_fops = {
	.owner = THIS_MODULE,
	.open = rc_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static int vdec_proc_show(struct seq_file *m, void *v)
{
	int idx;
	vdec_proc_info_t *pProcInfoShareMem = (vdec_proc_info_t *)pVdecShm;
	proc_debug_config_t *pVdecDebugConfig = (proc_debug_config_t *)pVdecDbgShm;
	VDEC_MOD_PARAM_S *pVdecModParam = (VDEC_MOD_PARAM_S *)(pVdecDbgShm + sizeof(proc_debug_config_t));

	seq_printf(m, "Module: [VDEC] System Build Time [%s]\n", UTS_VERSION);
	seq_printf(m, "VdecMaxChnNum: %u\t MiniBufMode: %u\t enVdecVBSource: %d\t ParallelMode: %u\n",
		MAX_VDEC_CHN_NUM,
		pVdecModParam->u32MiniBufMode,
		pVdecModParam->enVdecVBSource,
		pVdecModParam->u32ParallelMode);
#ifdef CONFIG_ARCH_CV182X
	seq_printf(m, "bSingleCore %d\n", bSingleCore);
#endif

	seq_printf(m, "MaxPicWidth: %u\t MaxPicHeight: %u\t MaxSliceNum: %u",
		pVdecModParam->stVideoModParam.u32MaxPicWidth,
		pVdecModParam->stVideoModParam.u32MaxPicHeight,
		pVdecModParam->stVideoModParam.u32MaxSliceNum);
	seq_printf(m, "\t VdhMsgNum: %u\t VdhBinSize: %u\t VdhExtMemLevel: %u",
		pVdecModParam->stVideoModParam.u32VdhMsgNum,
		pVdecModParam->stVideoModParam.u32VdhBinSize,
		pVdecModParam->stVideoModParam.u32VdhExtMemLevel);
	seq_printf(m, "\t MaxJpegeWidth: %u\t MaxJpegeHeight: %u",
		pVdecModParam->stPictureModParam.u32MaxPicWidth,
		pVdecModParam->stPictureModParam.u32MaxPicHeight);
	seq_printf(m, "\t SupportProgressive: %d\t DynamicAllocate: %d\t CapStrategy: %d\n",
		pVdecModParam->stPictureModParam.bSupportProgressive,
		pVdecModParam->stPictureModParam.bDynamicAllocate,
		pVdecModParam->stPictureModParam.enCapStrategy);

	for (idx = 0; idx < MAX_VDEC_CHN_NUM; idx++) {
		vdec_proc_info_t *ptVdecProcInfo = (pProcInfoShareMem + idx);

		if (ptVdecProcInfo->u8ChnUsed == 1) {
			char cCodecType[16] = {'\0'};
			char cDecMode[8] = {'\0'};
			char cOutputOrder[8] = {'\0'};
			char cCompressMode[8] = {'\0'};
			char cPixelFormat[8] = {'\0'};

			getCodecTypeStr(ptVdecProcInfo->chnAttr.enType, cCodecType);
			getPixelFormatStr(ptVdecProcInfo->stFrame.enPixelFormat, cPixelFormat);

			switch (ptVdecProcInfo->stChnParam.stVdecVideoParam.enDecMode) {
			case VIDEO_DEC_MODE_IP:
				strcpy(cDecMode, "IP");
				break;
			case VIDEO_DEC_MODE_I:
				strcpy(cDecMode, "I");
				break;
			case VIDEO_DEC_MODE_BUTT:
				strcpy(cDecMode, "BUTT");
				break;
			case VIDEO_DEC_MODE_IPB:
			default:
				strcpy(cDecMode, "IPB");
				break;
			}

			switch (ptVdecProcInfo->stChnParam.stVdecVideoParam.enOutputOrder) {
			case VIDEO_OUTPUT_ORDER_DEC:
				strcpy(cOutputOrder, "DEC");
				break;
			case VIDEO_OUTPUT_ORDER_BUTT:
				strcpy(cOutputOrder, "BUTT");
				break;
			case VIDEO_OUTPUT_ORDER_DISP:
			default:
				strcpy(cOutputOrder, "DISP");
				break;
			}

			switch (ptVdecProcInfo->stChnParam.stVdecVideoParam.enCompressMode) {
			case COMPRESS_MODE_TILE:
				strcpy(cCompressMode, "TILE");
				break;
			case COMPRESS_MODE_LINE:
				strcpy(cCompressMode, "LINE");
				break;
			case COMPRESS_MODE_FRAME:
				strcpy(cCompressMode, "FRAME");
				break;
			case COMPRESS_MODE_BUTT:
				strcpy(cCompressMode, "BUTT");
				break;
			case COMPRESS_MODE_NONE:
			default:
				strcpy(cCompressMode, "NONE");
				break;
			}

			seq_puts(m, "----- CHN COMM ATTR & PARAMS --------------------------------------\n");
			seq_printf(m, "ID: %d\t TYPE: %s\t MaxW: %u\t MaxH: %u\t Width: %u\t Height: %u",
				idx,
				cCodecType,
				MAX_DEC_PIC_WIDTH,
				MAX_DEC_PIC_HEIGHT,
				ptVdecProcInfo->stFrame.u32Width,
				ptVdecProcInfo->stFrame.u32Height);
			seq_printf(m, "\t Stride: %u\t PixelFormat: %s\t PTS: %llu\t PA: 0x%llx\n",
				ptVdecProcInfo->stFrame.u32Stride[0],
				cPixelFormat,
				ptVdecProcInfo->stFrame.u64PTS,
				ptVdecProcInfo->stFrame.u64PhyAddr[0]);

			getPixelFormatStr(ptVdecProcInfo->stChnParam.enPixelFormat, cPixelFormat);
			seq_printf(m, "StrInputMode: %s\t StrBufSize: %u\t FrmBufSize: %u\t ParamPixelFormat %s",
				"FRAME/NOBLOCK",
				ptVdecProcInfo->chnAttr.u32StreamBufSize,
				ptVdecProcInfo->chnAttr.u32FrameBufSize,
				cPixelFormat);
			seq_printf(m, "\t FrmBufCnt: %u\t TmvBufSize: %u\n",
				ptVdecProcInfo->chnAttr.u32FrameBufCnt,
				ptVdecProcInfo->chnAttr.stVdecVideoAttr.u32TmvBufSize);

			seq_printf(m, "ID: %d\t DispNum: %d\t DispMode: %s\t SetUserPic: %s\t EnUserPic: %s",
				idx, 2, "PLAYBACK", "N", "N");
			seq_printf(m, "\t Rotation: %u\t PicPoolId: %d\t TmvPoolId: %d\t STATE: %s\n",
				0, -1, -1, "START");

			seq_puts(m, "----- CHN VIDEO ATTR & PARAMS -------------------------------------\n");
			seq_printf(m, "ID: %d\t VfmwID: %d\t RefNum: %u\t TemporalMvp: %s\t ErrThr: %d",
				idx,
				ptVdecProcInfo->chnAttr.enType == PT_H265 ? 0 : 1,
				ptVdecProcInfo->chnAttr.stVdecVideoAttr.u32RefFrameNum,
				ptVdecProcInfo->chnAttr.stVdecVideoAttr.bTemporalMvpEnable ? "Y" : "N",
				ptVdecProcInfo->stChnParam.stVdecVideoParam.s32ErrThreshold);
			seq_printf(m, "\t DecMode: %s\t OutPutOrder: %s\t Compress: %s\t VideoFormat: %d",
				cDecMode,
				cOutputOrder,
				cCompressMode,
				ptVdecProcInfo->stChnParam.stVdecVideoParam.enVideoFormat);
			seq_printf(m, "\t MaxVPS: %u\t MaxSPS: %u\t MaxPPS: %u\t MaxSlice: %u\n",
				0, 0, 0, pVdecModParam->stVideoModParam.u32MaxSliceNum);

			seq_puts(m, "----- CHN PICTURE ATTR & PARAMS---------------------------------\n");
			seq_printf(m, "ID: %d\t Alpha: %u\n",
				idx,
				ptVdecProcInfo->stChnParam.stVdecPictureParam.u32Alpha);

			// TODO: following info should be amended later
			#if 0
			seq_puts(m, "----- CHN STATE -------------------------------------------------\n");
			seq_printf(m, "ID: %d\t PrtclErr: %u\t StrmUnSP: %u\t StrmError: %u\t RefNumErr: %u",
				idx, 0, 0, 0, 0);
			seq_printf(m, "\t PicSizeErr: %u\t Fmterror: %u\t PicBufSizeErr: %u",
				0, 0, 0);
			seq_printf(m, "\t StrSizeOver: %u\t Notify: %u\t UniqueId: %u\t State: %u\n",
				0, 0, 0, 0);
			seq_printf(m, "ID: %d\t fps: %d\t TimerCnt: %u\t BufFLen: %u\t DataLen: %u",
				idx, 30, 0, 0, 0);
			seq_printf(m, "\t RdRegionLen: %u\t SCDLeftLen: %u\t WrRegionLen: %u\t ptsBufF: %u",
				0, 0, 0, 0);
			seq_printf(m, "\t ptsBufU: %u\t StreamEnd: %u\t FrameEnd: %u\n",
				0, 0, 0);

			seq_puts(m, "----- Detail Stream STATE -----------------------------------------\n");
			seq_printf(m, "ID: %d\t MpiSndNum: %u\t MpiSndLen: %u\t VdecNum: %u\t VdecLen: %u",
				idx, 0, 0, 0, 0);
			seq_printf(m, "\t FmGetNum: %u\t FmGetLen: %u\t FmRlsNum: %u\t FmRlsLen: %u\n",
				0, 0, 0, 0);

			seq_printf(m, "ID: %d\t FmLstGet: %d\t FmRlsFail: %d\n", idx, 0, 0);

			seq_puts(m, "----- Detail FrameStore STATE ---------------------------------\n");
			seq_puts(m, "NOT SUPPORTED\n");

			seq_puts(m, "----- Detail UserData STATE ----------------------------------------\n");
			seq_printf(m, "ID: %d\t Enable: %d\t MaxUserDataLen: %u\n",
				idx, 1, 1024);
			seq_printf(m, "ID: %d\t MpiGet: %d\t MpiGetLen: %u\t MpiRls: %d\t MpiRlsLen: %d",
				idx, 0, 0, 0, 0);
			seq_printf(m, "\t Discard: %d\t DiscardLen: %d\t GetFromFm: %d", 0, 0, 0);
			seq_printf(m, "\t GetFromFmLen: %d\t UsrFLen: %d\t UsrLen: %d\n", 0, 0, 0);
			#endif

			seq_puts(m, "-----VDEC CHN PERFORMANCE------------------------------------------------\n");
			seq_printf(m, "ID: %d\t No.SendStreamPerSec: %u\t No.DecFramePerSec: %u\t HwDecTime: %llu us\n\n",
				idx,
				ptVdecProcInfo->stFPS.u32InFPS, ptVdecProcInfo->stFPS.u32OutFPS,
				ptVdecProcInfo->stFPS.u64HwTime);
		}
	}

	seq_puts(m, "\n----- CVITEK Debug Level STATE ----------------------------------------\n");
	seq_printf(m, "VdecDebugMask: 0x%X\t VdecStartFrmIdx: %u\t VdecEndFrmIdx: %u\t VdecDumpPath: %s\n",
		pVdecDebugConfig->u32DbgMask, pVdecDebugConfig->u32StartFrmIdx,
		pVdecDebugConfig->u32EndFrmIdx, pVdecDebugConfig->cDumpPath);
	return 0;
}

static int vdec_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, vdec_proc_show, PDE_DATA(inode));
}

static ssize_t vdec_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
    /* debug_level list, please refer to: cvi_comm_vdec.h
     * CVI_VDEC_MASK_ERR	0x1
     * CVI_VDEC_MASK_WARN	0x2
     * CVI_VDEC_MASK_INFO	0x4
     * CVI_VDEC_MASK_FLOW	0x8
     * CVI_VDEC_MASK_DBG	0x10
     * CVI_VDEC_MASK_BS		0x100
     * CVI_VDEC_MASK_SRC	0x200
     * CVI_VDEC_MASK_API	0x400
     * CVI_VDEC_MASK_DISP	0x800
     * CVI_VDEC_MASK_PERF	0x1000
     * CVI_VDEC_MASK_CFG	0x2000
     * CVI_VDEC_MASK_TRACE	0x4000
     * CVI_VDEC_MASK_DUMP_YUV	0x10000
     * CVI_VDEC_MASK_DUMP_BS	0x20000
     */
	char cProcInputdata[MAX_PROC_STR_SIZE] = {'\0'};
	char cVdecDbgPrefix[] = "vdec=0x";	// vdec=debug_levle
	char cVdecDbgStartFrmPrefix[] = "vdec_sfn=";	// vdec_sfn=frame_idx_begin
	char cVdecDbgEndFrmPrefix[] = "vdec_efn=";	// vdec_efn=frame_idx_end
	char cVdecDbgDirPrefix[] = "vdec_dir=";	// vdec_dir=your_preference_directory
	proc_debug_config_t *pVdecDebugConfig = (proc_debug_config_t *)pVdecDbgShm;

	return vcodec_proc_write_helper(user_buf, count, pVdecDebugConfig,
		cProcInputdata, cVdecDbgPrefix, cVdecDbgStartFrmPrefix, cVdecDbgEndFrmPrefix, cVdecDbgDirPrefix, NULL);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops vdec_proc_fops = {
	.proc_open = vdec_proc_open,
	.proc_read = seq_read,
	.proc_write = vdec_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations vdec_proc_fops = {
	.owner = THIS_MODULE,
	.open = vdec_proc_open,
	.read = seq_read,
	.write = vdec_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static int vpu_misc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	#ifdef VPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_vpu_device *ndev = (struct cvi_vpu_device *)pCviVpuDevice;
	#else
	struct cvi_vpu_device *ndev = container_of(filp->private_data, struct cvi_vpu_device, miscdev);
	#endif
	unsigned long vm_start = vma->vm_start;
	unsigned int vm_size = vma->vm_end - vma->vm_start;
	unsigned int offset = vma->vm_pgoff << PAGE_SHIFT;
	void *pos = ndev->vpu_shared_mem;

	if (offset < 0 || (vm_size + offset) > VCODEC_SHARE_MEM_SIZE)
		return -EINVAL;

	while (vm_size > 0) {
		if (remap_pfn_range(vma, vm_start, virt_to_pfn(pos), PAGE_SIZE, vma->vm_page_prot))
			return -EAGAIN;
		pr_debug("mmap vir(%p) phys(%#llx)\n", pos, virt_to_phys((void *) pos));
		vm_start += PAGE_SIZE;
		pos += PAGE_SIZE;
		vm_size -= PAGE_SIZE;
	}

	return 0;
}

static const struct file_operations vpu_miscdev_fops = {
	.owner = THIS_MODULE,
	.mmap = vpu_misc_mmap,
};

static int _register_miscdev(struct cvi_vpu_device *vdev)
{
	int rc;

	vdev->miscdev.minor = MISC_DYNAMIC_MINOR;
	vdev->miscdev.name = "cvi-vcodec";
	vdev->miscdev.fops = &vpu_miscdev_fops;

	rc = misc_register(&vdev->miscdev);
	if (rc) {
		dev_err(vdev->dev, "cvi_vcodec: failed to register misc device.\n");
		return rc;
	}

	return 0;
}

static int vpu_probe(struct platform_device *pdev)
{
	int err = 0;
	const struct of_device_id *match;
	struct device *dev = &pdev->dev;
	struct cvi_vpu_device *vdev;

	vdev = devm_kzalloc(&pdev->dev, sizeof(*vdev), GFP_KERNEL);
	if (!vdev)
		return -ENOMEM;

	memset(vdev, 0, sizeof(*vdev));
	#ifdef VPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	pCviVpuDevice = vdev;
	#endif

	vdev->dev = dev;

	BUILD_BUG_ON((sizeof(venc_proc_info_t) * MAX_VENC_CHN_NUM + sizeof(proc_debug_config_t) +
		sizeof(CVI_VENC_PARAM_MOD_S)) > VCODEC_VENC_SHARE_MEM_SIZE);
	BUILD_BUG_ON((sizeof(vdec_proc_info_t) * MAX_VDEC_CHN_NUM + sizeof(proc_debug_config_t) +
		sizeof(VDEC_MOD_PARAM_S)) > VCODEC_VDEC_SHARE_MEM_SIZE);

	vdev->vpu_shared_mem = kzalloc(VCODEC_SHARE_MEM_SIZE, GFP_KERNEL);
	if (!vdev->vpu_shared_mem)
		return -ENOMEM;

	pVencDbgShm = vdev->vpu_shared_mem;
	pVencShm = pVencDbgShm + sizeof(proc_debug_config_t) + sizeof(CVI_VENC_PARAM_MOD_S);
	pVdecDbgShm = vdev->vpu_shared_mem + VCODEC_VENC_SHARE_MEM_SIZE;
	pVdecShm = pVdecDbgShm + sizeof(proc_debug_config_t) + sizeof(VDEC_MOD_PARAM_S);

	match = of_match_device(cvi_vpu_match_table, &pdev->dev);
	if (!match)
		return -EINVAL;

	vdev->pdata = match->data;

	VCODEC_DBG_INFO("pdata version 0x%x quirks 0x%x\n", vdev->pdata->version, vdev->pdata->quirks);

	err = cviGetRegResource(vdev, pdev);
	if (err) {
		VCODEC_DBG_ERR("cviGetRegResource\n");
		goto ERROR_PROBE_DEVICE;
	}

	if (vdev->pdata->quirks & VCODEC_QUIRK_SUPPORT_REMAP_DDR)
		cviConfigDDR(vdev);

	err = cvi_vcodec_register_cdev(vdev);
	if (err < 0) {
		VCODEC_DBG_ERR("cvi_vcodec_register_cdev\n");
		goto ERROR_PROBE_DEVICE;
	}

	if (vdev->pdata->ops && vdev->pdata->ops->clk_get)
		vdev->pdata->ops->clk_get(vdev);

	err = cviCfgIrq(pdev);
	if (err) {
		VCODEC_DBG_ERR("cviCfgIrq\n");
		goto ERROR_PROBE_DEVICE;
	}

#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
	if (cvi_vcodec_allocate_memory(pdev) < 0) {
		VCODEC_DBG_ERR("fail to remap\n");
		goto ERROR_PROBE_DEVICE;
	}

	if (!s_video_memory.base) {
		VCODEC_DBG_ERR("fail to remap\n");
		goto ERROR_PROBE_DEVICE;
	}

	if (vmem_init(&s_vmem, s_video_memory.phys_addr, s_video_memory.size) <
	    0) {
		VCODEC_DBG_ERR(":  fail to init vmem system\n");
		goto ERROR_PROBE_DEVICE;
	}
	VCODEC_DBG_INFO("success to probe, pa = 0x%llx, base = 0x%llx\n",
		s_video_memory.phys_addr, s_video_memory.base);
#else
	VCODEC_DBG_INFO("success to probe\n");
#endif

	platform_set_drvdata(pdev, vdev);

	if (vdev->pdata->ops->clk_get) {
		if (proc_create_data(VENC_PROC_NAME, VIDEO_PROC_PERMS, VIDEO_PROC_PARENT, &venc_proc_fops, dev)
		    == NULL) {
			dev_err(&pdev->dev, "ERROR! /proc/%s create fail\n", VENC_PROC_NAME);
			remove_proc_entry(VENC_PROC_NAME, NULL);
		}

		if (proc_create_data(H265E_PROC_NAME, VIDEO_PROC_PERMS, VIDEO_PROC_PARENT, &h265e_proc_fops, dev)
		    == NULL) {
			dev_err(&pdev->dev, "ERROR! /proc/%s create fail\n", H265E_PROC_NAME);
			remove_proc_entry(H265E_PROC_NAME, NULL);
		}

		if (proc_create_data(H264E_PROC_NAME, VIDEO_PROC_PERMS, VIDEO_PROC_PARENT, &h264e_proc_fops, dev)
		    == NULL) {
			dev_err(&pdev->dev, "ERROR! /proc/%s create fail\n", H264E_PROC_NAME);
			remove_proc_entry(H264E_PROC_NAME, NULL);
		}

		if (proc_create_data(JPEGE_PROC_NAME, VIDEO_PROC_PERMS, VIDEO_PROC_PARENT, &jpege_proc_fops, dev)
		    == NULL) {
			dev_err(&pdev->dev, "ERROR! /proc/%s create fail\n", JPEGE_PROC_NAME);
			remove_proc_entry(JPEGE_PROC_NAME, NULL);
		}

		if (proc_create_data(RC_PROC_NAME, VIDEO_PROC_PERMS, VIDEO_PROC_PARENT, &rc_proc_fops, dev)
		    == NULL) {
			dev_err(&pdev->dev, "ERROR! /proc/%s create fail\n", RC_PROC_NAME);
			remove_proc_entry(RC_PROC_NAME, NULL);
		}

		if (proc_create_data(VDEC_PROC_NAME, VIDEO_PROC_PERMS, VIDEO_PROC_PARENT, &vdec_proc_fops, dev)
		    == NULL) {
			dev_err(&pdev->dev, "ERROR! /proc/%s create fail\n", VDEC_PROC_NAME);
			remove_proc_entry(VDEC_PROC_NAME, NULL);
		}
	}

	return 0;

ERROR_PROBE_DEVICE:

	if (vdev->s_vpu_major)
		unregister_chrdev_region(vdev->cdev_id, 1);

	cviReleaseRegResource(vdev);

	platform_set_drvdata(pdev, &vcodec_dev);

	return err;
}

static int cviGetRegResource(struct cvi_vpu_device *vdev, struct platform_device *pdev)
{
	struct cvi_vcodec_context *pvctx;
	struct resource *res = NULL;
	int idx;

	vpudrv_buffer_t *pReg;

	if (!pdev) {
		VCODEC_DBG_ERR("pdev = NULL\n");
		return -1;
	}

	for (idx = 0; idx < MAX_NUM_VPU_CORE; idx++) {
		pvctx = &vcodec_dev.vcodec_ctx[idx];

		res = platform_get_resource(pdev, IORESOURCE_MEM, idx);
		if (res) {
			pReg = &pvctx->s_vpu_register;
			pReg->phys_addr = res->start;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
			pReg->virt_addr = (__u8 *)ioremap(
					res->start, res->end - res->start);
#else
			pReg->virt_addr = (__u8 *)ioremap_nocache(
					res->start, res->end - res->start);
#endif
			pReg->size = res->end - res->start;
			VCODEC_DBG_INFO("idx = %d, reg base, pa = 0x%llX, va = 0x%llX\n",
				     idx, pReg->phys_addr, (__u64)pReg->virt_addr);
		} else
			return -ENXIO;
	}

	if (vdev->pdata->quirks & VCODEC_QUIRK_SUPPORT_VC_CTRL_REG) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
		if (res) {
			pReg = &vcodec_dev.ctrl_register;
			pReg->phys_addr = res->start;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
			pReg->virt_addr = (__u8 *)ioremap(
					res->start, res->end - res->start);
#else
			pReg->virt_addr = (__u8 *)ioremap_nocache(
					res->start, res->end - res->start);
#endif
			pReg->size = res->end - res->start;
			VCODEC_DBG_INFO("vc ctrl register, reg base, pa = 0x%llX, va = 0x%llX, size = 0x%X\n",
				     pReg->phys_addr, (__u64)pReg->virt_addr, pReg->size);
		} else {
			return -ENXIO;
		}
	}

	if (vdev->pdata->quirks & VCODEC_QUIRK_SUPPORT_VC_ADDR_REMAP) {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "vc_addr_remap");
		VCODEC_DBG_INFO("platform_get_resource_byname vc_addr_remap, res = %d\n", res);
		if (res) {
			pReg = &vcodec_dev.remap_register;
			pReg->phys_addr = res->start;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
			pReg->virt_addr = (__u8 *)ioremap(
					res->start, res->end - res->start);
#else
			pReg->virt_addr = (__u8 *)ioremap_nocache(
					res->start, res->end - res->start);
#endif
			pReg->size = res->end - res->start;
			VCODEC_DBG_INFO("vc_addr_remap reg base, pa = 0x%llX, va = 0x%llX, size = 0x%X\n",
					 pReg->phys_addr, (__u64)pReg->virt_addr, pReg->size);
		} else {
			return -ENXIO;
		}
	}

	if (vdev->pdata->quirks & VCODEC_QUIRK_SUPPORT_VC_SBM) {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "vc_sbm");
		VCODEC_DBG_INFO("platform_get_resource_byname vc_sbm, res = %d\n", res);
		if (res) {
			pReg = &vcodec_dev.sbm_register;
			pReg->phys_addr = res->start;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
			pReg->virt_addr = (__u8 *)ioremap(
					res->start, res->end - res->start);
#else
			pReg->virt_addr = (__u8 *)ioremap_nocache(
					res->start, res->end - res->start);
#endif
			pReg->size = res->end - res->start;
			VCODEC_DBG_INFO("vc_sbm reg base, pa = 0x%llX, va = 0x%llX, size = 0x%X\n",
					 pReg->phys_addr, (__u64)pReg->virt_addr, pReg->size);
		} else {
			return -ENXIO;
		}
	}

	return 0;
}

static int cvi_vcodec_register_cdev(struct cvi_vpu_device *vdev)
{
	int err = 0;

	vdev->vpu_class = class_create(THIS_MODULE, VPU_CLASS_NAME);
	if (IS_ERR(vdev->vpu_class)) {
		VCODEC_DBG_ERR("create class failed\n");
		return PTR_ERR(vdev->vpu_class);
	}

	/* get the major number of the character device */
	if ((alloc_chrdev_region(&vdev->cdev_id, 0, 1, VPU_DEV_NAME)) < 0) {
		err = -EBUSY;
		VCODEC_DBG_ERR("could not allocate major number\n");
		return err;
	}
	vdev->s_vpu_major = MAJOR(vdev->cdev_id);
	VCODEC_DBG_INFO("SUCCESS alloc_chrdev_region major %d\n", vdev->s_vpu_major);

	/* initialize the device structure and register the device with the kernel */
	cdev_init(&vdev->cdev, &vpu_fops);
	vdev->cdev.owner = THIS_MODULE;

	if ((cdev_add(&vdev->cdev, vdev->cdev_id, 1)) < 0) {
		err = -EBUSY;
		VCODEC_DBG_ERR("could not allocate chrdev\n");
		return err;
	}

	device_create(vdev->vpu_class, vdev->dev, vdev->cdev_id, NULL, "%s",
		      VPU_DEV_NAME);

	err = _register_miscdev(vdev);
	if (err < 0) {
		VCODEC_DBG_ERR("regsiter cv183x_vcodec chrdev error\n");
		return err;
	}

	return err;
}

static int cviCfgIrq(struct platform_device *pdev)
{
	struct cvi_vcodec_context *pvctx;
	static const char * const irq_name[] = {"h265", "h264"};
	int core, err;

	if (!pdev) {
		VCODEC_DBG_ERR("pdev = NULL\n");
		return -1;
	}

	for (core = 0; core < MAX_NUM_VPU_CORE; core++) {
		pvctx = &vcodec_dev.vcodec_ctx[core];

		pvctx->s_vcodec_irq = platform_get_irq_byname(pdev, irq_name[core]);

		if (pvctx->s_vcodec_irq < 0) {
			VCODEC_DBG_ERR("No IRQ resource for %s\n", irq_name[core]);
			return -ENODEV;
		}

		VCODEC_DBG_INFO("core = %d, s_vcodec_irq = %d\n",
				core, pvctx->s_vcodec_irq);

		err = request_irq(pvctx->s_vcodec_irq, vpu_irq_handler, 0, irq_name[core],
				(void *)pvctx);
		if (err) {
			VCODEC_DBG_ERR("fail to register interrupt handler\n");
			return -1;
		}
	}

	return 0;
}

static void cviFreeIrq(void)
{
	struct cvi_vcodec_context *pvctx;
	int core = 0;

	for (core = 0; core < MAX_NUM_VPU_CORE; core++) {
		pvctx = &vcodec_dev.vcodec_ctx[core];

		VCODEC_DBG_INFO("core = %d, s_vcodec_irq = %d\n", core, pvctx->s_vcodec_irq);
		free_irq(pvctx->s_vcodec_irq, (void *)pvctx);
	}
}

static void cviReleaseRegResource(struct cvi_vpu_device *vdev)
{
	int idx;
	vpudrv_buffer_t *pReg;

	for (idx = 0; idx < MAX_NUM_VPU_CORE; idx++) {
		pReg = &vcodec_dev.vcodec_ctx[idx].s_vpu_register;
		cviUnmapReg(pReg);
	}

	if (vdev->pdata->quirks & VCODEC_QUIRK_SUPPORT_VC_CTRL_REG) {
		pReg = &vcodec_dev.ctrl_register;
		cviUnmapReg(pReg);
	}
}

static void cviUnmapReg(vpudrv_buffer_t *pReg)
{
	if (pReg->virt_addr) {
		iounmap((void *)pReg->virt_addr);
		pReg->virt_addr = NULL;
	}
}

static int cvi_vcodec_allocate_memory(struct platform_device *pdev)
{
	struct device_node *target = NULL;
	struct reserved_mem *prmem = NULL;

	if (pdev) {
		target =
			of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	}

	if (target) {
		prmem = of_reserved_mem_lookup(target);
		of_node_put(target);

		if (!prmem) {
			VCODEC_DBG_ERR(": cannot acquire memory-region\n");
			return -1;
		}
	} else {
		VCODEC_DBG_ERR(": cannot find the node, memory-region\n");
		return -1;
	}

	VCODEC_DBG_INFO("pool name = %s, size = 0x%llx, base = 0x%llx\n",
	       prmem->name, prmem->size, prmem->base);

	s_video_memory.phys_addr = (unsigned long)prmem->base;
	s_video_memory.size = (unsigned int)prmem->size;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	s_video_memory.base = (unsigned long)devm_ioremap(
		&pdev->dev, s_video_memory.phys_addr, s_video_memory.size);
#else
	s_video_memory.base = (unsigned long)devm_ioremap_nocache(
		&pdev->dev, s_video_memory.phys_addr, s_video_memory.size);
#endif

	if (bSingleCore && s_video_memory.size >= (MAX_NUM_VPU_CORE * SIZE_COMMON)) {
		VCODEC_DBG_WARN("using singleCore but with 2 core reserved mem!\n");
	}
	if (!bSingleCore && s_video_memory.size < (MAX_NUM_VPU_CORE * SIZE_COMMON)) {
		VCODEC_DBG_ERR("not enough reserved memory for VPU\n");
		return -1;
	}

	if (!s_video_memory.base) {
		VCODEC_DBG_ERR("ioremap fail!\n");
		VCODEC_DBG_ERR("s_video_memory.base = 0x%llx\n", s_video_memory.base);
		return -1;
	}

	VCODEC_DBG_INFO("pa = 0x%llx, base = 0x%llx, size = 0x%x\n",
		s_video_memory.phys_addr, s_video_memory.base,
		s_video_memory.size);
	VCODEC_DBG_INFO("success to probe vcodec\n");

	return 0;
}

static int vpu_remove(struct platform_device *pdev)
{
	struct cvi_vpu_device *vdev = platform_get_drvdata(pdev);

	if (vcodec_dev.s_instance_pool.base) {
#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
		vfree((const void *)vcodec_dev.s_instance_pool.base);
#else
		vpu_free_dma_buffer(&vcodec_dev.s_instance_pool);
#endif
		vcodec_dev.s_instance_pool.base = 0;
	}

	if (vcodec_dev.s_common_memory.base) {
		vpu_free_dma_buffer(&vcodec_dev.s_common_memory);
		vcodec_dev.s_common_memory.base = 0;
	}

#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
	if (s_video_memory.base) {
		s_video_memory.base = 0;
		vmem_exit(&s_vmem);
	}
#endif

	kfree(vdev->vpu_shared_mem);
	pVencDbgShm = pVencShm = NULL;
	pVdecDbgShm = pVdecShm = NULL;

	if (vdev->s_vpu_major > 0) {
		VCODEC_DBG_INFO("vdev %p vdev->cdev %p\n", vdev, &vdev->cdev);
		device_destroy(vdev->vpu_class, vdev->cdev_id);
		class_destroy(vdev->vpu_class);
		cdev_del(&vdev->cdev);
		unregister_chrdev_region(vdev->cdev_id, 1);
		vdev->s_vpu_major = 0;
	}

	cviFreeIrq();

	cviReleaseRegResource(vdev);

	return 0;
}

#ifdef CONFIG_PM
#define W4_MAX_CODE_BUF_SIZE (512 * 1024)
#define W4_CMD_INIT_VPU (0x0001)
#define W4_CMD_SLEEP_VPU (0x0400)
#define W4_CMD_WAKEUP_VPU (0x0800)
#define W5_CMD_SLEEP_VPU (0x0004)
#define W5_CMD_WAKEUP_VPU (0x0002)

static void Wave4BitIssueCommand(int core, u32 cmd)
{
	struct cvi_vcodec_context *pvctx = &vcodec_dev.vcodec_ctx[core];

	WriteVpuRegister(W4_VPU_BUSY_STATUS, 1);
	WriteVpuRegister(W4_CORE_INDEX, 0);
	/*	coreIdx = ReadVpuRegister(W4_VPU_BUSY_STATUS);*/
	/*	coreIdx = 0;*/
	/*	WriteVpuRegister(W4_INST_INDEX,  (instanceIndex&0xffff)|(codecMode<<16));*/
	WriteVpuRegister(W4_COMMAND, cmd);
	WriteVpuRegister(W4_VPU_HOST_INT_REQ, 1);
}

static int vpu_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct cvi_vcodec_context *pvctx;
	int i;
	int core;
	unsigned long timeout = jiffies + HZ; /* vpu wait timeout to 1sec */
	int product_code;
	struct cvi_vpu_device *vdev = platform_get_drvdata(pdev);

	set_clock_enable(vdev, VCODEC_CLK_ENABLE, BIT(H264_CORE_IDX) | BIT(H265_CORE_IDX));

	if (s_vpu_open_ref_count > 0) {
		for (core = 0; core < MAX_NUM_VPU_CORE; core++) {
			pvctx = &vcodec_dev.vcodec_ctx[core];
			if (pvctx->s_bit_firmware_info.size == 0)
				continue;
			product_code =
				ReadVpuRegister(VPU_PRODUCT_CODE_REGISTER);

			if (PRODUCT_CODE_W_SERIES(product_code)) {
				unsigned long cmd_reg = W4_CMD_SLEEP_VPU;
				unsigned long suc_reg = W4_RET_SUCCESS;

				while (ReadVpuRegister(W4_VPU_BUSY_STATUS)) {
					if (time_after(jiffies, timeout)) {
						VCODEC_DBG_ERR("SLEEP_VPU BUSY timeout");
						goto DONE_SUSPEND;
					}
				}

				if (product_code == WAVE512_CODE ||
				    product_code == WAVE520_CODE) {
					cmd_reg = W5_CMD_SLEEP_VPU;
					suc_reg = W5_RET_SUCCESS;
				}
				Wave4BitIssueCommand(core, cmd_reg);

				while (ReadVpuRegister(W4_VPU_BUSY_STATUS)) {
					if (time_after(jiffies, timeout)) {
						VCODEC_DBG_ERR("SLEEP_VPU BUSY timeout");
						goto DONE_SUSPEND;
					}
				}
				if (ReadVpuRegister(suc_reg) == 0) {
					VCODEC_DBG_ERR("SLEEP_VPU failed [0x%x]",
						ReadVpuRegister(
							W4_RET_FAIL_REASON));
					goto DONE_SUSPEND;
				}
			} else if (PRODUCT_CODE_NOT_W_SERIES(product_code)) {
				while (ReadVpuRegister(BIT_BUSY_FLAG)) {
					if (time_after(jiffies, timeout))
						goto DONE_SUSPEND;
				}

				for (i = 0; i < 64; i++)
					s_vpu_reg_store[core][i] =
						ReadVpuRegister(
							BIT_BASE +
							(0x100 + (i * 4)));
			} else {
				VCODEC_DBG_ERR("Unknown product id : %08x\n",
					product_code);
				goto DONE_SUSPEND;
			}
		}
	}

	set_clock_enable(vdev, VCODEC_CLK_DISABLE, BIT(H264_CORE_IDX) | BIT(H265_CORE_IDX));

	return 0;

DONE_SUSPEND:

	set_clock_enable(vdev, VCODEC_CLK_DISABLE, BIT(H264_CORE_IDX) | BIT(H265_CORE_IDX));

	return -EAGAIN;
}

static int vpu_resume(struct platform_device *pdev)
{
	struct cvi_vcodec_context *pvctx;
	int i;
	int core;
	u32 val;
	unsigned long timeout = jiffies + HZ; /* vpu wait timeout to 1sec */
	int product_code;
	struct cvi_vpu_device *vdev = platform_get_drvdata(pdev);

	unsigned long code_base;
	u32 code_size;
	u32 remap_size;
	int regVal;
	u32 hwOption = 0;

	set_clock_enable(vdev, VCODEC_CLK_ENABLE, BIT(H264_CORE_IDX) | BIT(H265_CORE_IDX));

	for (core = 0; core < MAX_NUM_VPU_CORE; core++) {
		pvctx = &vcodec_dev.vcodec_ctx[core];
		if (pvctx->s_bit_firmware_info.size == 0) {
			continue;
		}

		product_code = ReadVpuRegister(VPU_PRODUCT_CODE_REGISTER);
		if (PRODUCT_CODE_W_SERIES(product_code)) {
			unsigned long addr_code_base_reg = W4_ADDR_CODE_BASE;
			unsigned long code_size_reg = W4_CODE_SIZE;
			unsigned long code_param_reg = W4_CODE_PARAM;
			unsigned long timeout_cnt_reg =
				W4_INIT_VPU_TIME_OUT_CNT;
			unsigned long hw_opt_reg = W4_HW_OPTION;
			unsigned long suc_reg = W4_RET_SUCCESS;

			if (product_code == WAVE512_CODE ||
			    product_code == WAVE520_CODE) {
				addr_code_base_reg = W5_ADDR_CODE_BASE;
				code_size_reg = W5_CODE_SIZE;
				code_param_reg = W5_CODE_PARAM;
				timeout_cnt_reg = W5_INIT_VPU_TIME_OUT_CNT;
				hw_opt_reg = W5_HW_OPTION;
				suc_reg = W5_RET_SUCCESS;
			}

			code_base = vcodec_dev.s_common_memory.phys_addr;
			/* ALIGN TO 4KB */
			code_size = (W4_MAX_CODE_BUF_SIZE & ~0xfff);
			if (code_size < pvctx->s_bit_firmware_info.size * 2) {
				goto DONE_WAKEUP;
			}

			regVal = 0;
			WriteVpuRegister(W4_PO_CONF, regVal);

			/* Reset All blocks */
			regVal = 0x7ffffff;
			WriteVpuRegister(W4_VPU_RESET_REQ,
					 regVal); /*Reset All blocks*/

			/* Waiting reset done */
			while (ReadVpuRegister(W4_VPU_RESET_STATUS)) {
				if (time_after(jiffies, timeout))
					goto DONE_WAKEUP;
			}

			WriteVpuRegister(W4_VPU_RESET_REQ, 0);

			/* remap page size */
			remap_size = (code_size >> 12) & 0x1ff;
			regVal = 0x80000000 | (W4_REMAP_CODE_INDEX << 12) |
				 (0 << 16) | (1 << 11) | remap_size;
			WriteVpuRegister(W4_VPU_REMAP_CTRL, regVal);
			WriteVpuRegister(W4_VPU_REMAP_VADDR,
					 0x00000000); /* DO NOT CHANGE! */
			WriteVpuRegister(W4_VPU_REMAP_PADDR, code_base);
			WriteVpuRegister(addr_code_base_reg, code_base);
			WriteVpuRegister(code_size_reg, code_size);
			WriteVpuRegister(code_param_reg, 0);
			WriteVpuRegister(timeout_cnt_reg, timeout);

			WriteVpuRegister(hw_opt_reg, hwOption);

			/* Interrupt */
			if (product_code == WAVE512_CODE) {
				// decoder
				regVal = (1 << W5_INT_INIT_SEQ);
				regVal |= (1 << W5_INT_DEC_PIC);
				regVal |= (1 << W5_INT_BSBUF_EMPTY);
			} else if (product_code == WAVE520_CODE) {
				regVal = (1 << W5_INT_ENC_SET_PARAM);
				regVal |= (1 << W5_INT_ENC_PIC);
			} else {
				regVal = (1 << W4_INT_DEC_PIC_HDR);
				regVal |= (1 << W4_INT_DEC_PIC);
				regVal |= (1 << W4_INT_QUERY_DEC);
				regVal |= (1 << W4_INT_SLEEP_VPU);
				regVal |= (1 << W4_INT_BSBUF_EMPTY);
			}

			WriteVpuRegister(W4_VPU_VINT_ENABLE, regVal);

			Wave4BitIssueCommand(core, W4_CMD_INIT_VPU);
			WriteVpuRegister(W4_VPU_REMAP_CORE_START, 1);

			while (ReadVpuRegister(W4_VPU_BUSY_STATUS)) {
				if (time_after(jiffies, timeout))
					goto DONE_WAKEUP;
			}

			if (ReadVpuRegister(suc_reg) == 0) {
				VCODEC_DBG_ERR("WAKEUP_VPU failed [0x%x]",
					ReadVpuRegister(W4_RET_FAIL_REASON));
				goto DONE_WAKEUP;
			}
		} else if (PRODUCT_CODE_NOT_W_SERIES(product_code)) {
			WriteVpuRegister(BIT_CODE_RUN, 0);

			/*---- LOAD BOOT CODE*/
			for (i = 0; i < 512; i++) {
				val = pvctx->s_bit_firmware_info.bit_code[i];
				WriteVpuRegister(BIT_CODE_DOWN,
						 ((i << 16) | val));
			}

			for (i = 0; i < 64; i++)
				WriteVpuRegister(BIT_BASE + (0x100 + (i * 4)),
						 s_vpu_reg_store[core][i]);

			WriteVpuRegister(BIT_BUSY_FLAG, 1);
			WriteVpuRegister(BIT_CODE_RESET, 1);
			WriteVpuRegister(BIT_CODE_RUN, 1);

			while (ReadVpuRegister(BIT_BUSY_FLAG)) {
				if (time_after(jiffies, timeout))
					goto DONE_WAKEUP;
			}

		} else {
			VCODEC_DBG_ERR("Unknown product id : %08x\n",
				product_code);
			goto DONE_WAKEUP;
		}
	}

	if (s_vpu_open_ref_count == 0)
		set_clock_enable(vdev, VCODEC_CLK_DISABLE, BIT(H264_CORE_IDX) | BIT(H265_CORE_IDX));

DONE_WAKEUP:

	if (s_vpu_open_ref_count > 0)
		set_clock_enable(vdev, VCODEC_CLK_ENABLE, BIT(H264_CORE_IDX) | BIT(H265_CORE_IDX));

	return 0;
}

#else
#define vpu_suspend NULL
#define vpu_resume NULL
#endif /* !CONFIG_PM */

static struct platform_driver vpu_driver = {
	.driver = {
		.name = VPU_PLATFORM_DEVICE_NAME,
		.of_match_table = cvi_vpu_match_table,
	},
	.probe = vpu_probe,
	.remove = vpu_remove,
	.suspend = vpu_suspend,
	.resume = vpu_resume,
};

static int __init vpu_init(void)
{
	struct cvi_vcodec_context *pvctx;
	int res, core;

	for (core = 0; core < MAX_NUM_VPU_CORE; core++) {
		pvctx = &vcodec_dev.vcodec_ctx[core];
		init_waitqueue_head(&pvctx->s_interrupt_wait_q);
	}

	init_waitqueue_head(&hasBsWaitQ);
	vcodec_dev.s_common_memory.base = 0;
	vcodec_dev.s_instance_pool.base = 0;
	res = platform_driver_register(&vpu_driver);

	return res;
}

static void __exit vpu_exit(void)
{
	remove_proc_entry(VENC_PROC_NAME, NULL);
	remove_proc_entry(H265E_PROC_NAME, NULL);
	remove_proc_entry(H264E_PROC_NAME, NULL);
	remove_proc_entry(JPEGE_PROC_NAME, NULL);
	remove_proc_entry(RC_PROC_NAME, NULL);
	remove_proc_entry(VDEC_PROC_NAME, NULL);

	platform_driver_unregister(&vpu_driver);
}

MODULE_AUTHOR("CVITEKVPU Inc.");
MODULE_DESCRIPTION("CVITEK VPU linux driver");
MODULE_LICENSE("GPL");

module_init(vpu_init);
module_exit(vpu_exit);

int vpu_hw_reset(void)
{
	return 0;
}
