// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_spinlock.c
 * Description:
 */

#ifdef __ALIOS__
#include "vip_spinlock.h"
// #include "arch_time.h"
// #include "core_rv64.h"
// #include "csi_rv64_gcc.h"
// #include "csr.h"
#include "delay.h"
#include "mmio.h"
#include "stdint.h"
#include "top_reg.h"
// #include "types.h"
#include "mailbox.h"
#endif

#ifdef __LINUX_DRV__
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/slab.h>
// #include <asm-generic/io.h>
#include <asm/io.h>
#endif

#include "cvi_spinlock.h"
#include "ipcm_common.h"

#ifdef __ALIOS__
static unsigned long reg_base = SPINLOCK_REG_BASE;
extern void udelay(uint32_t us);
#else
static unsigned long reg_base;
#endif

spinlock_t reg_write_lock;
spinlock_t reg_write_lock_ext;
unsigned long reg_write_flags_ext;

static unsigned char lockCount[SPIN_MAX+1] = {0};
#ifdef __LINUX_DRV__
static void *__iomem c906l_pc_reg;
#endif

void cvi_spinlock_init(void)
{
	spin_lock_init(&reg_write_lock);
	spin_lock_init(&reg_write_lock_ext);
	#ifdef __LINUX_DRV__
	c906l_pc_reg = ioremap(0x1901070, 4);
	if (c906l_pc_reg == NULL) {
		ipcm_err("c906l_pc_reg ioremap failed!\n");
	}
	#endif
	ipcm_debug("[%s] success\n", __func__);
}

void cvi_spinlock_uninit(void)
{
	#ifdef __LINUX_DRV__
	iounmap(c906l_pc_reg);
	#endif
}

void spinlock_base(unsigned long mb_base)
{
	reg_base = mb_base;
}

static inline int hw_spin_trylock(hw_raw_spinlock_t *lock)
{
#ifndef RISCV_QEMU
	writew(lock->locks, (void *)(reg_base + sizeof(int) * lock->hw_field));
	if (readw((void *)(reg_base + sizeof(int) * lock->hw_field)) == lock->locks)
		return MAILBOX_LOCK_SUCCESS;
	return MAILBOX_LOCK_FAILED;
#else
	return MAILBOX_LOCK_SUCCESS;
#endif
}

int hw_spin_lock(hw_raw_spinlock_t *lock)
{
	u64 i;
	u64 loops = 1000000;
	hw_raw_spinlock_t _lock;

	_lock.hw_field = lock->hw_field;
	_lock.locks = lock->locks;
	if (lock->hw_field >= SPIN_LINUX_RTOS) {
		unsigned long flags;

		spin_lock_irqsave(&reg_write_lock, flags);
		if (lockCount[lock->hw_field] == 0) {
			lockCount[lock->hw_field]++;
		}
	#ifdef __ALIOS__
		_lock.locks = lockCount[lock->hw_field] << 8;
	#else
		_lock.locks = lockCount[lock->hw_field];
	#endif
		lockCount[lock->hw_field]++;
		spin_unlock_irqrestore(&reg_write_lock, flags);
	} else {
		//....
	}
	for (i = 0; i < loops; i++) {
		spin_lock_irqsave(&reg_write_lock_ext, reg_write_flags_ext);
		if (hw_spin_trylock(&_lock) == MAILBOX_LOCK_SUCCESS) {
			lock->locks = _lock.locks;
			return MAILBOX_LOCK_SUCCESS;
		}
		spin_unlock_irqrestore(&reg_write_lock_ext, reg_write_flags_ext);
		udelay(1);
	}

#ifndef __LINUX_DRV__
	ipcm_err("__spin_lock_debug fail locks:%u field:%u\n", lock->locks, lock->hw_field);
#else
	ipcm_err("__spin_lock_debug fail! locks:%u field:%u loops = %lld\n", lock->locks, lock->hw_field, loops);
#endif
	return MAILBOX_LOCK_FAILED;
}

int _hw_raw_spin_lock_irqsave(hw_raw_spinlock_t *lock)
{
	int flag = MAILBOX_LOCK_SUCCESS;

	// lock
	if (hw_spin_lock(lock) == MAILBOX_LOCK_FAILED) {
	#ifndef __LINUX_DRV__
		ipcm_err("spin lock fail! reg_val=0x%x, lock->locks=0x%x\n",
				readw((void *)(reg_base + sizeof(int) * lock->hw_field)), lock->locks);
	#else
		ipcm_err("spin lock fail! C906L pc = 0x%x,reg_val=0x%x, lock->locks=0x%x\n",
			ioread32(c906l_pc_reg), readw((void *)(reg_base + sizeof(int) * lock->hw_field)), lock->locks);
	#endif
		return MAILBOX_LOCK_FAILED;
	}
	spin_unlock_irqrestore(&reg_write_lock_ext, reg_write_flags_ext);
	return flag;
}

int _hw_raw_spin_unlock_irqrestore(hw_raw_spinlock_t *lock, int flag)
{
	int ret = 0;
#ifndef RISCV_QEMU
	// unlock
	if (readw((void *)(reg_base + sizeof(int) * lock->hw_field)) == lock->locks) {
		writew(lock->locks, (void *)(reg_base + sizeof(int) * lock->hw_field));

	} else {
#ifndef __LINUX_DRV__
		ipcm_err("spin unlock fail\n");
#else
		ipcm_err("spin unlock fail! C906L pc=0x%x,reg_val=0x%x, lock->locks=0x%x\n",
			ioread32(c906l_pc_reg), readw((void *)(reg_base + sizeof(int) * lock->hw_field)), lock->locks);
#endif
		ret = -1;
	}
#else
	if (flag) {
		__enable_irq();
	}
#endif
	return ret;
}

int _hw_raw_spin_lock_irqsave_ext(hw_raw_spinlock_t *lock)
{
	int flag = MAILBOX_LOCK_SUCCESS;

	// lock
	if (hw_spin_lock(lock) == MAILBOX_LOCK_FAILED) {
	#ifndef __LINUX_DRV__
		ipcm_err("spin lock fail! reg_val=0x%x, lock->locks=0x%x\n",
				readw((void *)(reg_base + sizeof(int) * lock->hw_field)), lock->locks);
	#else
		ipcm_err("spin lock fail! C906L pc = 0x%x,reg_val=0x%x, lock->locks=0x%x\n",
			ioread32(c906l_pc_reg), readw((void *)(reg_base + sizeof(int) * lock->hw_field)), lock->locks);
	#endif
		return MAILBOX_LOCK_FAILED;
	}
	return flag;
}

int _hw_raw_spin_unlock_irqrestore_ext(hw_raw_spinlock_t *lock, int flag)
{
	int ret = 0;
#ifndef RISCV_QEMU
	// unlock
	if (readw((void *)(reg_base + sizeof(int) * lock->hw_field)) == lock->locks) {
		writew(lock->locks, (void *)(reg_base + sizeof(int) * lock->hw_field));
		spin_unlock_irqrestore(&reg_write_lock_ext, reg_write_flags_ext);
	} else {
#ifndef __LINUX_DRV__
		ipcm_err("spin unlock fail\n");
#else
		ipcm_err("spin unlock fail! C906L pc=0x%x,reg_val=0x%x, lock->locks=0x%x\n",
			ioread32(c906l_pc_reg), readw((void *)(reg_base + sizeof(int) * lock->hw_field)), lock->locks);
#endif
		ret = -1;
	}
#else
	if (flag) {
		__enable_irq();
	}
#endif
	return ret;
}
