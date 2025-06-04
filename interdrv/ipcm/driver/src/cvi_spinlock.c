// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_spinlock.c
 * Description:
 */

#include "ipcm_plat_adapter.h"
#include "cvi_spinlock.h"
#include "ipcm_common.h"

static unsigned long reg_base = 0;

// Global spinlocks for register access protection
spinlock_t reg_write_lock;
spinlock_t reg_write_lock_ext;
unsigned long reg_write_flags_ext;

// Per-lock usage counters
static unsigned char lockCount[SPIN_MAX+1] = {0};
static void *__IPCMPA_IOMEM c906l_pc_reg;

/**
 * @brief Initialize spinlock system
 *
 * Sets up global spinlocks and maps C906L program counter register
 * for debugging purposes
 */
void cvi_spinlock_init(void)
{
	spin_lock_init(&reg_write_lock);
	spin_lock_init(&reg_write_lock_ext);
	c906l_pc_reg = ipcmpa_ioremap(0x1901070, 4);
	if (c906l_pc_reg == NULL) {
		ipcm_err("c906l_pc_reg ioremap failed!\n");
	}
	ipcm_debug("[%s] success\n", __func__);
}

void cvi_spinlock_uninit(void)
{
	ipcmpa_iounmap(c906l_pc_reg);
}

void spinlock_base(unsigned long mb_base)
{
	reg_base = mb_base;
}

/**
 * @brief Try to acquire hardware spinlock
 * 
 * Attempts atomic lock acquisition through memory-mapped registers
 *
 * @param lock Lock structure containing lock ID and value
 * @return MAILBOX_LOCK_SUCCESS if lock acquired, MAILBOX_LOCK_FAILED otherwise
 */
static inline int hw_spin_trylock(hw_raw_spinlock_t *lock)
{
	writew(lock->locks, (void *)(reg_base + sizeof(int) * lock->hw_field));
	if (readw((void *)(reg_base + sizeof(int) * lock->hw_field)) == lock->locks)
		return MAILBOX_LOCK_SUCCESS;
	return MAILBOX_LOCK_FAILED;
}

/**
 * @brief Acquire hardware spinlock
 *
 * Repeatedly attempts to acquire lock until successful or timeout.
 * Handles lock counting for Linux-RTOS shared locks.
 *
 * @param lock Lock structure to acquire
 * @return MAILBOX_LOCK_SUCCESS if acquired, MAILBOX_LOCK_FAILED on timeout
 */
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
		_lock.locks = lockCount[lock->hw_field] << MB_LOCKCNT_SHIRT;
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

	ipcm_err("__spin_lock_debug fail! locks:%u field:%u loops = %lld\n", lock->locks, lock->hw_field, loops);
	return MAILBOX_LOCK_FAILED;
}

int _hw_raw_spin_lock_irqsave(hw_raw_spinlock_t *lock)
{
	int flag = MAILBOX_LOCK_SUCCESS;

	// lock
	if (hw_spin_lock(lock) == MAILBOX_LOCK_FAILED) {
		ipcm_err("spin lock fail! C906L pc = 0x%x,reg_val=0x%x, lock->locks=0x%x\n",
			ipcmpa_readpc(c906l_pc_reg), readw((void *)(reg_base + sizeof(int) * lock->hw_field)), lock->locks);
		return MAILBOX_LOCK_FAILED;
	}
	spin_unlock_irqrestore(&reg_write_lock_ext, reg_write_flags_ext);
	return flag;
}

int _hw_raw_spin_unlock_irqrestore(hw_raw_spinlock_t *lock, int flag)
{
	int ret = 0;
	// unlock
	if (readw((void *)(reg_base + sizeof(int) * lock->hw_field)) == lock->locks) {
		writew(lock->locks, (void *)(reg_base + sizeof(int) * lock->hw_field));

	} else {
		ipcm_err("spin unlock fail! C906L pc=0x%x,reg_val=0x%x, lock->locks=0x%x\n",
			ipcmpa_readpc(c906l_pc_reg), readw((void *)(reg_base + sizeof(int) * lock->hw_field)), lock->locks);

		ret = -1;
	}
	return ret;
}

int _hw_raw_spin_lock_irqsave_ext(hw_raw_spinlock_t *lock)
{
	int flag = MAILBOX_LOCK_SUCCESS;

	// lock
	if (hw_spin_lock(lock) == MAILBOX_LOCK_FAILED) {
		ipcm_err("spin lock fail! C906L pc = 0x%x,reg_val=0x%x, lock->locks=0x%x\n",
			ipcmpa_readpc(c906l_pc_reg), readw((void *)(reg_base + sizeof(int) * lock->hw_field)), lock->locks);
		return MAILBOX_LOCK_FAILED;
	}
	return flag;
}

int _hw_raw_spin_unlock_irqrestore_ext(hw_raw_spinlock_t *lock, int flag)
{
	int ret = 0;
	// unlock
	if (readw((void *)(reg_base + sizeof(int) * lock->hw_field)) == lock->locks) {
		writew(lock->locks, (void *)(reg_base + sizeof(int) * lock->hw_field));
		spin_unlock_irqrestore(&reg_write_lock_ext, reg_write_flags_ext);
	} else {
		ipcm_err("spin unlock fail! C906L pc=0x%x,reg_val=0x%x, lock->locks=0x%x\n",
			ipcmpa_readpc(c906l_pc_reg), readw((void *)(reg_base + sizeof(int) * lock->hw_field)), lock->locks);
		ret = -1;
	}
	return ret;
}
