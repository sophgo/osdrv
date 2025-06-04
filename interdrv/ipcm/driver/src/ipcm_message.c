/**
* @file ipcm_message.c
 * @brief Message handling and routing implementation for IPCM
 *
 * Implements message processing functionality including:
 * - Message queue management
 * - Message routing between ports
 * - Response handling
 * - Error recovery
 * - Performance monitoring
 */

#include "ipcm_plat_adapter.h"
#include "ipcm_dbg_rec_info.h"
#include "ipcm_port.h"
#include "ipcm.h"
#include "ipcm_message.h"


#define IPCM_MSG_PORT_NUM 1
typedef struct _ipcm_msg_ctx {
	IPCMPA_SEM sem;
	MsgData cur_msg;
	u32 cur_data_pos;
	s32 pre_msg_data_pos;
	MsgQueue *queue;
} ipcm_msg_ctx;

static POOLHANDLE _pool_handle = NULL;
static u32 _pool_offset = 0; // offset to pool manager

static ipcm_msg_ctx _msg_ctx[IPCM_MSG_PORT_NUM] = {};

static s32 _msg_recv_handle(u8 port_id, void *data)
{
	if (port_id >= IPCM_MSG_PORT_NUM) {
		ipcm_err("port_id(%d max:%d) is invalid.\n", port_id, IPCM_MSG_PORT_NUM-1);
		return -EINVAL;
	}

	if (_msg_ctx[port_id].queue)
		queue_put(_msg_ctx[port_id].queue, data);

	IPCMPA_SEM_UP(&_msg_ctx[port_id].sem);
	return 0;
}

/**
 * @brief Initialize message processing system
 *
 * Sets up message queues and processing structures:
 * - Allocates queue memory
 * - Initializes processing info
 * - Sets up synchronization
 *
 * @return Status code (0 on success, negative on error)
 */
static s32 _ipcm_msg_init(void)
{
	int ret = 0;
	int i = 0;

	ipcm_debug("sizeof(MsgData): %zu\n", sizeof(MsgData));

	for (i=0; i<IPCM_MSG_PORT_NUM; i++) {
		_msg_ctx[i].queue = ipcm_alloc(sizeof(MsgQueue) + (sizeof(MsgData) * MSG_QUEUE_LEN));
		if (_msg_ctx[i].queue == NULL) {
			ipcm_err("port queue alloc failed.\n");
			ret = -ENOMEM;
			goto error;
		}
		queue_init(_msg_ctx[i].queue, sizeof(MsgData), MSG_QUEUE_LEN);
		ipcm_debug("port id:%d queue addr:%lx queue data:%lx\n", i,
			(unsigned long)_msg_ctx[i].queue, (unsigned long)_msg_ctx[i].queue->data);

		//init_waitqueue_head(&_msg_ctx.msg_wait);
		IPCMPA_SEM_INIT(&_msg_ctx[i].sem, 0);
		ret = IPCMPA_SEM_IS_VALID(&_msg_ctx[i].sem);
		if (ret == 0) {
            IPCMPA_SEM_UNINIT(&_msg_ctx[i].sem);
			ipcm_warning("sem is invalid\n");
		}
		_msg_ctx[i].pre_msg_data_pos = -1;
	}
	ret = ipcm_register_port_handle(PORT_MSG, _msg_recv_handle);
	if (ret) {
		ipcm_err("ipcm_register_port_handle failed.\n");
		goto error;
	}
	return 0;

error:
	for(i=0; i<IPCM_MSG_PORT_NUM; i++) {
		if (_msg_ctx[i].queue) {
			queue_uninit(_msg_ctx[i].queue);
			ipcm_free(_msg_ctx[i].queue);
			_msg_ctx[i].queue = NULL;
		}
        IPCMPA_SEM_UNINIT(&_msg_ctx[i].sem);

	}
	return ret; // -ENOMEM
}

/**
 * @brief Clean up message processing system
 *
 * Releases message processing resources:
 * - Frees queue memory
 * - Cleans up processing info
 *
 * @return Status code (0 on success)
 */
static s32 _ipcm_msg_uninit(void)
{
	int i = 0;

	for(i=0; i<IPCM_MSG_PORT_NUM; i++) {
		// release the last pool buff
		if (-1 != _msg_ctx[i].pre_msg_data_pos)
			ipcm_msg_release_buff_by_offset(_msg_ctx[i].pre_msg_data_pos);

		if (_msg_ctx[i].queue) {
			queue_uninit(_msg_ctx[i].queue);
			ipcm_free(_msg_ctx[i].queue);
			_msg_ctx[i].queue = NULL;
		}

		IPCMPA_SEM_UNINIT(&_msg_ctx[i].sem);
	}

	return ipcm_degister_port_handle(PORT_MSG);
}

s32 ipcm_msg_srv_init(PoolConfig *config)
{
	_pool_handle = pool_create(&_pool_offset, config, IPCM_MSG_POOL_ID);
	if (!_pool_handle) {
		ipcm_err("msg pool create fail.\n");
		return -1;
	}
	return _ipcm_msg_init();
}

s32 ipcm_msg_srv_uninit(void)
{
	s32 ret = -1;
	ret = pool_destroy(_pool_handle);
	if (ret) {
		ipcm_err("msg pool_destroy fail,ret:%d\n", ret);
	}
	return _ipcm_msg_uninit();
}

s32 ipcm_msg_cli_init(void)
{
	s32 ret = -1;
	_pool_handle = pool_mgr_get(&_pool_offset, IPCM_MSG_POOL_ID);
	if (!_pool_handle) {
		ipcm_err("msg pool get fail,ret:%d\n", ret);
		return -1;
	}
	pool_print_info(_pool_handle, NULL);
	return _ipcm_msg_init();
}

s32 ipcm_msg_cli_uninit(void)
{
	return _ipcm_msg_uninit();
}

s32 ipcm_msg_data_packed(void *data, u32 len, MsgData *msg)
{
	return ipcm_port_data_packed(_pool_handle, data, len, msg);
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
	ret = ipcm_port_data_packed(_pool_handle, data, len, &stMsg);
	if (ret) {
		ipcm_err("ipcm_port_data_packed failed ret:%d.\n", ret);
		return ret;
	}

    return ipcm_send_msg(&stMsg);
}

MsgData * ipcm_msg_recv_msg(u8 port_id)
{
	return ipcm_port_recv_msg(_msg_ctx[port_id].queue);
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

	ret = IPCMPA_SEM_DOWN_TIMEOUT(&_msg_ctx[port_id].sem, IPCMPA_SEM_TIME_MS_CONVERT(timeout));
    return ret;
}

// for exit thread immediately
s32 ipcm_msg_up_blank_sem(u8 port_id)
{
	IPCMPA_SEM_UP(&_msg_ctx[port_id].sem);
	return 0;
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

	ipcm_debug("remain_len(%d) cur_offset(%d)\n",
		_msg_ctx[port_id].cur_msg.msg_param.msg_ptr.remaining_rd_len, _msg_ctx[port_id].cur_data_pos);
	if (_msg_ctx[port_id].cur_msg.msg_param.msg_ptr.remaining_rd_len != 0) {
		*data = ipcm_msg_get_data_by_offset(_msg_ctx[port_id].cur_data_pos);
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
		if (-1 != _msg_ctx[port_id].pre_msg_data_pos) {
			ipcm_msg_release_buff_by_offset(_msg_ctx[port_id].pre_msg_data_pos);
		}
		_msg_ctx[port_id].pre_msg_data_pos = _msg_ctx[port_id].cur_msg.msg_param.msg_ptr.data_pos;

		_msg_ctx[port_id].cur_data_pos = _msg_ctx[port_id].cur_msg.msg_param.msg_ptr.data_pos;
		*data = ipcm_msg_get_data_by_offset(_msg_ctx[port_id].cur_data_pos);
		ipcm_port_inv_data(*data, _msg_ctx[port_id].cur_msg.msg_param.msg_ptr.remaining_rd_len);
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

u32 ipcm_msg_get_buff_offset(u32 size)
{
    return pool_alloc_offset(_pool_handle, size);
}

s32 ipcm_msg_release_buff_by_offset(u32 pos)
{
    s32 ret = 0;

    ret = pool_free_by_offset(_pool_handle, pos);
	IPCM_DBG_R_BACKTRCE_BY_POS(_pool_handle, pos, 0);
	return ret;
}

void *ipcm_msg_get_data_by_offset(u32 data_pos)
{
    return pool_get_data_by_offset(_pool_handle, data_pos);
}

void *ipcm_msg_get_buff(u32 size)
{
	void *data;
	data = pool_alloc_buffer(_pool_handle, size);
	IPCM_DBG_R_BACKTRCE(_pool_handle, data, 1);

	return data;
}

s32 ipcm_msg_release_buff(void *data)
{
	s32 ret;

	ret = pool_free_buffer(_pool_handle, data);
	IPCM_DBG_R_BACKTRCE(_pool_handle, data, 0);

	return ret;
}

s32 ipcm_msg_pool_reset(void)
{
	return pool_reset(_pool_handle);
}
IPCMPA_EXPORT_SYMBOL_GPL(ipcm_msg_pool_reset);

s32 ipcm_msg_inv_data(void *data, u32 size)
{
	return ipcm_port_inv_data(data, size);
}

s32 ipcm_msg_flush_data(void *data, u32 size)
{
	return ipcm_port_flush_data(data, size);
}

s32 ipcm_msg_data_lock(u8 lock_id)
{
	return ipcm_port_data_lock(lock_id);
}

s32 ipcm_msg_data_unlock(u8 lock_id)
{
	return ipcm_port_data_unlock(lock_id);
}