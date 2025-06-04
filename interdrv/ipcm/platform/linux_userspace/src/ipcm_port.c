
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <pthread.h>
#include "linux/ipcm_linux.h"
#include "ipcm_port.h"

#define DEV_NAME "/dev/ipcm_port"
#define DEV_MEMMAP "/dev/mem"

typedef struct _ipcm_port_ctx {
	int port_fd;
	int mem_fd;
	void *rtos_base;
	POOLMGRHANDLE pool_mgr_base;
	u32 pool_mgr_paddr;
	u32 pool_mgr_capacity;
	u32 rtos_paddr;
	u32 rtos_size;
	void *boot_reg_base; // rtos boot stat reg base
} ipcm_port_ctx;

static ipcm_port_ctx _port_ctx = {};

static u32 _init_cnt = 0;

s32 pool_get_param(void)
{
	int ret = 0;
	IPCM_PADDR_INFO_S paddr_info;

	ret = ioctl(_port_ctx.port_fd , IPCM_IOC_GET_POOL_ADDR, &paddr_info);
	_port_ctx.pool_mgr_paddr = paddr_info.paddr;
	_port_ctx.pool_mgr_capacity = paddr_info.size;
	ipcm_info("ipcm_pool_addr= 0x%x, size = 0x%x\n", _port_ctx.pool_mgr_paddr, _port_ctx.pool_mgr_capacity);
	return ret;
}
s32 rtos_get_param(void)
{
	int ret = 0;
	IPCM_PADDR_INFO_S paddr_info;

	ret = ioctl(_port_ctx.port_fd , IPCM_IOC_GET_RTOS_ADDR, &paddr_info);
	_port_ctx.rtos_paddr = paddr_info.paddr;
	_port_ctx.rtos_size = paddr_info.size;
	ipcm_info("freertos addr = 0x%x, size = 0x%x\n", _port_ctx.rtos_paddr, _port_ctx.rtos_size);
	return ret;
}

s32 ipcm_port_init(void)
{
    int fd = -1;
    void *map_addr = NULL;

	if (_init_cnt) {
		ipcm_info("ipcm port has been inited, before increace _init_cnt is %u.\n", _init_cnt);
		_init_cnt++;
		// return _port_ctx.port_fd;
		return 0;
	}

	fd = open(DEV_NAME, O_RDWR);
	if (fd < 0) {
		ipcm_err("open %s fail. return:%d!\n", DEV_NAME, fd);
		return fd;
	}
    _port_ctx.port_fd = fd;

	fd = open(DEV_MEMMAP, O_RDWR | O_SYNC);
	if (fd < 0) {
		ipcm_err("open %s fail. return:%d!\n", DEV_MEMMAP, fd);
        close(_port_ctx.port_fd);
		return fd;
	}
    _port_ctx.mem_fd = fd;

	pool_get_param();
	rtos_get_param();

	map_addr = mmap(NULL, _port_ctx.rtos_size, PROT_READ|PROT_WRITE, MAP_SHARED, _port_ctx.mem_fd, _port_ctx.rtos_paddr);
	if (map_addr == (void *)-1) {
		ipcm_err("map_rtos_base mmap err.\n");
	}
    _port_ctx.rtos_base = map_addr;
	ipcm_debug("map_rtos_base:%p\n", map_addr);

	map_addr = mmap(NULL, _port_ctx.pool_mgr_capacity, PROT_READ|PROT_WRITE, MAP_SHARED, _port_ctx.mem_fd, _port_ctx.pool_mgr_paddr);
	if (map_addr == (void *)-1) {
		ipcm_err("map_rtos_base mmap err.\n");
	}
    _port_ctx.pool_mgr_base = map_addr;
	ipcm_debug("map share memory base:%p\n", map_addr);

	map_addr = mmap(NULL, AP_SYSTEM_REG_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, _port_ctx.mem_fd, AP_SYSTEM_REG_BASE);
	if (map_addr == (void *)-1) {
		ipcm_err("boot_reg_base mmap err.\n");
	}
    _port_ctx.boot_reg_base = map_addr;

	_init_cnt++;
	ipcm_info("ipcm_port_init success cnt:%d rtos:%p pool_mgr:%p boot_reg:%p.\n",
		_init_cnt, _port_ctx.rtos_base, _port_ctx.pool_mgr_base, _port_ctx.boot_reg_base);

	// return _port_ctx.port_fd;
	return 0;
}

s32 ipcm_port_uninit(void)
{
	if (0 == _init_cnt) {
		ipcm_warning("ipcm port has not been inited.\n");
		return -EFAULT;
	}

	_init_cnt--;
	if (_init_cnt) {
		ipcm_info("after decrease _init_cnt, _init_cnt %d not equal 0.\n", _init_cnt);
		return 0;
	}

	if (_port_ctx.rtos_base) {
		munmap(_port_ctx.rtos_base, _port_ctx.rtos_size);
		_port_ctx.rtos_base = NULL;
	}

	if (_port_ctx.pool_mgr_base) {
		munmap(_port_ctx.pool_mgr_base, _port_ctx.pool_mgr_capacity);
		_port_ctx.pool_mgr_base = NULL;
	}

	if (_port_ctx.boot_reg_base) {
		munmap(_port_ctx.boot_reg_base, AP_SYSTEM_REG_SIZE);
		_port_ctx.boot_reg_base = NULL;
	}

	if (_port_ctx.port_fd >= 0) {
		close(_port_ctx.port_fd);
		_port_ctx.port_fd = -1;
	}

	ipcm_info("ipcm_port_uninit success.\n");

	return 0;
}

POOLMGRHANDLE ipcm_port_get_pool_mgr_base(void)
{
	return _port_ctx.pool_mgr_base;
}

s32 ipcm_port_inv_data_by_offset(u32 pos, u32 size)
{
	IPCM_FLUSH_PARAM flush_param;

	flush_param.data_pos = pos;
	flush_param.len = size;
	return ioctl(_port_ctx.port_fd, IPCM_IOC_INV_DATA, &flush_param);
}

s32 ipcm_port_flush_data_by_offset(u32 pos, u32 size)
{
	IPCM_FLUSH_PARAM flush_param;

	flush_param.data_pos = pos;
	flush_param.len = size;
	return ioctl(_port_ctx.port_fd, IPCM_IOC_FLUSH_DATA, &flush_param);
}

s32 ipcm_port_inv_data(void *data, u32 size)
{
	u32 pos = 0;

	if (data == NULL) {
		ipcm_err("data is null.\n");
		return -EINVAL;
	}
	pos = data - _port_ctx.pool_mgr_base;

	return ipcm_port_inv_data_by_offset(pos, size);
}

s32 ipcm_port_flush_data(void *data, u32 size)
{
	u32 pos = 0;

	if (data == NULL) {
		ipcm_err("data is null.\n");
		return -EINVAL;
	}
	pos = data - _port_ctx.pool_mgr_base;

	return ipcm_port_flush_data_by_offset(pos, size);
}

s32 ipcm_port_data_lock(u8 lock_id)
{
	u8 id = lock_id%IPCM_DATA_SPIN_MAX;

	return ioctl(_port_ctx.port_fd, IPCM_IOC_LOCK, id);
}

s32 ipcm_port_data_unlock(u8 lock_id)
{
	u8 id = lock_id%IPCM_DATA_SPIN_MAX;

	return ioctl(_port_ctx.port_fd, IPCM_IOC_UNLOCK, id);
}


s32 ipcm_port_data_packed(POOLHANDLE handle, void *data, u32 len, MsgData *msg)
{
	if (data == NULL || msg == NULL) {
		ipcm_err("data(%p) or msg(%p) is null.\n", data, msg);
		return -EFAULT;
	}
	msg->msg_param.msg_ptr.data_pos = data - handle;
	msg->msg_param.msg_ptr.remaining_rd_len = len;
	ipcm_port_flush_data(data, len);
	return 0;
}

s32 ipcm_port_send_msg(MsgData *msg)
{
	if (msg == NULL) {
		ipcm_err("data is null.\n");
		return -1;
	}

	return write(_port_ctx.port_fd, msg, sizeof(MsgData));
}

int ipcm_port_get_shm_info(unsigned int *pool_mgr_paddr, unsigned int *pool_mgr_capacity)
{
	*pool_mgr_paddr = _port_ctx.pool_mgr_paddr;
	*pool_mgr_capacity = _port_ctx.pool_mgr_capacity;
	return 0;
}

void *ipcm_port_get_user_addr(u32 paddr)
{
    if (paddr < _port_ctx.rtos_paddr) {
        ipcm_err("paddr(%x) invalid, min is %x\n", paddr, _port_ctx.rtos_paddr);
        return 0;
    }
    return _port_ctx.rtos_base + (paddr - _port_ctx.rtos_paddr);
}

u32 ipcm_port_get_param_bin_addr(void)
{
	return *(u32 *)(_port_ctx.pool_mgr_base + _port_ctx.pool_mgr_capacity - 4*4);
}

u32 ipcm_port_get_param_bak_bin_addr(void)
{
	return *(u32 *)(_port_ctx.pool_mgr_base + _port_ctx.pool_mgr_capacity - 4*3);
}

u32 ipcm_port_get_pq_bin_addr(void)
{
	return *(u32 *)(_port_ctx.pool_mgr_base + _port_ctx.pool_mgr_capacity - 4*2);
}

s32 ipcm_set_rtos_boot_bit(RTOS_BOOT_STATUS_E stage, u8 stat)
{
	UNUSED(stage);
	UNUSED(stat);
    ipcm_warning("do not support set rtos boot status by linux.\n");
    return -EOPNOTSUPP;
}

s32 ipcm_get_rtos_boot_status(u32 *stat)
{
	if (stat == NULL) {
		ipcm_err("stat is null.\n");
		return -EINVAL;
	}

	if (_port_ctx.boot_reg_base) {
		*stat = *(u32 *)(_port_ctx.boot_reg_base + RTOS_BOOT_STATUS_OFFSET);
		return 0;
	}

	ipcm_err("boot_reg_base not been mapped.\n");
	return -EFAULT;
}

int ipcm_port_get_log_info(unsigned int *log_paddr, unsigned int *log_size)
{
	int ret = 0;
	IPCM_PADDR_INFO_S paddr_info;

	ret = ioctl(_port_ctx.port_fd , IPCM_IOC_GET_LOG_ADDR, &paddr_info);
	*log_paddr = paddr_info.paddr;
	*log_size = paddr_info.size;
	ipcm_info("ipcm log_paddr= 0x%x, size = 0x%x\n", paddr_info.paddr, paddr_info.size);
	return ret;
}
