/**
 * @file ipcm.c
 * @brief Core IPCM (Inter-Processor Communication Manager) implementation
 *
 * Implements the core functionality of the IPCM system including:
 * - Initialization and configuration
 * - Message routing and delivery
 * - Interrupt handling
 * - Resource management
 * - Error handling and recovery
 */

#include "ipcm_plat_adapter.h"

#include "cvi_spinlock.h"
#include "ipcm.h"
#include "ipcm_pool.h"
#include "cvi_mailbox.h"
#include "ipcm_dbg_rec_info.h"

// #define IPCM_DATA_SPIN_MAX (SPIN_MAX - SPIN_DATA + 1)

// Validate port type macro
#define IPCM_CHECK_PORT_ID(port_type, fail_ret) \
	do \
	{ \
		if ((port_type) >= PORT_BUTT) { \
			ipcm_err("port_type %d out of range,max is %d\n", (port_type), PORT_BUTT); \
			return (fail_ret); \
		} \
	} while (0)

// Synchronization primitives
static hw_raw_spinlock_t _lock[IPCM_DATA_SPIN_MAX];
static int _lock_flags[IPCM_DATA_SPIN_MAX];

// Port management
static IPCMHead ipcm_head[PORT_BUTT] = {};

static IPCMPA_MUTEX mailbox_mutex;

// Message handlers
static ipcm_pre_handle _m_pre_process = NULL;
static ipcm_pre_handle _m_pre_send = NULL;
static ipcm_pre_handle _m_send_hook   = NULL;

// Message statistics
static u32 _m_msg_recv_cnt = 0;
static u32 _m_msg_send_cnt = 0;

/**
 * @brief Mailbox receive handler
 *
 * Routes received messages to appropriate port handler after:
 * 1. Validating message format
 * 2. Extracting port information
 * 3. Calling pre-process handler if registered
 *
 * @param grp_id Group identifier
 * @param msg Message data
 * @param data Private handler data
 * @return Status code
 */
static s32 _ipcm_mb_recv_handle(u8 grp_id, void *msg, void *data)
{
	u8 port_type = 0;
	u8 port_id = 0;
	s32 ret = 0;

	ret = ipcm_get_port_id(grp_id, &port_type, &port_id);
	if (ret) {
		ipcm_err("ipcm get port type and id fail grp_id:%u ret:%d.\n", grp_id, ret);
		return ret;
	}
	IPCM_DBG_R_MSG_RECV(msg);
	_m_msg_recv_cnt++;
	if (_m_pre_process)
		_m_pre_process(grp_id, msg);
	if (ipcm_head[port_type].recv)
		return ipcm_head[port_type].recv(port_id, msg);
	return 0;
}

/**
 * @brief Initialize spinlocks
 *
 * Sets up hardware spinlocks for data synchronization
 */
static void ipcm_spin_lock_init(void)
{
	int i = 0;
	for (i=0; i<IPCM_DATA_SPIN_MAX; i++) {
		_lock[i].locks = __CVI_ARCH_SPIN_LOCK_UNLOCKED;
		_lock[i].hw_field = SPIN_DATA + i;
	}
}

/**
 * @brief Initialize IPCM system
 *
 * Sets up core IPCM functionality:
 * - Initializes mailbox hardware
 * - Sets up memory pools
 * - Registers interrupt handlers
 *
 * @param pool_mgr_paddr Physical address of pool manager
 * @param pool_mgr_capacity Size of pool manager region
 * @return Status code (0 on success, negative on error)
 */
s32 ipcm_init(u32 pool_mgr_paddr, u32 pool_mgr_capacity)
{
	s32 ret = 0;

	IPCMPA_MUTEX_INIT(&mailbox_mutex);
	mailbox_init(_ipcm_mb_recv_handle, NULL);
	cvi_spinlock_init();
	ipcm_spin_lock_init();
	ret = pool_mgr_init(pool_mgr_paddr, pool_mgr_capacity);
	if (ret) {
		ipcm_err("ipcm init fail,ret:%d\n", ret);
		return ret;
	}
	IPCM_DBG_R_MSG_INIT

	return ret;
}

/**
 * @brief Clean up IPCM system
 *
 * Releases IPCM resources:
 * - Cleans up mailbox
 * - Releases memory pools
 * - Unregisters handlers
 *
 * @return Status code (0 on success)
 */
s32 ipcm_uninit(void)
{
	IPCM_DBG_R_MSG_UNINIT
	IPCMPA_MUTEX_UNINIT(&mailbox_mutex);
	pool_mgr_uninit();
	return mailbox_uninit();
}

/**
 * @brief Register port message handler
 *
 * @param port_type Port type
 * @param handle Message handler function
 * @return Status code
 */
s32 ipcm_register_port_handle(PortType port_type, recv_notifier handle)
{
	s32 ret = 0;

	IPCM_CHECK_PORT_ID(port_type, -EINVAL);

	// ipcm_head[port_type].port_type = port_type;
	ipcm_head[port_type].recv = handle;

	return ret;
}

/**
 * @brief Deregister port message handler
 *
 * @param port_type Port type
 * @return Status code
 */
s32 ipcm_degister_port_handle(PortType port_type)
{
	IPCM_CHECK_PORT_ID(port_type, -EINVAL);

	ipcm_head[port_type].recv = NULL;

	return 0;
}

/**
 * @brief Register pre-receive message handler
 *
 * @param pre_process Handler to call before message processing
 * @return Status code
 */
s32 ipcm_register_irq_handle(ipcm_pre_handle pre_process)
{
	_m_pre_process = pre_process;
	return 0;
}

/**
 * @brief Register pre-send message handler
 *
 * @param handle Handler function to call before sending messages
 * @return Status code (0 on success, negative on error)
 */
s32 ipcm_register_pre_send_handle(ipcm_pre_handle pre_send)
{
	_m_pre_send = pre_send;
	return 0;
}

/**
 * @brief Register send hook
 *
 * @param send_hook Hook function to call before sending messages
 * @return Status code (0 on success, negative on error)
 */

s32 ipcm_register_send_hook(ipcm_pre_handle send_hook)
{
	_m_send_hook = send_hook;
	return 0;
}

/**
 * @brief Send message through IPCM
 *
 * Processes and sends message:
 * - Calls pre-send handler if registered
 * - Validates message parameters
 * - Transmits via mailbox hardware
 *
 * @param msg Message to send
 * @return Status code (0 on success, negative on error)
 */
s32 ipcm_send_msg(MsgData *data)
{
	s32 ret;

	if (data == NULL) {
		ipcm_err("data is null.\n");
		return -EFAULT;
	}

	if (_m_send_hook){
		ret = _m_send_hook(data->grp_id, data);
		if (ret) // msg send has been hooked
			return ret;
	}

	IPCM_DBG_R_MSG_SEND((void *)data);

	if (_m_pre_send) {
		_m_pre_send(data->grp_id, data);
	}
	IPCMPA_MUTEX_LOCK(&mailbox_mutex);
	// mailbox send
	ret = mailbox_send(data);
	_m_msg_send_cnt++;
	IPCMPA_MUTEX_UNLOCK(&mailbox_mutex);
	return ret;
}

/**
 * @brief Get count of received messages
 * @return Received message count
 */
u32 ipcm_get_recv_msg_cnt(void)
{
	return _m_msg_recv_cnt;
}

/**
 * @brief Get count of sent messages
 * @return Sent message count
 */
u32 ipcm_get_send_msg_cnt(void)
{
	return _m_msg_send_cnt;
}

/**
 * @brief Acquire data spinlock
 *
 * @param lock_id Lock identifier
 * @return 0 if lock acquired, negative on error
 */
s32 ipcm_data_spin_lock(u8 lock_id)
{
	if (lock_id >= IPCM_DATA_SPIN_MAX) {
		ipcm_debug("lock_id(%d) out of range, max is %d\n", lock_id, IPCM_DATA_SPIN_MAX-1);
		lock_id = lock_id % IPCM_DATA_SPIN_MAX;
	}
	drv_spin_lock_irqsave(&_lock[lock_id], _lock_flags[lock_id]);
	if (_lock_flags[lock_id] == MAILBOX_LOCK_FAILED) // lock failed
		return -1;
	return 0;
}

/**
 * @brief Release data spinlock
 *
 * @param lock_id Lock identifier
 * @return Status code
 */
s32 ipcm_data_spin_unlock(u8 lock_id)
{
	if (lock_id >= IPCM_DATA_SPIN_MAX) {
		ipcm_debug("lock_id(%d) out of range, max is %d\n", lock_id, IPCM_DATA_SPIN_MAX-1);
		lock_id = lock_id % IPCM_DATA_SPIN_MAX;
	}
	return drv_spin_unlock_irqrestore(&_lock[lock_id], _lock_flags[lock_id]);
}

/**
 * @brief Set target CPU for sending messages
 * @param cpu_id Target CPU identifier
 * @return Status code
 */
int ipcm_set_snd_cpu(int cpu_id)
{
	return mailbox_set_snd_cpu(cpu_id);
}

