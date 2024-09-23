
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <asm/io.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/syscore_ops.h>
#include <linux/interrupt.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "cvi_sys.h"
#include "mailbox.h"
#include "ipcm_pool.h"
#include "linux/ipcm_linux.h"
#include "ipcm_port_common.h"
#include "ipcm_drv_common.h"
#include "ipcm.h"

typedef struct _ipcm_drv_port_common_ctx {
    POOLHANDLE pool_shm;
    struct mutex data_lock[IPCM_DATA_SPIN_MAX];
	u32 pool_paddr;
	u32 pool_size;
	u32 rtos_paddr;
	u32 rtos_size;
	void *__iomem rtos_stat_base;
} ipcm_drv_port_common_ctx;

static ipcm_drv_port_common_ctx _port_ctx = {};

static s32 pool_get_param_from_sram(void)
{
	void *__iomem tpu_sram_ipcm_base = ioremap(TPU_SRAM_IPCM_BASE , 0x40);
	if(tpu_sram_ipcm_base == NULL)
	{
		ipcm_err("ioremap %x failed!\n", TPU_SRAM_IPCM_BASE);
		return -1;
	}
	else
	{
		ipcm_info("ipcm_freertos_addr = 0x%x,(0x%x)\n",ioread32(tpu_sram_ipcm_base + 0x2C), IPCM_FREERTOS_ADDR);
		ipcm_info("ipcm_freertos_size= 0x%x,(0x%x)\n",ioread32(tpu_sram_ipcm_base + 0x34), IPCM_FREERTOS_SIZE);
		ipcm_info("ipcm_pool_addr = 0x%x,(0x%x)\n",ioread32((tpu_sram_ipcm_base + 0x38)), IPCM_POOL_ADDR);
		ipcm_info("ipcm_pool_size = 0x%x,(0x%x)\n",ioread32((tpu_sram_ipcm_base +0x3C)), IPCM_POOL_SIZE);
		_port_ctx.rtos_paddr = (u32)ioread32(tpu_sram_ipcm_base + 0x2C);
		_port_ctx.rtos_size = (u32)ioread32(tpu_sram_ipcm_base + 0x34);
		_port_ctx.pool_paddr = (u32)ioread32(tpu_sram_ipcm_base + 0x38);
		_port_ctx.pool_size = (u32)ioread32(tpu_sram_ipcm_base + 0x3C);
		iounmap(tpu_sram_ipcm_base);
	}
	return 0;
}

static s32 dev_open(struct inode *inode, struct file *file)
{
	return 0;
}

static s32 dev_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t dev_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	int ret = 0;
	MsgData msg = {};

	if (count != sizeof(MsgData)) {
		ipcm_err("count %zu unexcept.\n", count);
		return -EINVAL;
	}

	ret = copy_from_user(&msg, buf, count);
	if (ret < 0) {
		ipcm_err("copy from user err");
		return -EFAULT;
	}

	ipcm_send_msg(&msg);

	return 0;
}

static long dev_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	int ret = -1;
	IPCM_FLUSH_PARAM flush_param;
	u32 shm_data_pos;

	switch (cmd) {
	case IPCM_IOC_GET_DATA:
		ret = ipcm_get_buff_to_pos(arg);
		break;
	case IPCM_IOC_RLS_DATA:
		ret = ipcm_release_buff_by_pos(arg);
		break;
	case IPCM_IOC_FLUSH_DATA:
		if (copy_from_user((void *)&flush_param, (void *)arg,
					sizeof(IPCM_FLUSH_PARAM))) {
			ipcm_err("copy from user err\n");
			return -EFAULT;
		}
		ret = ipcm_flush_data(ipcm_get_data_by_pos(flush_param.data_pos), flush_param.len);
		break;
	case IPCM_IOC_INV_DATA:
		if (copy_from_user((void *)&flush_param, (void *)arg,
					sizeof(IPCM_FLUSH_PARAM))) {
			ipcm_err("copy from user err\n");
			return -EFAULT;
		}
		ret = ipcm_inv_data(ipcm_get_data_by_pos(flush_param.data_pos), flush_param.len);
		break;
	case IPCM_IOC_LOCK:
		ret = ipcm_data_lock(arg);
		break;
	case IPCM_IOC_UNLOCK:
		ret = ipcm_data_unlock(arg);
		break;
	case IPCM_IOC_POOL_RESET:
		ret = ipcm_pool_reset();
		break;
	case IPCM_GET_POOL_ADDR:
		ret = copy_to_user((void *)arg , &_port_ctx.pool_paddr , sizeof(_port_ctx.pool_paddr));
		break;

	case IPCM_GET_POOL_SIZE:
		ret = copy_to_user((void *)arg , &_port_ctx.pool_size , sizeof(_port_ctx.pool_size));
		break;

	case IPCM_GET_FREERTOS_ADDR:
		ret = copy_to_user((void *)arg , &_port_ctx.rtos_paddr , sizeof(_port_ctx.rtos_paddr));
		break;

	case IPCM_GET_FREERTOS_SIZE:
		ret = copy_to_user((void *)arg , &_port_ctx.rtos_size , sizeof(_port_ctx.rtos_size));
		break;
	case IPCM_GET_SHM_DATA_POS:
		shm_data_pos = pool_get_shm_data_pos(_port_ctx.pool_shm);
		ret = copy_to_user((void *)arg , &shm_data_pos , sizeof(shm_data_pos));
		break;
	default:
		ret = -1;
		ipcm_err("cmd(%d) not support.\n", cmd);
		break;
	}
	return ret;
}

s32 ipcm_port_common_init(void)
{
    int i = 0;
	int ret = 0;

	ret = pool_get_param_from_sram();
	if (ret) {
		_port_ctx.pool_paddr = IPCM_POOL_ADDR;
		_port_ctx.pool_size = IPCM_POOL_SIZE;
		_port_ctx.rtos_paddr = IPCM_FREERTOS_ADDR;
		_port_ctx.rtos_size =  IPCM_FREERTOS_SIZE;
	}

	ret = ipcm_init(_port_ctx.pool_paddr, _port_ctx.pool_size);
	if (ret) {
		ipcm_err("ipcm_init failed ret:%d.\n", ret);
		return ret;
	}

	ipcm_debug("paddr.size %x.%x %x.%x\n", _port_ctx.pool_paddr, _port_ctx.pool_size,
		_port_ctx.rtos_paddr, _port_ctx.rtos_size);
	ipcm_sys_cache_invalidate(_port_ctx.pool_paddr, _port_ctx.pool_size);
	_port_ctx.pool_shm = memremap((unsigned long)_port_ctx.pool_paddr, _port_ctx.pool_size, MEMREMAP_WB);
	ipcm_debug("%s shm(%lx)\n", __func__, (unsigned long)_port_ctx.pool_shm);
	pool_print_info(_port_ctx.pool_shm, NULL);

    for (i=0; i<IPCM_DATA_SPIN_MAX; i++) {
        mutex_init(&_port_ctx.data_lock[i]);
    }

    pool_reset(_port_ctx.pool_shm);

    return 0;
}

s32 ipcm_port_common_uninit(void)
{
	if (_port_ctx.pool_shm)
		memunmap(_port_ctx.pool_shm);
    return 0;
}

u32 ipcm_get_buff_to_pos(u32 size)
{
    return pool_get_buff_to_pos(_port_ctx.pool_shm, size);
}

s32 ipcm_release_buff_by_pos(u32 pos)
{
    return pool_release_buff_by_pos(_port_ctx.pool_shm, pos);
}

s32 ipcm_inv_data(void *data, u32 size)
{
    return ipcm_sys_cache_invalidate(_port_ctx.pool_paddr + (data-_port_ctx.pool_shm), size);
}

s32 ipcm_flush_data(void *data, u32 size)
{
    return ipcm_sys_cache_flush(_port_ctx.pool_paddr +(data-_port_ctx.pool_shm), size);
}

s32 ipcm_data_lock(u8 lock_id)
{
	s32 ret = 0;
    u8 id = lock_id%IPCM_DATA_SPIN_MAX;

	mutex_lock(&_port_ctx.data_lock[id]);
	ret = ipcm_data_spin_lock(id);
	if (ret)
		mutex_unlock(&_port_ctx.data_lock[id]);

	return ret;
}

s32 ipcm_data_unlock(u8 lock_id)
{
	s32 ret = 0;
    u8 id = lock_id%IPCM_DATA_SPIN_MAX;

	ret = ipcm_data_spin_unlock(id);
	if (!ret)
		mutex_unlock(&_port_ctx.data_lock[id]);
	return ret;
}

void *ipcm_get_buff(u32 size)
{
    return pool_get_buff(_port_ctx.pool_shm, size);
}
EXPORT_SYMBOL_GPL(ipcm_get_buff);

s32 ipcm_release_buff(void *data)
{
    return pool_release_buff(_port_ctx.pool_shm, data);
}

s32 ipcm_data_packed(void *data, u32 len, MsgData *msg)
{
	if (data == NULL || msg == NULL) {
		ipcm_err("data or msg is null.\n");
		return -EFAULT;
	}
	if ((data < _port_ctx.pool_shm) || (data > (_port_ctx.pool_shm + _port_ctx.pool_size))) {
		ipcm_err("data(%lx) should from share memory(%lx-%lx).\n", (unsigned long)data,
			(unsigned long)_port_ctx.pool_shm, (unsigned long)(_port_ctx.pool_shm + _port_ctx.pool_size));
		return -EINVAL;
	}
	msg->msg_param.msg_ptr.data_pos = pool_get_data_pos(_port_ctx.pool_shm, data);
	msg->msg_param.msg_ptr.remaining_rd_len = len;
	ipcm_flush_data(data, len);

    return 0;
}
EXPORT_SYMBOL_GPL(ipcm_data_packed);

void *ipcm_get_data_by_pos(u32 data_pos)
{
    return pool_get_data_by_pos(_port_ctx.pool_shm, data_pos);
}

s32 ipcm_pool_reset(void)
{
    return pool_reset(_port_ctx.pool_shm);
}

static const struct file_operations dev_fops = {
	.owner          = THIS_MODULE,
	.open           = dev_open,
	.release        = dev_release,
	.write          = dev_write,
	.unlocked_ioctl = dev_ioctl,
};

static struct miscdevice ipcm_common_dev = {
	.minor  = MISC_DYNAMIC_MINOR,
	.fops   = &dev_fops,
	.name   = "ipcm_core"
};

int ipcm_common_register_dev(void)
{
	int rc;

	rc = misc_register(&ipcm_common_dev);
	if (rc) {
		ipcm_err("cvi_base: failed to register misc device.\n");
		return rc;
	}

	return 0;
}

void ipcm_common_deregister_dev(void)
{
	misc_deregister(&ipcm_common_dev);
}

s32 ipcm_common_send_msg(MsgData *msg)
{
	if (msg == NULL) {
		ipcm_err("data is null.\n");
		return -1;
	}

	return ipcm_send_msg(msg);
}

int ipcm_set_snd_cpu(int cpu_id)
{
	return mailbox_set_snd_cpu(cpu_id);
}

s32 ipcm_set_rtos_boot_bit(RTOS_BOOT_STATUS_E stage, u8 stat)
{
	UNUSED(stage);
	UNUSED(stat);
    ipcm_warning("do not support set rtos boot status by linux.\n");
    return -EOPNOTSUPP;
}

s32 ipcm_get_rtos_boot_status(u32 *stat)
{
	void *__iomem rtos_stat_base = NULL;

	if (NULL == stat) {
		ipcm_err("stat is null.\n");
		return -EINVAL;
	}

	rtos_stat_base = ioremap(RTOS_BOOT_STATUS_REG , 0x4);
	if(rtos_stat_base == NULL) {
		ipcm_err("ioremap %x failed!\n", RTOS_BOOT_STATUS_REG);
		return -1;
	} else {
		*stat = ioread32(rtos_stat_base);
		ipcm_info("rtos status = 0x%x\n", *stat);
		iounmap(rtos_stat_base);
	}

	return 0;
}
