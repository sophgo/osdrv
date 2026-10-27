#ifndef _MMU_H_
#define _MMU_H_

#include "vdi_debug.h"
#include "vpuconfig.h"
#include "vputypes.h"
#include "vdi.h"

typedef enum {
    MMU_FORCE_ENABLE = 0,
    MMU_ENABLE_BIT31,
    MMU_ENABLE_BIT30,
    MMU_DISABLE
} MmuMode;

typedef enum {
    MMU_PAGE_64KB   = 0,
    MMU_PAGE_128KB,
    MMU_PAGE_256KB,
    MMU_PAGE_512KB,
    MMU_PAGE_1MB,
    MMU_PAGE_2MB
} MmuPageSize;

typedef struct mmu_config_t
{
    MmuMode mmu_mode;           /* 0 enable; 1 disable */
    uint8_t mmu_offset_mode;    /* 1 enable; 0 disable */
    MmuPageSize page_size;      /* Refer to MmuPageSize */
} mmu_config_t;


#if defined(__cplusplus)
extern "C" {
#endif

int mmu_set_config(unsigned long core_idx, int config);
int mmu_get_config(unsigned long core_idx);
/* 1 if MMU is actually enabled after mmu_init (mmu_mode != MMU_DISABLE), else 0.
 * Use this instead of testing mmu_get_config() against 0: config value 0 now
 * means "enabled, 1MB page", so a >0 / !=0 test misclassifies the default. */
int mmu_is_enabled(unsigned long core_idx);
int mmu_init(unsigned long core_idx, mmu_config_t config);
int mmu_deinit(unsigned long core_idx);
int mmu_get_entry_index(int core_idx, int entry_cnt);
int mmu_alloc_virt_addr(unsigned long core_idx, vpu_buffer_t *vb);
int mmu_get_phys_addr(unsigned long core_idx, Uint64 mmu_addr, Uint64 *phys_addr);
int mmu_free_virt_addr(unsigned long core_idx, vpu_buffer_t *vb);

#if defined(__cplusplus)
}
#endif

#endif /* _MMU_H_ */
