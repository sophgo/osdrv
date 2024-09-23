
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

#include "ipcm_port_common.h"
#include "ipcm.h"

#define IPCM_MSG_PORT_NUM 1
typedef struct _ipcm_msg_ctx {
	MsgData cur_msg;
	u32 cur_data_pos;
	s32 pre_msg_data_pos;
    //wait_queue_head_t msg_wait;
	struct semaphore sem;
    //int wait_flag;
	MsgQueue *queue;
} ipcm_msg_ctx;

static ipcm_msg_ctx _msg_ctx[IPCM_MSG_PORT_NUM] = {};

static s32 _msg_recv_handle(u8 port_id, void *data)
{
    //_msg_ctx.wait_flag = 1;
	//wake_up_interruptible(&_msg_ctx.msg_wait);
	if (port_id >= IPCM_MSG_PORT_NUM) {
		ipcm_err("port_id(%d max:%d) is invalid.\n", port_id, IPCM_MSG_PORT_NUM-1);
		return -EINVAL;
	}
	if (_msg_ctx[port_id].queue)
		queue_put(_msg_ctx[port_id].queue, data);
	up(&_msg_ctx[port_id].sem);
	//ipcm_err("wake_up_interruptible.\n");
	return 0;
}

s32 ipcm_msg_init(void)
{
    s32 ret = 0;
	int i = 0;

	ipcm_debug("%s\n", __func__);

	for (i=0; i<IPCM_MSG_PORT_NUM; i++) {
		_msg_ctx[i].queue = ipcm_alloc(sizeof(MsgQueue) + (sizeof(MsgData) * MSG_QUEUE_LEN));
		if (_msg_ctx[i].queue == NULL) {
			ipcm_err("port queue alloc failed.\n");
			ipcm_port_uninit(PORT_MSG);
			goto error;
		}
		queue_init(_msg_ctx[i].queue, sizeof(MsgData), MSG_QUEUE_LEN);
		ipcm_debug("port id:%d queue addr:%lx queue data:%lx\n", i,
			(unsigned long)_msg_ctx[i].queue, (unsigned long)_msg_ctx[i].queue->data);

		//init_waitqueue_head(&_msg_ctx.msg_wait);
		sema_init(&_msg_ctx[i].sem, 0);

		_msg_ctx[i].pre_msg_data_pos = -1;
	}

    ret = ipcm_port_init(PORT_MSG, _msg_recv_handle);

	return ret;

error:
	for(i=0; i<IPCM_MSG_PORT_NUM; i++) {
		if (_msg_ctx[i].queue) {
			queue_uninit(_msg_ctx[i].queue);
			ipcm_free(_msg_ctx[i].queue);
			_msg_ctx[i].queue = NULL;
		}
	}

    return -ENOMEM;
}

s32 ipcm_msg_uninit(void)
{
	int i = 0;

	for(i=0; i<IPCM_MSG_PORT_NUM; i++) {
		// release the last pool buff
		if (-1 != _msg_ctx[i].pre_msg_data_pos)
			ipcm_release_buff_by_pos(_msg_ctx[i].pre_msg_data_pos);

		if (_msg_ctx[i].queue) {
			queue_uninit(_msg_ctx[i].queue);
			ipcm_free(_msg_ctx[i].queue);
			_msg_ctx[i].queue = NULL;
		}
	}

    return ipcm_port_uninit(PORT_MSG);
}

void *ipcm_msg_get_buff(u32 size)
{
    return ipcm_get_buff(size);
}

s32 ipcm_msg_release_buff(void *data)
{
    return ipcm_release_buff(data);
}

s32 ipcm_msg_inv_data(void *data, u32 size)
{
	return ipcm_inv_data(data, size);
}

s32 ipcm_msg_flush_data(void *data, u32 size)
{
    return ipcm_flush_data(data, size);
}

s32 ipcm_msg_send_msg(u8 port_id, u8 msg_id, void *data, u32 len)
{
	MsgData stMsg = {};
	u8 grp_id = 0;
	s32 ret = 0;

	if (data == NULL) {
		ipcm_err("data is null.\n");
		return -EFAULT;
	}
	if (port_id >= IPCM_MSG_PORT_NUM) {
		ipcm_err("port_id(%d max:%d) is invalid.\n", port_id, IPCM_MSG_PORT_NUM-1);
		return -EINVAL;
	}

	ret = ipcm_get_grp_id(PORT_MSG, port_id, &grp_id);
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

s32 ipcm_msg_send_param(u8 port_id, u8 msg_id, u32 param)
{
	MsgData stMsg = {};
	u8 grp_id = 0;
	s32 ret = 0;

	if (port_id >= IPCM_MSG_PORT_NUM) {
		ipcm_err("port_id(%d max:%d) is invalid.\n", port_id, IPCM_MSG_PORT_NUM-1);
		return -EINVAL;
	}

	ret = ipcm_get_grp_id(PORT_MSG, port_id, &grp_id);
	if (ret) {
		ipcm_err("ipcm get grp id fail ret:%d.\n", ret);
		return ret;
	}

	stMsg.grp_id = grp_id;
	stMsg.msg_id = msg_id;
	stMsg.func_type = MSG_TYPE_RAW_PARAM;
	stMsg.msg_param.param = param;
	return ipcm_send_msg(&stMsg);
}

s32 ipcm_msg_poll(u8 port_id, u32 timeout)
{
	int ret = -1;

	if (port_id >= IPCM_MSG_PORT_NUM) {
		ipcm_err("port_id(%d max:%d) is invalid.\n", port_id, IPCM_MSG_PORT_NUM-1);
		return -EINVAL;
	}

    //_msg_ctx.wait_flag = 0;
	ret = down_timeout(&_msg_ctx[port_id].sem, msecs_to_jiffies(timeout));
	//wait_event_interruptible_timeout(_msg_ctx.msg_wait, _msg_ctx.wait_flag, msecs_to_jiffies(timeout));
    return ret;
}

s32 ipcm_msg_get_cur_msginfo(u8 port_id, u8 *func_type, u8 *msg_id, u32 *remain_len)
{
	if (port_id >= IPCM_MSG_PORT_NUM) {
		ipcm_err("port_id(%d max:%d) is invalid.\n", port_id, IPCM_MSG_PORT_NUM-1);
		return -EINVAL;
	}

	if (!func_type || !msg_id || !remain_len) {
		ipcm_err("func_type(%lu) msg_id(%lu) remain_len(%lu) invalid.\n",
			(unsigned long)func_type, (unsigned long)msg_id, (unsigned long)remain_len);
		return -EINVAL;
	}

	*func_type = _msg_ctx[port_id].cur_msg.func_type;
	*msg_id = _msg_ctx[port_id].cur_msg.msg_id;
	*remain_len = _msg_ctx[port_id].cur_msg.msg_param.msg_ptr.remaining_rd_len;
	return 0;
}

s32 ipcm_msg_read_data(u8 port_id, void **data, u32 len)
{
	s32 ret = -1;

	if (port_id >= IPCM_MSG_PORT_NUM) {
		ipcm_err("port_id(%d max:%d) is invalid.\n", port_id, IPCM_MSG_PORT_NUM-1);
		return -EINVAL;
	}

	ipcm_debug("remain_len(%d) cur_pos(%d)\n",
		_msg_ctx[port_id].cur_msg.msg_param.msg_ptr.remaining_rd_len, _msg_ctx[port_id].cur_data_pos);
	if (_msg_ctx[port_id].cur_msg.msg_param.msg_ptr.remaining_rd_len != 0) {
		*data = ipcm_get_data_by_pos(_msg_ctx[port_id].cur_data_pos);
		if (len >= _msg_ctx[port_id].cur_msg.msg_param.msg_ptr.remaining_rd_len) {
			s32 tmp = _msg_ctx[port_id].cur_msg.msg_param.msg_ptr.remaining_rd_len;

			_msg_ctx[port_id].cur_msg.msg_param.msg_ptr.remaining_rd_len = 0;
			// _msg_ctx.cur_data_pos += _msg_ctx.cur_msg.msg_param.msg_ptr.remaining_rd_len;
			return tmp;
		}
		// len < _msg_ctx.cur_msg.msg_param.msg_ptr.remaining_rd_len
		_msg_ctx[port_id].cur_msg.msg_param.msg_ptr.remaining_rd_len -= len;
		_msg_ctx[port_id].cur_data_pos += len;
		return len;
	}
	{
		// _msg_ctx.cur_msg.msg_param.msg_ptr.remaining_rd_len == 0
		ret = queue_get(_msg_ctx[port_id].queue, &_msg_ctx[port_id].cur_msg);
		if (ret < 0) {
			// ipcm_err("read msg err.\n");
			return ret;
		}

		if (_msg_ctx[port_id].cur_msg.func_type) {
			*data = (void *)(unsigned long)_msg_ctx[port_id].cur_msg.msg_param.param;
			_msg_ctx[port_id].cur_msg.msg_param.msg_ptr.remaining_rd_len = 0;
			return sizeof(_msg_ctx[port_id].cur_msg.msg_param.param);
		}

		// _msg_ctx.pre_msg_data_pos will record previous data pos if it valid
		// after read msg success, release previous buff, ensure release one time
		if (-1 != _msg_ctx[port_id].pre_msg_data_pos)
			ipcm_release_buff_by_pos(_msg_ctx[port_id].pre_msg_data_pos);
		_msg_ctx[port_id].pre_msg_data_pos = _msg_ctx[port_id].cur_msg.msg_param.msg_ptr.data_pos;

		_msg_ctx[port_id].cur_data_pos = _msg_ctx[port_id].cur_msg.msg_param.msg_ptr.data_pos;
		*data = ipcm_get_data_by_pos(_msg_ctx[port_id].cur_data_pos);
		ipcm_inv_data(*data, _msg_ctx[port_id].cur_msg.msg_param.msg_ptr.remaining_rd_len);
		if (len >= _msg_ctx[port_id].cur_msg.msg_param.msg_ptr.remaining_rd_len) {
			s32 tmp = _msg_ctx[port_id].cur_msg.msg_param.msg_ptr.remaining_rd_len;
			_msg_ctx[port_id].cur_data_pos += tmp;
			_msg_ctx[port_id].cur_msg.msg_param.msg_ptr.remaining_rd_len = 0;
			return tmp;
		}
		// len < _msg_ctx.cur_msg.msg_param.msg_ptr.remaining_rd_len
		_msg_ctx[port_id].cur_msg.msg_param.msg_ptr.remaining_rd_len -= len;
		_msg_ctx[port_id].cur_data_pos += len;
		return len;
	}
	return ret;
}

s32 ipcm_msg_data_lock(u8 lock_id)
{
	return ipcm_data_lock(lock_id);
}

s32 ipcm_msg_data_unlock(u8 lock_id)
{
	return ipcm_data_unlock(lock_id);
}
