
// #define _DEBUG

#ifdef __LINUX_DRV__
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/mutex.h>
// #include <asm-generic/io.h>
#include <asm/io.h>
#include <asm-generic/errno.h>
#endif

#ifdef __ALIOS__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aos/kernel.h>
#include <errno.h>
#include "vip_spinlock.h"
#include "asm/barrier.h"
#endif

#include "ipcm_pool.h"

#if defined(IPCM_INFO_REC) && defined(IPCM_INFO_REC_POOL)
#include "ring.h"
#endif

#include "cvi_spinlock.h"
DEFINE_CVI_SPINLOCK(pool_lock, SPIN_SHM);

static ipcm_mutex_t pool_buf_mutex;

#define POOL_ALIGN_SIZE 64

typedef struct _BlockInfo {
	u32 size;
	u32 num;
	u32 start_pos;
} BlockInfo;

typedef struct _PoolHead {
	u32 magic;
	BlockInfo block_info[MAX_BLOCK_RANGE_NUM];
	u32 block_status[MAX_BLOCK_FLAG_SIZE];
	u32 block_range_num;
	u32 block_total;
	u32 data; // data pos relative to head
	u32 len; //data len
} PoolHead;

#if defined(IPCM_INFO_REC) && defined(IPCM_INFO_REC_POOL)
#define RING_NUM 128
typedef struct _PoolGetRlsRingItem {
	u32 t0;
	u32 status0_rec0;
	u32 t1;
	u32 status0_rec1;
	u32 t2;
	u32 status0_rec2;
	u32 t3;
	u32 status0_rec3;
	u32 data_pos;
	u8 block_idx;
	u8 func_type; // 0:rls pool buf  1:get pool buff
} PoolGetRlsRingItem;
static IPCMRing *_pool_get_rls_ring = NULL;
#endif

static u32 s_pool_paddr = IPCM_POOL_ADDR;
static u32 s_pool_size = IPCM_POOL_SIZE;

#if defined(IPCM_INFO_REC) && defined(IPCM_INFO_REC_POOL)
static void _print_ring_snap_get_rls_pool(IPCMRing *ring)
{
	PoolGetRlsRingItem *ring_data = NULL;
	u32 rear;
	int i = 0;

	if (NULL == ring) {
		return;
	}

	ring_data = ipcm_alloc(RING_NUM * sizeof(PoolGetRlsRingItem));
	ring_snap(ring, (void **)&ring_data, &rear);
	ipcm_info("rear:%d\n", rear);
	for (i=0; i<RING_NUM; i++) {
#ifdef __LINUX_DRV__
		PR(KERN_CONT "%4d %4x %4d %4x %4d %4x %4d %4x %4d %d %d,\n", ring_data[i].t0, ring_data[i].status0_rec0,
			ring_data[i].t1, ring_data[i].status0_rec1, ring_data[i].t2, ring_data[i].status0_rec2,
			ring_data[i].t3, ring_data[i].status0_rec3, ring_data[i].data_pos, ring_data[i].block_idx, ring_data[i].func_type);
#endif
#ifdef __ALIOS__
		PR("%4d %4x %4d %4x %4d %4x %4d %4x %4d %d %d,\n", ring_data[i].t0, ring_data[i].status0_rec0,
			ring_data[i].t1, ring_data[i].status0_rec1, ring_data[i].t2, ring_data[i].status0_rec2,
			ring_data[i].t3, ring_data[i].status0_rec3, ring_data[i].data_pos, ring_data[i].block_idx, ring_data[i].func_type);
#endif
	}

	ipcm_free(ring_data);
}

void dbg_print_pool_get_rls_ring(void)
{
	_print_ring_snap_get_rls_pool(_pool_get_rls_ring);
}
#endif

static void wait_block_release(POOLHANDLE handle)
{
	PoolHead *head = (PoolHead *)handle;
	int i = 0;
	u32 block_flag_num = 0;

	if (head == NULL) {
		ipcm_err("handle is null.\n");
		return;
	}

	block_flag_num = (head->block_total/32) + 1;
	while (1) {
		i = 0;
		while (i < block_flag_num) {
			if (head->block_status[i])
				break;
			i++;
		}

		if (i == block_flag_num)
			break;
		ipcm_msleep(10);
	}
}

s32 pool_init(u32 pool_paddr, u32 pool_size)
{
	s_pool_paddr = pool_paddr;
	s_pool_size = pool_size;
	ipcm_mutex_init(&pool_buf_mutex);
#if defined(IPCM_INFO_REC) && defined(IPCM_INFO_REC_POOL)
	{
		u32 ring_item_size = sizeof(PoolGetRlsRingItem);
		_pool_get_rls_ring = ipcm_alloc(sizeof(IPCMRing) + ring_item_size * RING_NUM);
		ipcm_warning("_pool_get_rls_ring addr:%px\n", (void *)_pool_get_rls_ring);
		memset(_pool_get_rls_ring, 0, sizeof(IPCMRing) + ring_item_size * RING_NUM);
		ring_init(_pool_get_rls_ring, ring_item_size, RING_NUM);
	}
#endif
	return 0;
}

s32 pool_uninit(void)
{
#if defined(IPCM_INFO_REC) && defined(IPCM_INFO_REC_POOL)
	if (_pool_get_rls_ring) {
		ring_uninit(_pool_get_rls_ring);
		ipcm_free(_pool_get_rls_ring);
		_pool_get_rls_ring = NULL;
	}
#endif
	ipcm_mutex_uninit(&pool_buf_mutex);
	return 0;
}

POOLHANDLE pool_create(BlockConfig *config, u32 num)
{
	u32 block_total = 0;
	u32 pool_len = 0;
	void *data;
	u32 pool_size = 0;
	PoolHead *head;
	u32 pos = 0;
	u32 size = 0;
	int i;

	if (num > MAX_BLOCK_RANGE_NUM) {
		ipcm_err("block range num out of range,block range num is %d\n", num);
		return NULL;
	}
	for (i = 0; i < num; i++) {
		block_total += config[i].num;
		pool_len += (ALIGN(config[i].size, POOL_ALIGN_SIZE) * config[i].num);
	}
	if (block_total > MAX_BLOCK_TOTAL_NUM) {
		ipcm_err("block total out of range, total is %d\n", block_total);
		return NULL;
	}

	ipcm_debug("pool len is %d\n", pool_len);
	pool_size = ALIGN((pool_len+ALIGN(sizeof(PoolHead), POOL_ALIGN_SIZE)), 128);
	if (pool_size > MAX_POOL_SIZE) {
		ipcm_err("pool size out of range, pool size is %d,max %d\n", pool_size, MAX_POOL_SIZE);
		return NULL;
	}
	ipcm_debug("sizeof(PoolHead) is %zu\n", sizeof(PoolHead));
	ipcm_debug("pool size is %d\n", pool_size);
	// data = ipcm_alloc(pool_size);
	// if (NULL == data) {
	//     ipcm_err("pool data alloc fail.\n");
	//     return NULL;
	// }
	#ifdef __ALIOS__
	data = (void *)(long)s_pool_paddr;
	#else
	data = phys_to_virt(s_pool_paddr);
	#endif

	// init head
	head = (PoolHead *)data;
	head->magic = IPCM_POOL_MAGIC;
	head->block_range_num = num;
	head->block_total = block_total;
	head->len = pool_len;
	head->data = ALIGN(sizeof(PoolHead), POOL_ALIGN_SIZE); // align 64 bytes
	for (i = 0; i < num; i++) {
		size = ALIGN(config[i].size, POOL_ALIGN_SIZE); // align 64 bytes
		head->block_info[i].size = size;
		head->block_info[i].num = config[i].num;
		head->block_info[i].start_pos = pos;
		pos += (size * config[i].num);
	}
	memset(head->block_status, 0, sizeof(head->block_status));

	ipcm_pool_cache_flush(s_pool_paddr, NULL, s_pool_size);

	ipcm_debug("pool(%lx) create, head:\n", (unsigned long)data);
	#ifdef _DEBUG
	pool_print_info(data, NULL);
	#endif

	return data;
}

s32 pool_destroy(POOLHANDLE handle)
{
	// void *data = handle;
	wait_block_release(handle);
	// return ipcm_free(data);
	return 0;
}

s32 pool_reset(POOLHANDLE handle)
{
	PoolHead *head = (PoolHead *)handle;
	int flags;
	int flush_pos = 0;
	int flush_size = 0;

	flush_pos = ALIGN_DOWN(sizeof(head->block_info), IPCM_CACHE_ALIGN_SIZE);
	flush_size = sizeof(head->block_status) + (sizeof(head->block_info) - flush_pos);
	flush_size = ALIGN(flush_size, IPCM_CACHE_ALIGN_SIZE);

	drv_spin_lock_irqsave_ext(&pool_lock, flags);
	ipcm_pool_cache_invalidate(s_pool_paddr+flush_pos, handle+flush_pos, flush_size);
	memset(&head->block_status, 0, sizeof(head->block_status));
	ipcm_pool_cache_flush(s_pool_paddr+flush_pos, handle+flush_pos, flush_size);
	drv_spin_unlock_irqrestore_ext(&pool_lock, flags);

	return 0;
}

u32 pool_get_buff_to_pos(POOLHANDLE handle, u32 size)
{
	PoolHead *head = (PoolHead *)handle;
	u32 block_range_start = 0;
	u32 block_start = 0;
	u32 block_fit = 0;
	u32 data_pos = 0;
	int i = 0;
	int flags;
	int flush_pos = 0;
	int flush_size = 0;
#if defined(IPCM_INFO_REC) && defined(IPCM_INFO_REC_POOL)
	PoolGetRlsRingItem item;
#endif

	if (head == NULL) {
		ipcm_err("handle is null.\n");
		return U32_MAX;
	}

	ipcm_debug("%s handle(%lx)\n", __func__, (unsigned long)handle);

	for (i = 0; i < head->block_range_num; i++) {
		if (size <= head->block_info[i].size)
			break;
		block_start += head->block_info[i].num;
	}

	if (i == head->block_range_num) {
		ipcm_err("i(%d) size(%d) out of range, max(%d)\n", i, size, head->block_info[i-1].size);
		return U32_MAX;
	}

	block_range_start = i;
	// ipcm_info("size %d in range idx %d\n", size, i);
	// ipcm_info("blk_start(%d) blk_total(%d)\n", block_start, head->block_total);
	#ifdef _DEBUG
	pool_print_info(handle, "bf get buf pool info:");
	#endif

	flush_pos = ALIGN_DOWN(sizeof(head->block_info), IPCM_CACHE_ALIGN_SIZE);
	flush_size = sizeof(head->block_status) + (sizeof(head->block_info) - flush_pos);
	flush_size = ALIGN(flush_size, IPCM_CACHE_ALIGN_SIZE);

	ipcm_mutex_lock(&pool_buf_mutex);
	drv_spin_lock_irqsave_ext(&pool_lock, flags);
	if (flags == MAILBOX_LOCK_FAILED) {
		ipcm_mutex_unlock(&pool_buf_mutex);
		return U32_MAX;
	}
#if defined(IPCM_INFO_REC) && defined(IPCM_INFO_REC_POOL)
	item.func_type = 1;
	item.t0 = timer_get_boot_us();
	item.status0_rec0 = head->block_status[0];
#endif
	ipcm_pool_cache_invalidate(s_pool_paddr+flush_pos, handle+flush_pos, flush_size);
#if defined(IPCM_INFO_REC) && defined(IPCM_INFO_REC_POOL)
	item.t1 = timer_get_boot_us();
	item.status0_rec1 = head->block_status[0];
#endif
	for (i = block_start; i < head->block_total; i++) {
		if (!((head->block_status[i/32] >> (i%32)) & 1))
			break;
	}
	if (i == head->block_total) {
		drv_spin_unlock_irqrestore_ext(&pool_lock, flags);
		ipcm_mutex_unlock(&pool_buf_mutex);
		ipcm_err("no free buff fit.\n");
		return U32_MAX;
	}
	block_fit = i;
	ipcm_debug("blk_fit(%d)\n", block_fit);
#if defined(IPCM_INFO_REC) && defined(IPCM_INFO_REC_POOL)
	item.block_idx = block_fit;
#endif
	head->block_status[i/32] |= (1 << (i%32));
	// *(volatile u32 *)&(head->block_status[i/32]) |= (1 << (i%32));
#if defined(IPCM_INFO_REC) && defined(IPCM_INFO_REC_POOL)
	item.t2 = timer_get_boot_us();
	item.status0_rec2 = head->block_status[0];
#endif
	ipcm_pool_cache_flush(s_pool_paddr+flush_pos, handle+flush_pos, flush_size);
#if defined(IPCM_INFO_REC) && defined(IPCM_INFO_REC_POOL)
	item.t3 = timer_get_boot_us();
	item.status0_rec3 = head->block_status[0];
#endif
	drv_spin_unlock_irqrestore_ext(&pool_lock, flags);
	ipcm_mutex_unlock(&pool_buf_mutex);

	#ifdef _DEBUG
	pool_print_info(handle, "af get buf pool info:");
	#endif

	for (i = block_range_start; i < head->block_range_num; i++) {
		if ((block_fit-block_start) < head->block_info[i].num)
			break;
		block_start += head->block_info[i].num;
	}

	data_pos = head->block_info[i].start_pos + (head->block_info[i].size * (block_fit-block_start));
#if defined(IPCM_INFO_REC) && defined(IPCM_INFO_REC_POOL)
	item.data_pos = data_pos;
	ring_put(_pool_get_rls_ring, &item);
#endif
	ipcm_debug("size(%d) range(%d) rang_start(%d) pos(%d) blk_fit(%d) blk_start(%d)\n", size, i,
		head->block_info[i].start_pos, data_pos, block_fit, block_start);

	return data_pos;
}

void *pool_get_buff(POOLHANDLE handle, u32 size)
{
	PoolHead *head = (PoolHead *)handle;
	void *data;
	u32 data_pos = 0;

	if (head == NULL) {
		ipcm_err("handle is null.\n");
		return NULL;
	}

	data_pos = pool_get_buff_to_pos(handle, size);
	if (data_pos == U32_MAX) {
		ipcm_err("pool_get_buff_to_pos fail.\n");
		return NULL;
	}

	data = handle + head->data + data_pos;
	return data;
}

s32 pool_release_buff_by_pos(POOLHANDLE handle, u32 pos)
{
	PoolHead *head = (PoolHead *)handle;
	u32 block_idx = 0;
	int flags;
	int flush_pos = 0;
	int flush_size = 0;
#if defined(IPCM_INFO_REC) && defined(IPCM_INFO_REC_POOL)
	PoolGetRlsRingItem item;
#endif

	if (head == NULL) {
		ipcm_err("handle is null.\n");
		return -EFAULT;
	}
	if (pos >= head->len) {
		ipcm_err("pos %d out of range", pos);
		return -EINVAL;
	}

	POOL_BUF_RLS_HOOK(pos);

	block_idx = pool_get_block_idx_by_pos(handle, pos);

	#ifdef _DEBUG
	pool_print_info(handle, "bf rls buf pool info:");
	#endif

#if defined(IPCM_INFO_REC) && defined(IPCM_INFO_REC_POOL)
	item.func_type = 0;
	item.data_pos = pos;
#endif

	flush_pos = ALIGN_DOWN(sizeof(head->block_info), IPCM_CACHE_ALIGN_SIZE);
	flush_size = sizeof(head->block_status) + (sizeof(head->block_info) - flush_pos);
	flush_size = ALIGN(flush_size, IPCM_CACHE_ALIGN_SIZE);
	ipcm_mutex_lock(&pool_buf_mutex);
	drv_spin_lock_irqsave_ext(&pool_lock, flags);
	if (flags == MAILBOX_LOCK_FAILED) {
		ipcm_mutex_unlock(&pool_buf_mutex);
		return -EINVAL;
	}
#if defined(IPCM_INFO_REC) && defined(IPCM_INFO_REC_POOL)
	item.t0 = timer_get_boot_us();
	item.status0_rec0 = head->block_status[0];
#endif
	ipcm_pool_cache_invalidate(s_pool_paddr+flush_pos, handle+flush_pos, flush_size);
#if defined(IPCM_INFO_REC) && defined(IPCM_INFO_REC_POOL)
	item.t1 = timer_get_boot_us();
	item.status0_rec1 = head->block_status[0];
	item.block_idx = block_idx;
#endif
	head->block_status[block_idx/32] &= (~(1<<(block_idx%32)));
	// *(volatile u32 *)&(head->block_status[block_idx/32]) &= (~(1<<(block_idx%32)));
#if defined(IPCM_INFO_REC) && defined(IPCM_INFO_REC_POOL)
	item.t2 = timer_get_boot_us();
	item.status0_rec2 = head->block_status[0];
#endif
	ipcm_pool_cache_flush(s_pool_paddr+flush_pos, handle+flush_pos, flush_size);
#if defined(IPCM_INFO_REC) && defined(IPCM_INFO_REC_POOL)
	item.t3 = timer_get_boot_us();
	item.status0_rec3 = head->block_status[0];
#endif
	drv_spin_unlock_irqrestore_ext(&pool_lock, flags);
	ipcm_mutex_unlock(&pool_buf_mutex);
#if defined(IPCM_INFO_REC) && defined(IPCM_INFO_REC_POOL)
	ring_put(_pool_get_rls_ring, &item);
#endif

	#ifdef _DEBUG
	pool_print_info(handle, "after rls buf pool info:");
	#endif
	return 0;
}

s32 pool_release_buff(POOLHANDLE handle, void *data)
{
	PoolHead *head = (PoolHead *)handle;
	u32 data_pos;

	if (head == NULL) {
		ipcm_err("handle is null.\n");
		return -EFAULT;
	}
	if ((data < (handle+head->data)) ||
		(data >= (handle + head->data + head->len))) {
		ipcm_err("data %lx out of range\n", (unsigned long)data);
		return -EINVAL;
	}
	data_pos = data - handle - head->data;
	return pool_release_buff_by_pos(handle, data_pos);
}

u32 pool_get_block_total(POOLHANDLE handle)
{
	PoolHead *head = (PoolHead *)handle;

	if (head == NULL) {
		ipcm_err("handle is null.\n");
		return 0;
	}
	return head->block_total;
}

u32 pool_get_block_idx_by_pos(POOLHANDLE handle, u32 pos)
{
	PoolHead *head = (PoolHead *)handle;
	u32 range_idx = 0;
	u32 block_idx = 0;
	int i = 0;

	if (head == NULL) {
		ipcm_err("handle is null.\n");
		return 0;
	}

	for (i = 1; i < head->block_range_num; i++) {
		if (pos < head->block_info[i].start_pos)
			break;
		block_idx += head->block_info[i-1].num;
	}
	range_idx = i-1;
	block_idx += (pos - head->block_info[range_idx].start_pos)/head->block_info[range_idx].size;

	ipcm_debug("pool_get_block_idx_by_pos pos(%d) range_idx(%d), block_idx(%d)\n", pos, range_idx, block_idx);

	return block_idx;
}

void *pool_get_data_by_pos(POOLHANDLE handle, u32 data_pos)
{
	PoolHead *head = (PoolHead *)handle;

	if (head == NULL) {
		ipcm_err("handle is null.\n");
		return NULL;
	}

	return handle + head->data + data_pos;
}

u32 pool_get_data_pos(POOLHANDLE handle, void *data)
{
	PoolHead *head = (PoolHead *)handle;

	if (head == NULL) {
		ipcm_err("handle is null.\n");
		return U32_MAX;
	}
	return data - handle - head->data;
}

u32 pool_get_shm_data_pos(POOLHANDLE handle)
{
	PoolHead *head = (PoolHead *)handle;

	if (head == NULL) {
		ipcm_err("handle is null.\n");
		return U32_MAX;
	}

	return head->data;
}

void pool_print_info(POOLHANDLE handle, const char *str)
{
	PoolHead *head = (PoolHead *)handle;
	int i = 0;

	if (head == NULL) {
		ipcm_err("handle is null.\n");
		return;
	}

	if (head->magic != IPCM_POOL_MAGIC) {
		ipcm_err("pool magic(%x) not vaild, expected(%x)\n", head->magic, IPCM_POOL_MAGIC);
		return;
	}

	if (str) {
		ipcm_info("%s\n", str);
	}

	ipcm_info("pool info:\n\taddr(%lx) range num(%d) total blk(%d) data pos(%d) len(%d)\n\tblock info:\n",
		(unsigned long)handle, head->block_range_num, head->block_total, head->data, head->len);

	for (i = 0; i < head->block_range_num; i++) {
		ipcm_info("\tidx(%d) size(%d) start pos(%d) num(%d)\n", i, head->block_info[i].size,
			head->block_info[i].start_pos, head->block_info[i].num);
	}

	for (i = 0; i < 8; i++) {
		ipcm_info("\t%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
			head->block_status[i*16+0], head->block_status[i*16+1], head->block_status[i*16+2],
			head->block_status[i*16+3], head->block_status[i*16+4], head->block_status[i*16+5],
			head->block_status[i*16+6], head->block_status[i*16+7], head->block_status[i*16+8],
			head->block_status[i*16+9], head->block_status[i*16+10], head->block_status[i*16+11],
			head->block_status[i*16+12], head->block_status[i*16+13], head->block_status[i*16+14],
			head->block_status[i*16+15]);
	}
}
