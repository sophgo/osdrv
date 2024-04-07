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
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/kthread.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_graph.h>
#include <linux/of_device.h>
#include <linux/reset.h>
#include <linux/of_reserved_mem.h>
#include <linux/streamline_annotate.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/clk.h>

#if LINUX_VERSION_CODE > KERNEL_VERSION(5,4,0)
#include <linux/sched/signal.h>
#endif


#include "../../../jpuapi/jpuconfig.h"
#include "jpu.h"
#include "jpulog.h"

//#define ENABLE_DEBUG_MSG
#ifdef ENABLE_DEBUG_MSG
#define DPRINTK(args...)           printk(KERN_INFO args)
#else
#define DPRINTK(args...)
#endif

static struct file *g_filp;


/* definitions to be changed as customer  configuration */
/* if you want to have clock gating scheme frame by frame */
//#define JPU_SUPPORT_CLOCK_CONTROL
#define JPU_SUPPORT_ISR
/* if the platform driver knows the name of this driver */
/* JPU_PLATFORM_DEVICE_NAME */
#define JPU_SUPPORT_PLATFORM_DRIVER_REGISTER

/* if this driver knows the dedicated video memory address */
//#define JPU_SUPPORT_RESERVED_VIDEO_MEMORY        //if this driver knows the dedicated video memory address

#define JPU_PLATFORM_DEVICE_NAME    "sophgo,jpu"
#define JPU_CLK_NAME                "jpege"
#define JPU_DEV_NAME                "jpu"

#define JPU_REG_BASE_ADDR           0x75300000
#define JPU_REG_SIZE                0x300


#define JPEG_TOP_REG                0x21000010
#define JPEG_TOP_RESET_REG          0x28103000


#ifdef JPU_SUPPORT_ISR
#define JPU_IRQ_NUM                 (15+32)
/* if the driver want to disable and enable IRQ whenever interrupt asserted. */
#define JPU_IRQ_CONTROL
#endif


#ifndef VM_RESERVED	/*for kernel up to 3.7.0 version*/
#define VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#endif

#define JPU_SUPPORT_ION_MEMORY

typedef struct jpu_drv_context_t {
    struct fasync_struct *async_queue;
    u32 open_count;                     /*!<< device reference count. Not instance count */
    u32 interrupt_reason[MAX_NUM_JPU_CORE][MAX_JPEG_NUM_INSTANCE];
} jpu_drv_context_t;


/* To track the allocated memory buffer */
typedef struct jpudrv_buffer_pool_t {
    struct list_head        list;
    struct jpudrv_buffer_t  jb;
    struct file*            filp;
} jpudrv_buffer_pool_t;

/* To track the instance index and buffer in instance pool */
typedef struct jpudrv_instance_list_t {
    struct list_head    list;
    unsigned long       inst_idx;
    struct file*        filp;
} jpudrv_instance_list_t;


typedef struct jpudrv_instance_pool_t {
    unsigned char codecInstPool[MAX_JPEG_NUM_INSTANCE][MAX_INST_HANDLE_SIZE];
} jpudrv_instance_pool_t;


//#define JPU_SUPPORT_RESERVED_VIDEO_MEMORY

#ifdef JPU_SUPPORT_RESERVED_VIDEO_MEMORY
//	#define JPU_INIT_VIDEO_MEMORY_SIZE_IN_BYTE  (16*1024*1024)
//	#define JPU_DRAM_PHYSICAL_BASE              (0x8AA00000)
#define JPU_INIT_VIDEO_MEMORY_SIZE_IN_BYTE  (4*1024*1024*1024)
#define JPU_DRAM_PHYSICAL_BASE              (0x0)
#include "jmm.h"
static jpu_mm_t         s_jmem;
static jpudrv_buffer_t  s_video_memory = {0};
#endif /* JPU_SUPPORT_RESERVED_VIDEO_MEMORY */




static int jpu_hw_reset(int idx);
#ifdef JPU_SUPPORT_CLOCK_CONTROL
struct clk *jpu_clk_get(struct device *dev);
static void jpu_clk_disable(struct clk *clk);
static int jpu_clk_enable(struct clk *clk);
#endif
// end customer definition

static jpudrv_buffer_t s_instance_pool = {0};
static jpu_drv_context_t s_jpu_drv_context;
static int s_jpu_major;
static int s_jpu_open_ref_count;
#ifdef JPU_SUPPORT_ISR
//static int s_jpu_irq = JPU_IRQ_NUM;
static int s_jpu_irq[MAX_NUM_JPU_CORE] = {JPU_IRQ_NUM};
static int jpu_core_irq_count[MAX_NUM_JPU_CORE] = {0};
#endif

struct class *jpu_class;
unsigned long virt_top_addr = 0;


static jpudrv_buffer_t s_jpu_register[MAX_NUM_JPU_CORE] = {0};

typedef struct _jpu_power_ctrl_ {
    struct reset_control *jpu_rst;
    struct clk           *jpu_clk;
    struct clk *jpu_apb_clk;
    struct clk *jpu_axi_clk;
} jpu_power_ctrl;
static jpu_power_ctrl jpu_pwm_ctrl = {0};
static struct device *jpu_dev;

static int s_interrupt_flag[MAX_NUM_JPU_CORE*MAX_JPEG_NUM_INSTANCE];
static wait_queue_head_t s_interrupt_wait_q[MAX_NUM_JPU_CORE*MAX_JPEG_NUM_INSTANCE];


// static spinlock_t s_jpu_lock = __SPIN_LOCK_UNLOCKED(s_jpu_lock);
static DEFINE_MUTEX(s_jpu_lock);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
static DECLARE_MUTEX(s_jpu_sem);
#else
static DEFINE_SEMAPHORE(s_jpu_sem);
#endif




static struct list_head s_jbp_head = LIST_HEAD_INIT(s_jbp_head);
static struct list_head s_inst_list_head = LIST_HEAD_INIT(s_inst_list_head);


#define NPT_BASE                                0x0000
#define NPT_REG_SIZE                            0x300
#define MJPEG_PIC_STATUS_REG(_inst_no)          (NPT_BASE + (_inst_no*NPT_REG_SIZE) + 0x004)

#define ReadJpuRegister(core,addr)           *(volatile unsigned int *)(s_jpu_register[core].virt_addr + addr)
#define WriteJpuRegister(core,addr, val)     *(volatile unsigned int *)(s_jpu_register[core].virt_addr + addr) = (unsigned int)val
#define WriteJpu(addr, val)             *(volatile unsigned int *)(addr) = (unsigned int)val;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
#define IOREMAP(addr, size) ioremap(addr, size)
#else
#define IOREMAP(addr, size) ioremap_nocache(addr, size)
#endif

int jpu_core_request_resource(int timeout);
int jpu_core_release_resource(int id);
int jpu_core_init_resources(unsigned int core_num);
void jpu_core_cleanup_resources(void);
extern int32_t base_ion_free(uint64_t u64PhyAddr);
extern int32_t base_ion_alloc(uint64_t *p_paddr, void **pp_vaddr, uint8_t *buf_name, uint32_t buf_len, bool is_cached);
extern int32_t base_ion_cache_invalidate(uint64_t addr_p, void *addr_v, uint32_t u32Len);
extern int32_t base_ion_cache_flush(uint64_t addr_p, void *addr_v, uint32_t u32Len);

uint32_t jpu_get_extension_address(int core_idx)
{
    uint32_t origin_value = 0;
    int shift = 0;

    if (!virt_top_addr) {
        JLOG(ERR, "[JPUDRV] JPEG_TOP_REG not mapped\n");
        return 0;
    }

    shift = (core_idx + 1) * 4;
    origin_value = readl((void *)virt_top_addr);
    return ((origin_value >> shift) & 0xf);
}

void jpu_set_extension_address(int core_idx, uint32_t addr)
{
    uint32_t origin_value = 0;
    uint32_t bit_mask = 0;
    int shift = 0;

    if (!virt_top_addr) {
        JLOG(ERR, "[JPUDRV] JPEG_TOP_REG not mapped\n");
        return;
    }

    DPRINTK("[JPUDRV] jpu_set_extension_address: core_idx=%d, addr=0x%lx\n", core_idx, addr);

    switch (core_idx) {
        case 0:  // jpu core 0
            bit_mask = 0xffffff0f;
            shift = 4;
            break;
        case 1:  // jpu core 1
            bit_mask = 0xfffff0ff;
            shift = 8;
            break;
        case 2:  // jpu core 2
            bit_mask = 0xffff0fff;
            shift = 12;
            break;
        case 3:  // jpu core 3
            bit_mask = 0xfff0ffff;
            shift = 16;
            break;
        default:
            JLOG(ERR, "[JPUDRV] jpu_set_extension_address failed, invalid core index: %d\n", core_idx);
            return;
    }

    origin_value = readl((void *)virt_top_addr);
    *(volatile unsigned int *)virt_top_addr = ((origin_value & bit_mask) | ((addr & 0xf) << shift));
    return;
}

void jpu_sw_top_reset(int core_idx)
{
    uint32_t origin_value = 0;
    uint32_t bit_mask = 0;
    unsigned long virt_top_reset_addr = 0;

    switch (core_idx) {
        case 0:  // jpu core 0
            bit_mask = 0xffffffff & (~(1 << 18));
            break;
        case 1:  // jpu core 1
            bit_mask = 0xffffffff & (~(1 << 19));
            break;
        case 2:  // jpu core 2
            bit_mask = 0xffffffff & (~(1 << 20));
            break;
        case 3:  // jpu core 3
            bit_mask = 0xffffffff & (~(1 << 21));
            break;
        default:
            JLOG(ERR, "[JPUDRV] jpu_top_reset failed, invalid core index: %d\n", core_idx);
            return;
    }
    DPRINTK("[JPUDRV] jpu_top_reset: bit_mask = 0x%lx\n", bit_mask);

    mutex_lock(&s_jpu_lock);
    virt_top_reset_addr = (unsigned long)ioremap(JPEG_TOP_RESET_REG, 4);
    origin_value = readl((void *)virt_top_reset_addr);
    DPRINTK("[JPUDRV] jpu_top_reset: origin_value = 0x%lx\n", origin_value);
    writel(origin_value & bit_mask, (void *)virt_top_reset_addr);
    udelay(1);
    writel(origin_value, (void *)virt_top_reset_addr);
    iounmap((void *)virt_top_reset_addr);
    mutex_unlock(&s_jpu_lock);

    return;
}

static int jpu_alloc_dma_buffer(jpudrv_buffer_t *jb)
{
    if (!jb)
        return -1;
#ifdef JPU_SUPPORT_RESERVED_VIDEO_MEMORY
    jb->phys_addr = (unsigned long long)jmem_alloc(&s_jmem, jb->size, 0);
    if ((unsigned long)jb->phys_addr  == (unsigned long)-1) {
        JLOG(ERR, "[JPUDRV] Physical memory allocation error size=%lu\n", jb->size);
        return -1;
    }

    jb->base = (unsigned long)(s_video_memory.base + (jb->phys_addr - s_video_memory.phys_addr));
    jb->virt_addr = jb->base;
#elif defined(JPU_SUPPORT_ION_MEMORY)
    if (base_ion_alloc((uint64_t *)&jb->phys_addr, (void **)&jb->virt_addr, "jpeg_ion", jb->size, jb->is_cached) != 0) {
        JLOG(ERR, "[JPUDRV] Physical memory allocation error size=%lu\n", jb->size);
        return -1;
    }
    jb->base = jb->virt_addr;

#else
    jb->base = (unsigned long)dma_alloc_coherent(jpu_dev, PAGE_ALIGN(jb->size), (dma_addr_t *) (&jb->phys_addr), GFP_DMA | GFP_KERNEL);
    if ((void *)(jb->base) == NULL) {
        JLOG(ERR, "[JPUDRV] Physical memory allocation error size=%lu\n", jb->size);
        return -1;
    }
    jb->virt_addr = jb->base;

    // pr_info("mark jpu alloc phys:0x%lx, virt:0x%lx, size:0x%x \n", jb->phys_addr, jb->virt_addr, jb->size);
#endif /* JPU_SUPPORT_RESERVED_VIDEO_MEMORY */
    return 0;
}

static void jpu_free_dma_buffer(jpudrv_buffer_t *jb)
{
    if (!jb) {
        return;
    }
    if (jb->base)
#ifdef JPU_SUPPORT_RESERVED_VIDEO_MEMORY
        jmem_free(&s_jmem, jb->phys_addr, 0);
#elif defined(JPU_SUPPORT_ION_MEMORY)
        base_ion_free(jb->phys_addr);
#else
        dma_free_coherent(jpu_dev, PAGE_ALIGN(jb->size), (void *)jb->base, jb->phys_addr);
#endif /* JPUR_SUPPORT_RESERVED_VIDEO_MEMORY */
}

static int jpu_invalidate_cache(jpudrv_buffer_t *jb)
{
#ifdef JPU_SUPPORT_ION_MEMORY
    if (base_ion_cache_invalidate((uint64_t)jb->phys_addr, (void *)jb->virt_addr, jb->size) != 0) {
        JLOG(ERR, "[JPUDRV] invalidate cache failed, paddr: %lx, vaddr: %lx, size=%lu\n", jb->phys_addr, jb->virt_addr, jb->size);
        return -1;
    }
#endif
    return 0;
}

static int jpu_flush_cache(jpudrv_buffer_t *jb)
{
#ifdef JPU_SUPPORT_ION_MEMORY
    if (base_ion_cache_flush((uint64_t)jb->phys_addr, (void *)jb->virt_addr, jb->size) != 0) {
        JLOG(ERR, "[JPUDRV] flush cache failed, paddr: %lx, vaddr: %lx, size=%lu\n", jb->phys_addr, jb->virt_addr, jb->size);
        return -1;
    }
#endif
    return 0;
}

static int get_max_num_jpu_core(void) {

    return MAX_NUM_JPU_CORE;
}

static irqreturn_t jpu_irq_handler(int irq, void *dev_id)
{
    jpu_drv_context_t*  dev = (jpu_drv_context_t *)dev_id;
    int i;
    u32 flag;
    int core;

    DPRINTK("[JPUDRV][+]%s, irq:%d\n", __func__, irq);

#ifdef JPU_IRQ_CONTROL
    disable_irq_nosync(irq);
#endif
    flag = 0;

    for(core = 0; core < MAX_NUM_JPU_CORE; core++)
    {
        if(s_jpu_irq[core] == irq)
        {
#ifdef JPU_IRQ_CONTROL
            jpu_core_irq_count[core]++;
#endif
            break;
        }
    }

    for (i=0; i< MAX_NUM_REGISTER_SET; i++) {
        flag = ReadJpuRegister(core, MJPEG_PIC_STATUS_REG(i));
        if (flag != 0) {
            break;
        }
    }

    dev->interrupt_reason[core][i] = flag;
    s_interrupt_flag[core*MAX_JPEG_NUM_INSTANCE + i] = 1;
    DPRINTK("[JPUDRV][%d] core:%d INTERRUPT FLAG: %08x, %08x\n", i, core, dev->interrupt_reason[core][i], MJPEG_PIC_STATUS_REG(i));

    if (dev->async_queue)
        kill_fasync(&dev->async_queue, SIGIO, POLL_IN);    // notify the interrupt to userspace

    wake_up(&s_interrupt_wait_q[core * MAX_JPEG_NUM_INSTANCE + i]);

    DPRINTK("[JPUDRV][-]%s flag=0x%x\n", __func__, flag);
    return IRQ_HANDLED;
}

int jpu_ioctl_GET_REGISTER_INFO(jpudrv_buffer_t *arg)
{
    memcpy(arg, &s_jpu_register, sizeof(jpudrv_buffer_t)*get_max_num_jpu_core());
    return 0;
}
EXPORT_SYMBOL(jpu_ioctl_GET_REGISTER_INFO);

int jpu_ioctl_reset(unsigned int core_idx)
{
    jpu_hw_reset(core_idx);
    return 0;
}
EXPORT_SYMBOL(jpu_ioctl_reset);

int jpu_enable_irq(int coreidx)
{
#ifdef JPU_IRQ_CONTROL
    if(jpu_core_irq_count[coreidx] <= 0)
    {
        jpu_core_irq_count[coreidx] = 0;
        return 0;
    }
    enable_irq(s_jpu_irq[coreidx]);
    jpu_core_irq_count[coreidx]--;
#endif
    return 0;
}

int jpu_ioctl_WAIT_INTERRUPT(jpudrv_intr_info_t *arg)
{
    jpudrv_intr_info_t *p_info = (jpudrv_intr_info_t *)arg;
    int ret;
    struct jpu_drv_context_t *dev = (struct jpu_drv_context_t *)g_filp->private_data;
    u32 instance_no;
    u32 core_idx;

    instance_no = p_info->inst_idx;
    core_idx = p_info->core_idx;
    DPRINTK("[JPUDRV] 2 INSTANCE NO: %u, core_idx:%u s_interrupt_flag:%d\n", instance_no, core_idx, s_interrupt_flag[core_idx* MAX_JPEG_NUM_INSTANCE + instance_no]);
    ret = wait_event_timeout(s_interrupt_wait_q[core_idx * MAX_JPEG_NUM_INSTANCE + instance_no], s_interrupt_flag[core_idx* MAX_JPEG_NUM_INSTANCE + instance_no] != 0, msecs_to_jiffies(p_info->timeout));
    if (!ret) {
        DPRINTK("[JPUDRV] INSTANCE NO: %d ETIME\n", instance_no);
        return -ETIME;
    }

    /*
    DPRINTK("[JPUDRV] waked up ok,INSTANCE NO: %u, core_idx:%u\n", instance_no, core_idx);
    if (signal_pending(current)) {
        ret = -ERESTARTSYS;
        JLOG(ERR, "[JPUDRV] CORE:%d INSTANCE NO: %u ERESTARTSYS\n", core_idx, instance_no);
        return ret;
    }
    */

    DPRINTK("[JPUDRV] INST(%u) s_interrupt_flag(%d), reason(0x%08x)\n", instance_no, s_interrupt_flag[core_idx* MAX_JPEG_NUM_INSTANCE + instance_no],
        dev->interrupt_reason[core_idx][instance_no]);
    p_info->intr_reason = dev->interrupt_reason[core_idx][instance_no];
    s_interrupt_flag[core_idx* MAX_JPEG_NUM_INSTANCE + instance_no] = 0;
    dev->interrupt_reason[core_idx][instance_no] = 0;
#if 0
#ifdef JPU_IRQ_CONTROL
    enable_irq(s_jpu_irq[core_idx]);
#endif
#endif
    return 0;
}
EXPORT_SYMBOL(jpu_ioctl_WAIT_INTERRUPT);

int jpu_ioctl_FREE_PHYSICAL_MEMORY(jpudrv_buffer_t *arg)
{
    jpudrv_buffer_pool_t *jbp, *n;
    jpudrv_buffer_t jb;

    DPRINTK("[JPUDRV][+]VDI_IOCTL_FREE_PHYSICALMEMORY\n");
    down(&s_jpu_sem);
    memcpy(&jb, (jpudrv_buffer_t *)arg, sizeof(jpudrv_buffer_t));

    if (jb.base)
        jpu_free_dma_buffer(&jb);

    mutex_lock(&s_jpu_lock);
    list_for_each_entry_safe(jbp, n, &s_jbp_head, list) {
        if (jbp->jb.base == jb.base) {
            list_del(&jbp->list);
            kfree(jbp);
            break;
        }
    }
    mutex_unlock(&s_jpu_lock);

    up(&s_jpu_sem);

    DPRINTK("[JPUDRV][-]VDI_IOCTL_FREE_PHYSICALMEMORY\n");

    return 0;
}
EXPORT_SYMBOL(jpu_ioctl_FREE_PHYSICAL_MEMORY);

int jpu_ioctl_ALLOCATE_PHYSICAL_MEMORY(jpudrv_buffer_t *arg)
{
    jpudrv_buffer_pool_t *jbp;
    int ret;

    DPRINTK("[JPUDRV][+]JDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY\n");
    down(&s_jpu_sem);
    jbp = kzalloc(sizeof(jpudrv_buffer_pool_t), GFP_KERNEL);
    if (!jbp) {
        up(&s_jpu_sem);
        return -ENOMEM;
    }

    memcpy(&(jbp->jb), (jpudrv_buffer_t *)arg, sizeof(jpudrv_buffer_t));

    ret = jpu_alloc_dma_buffer(&(jbp->jb));
    if (ret == -1)
    {
        ret = -ENOMEM;
        kfree(jbp);
        up(&s_jpu_sem);
        return ret;
    }
    memcpy(arg, &(jbp->jb), sizeof(jpudrv_buffer_t));

    jbp->filp = g_filp;
    mutex_lock(&s_jpu_lock);
    list_add(&jbp->list, &s_jbp_head);
    mutex_unlock(&s_jpu_lock);

    up(&s_jpu_sem);

    DPRINTK("[JPUDRV][-]JDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY\n");

    return 0;
}
EXPORT_SYMBOL(jpu_ioctl_ALLOCATE_PHYSICAL_MEMORY);

int jpu_ioctl_INVALIDATE_CACHE(jpudrv_buffer_t *arg)
{
    jpudrv_buffer_t jb;

    memcpy(&jb, (jpudrv_buffer_t *)arg, sizeof(jpudrv_buffer_t));
    return jpu_invalidate_cache(&jb);
}
EXPORT_SYMBOL(jpu_ioctl_INVALIDATE_CACHE);

int jpu_ioctl_FLUSH_CACHE(jpudrv_buffer_t *arg)
{
    jpudrv_buffer_t jb;

    memcpy(&jb, (jpudrv_buffer_t *)arg, sizeof(jpudrv_buffer_t));
    return jpu_flush_cache(&jb);
}
EXPORT_SYMBOL(jpu_ioctl_FLUSH_CACHE);

int jpu_ioctl_close_instance_core_index(unsigned int coreIdx)
{
    return jpu_core_release_resource(coreIdx);
}
EXPORT_SYMBOL(jpu_ioctl_close_instance_core_index);


//int jpu_ioctl_get_instance_core_index(jpudrv_remap_info_t *pinfo);
int jpu_ioctl_get_max_jpu_core(void)
{
    return get_max_num_jpu_core();
}
EXPORT_SYMBOL(jpu_ioctl_get_max_jpu_core);


int jpu_open_device(void)
{
    DPRINTK("[JPUDRV][+] %s\n", __func__);

    mutex_lock(&s_jpu_lock);

    s_jpu_drv_context.open_count++;

    if(!g_filp)
        g_filp = vmalloc(sizeof(struct file));
    g_filp->private_data = (void *)(&s_jpu_drv_context);
    mutex_unlock(&s_jpu_lock);

    DPRINTK("[JPUDRV][-] %s\n", __func__);

    return 1;
}
EXPORT_SYMBOL(jpu_open_device);

int jpu_ioctl_GET_INSTANCE_CORE_INDEX(int timeout)
{
    return jpu_core_request_resource(timeout);
}
EXPORT_SYMBOL(jpu_ioctl_GET_INSTANCE_CORE_INDEX);


int jpu_ioctl_getinstancepool(jpudrv_buffer_t* arg)
{
    int ret;
    DPRINTK("[JPUDRV][+]JDI_IOCTL_GET_INSTANCE_POOL\n");

    down(&s_jpu_sem);
    if (s_instance_pool.base != 0) {
        memcpy(arg, &s_instance_pool, sizeof(jpudrv_buffer_t));
    } else {
        memcpy(&s_instance_pool, (jpudrv_buffer_t *)arg, sizeof(jpudrv_buffer_t));
        if (ret == 0) {
            s_instance_pool.size      = PAGE_ALIGN(s_instance_pool.size);
            s_instance_pool.base      = (unsigned long)vmalloc(s_instance_pool.size);
            s_instance_pool.phys_addr = s_instance_pool.base;

            if (s_instance_pool.base != 0) {
                memset((void *)s_instance_pool.base, 0x0, s_instance_pool.size); /*clearing memory*/
                memcpy(arg, &s_instance_pool, sizeof(jpudrv_buffer_t));
             }else {
                JLOG(ERR, "allocate failed,no memory for jpeg instance pool.\n");
             }
        }
    }
    up(&s_jpu_sem);

    DPRINTK("[JPUDRV][-]JDI_IOCTL_GET_INSTANCE_POOL: %s base: %lx, size: %lu\n",
            (ret==0 ? "OK" : "NG"), s_instance_pool.base, s_instance_pool.size);
    return 0;
}
EXPORT_SYMBOL(jpu_ioctl_getinstancepool);


int jpu_ioctl_open_instance(jpudrv_inst_info_t *instInfo)
{
    jpudrv_inst_info_t inst_info;

    memcpy(&inst_info, (jpudrv_inst_info_t *)instInfo, sizeof(jpudrv_inst_info_t));

    mutex_lock(&s_jpu_lock);
    s_jpu_open_ref_count++; /* flag just for that jpu is in opened or closed */
    inst_info.inst_open_count = s_jpu_open_ref_count;
    mutex_unlock(&s_jpu_lock);

    memcpy(instInfo, &inst_info, sizeof(jpudrv_inst_info_t));

    DPRINTK("[JPUDRV] JDI_IOCTL_OPEN_INSTANCE inst_idx=%d, s_jpu_open_ref_count=%d, inst_open_count=%d\n",
            (int)inst_info.inst_idx, s_jpu_open_ref_count, inst_info.inst_open_count);
    return 0;
}
EXPORT_SYMBOL(jpu_ioctl_open_instance);


int jpu_ioctl_close_instance(jpudrv_inst_info_t *instInfo)
{
    jpudrv_inst_info_t inst_info;

    DPRINTK("[JPUDRV][+]JDI_IOCTL_CLOSE_INSTANCE\n");
    memcpy(&inst_info, instInfo, sizeof(jpudrv_inst_info_t));

    mutex_lock(&s_jpu_lock);
    s_jpu_open_ref_count--; /* flag just for that jpu is in opened or closed */
    inst_info.inst_open_count = s_jpu_open_ref_count;
    mutex_unlock(&s_jpu_lock);

    memcpy(instInfo, &inst_info, sizeof(jpudrv_inst_info_t));
    //jpu_core_release_resource(inst_info.core_idx);

    DPRINTK("[JPUDRV] JDI_IOCTL_CLOSE_INSTANCE inst_idx=%d, s_jpu_open_ref_count=%d, inst_open_count=%d\n",
            (int)inst_info.inst_idx, s_jpu_open_ref_count, inst_info.inst_open_count);
    return 0;
}
EXPORT_SYMBOL(jpu_ioctl_close_instance);


int jpu_ioctl_set_clock_gate(int *enable)
{
    u32 clkgate;

    clkgate = *enable;
#ifdef JPU_SUPPORT_CLOCK_CONTROL
    if (clkgate)
        jpu_clk_enable(s_jpu_clk);
    else
        jpu_clk_disable(s_jpu_clk);
#endif /* JPU_SUPPORT_CLOCK_CONTROL */

    return 0;
}
EXPORT_SYMBOL(jpu_ioctl_set_clock_gate);

int jpu_register_clk(struct platform_device *pdev)
{
    int ret;
    const char *clk_name;

    of_property_read_string_index(pdev->dev.of_node, "clock-names", 0, &clk_name);

    jpu_pwm_ctrl.jpu_apb_clk = devm_clk_get(&pdev->dev, clk_name);
    if (IS_ERR(jpu_pwm_ctrl.jpu_apb_clk)) {
        ret = PTR_ERR(jpu_pwm_ctrl.jpu_apb_clk);
        dev_err(&pdev->dev, "failed to retrieve jpu %s clock",  clk_name);
        return ret;
    }
    printk(KERN_ERR "retrive %s clk succeed.\n", clk_name);
    of_property_read_string_index(pdev->dev.of_node, "clock-names", 1, &clk_name);

    jpu_pwm_ctrl.jpu_axi_clk  = devm_clk_get(&pdev->dev, clk_name);
    if (IS_ERR(jpu_pwm_ctrl.jpu_axi_clk)) {
        ret = PTR_ERR(jpu_pwm_ctrl.jpu_axi_clk);
        dev_err(&pdev->dev, "failed to retrieve jpu %s clock", clk_name);
        return ret;
    }
    printk(KERN_ERR "retrive %s clk succeed.\n", clk_name);
    return 0;
}

#ifdef CONFIG_PM
int jpeg_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
#ifdef JPU_SUPPORT_CLOCK_CONTROL
    jpu_clk_disable(s_jpu_clk);
#endif
    return 0;

}
int jpeg_drv_resume(struct platform_device *pdev)
{
#ifdef JPU_SUPPORT_CLOCK_CONTROL
    jpu_clk_enable(s_jpu_clk);
#endif

    return 0;
}
#endif /* !CONFIG_PM */


int jpeg_platform_init(struct platform_device *pdev)
{
    u32 i;
    int err = 0;
    struct resource *res = NULL;
    unsigned long size;
    u32 originValue = 0;
    u32 highPart = 1;
    struct device_node *target_ion = NULL;
    struct device_node *target_ion_heap_vpp = NULL;
    struct device_node *target_ion_heap_vpp_mem = NULL;
    u32 reg;

    DPRINTK("[JPUDRV] begin jpeg_platform_init\n");

    for (i=0; i<MAX_NUM_JPU_CORE * MAX_JPEG_NUM_INSTANCE; i++) {
        init_waitqueue_head(&s_interrupt_wait_q[i]);
        s_interrupt_flag[i] = 0;
    }

    jpu_core_init_resources(MAX_NUM_JPU_CORE);
    s_instance_pool.base = 0;

    for(i = 0; i < MAX_NUM_JPU_CORE; i++) {

        if (pdev) {
            res = platform_get_resource(pdev, IORESOURCE_MEM, i);
        }
        if (res) {/* if platform driver is implemented */
            s_jpu_register[i].phys_addr = res->start;
            size = resource_size(res);
            s_jpu_register[i].virt_addr = (unsigned long)IOREMAP(res->start, size);
            s_jpu_register[i].size      = size;
            DPRINTK("[JPUDRV] : jpu base address res  physical base addr==0x%lx, virtual base=0x%lx, size=%lx\n", s_jpu_register[i].phys_addr , s_jpu_register[i].virt_addr, size);
        } else {
            s_jpu_register[i].phys_addr = JPU_REG_BASE_ADDR;
            s_jpu_register[i].virt_addr = (unsigned long)IOREMAP(s_jpu_register[i].phys_addr, JPU_REG_SIZE);
            s_jpu_register[i].size      = JPU_REG_SIZE;
            DPRINTK("[JPUDRV] : jpu base address get from defined value physical base addr==0x%lx, virtual base=0x%lx\n", s_jpu_register[i].phys_addr, s_jpu_register[i].virt_addr);
        }
    }

    jpu_dev = &pdev->dev;
    err = dma_set_mask_and_coherent(jpu_dev, DMA_BIT_MASK(64));
    if (err) {
        JLOG(ERR, "dma_set_mask_and_coherent 64 fail\n");
        err = dma_set_mask_and_coherent(jpu_dev, DMA_BIT_MASK(32));
    }
    if (err) {
        JLOG(ERR, "dma_set_mask_and_coherent 32 fail\n");
        goto ERROR_PROVE_DEVICE;
    }

#ifdef JPU_SUPPORT_ISR
#ifdef JPU_SUPPORT_PLATFORM_DRIVER_REGISTER
        for(i = 0; i < MAX_NUM_JPU_CORE; i++) {
            if(pdev)
                res = platform_get_resource(pdev, IORESOURCE_IRQ, i);
            if (res) {/* if platform driver is implemented */
                s_jpu_irq[i] = res->start;
                DPRINTK("[JPUDRV] : jpu irq number get from platform driver irq=0x%x\n", s_jpu_irq[i]);
            } else {
                DPRINTK("[JPUDRV] : jpu irq number get from defined value irq=0x%x\n", s_jpu_irq[i] );
            }
#else
            DPRINTK("[JPUDRV] : jpu irq number get from defined value irq=0x%x\n", s_jpu_irq[i]);
#endif
            err = request_irq(s_jpu_irq[i], jpu_irq_handler, IRQF_TRIGGER_NONE, "JPU_CODEC_IRQ", (void *)(&s_jpu_drv_context));
            if (err) {
                printk(KERN_ERR "[JPUDRV] :  fail to register interrupt handler\n");
                goto ERROR_PROVE_DEVICE;
            }
        }
#endif

#ifdef JPU_SUPPORT_ION_MEMORY
        target_ion = of_find_compatible_node(NULL, NULL, "cvitek,cvitek-ion");
        if (target_ion) {
            target_ion_heap_vpp = of_find_compatible_node(target_ion, NULL, "cvitek,carveout_vpp");
            if (target_ion_heap_vpp) {
                target_ion_heap_vpp_mem = of_find_compatible_node(target_ion_heap_vpp, NULL, "vpp-region");
                if (target_ion_heap_vpp_mem) {
                    if (!of_property_read_u32(target_ion_heap_vpp_mem, "reg", &reg)) {
                        DPRINTK("[JPUDRV] : vpp memory-region reg[0]=%lu\n", reg);
                        highPart = reg & 0xf;
                    } else {
                        DPRINTK("[JPUDRV] : can't acquire memory-region\n");
                    }
                } else {
                    DPRINTK("[JPUDRV] : vpp memory-region not found\n");
                }
            } else {
                DPRINTK("[JPUDRV] : vpp heap not found\n");
            }
        } else {
            DPRINTK("[JPUDRV] : ion node not found\n");
        }
#endif

        virt_top_addr = (unsigned long)ioremap(JPEG_TOP_REG,4);
        originValue = readl((void *)virt_top_addr);
        *(volatile unsigned int *)virt_top_addr = ((originValue & 0xf) | (highPart << 4) | (highPart << 8) | (highPart << 12) | (highPart << 16));

        return 0;
ERROR_PROVE_DEVICE:

    if (s_jpu_major)
        unregister_chrdev_region(s_jpu_major, 1);

    for(i=0; i < MAX_NUM_JPU_CORE; i++) {
        if (s_jpu_register[i].virt_addr)
            iounmap((void *)s_jpu_register[i].virt_addr);
        s_jpu_register[i].virt_addr = 0;
    }


    DPRINTK("[JPUDRV] end jpeg_init result=0x%x\n", err);
    return 0;
}

void jpeg_platform_exit(void)
{
    int i;
    DPRINTK("[JPUDRV] [+]jpeg_exit\n");
#ifdef JPU_SUPPORT_PLATFORM_DRIVER_REGISTER

    if (s_instance_pool.base) {
        vfree((const void *)s_instance_pool.base);
        s_instance_pool.base = 0;
    }
#ifdef JPU_SUPPORT_RESERVED_VIDEO_MEMORY
    if (s_video_memory.base) {
        iounmap((void *)s_video_memory.base);
        s_video_memory.base = 0;
        jmem_exit(&s_jmem);
    }
#endif

#ifdef JPU_SUPPORT_ISR
    for(i = 0; i < MAX_NUM_JPU_CORE; i++) {
        if (s_jpu_irq[i])
            free_irq(s_jpu_irq[i], &s_jpu_drv_context);
    }
#endif

    for(i = 0; i < MAX_NUM_JPU_CORE; i++) {
        if (s_jpu_register[i].virt_addr)
            iounmap((void*)s_jpu_register[i].virt_addr);
        s_jpu_register[i].virt_addr = 0;
    }

#endif /* JPU_SUPPORT_PLATFORM_DRIVER_REGISTER */

    if (virt_top_addr) {
        iounmap((void *)virt_top_addr);
        virt_top_addr = 0;
    }

    DPRINTK("[JPUDRV] [-]jpeg_exit\n");

    return;
}

static int jpu_hw_reset(int idx)
{
    DPRINTK("[JPUDRV] request jpu reset from application. \n");
    return 0;
}

#ifdef JPU_SUPPORT_CLOCK_CONTROL
struct clk *jpu_clk_get(struct device *dev)
{
    return devm_clk_get(dev, JPU_CLK_NAME);
}
void jpu_clk_put(struct clk *clk)
{
   // if (!(clk == NULL || IS_ERR(clk)))
    //    clk_put(clk);
}
int jpu_clk_enable(struct clk *clk)
{
    if (clk) {
        DPRINTK("[JPUDRV] jpu_clk_enable\n");
        // You need to implement it
     //   clk_prepare_enable(clk);
        return 1;
    }
    return 0;
}

void jpu_clk_disable(struct clk *clk)
{
    if (!(clk == NULL || IS_ERR(clk))) {
        DPRINTK("[JPUDRV] jpu_clk_disable\n");
    //    clk_disable_unprepare(clk);
        // You need to implement it
    }
}
#endif



