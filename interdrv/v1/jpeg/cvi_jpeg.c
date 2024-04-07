/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_jpeg.c
 * Description: jpeg system interface
 */

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_graph.h>
#include <linux/of_device.h>
#include <linux/reset.h>
#include <linux/of_reserved_mem.h>
#include <linux/streamline_annotate.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/signal.h>
#endif

#include "cvi_jpeg.h"
#include "jpeg_common.h"

#define VERSION "CVITEK"
//#define ENABLE_DEBUG_MSG
#ifdef ENABLE_DEBUG_MSG
#define DPRINTK(args...) pr_info(args)
#else
#define DPRINTK(args...)
#endif

#define JPU_PLATFORM_DEVICE_NAME	"jpeg"
#define JPU_CLK_NAME			"jpeg_clk"
#define JPU_CLASS_NAME			"jpu"
#define JPU_DEV_NAME			"jpu"

#define JPU_REG_SIZE 0x300
#define JPU_CONTROL_REG_ADDR 0x50008008
#define JPU_CONTROL_REG_SIZE 0x4

#ifndef VM_RESERVED // for kernel up to 3.7.0 version
#define VM_RESERVED (VM_DONTEXPAND | VM_DONTDUMP)
#endif

#define MJPEG_PIC_STATUS_REG 0x4
#define MJPEG_INTR_MASK_REG  0x0C0

static DEFINE_SEMAPHORE(s_jpu_sem);

int jpu_mask = JPU_MASK_ERR;
module_param(jpu_mask, int, 0644);

#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
// global variable to avoid kernal config mismatch in filp->private_data
static void *pCviJpuDevice;
#endif

static int cvi_jpu_register_cdev(struct cvi_jpu_device *jdev);
static void cvi_jpu_unregister_cdev(struct platform_device *pdev);
static int jpu_allocate_memory(struct cvi_jpu_device *jdev, struct platform_device *pdev);

static void set_clock_enable(struct cvi_jpu_device *jdev, int enable)
{
	if (jdev->pdata->quirks & (JPU_QUIRK_SUPPORT_CLOCK_CONTROL | JPU_QUIRK_SUPPORT_FPGA)) {
		if (enable) {
			if (jdev->pdata->ops->clk_enable)
				jdev->pdata->ops->clk_enable(jdev);
		} else {
			if (jdev->pdata->ops->clk_disable)
			jdev->pdata->ops->clk_disable(jdev);
		}
	}
}

static int jpu_alloc_dma_buffer(struct cvi_jpu_device *jdev, struct jpudrv_buffer_t *jb)
{
	JPU_DBG_MEM("size = 0x%x\n", jb->size);

	jb->phys_addr = (unsigned long)jmem_alloc(&jdev->jmem, jb->size, 0);
	if (jb->phys_addr == (unsigned long)-1) {
		pr_err("[JPUDRV] Physical memory allocation error size=%d\n",
		       jb->size);
		return -1;
	}

	jb->base = (unsigned long)(jdev->video_memory.base +
				   (jb->phys_addr - jdev->video_memory.phys_addr));

	return 0;
}

static int jpu_free_dma_buffer(struct cvi_jpu_device *jdev, struct jpudrv_buffer_t *jb)
{
	if (jb->base) {
		int ret = jmem_free(&jdev->jmem, jb->phys_addr, 0);

		if (ret < 0)
			return -1;
	}

	return 0;
}

static int jpu_free_instances(struct file *filp)
{
	u32 inst_idx;
	struct jpudrv_instanace_list_t *jil, *n;
	struct jpudrv_instance_pool_t *jip;
	#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_jpu_device *jdev = (struct cvi_jpu_device *)pCviJpuDevice;
	#else
	struct cvi_jpu_device *jdev = filp->private_data;
	#endif
	struct list_head s_jpu_inst_list_head_tmp;
	unsigned long flags = 0;

	INIT_LIST_HEAD(&s_jpu_inst_list_head_tmp);

	spin_lock_irqsave(&jdev->jpu_lock, flags);
	list_for_each_entry_safe(jil, n, &jdev->s_jpu_inst_list_head, list) {
		if (jil->filp == filp) {
			inst_idx = jil->inst_idx;
			jip = (struct jpudrv_instance_pool_t *)jdev->jpu_instance_pool.base;
			if (jip)
				memset(&jip->jpgInstPool[inst_idx], 0x00,
				       4); // only first 4 byte is key point to free the corresponding instance.
			jdev->open_instance_count--;
			list_add(&jil->list, &s_jpu_inst_list_head_tmp);
			list_del(&jil->list);
		}
	}
	spin_unlock_irqrestore(&jdev->jpu_lock, flags);

	list_for_each_entry_safe(jil, n, &s_jpu_inst_list_head_tmp, list)
		kfree(jil);

	return jdev->open_instance_count;
}

static int jpu_free_buffers(struct file *filp)
{
	struct jpudrv_buffer_pool_t *jpb, *n;
	struct jpudrv_buffer_t jb;
	#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_jpu_device *jdev = (struct cvi_jpu_device *)pCviJpuDevice;
	#else
	struct cvi_jpu_device *jdev = filp->private_data;
	#endif
	struct list_head s_jbp_head_tmp;
	unsigned long flags = 0;

	INIT_LIST_HEAD(&s_jbp_head_tmp);

	spin_lock_irqsave(&jdev->jpu_lock, flags);
	list_for_each_entry_safe(jpb, n, &jdev->s_jbp_head, list) {
		if (jpb->filp == filp) {
			jb = jpb->jb;
			if (jb.base) {
				list_add(&jpb->list, &s_jbp_head_tmp);
				list_del(&jpb->list);
			}
		}
	}
	spin_unlock_irqrestore(&jdev->jpu_lock, flags);

	list_for_each_entry_safe(jpb, n, &s_jbp_head_tmp, list) {
		jb = jpb->jb;
		jpu_free_dma_buffer(jdev, &jb);
		kfree(jpb);
	}

	return 0;
}

static irqreturn_t jpu_irq_handler(int irq, void *data)
{
	struct cvi_jpu_device *jdev = data;

	jpudrv_buffer_t *pReg = &jdev->jpu_register;
	writel(0xFF, pReg->virt_addr + MJPEG_INTR_MASK_REG);//disable_irq_nosync(jdev->jpu_irq);

	if (jdev->async_queue)
		kill_fasync(&jdev->async_queue, SIGIO,
			    POLL_IN); // notify the interrupt to userspace

	jdev->interrupt_flag = 1;

	wake_up_interruptible(&jdev->interrupt_wait_q);

	return IRQ_HANDLED;
}

static int jpu_open(struct inode *inode, struct file *filp)
{
	unsigned long flags = 0;
	#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_jpu_device *jdev = (struct cvi_jpu_device *)pCviJpuDevice;
	#else
	struct cvi_jpu_device *jdev =
		container_of(inode->i_cdev, struct cvi_jpu_device, cdev);
	#endif

	spin_lock_irqsave(&jdev->jpu_lock, flags);
	jdev->process_count++;
	spin_unlock_irqrestore(&jdev->jpu_lock, flags);

	JPU_DBG_INFO("jpu_open: jdev->process_count=%d\n", jdev->process_count);

	#ifndef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	filp->private_data = jdev;
	#endif

	return 0;
}

static long jpu_ioctl(struct file *filp, u_int cmd, u_long arg)
{
	unsigned long flags = 0;
	#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_jpu_device *jdev = (struct cvi_jpu_device *)pCviJpuDevice;
	#else
	struct cvi_jpu_device *jdev = filp->private_data;
	#endif
	int ret = 0;

	switch (cmd) {
	case JDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY: {
		struct jpudrv_buffer_pool_t *jbp;

		down(&s_jpu_sem);

		jbp = kzalloc(sizeof(*jbp), GFP_KERNEL);
		if (!jbp) {
			up(&s_jpu_sem);
			return -ENOMEM;
		}

		ret = copy_from_user(&(jbp->jb), (struct jpudrv_buffer_t *)arg,
				     sizeof(struct jpudrv_buffer_t));
		if (ret) {
			kfree(jbp);
			up(&s_jpu_sem);
			return -EFAULT;
		}

		ret = jpu_alloc_dma_buffer(jdev, &(jbp->jb));
		if (ret == -1) {
			ret = -ENOMEM;
			kfree(jbp);
			up(&s_jpu_sem);
			break;
		}

		ret = copy_to_user((void __user *)arg, &(jbp->jb),
				   sizeof(struct jpudrv_buffer_t));
		if (ret) {
			kfree(jbp);
			ret = -EFAULT;
			up(&s_jpu_sem);
			break;
		}

		jbp->filp = filp;
		spin_lock_irqsave(&jdev->jpu_lock, flags);
		list_add(&jbp->list, &jdev->s_jbp_head);
		spin_unlock_irqrestore(&jdev->jpu_lock, flags);
		up(&s_jpu_sem);
	} break;

	case JDI_IOCTL_FREE_PHYSICAL_MEMORY: {
		struct jpudrv_buffer_pool_t *jbp, *n;
		struct jpudrv_buffer_t jb;
		bool find_jpb = false;

		down(&s_jpu_sem);

		ret = copy_from_user(&jb, (struct jpudrv_buffer_t *)arg,
				     sizeof(struct jpudrv_buffer_t));
		if (ret) {
			up(&s_jpu_sem);
			return -EACCES;
		}

		if (jb.base) {
			ret = jpu_free_dma_buffer(jdev, &jb);
			if (ret < 0) {
				up(&s_jpu_sem);
				return -EACCES;
			}
		}

		spin_lock_irqsave(&jdev->jpu_lock, flags);
		list_for_each_entry_safe(jbp, n, &jdev->s_jbp_head, list) {
			if (jbp->jb.base == jb.base) {
				list_del(&jbp->list);
				find_jpb = true;
				break;
			}
		}
		spin_unlock_irqrestore(&jdev->jpu_lock, flags);

		if (find_jpb)
			kfree(jbp);

		up(&s_jpu_sem);
	} break;

	case JDI_IOCTL_GET_RESERVED_VIDEO_MEMORY_INFO: {
		if (jdev->video_memory.base == 0) {
			ret = -EFAULT;
			break;
		}

		ret = copy_to_user((void __user *)arg, &jdev->video_memory,
				   sizeof(struct jpudrv_buffer_t));
		if (ret != 0)
			ret = -EFAULT;
	} break;

	case JDI_IOCTL_WAIT_INTERRUPT: {
		struct jpu_intr_info_t info;
		jpudrv_buffer_t *pReg = &jdev->jpu_register;

		ret = copy_from_user(&info, (struct jpu_intr_info_t *)arg, sizeof(struct jpu_intr_info_t));
		if (ret != 0) {
			JPU_DBG_ERR("ret = %d\n", ret);
			ret = -EFAULT;
			break;
		}

		if (!wait_event_interruptible_timeout(
					jdev->interrupt_wait_q, jdev->interrupt_flag != 0,
					msecs_to_jiffies(info.timeout))) {
			JPU_DBG_ERR("wait interrupt timeout\n");
			ret = -ETIME;
			break;
		}

		ANNOTATE_CHANNEL_COLOR(1, ANNOTATE_GREEN, "jpeg end");
		if (signal_pending(current)) {
			JPU_DBG_ERR("signal_pending\n");
			ret = -ERESTARTSYS;
			break;
		}

		info.intr_reason = readl(pReg->virt_addr + MJPEG_PIC_STATUS_REG);
		jdev->interrupt_flag = 0;

		ret = copy_to_user((void __user *)arg, &info,
				   sizeof(struct jpu_intr_info_t));
		if (ret != 0) {
			JPU_DBG_ERR("ret = %d\n", ret);
			return -EFAULT;
		}

		writel(0x0, pReg->virt_addr + MJPEG_INTR_MASK_REG);//enable_irq(jdev->jpu_irq);
		ANNOTATE_CHANNEL_END(1);
		ANNOTATE_NAME_CHANNEL(1, 1, "jpeg end");
	} break;

	case JDI_IOCTL_SET_CLOCK_GATE: {
		u32 clkgate;

		if (get_user(clkgate, (u32 __user *)arg)) {
			pr_err("get clkgate failed\n");
			return -EFAULT;
		}

		set_clock_enable(jdev, clkgate);

	} break;

	case JDI_IOCTL_GET_INSTANCE_POOL: {
		down(&s_jpu_sem);

		if (jdev->jpu_instance_pool.base == 0) {
			ret = copy_from_user(&jdev->jpu_instance_pool,
					     (struct jpudrv_buffer_t *)arg,
					     sizeof(struct jpudrv_buffer_t));
			if (ret != 0) {
				ret = -EFAULT;
				up(&s_jpu_sem);
				break;
			}

#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
			jdev->jpu_instance_pool.size =
				PAGE_ALIGN(jdev->jpu_instance_pool.size);
			jdev->jpu_instance_pool.phys_addr =
				jdev->jpu_instance_pool.base =
					(unsigned long)vmalloc(
						jdev->jpu_instance_pool.size);
			if (jdev->jpu_instance_pool.base == 0) {
				ret = -EFAULT;
				up(&s_jpu_sem);
				break;
			}
			memset((void *)jdev->jpu_instance_pool.base, 0x0,
			       jdev->jpu_instance_pool.size);
#else
			if (jpu_alloc_dma_buffer(jdev, &jdev->jpu_instance_pool) == -1) {
				ret = -EFAULT;
				up(&s_jpu_sem);
				break;
			}
			memset_io((void *)jdev->jpu_instance_pool.base, 0x0,
				  jdev->jpu_instance_pool.size);
#endif
			JPU_DBG_INFO("jdev->jpu_instance_pool base: 0x%llx, size: %d\n",
				jdev->jpu_instance_pool.base,
				jdev->jpu_instance_pool.size);
		}

		ret = copy_to_user((void __user *)arg, &jdev->jpu_instance_pool,
				   sizeof(struct jpudrv_buffer_t));
		if (ret != 0)
			ret = -EFAULT;

		up(&s_jpu_sem);
	} break;
	case JDI_IOCTL_OPEN_INSTANCE: {
		u32 inst_idx;
		struct jpudrv_instanace_list_t *jil;

		jil = kzalloc(sizeof(*jil), GFP_KERNEL);
		if (!jil)
			return -ENOMEM;

		if (get_user(inst_idx, (u32 __user *)arg)) {
			kfree(jil);
			return -EFAULT;
		}

		jil->inst_idx = inst_idx;
		jil->filp = filp;
		spin_lock_irqsave(&jdev->jpu_lock, flags);
		list_add(&jil->list, &jdev->s_jpu_inst_list_head);
		jdev->open_instance_count++;
		spin_unlock_irqrestore(&jdev->jpu_lock, flags);
	} break;
	case JDI_IOCTL_CLOSE_INSTANCE: {
		u32 inst_idx;
		struct jpudrv_instanace_list_t *jil, *n;
		bool find_jil = false;

		if (get_user(inst_idx, (u32 __user *)arg))
			return -EFAULT;

		spin_lock_irqsave(&jdev->jpu_lock, flags);
		list_for_each_entry_safe(jil, n, &jdev->s_jpu_inst_list_head, list) {
			if (jil->inst_idx == inst_idx) {
				jdev->open_instance_count--;
				list_del(&jil->list);
				find_jil = true;
				break;
			}
		}
		spin_unlock_irqrestore(&jdev->jpu_lock, flags);

		if (find_jil) {
			kfree(jil);
			JPU_DBG_INFO("IOCTL_CLOSE_INSTANCE, inst_idx=%d, open_count=%d\n",
				(int)inst_idx, jdev->open_instance_count);
		}

	} break;
	case JDI_IOCTL_GET_INSTANCE_NUM: {
		down(&s_jpu_sem);

		ret = copy_to_user((void __user *)arg, &jdev->open_instance_count,
				   sizeof(int));
		if (ret != 0)
			ret = -EFAULT;

		JPU_DBG_INFO("IOCTL_GET_INSTANCE_NUM open_count=%d\n",
			jdev->open_instance_count);

		up(&s_jpu_sem);
	} break;
	case JDI_IOCTL_RESET: {
	} break;
	case JDI_IOCTL_GET_REGISTER_INFO: {
		JPU_DBG_INFO("[+]JDI_IOCTL_GET_REGISTER_INFO\n");
		ret = copy_to_user((void __user *)arg, &jdev->jpu_register,
				   sizeof(struct jpudrv_buffer_t));
		if (ret != 0)
			ret = -EFAULT;
		JPU_DBG_INFO("[-]GET_REGISTER_INFO, pa = 0x%llx, va = %p, size = %d\n",
			jdev->jpu_register.phys_addr, jdev->jpu_register.virt_addr,
			jdev->jpu_register.size);
	} break;
	case JDI_IOCTL_GET_CONTROL_REG: {
		JPU_DBG_INFO("[+]GET_CONTROL_REG\n");
		ret = copy_to_user((void __user *)arg, &jdev->jpu_control_register,
				   sizeof(struct jpudrv_buffer_t));
		if (ret != 0)
			ret = -EFAULT;
		JPU_DBG_INFO("[-]GET_CONTROL_REG, pa = 0x%llx, va = %p, size = %d\n",
			jdev->jpu_control_register.phys_addr,
			jdev->jpu_control_register.virt_addr,
			jdev->jpu_control_register.size);
	} break;
	default: {
		pr_err("No such IOCTL, cmd is %d\n", cmd);
	} break;
	}
	return ret;
}

static int jpu_release(struct inode *inode, struct file *filp)
{
	unsigned long flags = 0;
	#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_jpu_device *jdev = (struct cvi_jpu_device *)pCviJpuDevice;
	#else
	struct cvi_jpu_device *jdev =
		container_of(inode->i_cdev, struct cvi_jpu_device, cdev);
	#endif

	JPU_DBG_FLOW("jpu_release: jdev->process_count=%d\n", jdev->process_count);
	down(&s_jpu_sem); // TODO

	/* found and free the not handled buffer by user applications */
	jpu_free_buffers(filp);

	spin_lock_irqsave(&jdev->jpu_lock, flags);
	jdev->process_count--;
	spin_unlock_irqrestore(&jdev->jpu_lock, flags);

	if (jdev->process_count == 0) {
		/* found and free the instance not closed by user applications */
		jpu_free_instances(filp);

		if (jdev->jpu_instance_pool.base) {
			JPU_DBG_INFO("[JPUDRV] free instance pool, base = 0x%llX\n", jdev->jpu_instance_pool.base);
#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
			vfree((const void *)jdev->jpu_instance_pool.base);
#else
			jpu_free_dma_buffer(jdev, &jdev->jpu_instance_pool);
#endif
			jdev->jpu_instance_pool.base = 0;
		}
	}

	up(&s_jpu_sem); // TODO
	return 0;
}

static int jpu_fasync(int fd, struct file *filp, int mode)
{
	#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_jpu_device *jdev = (struct cvi_jpu_device *)pCviJpuDevice;
	#else
	struct cvi_jpu_device *jdev = filp->private_data;
	#endif

	return fasync_helper(fd, filp, mode, &jdev->async_queue);
}

static int jpu_map_to_register(struct file *filp, struct vm_area_struct *vm)
{
	unsigned long pfn;
	#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_jpu_device *jdev = (struct cvi_jpu_device *)pCviJpuDevice;
	#else
	struct cvi_jpu_device *jdev = filp->private_data;
	#endif

	vm->vm_flags |= VM_IO | VM_RESERVED;
	vm->vm_page_prot = pgprot_noncached(vm->vm_page_prot);
	pfn = jdev->jpu_register.phys_addr >> PAGE_SHIFT;

	return remap_pfn_range(vm, vm->vm_start, pfn, vm->vm_end - vm->vm_start,
			       vm->vm_page_prot) ?
		       -EAGAIN :
		       0;
}

static int jpu_map_to_physical_memory(struct file *filp,
				      struct vm_area_struct *vm)
{
	vm->vm_flags |= VM_IO | VM_RESERVED;
	vm->vm_page_prot = pgprot_writecombine(vm->vm_page_prot);

	return remap_pfn_range(vm, vm->vm_start, vm->vm_pgoff,
			       vm->vm_end - vm->vm_start, vm->vm_page_prot) ?
		       -EAGAIN :
		       0;
}

static int jpu_map_to_instance_pool_memory(struct file *filp,
					   struct vm_area_struct *vm)
{
#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
	int ret;
	#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_jpu_device *jdev = (struct cvi_jpu_device *)pCviJpuDevice;
	#else
	struct cvi_jpu_device *jdev = filp->private_data;
	#endif
	long length = vm->vm_end - vm->vm_start;
	unsigned long start = vm->vm_start;
	char *vmalloc_area_ptr = (char *)jdev->jpu_instance_pool.base;
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

	return remap_pfn_range(vm, vm->vm_start, vm->vm_pgoff,
			       vm->vm_end - vm->vm_start, vm->vm_page_prot) ?
		       -EAGAIN :
		       0;
#endif
}

/*!
 * @brief memory map interface for jpu file operation
 * @return  0 on success or negative error code on error
 */
static int jpu_mmap(struct file *filp, struct vm_area_struct *vm)
{
	#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_jpu_device *jdev = (struct cvi_jpu_device *)pCviJpuDevice;
	#else
	struct cvi_jpu_device *jdev = filp->private_data;
	#endif

#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
	if (vm->vm_pgoff == 0)
		return jpu_map_to_instance_pool_memory(filp, vm);

	if (vm->vm_pgoff == (jdev->jpu_register.phys_addr >> PAGE_SHIFT))
		return jpu_map_to_register(filp, vm);
#else
	if (vm->vm_pgoff == 0)
		return jpu_map_to_register(filp, vm);

	if (vm->vm_pgoff == (jdev->jpu_instance_pool.phys_addr >> PAGE_SHIFT))
		return jpu_map_to_instance_pool_memory(filp, vm);
#endif

	return jpu_map_to_physical_memory(filp, vm);
}

const struct file_operations jpu_fops = {
	.owner = THIS_MODULE,
	.open = jpu_open,
	.unlocked_ioctl = jpu_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = jpu_ioctl,
#endif
	.release = jpu_release,
	.fasync = jpu_fasync,
	.mmap = jpu_mmap,
};

static int jpu_probe(struct platform_device *pdev)
{
	int err = 0;
	const struct of_device_id *match;
	struct device *dev = &pdev->dev;
	struct cvi_jpu_device *jdev;

	struct resource *res = NULL;

	jdev = devm_kzalloc(&pdev->dev, sizeof(*jdev), GFP_KERNEL);
	if (!jdev)
		return -ENOMEM;

	memset(jdev, 0, sizeof(*jdev));
	#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	pCviJpuDevice = jdev;
	#endif

	jdev->dev = dev;

	match = of_match_device(cvi_jpu_match_table, &pdev->dev);
	if (!match)
		return -EINVAL;

	jdev->pdata = match->data;

	if (pdev)
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (res) {
		unsigned long size = resource_size(res);

		jdev->jpu_register.phys_addr = res->start;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		jdev->jpu_register.virt_addr =
			(__u8 *)ioremap(res->start, size);
#else
		jdev->jpu_register.virt_addr =
			(__u8 *)ioremap_nocache(res->start, size);
#endif
		jdev->jpu_register.size = size;
		JPU_DBG_INFO("jpu reg pa = 0x%llx, va = %p, size = %u\n",
			jdev->jpu_register.phys_addr, jdev->jpu_register.virt_addr,
			jdev->jpu_register.size);
	} else {
		pr_err("Unable to get base register\n");
		return -EINVAL;
	}

	if (pdev)
		res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	// if platform driver is implemented
	if (res) {
		unsigned long size = resource_size(res);

		jdev->jpu_control_register.phys_addr = res->start;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		jdev->jpu_control_register.virt_addr =
			(__u8 *)ioremap(res->start, size);
#else
		jdev->jpu_control_register.virt_addr =
			(__u8 *)ioremap_nocache(res->start, size);
#endif
		jdev->jpu_control_register.size = size;
		pr_info("jpu ctrl reg pa = 0x%llx, va = %p, size = %u\n",
			jdev->jpu_control_register.phys_addr,
			jdev->jpu_control_register.virt_addr,
			jdev->jpu_control_register.size);
	} else {
		//TODO get res from kernel
		jdev->jpu_control_register.phys_addr = JPU_CONTROL_REG_ADDR;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		jdev->jpu_control_register.virt_addr =
			(__u8 *)ioremap(
				jdev->jpu_control_register.phys_addr,
				JPU_CONTROL_REG_SIZE);
#else
		jdev->jpu_control_register.virt_addr =
			(__u8 *)ioremap_nocache(
				jdev->jpu_control_register.phys_addr,
				JPU_CONTROL_REG_SIZE);
#endif
		jdev->jpu_control_register.size = JPU_CONTROL_REG_SIZE;
		JPU_DBG_INFO("jpu ctrl reg pa = 0x%llx, va = %p, size = %u\n",
			jdev->jpu_control_register.phys_addr,
			jdev->jpu_control_register.virt_addr,
			jdev->jpu_control_register.size);
	}

	err = cvi_jpu_register_cdev(jdev);

	if (err < 0) {
		pr_err("jpu_register_cdev\n");
		goto ERROR_PROBE_DEVICE;
	}

	if (jdev->pdata->quirks & JPU_QUIRK_SUPPORT_CLOCK_CONTROL) {
		if (jdev->pdata->ops->clk_get)
			jdev->pdata->ops->clk_get(jdev);  // jpu_clk_get
	}

	if (pdev)
		res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	// if platform driver is implemented
	if (res) {
		jdev->jpu_irq = res->start;
		JPU_DBG_INFO("jpu irq number irq = %d\n", jdev->jpu_irq);
	} else {
		JPU_DBG_ERR("jpu irq number irq = %d\n", jdev->jpu_irq);
	}

	err = request_irq(jdev->jpu_irq, jpu_irq_handler, 0, "JPU_CODEC_IRQ", jdev);
	if (err) {
		pr_err("[JPUDRV] :  fail to register interrupt handler\n");
		goto ERROR_PROBE_DEVICE;
	}

#ifndef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
	if (jpu_allocate_memory(jdev, pdev) < 0) {
		pr_err("[JPUDRV] :  fail to remap jpu memory\n");
		goto ERROR_PROBE_DEVICE;
	}

	if (jmem_init(&jdev->jmem, jdev->video_memory.phys_addr, jdev->video_memory.size) <
	    0) {
		pr_err("[JPUDRV] :  fail to init vmem system\n");
		goto ERROR_PROBE_DEVICE;
	}
#endif

	jdev->jpu_instance_pool.base = 0;

	if (jdev->pdata->quirks & JPU_QUIRK_SUPPORT_SWITCH_TO_PLL && jdev->pdata->ops->config_pll) {
		jdev->pdata->ops->config_pll(jdev); //cv1835_config_pll
	}

	init_waitqueue_head(&jdev->interrupt_wait_q);

	spin_lock_init(&jdev->jpu_lock);

	INIT_LIST_HEAD(&jdev->s_jbp_head);
	INIT_LIST_HEAD(&jdev->s_jpu_inst_list_head);

	platform_set_drvdata(pdev, jdev);

	return 0;

ERROR_PROBE_DEVICE:

	if (jdev->s_jpu_major)
		unregister_chrdev_region(jdev->cdev_id, 1);

	if (jdev->jpu_register.virt_addr) {
		iounmap((void *)jdev->jpu_register.virt_addr);
		jdev->jpu_register.virt_addr = 0x00;
	}

	if (jdev->jpu_control_register.virt_addr) {
		iounmap((void *)jdev->jpu_control_register.virt_addr);
		jdev->jpu_control_register.virt_addr = 0x00;
	}

	return err;
}

static int cvi_jpu_register_cdev(struct cvi_jpu_device *jdev)
{
	int err = 0;

	jdev->jpu_class = class_create(THIS_MODULE, JPU_CLASS_NAME);
	if (IS_ERR(jdev->jpu_class)) {
		pr_err("create class failed\n");
		return PTR_ERR(jdev->jpu_class);
	}

	/* get the major number of the character device */
	if ((alloc_chrdev_region(&jdev->cdev_id, 0, 1, JPU_DEV_NAME)) < 0) {
		err = -EBUSY;
		pr_err("could not allocate major number\n");
		return err;
	}
	jdev->s_jpu_major = MAJOR(jdev->cdev_id);

	/* initialize the device structure and register the device with the kernel */
	cdev_init(&jdev->cdev, &jpu_fops);
	jdev->cdev.owner = THIS_MODULE;

	if ((cdev_add(&jdev->cdev, jdev->cdev_id, 1)) < 0) {
		err = -EBUSY;
		pr_err("could not allocate chrdev\n");
		return err;
	}

	device_create(jdev->jpu_class, jdev->dev, jdev->cdev_id, NULL, "%s",
		      JPU_DEV_NAME);

	return err;
}

static int jpu_allocate_memory(struct cvi_jpu_device *jdev, struct platform_device *pdev)
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
			pr_err("[JPUDRV]: cannot acquire memory-region\n");
			return -1;
		}
	} else {
		pr_err("[JPUDRV]: cannot find the node, memory-region\n");
		return -1;
	}

	JPU_DBG_INFO("pool name = %s, size = 0x%llx, base = 0x%llx\n",
	       prmem->name, prmem->size, prmem->base);

	jdev->video_memory.phys_addr = (unsigned long)prmem->base;
	jdev->video_memory.size = (unsigned int)prmem->size;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	jdev->video_memory.base = (unsigned long)devm_ioremap(
		&pdev->dev, jdev->video_memory.phys_addr, jdev->video_memory.size);
#else
	jdev->video_memory.base = (unsigned long)devm_ioremap_nocache(
		&pdev->dev, jdev->video_memory.phys_addr, jdev->video_memory.size);
#endif

	if (!jdev->video_memory.base) {
		pr_err("[JPUDRV] ioremap fail!\n");
		pr_err("jdev->video_memory.base = 0x%llx\n", jdev->video_memory.base);
		return -1;
	}

	JPU_DBG_INFO("reserved mem pa = 0x%llx, base = 0x%llx, size = 0x%x\n",
		jdev->video_memory.phys_addr, jdev->video_memory.base,
		jdev->video_memory.size);
	JPU_DBG_INFO("success to probe jpu\n");

	return 0;
}

static int jpu_remove(struct platform_device *pdev)
{
	struct cvi_jpu_device *jdev = platform_get_drvdata(pdev);

	pr_info("jpu_remove\n");

	if (jdev->jpu_instance_pool.base) {
#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
		vfree((const void *)jdev->jpu_instance_pool.base);
#else
		jpu_free_dma_buffer(jdev, &jdev->jpu_instance_pool);
#endif
		jdev->jpu_instance_pool.base = 0;
	}
#ifndef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
	if (jdev->video_memory.base) {
		jdev->video_memory.base = 0;
		jmem_exit(&jdev->jmem);
	}
#endif

	cvi_jpu_unregister_cdev(pdev);

	if (jdev->jpu_irq)
		free_irq(jdev->jpu_irq, jdev);

	if (jdev->jpu_register.virt_addr) {
		iounmap((void *)jdev->jpu_register.virt_addr);
		jdev->jpu_register.virt_addr = 0x00;
	}

	if (jdev->jpu_control_register.virt_addr) {
		iounmap((void *)jdev->jpu_control_register.virt_addr);
		jdev->jpu_control_register.virt_addr = 0x00;
	}

	return 0;
}

static void cvi_jpu_unregister_cdev(struct platform_device *pdev)
{
	struct cvi_jpu_device *jdev = platform_get_drvdata(pdev);

	if (jdev->s_jpu_major > 0) {
		device_destroy(jdev->jpu_class, jdev->cdev_id);
		class_destroy(jdev->jpu_class);
		cdev_del(&jdev->cdev);
		unregister_chrdev_region(jdev->cdev_id, 1);
		jdev->s_jpu_major = 0;
	}
}

#ifdef CONFIG_PM
static int jpu_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct cvi_jpu_device *jdev = platform_get_drvdata(pdev);

	set_clock_enable(jdev, JPEG_CLK_DISABLE);

	return 0;
}

static int jpu_resume(struct platform_device *pdev)
{
	struct cvi_jpu_device *jdev = platform_get_drvdata(pdev);

	set_clock_enable(jdev, JPEG_CLK_ENABLE);

	return 0;
}
#else
#define jpu_suspend NULL
#define jpu_resume NULL
#endif /* !CONFIG_PM */

static struct platform_driver jpu_driver = {
	.driver   = {
		.owner = THIS_MODULE,
		.name = JPU_PLATFORM_DEVICE_NAME,
		.of_match_table = cvi_jpu_match_table,
	},
	.probe    = jpu_probe,
	.remove   = jpu_remove,
#ifdef CONFIG_PM
	.suspend  = jpu_suspend,
	.resume   = jpu_resume,
#endif /* !CONFIG_PM */
};

static int __init jpu_init(void)
{
	int res = 0;

	res = platform_driver_register(&jpu_driver);

	pr_info("end jpu_init result = 0x%x\n", res);
	return res;
}

static void __exit jpu_exit(void)
{
	pr_info("jpu_exit\n");
	platform_driver_unregister(&jpu_driver);
}

MODULE_AUTHOR("Cvitek");
MODULE_DESCRIPTION("JPEG linux driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(VERSION);

module_init(jpu_init);
module_exit(jpu_exit);
