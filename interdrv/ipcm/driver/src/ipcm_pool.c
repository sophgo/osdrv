/**
 * @file ipcm_pool.c
 * @brief Memory pool implementation for IPCM shared memory
 *
 * Implements a shared memory pool system for message passing between processors.
 * Features include:
 * - Variable sized memory blocks
 * - Thread-safe allocation/deallocation
 * - Memory tracking and statistics
 * - Debug support
 */

// #define _DEBUG
#include "ipcm_plat_adapter.h"

#include "ipcm_pool.h"

#include "ipcm_dbg_rec_info.h"

#include "cvi_spinlock.h"
DEFINE_CVI_SPINLOCK(pool_lock, SPIN_SHM);

static IPCMPA_MUTEX pool_buf_mutex;

// Pool manager magic number for validation
#define POOL_MGR_MAGIC 0x12345678


// Alignment requirements for pool blocks
#define POOL_ALIGN_SIZE 64

#define MAX_POOL_NUM 30
typedef struct _PoolManagerItem {
	u32 addr_offset;
	u32 size;
} PoolManagerItem;

typedef struct _PoolManager {
	u32 magic;
	u32 pool_num;
	u32 pool_start_pos;
	u32 pool_end_pos;
	PoolManagerItem pool_list[MAX_POOL_NUM];
} PoolManager;

typedef struct _BlockInfo {
	u32 size;
	u32 num;
	u32 start_pos; // pos relative to pool head
} BlockInfo;

/**
 * @brief Pool header structure
 */
typedef struct _PoolHead {
	u32 magic;
	u32 id;
	u32 paddr;
	BlockInfo block_info[MAX_BLOCK_RANGE_NUM];
	u32 block_status[MAX_BLOCK_FLAG_SIZE];
	u32 block_range_num;
	u32 block_total;
	u32 data; // data pos relative to head
	u32 len; //data len
} PoolHead;

static u32 s_region_paddr;
static u32 s_region_size;

PoolManager *s_pmgr = NULL;
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

/**
 * @brief Initialize pool manager
 *
 * Sets up pool management system:
 * - Maps shared memory region
 * - Initializes pool manager structure
 * - Sets up synchronization
 *
 * @param region_paddr Physical address of shared memory region
 * @param region_size Size of shared memory region
 * @return Status code (0 on success, negative on error)
 */
s32 pool_mgr_init(u32 region_paddr, u32 region_size)
{
	int flags;

	s_region_paddr = region_paddr;
	s_region_size = region_size;
	s_pmgr = (PoolManager *)ipcmpa_phys_to_virt(s_region_paddr);
	if (!s_pmgr) {
		ipcm_err("pool mgr init fail!\n");
		return -1;
	}
	ipcm_debug("pool mgr init start!\n");
	drv_spin_lock_irqsave_ext(&pool_lock, flags);
	ipcm_pool_cache_invalidate(region_paddr, NULL, ALIGN(sizeof(PoolManager), POOL_ALIGN_SIZE));
	if (s_pmgr->magic != POOL_MGR_MAGIC) {
		ipcm_debug("pool mgr not inited, init now!\n");
		s_pmgr->magic = POOL_MGR_MAGIC;
		s_pmgr->pool_num = 0;
		s_pmgr->pool_start_pos = ALIGN(sizeof(PoolManager), POOL_ALIGN_SIZE);
		s_pmgr->pool_end_pos = region_size;
		ipcm_pool_cache_flush(region_paddr, NULL, s_pmgr->pool_start_pos);
	}
	drv_spin_unlock_irqrestore_ext(&pool_lock, flags);
	IPCMPA_MUTEX_INIT(&pool_buf_mutex);
	IPCM_DBG_R_POOL_INIT
	return 0;
}

s32 pool_mgr_uninit(void)
{
	IPCM_DBG_R_POOL_UNINIT
	IPCMPA_MUTEX_UNINIT(&pool_buf_mutex);
	return 0;
}

s32 pool_mgr_reset(void)
{
	int flags;

	drv_spin_lock_irqsave_ext(&pool_lock, flags);
	ipcm_pool_cache_invalidate(s_region_paddr, NULL, ALIGN(sizeof(PoolManager), POOL_ALIGN_SIZE));
	if (!s_pmgr || s_pmgr->magic != POOL_MGR_MAGIC) {
		ipcm_err("pool mgr not inited!\n");
		drv_spin_unlock_irqrestore_ext(&pool_lock, flags);
	}
	s_pmgr->magic = POOL_MGR_MAGIC;
	s_pmgr->pool_num = 0;
	s_pmgr->pool_start_pos = ALIGN(sizeof(PoolManager), POOL_ALIGN_SIZE);
	s_pmgr->pool_end_pos = s_region_size;
	ipcm_pool_cache_flush(s_region_paddr, NULL, s_pmgr->pool_start_pos);
	drv_spin_unlock_irqrestore_ext(&pool_lock, flags);
	return 0;
}

s32 pool_mgr_alloc(u32 size)
{
	u32 start_pos = s_pmgr->pool_start_pos;
	u32 end_pos = s_pmgr->pool_end_pos;
	int i = 0;
	int flags;
	int ret = -1;

	drv_spin_lock_irqsave_ext(&pool_lock, flags);
	ipcm_pool_cache_invalidate(s_region_paddr, NULL, ALIGN(sizeof(PoolManager), POOL_ALIGN_SIZE));
	do {
		if (s_pmgr->magic != POOL_MGR_MAGIC) {
			ipcm_err("pool mgr not inited!\n");
			ret = -1;
			break;
		}
		if (s_pmgr->pool_num >= MAX_POOL_NUM) {
			ipcm_err("pool num out of range!\n");
			ret = -1;
			break;
		}

		size = ALIGN(size, POOL_ALIGN_SIZE);
		// first pool
		if (s_pmgr->pool_num == 0) {
			if (size > (end_pos - start_pos)) {
				ipcm_err("pool size out of range!\n");
				ret = -1;
				break;
			}
			s_pmgr->pool_list[0].addr_offset = start_pos;
			s_pmgr->pool_list[0].size = size;
			s_pmgr->pool_num = 1;
			ipcm_pool_cache_flush(s_region_paddr, NULL, s_pmgr->pool_start_pos);
			ret = start_pos;
			break;
		}
		if (s_pmgr) {
			for (i = 0; i < s_pmgr->pool_num; i++) {
				if (start_pos == s_pmgr->pool_list[i].addr_offset) {
					start_pos += s_pmgr->pool_list[i].size;
					continue;
				}
				if (start_pos < s_pmgr->pool_list[i].addr_offset) {
					end_pos = s_pmgr->pool_list[i].addr_offset - start_pos;
					if (size <= (end_pos - start_pos)) {
						// found
						memmove(&s_pmgr->pool_list[i+1], &s_pmgr->pool_list[i], sizeof(PoolManagerItem) * (s_pmgr->pool_num - i));
						s_pmgr->pool_list[i].addr_offset = start_pos;
						s_pmgr->pool_list[i].size = size;
						s_pmgr->pool_num++;
						ipcm_pool_cache_flush(s_region_paddr, NULL, s_pmgr->pool_start_pos);
						drv_spin_unlock_irqrestore_ext(&pool_lock, flags);
						return start_pos;
					}
					start_pos = s_pmgr->pool_list[i].addr_offset + s_pmgr->pool_list[i].size;
				}
			}
			if (size <= (s_pmgr->pool_end_pos - start_pos)) {
				s_pmgr->pool_list[s_pmgr->pool_num].addr_offset = start_pos;
				s_pmgr->pool_list[s_pmgr->pool_num].size = size;
				s_pmgr->pool_num++;
				ipcm_pool_cache_flush(s_region_paddr, NULL, s_pmgr->pool_start_pos);
				ret = start_pos;
				break;
			}
		}
	} while(0);
	drv_spin_unlock_irqrestore_ext(&pool_lock, flags);
	if (-1 == ret) {
		ipcm_err("pool mgr alloc fail!\n");
		for (i = 0; i < s_pmgr->pool_num; i++) {
			pool_print_info((POOLHANDLE)(s_pmgr + s_pmgr->pool_list[i].addr_offset), NULL);
		}
	}
	return ret;
}

s32 pool_mgr_free(u32 pos)
{
	int i = 0;
	int flags;

	drv_spin_lock_irqsave_ext(&pool_lock, flags);
	ipcm_pool_cache_invalidate(s_region_paddr, NULL, ALIGN(sizeof(PoolManager), POOL_ALIGN_SIZE));
	for (i = 0; i < s_pmgr->pool_num; i++) {
		if (s_pmgr->pool_list[i].addr_offset == pos) {
			memmove(&s_pmgr->pool_list[i], &s_pmgr->pool_list[i+1], sizeof(PoolManagerItem) * (s_pmgr->pool_num - i));
			s_pmgr->pool_num--;
			ipcm_pool_cache_flush(s_region_paddr, NULL, s_pmgr->pool_start_pos);
			drv_spin_unlock_irqrestore_ext(&pool_lock, flags);
			return 0;
		}
	}
	drv_spin_unlock_irqrestore_ext(&pool_lock, flags);
	ipcm_err("pool free fail pos:%d!\n", pos);
	for (i = 0; i < s_pmgr->pool_num; i++) {
		ipcm_err("pool:%d pos:%d size:%d\n", i, s_pmgr->pool_list[i].addr_offset, s_pmgr->pool_list[i].size);
	}
	return -1;
}

POOLHANDLE pool_mgr_get(u32 *pool_offset, u32 pool_id)
{
	PoolHead *head = NULL;
	int i = 0;
	int flags;
	POOLHANDLE handle = NULL;

	do {
		drv_spin_lock_irqsave_ext(&pool_lock, flags);
		ipcm_pool_cache_invalidate(s_region_paddr, NULL, ALIGN(sizeof(PoolManager), POOL_ALIGN_SIZE));
		if (!s_pmgr || s_pmgr->magic != POOL_MGR_MAGIC) {
			ipcm_err("pool mgr not inited!\n");
			handle =  NULL;
			break;
		}
		for (i = 0; i < s_pmgr->pool_num; i++) {
			head = (PoolHead *)((void *)s_pmgr + s_pmgr->pool_list[i].addr_offset);
			if (head->id == pool_id) {
				handle = (POOLHANDLE)head;
				break;
			}
		}
	} while(0);
	drv_spin_unlock_irqrestore_ext(&pool_lock, flags);
	if (!handle)
		ipcm_err("pool mgr get pool fail magic:0x%x!\n", pool_id);
	if (pool_offset)
		*pool_offset = s_pmgr->pool_list[i].addr_offset;
	return handle;
}

POOLHANDLE pool_create(u32 *pool_offset, PoolConfig *config, u32 pool_id)
{
	u32 block_total = 0;
	u32 pool_len = 0;
	u32 pool_size = 0;
	PoolHead *head;
	u32 pos = 0;
	u32 size = 0;
	int i;
	u32 pool_paddr;
	s32 tmp_offset;
	u32 num;

	if (!s_pmgr || s_pmgr->magic != POOL_MGR_MAGIC) {
		ipcm_err("pool mgr not inited!\n");
		return NULL;
	}

	if (config == NULL) {
		ipcm_err("config is null.\n");
		return NULL;
	}

	num = config->num;
	if (num > MAX_BLOCK_RANGE_NUM) {
		ipcm_err("block range num out of range,block range num is %d\n", num);
		return NULL;
	}
	for (i = 0; i < num; i++) {
		block_total += config->blk_conf[i].num;
		pool_len += (ALIGN(config->blk_conf[i].size, POOL_ALIGN_SIZE) * config->blk_conf[i].num);
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

	tmp_offset = pool_mgr_alloc(pool_size);
	if (tmp_offset < 0) {
		ipcm_err("pool mgr alloc fail!\n");
		return NULL;
	}
	pool_paddr = s_region_paddr + tmp_offset;

	// init head
	head = (PoolHead *)((void *)s_pmgr + tmp_offset);
	head->magic = IPCM_POOL_MAGIC;
	head->id = pool_id;
	head->paddr = pool_paddr;
	head->block_range_num = num;
	head->block_total = block_total;
	head->len = pool_len;
	head->data = ALIGN(sizeof(PoolHead), POOL_ALIGN_SIZE); // align 64 bytes
	pos = head->data;
	for (i = 0; i < num; i++) {
		size = ALIGN(config->blk_conf[i].size, POOL_ALIGN_SIZE); // align 64 bytes
		head->block_info[i].size = size;
		head->block_info[i].num = config->blk_conf[i].num;
		head->block_info[i].start_pos = pos;
		pos += (size * config->blk_conf[i].num);
	}
	memset(head->block_status, 0, sizeof(head->block_status));

	ipcm_pool_cache_flush(pool_paddr, NULL, pool_size);
	if (pool_offset)
		*pool_offset = tmp_offset;

	ipcm_debug("pool(%lx) create, head:\n", (unsigned long)head);
	pool_print_info((POOLHANDLE)head, NULL);

	return (POOLHANDLE)head;
}

s32 pool_destroy(POOLHANDLE handle)
{
	PoolHead *head = (PoolHead *)handle;

	if (head == NULL) {
		ipcm_err("handle is null.\n");
		return -1;
	}
	wait_block_release(head);
	pool_mgr_free(head->paddr - s_region_paddr);
	return 0;
}

s32 pool_reset(POOLHANDLE handle)
{
	PoolHead *head = (PoolHead *)handle;
	int flags;
	int flush_offset = 0;
	int flush_size = 0;

	flush_offset = ALIGN_DOWN(sizeof(head->block_info), IPCM_CACHE_ALIGN_SIZE);
	flush_size = sizeof(head->block_status) + (sizeof(head->block_info) - flush_offset);
	flush_size = ALIGN(flush_size, IPCM_CACHE_ALIGN_SIZE);

	drv_spin_lock_irqsave_ext(&pool_lock, flags);
	ipcm_pool_cache_invalidate(head->paddr+flush_offset, handle+flush_offset, flush_size);
	memset(&head->block_status, 0, sizeof(head->block_status));
	ipcm_pool_cache_flush(head->paddr+flush_offset, handle+flush_offset, flush_size);
	drv_spin_unlock_irqrestore_ext(&pool_lock, flags);

	return 0;
}

u32 pool_alloc_offset(POOLHANDLE handle, u32 size)
{
	PoolHead *head = (PoolHead *)handle;
	u32 block_range_start = 0;
	u32 block_start = 0;
	u32 block_fit = 0;
	u32 data_pos = 0;
	int i = 0;
	int flags;
	int flush_offset = 0;
	int flush_size = 0;

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

	flush_offset = ALIGN_DOWN(sizeof(head->block_info), IPCM_CACHE_ALIGN_SIZE);
	flush_size = sizeof(head->block_status) + (sizeof(head->block_info) - flush_offset);
	flush_size = ALIGN(flush_size, IPCM_CACHE_ALIGN_SIZE);

	IPCMPA_MUTEX_LOCK(&pool_buf_mutex);
	drv_spin_lock_irqsave_ext(&pool_lock, flags);
	if (flags == MAILBOX_LOCK_FAILED) {
		IPCMPA_MUTEX_UNLOCK(&pool_buf_mutex);
		return U32_MAX;
	}

	IPCM_DBG_R_POOL_BLOCK_IDX(1)
	IPCM_DBG_R_POOL_T0(head->block_status[0])

	ipcm_pool_cache_invalidate(head->paddr+flush_offset, handle+flush_offset, flush_size);
	IPCM_DBG_R_POOL_T1(head->block_status[0])
	for (i = block_start; i < head->block_total; i++) {
		if (!((head->block_status[i/32] >> (i%32)) & 1))
			break;
	}
	if (i == head->block_total) {
		drv_spin_unlock_irqrestore_ext(&pool_lock, flags);
		IPCMPA_MUTEX_UNLOCK(&pool_buf_mutex);
		ipcm_err("no free buff fit.\n");
		return U32_MAX;
	}
	block_fit = i;
	ipcm_debug("blk_fit(%d)\n", block_fit);
	IPCM_DBG_R_POOL_BLOCK_IDX(block_fit)
	head->block_status[i/32] |= (1 << (i%32));
	// *(volatile u32 *)&(head->block_status[i/32]) |= (1 << (i%32));
	IPCM_DBG_R_POOL_T2(head->block_status[0])
	ipcm_pool_cache_flush(head->paddr+flush_offset, handle+flush_offset, flush_size);
	IPCM_DBG_R_POOL_T3(head->block_status[0])
	drv_spin_unlock_irqrestore_ext(&pool_lock, flags);
	IPCMPA_MUTEX_UNLOCK(&pool_buf_mutex);

	#ifdef _DEBUG
	pool_print_info(handle, "af get buf pool info:");
	#endif

	for (i = block_range_start; i < head->block_range_num; i++) {
		if ((block_fit-block_start) < head->block_info[i].num)
			break;
		block_start += head->block_info[i].num;
	}

	data_pos = head->block_info[i].start_pos + (head->block_info[i].size * (block_fit-block_start));
	IPCM_DBG_R_POOL_DATA_POS(data_pos)
	IPCM_DBG_R_POOL_PUT_RING
	ipcm_debug("size(%d) range(%d) rang_start(%d) pos(%d) blk_fit(%d) blk_start(%d)\n", size, i,
		head->block_info[i].start_pos, data_pos, block_fit, block_start);

	return data_pos;
}

void *pool_alloc_buffer(POOLHANDLE handle, u32 size)
{
	PoolHead *head = (PoolHead *)handle;
	void *data;
	u32 data_pos = 0;

	if (head == NULL) {
		ipcm_err("handle is null.\n");
		return NULL;
	}

	data_pos = pool_alloc_offset(handle, size);
	if (data_pos == U32_MAX) {
		ipcm_err("pool_alloc_offset fail.\n");
		return NULL;
	}

	data = handle + data_pos;
	return data;
}

s32 pool_free_by_offset(POOLHANDLE handle, u32 pos)
{
	PoolHead *head = (PoolHead *)handle;
	u32 block_idx = 0;
	int flags;
	int flush_offset = 0;
	int flush_size = 0;

	if (head == NULL) {
		ipcm_err("handle is null.\n");
		return -EFAULT;
	}
	if (pos >= head->len) {
		ipcm_err("pos %d out of range", pos);
		return -EINVAL;
	}

	POOL_BUF_RLS_HOOK(pos);

	block_idx = pool_get_block_idx_by_offset(handle, pos);

	#ifdef _DEBUG
	pool_print_info(handle, "bf rls buf pool info:");
	#endif

	IPCM_DBG_R_POOL_FUNC_TYPE(0)
	IPCM_DBG_R_POOL_DATA_POS(pos)

	flush_offset = ALIGN_DOWN(sizeof(head->block_info), IPCM_CACHE_ALIGN_SIZE);
	flush_size = sizeof(head->block_status) + (sizeof(head->block_info) - flush_offset);
	flush_size = ALIGN(flush_size, IPCM_CACHE_ALIGN_SIZE);
	IPCMPA_MUTEX_LOCK(&pool_buf_mutex);
	drv_spin_lock_irqsave_ext(&pool_lock, flags);
	if (flags == MAILBOX_LOCK_FAILED) {
		IPCMPA_MUTEX_UNLOCK(&pool_buf_mutex);
		return -EINVAL;
	}
	IPCM_DBG_R_POOL_T0(head->block_status[0])
	ipcm_pool_cache_invalidate(head->paddr+flush_offset, handle+flush_offset, flush_size);
	IPCM_DBG_R_POOL_T1(head->block_status[0])
	IPCM_DBG_R_POOL_BLOCK_IDX(block_idx)
	head->block_status[block_idx/32] &= (~(1<<(block_idx%32)));
	// *(volatile u32 *)&(head->block_status[block_idx/32]) &= (~(1<<(block_idx%32)));
	IPCM_DBG_R_POOL_T2(head->block_status[0])
	ipcm_pool_cache_flush(head->paddr+flush_offset, handle+flush_offset, flush_size);
	IPCM_DBG_R_POOL_T3(head->block_status[0])
	drv_spin_unlock_irqrestore_ext(&pool_lock, flags);
	IPCMPA_MUTEX_UNLOCK(&pool_buf_mutex);
	IPCM_DBG_R_POOL_PUT_RING

	#ifdef _DEBUG
	pool_print_info(handle, "after rls buf pool info:");
	#endif
	return 0;
}

s32 pool_free_buffer(POOLHANDLE handle, void *data)
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
	data_pos = data - handle;
	return pool_free_by_offset(handle, data_pos);
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

u32 pool_get_block_idx_by_offset(POOLHANDLE handle, u32 pos)
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

	ipcm_debug("pool_get_block_idx_by_offset pos(%d) range_idx(%d), block_idx(%d)\n", pos, range_idx, block_idx);

	return block_idx;
}

void *pool_get_data_by_offset(POOLHANDLE handle, u32 data_pos)
{
	PoolHead *head = (PoolHead *)handle;

	if (head == NULL) {
		ipcm_err("handle is null.\n");
		return NULL;
	}

	return handle + data_pos;
}

u32 pool_get_data_offset(POOLHANDLE handle, void *data)
{
	PoolHead *head = (PoolHead *)handle;

	if (head == NULL) {
		ipcm_err("handle is null.\n");
		return U32_MAX;
	}
	return data - handle;
}

u32 pool_get_shm_data_offset(POOLHANDLE handle)
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
		ipcm_err("pool magic not match!\n");
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
