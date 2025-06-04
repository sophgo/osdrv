// #define _DEBUG
#include "ipcm_plat_adapter.h"
#include "ipcm_port.h"
#include "ipcm_custom.h"
#include "ipcm.h"
#include <console_uart.h>
#include "cvi_comm_ipcm.h"
// Custom IPCM context structure
typedef struct _ipcm_cust_ctx {
	unsigned int b_stop;          // Stop flag for task
	unsigned int timeout;         // Timeout value for semaphore
	void *handler_data;          // User data for message handler
	CUST_MSGPROC_FN handler;     // Message processing callback
	MsgQueue *queue;             // Message queue
	aos_task_t task;            // Task handle
	aos_sem_t sem;              // Semaphore for synchronization
} ipcm_cust_ctx;

// Global custom IPCM context
static ipcm_cust_ctx _cust_ctx = {};

static s32 vuart_process_handler(void *priv, ipcm_cust_msg_t *data)
{
	unsigned char port_id, msg_id, data_type;
	unsigned int  data_len;
	int           ret = 0;

	if (data == NULL) {
		printf("%s fail, handle data is null.\n", __func__);
		return -1;
	}

	port_id   = data->port_id;
	msg_id    = data->msg_id;
	data_type = data->data_type;
	data_len  = data->size;

	if ((port_id == CVI_IPCM_CUST_VUART_PORT) && (msg_id == 0) &&
		(data_type == IPCM_MSG_TYPE_SHM)) {
		ret = console_push_data_to_ringbuffer(data->data, data_len);
	} else {
		printf("%s port_id:%d msg_id:%d data_type:%d not handled.\n", __func__,
			port_id, msg_id, data_type);
	}

	return ret;
}

// Handle received messages from custom port
static int _cust_recv_handle(u8 port_id, void *data)
{
	if (port_id < IPCM_CUST_PORT_MAX) {
		if (_cust_ctx.queue)
			queue_put(_cust_ctx.queue, data);
		aos_sem_signal(&_cust_ctx.sem);
	} else {
		ipcm_err("port_id(%d max:%d) is invalid.\n", port_id, IPCM_CUST_PORT_MAX-1);
		return -EINVAL;
	}
	return 0;
}

// Custom message processing task
static void _ipcm_cust_proc(void *arg)
{
	ipcm_cust_ctx *ctx;
	MsgData tmsg;
	unsigned int timeout = 0;
	int ret = 0;
	u8 port_type = 0;
	u8 port_id = 0;

	if (arg == NULL) {
		ipcm_err("param err.\n");
		return;
	}

	ctx = (ipcm_cust_ctx *)arg;
	timeout = ctx->timeout;

	// Main message processing loop
	while (!ctx->b_stop) {
		ret = aos_sem_wait(&ctx->sem, timeout);
		if (!ret) {
			while (!queue_is_empty(ctx->queue)) {
				ret = queue_get(ctx->queue, &tmsg);
				if (ret) {
					ipcm_warning("cust msg get fail.\n");
					continue;
				}
				ipcm_debug("cust recv msg:%lx\n", *(unsigned long*)&tmsg);
				ret = ipcm_get_port_id(tmsg.grp_id, &port_type, &port_id);
				if (ret) {
					ipcm_err("ipcm get port type and id fail ret:%d.\n", ret);
					continue;
				}

				// Process message using registered handler
				if (ctx->handler) {
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
					ctx->handler(ctx->handler_data, &cust_msg);
				} else {
					ipcm_warning("cust recv param err, handle(%p), port id(%u) msg id(%u)\n",
						ctx->handler, port_id, tmsg.msg_id);
				}
				// Release pool buffer if using shared memory
				if (tmsg.func_type == MSG_TYPE_SHM) {
					ipcm_cust_release_buff_by_offset(tmsg.msg_param.msg_ptr.data_pos);
				}
			}
		}
	}

	return;
}

// Initialize custom IPCM module
s32 ipcm_cust_plat_init(void)
{
	int ret = 0;

	ipcm_debug("%s\n", __func__);

	// Allocate and initialize message queue
	_cust_ctx.queue = ipcm_alloc(sizeof(MsgQueue) + (sizeof(MsgData) * MSG_QUEUE_LEN));
	if (_cust_ctx.queue == NULL) {
		ipcm_err("port queue alloc failed.\n");
		return -ENOMEM;
	}
	queue_init(_cust_ctx.queue, sizeof(MsgData), MSG_QUEUE_LEN);
	ipcm_debug("queue addr:%lx queue data:%lx\n",
		(unsigned long)_cust_ctx.queue, (unsigned long)_cust_ctx.queue->data);

	// Register port handler
	ret = ipcm_register_port_handle(PORT_CUST, _cust_recv_handle);

	// Initialize semaphore
	ret = IPCMPA_SEM_INIT(&_cust_ctx.sem, 0);
	if (ret) {
		ipcm_err("sem new fail ret(%d).\n", ret);
	}
	ret = IPCMPA_SEM_IS_VALID(&_cust_ctx.sem);
	if (ret == 0) {
		ipcm_err("sem is invalid\n");
	}
	// Create message processing task
	_cust_ctx.timeout = 3000;
	_cust_ctx.b_stop = 0;

	ipcm_cust_register_handle(vuart_process_handler, NULL);
	ret = IPCMPA_TASK_NEW(&_cust_ctx.task, IPCM_CUST_TASK_NAME, _ipcm_cust_proc, &_cust_ctx, 8192, IPCM_CUST_TASK_PRI);
	if (ret) {
		ipcm_err("aos_task_new fail ret:%d\n", ret);
		return ret;
	}

    return ret;
}

// Uninitialize custom IPCM module
u32 ipcm_cust_plat_uninit(void)
{
	_cust_ctx.b_stop = 1;
	// aos_sem_signal(&_cust_ctx.sem);
	IPCMPA_SEM_UNINIT(&_cust_ctx.sem);

	if (_cust_ctx.queue) {
		queue_uninit(_cust_ctx.queue);
		ipcm_free(_cust_ctx.queue);
		_cust_ctx.queue = NULL;
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

// Deregister custom message handler
s32 ipcm_cust_deregister_handle(void)
{
	_cust_ctx.handler = NULL;
	_cust_ctx.handler_data = NULL;
	return 0;
}

// Get message from custom queue
MsgData * ipcm_cust_recv_msg(void)
{
	return ipcm_port_recv_msg(_cust_ctx.queue);
}

// Send parameter message through custom port
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
