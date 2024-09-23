
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
#include <linux/kthread.h>
#include <linux/semaphore.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "ipcm_anonymous.h"
#include "ipcm_port_anon.h"
#include "ipcm.h"

#include "linux/ipcm_linux.h"

typedef struct _ipcm_anon_ctx {
	wait_queue_head_t anon_wait;
	struct semaphore sem;
	int kernel_thread_run;
	struct task_struct *process_thread;
	ANON_MSGPROC_FN kernel_handler;
	void *handler_data;
	MsgQueue *queue;
	MsgQueue *queue_kernel;
} ipcm_anon_ctx;

static ipcm_anon_ctx _anon_ctx = {};

static s32 _anon_recv_handle(u8 port_id, void *data)
{
	if (port_id < IPCM_ANON_KER_PORT_ST) {
		if (_anon_ctx.queue)
			queue_put(_anon_ctx.queue, data);
		wake_up_interruptible(&_anon_ctx.anon_wait);
	} else if (port_id < IPCM_ANON_KER_PORT_ST) {
		if (_anon_ctx.queue_kernel)
			queue_put(_anon_ctx.queue_kernel, data);
		up(&_anon_ctx.sem);
	} else {
		ipcm_err("port_id(%d max:%d) is invalid.\n", port_id, IPCM_ANON_PORT_MAX-1);
		return -EINVAL;
	}
	return 0;
}

static s32 _anon_kernel_process(void *data)
{
	s32 ret = 0;
	s32 kernel_handled = 0;
	u8 port_type = 0;
	u8 port_id = 0;

	while (_anon_ctx.kernel_thread_run && !kthread_should_stop()) {
		ret = down_interruptible(&_anon_ctx.sem);
		ipcm_debug("down ret is %d\n", ret);
		if ((0 == ret) && _anon_ctx.kernel_thread_run) {
			MsgData tmsg;

			if (queue_is_empty(_anon_ctx.queue_kernel)) {
				ipcm_warning("anon kernel queue is empty.\n");
				continue;
			}
			ipcm_debug("msg not empty.\n");
			ret = queue_get(_anon_ctx.queue_kernel, &tmsg);
			if (ret) {
				ipcm_warning("anon kernel msg get fail.\n");
				continue;
			}
			ipcm_debug("anon read msg:%lx\n", *(unsigned long*)&tmsg);
            ret = ipcm_get_port_id(tmsg.grp_id, &port_type, &port_id);
            if (ret) {
                ipcm_err("ipcm get port type and id fail ret:%d.\n", ret);
                goto rls_pool;
            }

			if (_anon_ctx.kernel_handler) {
                ipcm_anon_msg_t anon_msg;
                anon_msg.port_id = port_id;
                anon_msg.msg_id = tmsg.msg_id;
                anon_msg.data_type = tmsg.func_type;
                if (tmsg.func_type == MSG_TYPE_SHM) {
                    anon_msg.data = ipcm_get_data_by_pos(tmsg.msg_param.msg_ptr.data_pos);
                    anon_msg.size = tmsg.msg_param.msg_ptr.remaining_rd_len;
					ipcm_inv_data(anon_msg.data, anon_msg.size);
                } else {
                    anon_msg.data = (void *)(unsigned long)tmsg.msg_param.param;
                    anon_msg.size = 4;
                }
				kernel_handled = _anon_ctx.kernel_handler(_anon_ctx.handler_data, &anon_msg);
			}

			if (0 == kernel_handled) {
				/** push msg to userspace
				 * 1. copy msg to userpase queue
				 * 2. wait up _anon_ctx.anon_wait: wake_up_interruptible(&_anon_ctx.anon_wait);
				 */
				queue_put(_anon_ctx.queue, &tmsg);
				wake_up_interruptible(&_anon_ctx.anon_wait);
			}
rls_pool:
            // if release pool buff
            if (tmsg.func_type == MSG_TYPE_SHM && 1 == kernel_handled) {
                ipcm_release_buff_by_pos(tmsg.msg_param.msg_ptr.data_pos);
            }
		}
	}
	return 0;
}

s32 ipcm_drv_anon_init(void)
{
	ipcm_debug("%s\n", __func__);

	init_waitqueue_head(&_anon_ctx.anon_wait);

	_anon_ctx.queue = ipcm_alloc(sizeof(MsgQueue) + (sizeof(MsgData) * MSG_QUEUE_LEN));
	if (_anon_ctx.queue == NULL) {
		ipcm_err("port queue alloc failed.\n");
		return -ENOMEM;
	}
	queue_init(_anon_ctx.queue, sizeof(MsgData), MSG_QUEUE_LEN);
	ipcm_debug("queue addr:%lx queue data:%lx\n",
		(unsigned long)_anon_ctx.queue, (unsigned long)_anon_ctx.queue->data);

	_anon_ctx.queue_kernel = ipcm_alloc(sizeof(MsgQueue) + (sizeof(MsgData) * MSG_QUEUE_LEN));
	if (_anon_ctx.queue_kernel == NULL) {
		ipcm_err("port queue alloc failed.\n");
		if (_anon_ctx.queue)
			ipcm_free(_anon_ctx.queue);
		return -ENOMEM;
	}
	queue_init(_anon_ctx.queue_kernel, sizeof(MsgData), MSG_QUEUE_LEN);
	ipcm_debug("queue addr:%lx queue data:%lx\n",
		(unsigned long)_anon_ctx.queue_kernel, (unsigned long)_anon_ctx.queue_kernel->data);

	ipcm_port_init(PORT_ANON, _anon_recv_handle);

	sema_init(&_anon_ctx.sem, 0);

	_anon_ctx.kernel_thread_run = 1;
	_anon_ctx.process_thread = kthread_run(_anon_kernel_process, NULL, "kernel anon process");

	return 0;
}

s32 ipcm_drv_anon_uninit(void)
{
	_anon_ctx.kernel_thread_run = 0;
	up(&_anon_ctx.sem);
	if (_anon_ctx.process_thread)
		kthread_stop(_anon_ctx.process_thread);

	if (_anon_ctx.queue) {
		queue_uninit(_anon_ctx.queue);
		ipcm_free(_anon_ctx.queue);
		_anon_ctx.queue = NULL;
	}

	if (_anon_ctx.queue_kernel) {
		queue_uninit(_anon_ctx.queue_kernel);
		ipcm_free(_anon_ctx.queue_kernel);
		_anon_ctx.queue_kernel = NULL;
	}

    ipcm_port_uninit(PORT_ANON);

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

	ipcm_debug("anon write msg:%lx\n", *(unsigned long*)&msg);
	ipcm_send_msg(&msg);

	return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf,
		size_t count, loff_t *f_pos)
{
	MsgData *msg;
	int read = 0;

	if (count != sizeof(MsgData)) {
		ipcm_err("count %zu unexcept.\n", count);
		return -EINVAL;
	}

	if (queue_is_empty(_anon_ctx.queue)) {
		ipcm_err("queue is empty.\n");
		return -EQEMPTY;
	}

	msg = queue_get_no_cpy(_anon_ctx.queue);
	ipcm_debug("anon read msg:%lx\n", *(unsigned long*)msg);
	read = copy_to_user(buf, msg, count);
	if (read < 0) {
		ipcm_err("copy to user err");
	}

	// if success read is 0;
	if (read == 0)
		read = count;

	return read;
}

static u32 dev_poll(struct file *file, struct poll_table_struct *table)
{
	poll_wait(file, &_anon_ctx.anon_wait, table);
	if (!queue_is_empty(_anon_ctx.queue))
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

static struct miscdevice ipcm_anon_dev = {
	.minor  = MISC_DYNAMIC_MINOR,
	.fops   = &dev_fops,
	.name   = "ipcm_anon"
};

int ipcm_anon_register_dev(void)
{
	int ret;

	ret = misc_register(&ipcm_anon_dev);
	if (ret) {
		ipcm_err("module register failed");
	}

	return ret;
}

void ipcm_anon_deregister_dev(void)
{
	misc_deregister(&ipcm_anon_dev);
}

s32 ipcm_anon_init(void)
{
	// nothing to do
	return 0;
}
EXPORT_SYMBOL_GPL(ipcm_anon_init);

s32 ipcm_anon_uninit(void)
{
	// nothing to do
	return 0;
}
EXPORT_SYMBOL_GPL(ipcm_anon_uninit);

s32 ipcm_anon_register_handle(ANON_MSGPROC_FN handler, void *data)
{
	if (handler) {
		_anon_ctx.kernel_handler = handler;
		_anon_ctx.handler_data = data;
		return 0;
	}
	ipcm_err("%s handler is null.\n", __func__);
	return -EINVAL;
}
EXPORT_SYMBOL_GPL(ipcm_anon_register_handle);

s32 ipcm_anon_deregister_handle(void)
{
	_anon_ctx.kernel_handler = NULL;
	_anon_ctx.handler_data = NULL;
	return 0;
}
EXPORT_SYMBOL_GPL(ipcm_anon_deregister_handle);

// send msg if msg len > 4; max msg length is limited by pool block (2048?)
s32 ipcm_anon_send_msg(u8 port_id, u8 msg_id, void *data, u32 len)
{
	MsgData stMsg = {};
	u8 grp_id = 0;
	s32 ret = 0;

	if (data == NULL) {
		ipcm_err("data is null.\n");
		return -EFAULT;
	}
	if (port_id >= IPCM_ANON_PORT_MAX) {
		ipcm_err("port_id(%d max:%d) is invalid.\n", port_id, IPCM_ANON_PORT_MAX-1);
		return -EINVAL;
	}

	ret = ipcm_get_grp_id(PORT_ANON, port_id, &grp_id);
	if (ret) {
		ipcm_err("ipcm get grp id fail ret:%d.\n", ret);
		return ret;
	}

	stMsg.grp_id = grp_id;
	stMsg.msg_id = msg_id;
	stMsg.func_type = MSG_TYPE_SHM;
	ret = ipcm_data_packed(data, len, &stMsg);
	if (ret) {
		ipcm_err("ipcm_data_packed failed ret:%d.\n", ret);
		return ret;
	}

	return ipcm_send_msg(&stMsg);
}
EXPORT_SYMBOL_GPL(ipcm_anon_send_msg);

// send param if msg len <= 4 or send 32 bits addr
s32 ipcm_anon_send_param(u8 port_id, u8 msg_id, u32 param)
{
	MsgData tmsg = {};
	u8 grp_id = 0;
	s32 ret = 0;

	if (port_id >= IPCM_ANON_PORT_MAX) {
		ipcm_err("port_id(%u) out of range, max(%u).\n", port_id,
			IPCM_ANON_PORT_MAX - 1);
		return -EINVAL;
	}
	ret = ipcm_get_grp_id(PORT_ANON, port_id, &grp_id);
	if (ret) {
		ipcm_err("ipcm get grp id fail ret:%d.\n", ret);
		return ret;
	}

	tmsg.grp_id = grp_id;
	tmsg.msg_id = msg_id;
	tmsg.func_type = MSG_TYPE_RAW_PARAM;
	tmsg.msg_param.param = param;
	return ipcm_send_msg(&tmsg);
}
EXPORT_SYMBOL_GPL(ipcm_anon_send_param);

void *ipcm_anon_get_user_addr(u32 paddr)
{
	ipcm_err("Not support in kernel space, should be mapped by self.\n");
	return NULL;
}
