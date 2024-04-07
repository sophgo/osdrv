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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/io.h>

#include "driver/jpu.h"
#include "../jdi.h"
#include "jpulog.h"
#include "jputypes.h"
#include "regdefine.h"

#include <linux/delay.h>

#define JPU_DEVICE_NAME "/dev/jpu"
#define JDI_INSTANCE_POOL_SIZE          sizeof(jpu_instance_pool_t)
#define JDI_INSTANCE_POOL_TOTAL_SIZE    (JDI_INSTANCE_POOL_SIZE + sizeof(MUTEX_HANDLE)*JDI_NUM_LOCK_HANDLES)
typedef void *  		MUTEX_HANDLE;
#endif /* CNM_FPGA_PLATFORM */

#if defined(CNM_FPGA_PLATFORM)
static int io_lock(void);
static int io_unlock(void);
#endif

extern int jpu_ioctl_GET_REGISTER_INFO(jpudrv_buffer_t *arg);
extern int jpu_ioctl_reset(unsigned int core_idx);
extern int jpu_ioctl_WAIT_INTERRUPT(jpudrv_intr_info_t *arg);
extern int jpu_ioctl_FREE_PHYSICAL_MEMORY(jpudrv_buffer_t *arg);
extern int jpu_ioctl_ALLOCATE_PHYSICAL_MEMORY(jpudrv_buffer_t *arg);
extern int jpu_ioctl_INVALIDATE_CACHE(jpudrv_buffer_t *arg);
extern int jpu_ioctl_FLUSH_CACHE(jpudrv_buffer_t *arg);
extern int jpu_ioctl_close_instance_core_index(unsigned int instIdx);
extern int jpu_ioctl_get_max_jpu_core(void);
extern int jpu_ioctl_GET_INSTANCE_CORE_INDEX(int timeout);
//extern int jpu_ioctl_get_instance_core_index(jpudrv_remap_info_t *pinfo);
extern int jpu_ioctl_get_max_jpu_core(void);
extern int jpu_open_device(void);
extern int jpu_ioctl_getinstancepool(jpudrv_buffer_t* arg);
extern int jpu_ioctl_open_instance(jpudrv_inst_info_t *instInfo);
extern int jpu_ioctl_close_instance(jpudrv_inst_info_t *instInfo);
extern int jpu_ioctl_set_clock_gate(int *enable);
extern uint32_t jpu_get_extension_address(int core_idx);
extern void jpu_set_extension_address(int core_idx, uint32_t addr);
extern void jpu_sw_top_reset(int core_idx);

static Uint32 jdi_core_stat_fps[MAX_NUM_JPU_CORE] = {0};
static Uint64 jdi_core_stat_lastts[MAX_NUM_JPU_CORE] = {0};
static int jpu_show_fps = 0;
module_param(jpu_show_fps, uint, 0644);


/***********************************************************************************
*
***********************************************************************************/
#define JDI_DRAM_PHYSICAL_BASE          0x00
#define JDI_DRAM_PHYSICAL_SIZE          (4*1024*1024*1024)
#define JDI_SYSTEM_ENDIAN               JDI_LITTLE_ENDIAN
#define JPU_REG_SIZE                    0x300
#define JDI_NUM_LOCK_HANDLES            4

typedef struct jpudrv_buffer_pool_t
{
    jpudrv_buffer_t jdb;
    BOOL            inuse;
} jpudrv_buffer_pool_t;

typedef struct  {
    Int32                   jpu_fd;
    jpu_instance_pool_t*    pjip;
    Int32                   task_num;
    Int32                   clock_state;
    jpudrv_buffer_t         jdb_register[MAX_NUM_JPU_CORE];
    jpudrv_buffer_pool_t    jpu_buffer_pool[MAX_JPU_BUFFER_POOL];
    Int32                   jpu_buffer_pool_count;
//    void*                   jpu_mutex;
    struct mutex            jpu_mutex;
} jdi_info_t;

static jdi_info_t s_jdi_info;

static Int32 swap_endian(BYTE* data, size_t len, Uint32 endian);

extern int is_jpu_init;

unsigned long jdi_get_extension_address(int coreidx)
{
    return jpu_get_extension_address(coreidx);
}

void jdi_set_extension_address(int coreidx, unsigned long addr)
{
    jpu_set_extension_address(coreidx, addr);
}

void jdi_sw_top_reset(int coreidx)
{
    jpu_sw_top_reset(coreidx);
}

pid_t mapCorePid[MAX_NUM_JPU_CORE] = {0};
int jdi_core_hash_table_set_core(pid_t pid, int coreidx)
{
    if (mapCorePid[coreidx] != 0) {
        JLOG(ERR, "core:%d already map to pid:%d \n", coreidx, pid);
        return -1;
    }

    mapCorePid[coreidx] = pid;
    return 0;
}


int jdi_core_hash_table_get_core(pid_t pid)
{
    int i;
    for (i = 0; i < MAX_NUM_JPU_CORE; i++) {
        if (mapCorePid[i] == pid) {
            return i;
        }
    }

    return -1;
}

void jdi_core_hash_table_clear_core(int coreidx)
{
    mapCorePid[coreidx] = 0;
    jpu_ioctl_close_instance_core_index(coreidx);

    return;
}

int jdi_core_request_one_core(int timeout)
{
    return jpu_ioctl_GET_INSTANCE_CORE_INDEX(timeout);
}


int jdi_probe(void)
{
    int ret;

    ret = jdi_init();
    jdi_release();

    return ret;
}

/* @return number of tasks.
 */
int jdi_get_task_num(void)
{
    jdi_info_t *jdi;

    jdi = &s_jdi_info;

    if (jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00) {
        return 0;
    }

    return jdi->task_num;
}

void jdi_delay_us(unsigned int us)
{
    usleep_range(us,2*us);
}
int jdi_init(void)
{
    jdi_info_t *jdi;
    int i;

    jdi = &s_jdi_info;

    if (jdi->jpu_fd != -1 && jdi->jpu_fd != 0x00)
    {
        jdi_lock();
        jdi->task_num++;
        jdi_unlock();
        return 0;
    }

#ifdef ANDROID
    system("/system/lib/modules/load_android.sh");
#endif /* ANDROID */

    jdi->jpu_fd = jpu_open_device();
    if (jdi->jpu_fd < 0) {
        JLOG(ERR, "[JDI] Can't open jpu driver. try to run jdi/linux/driver/load.sh script \n");
        return -1;
    }

    memset(jdi->jpu_buffer_pool, 0x00, sizeof(jpudrv_buffer_pool_t)*MAX_JPU_BUFFER_POOL);

    if (!jdi_get_instance_pool()) {
        JLOG(ERR, "[JDI] fail to create instance pool for saving context \n");
        goto ERR_JDI_INIT;
    }

    if (jdi->pjip->instance_pool_inited == FALSE) {
        Uint32* pCodecInst;

        mutex_init(&jdi->jpu_mutex);
        for( i = 0; i < MAX_JPEG_NUM_INSTANCE; i++) {
            pCodecInst    = (Uint32*)jdi->pjip->jpgInstPool[i];
            pCodecInst[1] = i;    // indicate instIndex of CodecInst
            pCodecInst[0] = 0;    // indicate inUse of CodecInst
        }

        jdi->pjip->instance_pool_inited = TRUE;
    }

    jpu_ioctl_GET_REGISTER_INFO(jdi->jdb_register);
    jdi_set_clock_gate(1);

    if (jdi_lock() < 0)
    {
        JLOG(ERR, "[JDI] fail to handle lock function\n");
        goto ERR_JDI_INIT;
    }

    jdi->task_num++;
    jdi_unlock();

    JLOG(INFO, "[JDI] success to init driver \n");
    return 0;

ERR_JDI_INIT:
    jdi_unlock();
    jdi_release();
    return -1;
}

int jdi_release(void)
{
    jdi_info_t *jdi;

    jdi = &s_jdi_info;
    if (!jdi || jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00) {
        return 0;
    }

    if (jdi_lock() < 0) {
        JLOG(ERR, "[JDI] fail to handle lock function\n");
        return -1;
    }

    if (jdi->task_num == 0) {
        JLOG(ERR, "[JDI] %s:%d task_num is 0\n", __FUNCTION__, __LINE__);
        jdi_unlock();
        return 0;
    }

    jdi->task_num--;
    if (jdi->task_num > 0) {// means that the opened instance remains
        jdi_unlock();
        return 0;
    }

   // core_idx = jdi_core_hash_table_get_core(task_pid_nr(current));
   // iounmap(jdi->jdb_register[core_idx].virt_addr);
    jdi_unlock();
   // memset(jdi, 0x00, sizeof(jdi_info_t));

    return 0;
}

jpu_instance_pool_t *jdi_get_instance_pool(void)
{
    jdi_info_t *jdi;
    jpudrv_buffer_t jdb = {0};

    jdi = &s_jdi_info;

    if(!jdi || jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00 )
        return NULL;

    memset(&jdb, 0x00, sizeof(jpudrv_buffer_t));
    if (!jdi->pjip) {
        jdb.size = JDI_INSTANCE_POOL_TOTAL_SIZE;
        jpu_ioctl_getinstancepool(&jdb);
        //jdb.virt_addr = vmalloc(jdb.size);
        jdi->pjip      = (jpu_instance_pool_t *)jdb.base;//lint !e511
        //change the pointer of jpu_mutex to at end pointer of jpu_instance_pool_t to assign at allocated position.
        mutex_init(&jdi->jpu_mutex);
        JLOG(INFO, "[JDI] instance pool physaddr=0x%lx, virtaddr=0x%lx, base=0x%lx, size=%ld\n", jdb.phys_addr, jdb.virt_addr, jdb.base, jdb.size);
    }

    return (jpu_instance_pool_t *)jdi->pjip;
}

int jdi_open_instance(unsigned long inst_idx)
{
    jdi_info_t *jdi;
    jpudrv_inst_info_t inst_info;

    jdi = &s_jdi_info;

    if(!jdi || jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00)
        return -1;

    inst_info.inst_idx = inst_idx;
    jpu_ioctl_open_instance(&inst_info);
    jdi->pjip->jpu_instance_num = inst_info.inst_open_count;

    return 0;
}

int jdi_close_instance(unsigned long inst_idx)
{
    jdi_info_t *jdi;
    jpudrv_inst_info_t inst_info;

    jdi = &s_jdi_info;

    if(!jdi || jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00)
        return -1;

    inst_info.inst_idx = inst_idx;
    //inst_info.core_idx = jdi_core_hash_table_get_value(task_pid_nr(current));
    jpu_ioctl_close_instance(&inst_info);
    jdi->pjip->jpu_instance_num = inst_info.inst_open_count;

    return 0;
}
int jdi_get_instance_num(void)
{
    jdi_info_t *jdi;
    jdi = &s_jdi_info;

    if(!jdi || jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00)
        return -1;

    return jdi->pjip->jpu_instance_num;
}

int jdi_hw_reset(void)
{
    jdi_info_t *jdi;
    int core_idx;
    jdi = &s_jdi_info;

    if(!jdi || jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00)
        return -1;

    core_idx = jdi_core_hash_table_get_core(task_pid_nr(current));
    return jpu_ioctl_reset(core_idx);
}

int jdi_lock(void)
{
    jdi_info_t *jdi = &s_jdi_info;

    if(!jdi || jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00) {
        JLOG(ERR, "%s:%d JDI handle isn't initialized\n", __FUNCTION__, __LINE__);
        return -1;
    }

    mutex_lock(&jdi->jpu_mutex);
    return 0;//lint !e454
}

void jdi_unlock(void)
{
    jdi_info_t *jdi;

    jdi = &s_jdi_info;

    if(!jdi || jdi->jpu_fd <= 0)
        return;

    mutex_unlock(&jdi->jpu_mutex);
}



void jdi_write_register_ext(int core_idx, unsigned long addr, unsigned int data)
{
    jdi_info_t *jdi = &s_jdi_info;
    unsigned long *reg_addr;

    if(!jdi || jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00)
        return;

#if defined(CNM_FPGA_PLATFORM)
    if(!jdi->clock_state)
    {
        JLOG(ERR, "[JDI] jdi_write_register clock is in off. enter infinite loop\n");
        while (1);
    }
#endif

    reg_addr = (unsigned long *)(addr + (unsigned long)jdi->jdb_register[core_idx].virt_addr);
    //JLOG(INGO, "jdi_write_register core[%d] reg_addr:%x data:%x\n", core_idx, addr, data);
    *(volatile unsigned int *)reg_addr = data;
}

unsigned long jdi_read_register_ext(int core_idx, unsigned long addr)
{
    jdi_info_t *jdi;
#if defined(CNM_FPGA_PLATFORM)
#else
    unsigned long *reg_addr;
#endif

    jdi = &s_jdi_info;

    if(!jdi || jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00)
        return (unsigned int)-1;

#if defined(CNM_FPGA_PLATFORM)
    if(!jdi->clock_state)
    {
        JLOG(ERR, "[JDI] jdi_read_register clock is in off. enter infinite loop\n");
        while (1);
    }
#endif

    reg_addr = (unsigned long *)(addr + (unsigned long)jdi->jdb_register[core_idx].virt_addr);
    //JLOG(ERR, "jdi_read_register core[%d] reg_addr:0x%lx\n", core_idx, reg_addr);
    return *(volatile unsigned int *)reg_addr;
}




void jdi_write_register(unsigned long addr, unsigned int data)
{
    jdi_info_t *jdi = &s_jdi_info;
    int core_idx;
    unsigned long *reg_addr;

    if(!jdi || jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00)
        return;

#if defined(CNM_FPGA_PLATFORM)
    if(!jdi->clock_state)
    {
        JLOG(ERR, "[JDI] jdi_write_register clock is in off. enter infinite loop\n");
        while (1);
    }
#endif

    core_idx = jdi_core_hash_table_get_core(task_pid_nr(current));
    if (core_idx < 0 || core_idx >= MAX_NUM_JPU_CORE) {
        JLOG(ERR, "jdi_read_register invalid core:%d\n", core_idx);
        return;
    }
    reg_addr = (unsigned long *)(addr + (unsigned long)jdi->jdb_register[core_idx].virt_addr);
    //JLOG(INFO, "jdi_write_register core[%d] reg_addr:%x data:%x\n", core_idx, addr, data);
    *(volatile unsigned int *)reg_addr = data;
}

unsigned long jdi_read_register(unsigned long addr)
{
    jdi_info_t *jdi;
    int core_idx;
    unsigned long *reg_addr;

    jdi = &s_jdi_info;

    if(!jdi || jdi->jpu_fd == -1 || jdi->jpu_fd == 0x00)
        return (unsigned int)-1;

#if defined(CNM_FPGA_PLATFORM)
    if(!jdi->clock_state)
    {
        JLOG(ERR, "[JDI] jdi_read_register clock is in off. enter infinite loop\n");
        while (1);
    }
#endif

    core_idx = jdi_core_hash_table_get_core(task_pid_nr(current));
    if (core_idx < 0 || core_idx >= MAX_NUM_JPU_CORE) {
        JLOG(ERR, "jdi_read_register invalid core:%d\n", core_idx);
        return 0;
    }
    reg_addr = (unsigned long *)(addr + (unsigned long)jdi->jdb_register[core_idx].virt_addr);
    // JLOG(INFO, "jdi_read_register core[%d] reg_addr:0x%x\n", core_idx, addr);
    return *(volatile unsigned int *)reg_addr;
}

size_t jdi_write_memory(unsigned long addr, unsigned char *data, size_t len, int endian)
{
    jdi_info_t *jdi;
    jpudrv_buffer_t jdb;
    Uint32          offset;
    Uint32          i;

    jdi = &s_jdi_info;

    if(!jdi || jdi->jpu_fd==-1 || jdi->jpu_fd == 0x00)
        return 0;

    memset(&jdb, 0x00, sizeof(jpudrv_buffer_t));

    jdi_lock();
    for (i=0; i<MAX_JPU_BUFFER_POOL; i++)
    {
        if (jdi->jpu_buffer_pool[i].inuse == 1)
        {
            jdb = jdi->jpu_buffer_pool[i].jdb;
            if (addr >= jdb.phys_addr && addr < (jdb.phys_addr + jdb.size)) {
                break;
            }
        }
    }
    jdi_unlock();

    if (i == MAX_JPU_BUFFER_POOL) {
        JLOG(ERR, "%s NOT FOUND ADDRESS: %08lx\n", __FUNCTION__, addr);
        return 0;
    }

    if (!jdb.size) {
        JLOG(ERR, "address 0x%08lx is not mapped address!!!\n", addr);
        return 0;
    }

    offset = addr - (unsigned long)jdb.phys_addr;
    if (data == NULL || (jdb.virt_addr+offset) == 0 ) {
        return 0;
    }
    swap_endian(data, len, endian);
    memcpy((void *)((unsigned long)jdb.virt_addr+offset), data, len);

    if(jdb.is_cached)
    {
        if (jpu_ioctl_FLUSH_CACHE(&jdb) < 0) {
            JLOG(ERR, "fail to fluch dcache mem addr 0x%lx size=%ld\n", jdb.phys_addr, jdb.size);
            return -1;
        }
    }

    return len;
}

size_t jdi_read_memory(unsigned long addr, unsigned char *data, size_t len, int endian)
{
    jdi_info_t *jdi;
    jpudrv_buffer_t jdb;
#if defined(CNM_FPGA_PLATFORM)
#else
    unsigned long offset;
#endif
    int i;

    jdi = &s_jdi_info;

    if(!jdi || jdi->jpu_fd==-1 || jdi->jpu_fd == 0x00)
        return -1;

    memset(&jdb, 0x00, sizeof(jpudrv_buffer_t));

    jdi_lock();
    for (i=0; i<MAX_JPU_BUFFER_POOL; i++)
    {
        if (jdi->jpu_buffer_pool[i].inuse == 1)
        {
            jdb = jdi->jpu_buffer_pool[i].jdb;
            if (addr >= jdb.phys_addr && addr < (jdb.phys_addr + jdb.size))
                break;
        }
    }
    jdi_unlock();

    if (len == 0) {
        return 0;
    }

    if (!jdb.size)
        return -1;

    offset = addr - (unsigned long)jdb.phys_addr;

    if(jdb.is_cached)
    {
        if (jpu_ioctl_INVALIDATE_CACHE(&jdb) < 0) {
            JLOG(ERR, "fail to invalidate dcache mem addr 0x%lx size=%ld\n", jdb.phys_addr, jdb.size);
            return -1;
        }
    }

    memcpy(data, (const void *)((unsigned long)jdb.virt_addr+offset), len);
    swap_endian(data, len,  endian);

    return len;
}

extern int32_t sys_ion_alloc(uint64_t *p_paddr, void **pp_vaddr, uint8_t *buf_name, uint32_t buf_len, bool is_cached);
extern int32_t sys_ion_free(uint64_t u64PhyAddr);



int jdi_allocate_dma_memory(jpu_buffer_t *vb)
{
    jdi_info_t *jdi;
    int i;
    jpudrv_buffer_t jdb;

    jdi = &s_jdi_info;

    if(!jdi || jdi->jpu_fd==-1 || jdi->jpu_fd == 0x00)
        return -1;

    memset(&jdb, 0x00, sizeof(jpudrv_buffer_t));

    jdb.size = vb->size;
    jdb.is_cached = vb->is_cached;
    if(jpu_ioctl_ALLOCATE_PHYSICAL_MEMORY(&jdb)) {
        JLOG(ERR, "alloc memory fail, size:%ld\n", jdb.size);
        return -1;
    }

    if (jdb.phys_addr == 0) {
        JLOG(ERR, "Zero addr\n");
        return -1;
    }

    vb->phys_addr = (unsigned long)jdb.phys_addr;
    vb->base = (unsigned long)jdb.base;
    //jdb.virt_addr = ioremap(vb->phys_addr, jdb.size);
    vb->virt_addr = (unsigned long )jdb.virt_addr;

    jdi_lock();
    for (i=0; i<MAX_JPU_BUFFER_POOL; i++)
    {
        if (jdi->jpu_buffer_pool[i].inuse == 0)
        {
            jdi->jpu_buffer_pool[i].jdb = jdb;
            jdi->jpu_buffer_pool_count++;
            jdi->jpu_buffer_pool[i].inuse = 1;
            break;
        }
    }
    jdi_unlock();
    JLOG(INFO, "[JDI] jdi_allocate_dma_memory, physaddr=0x%lx, virtaddr=0x%lx~0x%lx, size=0x%lx\n",
         vb->phys_addr, vb->virt_addr, vb->virt_addr + vb->size, vb->size);
    return 0;
}

void jdi_free_dma_memory(jpu_buffer_t *vb)
{
    jdi_info_t *jdi;
    int i;
    jpudrv_buffer_t jdb;


    jdi = &s_jdi_info;

    if(!vb || !jdi || jdi->jpu_fd==-1 || jdi->jpu_fd == 0x00)
        return;

    if (vb->size == 0)
        return ;

    memset(&jdb, 0x00, sizeof(jpudrv_buffer_t));

    jdi_lock();
    for (i=0; i<MAX_JPU_BUFFER_POOL; i++) {
        if (jdi->jpu_buffer_pool[i].jdb.phys_addr == vb->phys_addr) {
            jdi->jpu_buffer_pool[i].inuse = 0;
            jdi->jpu_buffer_pool_count--;
            jdb = jdi->jpu_buffer_pool[i].jdb;
            break;
        }
    }
    jdi_unlock();

    if (!jdb.size)
    {
        JLOG(ERR, "[JDI] invalid buffer to free address = 0x%lx, vb addr:%lx, size:%ld\n"
            , jdb.virt_addr, vb->phys_addr, vb->size);
        return ;
    }

    jpu_ioctl_FREE_PHYSICAL_MEMORY(&jdb);
    //sys_ion_free(jdb.phys_addr);
    memset(&jdb, 0, sizeof(jpudrv_buffer_t));
    memset(vb, 0, sizeof(jpu_buffer_t));
}

int jdi_invalidate_cache(jpu_buffer_t *vb)
{
    jpudrv_buffer_t jdb;

    memset(&jdb, 0x00, sizeof(jpudrv_buffer_t));

    jdb.size = vb->size;
    jdb.phys_addr = vb->phys_addr;
    jdb.virt_addr = vb->virt_addr;
    jdb.is_cached = vb->is_cached;

    if(jpu_ioctl_INVALIDATE_CACHE(&jdb)) {
        JLOG(ERR, "jpu_ioctl_INVALIDATE_CACHE failed, size:%ld\n", jdb.size);
        return -1;
    }

    return 0;
}

int jdi_flush_cache(jpu_buffer_t *vb)
{
    jpudrv_buffer_t jdb;

    memset(&jdb, 0x00, sizeof(jpudrv_buffer_t));

    jdb.size = vb->size;
    jdb.phys_addr = vb->phys_addr;
    jdb.virt_addr = vb->virt_addr;
    jdb.is_cached = vb->is_cached;

    if(jpu_ioctl_FLUSH_CACHE(&jdb)) {
        JLOG(ERR, "jpu_ioctl_FLUSH_CACHE failed, size:%ld\n", jdb.size);
        return -1;
    }
    return 0;
}

int jdi_set_clock_gate(int enable)
{
    jdi_info_t *jdi = NULL;
    int ret;

    jdi = &s_jdi_info;
    if(!jdi || jdi->jpu_fd==-1 || jdi->jpu_fd == 0x00)
        return -1;

#if defined(CNM_FPGA_PLATFORM)
    io_lock();
#endif /* CNM_FPGA_PLATFORM */
    jdi->clock_state = enable;
    jpu_ioctl_set_clock_gate(&enable);
#if defined(CNM_FPGA_PLATFORM)
    io_unlock();
#endif /* CNM_FPGA_PLATFORM */

    return ret;
}

int jdi_get_clock_gate(void)
{
    jdi_info_t *jdi;
    int ret;

    jdi = &s_jdi_info;

    if(!jdi || jdi->jpu_fd==-1 || jdi->jpu_fd == 0x00)
        return -1;

    ret = jdi->clock_state;

    return ret;
}

int jdi_wait_inst_ctrl_busy(int core_idx, int timeout, unsigned int addr_flag_reg, unsigned int flag)
{
    Int64 elapse, cur;
    unsigned int data_flag_reg;
    struct timespec64 ts;

    ktime_get_ts64(&ts);
    elapse = ts.tv_sec * 1000 + ts.tv_nsec/1000000;

    while(1)
    {
        data_flag_reg = jdi_read_register_ext(core_idx, addr_flag_reg);

        if (((data_flag_reg >> 4)&0xf) == flag) {
            break;
        }

        ktime_get_ts64(&ts);
        cur = ts.tv_sec * 1000 + ts.tv_nsec/1000000;

        if (timeout > 0 && (cur - elapse) > timeout)
        {
            JLOG(ERR, "0x%x=0x%lx\n", addr_flag_reg, jdi_read_register_ext(core_idx, addr_flag_reg));
            return -1;
        }
    }

    return 0;
}

#if defined(SUPPORT_WRESP_DONE_CHECKING) || defined(SUPPORT_RRESP_DONE_CHECKING)
int jdi_wait_bus_busy(int timeout, unsigned long instIdx, unsigned long reg)
{
    Int64 elapse, cur;
    Uint32 respFlag;
    struct timeval tv;
    int retry_count;
    struct timespec64 ts;

    ktime_get_ts64(&ts);
    elapse = ts.tv_sec * 1000 + ts.tv_nsec/1000000;

    while(1)
    {
        respFlag = JpuReadInstReg(instIdx, reg);

        if (respFlag == 0)
            break;

        ktime_get_ts64(&ts);
        cur = ts.tv_sec * 1000 + ts.tv_nsec/1000000;

        if (timeout > 0 && (cur - elapse) > timeout)
        {
            for (retry_count=0; retry_count<10; retry_count++) {
                JLOG(ERR, "[JDI] %s timeout, inst=%d, 0x%x=0x%lx\n", __FUNCTION__, instIdx, MJPEG_WRESP_CHECK_REG, JpuReadInstReg(instIdx, MJPEG_WRESP_CHECK_REG));
            }
            return -1;
        }
    }
    return 0;
}
#endif

static u64 jdi_get_current_time(void)
{
    struct timespec64 ts;


    ktime_get_ts64(&ts);

    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000; // in ms
}


int jdi_wait_interrupt(unsigned int core_idx, int timeout, unsigned long instIdx)
{
    int intr_reason = 0;
    jdi_info_t *jdi;
    u64 currentTs = 0;
    int ret;
    jpudrv_intr_info_t intr_info;

    jdi = &s_jdi_info;

    if(!jdi || jdi->jpu_fd <= 0)
        return -1;

    intr_info.timeout     = timeout;
    intr_info.intr_reason = 0;
    intr_info.inst_idx    = instIdx;
    intr_info.core_idx = core_idx;

    ret = jpu_ioctl_WAIT_INTERRUPT(&intr_info);
    if (ret != 0) {
        JLOG(WARN, "timeout or ctrl+C core:%d\n", core_idx);
        return -1;
    }

    intr_reason = intr_info.intr_reason;
    JLOG(INFO, "jdi_wait_interrupt intr_reason:%d\n", intr_reason);

    if (jpu_show_fps && (intr_reason & (1<<0) || intr_reason & (1<<9)))  {
        currentTs = jdi_get_current_time();
        jdi_core_stat_fps[intr_info.core_idx] += 1;

        if (jdi_core_stat_lastts[intr_info.core_idx] == 0) {
            jdi_core_stat_lastts[intr_info.core_idx] = currentTs;
        }

        if (currentTs >= jdi_core_stat_lastts[intr_info.core_idx] + 1000 ) {
            JLOG(ERR, "jdi core:%d fps:%d  \n"
                , intr_info.core_idx, jdi_core_stat_fps[intr_info.core_idx]);

            jdi_core_stat_lastts[intr_info.core_idx] = currentTs;
            jdi_core_stat_fps[intr_info.core_idx] = 0;
        }
    }

    return intr_reason;
}


void jdi_log(int cmd, int step, int inst)
{
    Int32   i;

    switch(cmd)
    {
    case JDI_LOG_CMD_PICRUN:
        if (step == 1)    //
            JLOG(INFO, "\n**PIC_RUN INST=%d start\n", inst);
        else
            JLOG(INFO, "\n**PIC_RUN INST=%d  end \n", inst);
        break;
    case JDI_LOG_CMD_INIT:
        if (step == 1)    //
            JLOG(INFO, "\n**INIT INST=%d  start\n", inst);
        else
            JLOG(INFO, "\n**INIT INST=%d  end \n", inst);
        break;
    case JDI_LOG_CMD_RESET:
        if (step == 1)    //
            JLOG(INFO, "\n**RESET INST=%d  start\n", inst);
        else
            JLOG(INFO, "\n**RESET INST=%d  end \n", inst);
        break;
    case JDI_LOG_CMD_PAUSE_INST_CTRL:
        if (step == 1)    //
            JLOG(INFO, "\n**PAUSE INST_CTRL  INST=%d start\n", inst);
        else
            JLOG(INFO, "\n**PAUSE INST_CTRL  INST=%d end\n", inst);
        break;
    }

    for (i=(inst*NPT_REG_SIZE); i<=((inst*NPT_REG_SIZE)+0x250); i=i+16)
    {
        JLOG(INFO, "0x%04xh: 0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", i,
            jdi_read_register(i), jdi_read_register(i+4),
            jdi_read_register(i+8), jdi_read_register(i+0xc));
    }

    JLOG(INFO, "0x%04xh: 0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", NPT_PROC_BASE,
        jdi_read_register(NPT_PROC_BASE+0x00), jdi_read_register(NPT_PROC_BASE+4),
        jdi_read_register(NPT_PROC_BASE+8), jdi_read_register(NPT_PROC_BASE+0xc));
}


static void SwapByte(Uint8* data, size_t len)
{
    Uint8   temp;
    size_t  i;

    for (i=0; i<len; i+=2) {
        temp      = data[i];
        data[i]   = data[i+1];
        data[i+1] = temp;
    }
}

static void SwapWord(Uint8* data, size_t len)
{
    Uint16  temp;
    Uint16* ptr = (Uint16*)data;
    size_t  i, size = len/sizeof(Uint16);

    for (i=0; i<size; i+=2) {
        temp      = ptr[i];
        ptr[i]   = ptr[i+1];
        ptr[i+1] = temp;
    }
}

static void SwapDword(Uint8* data, size_t len)
{
    Uint32  temp;
    Uint32* ptr = (Uint32*)data;
    size_t  i, size = len/sizeof(Uint32);

    for (i=0; i<size; i+=2) {
        temp      = ptr[i];
        ptr[i]   = ptr[i+1];
        ptr[i+1] = temp;
    }
}

Int32 swap_endian(BYTE* data, size_t len, Uint32 endian)
{
    Uint8   endianMask[8] = {   // endianMask : [2] - 4byte unit swap
        0x00, 0x07, 0x04, 0x03, //              [1] - 2byte unit swap
        0x06, 0x05, 0x02, 0x01  //              [0] - 1byte unit swap
    };
    Uint8   targetEndian;
    Uint8   systemEndian;
    Uint8   changes;
    BOOL    byteSwap=FALSE, wordSwap=FALSE, dwordSwap=FALSE;

    if (endian > 7) {
        JLOG(ERR, "Invalid endian mode: %d, expected value: 0~7\n", endian);
        return -1;
    }

    targetEndian = endianMask[endian];
    systemEndian = endianMask[JDI_SYSTEM_ENDIAN];
    changes      = targetEndian ^ systemEndian;
    byteSwap     = changes & 0x01 ? TRUE : FALSE;
    wordSwap     = changes & 0x02 ? TRUE : FALSE;
    dwordSwap    = changes & 0x04 ? TRUE : FALSE;

    if (byteSwap == TRUE)  SwapByte(data, len);
    if (wordSwap == TRUE)  SwapWord(data, len);
    if (dwordSwap == TRUE) SwapDword(data, len);

    return changes == 0 ? 0 : 1;
}


#if defined(CNM_FPGA_PLATFORM)

static unsigned long s_dram_base;
static int io_lock(
    void
    )
{
    jdi_info_t* jdi;
    jdi = &s_jdi_info;

    if(!jdi || jdi->jpu_fd <= 0)
        return -1;

    return ioctl(jdi->jpu_fd, JDI_IOCTL_IO_LOCK, NULL);
}

static int io_unlock(
    void
    )
{
    jdi_info_t* jdi;
    int         ret;
    jdi = &s_jdi_info;

    if(!jdi || jdi->jpu_fd==-1 || jdi->jpu_fd == 0x00) {
        return -1;
    }

    ret = ioctl(jdi->jpu_fd, JDI_IOCTL_IO_UNLOCK, NULL);

    return ret;
}

int jdi_set_clock_freg(int Device, int OutFreqMHz, int InFreqMHz )
{
    jdi_info_t *jdi;

    jdi = &s_jdi_info;

    if(!jdi || jdi->jpu_fd==-1 || jdi->jpu_fd == 0x00)
        return -1;

    return 0;
}

#endif

