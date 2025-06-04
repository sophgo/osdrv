/**
 * @brief IPCM port management interface
 *
 * Defines port-related structures and functions for message routing and handling
 */

#ifndef __IPCM_PORT_H__
#define __IPCM_PORT_H__

#include "ipcm_common.h"
#include "msg_queue.h"

// Magic numbers for pool identification
#define IPCM_MSG_POOL_ID 0x4D534700   // "MSG\0"
#define IPCM_CUST_POOL_ID 0x43555354  // "CUST"

/**
 * @brief Message processing function type
 */
typedef s32 (*MSGPROC_FN)(void *msg, void *priv);

/**
 * @brief Message processor item structure
 */
typedef struct _msg_proc_item {
    u32 msg_id;         // Message identifier
    MSGPROC_FN func;    // Processing function
    ipcm_sem sem;       // Semaphore for synchronization
} msg_proc_item;

/**
 * @brief Message processor info structure
 */
typedef struct _msg_proc_info {
    u32 port_id;        // Port identifier
    u32 func_amount;    // Number of processing functions
    msg_proc_item *table; // Function table
} msg_proc_info;

/**
 * @brief System message identifiers
 */
typedef enum _IPCM_SYS_MSG_ID_E {
    IPCM_MSG_GET_SYSINFO,      // Get system information
    IPCM_MSG_GET_SYSINFO_RSP,  // System information response
    IPCM_MSG_GET_LOG,          // Get log data
    IPCM_MSG_GET_LOG_RSP,      // Log data response
    IPCM_SYS_MSG_BUTT
} IPCM_SYS_MSG_ID_E;

/**
 * @brief Get message processing function for given message ID
 * @param msg_id Message identifier
 * @param proc_info Processor info structure
 * @return Message processing function pointer
 */
MSGPROC_FN port_get_msg_fn(u32 msg_id, msg_proc_info *proc_info);

/**
 * @brief Initialize IPCM port system
 * @return Status code
 */
s32 ipcm_port_init(void);

/**
 * @brief Uninitialize IPCM port system
 * @return Status code
 */
s32 ipcm_port_uninit(void);

/**
 * @brief Reset pool manager
 * @return Status code
 */
s32 ipcm_port_reset_pool_mgr(void);

/**
 * @brief Get pool manager base handle
 * @return Pool manager handle
 */
POOLMGRHANDLE ipcm_port_get_pool_mgr_base(void);

/**
 * @brief Cache management functions
 */
s32 ipcm_port_inv_data(void *data, u32 size);

s32 ipcm_port_inv_data_by_offset(u32 offset, u32 size);

s32 ipcm_port_flush_data(void *data, u32 size);

s32 ipcm_port_flush_data_by_offset(u32 offset, u32 size);

/**
 * @brief Data locking functions
 */
s32 ipcm_port_data_lock(u8 lock_id);

s32 ipcm_port_data_unlock(u8 lock_id);

/**
 * @brief Message handling functions
 */
s32 ipcm_port_data_packed(POOLHANDLE handle, void *data, u32 len, MsgData *msg);

s32 ipcm_port_send_msg(MsgData *msg);

MsgData * ipcm_port_recv_msg(MsgQueue *queue);

/**
 * @brief Memory management functions
 */
void *ipcm_port_get_user_addr(u32 paddr);

int ipcm_port_get_shm_info(unsigned int *pool_mgr_paddr, unsigned int *pool_mgr_capacity);

int ipcm_port_get_rtos_info(unsigned int *rtos_paddr, unsigned int *rtos_size);

int ipcm_port_get_log_info(unsigned int *rtos_paddr, unsigned int *rtos_size);

/**
 * @brief Parameter binary access functions
 */
u32 ipcm_port_get_param_bin_addr(void);

u32 ipcm_port_get_param_bak_bin_addr(void);

u32 ipcm_port_get_pq_bin_addr(void);

/**
 * @brief RTOS boot status management
 */
s32 ipcm_set_rtos_boot_bit(RTOS_BOOT_STATUS_E stage, u8 stat);

s32 ipcm_get_rtos_boot_status(u32 *stat);
#endif
