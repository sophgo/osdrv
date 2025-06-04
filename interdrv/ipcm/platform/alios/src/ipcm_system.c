/**
 * @file ipcm_system.c
 * @author allen.huang (allen.huang@cvitek.com)
 * @brief IPCM system module implementation for AliOS
 * @version 0.1
 * @date 2023-01-17
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <aos/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ipcm_port.h"
#include "arch_helpers.h"
#include "ipcm_system.h"
#include "ipcm.h"

// System context structure
typedef struct _IPCM_SYS_CTX {
	unsigned int b_stop;     // Stop flag for task
	unsigned int timeout;    // Timeout value for semaphore
	MsgQueue *queue;        // Message queue
} IPCM_SYS_CTX;

// Synchronization semaphore
static aos_sem_t sem;

// Global system context
static IPCM_SYS_CTX _sys_ctx;

// Handle received system messages
static s32 _sys_recv_handle(u8 port_id, void *data)
{
	ipcm_debug("recv sys msg\n");
	if (_sys_ctx.queue)
		queue_put(_sys_ctx.queue, data);

	aos_sem_signal(&sem);
	return 0;
}

// Get UART dump handle and length
extern int get_dump_uart_handle(void **handle, unsigned int *len);

// Process GET_LOG message
static s32 _msg_proc_get_log(void *msg, void *priv)
{
	void *uart_handle;
	u32 uart_len;
	s32 ret;
	MsgData _msg = {};
	u8 grp_id = 0;

	ipcm_debug("%s %lx\n", __func__, *(unsigned long*)msg);
	ret = get_dump_uart_handle(&uart_handle, &uart_len);
	if (ret) {
		ipcm_err("get_dump_uart_handle fail ret:%d\n", ret);
		return ret;
	}

	ipcm_debug("%p %d\n", uart_handle, uart_len);

	ret = ipcm_get_grp_id(PORT_SYSTEM, 0, &grp_id);
	if (ret) {
		ipcm_err("ipcm get grp id fail ret:%d.\n", ret);
		return ret;
	}

	// Send response message with UART handle
	_msg.grp_id = grp_id;
	_msg.msg_id = IPCM_MSG_GET_LOG_RSP;
	_msg.func_type = MSG_TYPE_RAW_PARAM;
	_msg.msg_param.param = (unsigned long int)uart_handle;
	flush_dcache_range((uintptr_t)uart_handle, uart_len);

	return ipcm_send_msg(&_msg);
}

// Process GET_SYSINFO message
static s32 _msg_proc_get_sysinfo(void *msg, void *priv)
{
	return 0;
}

// System message processing table
static msg_proc_item _port_sys_proc_table[] = {
	{IPCM_MSG_GET_SYSINFO, _msg_proc_get_sysinfo},
	{IPCM_MSG_GET_LOG, _msg_proc_get_log},
};

// System message processing info
static msg_proc_info _port_sys_proc_info = {
	PORT_SYSTEM,
	sizeof(_port_sys_proc_table) / sizeof(msg_proc_item),
	&_port_sys_proc_table[0]
};

// System message processing task
static void _ipcm_sys_proc(void *arg)
{
	IPCM_SYS_CTX *ctx;
	MsgData tmsg;
	unsigned int timeout = 0;
	int ret = 0;
	MSGPROC_FN fn;

	if (arg == NULL) {
		ipcm_err("param err.\n");
		return;
	}

	ctx = (IPCM_SYS_CTX *)arg;
	timeout = ctx->timeout;

	// Main message processing loop
	while (!ctx->b_stop) {
		ret = aos_sem_wait(&sem, timeout);
		if (!ret) {
			while (!queue_get(_sys_ctx.queue, &tmsg)) {
				ipcm_warning("resv msg:%lx\n", *(long *)&tmsg);
				fn = port_get_msg_fn(tmsg.msg_id, &_port_sys_proc_info);
				if (fn) {
					fn(&tmsg, NULL);
				}
			}
		}
	}

	return;
}

// Initialize system IPCM module
s32 ipcm_sys_init(void)
{
	s32 ret;

	// Allocate and initialize message queue
	_sys_ctx.queue = ipcm_alloc(sizeof(MsgQueue) + (sizeof(MsgData) * MSG_QUEUE_LEN));
	if (_sys_ctx.queue == NULL) {
		ipcm_err("port queue alloc failed.\n");
		return -ENOMEM;
	}
	queue_init(_sys_ctx.queue, sizeof(MsgData), MSG_QUEUE_LEN);
	ipcm_debug("queue addr:%lx queue data:%lx\n",
		(unsigned long)_sys_ctx.queue, (unsigned long)_sys_ctx.queue);

	// Register port handler
	ret = ipcm_register_port_handle(PORT_SYSTEM, _sys_recv_handle);
	if (ret) {
		ipcm_err("ipcm_register_port_handle failed.\n");
		return -EFAULT;
	}

	// Initialize semaphore
	ret = aos_sem_new(&sem, 0);
	if (ret) {
		ipcm_err("sem new fail ret(%d).\n", ret);
	}
	ret = aos_sem_is_valid(&sem);
	if (ret == 0) {
		printf("sem is invalid\n");
	}

	// Create message processing task
	_sys_ctx.timeout = 3000;
	_sys_ctx.b_stop = 0;
	ret = aos_task_new("sys proc", _ipcm_sys_proc, &_sys_ctx, 8192);
	if (ret) {
		ipcm_err("aos_task_new fail ret:%d\n", ret);
		return ret;
	}

	return ret;
}

// Uninitialize system IPCM module
s32 ipcm_sys_uninit(void)
{
	_sys_ctx.b_stop = 1;
	if (aos_sem_is_valid(&sem))
		aos_sem_free(&sem);

	if (_sys_ctx.queue) {
		queue_uninit(_sys_ctx.queue);
		ipcm_free(_sys_ctx.queue);
		_sys_ctx.queue = NULL;
	}

	return ipcm_degister_port_handle(PORT_SYSTEM);
}
