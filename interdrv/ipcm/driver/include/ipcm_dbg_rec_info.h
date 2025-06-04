/**
 * @brief Debug and record information for IPCM
 *
 * Provides debugging and information recording functionality for IPCM operations
 */

#ifndef __IPCM_DBG_REC_INFO_H__
#define __IPCM_DBG_REC_INFO_H__

/**
 * @brief Processor support debug macros
 * 
 * When IPCM_PROC_SUPPORT is enabled, these macros map to actual debug functions.
 * Otherwise, they are defined as no-ops.
 */
#if IPCM_PROC_SUPPORT
#define IPCM_DBG_R_PROC_INIT(x) ipcm_dbg_r_proc_init(x);
#define IPCM_DBG_R_PROC_UNINIT ipcm_dbg_r_proc_uninit();
#define IPCM_DBG_R_BACKTRCE(a, b, c) ipcm_dbg_record_pool_bt(a, b, c)
#define IPCM_DBG_R_BACKTRCE_BY_POS(a, b, c) ipcm_dbg_record_pool_bt_bypos(a, b, c)
#define IPCM_DBG_PRINT_PROC ipcm_dbg_print_pool_proc

/**
 * @brief Initialize debug recording with given block total
 * @param block_total Total number of blocks to track
 */
void ipcm_dbg_r_proc_init(unsigned int block_total);

/**
 * @brief Uninitialize debug recording
 */
void ipcm_dbg_r_proc_uninit(void);

/**
 * @brief Record pool backtrace information
 * @param handle Pool handle
 * @param data Data pointer
 * @param get Get operation flag
 * @return Status code
 */
int ipcm_dbg_record_pool_bt(POOLHANDLE handle, void *data, unsigned char get);

/**
 * @brief Record pool backtrace by position
 * @param handle Pool handle
 * @param pos Position in pool
 * @param get Get operation flag
 * @return Status code
 */
int ipcm_dbg_record_pool_bt_bypos(POOLHANDLE handle, u32 pos, unsigned char get);

/**
 * @brief Print pool debug information
 * @return Status code
 */
int ipcm_dbg_print_pool_proc(void);

#else
// No-op definitions when IPCM_PROC_SUPPORT is disabled
#define IPCM_DBG_R_PROC_INIT(x) ((void)(x))
#define IPCM_DBG_R_PROC_UNINIT
#define IPCM_DBG_R_BACKTRCE(a, b, c)
#define IPCM_DBG_R_BACKTRCE_BY_POS(a, b, c)
#define IPCM_DBG_PRINT_PROC ipcm_dbg_print_pool_proc
#endif

/**
 * @brief Message recording macros and functions
 * 
 * When IPCM_INFO_REC is enabled, these provide message tracking functionality
 */
#ifdef IPCM_INFO_REC
#define IPCM_DBG_R_MSG_INIT ipcm_dbg_r_msg_init();
#define IPCM_DBG_R_MSG_UNINIT ipcm_dbg_r_msg_uninit();
#define IPCM_DBG_R_MSG_RECV(msg) ipcm_dbg_r_msg_recv(msg)
#define IPCM_DBG_R_MSG_SEND(msg) ipcm_dbg_r_msg_send(msg)

void ipcm_dbg_r_msg_init(void);
void ipcm_dbg_r_msg_uninit(void);
void ipcm_dbg_r_msg_recv(void *msg);
void ipcm_dbg_r_msg_send(void *msg);
#else
// No-op definitions when IPCM_INFO_REC is disabled
#define IPCM_DBG_R_MSG_INIT 
#define IPCM_DBG_R_MSG_UNINIT
#define IPCM_DBG_R_MSG_RECV(msg)
#define IPCM_DBG_R_MSG_SEND(msg)
#endif

/**
 * @brief Pool recording macros and functions
 * 
 * When both IPCM_INFO_REC_POOL and IPCM_INFO_REC are enabled,
 * these provide detailed pool operation tracking
 */
#if defined(IPCM_INFO_REC_POOL) && defined(IPCM_INFO_REC)
#define IPCM_DBG_R_POOL_INIT ipcm_dbg_r_pool_init();
#define IPCM_DBG_R_POOL_UNINIT ipcm_dbg_r_pool_uninit();
#define IPCM_DBG_R_POOL_T0(status) ipcm_dbg_r_pool_t0(status);
#define IPCM_DBG_R_POOL_T1(status) ipcm_dbg_r_pool_t1(status);
#define IPCM_DBG_R_POOL_T2(status) ipcm_dbg_r_pool_t2(status);
#define IPCM_DBG_R_POOL_T3(status) ipcm_dbg_r_pool_t3(status);
#define IPCM_DBG_R_POOL_DATA_POS(data_pos) ipcm_dbg_r_pool_data_offset(data_pos);
#define IPCM_DBG_R_POOL_BLOCK_IDX(block_idx) ipcm_dbg_r_pool_block_idx(block_idx);
#define IPCM_DBG_R_POOL_FUNC_TYPE(func_type) ipcm_dbg_r_pool_func_type(func_type);
#define IPCM_DBG_R_POOL_PUT_RING ipcm_dbg_r_pool_put_ring();

void ipcm_dbg_r_pool_init(void);
void ipcm_dbg_r_pool_uninit(void);
void ipcm_dbg_r_pool_t0(unsigned int status);
void ipcm_dbg_r_pool_t1(unsigned int status);
void ipcm_dbg_r_pool_t2(unsigned int status);
void ipcm_dbg_r_pool_t3(unsigned int status);
void ipcm_dbg_r_pool_data_offset(unsigned int data_pos);
void ipcm_dbg_r_pool_block_idx(unsigned char block_idx);
void ipcm_dbg_r_pool_func_type(unsigned char func_type);
void ipcm_dbg_r_pool_put_ring(void);
#else
// No-op definitions when pool recording is disabled
#define IPCM_DBG_R_POOL_INIT
#define IPCM_DBG_R_POOL_UNINIT
#define IPCM_DBG_R_POOL_T0(status)
#define IPCM_DBG_R_POOL_T1(status)
#define IPCM_DBG_R_POOL_T2(status)
#define IPCM_DBG_R_POOL_T3(status)
#define IPCM_DBG_R_POOL_DATA_POS(data_pos)
#define IPCM_DBG_R_POOL_BLOCK_IDX(block_idx)
#define IPCM_DBG_R_POOL_FUNC_TYPE(func_type)
#define IPCM_DBG_R_POOL_PUT_RING
#endif

#endif /* __IPCM_DBG_REC_INFO_H__ */
