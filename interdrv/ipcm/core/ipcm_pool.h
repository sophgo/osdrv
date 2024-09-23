
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

#ifndef __IPCM_POOL__
#define __IPCM_POOL__

#include "ipcm_common.h"

#ifdef IPCM_INFO_REC
extern void pool_buff_release_hook(u32 pos);
#define POOL_BUF_RLS_HOOK(pos) pool_buff_release_hook(pos)
#else
#define POOL_BUF_RLS_HOOK(pos)
#endif

// Ipcm Pool MaGic: I(0x49) P(0x50) M(0xdD) G(0x47)
#define IPCM_POOL_MAGIC 0x49504D47

#define RTC_SRAM_ADDR 0x5200000
#define RTC_SRAM_SIZE 0x6000

#define TPU_SRAM_IPCM_BASE		0xE000000
#define IPCM_POOL_ADDR  0x87ec0000
#define IPCM_POOL_SIZE  0x20000
#define IPCM_FREERTOS_ADDR  0x83a00000
#define IPCM_FREERTOS_SIZE  0x4600000
// sram
// #define IPCM_POOL_ADDR  RTC_SRAM_ADDR
// #define IPCM_POOL_SIZE  RTC_SRAM_SIZE

#define MAX_BLOCK_RANGE_NUM 8
#define MAX_BLOCK_FLAG_SIZE 128
#define MAX_BLOCK_TOTAL_NUM (32 * MAX_BLOCK_FLAG_SIZE)
#define MAX_POOL_SIZE (32*1024*1024)

s32 pool_init(u32 pool_paddr, u32 pool_size);

s32 pool_uninit(void);

POOLHANDLE pool_create(BlockConfig *config, u32 num);

s32 pool_destroy(POOLHANDLE handle);

s32 pool_reset(POOLHANDLE handle);

// return pos relative to head->data
u32 pool_get_buff_to_pos(POOLHANDLE handle, u32 size);

s32 pool_release_buff_by_pos(POOLHANDLE handle, u32 pos);

void *pool_get_buff(POOLHANDLE handle, u32 size);

s32 pool_release_buff(POOLHANDLE handle, void *data);

u32 pool_get_block_total(POOLHANDLE handle);

u32 pool_get_block_idx_by_pos(POOLHANDLE handle, u32 pos);

void *pool_get_data_by_pos(POOLHANDLE handle, u32 data_pos);

u32 pool_get_data_pos(POOLHANDLE handle, void *data);

u32 pool_get_shm_data_pos(POOLHANDLE handle);

void pool_print_info(POOLHANDLE handle, const char *str);

#endif // __IPCM_POOL__
