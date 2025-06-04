
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "ipcm_port.h"
#include "ipcm_custom.h"
#include "linux/ipcm_linux.h"

#define DEV_NAME "/dev/ipcm_cust"

typedef struct _ipcm_cust_ctx {
    int cust_fd;
    pthread_t t_thread_id;
	unsigned int b_stop;
	unsigned int timeout;
    CUST_MSGPROC_FN handler;
    void *handler_data;
} ipcm_cust_ctx;

static POOLHANDLE _pool_handle;
static u32 _pool_offset = 0; // offset to pool manager

static ipcm_cust_ctx _cust_ctx = {};

static void *_msg_cust_proc(void *args)
{
	int ret;
	fd_set rfds;
	struct timeval timeout;
	MsgData tmsg;
	ipcm_cust_ctx *ctx = NULL;
	u8 port_type = 0;
	u8 port_id = 0;

	if (args == NULL) {
		ipcm_err("%s args is null.\n", __func__);
		return NULL;
	}

	ctx = (ipcm_cust_ctx *)args;

	while(!ctx->b_stop) {
		timeout.tv_sec  = ctx->timeout / 1000;
		timeout.tv_usec = (ctx->timeout % 1000) * 1000;
		FD_ZERO(&rfds);
		FD_SET(ctx->cust_fd, &rfds);
		ret = select(ctx->cust_fd + 1, &rfds, NULL, NULL, &timeout);
		if (ret == -1) {
			ipcm_err("SELECT error\n");
			break;
		} else if (!ret) {
			ipcm_debug("SELECT timeout\n");
			continue;
		}
		if (FD_ISSET(ctx->cust_fd, &rfds)) {
			ret = read(ctx->cust_fd, &tmsg, sizeof(MsgData));
			if (ret != sizeof(MsgData)) {
				ipcm_err("cust read err ret:%d\n", ret);
				continue;
			}
			ret = ipcm_get_port_id(tmsg.grp_id, &port_type, &port_id);
			if (ret) {
				ipcm_err("ipcm get port type and id fail ret:%d.\n", ret);
				goto rls_pool;
			}
			if (ctx->handler && (port_id < IPCM_CUST_PORT_MAX)) {
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
				ipcm_warning("cust recv param err, handle(%p), port id(%u) msg id(%u).\n",
					ctx->handler, port_id, tmsg.msg_id);
			}
rls_pool:
			// release pool buff
			if (tmsg.func_type == MSG_TYPE_SHM) {
				ipcm_cust_release_buff_by_offset(tmsg.msg_param.msg_ptr.data_pos);
			}
		}
	}

	return NULL;
}

s32 ipcm_cust_cli_init(void)
{
	int fd = -1;

	fd = open(DEV_NAME, O_RDWR);
	if (fd < 0) {
		ipcm_err("open %s fail. return:%d!\n", DEV_NAME, fd);
		return fd;
	}
	_cust_ctx.cust_fd = fd;

	ioctl(_cust_ctx.cust_fd, IPCM_IOC_CUST_GET_POOL_OFF, &_pool_offset);
	_pool_handle = ipcm_port_get_pool_mgr_base() + _pool_offset;
	// reset pool
	// ioctl(_cust_ctx.cust_fd, IPCM_IOC_CUST_POOL_RESET, 0);

	_cust_ctx.b_stop = 0;
	_cust_ctx.timeout = 3000;
	pthread_create(&_cust_ctx.t_thread_id, NULL, _msg_cust_proc, &_cust_ctx);
	return 0;
}

s32 ipcm_cust_cli_uninit(void)
{
	_cust_ctx.b_stop = 1;
	pthread_join(_cust_ctx.t_thread_id, NULL);

	if (_cust_ctx.cust_fd)
		close(_cust_ctx.cust_fd);

	return 0;
}

s32 ipcm_cust_send_msg(u8 port_id, u8 msg_id, void *data, u32 len)
{
	MsgData stMsg = {};
	u8 grp_id = 0;
	s32 ret = 0;

	if (data == NULL) {
		ipcm_err("data is null.\n");
		return -EFAULT;
	}
	if (port_id >= IPCM_CUST_PORT_MAX) {
		ipcm_err("port_id(%d max:%d) is invalid.\n", port_id, IPCM_CUST_PORT_MAX-1);
		return -EINVAL;
	}

	ret = ipcm_get_grp_id(PORT_CUST, port_id, &grp_id);
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
	return write(_cust_ctx.cust_fd, &stMsg, sizeof(MsgData));
}

s32 ipcm_cust_send_param(u8 port_id, u8 msg_id, u32 param)
{
	MsgData tmsg = {};
	u8 grp_id = 0;
	s32 ret = 0;

	if (port_id >= IPCM_CUST_PORT_MAX) {
		ipcm_err("port_id(%u) out of range, max(%u).\n", port_id,
			IPCM_CUST_PORT_MAX - 1);
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

	return write(_cust_ctx.cust_fd, &tmsg, sizeof(MsgData));
}

s32 ipcm_cust_register_handle(CUST_MSGPROC_FN handler, void *data)
{
	if (handler) {
		_cust_ctx.handler = handler;
		_cust_ctx.handler_data = data;
		return 0;
	}
	ipcm_err("ipcm_cust_register_handle handler is null.\n");
	return -EINVAL;
}

s32 ipcm_cust_deregister_handle(void)
{
	_cust_ctx.handler = NULL;
	_cust_ctx.handler_data = NULL;
	return 0;
}


void *ipcm_cust_get_data_by_offset(u32 data_pos)
{
	return _pool_handle + data_pos;
}

u32 ipcm_cust_get_buff_offset(u32 size)
{
	u32 pos = 0;

	pos = ioctl(_cust_ctx.cust_fd, IPCM_IOC_CUST_GET_DATA, size);
	ipcm_debug("get buff pos(%d)\n", pos);

	return pos;
}

void *ipcm_cust_get_buff(u32 size)
{
	u32 pos = 0;

	pos = ipcm_cust_get_buff_offset(size);
	ipcm_debug("get buff pos(%d)\n", pos);
	if (pos == U32_MAX) {
		return NULL;
	}
	return _pool_handle + pos;
}

s32 ipcm_cust_release_buff_by_offset(u32 pos)
{
	ipcm_debug("release buff pos(%d)\n", pos);
	return ioctl(_cust_ctx.cust_fd, IPCM_IOC_CUST_RLS_DATA, pos);
}

s32 ipcm_cust_release_buff(void *data)
{
	u32 pos = 0;

    if (data == NULL) {
        ipcm_err("data is null.\n");
        return -EINVAL;
    }

	pos = data - _pool_handle;
	return ipcm_cust_release_buff_by_offset(pos);
}
