
#include <asm/io.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/syscore_ops.h>
#include <linux/io.h>
#include <linux/interrupt.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "cvi_sys.h"

#include "ipcm_port_msg.h"
#include "ipcm.h"

#include "linux/ipcm_linux.h"

static wait_queue_head_t _msg_wait;
static MsgQueue *_m_queue = NULL;

static s32 _msg_recv_handle(u8 port_id, void *data)
{
	wake_up_interruptible(&_msg_wait);
	return 0;
}

s32 ipcm_drv_msg_init(void)
{
	_m_queue = ipcm_alloc(sizeof(MsgQueue) + (sizeof(MsgData) * MSG_QUEUE_LEN));
	if (_m_queue == NULL) {
		ipcm_err("port queue alloc failed.\n");
		return -ENOMEM;
	}
	queue_init(_m_queue, sizeof(MsgData), MSG_QUEUE_LEN);
	ipcm_debug("queue addr:%lx queue data:%lx\n",
		(unsigned long)_m_queue, (unsigned long)_m_queue);

	return ipcm_port_init(PORT_MSG, _msg_recv_handle);
}

s32 ipcm_drv_msg_uninit(void)
{
	if (_m_queue) {
		queue_uninit(_m_queue);
		ipcm_free(_m_queue);
		_m_queue = NULL;
	}

    return ipcm_port_uninit(PORT_MSG);
}

static s32 dev_open(struct inode *inode, struct file *file)
{
	ipcm_debug("%s\n", __func__);

	init_waitqueue_head(&_msg_wait);

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
		ipcm_err("count %ld unexcept.\n", count);
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

static ssize_t dev_read(struct file *file, char __user *buf,
		size_t count, loff_t *f_pos)
{
	MsgData *msg;
	int read = 0;

	if (count != sizeof(MsgData)) {
		ipcm_err("count %ld unexcept.\n", count);
		return -EINVAL;
	}

	if (queue_is_empty(_m_queue)) {
		ipcm_err("queue is empty.\n");
		return -EQEMPTY;
	}

	msg = queue_get_no_cpy(_m_queue);
	read = copy_to_user(buf, msg, count);
	if (read < 0)
		ipcm_err("copy to user err");

	// if success read is 0
	if (read == 0)
		read = count;

	return read;
}

u32 dev_poll(struct file *file, struct poll_table_struct *table)
{
	poll_wait(file, &_msg_wait, table);
	if (!queue_is_empty(_m_queue))
		return POLLIN | POLLWRNORM;
	return 0;
}

static const struct file_operations dev_fops = {
	.owner          = THIS_MODULE,
	.open           = dev_open,
	.release        = dev_release,
	.write          = dev_write,
	.read           = dev_read,
	.poll           = dev_poll,
};

static struct miscdevice ipcm_msg_dev = {
	.minor  = MISC_DYNAMIC_MINOR,
	.fops   = &dev_fops,
	.name   = "ipcm"
};

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

void ipcm_msg_deregister_dev(void)
{
	misc_deregister(&ipcm_msg_dev);
}
