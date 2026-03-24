/**
 * @file ipcm_port.c
 * @brief Port management implementation for IPCM
 *
 * Implements port-based communication channels between processors.
 * Features include:
 * - Multiple port types (message, system, custom)
 * - Port registration and deregistration
 * - Message routing between ports
 * - Synchronization and data protection
 */

#include "ipcm_plat_adapter.h"
#include "cvi_mailbox.h"
#include "ipcm.h"
#include "ipcm_pool.h"
#include "ipcm_port.h"

typedef struct _ipcm_port_ctx {
    POOLMGRHANDLE pool_mgr_base;
	IPCMPA_MUTEX data_lock[IPCM_DATA_SPIN_MAX];
	u32 pool_mgr_paddr;
	u32 pool_mgr_capacity;
	u32 rtos_paddr;
	u32 rtos_size;
	u8 open_recv_send_log;
	void *__IPCMPA_IOMEM rtos_stat_base;
} ipcm_port_ctx;

static ipcm_port_ctx _port_ctx = {};

static u32 _init_status = 0;

unsigned long long t_recv;

u32 ipcm_shared_addr;
u32 ipcm_shared_size;
u32 rtos_ion_addr;
u32 rtos_ion_size;
u32 rtos_log_addr;
u32 rtos_log_size;

s32 ipcm_release_buff_by_msg(void *msg)
{
	if (msg) {
		MsgData *msg_data = (MsgData *)msg;
		if (msg_data->func_type == MSG_TYPE_SHM) {
			pool_free_by_offset(_port_ctx.pool_mgr_base,
					msg_data->msg_param.msg_ptr.data_pos);
		}
		return 0;
	}
	return -EFAULT;
}

static s32 _ipcm_send_hook(u8 port_id, void *msg)
{
	u32 stat = 0;
	ipcm_get_rtos_boot_status(&stat);
	if (!(stat & (1 << RTOS_IPCM_DONE))) {
		ipcm_debug("alios may be in reset process, stat is %u\n", stat);
		// hooked, and free pool buff
		if (msg) {
			ipcm_release_buff_by_msg(msg);
		}
		return 1;
	}
	return 0;
}

static s32 _ipcm_irq_pre_process(u8 port_id, void *msg)
{
	if (_port_ctx.open_recv_send_log || ipcm_log_level_debug()) {
		if (msg)
			ipcm_info(PLATFORM_STRING" recv msg:%lx\n", *(unsigned long int *)msg);
		else {
			ipcm_warning(PLATFORM_STRING" recved msg, but msg is null.\n");
		}
	}
	t_recv = timer_get_boot_us();
	return 0;
}

static s32 _ipcm_send_pre_process(u8 port_id, void *msg)
{
	if (_port_ctx.open_recv_send_log || ipcm_log_level_debug()) {
		ipcm_info(PLATFORM_STRING" send msg:%lx\n", *(unsigned long int *)msg);
	}
	return 0;
}

/**
 * @brief Initialize port system
 *
 * Sets up port management:
 * - Initializes port contexts
 * - Sets up synchronization
 * - Clears handler tables
 *
 * @return Status code (0 on success, negative on error)
 */

s32 ipcm_port_init(void)
{
    int i = 0;
	s32 ret = 0;

	if (_init_status) {
		ipcm_warning("ipcm port has been inited.\n");
		return 0;
	}
#ifdef IPCM_POOL_ADDR
	_port_ctx.pool_mgr_paddr = IPCM_POOL_ADDR;
	_port_ctx.pool_mgr_capacity = IPCM_POOL_SIZE;
	_port_ctx.rtos_paddr = IPCM_RTOS_ADDR;
	_port_ctx.rtos_size =  IPCM_RTOS_SIZE;
#else
	_port_ctx.pool_mgr_paddr = ipcm_shared_addr;
	_port_ctx.pool_mgr_capacity = ipcm_shared_size;
	_port_ctx.rtos_paddr = rtos_ion_addr;
	_port_ctx.rtos_size =  rtos_ion_size;
	if (_port_ctx.pool_mgr_paddr == 0 || _port_ctx.pool_mgr_capacity == 0) {
		ipcm_err("ipcm shared mem info invalid.paddr:%x size:%x\n",
			_port_ctx.pool_mgr_paddr, _port_ctx.pool_mgr_capacity);
		return -EFAULT;
	}
#endif

	ipcmpa_sys_cache_invalidate(_port_ctx.pool_mgr_paddr, _port_ctx.pool_mgr_capacity);
	ret = ipcm_init(_port_ctx.pool_mgr_paddr, _port_ctx.pool_mgr_capacity);
	if (ret) {
		ipcm_err("ipcm_init failed.\n");
		return ret;
	}

	if (_port_ctx.rtos_stat_base == NULL) {
		_port_ctx.rtos_stat_base = ipcmpa_ioremap(RTOS_BOOT_STATUS_REG , 0x4);
		if(_port_ctx.rtos_stat_base == NULL)
			ipcm_err("rtos_stat_base ioremap %x failed!\n", RTOS_BOOT_STATUS_REG);
	}

	_port_ctx.pool_mgr_base = (POOLMGRHANDLE)(long)IPCMPA_MEMREMAP((unsigned long)_port_ctx.pool_mgr_paddr, _port_ctx.pool_mgr_capacity);

	for (i=0; i<IPCM_DATA_SPIN_MAX; i++) {
		IPCMPA_MUTEX_INIT(&_port_ctx.data_lock[i]);
	}

	ret = ipcm_register_irq_handle(_ipcm_irq_pre_process);

	ret = ipcm_register_pre_send_handle(_ipcm_send_pre_process);

	ret = ipcm_register_send_hook(_ipcm_send_hook);

	ipcm_set_rtos_boot_bit(RTOS_IPCM_DONE, 1);

	_init_status = 1;

	ipcm_debug("paddr.size %x.%x %x.%x\n", _port_ctx.pool_mgr_paddr, _port_ctx.pool_mgr_capacity,
		_port_ctx.rtos_paddr, _port_ctx.rtos_size);
	ipcm_debug("%s shm(%lx)\n", __func__, (unsigned long)_port_ctx.pool_mgr_base);

    return 0;
}

/**
 * @brief Clean up port system
 *
 * Releases port resources:
 * - Cleans up synchronization
 * - Resets handler tables
 *
 * @return Status code (0 on success)
 */
s32 ipcm_port_uninit(void)
{
	int i = 0;

	if (0 == _init_status) {
		ipcm_warning("ipcm port has not been inited.\n");
		return -EFAULT;
	}

	for (i=0; i<IPCM_DATA_SPIN_MAX; i++) {
		IPCMPA_MUTEX_UNINIT(&_port_ctx.data_lock[i]);
	}

	if (_port_ctx.rtos_stat_base)
		ipcmpa_iounmap(_port_ctx.rtos_stat_base);

	if (_port_ctx.pool_mgr_base)
		IPCMPA_MEMUNMAP(_port_ctx.pool_mgr_base);

	ipcm_register_send_hook(NULL);
	ipcm_register_irq_handle(NULL);
	ipcm_register_pre_send_handle(NULL);
        // IPCM_DBG_R_PROC_UNINIT

	ipcm_uninit();

	_init_status = 0;

    return 0;
}

/**
 * @brief Reset pool manager to initial state
 * @return Status code
 */
s32 ipcm_port_reset_pool_mgr(void)
{
	return pool_mgr_reset();
}

/**
 * @brief Get base address of pool manager
 * @return Pool manager handle
 */
POOLMGRHANDLE ipcm_port_get_pool_mgr_base(void)
{
	return _port_ctx.pool_mgr_base;
}

/**
 * @brief Set receive/send logging status
 * @param stat Enable (1) or disable (0) logging
 * @return Status code
 */
int ipcm_port_set_rs_log_stat(u8 stat)
{
	_port_ctx.open_recv_send_log = stat;
	return 0;
}

/**
 * @brief Invalidate data in cache
 * 
 * Ensures cache coherency by invalidating cached data
 *
 * @param data Virtual address of data
 * @param size Size of data to invalidate
 * @return Status code
 */
s32 ipcm_port_inv_data(void *data, u32 size)
{
	return ipcmpa_sys_cache_invalidate(_port_ctx.pool_mgr_paddr + (data-_port_ctx.pool_mgr_base), size);
}

/**
 * @brief Invalidate data in cache by offset
 *
 * @param offset Offset from pool base address
 * @param size Size of data to invalidate
 * @return Status code
 */
s32 ipcm_port_inv_data_by_offset(u32 offset, u32 size)
{
	return ipcmpa_sys_cache_invalidate(_port_ctx.pool_mgr_paddr + offset, size);
}

/**
 * @brief Flush data from cache to memory
 *
 * @param data Virtual address of data
 * @param size Size of data to flush
 * @return Status code
 */
s32 ipcm_port_flush_data(void *data, u32 size)
{
	return ipcmpa_sys_cache_flush(_port_ctx.pool_mgr_paddr + (data-_port_ctx.pool_mgr_base), size);
}

/**
 * @brief Flush data from cache by offset
 *
 * @param offset Offset from pool base address
 * @param size Size of data to flush
 * @return Status code
 */
s32 ipcm_port_flush_data_by_offset(u32 offset, u32 size)
{
	return ipcmpa_sys_cache_flush(_port_ctx.pool_mgr_paddr + offset, size);
}

/**
 * @brief Send message through port
 *
 * @param msg Message to send
 * @return Status code
 */
s32 ipcm_port_send_msg(MsgData *msg)
{
	if (msg == NULL) {
		ipcm_err("data is null.\n");
		return -1;
	}

	return ipcm_send_msg(msg);
}

/**
 * @brief Receive message from queue
 *
 * @param queue Message queue to receive from
 * @return Pointer to received message or NULL on error
 */
MsgData * ipcm_port_recv_msg(MsgQueue *queue)
{
	if (queue == NULL) {
		ipcm_err("queue is null.\n");
		return NULL;
	}

	if (queue_is_empty(queue)) {
		ipcm_err("queue is empty.\n");
		return NULL;
	}

	return queue_get_no_cpy(queue);
}

/**
 * @brief Convert physical address to user virtual address
 * @param paddr Physical address
 * @return Virtual address
 */
void *ipcm_port_get_user_addr(u32 paddr)
{
    return (void *)(long)paddr;
}

/**
 * @brief Get parameter binary address
 * @return Physical address of parameter binary
 */
u32 ipcm_port_get_param_bin_addr(void)
{
	return *(u32 *)(_port_ctx.pool_mgr_base + _port_ctx.pool_mgr_capacity - 4*4);
}

/**
 * @brief Get backup parameter binary address
 * @return Physical address of backup parameter binary
 */
u32 ipcm_port_get_param_bak_bin_addr(void)
{
	return *(u32 *)(_port_ctx.pool_mgr_base + _port_ctx.pool_mgr_capacity - 4*3);
}

/**
 * @brief Get PQ binary address
 * @return Physical address of PQ binary
 */
u32 ipcm_port_get_pq_bin_addr(void)
{
	return *(u32 *)(_port_ctx.pool_mgr_base + _port_ctx.pool_mgr_capacity - 4*2);
}

/**
 * @brief Get shared memory information
 *
 * @param pool_mgr_paddr Output parameter for pool manager physical address
 * @param pool_mgr_capacity Output parameter for pool manager capacity
 * @return Status code
 */
int ipcm_port_get_shm_info(unsigned int *pool_mgr_paddr, unsigned int *pool_mgr_capacity)
{
	*pool_mgr_paddr = _port_ctx.pool_mgr_paddr;
	*pool_mgr_capacity = _port_ctx.pool_mgr_capacity;
	return 0;
}

/**
 * @brief Get RTOS memory information
 *
 * @param rtos_paddr Output parameter for RTOS memory physical address
 * @param rtos_size Output parameter for RTOS memory size
 * @return Status code
 */
int ipcm_port_get_rtos_info(unsigned int *rtos_paddr, unsigned int *rtos_size)
{
	*rtos_paddr = _port_ctx.rtos_paddr;
	*rtos_size = _port_ctx.rtos_size;
	return 0;
}

/**
 * @brief Get log buffer information
 *
 * @param log_paddr Output parameter for log buffer physical address
 * @param log_size Output parameter for log buffer size
 * @return Status code
 */
int ipcm_port_get_log_info(unsigned int *log_paddr, unsigned int *log_size)
{
#ifdef IPCM_LOG_ADDR
	*log_paddr = IPCM_LOG_ADDR;
	*log_size  = IPCM_LOG_SIZE;
#else
	*log_paddr = rtos_log_addr;
	*log_size = rtos_log_size;
#endif
	return 0;
}

/**
 * @brief Lock data access for synchronization
 *
 * Acquires both mutex and spinlock for data access synchronization
 *
 * @param lock_id Lock identifier
 * @return Status code (0 on success, negative on error)
 */
s32 ipcm_port_data_lock(u8 lock_id)
{
	s32 ret = 0;
    u8 id = lock_id % IPCM_DATA_SPIN_MAX;

	IPCMPA_MUTEX_LOCK(&_port_ctx.data_lock[id]);
	ret = ipcm_data_spin_lock(id);
	if (ret)
		IPCMPA_MUTEX_UNLOCK(&_port_ctx.data_lock[id]);

	return ret;
}

/**
 * @brief Unlock data access
 *
 * Releases both spinlock and mutex for data access
 *
 * @param lock_id Lock identifier
 * @return Status code (0 on success, negative on error)
 */
s32 ipcm_port_data_unlock(u8 lock_id)
{
	s32 ret = 0;
    u8 id = lock_id % IPCM_DATA_SPIN_MAX;

	ret = ipcm_data_spin_unlock(id);
	if (!ret)
		IPCMPA_MUTEX_UNLOCK(&_port_ctx.data_lock[id]);

	return ret;
}

/**
 * @brief Pack data for message transmission
 *
 * Prepares data for transmission by:
 * 1. Validating input parameters
 * 2. Checking data is within shared memory
 * 3. Setting up message parameters
 * 4. Flushing data to memory
 *
 * @param handle Pool handle
 * @param data Data to pack
 * @param len Data length
 * @param msg Message to pack data into
 * @return Status code (0 on success, negative on error)
 */
s32 ipcm_port_data_packed(POOLHANDLE handle, void *data, u32 len, MsgData *msg)
{
	if (data == NULL || msg == NULL || handle == NULL) {
		ipcm_err("data or msg or handle is null %p %p %p.\n", data, msg, handle);
		return -EFAULT;
	}
	if ((data < (_port_ctx.pool_mgr_base)) ||
		(data > (_port_ctx.pool_mgr_base + _port_ctx.pool_mgr_capacity))) {
		ipcm_err("data(%lx) should from share memory(%lx-%lx).\n", (unsigned long)data,
			(unsigned long)_port_ctx.pool_mgr_base, 
			(unsigned long)(_port_ctx.pool_mgr_base + _port_ctx.pool_mgr_capacity));
		return -EINVAL;
	}
	msg->msg_param.msg_ptr.data_pos = pool_get_data_offset(handle, data);
	msg->msg_param.msg_ptr.remaining_rd_len = len;
	ipcm_port_flush_data(data, len);

    return 0;
}
IPCMPA_EXPORT_SYMBOL_GPL(ipcm_port_data_packed);

/**
 * @brief Get RTOS boot status
 *
 * Reads boot status from hardware register
 *
 * @param stat Output parameter for boot status
 * @return Status code (0 on success, negative on error)
 */
s32 ipcm_get_rtos_boot_status(u32 *stat)
{
    void * __IPCMPA_IOMEM rtos_stat_base = NULL;
    if (NULL == stat) {
        ipcm_err("stat is null.\n");
        return -EINVAL;
    }
    if (rtos_stat_base == NULL) {
        rtos_stat_base = ipcmpa_ioremap(RTOS_BOOT_STATUS_REG, 0x4);
        if (rtos_stat_base == NULL) {
            ipcm_err("rtos_stat_base ioremap %x failed!\n",
                     RTOS_BOOT_STATUS_REG);
            return -1;
        }
    }

    *stat = ipcmpa_ioread32(rtos_stat_base);
    ipcm_debug("rtos status = 0x%x\n", *stat);
	ipcmpa_iounmap(rtos_stat_base);
    return 0;
}

/**
 * @brief Set RTOS boot status bit
 *
 * Sets or clears specified boot status bit in hardware register
 *
 * @param stage Boot stage to update
 * @param stat Status value (1 to set, 0 to clear)
 * @return Status code (0 on success, negative on error)
 */
s32 ipcm_set_rtos_boot_bit(RTOS_BOOT_STATUS_E stage, u8 stat)
{
	void *__IPCMPA_IOMEM rtos_stat_base = NULL;
	u32 val;

	if (stage >= RTOS_BOOT_STATUS_BUTT) {
		ipcm_err("stage(%d) not equal, max(<%d)\n", stage, RTOS_BOOT_STATUS_BUTT);
		return -EINVAL;
	}

	rtos_stat_base = ipcmpa_ioremap(RTOS_BOOT_STATUS_REG , 0x4);
	if(rtos_stat_base == NULL) {
		ipcm_err("ipcmpa_ioremap %x failed!\n", RTOS_BOOT_STATUS_REG);
		return -1;
	}

	val = ipcmpa_ioread32(rtos_stat_base);
	if (stat) {
		val |= (1 << stage);
	} else {
		val &= (~(1<<stage));
	}

	ipcmpa_iowrite32(val, rtos_stat_base);
	ipcmpa_iounmap(rtos_stat_base);

	return 0;
}

/**
 * @brief Get message processing function for message ID
 *
 * Looks up message handler function in processing table
 *
 * @param msg_id Message identifier
 * @param proc_info Processing information structure
 * @return Function pointer to message handler, NULL if not found
 */
MSGPROC_FN port_get_msg_fn(u32 msg_id, msg_proc_info *proc_info)
{
    u32 i = 0;
    if (proc_info == NULL) {
        ipcm_err("proc_info is null\n");
        return NULL;
    }
    for (i=0; i < proc_info->func_amount; i++) {
        if (msg_id == proc_info->table[i].msg_id) {
            return proc_info->table[i].func;
        }
    }
    ipcm_warning("msg_id:%u not register func, port id:%u\n", msg_id, proc_info->port_id);
    return NULL;
}
