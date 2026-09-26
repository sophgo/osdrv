#if defined(linux) || defined(__linux) || defined(ANDROID)
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/dma-buf.h>
#include <linux/time.h>
#include <asm/io.h>

#include <asm/barrier.h>

#include "../vdi.h"
#include "../vdi_osal.h"
#include "main_helper.h"
#include "../mmu.h"


typedef struct free_block_node {
    int start;
    int size;
    struct free_block_node *next;
} free_block_node_t;

typedef struct
{
    unsigned int core_idx;
    mmu_config_t mmu_config;
    unsigned int *mmu_entry_table;
    osal_mutex_t mmu_mutex;
    free_block_node_t *mmu_free_blocks;
    int mmu_entry_used_cnt;
} mmu_info_t;

/*
*  g_mmu_config
*  0: page_size 1MB; 1: page_size 2MB; 2: page_size 1MB; 3: page_size 512KB; 6: disable MMU
*/
static int g_mmu_config[MAX_NUM_VPU_CORE] = {0};
static mmu_info_t *s_mmu_info[MAX_NUM_VPU_CORE] = {0};

static void mmu_print_info(unsigned long core_idx)
{
    mmu_info_t *mmu_info;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return;

    mmu_info = s_mmu_info[core_idx];
    if (!mmu_info)
        return;

    VLOG(INFO, "CORE %d MMU INFO", mmu_info->core_idx);
    VLOG(INFO,
         "MMU MODE: %d  MMU OFFSET MODE: %d  MMU PAGE SIZE: %d  MMU ENTRY USED CNT: %d\n",
         mmu_info->mmu_config.mmu_mode,
         mmu_info->mmu_config.mmu_offset_mode,
         mmu_info->mmu_config.page_size,
         mmu_info->mmu_entry_used_cnt);
}

static int mmu_get_entry_search_range(unsigned long core_idx,
                                      int *start, int *max_entry_num)
{
    mmu_info_t *mmu_info;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    mmu_info = s_mmu_info[core_idx];
    if (!mmu_info || !start || !max_entry_num)
        return -1;

    /*  special address can't be mapped in MMU
        0x0000 0000 ~ 0x0030 0000
        0xFFFE 0000 ~ 0xFFFF FFFF
     */
    switch (mmu_info->mmu_config.page_size) {
    case MMU_PAGE_2MB:
        *start = 2;
        *max_entry_num = 2046;
        break;
    case MMU_PAGE_1MB:
        *start = 3;
        *max_entry_num = 4094;
        break;
    case MMU_PAGE_512KB:
        *start = 6;
        *max_entry_num = 4094;
        break;
    case MMU_PAGE_256KB:
        *start = 12;
        *max_entry_num = 4094;
        break;
    case MMU_PAGE_128KB:
        *start = 24;
        *max_entry_num = 4094;
        break;
    case MMU_PAGE_64KB:
        *start = 48;
        *max_entry_num = 4094;
        break;
    default:
        VLOG(ERR, "[MMU] Invalid MMU page size\n");
        return -1;
    }

    return 0;
}

int mmu_set_config(unsigned long core_idx, int config)
{
    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    g_mmu_config[core_idx] = config;
    return 0;
}

int mmu_get_config(unsigned long core_idx)
{
    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    return g_mmu_config[core_idx];
}

int mmu_is_enabled(unsigned long core_idx)
{
    mmu_info_t *mmu_info;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return 0;

    mmu_info = s_mmu_info[core_idx];
    if (!mmu_info)
        return 0;

    return mmu_info->mmu_config.mmu_mode != MMU_DISABLE;
}

#define MMU_MODE_BIT_MASK           0x03
#define MMU_PAGESIZE_BIT_MASK       0x01
#define MMU_OFFSET_MODE_BIT_MASK    0x07
int mmu_init(unsigned long core_idx, mmu_config_t config)
{
    mmu_info_t *mmu_info;
    Uint32 reg_val;
    Uint32 config_val = 0;
    int start;
    int max_entry_num;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    if (s_mmu_info[core_idx] == NULL)
        s_mmu_info[core_idx] = vzalloc(sizeof(mmu_info_t));

    mmu_info = s_mmu_info[core_idx];
    mmu_info->mmu_entry_table = osal_malloc(MAX_NUM_MMU_ENTRY *
                                            sizeof(unsigned int));
    if (mmu_info->mmu_entry_table == NULL)
        return -1;
    osal_memset(mmu_info->mmu_entry_table, 0,
                MAX_NUM_MMU_ENTRY * sizeof(unsigned int));

    mmu_info->mmu_mutex = osal_mutex_create();
    mmu_info->mmu_config.mmu_mode = config.mmu_mode;
    mmu_info->mmu_config.mmu_offset_mode = config.mmu_offset_mode;
    mmu_info->mmu_config.page_size = config.page_size;
    mmu_info->core_idx = core_idx;
    mmu_info->mmu_entry_used_cnt = 0;

    config_val |= (mmu_info->mmu_config.mmu_mode & MMU_MODE_BIT_MASK);
    config_val |= (mmu_info->mmu_config.mmu_offset_mode & MMU_PAGESIZE_BIT_MASK) << 3;
    config_val |= (mmu_info->mmu_config.page_size & MMU_OFFSET_MODE_BIT_MASK) << 4;

    reg_val = vdi_read_top_register(core_idx, TOP_MMU_CRTL);
    reg_val &= 0xFFFF0000UL;
    reg_val |= config_val;
    vdi_write_top_register(core_idx, TOP_MMU_CRTL, reg_val);

    /* Initialize free blocks list */
    if (mmu_get_entry_search_range(core_idx, &start, &max_entry_num) == 0) {
        free_block_node_t *block;

        block = osal_malloc(sizeof(free_block_node_t));
        if (!block) {
            VLOG(ERR, "[MMU] core%d allocate mmu free_block list fail.\n",
                 mmu_info->core_idx);
            return -1;
        }
        block->start = start;
        block->size = max_entry_num - start;
        block->next = NULL;
        mmu_info->mmu_free_blocks = block;
    }

    mmu_print_info(core_idx);

    return 0;
}

int mmu_deinit(unsigned long core_idx)
{
    mmu_info_t *mmu_info;
    free_block_node_t *current_block;
    free_block_node_t *next;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    mmu_info = s_mmu_info[core_idx];
    if (mmu_info == NULL)
        return 0;

    if (mmu_info->mmu_entry_table)
        osal_free(mmu_info->mmu_entry_table);

    osal_mutex_lock(mmu_info->mmu_mutex);
    /* Free free blocks list */
    if (mmu_info->mmu_free_blocks) {
        current_block = mmu_info->mmu_free_blocks;
        while (current_block != NULL) {
            next = current_block->next;
            osal_free(current_block);
            current_block = next;
        }
        mmu_info->mmu_free_blocks = NULL;
    }
    osal_mutex_unlock(mmu_info->mmu_mutex);

    if (mmu_info->mmu_mutex)
        osal_mutex_destroy(mmu_info->mmu_mutex);

    mmu_set_config(core_idx, 6);
    osal_free(mmu_info);
    s_mmu_info[core_idx] = NULL;

    return 0;
}

/*
 * Find and reserve a contiguous MMU entry range from free list.
 *
 * Free list uses [start, end) interval semantics.
 *
 * Example (entry_cnt = 3):
 *
 *   free list:
 *     [ 10, 18 ) -> [ 30, 32 ) -> [ 40, 48 )
 *        size 8         size 2         size 8
 *
 *   best-fit picks the smallest block with size >= 3:
 *     [ 10, 18 )  (or [40,48), whichever first with minimal size)
 *
 *   allocate from head of chosen block:
 *     return index = 10
 *     block becomes [ 13, 18 )
 *
 * If no block can hold @entry_cnt, return -1.
 */
int mmu_get_entry_index(int core_idx, int entry_cnt)
{
    int start;
    int max_entry_num;
    int index;
    mmu_info_t *mmu_info;
    free_block_node_t *prev_block = NULL;
    free_block_node_t *current_block = NULL;
    free_block_node_t *best_fit_prev = NULL;
    free_block_node_t *best_fit_current = NULL;
    int best_size = 0;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    mmu_info = s_mmu_info[core_idx];

    if (!mmu_info || !mmu_info->mmu_entry_table ||
        !mmu_info->mmu_free_blocks || entry_cnt <= 0) {
        VLOG(ERR, "[MMU] Invalid mmu info. core: %d entry cnt: %d\n",
             core_idx, entry_cnt);
        return -1;
    }

    if (mmu_get_entry_search_range(core_idx, &start, &max_entry_num) < 0)
        return -1;

    /* Find best fit block from free blocks list */
    current_block = mmu_info->mmu_free_blocks;
    while (current_block != NULL) {
        if (current_block->size >= entry_cnt) {
            if (best_fit_current == NULL || current_block->size < best_size) {
                best_fit_prev = prev_block;
                best_fit_current = current_block;
                best_size = current_block->size;
            }
        }
        prev_block = current_block;
        current_block = current_block->next;
    }

    if (best_fit_current == NULL)
        return -1;

    /* Allocate from the best fit block */
    index = best_fit_current->start;
    if (best_fit_current->size == entry_cnt) {
        /* Exact match, remove the block */
        if (best_fit_prev) {
            best_fit_prev->next = best_fit_current->next;
        } else {
            mmu_info->mmu_free_blocks = best_fit_current->next;
        }
        osal_free(best_fit_current);
    } else {
        /* Partial match, update the block */
        best_fit_current->start += entry_cnt;
        best_fit_current->size -= entry_cnt;
    }

    return index;
}

/* In 2MB mode the encoder core (core 0) needs the upper LUT half to mirror the
 * lower half (entry[i] == entry[i+2048]); other page sizes and decoder cores
 * write the entry directly. */
#define MMU_LUT_MIRROR_OFFSET 2048

static void mmu_write_entry(unsigned long core_idx, mmu_info_t *mmu_info,
                            int index, Uint32 val)
{
    vdi_write_mmu_entry(core_idx, index * 4, val);
    if (mmu_info->mmu_config.page_size == MMU_PAGE_2MB && core_idx == 0)
        vdi_write_mmu_entry(core_idx, (index + MMU_LUT_MIRROR_OFFSET) * 4, val);
}

/*
 * Search reusable contiguous MMU entries by comparing expected mapping values.
 *
 * Conceptual timeline for a candidate window [s, s + entry_cnt):
 *
 *   expected:  E0  E1  E2  E3  E4
 *   current :  E0  E1  --  --  --     ('--' means free entry)
 *              ^           ^
 *              |           +-- first free after matched prefix
 *              +-- window start s
 *
 * Cases:
 *
 *   1) Full match
 *
 *      window:   [ s ................................ s+N )
 *      expected:  E0  E1  E2  E3  E4
 *      current :  E0  E1  E2  E3  E4
 *
 *      result: reuse_count = N, return s
 *
 *   2) Partial head match + free tail
 *
 *      window:   [ s ................................ s+N )
 *      expected:  E0  E1  E2  E3  E4
 *      current :  E0  E1  --  --  --
 *                        ^
 *                        +-- split point k (head matched, tail free)
 *
 *      result: reuse_count = k, return s
 *
 *   3) Mismatch or occupied non-matching tail
 *
 *      window:   [ s ................................ s+N )
 *      expected:  E0  E1  E2  E3  E4
 *      current :  E0  E1  XX  --  --    (XX != E2, or occupied by other map)
 *
 *      result: this window is not reusable, continue searching next window
 *
 * Return: start index s on (1)/(2), otherwise -1.
 */
static int mmu_find_reusable_entry(unsigned long core_idx,
                                   const Uint32 *expected_val,
                                   int entry_cnt, int *out_reuse_count)
{
    mmu_info_t *mmu_info;
    int start;
    int max_entry_num;
    int s;
    int j;
    int k;
    Uint32 val;
    int pra_shift;
    int used_cnt = 0;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    mmu_info = s_mmu_info[core_idx];
    if (!mmu_info || !mmu_info->mmu_entry_table || !expected_val ||
        entry_cnt <= 0) {
        VLOG(ERR, "[MMU] Invalid mmu info. core: %d entry cnt: %d\n",
             core_idx, entry_cnt);
        return -1;
    }
    if (mmu_get_entry_search_range(core_idx, &start, &max_entry_num) < 0)
        return -1;
    if (start + entry_cnt > max_entry_num)
        return -1;
    if (out_reuse_count)
        *out_reuse_count = 0;

    pra_shift = 12 + mmu_info->mmu_config.page_size;

    for (s = start; s <= max_entry_num - entry_cnt; s++) {
        if (used_cnt >= mmu_info->mmu_entry_used_cnt)
            break;

        if (mmu_info->mmu_entry_table[s] != 0)
            used_cnt++;

        k = 0;
        while (k < entry_cnt && mmu_info->mmu_entry_table[s + k] > 0) {
            val = vdi_read_mmu_entry(core_idx, (s + k) * 4);
            if (mmu_info->mmu_config.mmu_offset_mode == 0) {
                if ((val >> pra_shift) != (expected_val[k] >> pra_shift))
                    break;
            } else {
                if (val != expected_val[k])
                    break;
            }
            k++;
        }

        if (k == 0)
            continue;

        /* Full match */
        if (k == entry_cnt) {
            if (out_reuse_count)
                *out_reuse_count = entry_cnt;
            return s;
        }

        /* Partial match */
        for (j = k; j < entry_cnt; j++) {
            if (mmu_info->mmu_entry_table[s + j] != 0)
                break;
        }
        if (j == entry_cnt) {
            if (out_reuse_count)
                *out_reuse_count = k;
            return s;
        }
    }
    return -1;
}

/*
 * Insert a free range [start, start + size) into ordered free list.
 *
 * The list stays sorted by start index and tries to merge neighbors:
 *
 *   before: [10,20) -> [30,40)
 *
 *   A) insert [20,30): touches both sides
 *      after : [10,40)
 *
 *   B) insert [20,25): touches left only
 *      after : [10,25) -> [30,40)
 *
 *   C) insert [25,30): touches right only
 *      after : [10,20) -> [25,40)
 *
 *   D) insert [22,24): touches none
 *      after : [10,20) -> [22,24) -> [30,40)
 */
static void mmu_insert_free_block(unsigned long core_idx, int start, int size)
{
    mmu_info_t *mmu_info;
    free_block_node_t *new_block = NULL;
    free_block_node_t *prev_block = NULL;
    free_block_node_t *current_block = NULL;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return;

    mmu_info = s_mmu_info[core_idx];
    if (!mmu_info) {
        VLOG(ERR, "[MMU] Invalid mmu info. core: %d\n", core_idx);
        return;
    }

    current_block = mmu_info->mmu_free_blocks;

    new_block = osal_malloc(sizeof(free_block_node_t));
    if (!new_block)
        return;

    new_block->start = start;
    new_block->size = size;
    new_block->next = NULL;

    /* Find the correct position and merge adjacent blocks */
    while (current_block != NULL) {
        /* Merge with next block */
        if (start + size == current_block->start) {
            new_block->size += current_block->size;
            new_block->next = current_block->next;
            if (prev_block) {
                prev_block->next = new_block;
                if (prev_block->start + prev_block->size == new_block->start) {
                    prev_block->size += new_block->size;
                    prev_block->next = new_block->next;
                    osal_free(new_block);
                }
            } else {
                mmu_info->mmu_free_blocks = new_block;
            }
            osal_free(current_block);
            return;
        }
        /* Merge with previous block */
        if (current_block->start + current_block->size == start) {
            free_block_node_t *next_block = current_block->next;
            current_block->size += size;

            if (next_block &&
                current_block->start + current_block->size ==
                next_block->start) {
                current_block->size += next_block->size;
                current_block->next = next_block->next;
                osal_free(next_block);
            }
            osal_free(new_block);
            return;
        }
        /* Insert in middle */
        if (current_block->start > start) {
            new_block->next = current_block;
            if (prev_block) {
                prev_block->next = new_block;
            } else {
                mmu_info->mmu_free_blocks = new_block;
            }
            return;
        }
        prev_block = current_block;
        current_block = current_block->next;
    }

    /* Insert at end */
    if (prev_block) {
        prev_block->next = new_block;
    } else {
        mmu_info->mmu_free_blocks = new_block;
    }
}

/*
 * Reserve range [start, start + size) from ordered free list.
 *
 * One containing block [B0, B1) is updated in place.
 *
 *   before: [10,20) -> [30,40)
 *
 *   A) reserve [10,20): exact cover
 *      after : [30,40)
 *
 *   B) reserve [10,15): cut from head
 *      after : [15,20) -> [30,40)
 *
 *   C) reserve [15,20): cut from tail
 *      after : [10,15) -> [30,40)
 *
 *   D) reserve [13,17): cut from middle
 *      after : [10,13) -> [17,20) -> [30,40)
 *
 * Return 0 on success, -1 if no containing block exists.
 */
static int mmu_update_free_block(unsigned long core_idx, int start, int size)
{
    mmu_info_t *mmu_info;
    free_block_node_t *prev_block = NULL;
    free_block_node_t *current_block = NULL;
    int alloc_end;
    int block_start, block_end;

    if (core_idx >= MAX_NUM_VPU_CORE || size <= 0)
        return -1;

    mmu_info = s_mmu_info[core_idx];
    if (!mmu_info) {
        VLOG(ERR, "[MMU] Invalid mmu info. core:%d\n", core_idx);
        return -1;
    }

    alloc_end = start + size;
    current_block = mmu_info->mmu_free_blocks;

    while (current_block != NULL) {
        block_start = current_block->start;
        block_end = current_block->start + current_block->size;

        if (start >= block_start && alloc_end <= block_end) {
            if (start == block_start && alloc_end == block_end) {
                if (prev_block)
                    prev_block->next = current_block->next;
                else
                    mmu_info->mmu_free_blocks = current_block->next;
                osal_free(current_block);
            } else if (start == block_start) {
                current_block->start = alloc_end;
                current_block->size = block_end - alloc_end;
            } else if (alloc_end == block_end) {
                current_block->size = start - block_start;
            } else {
                free_block_node_t *tail_block;

                tail_block = osal_malloc(sizeof(free_block_node_t));
                if (!tail_block)
                    return -1;

                tail_block->start = alloc_end;
                tail_block->size = block_end - alloc_end;
                tail_block->next = current_block->next;

                current_block->size = start - block_start;
                current_block->next = tail_block;
            }

            return 0;
        }

        prev_block = current_block;
        current_block = current_block->next;
    }

    return -1;
}

int mmu_alloc_virt_addr(unsigned long core_idx, vpu_buffer_t *vb)
{
    int ret;
    mmu_info_t *mmu_info;
    int i;
    int index, entry_cnt;
    Uint32 val;
    Uint32 poa_value;
    Uint32 poa_shift_bit;
    Uint64 phys_addr;
    Uint32 *expected_val = NULL;
    int reuse_count = 0;

    VLOG(TRACE, "enter %s\n", __func__);
    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    mmu_info = s_mmu_info[core_idx];
    if (!mmu_info || !vb) {
        VLOG(ERR, "[MMU] Invalid mmu info. core: %d\n", core_idx);
        return -1;
    }

    /* default to the "unmapped" sentinel so every failure/early-return path below
     * leaves values that make mmu_free_virt_addr a no-op (entry_index<0);
     * success paths overwrite these with the real index/num. */
    vb->mmu_entry_index = -1;
    vb->mmu_entry_num = -1;

    if (vb->phys_addr == 0 || vb->size == 0)
        return -1;

    vb->phys_addr_36bit = vb->phys_addr;
    if (mmu_info->mmu_config.mmu_mode == MMU_DISABLE)  /* entry sentinel already set above */
        return 0;

    phys_addr = vb->phys_addr_36bit & 0xFFFFFFFFFUL;
    poa_shift_bit = 16 + mmu_info->mmu_config.page_size;
    if (mmu_info->mmu_config.mmu_offset_mode == 0)
        poa_value = vb->phys_addr & ((1UL << poa_shift_bit) - 1);
    else
        poa_value = vb->phys_addr & 0xFUL;
    entry_cnt =
        (vb->size + poa_value + ((1UL << poa_shift_bit) - 1)) >> poa_shift_bit;

    expected_val = osal_malloc(entry_cnt * sizeof(Uint32));
    if (!expected_val)
        return -1;

    for (i = 0; i < entry_cnt; i++)
        expected_val[i] =
            (Uint32)((phys_addr + ((Uint64)i << poa_shift_bit)) >> 4);

    osal_mutex_lock(mmu_info->mmu_mutex);

    /* Reuse: full or partial contiguous */
    if (expected_val) {
        index = mmu_find_reusable_entry(core_idx, expected_val,
                                        entry_cnt, &reuse_count);
        if (index >= 0) {
            int new_alloc_cnt = entry_cnt - reuse_count;

            if (new_alloc_cnt > 0) {
                ret = mmu_update_free_block(core_idx, index + reuse_count,
                                            new_alloc_cnt);
                if (ret < 0) {
                    osal_mutex_unlock(mmu_info->mmu_mutex);
                    osal_free(expected_val);
                    VLOG(ERR, "[MMU] Can't get mmu free block\n");
                    return -1;
                }
            }

            for (i = 0; i < reuse_count; i++)
                mmu_info->mmu_entry_table[index + i]++;
            for (i = reuse_count; i < entry_cnt; i++) {
                mmu_write_entry(core_idx, mmu_info, index + i, expected_val[i]);
                mmu_info->mmu_entry_table[index + i] = 1;
                mmu_info->mmu_entry_used_cnt++;
            }

            mb();
            osal_mutex_unlock(mmu_info->mmu_mutex);
            vb->phys_addr = ((Uint64)index << poa_shift_bit) | poa_value;
            vb->mmu_entry_index = index;
            vb->mmu_entry_num = entry_cnt;
            osal_free(expected_val);
            return 0;
        }
    }

    /* Allocate new contiguous entries */
    index = mmu_get_entry_index(core_idx, entry_cnt);
    if (index < 0) {
        osal_mutex_unlock(mmu_info->mmu_mutex);
        if (expected_val)
            osal_free(expected_val);
        return -1;
    }
    vb->phys_addr = ((Uint64)index << poa_shift_bit) | poa_value;
    vb->mmu_entry_index = index;
    vb->mmu_entry_num = entry_cnt;

    for (i = 0; i < entry_cnt; i++) {
        val = (Uint32)(phys_addr >> 4);
        mmu_write_entry(core_idx, mmu_info, index, val);
        mmu_info->mmu_entry_table[index] = 1;
        mmu_info->mmu_entry_used_cnt++;

        phys_addr += (1ULL << poa_shift_bit);
        index++;
    }

    mb();
    osal_mutex_unlock(mmu_info->mmu_mutex);
    if (expected_val)
        osal_free(expected_val);

    VLOG(TRACE, "leave %s\n", __func__);
    return 0;
}

int mmu_free_virt_addr(unsigned long core_idx, vpu_buffer_t *vb)
{
    mmu_info_t *mmu_info;
    int i;
    int start = -1;
    int size = 0;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    mmu_info = s_mmu_info[core_idx];
    if (!mmu_info || !vb) {
        VLOG(ERR, "[MMU] Invalid mmu info. core: %d\n", core_idx);
        return -1;
    }

    if (mmu_info->mmu_config.mmu_mode == MMU_DISABLE)
        return 0;

    osal_mutex_lock(mmu_info->mmu_mutex);
    if (vb->mmu_entry_index >= 0 && vb->mmu_entry_num > 0 &&
        mmu_info->mmu_entry_table) {
        for (i = vb->mmu_entry_index;
             i < vb->mmu_entry_index + vb->mmu_entry_num; i++) {
            if (mmu_info->mmu_entry_table[i] > 1) {
                mmu_info->mmu_entry_table[i]--;
                continue;
            }

            mmu_write_entry(core_idx, mmu_info, i, 0);
            mmu_info->mmu_entry_table[i] = 0;
            mmu_info->mmu_entry_used_cnt--;
        }
    }

    /* Update free blocks list */
    if (vb->mmu_entry_index >= 0 && vb->mmu_entry_num > 0 &&
        mmu_info->mmu_free_blocks) {
        for (i = vb->mmu_entry_index;
             i < vb->mmu_entry_index + vb->mmu_entry_num; i++) {
            if (mmu_info->mmu_entry_table[i] == 0) {
                if (start == -1) {
                    start = i;
                    size = 1;
                } else {
                    size++;
                }
            } else {
                if (start != -1) {
                    /* Insert contiguous free region to free blocks list */
                    mmu_insert_free_block(core_idx, start, size);
                    start = -1;
                    size = 0;
                }
            }
        }

        if (start != -1) {
            mmu_insert_free_block(core_idx, start, size);
        }
    }
    osal_mutex_unlock(mmu_info->mmu_mutex);

    return 0;
}

int mmu_get_phys_addr(unsigned long core_idx, Uint64 mmu_addr,
                      Uint64 *phys_addr)
{
    mmu_info_t *mmu_info;
    int index, page_size;
    Uint32 val;
    Uint64 offset;
    Uint64 pra_addr, roa_addr;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    mmu_info = s_mmu_info[core_idx];
    if (!mmu_info) {
        VLOG(ERR, "[MMU] Invalid mmu info. core: %d\n", core_idx);
        return -1;
    }

    if (mmu_info->mmu_config.mmu_mode == MMU_DISABLE) {
        *phys_addr = mmu_addr;
        return 0;
    }

    page_size = mmu_info->mmu_config.page_size;
    index = (mmu_addr >> (16 + page_size)) & 0xFFFU;
    offset = mmu_addr & ((1 << (16 + page_size)) - 1);

    val = vdi_read_mmu_entry(core_idx, index * 4);
    pra_addr = val >> (12 + page_size);
    roa_addr = val & ((1 << (12 + page_size)) - 1);
    if (mmu_info->mmu_config.mmu_offset_mode == 1)
        *phys_addr = (pra_addr << (16 + page_size)) + (roa_addr << 4) + offset;
    else
        *phys_addr = (pra_addr << (16 + page_size)) + offset;

    /* DDR start address 0x10 0000 0000 */
    *phys_addr |= 0x1000000000UL;

    VLOG(TRACE, "%s. core_idx: %d mmu_addr:0x%lx phys_addr:0x%lx\n",
         __func__, mmu_info->core_idx, mmu_addr, *phys_addr);
    return 0;
}

#endif /* defined(linux) || defined(__linux) || defined(ANDROID) */
