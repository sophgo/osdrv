//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
//
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
//
// The entire notice above must be reproduced on all authorized copies.
//
// Description  :
//-----------------------------------------------------------------------------

#if defined(linux) || defined(__linux) || defined(ANDROID)
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/dma-buf.h>
#include <linux/time.h>
#include <asm/io.h>

#include "../vdi.h"
#include "../vdi_osal.h"
#if defined(MEDIA_V3)
#include "../mmu.h"
#endif
#include "coda9/coda9_regdefine.h"
#include "wave/wave5_regdefine.h"
#include "wave/wave6_regdefine.h"
#include "device.h"
#include "main_helper.h"
#include "misc/debug.h"
#include "platform.h"

#define VPU_BIT_REG_SIZE                    (0x4000*MAX_NUM_VPU_CORE)

typedef struct vpudrv_buffer_pool_t
{
    vpudrv_buffer_t vdb;
    int inuse;
#ifdef MEDIA_V3
    PhysicalAddress phys_addr_36bit;   /* real 36bit phys, for vpu_free_physical_memory */
    int mmu_entry_index;
    int mmu_entry_num;
#endif
} vpudrv_buffer_pool_t;

typedef struct  {
    unsigned long           core_idx;
    unsigned int            product_code;
    VPU_FD                  vpu_fd;
    int                     support_cq;
    vpudrv_buffer_pool_t    vpu_buffer_pool[MAX_VPU_BUFFER_POOL];
    vpu_instance_pool_t*    pvip;
    int                     task_num;
    int                     clock_state;
    vpudrv_buffer_t         vdb_register;
#if defined(MEDIA_V3)
    vpudrv_buffer_t         vdb_top_register;
    vpudrv_buffer_t         vdb_mmu_entry;
#endif
    vpu_buffer_t            vpu_common_memory;
    int                     vpu_buffer_pool_count;
    pid_t pid;
    unsigned char             ext_addr;
    unsigned int instance_start_flag;
    atomic_t instance_count;
} vdi_info_t;

static vdi_info_t *s_vdi_info[MAX_NUM_VPU_CORE] = {0};

#define VDI_SRAM_BASE_ADDR                  0x00000000    // if we can know the sram address in SOC directly for vdi layer. it is possible to set in vdi layer without allocation from driver
#define VDI_SYSTEM_ENDIAN                   VDI_LITTLE_ENDIAN
#define VDI_128BIT_BUS_SYSTEM_ENDIAN        VDI_128BIT_LITTLE_ENDIAN

extern vpudrv_buffer_t s_vpu_register[MAX_NUM_VPU_CORE];
#if defined(MEDIA_V3)
extern vpudrv_buffer_t s_vpu_top_register[MAX_NUM_VPU_CORE];
extern vpudrv_buffer_t s_vpu_mmu_entry[MAX_NUM_VPU_CORE];
static unsigned int vpu_show_bw = 0;
module_param(vpu_show_bw, int, 0644);

static unsigned int vpu_bwl[4] = {0, 0, 0, 0};
module_param_array(vpu_bwl, uint, NULL, 0644);

static unsigned int vpu_ck_downspeed[5] = {0x80, 0x80, 0x80, 0x80, 0x80};
module_param_array(vpu_ck_downspeed, uint, NULL, 0644);

#endif

int swap_endian(unsigned long core_idx, unsigned char *data, int len, int endian);

int vdi_invalidate_ion_cache(uint64_t u64PhyAddr, void *pVirAddr,
                 uint32_t u32Len)
{
    vpudrv_buffer_t vdb;

    vdb.phys_addr = u64PhyAddr;
    vdb.virt_addr = (unsigned long)pVirAddr;
    vdb.size = u32Len;

    return vpu_invalidate_dcache(&vdb);
}

int vdi_flush_ion_cache(uint64_t u64PhyAddr, void *pVirAddr, uint32_t u32Len)
{
    vpudrv_buffer_t vdb;

    vdb.phys_addr = u64PhyAddr;
    vdb.virt_addr = (unsigned long)pVirAddr;
    vdb.size = u32Len;

    return vpu_flush_dcache(&vdb);
}

int vdi_probe(unsigned long core_idx)
{
    int ret;

#if defined(MEDIA_V3)
    mmu_set_config(core_idx, 0);
#endif
    ret = vdi_init(core_idx);
    vdi_release(core_idx);
    return ret;
}

#if defined(MEDIA_V3)
void vdi_write_mmu_entry(unsigned long core_idx, unsigned int addr, unsigned int data)
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return;

    platform_write_register(addr + vdi->vdb_mmu_entry.phys_addr, (unsigned int *)(addr + vdi->vdb_mmu_entry.virt_addr), data);
}

unsigned int vdi_read_mmu_entry(unsigned long core_idx, unsigned int addr)
{
    vdi_info_t *vdi;
    unsigned int val;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return (unsigned int)-1;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return (unsigned int)-1;

    val = platform_read_register(addr + vdi->vdb_mmu_entry.phys_addr, (unsigned int *)(addr + vdi->vdb_mmu_entry.virt_addr));
    return val;
}

void vdi_bwc_showinfo(unsigned long core_idx)
{
    vdi_info_t *vdi;
    unsigned int r0, r1;
    unsigned int w0, w1, w2;
    unsigned long sum_r, sum_w;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return;

    if(core_idx == 0) {
        r0 = vdi_read_top_register(core_idx, TOP_VE_BWC_R);
        w0 = vdi_read_top_register(core_idx, TOP_VE_BWC_W);
        sum_r = r0;
        sum_w = w0;
    }
    else {
        r0 = vdi_read_top_register(core_idx, TOP_VD_BWC_R0);
        r1 = vdi_read_top_register(core_idx, TOP_VD_BWC_R1);
        w0 = vdi_read_top_register(core_idx, TOP_VD_BWC_W0);
        w1 = vdi_read_top_register(core_idx, TOP_VD_BWC_W1);
        w2 = vdi_read_top_register(core_idx, TOP_VD_BWC_W2);
        sum_r = r0 + r1;
        sum_w = w0 + w1 + w2;
    }

    printk("[VDI] core: %ld, BW read: %ld KB, BW write: %ld KB\n", core_idx, (sum_r*16/1024), (sum_w*16/1024));
}

int vdi_set_bwl(int coreIdx, bwl_param_t bwl)
{
    unsigned int val;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    val = vdi_read_top_register(coreIdx, TOP_SET);
    if(coreIdx == 0)
        val |= (0x3 << 24);
    else
        val |= (0x3 << 27); 
    vdi_write_top_register(coreIdx, TOP_SET, val);


    val = (bwl.aw_vld << 16) | (bwl.aw_win & 0xFFFF);
    vdi_write_top_register(coreIdx, TOP_VD_BWL_AW, val);

    val = (bwl.ar_vld << 16) | (bwl.ar_win & 0xFFFF);
    vdi_write_top_register(coreIdx, TOP_VD_BWL_AR, val);

    VLOG(INFO, "[VDI] vdi_set_bwl bwl_aw: %x bwl_ar: %x\n", vdi_read_top_register(coreIdx, TOP_VD_BWL_AW), vdi_read_top_register(coreIdx, TOP_VD_BWL_AR));
    return 0;
}

int vdi_get_bwl(int coreIdx, bwl_param_t *bwl)
{
    unsigned int val;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    val = vdi_read_top_register(coreIdx, TOP_VD_BWL_AW);
    bwl->aw_win = val & 0xFFFF;
    bwl->aw_vld = (val >> 16) & 0xFFFF;

    val = vdi_read_top_register(coreIdx, TOP_VD_BWL_AR);        
    bwl->ar_win = val & 0xFFFF;
    bwl->ar_vld = (val >> 16) & 0xFFFF;

    VLOG(INFO, "[VDI] vdi_get_bwl aw_win: %x, aw_vld: %x, ar_win: %x, ar_vld: %x\n", bwl->aw_win, bwl->aw_vld, bwl->ar_win, bwl->ar_vld);
    return 0;
}

int vdi_ck_downspeed_set(int coreIdx, unsigned int coef)
{
    unsigned int val;
    unsigned int ck_low;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    ck_low = (coef & 0xFF) | ((coef & 0xFF) << 8);

    val = vdi_read_top_register(coreIdx, TOP_REG_CK_0);
    val = (val & 0xFFFF0000) | ck_low;
    vdi_write_top_register(coreIdx, TOP_REG_CK_0, val);

    val = vdi_read_top_register(coreIdx, TOP_REG_CK_1);
    val = (val & 0xFFFF0000) | ck_low;
    vdi_write_top_register(coreIdx, TOP_REG_CK_1, val);

    VLOG(INFO, "[VDI] vdi_set_ck_downspeed core %d, CK_0: %x CK_1: %x\n",
         coreIdx,
         vdi_read_top_register(coreIdx, TOP_REG_CK_0),
         vdi_read_top_register(coreIdx, TOP_REG_CK_1));

    return 0;
}

static int vdi_media_v3_init(unsigned long core_idx)
{
    vdi_info_t *vdi;
    mmu_config_t config;
    unsigned int val;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    memcpy(&vdi->vdb_top_register, &s_vpu_top_register[core_idx], sizeof(vpudrv_buffer_t));
    VLOG(INFO, "[VDI] map vdb_top_register core_idx=%d, phyaddr:0x%x, virtaddr=0x%x, size=%d\n", core_idx, vdi->vdb_top_register.phys_addr, (int)vdi->vdb_top_register.virt_addr, vdi->vdb_top_register.size);

    memcpy(&vdi->vdb_mmu_entry, &s_vpu_mmu_entry[core_idx], sizeof(vpudrv_buffer_t));
    VLOG(INFO, "[VDI] map vdb_mmu_entry core_idx=%d, phyaddr:0x%x, virtaddr=0x%x, size=%d\n", core_idx, vdi->vdb_mmu_entry.phys_addr, (int)vdi->vdb_mmu_entry.virt_addr, vdi->vdb_mmu_entry.size);

    osal_memset(&config, 0, sizeof(mmu_config_t));
    switch(mmu_get_config(core_idx))
    {
        case 0:
            config.mmu_mode = MMU_FORCE_ENABLE;
            config.page_size = MMU_PAGE_1MB;
            break;
        case 1:
            config.mmu_mode = MMU_FORCE_ENABLE;
            config.page_size = MMU_PAGE_2MB;
            break;
        case 2:
            config.mmu_mode = MMU_FORCE_ENABLE;
            config.page_size = MMU_PAGE_1MB;
            break;
        case 3:
            config.mmu_mode = MMU_FORCE_ENABLE;
            config.page_size = MMU_PAGE_512KB;
            break;
        case 6:
            config.mmu_mode = MMU_DISABLE;
            break;
        default:
            config.mmu_mode = MMU_DISABLE;
    }
    if(mmu_init(core_idx, config) != 0) {
        VLOG(ERR, "[VDI] fail to init mmu\n");
        return -1;
    }

    if(vpu_show_bw) {
        /* Enable BWC */
        val = vdi_read_top_register(core_idx, TOP_SET);
        val |= 1UL << 31;
        vdi_write_top_register(core_idx, TOP_SET, val);
    }

    if(vpu_bwl[0] || vpu_bwl[1] || vpu_bwl[2] || vpu_bwl[3]) {
        bwl_param_t bwl;
        bwl.aw_vld = vpu_bwl[0];
        bwl.aw_win = vpu_bwl[1];
        bwl.ar_vld = vpu_bwl[2];
        bwl.ar_win = vpu_bwl[3];
        vdi_set_bwl(core_idx, bwl);
    }

    if (vpu_ck_downspeed[core_idx])
        vdi_ck_downspeed_set(core_idx, vpu_ck_downspeed[core_idx]);

    return 0;
}
#endif

int vdi_init(unsigned long core_idx)
{
    vdi_info_t *vdi;
    int* pCodecInst;
    int i;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return 0;

    if (s_vdi_info[core_idx] == NULL)
        s_vdi_info[core_idx] = vzalloc(sizeof(vdi_info_t));

    vdi = s_vdi_info[core_idx];

    if (vdi->vpu_fd != (VPU_FD)-1 && vdi->vpu_fd != (VPU_FD)0x00)
    {
        vdi->task_num++;
        return 0;
    }

    if (vpu_op_open(core_idx) < 0){
        VLOG(ERR, "[VDI] Can't open vpu driver\n");
        goto ERR_VDI_INIT;
    }
    vdi->vpu_fd = 'v' + core_idx;

    memset(vdi->vpu_buffer_pool, 0x00, sizeof(vpudrv_buffer_pool_t)*MAX_VPU_BUFFER_POOL);

    if (!vdi_get_instance_pool(core_idx))
    {
        VLOG(INFO, "[VDI] fail to create shared info for saving context \n");
        goto ERR_VDI_INIT;
    }

    if (vdi->pvip->instance_pool_inited == FALSE)
    {

        for( i = 0; i < MAX_NUM_INSTANCE; i++) {
            pCodecInst = (int *)vdi->pvip->codecInstPool[i];
            pCodecInst[1] = i;  // indicate instIndex of CodecInst
            pCodecInst[0] = 0;  // indicate inUse of CodecInst
        }
        vdi->pvip->instance_pool_inited = TRUE;
    }
    vdi->vdb_register.size = core_idx;
    memcpy(&vdi->vdb_register, &s_vpu_register[vdi->vdb_register.size], sizeof(vpudrv_buffer_t));

    VLOG(INFO, "[VDI] map vdb_register core_idx=%d, phyaddr:0x%x, virtaddr=0x%x, size=%d\n", core_idx, vdi->vdb_register.phys_addr, (int)vdi->vdb_register.virt_addr, vdi->vdb_register.size);

#if defined(MEDIA_V3)
    if(vdi_media_v3_init(core_idx) != 0) {
        goto ERR_VDI_INIT;
    }
#endif

    vdi_set_clock_gate(core_idx, 1);

    vdi->product_code = vdi_read_register(core_idx, VPU_PRODUCT_CODE_REGISTER);

    if (vdi_allocate_common_memory(core_idx) < 0)
    {
        VLOG(ERR, "[VDI] fail to get vpu common buffer from driver\n");
        goto ERR_VDI_INIT;
    }


    vdi->core_idx = core_idx;
    vdi->task_num++;
    vdi_set_clock_gate(core_idx, 0);
    atomic_set(&vdi->instance_count, 1);
    VLOG(INFO, "[VDI] success to init driver \n");
    return 0;

ERR_VDI_INIT:
#if defined(MEDIA_V3)
    mmu_deinit(core_idx);
#endif
    vdi_release(core_idx);
    return -1;
}

int vdi_set_bit_firmware_to_pm(unsigned long core_idx, const unsigned short *code)
{
    int i;
    vpu_bit_firmware_info_t *bit_firmware_info;
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return 0;

    vdi = s_vdi_info[core_idx];

    if (!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return 0;

    bit_firmware_info = vzalloc(sizeof(bit_firmware_info));
    bit_firmware_info->size = sizeof(vpu_bit_firmware_info_t);
    bit_firmware_info->core_idx = core_idx;
    bit_firmware_info->reg_base_offset = 0;
    if (PRODUCT_CODE_CODA_SERIES(vdi->product_code)) {
        for (i=0; i<512; i++) {
            bit_firmware_info->bit_code[i] = code[i];
        }
    }

    if (vpu_op_write((const char *)(bit_firmware_info), bit_firmware_info->size) < 0)
    {
        VLOG(ERR, "[VDI] fail to vdi_set_bit_firmware core=%d\n", bit_firmware_info->core_idx);
        vfree(bit_firmware_info);
        return -1;
    }

    vfree(bit_firmware_info);
    return 0;
}


int vdi_get_task_num(unsigned long core_idx)
{
    vdi_info_t *vdi;
    vdi = s_vdi_info[core_idx];

    if (!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == 0x00)
        return -1;

    return vdi->task_num;
}

int vdi_release(unsigned long core_idx)
{
    int i;
    vpudrv_buffer_t vdb;
    vdi_info_t *vdi;
    unsigned int val;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return 0;

    vdi = s_vdi_info[core_idx];

    if (!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return 0;

    if (vdi->task_num > 1) // means that the opened instance remains
    {
        vdi->task_num--;
        return 0;
    }

    osal_memset(&vdi->vdb_register, 0x00, sizeof(vpudrv_buffer_t));
#ifdef MEDIA_V3
    if(vpu_show_bw) {
        /* Disable BWC */
        val = vdi_read_top_register(core_idx, TOP_SET);
        val &= ~(1UL << 31);
        vdi_write_top_register(core_idx, TOP_SET, val);
        vdi_bwc_showinfo(core_idx);
    }

    osal_memset(&vdi->vdb_top_register, 0x00, sizeof(vpudrv_buffer_t));
    osal_memset(&vdi->vdb_mmu_entry, 0x00, sizeof(vpudrv_buffer_t));

    mmu_deinit(core_idx);
#endif

    vdb.size = 0;
    // get common memory information to free virtual address
    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_common_memory.phys_addr >= vdi->vpu_buffer_pool[i].vdb.phys_addr &&
            vdi->vpu_common_memory.phys_addr < (vdi->vpu_buffer_pool[i].vdb.phys_addr + vdi->vpu_buffer_pool[i].vdb.size))
        {
            vdi->vpu_buffer_pool[i].inuse = 0;
            vdi->vpu_buffer_pool_count--;
            vdb = vdi->vpu_buffer_pool[i].vdb;
            break;
        }
    }

    if (vdb.size > 0)
    {
        osal_memset(&vdi->vpu_common_memory, 0x00, sizeof(vpu_buffer_t));
    }

    vdi->task_num--;
    vpu_op_close(core_idx);
    vdi->vpu_fd = -1;
    osal_memset(vdi, 0x00, sizeof(vdi_info_t));
    vfree(s_vdi_info[core_idx]);
    s_vdi_info[core_idx] = NULL;

    return 0;
}

int vdi_get_common_memory(unsigned long core_idx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00) {
        return -1;
    }

    osal_memcpy(vb, &vdi->vpu_common_memory, sizeof(vpu_buffer_t));

    return 0;
}

int vdi_allocate_common_memory(unsigned long core_idx)
{
    vdi_info_t *vdi = s_vdi_info[core_idx];
    vpudrv_buffer_t vdb;
    vpu_buffer_t vb;
    int i;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    if(!vdi || vdi->vpu_fd==(VPU_FD)-1 || vdi->vpu_fd==(VPU_FD)0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    vdb.core_idx = core_idx;
    vdb.size = SIZE_COMMON;

    if (vpu_get_common_memory(&vdb) < 0)
    {
        VLOG(ERR, "[VDI] fail to vdi_allocate_common_memory size=%d\n", vdb.size);
        return -1;
    }

#ifdef MEDIA_V3
    vb.phys_addr = vdb.phys_addr;
    vb.size = vdb.size;
    mmu_alloc_virt_addr(core_idx, &vb);
    vdb.phys_addr = vb.phys_addr;
#endif
    VLOG(INFO, "[VDI] vdi_allocate_common_memory, physaddr=0x%lx, virtaddr=0x%lx\n", vdb.phys_addr, vdb.virt_addr);
    // convert os driver buffer type to vpu buffer type
    vdi->pvip->vpu_common_buffer.size = SIZE_COMMON;
    vdi->pvip->vpu_common_buffer.phys_addr = (unsigned long)(vdb.phys_addr);
    vdi->pvip->vpu_common_buffer.base = (unsigned long)(vdb.base);
    vdi->pvip->vpu_common_buffer.virt_addr = (unsigned long)(vdb.virt_addr);

    osal_memcpy(&vdi->vpu_common_memory, &vdi->pvip->vpu_common_buffer, sizeof(vpu_buffer_t));

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 0)
        {
            vdi->vpu_buffer_pool[i].vdb = vdb;
            vdi->vpu_buffer_pool_count++;
            vdi->vpu_buffer_pool[i].inuse = 1;
#ifdef MEDIA_V3
            vdi->vpu_buffer_pool[i].phys_addr_36bit = vb.phys_addr_36bit;
            vdi->vpu_buffer_pool[i].mmu_entry_index = vb.mmu_entry_index;
            vdi->vpu_buffer_pool[i].mmu_entry_num   = vb.mmu_entry_num;
#endif
            break;
        }
    }

#ifdef MEDIA_V3
    vdi_set_ddr_map(core_idx, vb.phys_addr_36bit >> 32);
#else
    vdi_set_ddr_map(core_idx, vdb.phys_addr >> 32);
#endif

    VLOG(INFO, "[VDI] vdi_get_common_memory physaddr=0x%lx ~ 0x%lx, size=%d, virtaddr=0x%lx\n", \
        vdi->vpu_common_memory.phys_addr, vdi->vpu_common_memory.phys_addr + vdi->vpu_common_memory.size,
        (int)vdi->vpu_common_memory.size, vdi->vpu_common_memory.virt_addr);

    return 0;
}

vpu_instance_pool_t *vdi_get_instance_pool(unsigned long core_idx)
{
    vdi_info_t *vdi;
    vpudrv_buffer_t vdb;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return NULL;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00 )
        return NULL;

    if (sizeof(CodecInst) > MAX_INST_HANDLE_SIZE) {
        VLOG(ERR, "[VDI] CodecInst = %d, MAX_INST_HANDLE_SIZE = %d\n",
                (int)sizeof(CodecInst), MAX_INST_HANDLE_SIZE);
    }

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));
    if (!vdi->pvip)
    {
        vdb.core_idx = core_idx;
        vdb.size = sizeof(vpu_instance_pool_t) + sizeof(MUTEX_HANDLE)*VDI_NUM_LOCK_HANDLES;

        if (vpu_get_instance_pool(&vdb) < 0)
        {
            VLOG(ERR, "[VDI] fail to allocate get instance pool physical space=%d\n",
                 vdb.size);
            return NULL;
        }

        vdb.virt_addr = vdb.base;
        if ((void *)vdb.virt_addr == NULL)
        {
            VLOG(ERR, "[VDI] fail to map instance pool phyaddr=0x%lx, size = %d\n", vdb.phys_addr, vdb.size);
            return NULL;
        }

        vdi->pvip = (vpu_instance_pool_t *)(vdb.virt_addr);
        VLOG(INFO, "[VDI] instance pool physaddr=0x%x, virtaddr=0x%x, base=0x%x, size=%d\n", (int)vdb.phys_addr, (int)vdb.virt_addr, (int)vdb.base, (int)vdb.size);
    }

    return (vpu_instance_pool_t *)vdi->pvip;
}

int vdi_open_instance(unsigned long core_idx, unsigned long inst_idx, int support_cq)
{
    vdi_info_t *vdi;
    vpudrv_inst_info_t inst_info = {0, };

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    inst_info.core_idx = core_idx;
    inst_info.inst_idx = inst_idx;
    inst_info.support_cq = support_cq;
    vdi->support_cq = support_cq;

    if (vpu_open_instance(&inst_info) < 0)
    {
        VLOG(ERR, "[VDI] fail to deliver open instance num inst_idx=%d\n", (int)inst_idx);
        return -1;
    }

    vdi->pvip->vpu_instance_num = inst_info.inst_open_count;

    return 0;
}

int vdi_close_instance(unsigned long core_idx, unsigned long inst_idx)
{
    vdi_info_t *vdi;
    vpudrv_inst_info_t inst_info = {0, };
    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    inst_info.core_idx = core_idx;
    inst_info.inst_idx = inst_idx;

    if (vpu_close_instance(&inst_info) < 0)
    {
        VLOG(ERR, "[VDI] fail to deliver open instance num inst_idx=%d\n", (int)inst_idx);
        return -1;
    }

    vdi->pvip->vpu_instance_num = inst_info.inst_open_count;

    return 0;
}

int vdi_get_instance_num(unsigned long core_idx)
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    return vdi->pvip->vpu_instance_num;
}


int vdi_hw_reset(unsigned long core_idx) // DEVICE_ADDR_SW_RESET
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    return vpu_hw_reset(core_idx);

}

int vdi_vpu_reset(unsigned long core_idx)
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    //return ioctl(vdi->vpu_fd, VDI_IOCTL_VPU_RESET, 0); // TODO
    return 0;
}

int vdi_lock(unsigned long core_idx)
{
    return vpu_core_lock(core_idx);
}

void vdi_unlock(unsigned long core_idx)
{
    vpu_core_unlock(core_idx);
}

int vdi_disp_lock(unsigned long core_idx)
{
    return vpu_disp_lock(core_idx);
}

void vdi_disp_unlock(unsigned long core_idx)
{
    vpu_disp_unlock(core_idx);
}

static int vmem_lock(unsigned long core_idx)
{
    return vpu_mem_lock(core_idx);
}

static void vmem_unlock(unsigned long core_idx)
{
    vpu_mem_unlock(core_idx);;
}

void vdi_write_register(unsigned long core_idx, unsigned int addr, unsigned int data)
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return;

    platform_write_register(addr + vdi->vdb_register.phys_addr, (unsigned int *)(addr + vdi->vdb_register.virt_addr), data);
}

unsigned int vdi_read_register(unsigned long core_idx, unsigned int addr)
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return (unsigned int)-1;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return (unsigned int)-1;

    return platform_read_register(addr + vdi->vdb_register.phys_addr, (unsigned int *)(addr + vdi->vdb_register.virt_addr));
}

#if defined(MEDIA_V3)
void vdi_write_top_register(unsigned long core_idx, unsigned int addr, unsigned int data)
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return;

    platform_write_register(addr + vdi->vdb_top_register.phys_addr, (unsigned int *)(addr + vdi->vdb_top_register.virt_addr), data);
}

unsigned int vdi_read_top_register(unsigned long core_idx, unsigned int addr)
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return (unsigned int)-1;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return (unsigned int)-1;

    return platform_read_register(addr + vdi->vdb_top_register.phys_addr, (unsigned int *)(addr + vdi->vdb_top_register.virt_addr));
}
#endif

#define FIO_TIMEOUT         100

unsigned int vdi_fio_read_register(unsigned long core_idx, unsigned int addr)
{
    unsigned int ctrl;
    unsigned int count = 0;
    unsigned int data  = 0xffffffff;

    ctrl  = (addr&0xffff);
    ctrl |= (0<<16);    /* read operation */
    vdi_write_register(core_idx, W5_VPU_FIO_CTRL_ADDR, ctrl);
    count = FIO_TIMEOUT;
    while (count--) {
        ctrl = vdi_read_register(core_idx, W5_VPU_FIO_CTRL_ADDR);
        if (ctrl & 0x80000000) {
            data = vdi_read_register(core_idx, W5_VPU_FIO_DATA);
            break;
        }
    }

    return data;
}

void vdi_fio_write_register(unsigned long core_idx, unsigned int addr, unsigned int data)
{
    unsigned int ctrl;
    unsigned int count = 0;

    vdi_write_register(core_idx, W5_VPU_FIO_DATA, data);
    ctrl  = (addr&0xffff);
    ctrl |= (1<<16);    /* write operation */
    vdi_write_register(core_idx, W5_VPU_FIO_CTRL_ADDR, ctrl);

    count = FIO_TIMEOUT;
    while (count--) {
        ctrl = vdi_read_register(core_idx, W5_VPU_FIO_CTRL_ADDR);
        if (ctrl & 0x80000000) {
            break;
        }
    }
}


int vdi_clear_memory(unsigned long core_idx, PhysicalAddress addr, int len, int endian)
{
    vdi_info_t *vdi;
    vpudrv_buffer_t vdb;
    int i;
    Uint8*  zero;
#ifdef PLATFORM_SOC
    unsigned long offset;
#endif

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 1)
        {
            vdb = vdi->vpu_buffer_pool[i].vdb;
            if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
                break;
			vdb.size = 0;
        }
    }

    if (!vdb.size) {
        VLOG(ERR, "address 0x%08x is not mapped address!!!\n", (int)addr);
        return -1;
    }

    zero = (Uint8*)osal_malloc(len);
    osal_memset((void*)zero, 0x00, len);

#ifdef PLATFORM_SOC
    offset = addr - (unsigned long)vdb.phys_addr;
    osal_memcpy((void *)((unsigned long)vdb.virt_addr+offset), zero, len);
#else
    pcie_memcpy_s2d(addr, zero, len);
#endif

    if (vdb.is_cached) {
#ifdef MEDIA_V3
        /* mmu_alloc_virt_addr() clobbered phys_addr with the VPU MMU-virtual
         * address; cache maintenance goes through phys_to_virt() on the
         * physical, so it must use the real 36-bit physical saved in the pool
         * (the MMU-virtual address would fault in phys_to_virt). */
        if (vdi->vpu_buffer_pool[i].phys_addr_36bit)
            vdb.phys_addr = vdi->vpu_buffer_pool[i].phys_addr_36bit;
#endif
        if (vpu_flush_dcache(&vdb) < 0) {
            VLOG(ERR, "[VDI] fail to fluch dcache mem addr 0x%lx size=%d\n", vdb.phys_addr, vdb.size);
        }
    }
    osal_free(zero);

    return len;
}

int vdi_set_memory(unsigned long core_idx, PhysicalAddress addr, int len, int endian, Uint32 data)
{
    vdi_info_t *vdi;
    vpudrv_buffer_t vdb;
    int i;
    Uint8*  zero;
#ifdef PLATFORM_SOC
    unsigned long offset;
#endif

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 1)
        {
            vdb = vdi->vpu_buffer_pool[i].vdb;
            if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
                break;
			vdb.size = 0;
        }
    }

    if (!vdb.size) {
        VLOG(ERR, "address 0x%08x is not mapped address!!!\n", (int)addr);
        return -1;
    }

    zero = (Uint8*)osal_malloc(len);
    osal_memset((void*)zero, data, len);

#ifdef PLATFORM_SOC
    offset = addr - (unsigned long)vdb.phys_addr;
    osal_memcpy((void *)((unsigned long)vdb.virt_addr+offset), zero, len);
#else
	pcie_memcpy_s2d(addr, zero, len);
#endif

    if (vdb.is_cached) {
#ifdef MEDIA_V3
        /* mmu_alloc_virt_addr() clobbered phys_addr with the VPU MMU-virtual
         * address; cache maintenance goes through phys_to_virt() on the
         * physical, so it must use the real 36-bit physical saved in the pool
         * (the MMU-virtual address would fault in phys_to_virt). */
        if (vdi->vpu_buffer_pool[i].phys_addr_36bit)
            vdb.phys_addr = vdi->vpu_buffer_pool[i].phys_addr_36bit;
#endif
        if (vpu_flush_dcache(&vdb) < 0) {
            VLOG(ERR, "[VDI] fail to fluch dcache mem addr 0x%lx size=%d\n", vdb.phys_addr, vdb.size);
        }
    }
    osal_free(zero);

    return len;
}

int vdi_write_memory(unsigned long core_idx, PhysicalAddress addr, unsigned char *data, int len, int endian)
{
    vdi_info_t *vdi;
    vpudrv_buffer_t vdb;
    int i;
#ifdef PLATFORM_SOC
    unsigned long offset;
#endif

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    if (!data)
        return -1;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 1)
        {
            vdb = vdi->vpu_buffer_pool[i].vdb;
            if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
                break;
            vdb.size = 0;
        }
    }

    if (!vdb.size) {
        VLOG(ERR, "address 0x%08x is not mapped address!!!\n", (int)addr);
        return -1;
    }

    swap_endian(core_idx, data, len, endian);
#ifdef PLATFORM_SOC
    offset = addr - (unsigned long)vdb.phys_addr;
    osal_memcpy((void *)((unsigned long)vdb.virt_addr+offset), data, len);
#else
    pcie_memcpy_s2d(addr, data, len);
#endif
    if (vdb.is_cached) {
#ifdef MEDIA_V3
        /* mmu_alloc_virt_addr() clobbered phys_addr with the VPU MMU-virtual
         * address; cache maintenance goes through phys_to_virt() on the
         * physical, so it must use the real 36-bit physical saved in the pool
         * (the MMU-virtual address would fault in phys_to_virt). */
        if (vdi->vpu_buffer_pool[i].phys_addr_36bit)
            vdb.phys_addr = vdi->vpu_buffer_pool[i].phys_addr_36bit;
#endif
        if (vpu_flush_dcache(&vdb) < 0) {
            VLOG(ERR, "[VDI] fail to fluch dcache mem addr 0x%lx size=%d\n", vdb.phys_addr, vdb.size);
            return -1;
        }
    }

    return len;
}

int vdi_read_memory(unsigned long core_idx, PhysicalAddress addr, unsigned char *data, int len, int endian)
{
    vdi_info_t *vdi;
    vpudrv_buffer_t vdb;
    int i;
    PhysicalAddress match_base = 0;
#ifdef PLATFORM_SOC
    unsigned long offset;
#endif

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd== (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 1)
        {
            vdb = vdi->vpu_buffer_pool[i].vdb;
            if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
            {
                match_base = vdb.phys_addr;
                break;
            }
            vdb.size = 0;
        }
    }

    if (!vdb.size) {
        return -1;
    }

    if (vdb.is_cached) {
        vpudrv_buffer_t vdb_cache = vdb;
#ifdef MEDIA_V3
        /* mmu_alloc_virt_addr() clobbered phys_addr with the VPU MMU-virtual
         * address; cache maintenance goes through phys_to_virt() on the
         * physical, so it must use the real 36-bit physical saved in the pool
         * (the MMU-virtual address would fault in phys_to_virt). Keep the
         * MMU-virtual addr in vdb for the offset math below. */
        if (vdi->vpu_buffer_pool[i].phys_addr_36bit)
            vdb_cache.phys_addr = vdi->vpu_buffer_pool[i].phys_addr_36bit;
#endif
        if (vpu_invalidate_dcache(&vdb_cache) < 0) {
            VLOG(ERR, "[VDI] fail to fluch dcache mem addr 0x%lx size=%d\n", vdb_cache.phys_addr, vdb_cache.size);
            return -1;
        }
    }
#ifdef PLATFORM_SOC
    offset = addr - (unsigned long)match_base;
    osal_memcpy(data, (const void *)((unsigned long)vdb.virt_addr+offset), len);
#else
    pcie_memcpy_d2s(data, addr, len);
#endif
    swap_endian(core_idx, data, len,  endian);

    return len;
}

int vdi_allocate_dma_memory(unsigned long core_idx, vpu_buffer_t *vb, char* buf_name, int instIndex)
{
    vdi_info_t *vdi;
    int i;
    vpudrv_buffer_t vdb;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    vdb.core_idx = core_idx;
    vdb.size = vb->size;

    if (vpu_allocate_physical_memory(&vdb, buf_name) < 0)
    {
        VLOG(ERR, "[VDI] fail to vdi_allocate_dma_memory type:%s, size=%d\n", buf_name, vdb.size);
        return -1;
    }

    vb->phys_addr = (unsigned long)vdb.phys_addr;
    vb->base = (unsigned long)vdb.base;
    vb->virt_addr = vdb.virt_addr;

#ifdef MEDIA_V3
    mmu_alloc_virt_addr(core_idx, vb);
    vdb.phys_addr = vb->phys_addr;
#endif

    vmem_lock(core_idx);
    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 0)
        {
            vdi->vpu_buffer_pool[i].vdb = vdb;
            vdi->vpu_buffer_pool_count++;
            vdi->vpu_buffer_pool[i].inuse = 1;
#ifdef MEDIA_V3
            vdi->vpu_buffer_pool[i].phys_addr_36bit = vb->phys_addr_36bit;
            vdi->vpu_buffer_pool[i].mmu_entry_index = vb->mmu_entry_index;
            vdi->vpu_buffer_pool[i].mmu_entry_num   = vb->mmu_entry_num;
#endif
            break;
        }
    }

    if (MAX_VPU_BUFFER_POOL == i) {
        VLOG(ERR, "[VDI] fail to vdi_allocate_dma_memory, vpu_buffer_pool_count=%d MAX_VPU_BUFFER_POOL=%d\n", vdi->vpu_buffer_pool_count, MAX_VPU_BUFFER_POOL);
        vmem_unlock(core_idx);
        return -1;
    }
    vmem_unlock(core_idx);

    VLOG(INFO, "[VDI] vdi_allocate_dma_memory, physaddr=0x%llx ~ 0x%lx, virtaddr=0x%llx~0x%llx, size=%d, mem type:%s, count:%d\n",
       vb->phys_addr, vb->phys_addr + vb->size, vb->virt_addr, vb->virt_addr + vb->size, vb->size, buf_name, vdi->vpu_buffer_pool_count);

    return 0;
}

int vdi_insert_extern_memory(unsigned long core_idx, vpu_buffer_t *vb, int memTypes, int instIndex)
{
    vdi_info_t *vdi;
    int i;
    vpudrv_buffer_t vdb;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];
    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    vdb.core_idx = core_idx;
    vdb.size = vb->size;
    vdb.phys_addr = vb->phys_addr;
    vdb.base = vb->base;
    vdb.virt_addr = vb->virt_addr;

    vmem_lock(core_idx);
    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 0)
        {
            vdi->vpu_buffer_pool[i].vdb = vdb;
            vdi->vpu_buffer_pool_count++;
            vdi->vpu_buffer_pool[i].inuse = 1;
#ifdef MEDIA_V3
            vdi->vpu_buffer_pool[i].phys_addr_36bit = vb->phys_addr_36bit;
            vdi->vpu_buffer_pool[i].mmu_entry_index = vb->mmu_entry_index;
            vdi->vpu_buffer_pool[i].mmu_entry_num   = vb->mmu_entry_num;
#endif
            break;
        }
    }

    if (MAX_VPU_BUFFER_POOL == i) {
        VLOG(ERR, "[VDI] fail to vdi_allocate_dma_memory, vpu_buffer_pool_count=%d MAX_VPU_BUFFER_POOL=%d\n", vdi->vpu_buffer_pool_count, MAX_VPU_BUFFER_POOL);
        vmem_unlock(core_idx);
        return -1;
    }
    vmem_unlock(core_idx);

    return 0;
}

unsigned long vdi_get_dma_memory_free_size(unsigned long core_idx)
{
    vdi_info_t *vdi;
    unsigned long size;

    vdi = s_vdi_info[core_idx];
    if (vpu_get_free_mem_size(&size) < 0) {
        VLOG(ERR, "[VDI] fail VDI_IOCTL_GET_FREE_MEM_SIZE size=%ld\n", size);
        return 0;
    }

    return size;
}

int vdi_attach_dma_memory(unsigned long core_idx, vpu_buffer_t *vb, unsigned char is_cached)
{
    vdi_info_t *vdi;
    int i;
    vpudrv_buffer_t vdb;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

#ifdef MEDIA_V3
    /* self-gated: when MMU enabled, overwrites vb->phys_addr with MMU virt addr and fills entry;
     * when disabled, leaves vb->phys_addr untouched and sets entry=-1. Symmetric with allocate. */
    mmu_alloc_virt_addr(core_idx, vb);
#endif

    vdb.size = vb->size;
    vdb.phys_addr = vb->phys_addr;
    vdb.base = vb->base;

    vdb.virt_addr = vb->virt_addr;
    vdb.is_cached = is_cached;

    vmem_lock(core_idx);
    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr)
        {
            vdi->vpu_buffer_pool[i].vdb = vdb;
            vdi->vpu_buffer_pool[i].inuse = 1;
#ifdef MEDIA_V3
            vdi->vpu_buffer_pool[i].phys_addr_36bit = vb->phys_addr_36bit;
            vdi->vpu_buffer_pool[i].mmu_entry_index = vb->mmu_entry_index;
            vdi->vpu_buffer_pool[i].mmu_entry_num   = vb->mmu_entry_num;
#endif
            break;
        }
        else
        {
            if (vdi->vpu_buffer_pool[i].inuse == 0)
            {
                vdi->vpu_buffer_pool[i].vdb = vdb;
                vdi->vpu_buffer_pool_count++;
                vdi->vpu_buffer_pool[i].inuse = 1;
#ifdef MEDIA_V3
                vdi->vpu_buffer_pool[i].phys_addr_36bit = vb->phys_addr_36bit;
                vdi->vpu_buffer_pool[i].mmu_entry_index = vb->mmu_entry_index;
                vdi->vpu_buffer_pool[i].mmu_entry_num   = vb->mmu_entry_num;
#endif
                break;
            }
        }
    }

    if (i == MAX_VPU_BUFFER_POOL) {
        vmem_unlock(core_idx);
        /* pool full (unreachable in practice: MAX_NUM_INSTANCE*100 slots). Signal the
         * failure so callers that check the return (e.g. vpuapi SET_ADDR_REP_USERDATA)
         * report it. Do NOT mmu_free the mapping here: the vdec bitstream/Ytbl/Ctbl
         * callers ignore the return and would then hand a freed MMU address to the VPU.
         * Any mapping leaked on this dead path mirrors vdi_allocate_dma_memory's behavior. */
        VLOG(ERR, "[VDI] vdi_attach_dma_memory fail: pool full, count=%d MAX=%d\n",
             vdi->vpu_buffer_pool_count, MAX_VPU_BUFFER_POOL);
        return -1;
    }
    vmem_unlock(core_idx);

    return 0;
}

int vdi_dettach_dma_memory(unsigned long core_idx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi;
    int i;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    if(!vb || !vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    if (vb->size == 0)
        return -1;

    vmem_lock(core_idx);
    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr)
        {
#ifdef MEDIA_V3
            /* recover MMU entry from pool (single source of truth) and free it.
             * self-gated: no-op when MMU disabled or entry_index<0. Symmetric with free. */
            vb->mmu_entry_index = vdi->vpu_buffer_pool[i].mmu_entry_index;
            vb->mmu_entry_num   = vdi->vpu_buffer_pool[i].mmu_entry_num;
            mmu_free_virt_addr(core_idx, vb);
#endif
            vdi->vpu_buffer_pool[i].inuse = 0;
            vdi->vpu_buffer_pool_count--;
            break;
        }
    }
    vmem_unlock(core_idx);

    return 0;
}

void vdi_free_dma_memory(unsigned long core_idx, vpu_buffer_t *vb, int memTypes, int instIndex)
{
    vdi_info_t *vdi;
    int i;
    vpudrv_buffer_t vdb;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return;

    vdi = s_vdi_info[core_idx];

    if(!vb || !vdi || vdi->vpu_fd== (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return;

    if (vb->size == 0)
        return ;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    vmem_lock(core_idx);
    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr)
        {
            vdi->vpu_buffer_pool[i].inuse = 0;
            vdi->vpu_buffer_pool_count--;
            vdb = vdi->vpu_buffer_pool[i].vdb;
#ifdef MEDIA_V3
            /* recover MMU entry + real phys from pool (single source of truth) */
            vb->phys_addr_36bit = vdi->vpu_buffer_pool[i].phys_addr_36bit;
            vb->mmu_entry_index = vdi->vpu_buffer_pool[i].mmu_entry_index;
            vb->mmu_entry_num   = vdi->vpu_buffer_pool[i].mmu_entry_num;
#endif
            break;
        }
    }

    if (!vdb.size)
    {
        VLOG(ERR, "[VDI] invalid buffer to free address = 0x%x\n", (int)vdb.virt_addr);
        vmem_unlock(core_idx);
        return ;
    }

#ifdef MEDIA_V3
    vdb.phys_addr = vb->phys_addr_36bit;
    mmu_free_virt_addr(core_idx, vb);
#endif
    vpu_free_physical_memory(&vdb);
    osal_memset(vb, 0, sizeof(vpu_buffer_t));
    vmem_unlock(core_idx);
}

int vdi_is_dma_memory(unsigned long core_idx, PhysicalAddress addr)
{
    vdi_info_t *vdi;
    int i;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return 0;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd== (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return 0;

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == addr)
            return 1;
    }

    return 0;
}

void vdi_remove_extern_memory(unsigned long core_idx, vpu_buffer_t *vb, int memTypes, int instIndex)
{
    vdi_info_t *vdi;
    int i;
    vpudrv_buffer_t vdb;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return;

    vdi = s_vdi_info[core_idx];
    if(!vb || !vdi || vdi->vpu_fd== (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return;

    if (vb->size == 0)
        return ;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    vmem_lock(core_idx);
    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr)
        {
            vdi->vpu_buffer_pool[i].inuse = 0;
            vdi->vpu_buffer_pool_count--;
            vdb = vdi->vpu_buffer_pool[i].vdb;
            break;
        }
    }

    if (!vdb.size)
    {
        VLOG(ERR, "[VDI] invalid buffer to free address = 0x%x\n", (int)vdb.virt_addr);
        vmem_unlock(core_idx);
        return ;
    }

    osal_memset(vb, 0, sizeof(vpu_buffer_t));
    vmem_unlock(core_idx);
}

int vdi_get_sram_memory(unsigned long core_idx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi = NULL;
    Uint32 sram_size = 0;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    if(!vb || !vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    switch (vdi->product_code) {
    case BODA950_CODE:
    case CODA960_CODE:
    case CODA980_CODE:
    // 4K : 0x34600, FHD : 0x17D00
        sram_size = 0x34600; break;
    case WAVE511_CODE:
    /* 10bit profile : 8Kx8K -> 129024, 4Kx2K -> 64512
     */
        sram_size = 0x1F800; break;
    case WAVE517_CODE:
    /* 10bit profile : 8Kx8K -> 272384, 4Kx2K -> 104448
     */
        sram_size = 0x18000; break;
    case WAVE537_CODE:
    /* 10bit profile : 8Kx8K -> 272384, 4Kx2K -> 104448
     */
        sram_size = 0x42800; break;
    case WAVE521_CODE:
    /* 10bit profile : 8Kx8K -> 126976, 4Kx2K -> 63488
     */
        sram_size = 0x1F000; break;
    case WAVE521E1_CODE:
    /* 10bit profile : 8Kx8K -> 126976, 4Kx2K -> 63488
     */
        sram_size = 0x1F000; break;
    case WAVE521C_CODE:
    /* 10bit profile : 8Kx8K -> 129024, 4Kx2K -> 64512
     * NOTE: Decoder > Encoder
     */
        sram_size = 0xEC00; break;
    case WAVE521C_DUAL_CODE:
    /* 10bit profile : 8Kx8K -> 129024, 4Kx2K -> 64512
     * NOTE: Decoder > Encoder
     */
        sram_size = 0x1F800; break;
    case WAVE617_CODE:
    case WAVE627_CODE:
    case WAVE633_CODE:
    case WAVE637_CODE:
    case WAVE663_CODE:
    case WAVE677_CODE:
        sram_size = 0x1C1500; break;
    default:
        VLOG(ERR, "[VDI] check product_code(%x)\n", vdi->product_code);
        break;
    }


    // if we can know the sram address directly in vdi layer, we use it first for sdram address
    vb->phys_addr = VDI_SRAM_BASE_ADDR;
    vb->size      = sram_size;

    return 0;
}

int vdi_set_clock_gate(unsigned long core_idx, int enable)
{
    vdi_info_t *vdi = NULL;
    int ret;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    if (!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    if (PRODUCT_CODE_W5_SERIES(vdi->product_code) || PRODUCT_CODE_W6_SERIES(vdi->product_code) || vdi->product_code == 0) {
        //CommandQueue does not support clock gate
        //first value 0
        vdi->clock_state = 1;
        return 0;
    }

    vdi->clock_state = enable;
    ret = 0;//vpu_set_clock_gate(&enable);

    return ret;
}

int vdi_get_clock_gate(unsigned long core_idx)
{
    vdi_info_t *vdi;
    int ret;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    ret = vdi->clock_state;
    return ret;
}

static int get_pc_addr(Uint32 product_code)
{
    if (PRODUCT_CODE_W_SERIES(product_code)) {
        return W5_VCPU_CUR_PC;
    }
    else if (PRODUCT_CODE_CODA_SERIES(product_code)) {
        return BIT_CUR_PC;
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", product_code);
        return -1;
    }
}

int vdi_wait_bus_busy(unsigned long core_idx, int timeout, unsigned int gdi_busy_flag)
{
    Uint64 elapse, cur;
    Uint32 pc;
    vdi_info_t *vdi;
    Uint32 gdi_status_check_value = 0x3f;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    elapse = osal_gettime();

    pc = get_pc_addr(vdi->product_code);
    if (pc == (Uint32)-1)
        return -1;
    if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
        gdi_status_check_value = 0x3f;
        if (PRODUCT_CODE_W6_SERIES(vdi->product_code)) {
            gdi_status_check_value = 0;
        }
        if (vdi->product_code == WAVE521C_CODE || vdi->product_code == WAVE521_CODE || vdi->product_code == WAVE521E1_CODE) {
            gdi_status_check_value = 0x00ff1f3f;
        }
    }
    while(1)
    {
        if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
            if (vdi_fio_read_register(core_idx, gdi_busy_flag) == gdi_status_check_value) break;
        }
        else if (PRODUCT_CODE_CODA_SERIES(vdi->product_code)) {
            if (vdi_read_register(core_idx, gdi_busy_flag) == 0x77) break;
        }
        else {
            VLOG(ERR, "Unknown product id : %08x\n", vdi->product_code);
            return -1;
        }

        if (timeout > 0) {
            cur = osal_gettime();

            if ((cur - elapse) > timeout) {
                print_busy_timeout_status(core_idx, vdi->product_code, pc);
                return -1;
            }
        }
        usleep_range(5, 10);    // delay more to give idle time to OS;
    }
    return 0;
}

int vdi_wait_vpu_busy(unsigned long core_idx, int timeout, unsigned int addr_bit_busy_flag)
{
    Uint64 elapse, cur;
    Uint32 pc;
    vdi_info_t *vdi;
    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    elapse = osal_gettime();

    pc = get_pc_addr(vdi->product_code);
    if (pc == (Uint32)-1)
        return -1;
    while(1)
    {
        if (vdi_read_register(core_idx, addr_bit_busy_flag) == 0)
            break;

        if (timeout > 0) {
            cur = osal_gettime();

            if ((cur - elapse) > timeout) {
                print_busy_timeout_status(core_idx, vdi->product_code, pc);
                return -1;
            }
        }
        usleep_range(5, 10);   // delay more to give idle time to OS;
    }
    return 0;
}

int vdi_wait_vcpu_bus_busy(unsigned long core_idx, int timeout, unsigned int gdi_busy_flag)
{
    Uint64 elapse, cur;
    Uint32 pc;
    vdi_info_t *vdi;
    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    elapse = osal_gettime();

    pc = get_pc_addr(vdi->product_code);
    if (pc == (Uint32)-1)
        return -1;
    while(1)
    {
        if (vdi_fio_read_register(core_idx, gdi_busy_flag) == 0x00)
            break;
        if (timeout > 0) {
            cur = osal_gettime();

            if ((cur - elapse) > timeout) {
                print_busy_timeout_status(core_idx, vdi->product_code, pc);
                return -1;
            }
        }
        usleep_range(5, 10);   // delay more to give idle time to OS;
    }
    return 0;
}

int vdi_wait_interrupt(unsigned long core_idx, unsigned int instIdx, int timeout)
{
    vdi_info_t *vdi = s_vdi_info[core_idx];
    int intr_reason = -1;
    int ret;
    vpudrv_intr_info_t intr_info;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    intr_info.core_idx = core_idx;
    intr_info.timeout     = timeout;
    intr_info.intr_reason = 0;
    intr_info.intr_inst_index = instIdx;

    if (!(vdi->support_cq)) {
        intr_info.intr_inst_index = 0;
    }

    ret = vpu_wait_interrupt(&intr_info);
    if (ret != 0) {
        if (ret == -ETIME)
            return -1;
        else
            return -2;
    }

    intr_reason = intr_info.intr_reason;

    return intr_reason;
}

//------------------------------------------------------------------------------
// LOG & ENDIAN functions
//------------------------------------------------------------------------------

int vdi_get_system_endian(unsigned long core_idx)
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
        return VDI_128BIT_BUS_SYSTEM_ENDIAN;
    }
    else if(PRODUCT_CODE_CODA_SERIES(vdi->product_code)) {
        return VDI_SYSTEM_ENDIAN;
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", vdi->product_code);
        return -1;
    }
}

int vdi_convert_endian(unsigned long core_idx, unsigned int endian)
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
        switch (endian) {
        case VDI_LITTLE_ENDIAN:       endian = 0x00; break;
        case VDI_BIG_ENDIAN:          endian = 0x0f; break;
        case VDI_32BIT_LITTLE_ENDIAN: endian = 0x04; break;
        case VDI_32BIT_BIG_ENDIAN:    endian = 0x03; break;
        }
    }
    else if(PRODUCT_CODE_CODA_SERIES(vdi->product_code)) {
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", vdi->product_code);
        return -1;
    }

    return (endian&0x0f);
}

static Uint32 convert_endian_coda9_to_wave4(Uint32 endian)
{
    Uint32 converted_endian = endian;
    switch(endian) {
    case VDI_LITTLE_ENDIAN:       converted_endian = 0; break;
    case VDI_BIG_ENDIAN:          converted_endian = 7; break;
    case VDI_32BIT_LITTLE_ENDIAN: converted_endian = 4; break;
    case VDI_32BIT_BIG_ENDIAN:    converted_endian = 3; break;
    }
    return converted_endian;
}



int swap_endian(unsigned long core_idx, unsigned char *data, int len, int endian)
{
    vdi_info_t *vdi;
    int changes;
    int sys_endian;
    BOOL byteChange, wordChange, dwordChange, lwordChange;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == (VPU_FD)-1 || vdi->vpu_fd == (VPU_FD)0x00)
        return -1;

    if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
        sys_endian = VDI_128BIT_BUS_SYSTEM_ENDIAN;
    }
    else if(PRODUCT_CODE_CODA_SERIES(vdi->product_code)) {
        sys_endian = VDI_SYSTEM_ENDIAN;
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", vdi->product_code);
        return -1;
    }

    endian     = vdi_convert_endian(core_idx, endian);
    sys_endian = vdi_convert_endian(core_idx, sys_endian);
    if (endian == sys_endian)
        return 0;

    if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
    }
    else if (PRODUCT_CODE_CODA_SERIES(vdi->product_code)) {
        endian     = convert_endian_coda9_to_wave4(endian);
        sys_endian = convert_endian_coda9_to_wave4(sys_endian);
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", vdi->product_code);
        return -1;
    }

    changes     = endian ^ sys_endian;
    byteChange  = changes&0x01;
    wordChange  = ((changes&0x02) == 0x02);
    dwordChange = ((changes&0x04) == 0x04);
    lwordChange = ((changes&0x08) == 0x08);

    if (byteChange)  byte_swap(data, len);
    if (wordChange)  word_swap(data, len);
    if (dwordChange) dword_swap(data, len);
    if (lwordChange) lword_swap(data, len);

    return 1;
}

int vdi_set_ddr_map(unsigned long core_idx, unsigned int ext_addr)
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    vdi->ext_addr = ext_addr;

    return 0;
}

int vdi_get_ddr_map(unsigned long core_idx)
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = s_vdi_info[core_idx];

    return  vdi->ext_addr;
}

int vdi_get_suspend_state(void)
{
#ifdef VC_SUPPORT_CLOCK_CONTROL
    return vpu_get_suspend_state();
#else
    return 0;
#endif
}

#endif	//#if defined(linux) || defined(__linux) || defined(ANDROID)
