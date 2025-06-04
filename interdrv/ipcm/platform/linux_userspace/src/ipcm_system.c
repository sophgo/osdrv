
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "ipcm_system.h"
#include "linux/ipcm_linux.h"
#include "ipcm_port.h"

#define DUMP_PRINT_DEFAULT_SIZE 0x1000

#define DEV_NAME "/dev/ipcm_sys"
#define DEV_MEMMAP "/dev/mem"

typedef struct _ipcm_port_sys_ctx {
    int sys_fd;
    void *rtos_base;
	u32 sys_info_off;
	u32 log_off;
} ipcm_port_sys_ctx;

static ipcm_port_sys_ctx _sys_ctx = {};

s32 ipcm_sys_init(void)
{
	int fd;
	fd = open(DEV_NAME, O_RDWR);
	if (fd < 0) {
		ipcm_err("open %s fail. return:%d!\n", DEV_NAME, fd);
		return fd;
	}
	_sys_ctx.sys_fd = fd;

	fd = open(DEV_MEMMAP, O_RDWR | O_SYNC);
	if (fd < 0) {
		ipcm_err("open %s fail. return:%d!\n", DEV_MEMMAP, fd);
        close(_sys_ctx.sys_fd);
		return fd;
	}

    return 0;
}

s32 ipcm_sys_uninit(void)
{
	if (_sys_ctx.sys_fd)
		close(_sys_ctx.sys_fd);

	return 0;
}

static s32 _ipcm_msg_sys_send_cmd(IPCM_SYS_CMD_E cmd, MsgData *msg)
{
	s32 ret = -1;
	if (msg == NULL) {
		ipcm_err("msg is null.\n");
		return -EFAULT;
	}

	ret = ioctl(_sys_ctx.sys_fd, cmd, msg);
	return ret;
}

s32 ipcm_msg_sys_get_sysinfo(void)
{
	MsgData tmsg;
	s32 ret;

	ret = _ipcm_msg_sys_send_cmd(IPCM_IOC_GET_SYSINFO, &tmsg);
	if (ret) {
		ipcm_err("_ipcm_msg_sys_send_cmd err.\n");
		return ret;
	}
	return ret;
}

s32 ipcm_msg_sys_release_sysinfo(void)
{
	return 0;
}

s32 ipcm_msg_sys_get_log(struct dump_uart_s **uart)
{
	MsgData tmsg;
	s32 ret;

	ret = _ipcm_msg_sys_send_cmd(IPCM_IOC_GET_LOG, &tmsg);
	if (ret) {
		ipcm_err("_ipcm_msg_sys_send_cmd err ret:%d.\n", ret);
		return ret;
	}

	ipcm_info("get log paddr:%x\n", tmsg.msg_param.param);

	*uart = (struct dump_uart_s *)ipcm_port_get_user_addr(tmsg.msg_param.param);
	ipcm_debug("*uart:%p\n", *uart);

	return ret;
}

s32 ipcm_msg_sys_release_log(void *data)
{
	(void)data;
	return 0;
}

