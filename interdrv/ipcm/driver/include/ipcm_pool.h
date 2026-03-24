
/* ipcm pool implement header file
 *  ipcm pool provide Create,Destroy,ipcm_msg_get_buff,ipcm_msg_release_buff interface
 *  for example, if we create a pool has 4 blocks of 64bytes, 3 blocks of 128bytes,
 *   3 blocks of 512bytes, 2 blocks of 1024bytes, the memory design as follows:
 *           |----64bytes------|
 *           |----64bytes------|
 *           |----64bytes------|
 *           |----64bytes------|
 *           |----128bytes-----|
 *           |----128bytes-----|
 *           |----128bytes-----|
 *           |----512bytes-----|
 *           |----512bytes-----|
 *           |----512bytes-----|
 *           |----1024bytes----|
 *           |----1024bytes----|
 */

/**
 * @brief Memory pool management for IPCM
 *
 * Implements a memory pool system with variable sized blocks for efficient 
 * message buffer allocation. The pool is organized as follows:
 *
 * Memory Layout Example:
 * |----64bytes-----|  Block 1
 * |----64bytes-----|  Block 2 
 * |----128bytes----|  Block 3
 * |----512bytes----|  Block 4
 * |----1024bytes---|  Block 5
 */
#ifndef __IPCM_POOL__
#define __IPCM_POOL__

#include "ipcm_common.h"
#ifdef __riscv
#include "cvi_board_memmap.h"
#endif
#ifdef IPCM_INFO_REC
extern void pool_buff_release_hook(u32 pos);
#define POOL_BUF_RLS_HOOK(pos) pool_buff_release_hook(pos)
#else
#define POOL_BUF_RLS_HOOK(pos)
#endif

// Ipcm Pool MAGIC: I(0x49) P(0x50) M(0xdD) G(0x47)
#define IPCM_POOL_MAGIC 0x49504D47

// Memory region definitions
#define RTC_SRAM_ADDR 0x5200000     // RTC SRAM base address
#define RTC_SRAM_SIZE 0x6000        // RTC SRAM size

#define TPU_SRAM_IPCM_BASE		0xE000000
#ifdef __riscv
#define IPCM_POOL_ADDR  CVIMMAP_SHARE_MEM_ADDR
#define IPCM_POOL_SIZE  CVIMMAP_SHARE_MEM_SIZE
#define IPCM_RTOS_ADDR  CVIMMAP_RTOS_ION_ADDR
#define IPCM_RTOS_SIZE  CVIMMAP_RTOS_ION_SIZE
#define IPCM_LOG_ADDR  CVIMMAP_RTOS_LOG_ADDR
#define IPCM_LOG_SIZE  CVIMMAP_RTOS_LOG_SIZE
#endif

#define MAX_BLOCK_RANGE_NUM 8       // Maximum number of block size ranges
#define MAX_BLOCK_FLAG_SIZE 128     // Maximum block flag array size
#define MAX_BLOCK_TOTAL_NUM (32 * MAX_BLOCK_FLAG_SIZE)  // Maximum total blocks
#define MAX_POOL_SIZE (32*1024*1024) // Maximum pool size (32MB)

s32 pool_mgr_init(u32 pool_paddr, u32 pool_size);

s32 pool_mgr_uninit(void);

s32 pool_mgr_reset(void);

POOLHANDLE pool_mgr_get(u32 *pool_offset, u32 pool_id);

POOLHANDLE pool_create(u32 *pool_offset, PoolConfig *config, u32 pool_id);

s32 pool_destroy(POOLHANDLE handle);

s32 pool_reset(POOLHANDLE handle);

// return pos relative to head->data
u32 pool_alloc_offset(POOLHANDLE handle, u32 size);

s32 pool_free_by_offset(POOLHANDLE handle, u32 pos);

void *pool_alloc_buffer(POOLHANDLE handle, u32 size);

s32 pool_free_buffer(POOLHANDLE handle, void *data);

u32 pool_get_block_total(POOLHANDLE handle);

u32 pool_get_block_idx_by_offset(POOLHANDLE handle, u32 pos);

void *pool_get_data_by_offset(POOLHANDLE handle, u32 data_pos);

u32 pool_get_data_offset(POOLHANDLE handle, void *data);

u32 pool_get_shm_data_offset(POOLHANDLE handle);

void pool_print_info(POOLHANDLE handle, const char *str);

#endif // __IPCM_POOL__
