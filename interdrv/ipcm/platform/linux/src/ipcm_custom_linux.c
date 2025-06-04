// #define _DEBUG
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>
#include "ipcm_plat_adapter.h"
#include "ipcm_port.h"
#include "ipcm_custom.h"
#include "ipcm.h"

// Custom IPCM context structure
typedef struct _ipcm_cust_ctx {
	unsigned int b_stop;          // Stop flag for task
	unsigned int timeout;         // Timeout value for semaphore
	void *handler_data;          // User data for message handler
	CUST_MSGPROC_FN handler;     // Message processing callback
	MsgQueue *queue;             // Message queue
	MsgQueue *queue_kernel;
	struct task_struct *task;    // Task handle
	wait_queue_head_t cust_wait;
	struct semaphore sem;        // Semaphore for synchronization
} ipcm_cust_ctx;

// Global custom IPCM context
static ipcm_cust_ctx _cust_ctx = {};

// Handle received messages from custom port
static int _cust_recv_handle(u8 port_id, void *data)
{
	if (port_id < IPCM_CUST_KER_PORT_ST) {
		if (_cust_ctx.queue)
			queue_put(_cust_ctx.queue, data);
		wake_up_interruptible(&_cust_ctx.cust_wait);
	} else if (port_id < IPCM_CUST_PORT_MAX) {
		if (_cust_ctx.queue_kernel)
			queue_put(_cust_ctx.queue_kernel, data);
		up(&_cust_ctx.sem);
	} else {
		ipcm_err("port_id(%d max:%d) is invalid.\n", port_id, IPCM_CUST_PORT_MAX-1);
		return -EINVAL;
	}
	return 0;
}

// Custom message processing task
static s32 _ipcm_cust_proc(void *data)
{
	s32 ret = 0;
	s32 kernel_handled = 0;
	u8 port_type = 0;
	u8 port_id = 0;

    ipcm_cust_ctx *pctx = (ipcm_cust_ctx *)data;

	if (pctx == NULL) {
		ipcm_err("ctx is null.\n");
		return -EINVAL;
	}

	while (!pctx->b_stop && !kthread_should_stop()) {
		ret = down_interruptible(&pctx->sem);
		ipcm_debug("down ret is %d\n", ret);
		if ((0 == ret) && !pctx->b_stop) {
			MsgData tmsg;

			if (queue_is_empty(pctx->queue_kernel)) {
				ipcm_warning("cust kernel queue is empty.\n");
				continue;
			}
			ipcm_debug("msg not empty.\n");
			ret = queue_get(pctx->queue_kernel, &tmsg);
			if (ret) {
				ipcm_warning("cust kernel msg get fail.\n");
				continue;
			}
			ipcm_debug("cust read msg:%lx\n", *(unsigned long*)&tmsg);
			ret = ipcm_get_port_id(tmsg.grp_id, &port_type, &port_id);
			if (ret) {
				ipcm_err("ipcm get port type and id fail ret:%d.\n", ret);
				goto rls_pool;
			}

			if (pctx->handler) {
				ipcm_cust_msg_t cust_msg;
				cust_msg.port_id = port_id;
				cust_msg.msg_id = tmsg.msg_id;
				cust_msg.data_type = tmsg.func_type;
				if (tmsg.func_type == MSG_TYPE_SHM) {
					cust_msg.data = ipcm_cust_get_data_by_offset(tmsg.msg_param.msg_ptr.data_pos);
					cust_msg.size = tmsg.msg_param.msg_ptr.remaining_rd_len;
					ipcm_port_inv_data(cust_msg.data, cust_msg.size);
				} else {
					cust_msg.data = (void *)(unsigned long)tmsg.msg_param.param;
					cust_msg.size = 4;
				}
				kernel_handled = pctx->handler(pctx->handler_data, &cust_msg);
			}

			if (0 == kernel_handled) {
				/** push msg to userspace
				 * 1. copy msg to userpase queue
				 * 2. wait up pctx->cust_wait: wake_up_interruptible(&pctx->cust_wait);
				 */
				queue_put(pctx->queue, &tmsg);
				wake_up_interruptible(&pctx->cust_wait);
			}
rls_pool:
			// if release pool buff
			if (tmsg.func_type == MSG_TYPE_SHM && 1 == kernel_handled) {
				ipcm_cust_release_buff_by_offset(tmsg.msg_param.msg_ptr.data_pos);
			}
		}
	}
	return 0;
}

// Initialize custom IPCM module
s32 ipcm_cust_plat_init(void)
{
	int ret	= 0;
	struct sched_param tsk;

	ipcm_debug("%s\n", __func__);

	init_waitqueue_head(&_cust_ctx.cust_wait);

	_cust_ctx.queue = ipcm_alloc(sizeof(MsgQueue) + (sizeof(MsgData) * MSG_QUEUE_LEN));
	if (_cust_ctx.queue == NULL) {
		ipcm_err("port queue alloc failed.\n");
		return -ENOMEM;
	}
	queue_init(_cust_ctx.queue, sizeof(MsgData), MSG_QUEUE_LEN);
	ipcm_debug("queue addr:%lx queue data:%lx\n",
		(unsigned long)_cust_ctx.queue, (unsigned long)_cust_ctx.queue->data);

	_cust_ctx.queue_kernel = ipcm_alloc(sizeof(MsgQueue) + (sizeof(MsgData) * MSG_QUEUE_LEN));
	if (_cust_ctx.queue_kernel == NULL) {
		ipcm_err("port queue alloc failed.\n");
		if (_cust_ctx.queue)
			ipcm_free(_cust_ctx.queue);
		return -ENOMEM;
	}
	queue_init(_cust_ctx.queue_kernel, sizeof(MsgData), MSG_QUEUE_LEN);
	ipcm_debug("queue addr:%lx queue data:%lx\n",
		(unsigned long)_cust_ctx.queue_kernel, (unsigned long)_cust_ctx.queue_kernel->data);

	ipcm_register_port_handle(PORT_CUST, _cust_recv_handle);

	sema_init(&_cust_ctx.sem, 0);

	tsk.sched_priority = MAX_USER_RT_PRIO - 10;
	_cust_ctx.b_stop = 0;
	_cust_ctx.task = kthread_run(_ipcm_cust_proc, &_cust_ctx, "cvitask_cust_process");
	ret = sched_setscheduler(_cust_ctx.task, SCHED_RR, &tsk);
	if (ret)
		ipcm_warning("vpss thread priority update failed: %d\n", ret);

	return 0;
}

// Uninitialize custom IPCM module
u32 ipcm_cust_plat_uninit(void)
{
	_cust_ctx.b_stop = 1;
	up(&_cust_ctx.sem)
	IPCMPA_SEM_UNINIT(&_cust_ctx.sem);

	if (_cust_ctx.task)
		kthread_stop(_cust_ctx.task);

	if (_cust_ctx.queue) {
		queue_uninit(_cust_ctx.queue);
		ipcm_free(_cust_ctx.queue);
		_cust_ctx.queue = NULL;
	}

	if (_cust_ctx.queue_kernel) {
		queue_uninit(_cust_ctx.queue_kernel);
		ipcm_free(_cust_ctx.queue_kernel);
		_cust_ctx.queue_kernel = NULL;
	}

	return ipcm_degister_port_handle(PORT_CUST);
}

// Register custom message handler
s32 ipcm_cust_register_handle(CUST_MSGPROC_FN handler, void *data)
{
	if (handler) {
		_cust_ctx.handler = handler;
		_cust_ctx.handler_data = data;
		return 0;
	}
	ipcm_err("%s handler is null.\n", __func__);
	return -EINVAL;
}
IPCMPA_EXPORT_SYMBOL_GPL(ipcm_cust_register_handle);

// Deregister custom message handler
s32 ipcm_cust_deregister_handle(void)
{
	_cust_ctx.handler = NULL;
	_cust_ctx.handler_data = NULL;
	return 0;
}
IPCMPA_EXPORT_SYMBOL_GPL(ipcm_cust_deregister_handle);

u32 ipcm_linux_drv_cust_poll(struct file *file, struct poll_table_struct *table)
{
	poll_wait(file, &_cust_ctx.cust_wait, table);
	if (!queue_is_empty(_cust_ctx.queue))
		return POLLIN | POLLWRNORM;
	return 0;
}

MsgData * ipcm_cust_recv_msg(void)
{
	return ipcm_port_recv_msg(_cust_ctx.queue);
}

s32 ipcm_cust_send_param(u8 port_id, u8 msg_id, u32 param)
{
    MsgData tmsg = {};
	u8 grp_id = 0;
	s32 ret = 0;

	if (port_id >= IPCM_CUST_PORT_MAX) {
		ipcm_err("port_id(%d max:%d) is invalid.\n", port_id, IPCM_CUST_PORT_MAX-1);
		return -EINVAL;
	}

	ret = ipcm_get_grp_id(PORT_CUST, port_id, &grp_id);
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
IPCMPA_EXPORT_SYMBOL_GPL(ipcm_cust_send_param);
