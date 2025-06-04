
#ifndef __IPCM_PLAT_ADAPTER_H__
#define __IPCM_PLAT_ADAPTER_H__

#include <errno.h>
#include "ipcm_plat_adapter_common.h"

#define RECEIVE_CPU CPU_C906B
#define SEND_TO_CPU CPU_C960L
#define MB_LOCKCNT_SHIRT 0

#define PLATFORM_STRING "linux"

#define IPCMPA_MEMREMAP(addr, size) memremap(addr, size, MEMREMAP_WB)
#define IPCMPA_MEMUNMAP(addr) memunmap(addr)
#define __IPCMPA_IOMEM
#define ipcmpa_phys_addr unsigned long

#define IPCMPA_IRQ_RETURN_TYPE irqreturn_t
#define IPCMPA_IRQ_RETURN_HANDLE return IRQ_HANDLED;
// sem
#define IPCMPA_SEM struct semaphore
#define IPCMPA_SEM_INIT sema_init
#define IPCMPA_SEM_UNINIT(x)
#define IPCMPA_SEM_IS_VALID(x) (x)?1:0
#define IPCMPA_SEM_UP up
#define IPCMPA_SEM_DOWN_TIMEOUT down_timeout
#define IPCMPA_SEM_TIME_MS_CONVERT(x) msecs_to_jiffies(x)
// wait queue
#define IPCMPA_WQ_HEAD wait_queue_head_t
#define IPCMPA_WQ_WAKE_UP wake_up_interruptible

//task
#define IPCMPA_TASK struct task_struct *
#define IPCMPA_TASK_INIT
#define IPCMPA_TASK_UNINIT
#define IPCMPA_TASK_NEW
#define IPCMPA_TASK_STOP kthread_stop
//mutex
#define IPCMPA_MUTEX struct mutex
#define IPCMPA_MUTEX_INIT mutex_init
#define IPCMPA_MUTEX_UNINIT(x) mutex_destroy(x)
#define IPCMPA_MUTEX_LOCK mutex_lock
#define IPCMPA_MUTEX_UNLOCK mutex_unlock
#define IPCMPA_BACKTRACE_NOW(a, b, c) ((void)(a));

#define IPCMPA_SPIN_LOCK_DESTROY(lock)
#define IPCMPA_EXPORT_SYMBOL_GPL(f) EXPORT_SYMBOL_GPL(f)

void __IPCMPA_IOMEM *ipcmpa_ioremap(ipcmpa_phys_addr offset, size_t size);

void ipcmpa_iounmap(void __IPCMPA_IOMEM *addr);

unsigned int ipcmpa_ioread32(const void __IPCMPA_IOMEM *addr);

int ipcmpa_iowrite32(unsigned int b, void __IPCMPA_IOMEM *addr);

unsigned int ipcmpa_readpc(const void __IPCMPA_IOMEM *addr);

#endif
