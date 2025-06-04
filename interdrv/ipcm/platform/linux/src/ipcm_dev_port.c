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

#include "cvi_mailbox.h"
#include "ipcm_pool.h"
#include "linux/ipcm_linux.h"
#include "ipcm_port.h"
#include "ipcm.h"
#include "ipcm_message.h"
#include "ipcm_custom.h"

// Open device file
static s32 dev_open(struct inode *inode, struct file *file)
{
	return 0;
}

// Release device file
static s32 dev_release(struct inode *inode, struct file *file)
{
	return 0;
}

// Write message to device
static ssize_t dev_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_offset)
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

// Read message from custom device
static ssize_t dev_cust_read(struct file *file, char __user *buf,
		size_t count, loff_t *f_offset)
{
	MsgData *msg;
	int read = 0;

	if (count != sizeof(MsgData)) {
		ipcm_err("count %zu unexcept.\n", count);
		return -EINVAL;
	}

	msg = ipcm_cust_recv_msg();
	if (!msg) {
		return 0;
	}

	read = copy_to_user(buf, msg, count);
	if (read < 0)
		ipcm_err("copy to user err");

	// if success read is 0
	if (read == 0)
		read = count;

	return read;
}

static ssize_t dev_msg_read(struct file *file, char __user *buf,
		size_t count, loff_t *f_offset)
{
	MsgData *msg;
	int read = 0;

	if (count != sizeof(MsgData)) {
		ipcm_err("count %zu unexcept.\n", count);
		return -EINVAL;
	}

	msg = ipcm_msg_recv_msg(0);
	if (!msg) {
		return 0;
	}

	read = copy_to_user(buf, msg, count);
	if (read < 0)
		ipcm_err("copy to user err");

	// if success read is 0
	if (read == 0)
		read = count;

	return read;
}

extern u32 ipcm_linux_drv_cust_poll(struct file *file, struct poll_table_struct *table);

// Poll for custom device events
static u32 dev_cust_poll(struct file *file, struct poll_table_struct *table)
{
	return ipcm_linux_drv_cust_poll(file, table);
}

static long dev_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	int ret = -1;
	IPCM_FLUSH_PARAM flush_param;
	IPCM_PADDR_INFO_S paddr_info;

	switch (cmd) {
	case IPCM_IOC_FLUSH_DATA:
		if (copy_from_user((void *)&flush_param, (void *)arg,
					sizeof(IPCM_FLUSH_PARAM))) {
			ipcm_err("copy from user err\n");
			return -EFAULT;
		}
		ret = ipcm_port_flush_data_by_offset(flush_param.data_pos, flush_param.len);
		break;
	case IPCM_IOC_INV_DATA:
		if (copy_from_user((void *)&flush_param, (void *)arg,
					sizeof(IPCM_FLUSH_PARAM))) {
			ipcm_err("copy from user err\n");
			return -EFAULT;
		}
		ret = ipcm_port_inv_data_by_offset(flush_param.data_pos, flush_param.len);
		break;
	case IPCM_IOC_LOCK:
		ret = ipcm_port_data_lock(arg);
		break;
	case IPCM_IOC_UNLOCK:
		ret = ipcm_port_data_unlock(arg);
		break;
	case IPCM_IOC_GET_POOL_ADDR:
		ipcm_port_get_shm_info(&paddr_info.paddr, &paddr_info.size);
		ret = copy_to_user((void *)arg , &paddr_info , sizeof(paddr_info));
		break;

	case IPCM_IOC_GET_RTOS_ADDR:
		ipcm_port_get_rtos_info(&paddr_info.paddr, &paddr_info.size);
		ret = copy_to_user((void *)arg , &paddr_info , sizeof(paddr_info));
		break;

	case IPCM_IOC_GET_LOG_ADDR:
		ipcm_port_get_log_info(&paddr_info.paddr, &paddr_info.size);
		ret = copy_to_user((void *)arg , &paddr_info , sizeof(paddr_info));
		break;

	default:
		ret = -1;
		ipcm_err("cmd(%d) not support.\n", cmd);
		break;
	}
	return ret;
}

static long dev_cust_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	int ret = -1;
	u32 pool_offset;
	switch (cmd) {
	case IPCM_IOC_CUST_GET_POOL_OFF:
		pool_offset = ipcm_cust_get_pool_offset();
		ret = copy_to_user((void *)arg , &pool_offset , sizeof(pool_offset));
		break;
	case IPCM_IOC_CUST_GET_DATA:
		ret = ipcm_cust_get_buff_offset(arg);
		break;
	case IPCM_IOC_CUST_RLS_DATA:
		ret = ipcm_cust_release_buff_by_offset(arg);
		break;
	case IPCM_IOC_CUST_POOL_RESET:
		ret = ipcm_cust_pool_reset();
		break;
	default:
		ret = -1;
		ipcm_err("cmd(%d) not support.\n", cmd);
		break;
	}
	return ret;
}

static const struct file_operations dev_port_fops = {
	.owner          = THIS_MODULE,
	.open           = dev_open,
	.release        = dev_release,
	.write          = dev_write,
	.unlocked_ioctl = dev_ioctl,
};

static struct miscdevice ipcm_port_dev = {
	.minor  = MISC_DYNAMIC_MINOR,
	.fops   = &dev_port_fops,
	.name   = "ipcm_port"
};

int ipcm_register_dev(void)
{
	int rc;

	rc = misc_register(&ipcm_port_dev);
	if (rc) {
		ipcm_err("cvi_base: failed to register misc device.\n");
		return rc;
	}

	return 0;
}

void ipcm_deregister_dev(void)
{
	misc_deregister(&ipcm_port_dev);
}

// Custom device file operations
static const struct file_operations dev_cust_fops = {
	.owner          = THIS_MODULE,
	.open           = dev_open,
	.release        = dev_release,
	.write          = dev_write,
	.unlocked_ioctl = dev_cust_ioctl,
	.read           = dev_cust_read,
	.poll           = dev_cust_poll,
};

// Custom device registration structure
static struct miscdevice ipcm_cust_dev = {
	.minor  = MISC_DYNAMIC_MINOR,
	.fops   = &dev_cust_fops,
	.name   = "ipcm_cust"
};

// Register custom device
int ipcm_cust_register_dev(void)
{
	int ret;

	ret = misc_register(&ipcm_cust_dev);
	if (ret) {
		ipcm_err("module register failed");
	}

	return ret;
}

// Deregister custom device
void ipcm_cust_deregister_dev(void)
{
	misc_deregister(&ipcm_cust_dev);
}

// Message device file operations
static const struct file_operations dev_msg_fops = {
	.owner          = THIS_MODULE,
	.open           = dev_open,
	.release        = dev_release,
	.write          = dev_write,
	.read           = dev_msg_read,
};

// Message device registration structure
static struct miscdevice ipcm_msg_dev = {
	.minor  = MISC_DYNAMIC_MINOR,
	.fops   = &dev_msg_fops,
	.name   = "ipcm_msg"
};

// Register message device
int ipcm_msg_register_dev(void)
{
	int rc;

	rc = misc_register(&ipcm_msg_dev);
	if (rc) {
		ipcm_err("cvi_base: failed to register misc device.\n");
		return rc;
	}

	return 0;
}

// Deregister message device
void ipcm_msg_deregister_dev(void)
{
	misc_deregister(&ipcm_msg_dev);
}
