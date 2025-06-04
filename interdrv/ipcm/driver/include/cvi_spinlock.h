/* SPDX-License-Identifier: GPL-2.0 */
/**
 * @brief Hardware spinlock implementation
 *
 * Provides hardware-based spinlock mechanisms for synchronization between processors
 */

#ifndef __DRV_SPINLOCK_H__
#define __DRV_SPINLOCK_H__

/**
 * @brief Spinlock field identifiers
 */
enum SPINLOCK_FIELD {
	SPIN_UART,                // UART access lock
	SPIN_LINUX_RTOS = 1,     // Linux & RTOS shared lock
	SPIN_MBOX = SPIN_LINUX_RTOS, // Mailbox access lock
	SPIN_SHM = SPIN_LINUX_RTOS + 1,  // Shared memory access lock
	SPIN_DATA = SPIN_SHM + 1, // Data access lock
	SPIN_MAX = 7,            // Maximum spinlock fields
};

/**
 * @brief Hardware raw spinlock structure
 */
typedef struct hw_raw_spinlock {
	unsigned short locks;    // Lock status
	unsigned short hw_field; // Hardware field identifier
} hw_raw_spinlock_t;

#define MAILBOX_LOCK_SUCCESS 1
#define MAILBOX_LOCK_FAILED  (-1)

#define __CVI_ARCH_SPIN_LOCK_UNLOCKED \
	(0)

#define __CVI_RAW_SPIN_LOCK_INITIALIZER(spinlock_hw_field) \
	{ .locks = __CVI_ARCH_SPIN_LOCK_UNLOCKED, .hw_field = spinlock_hw_field, }

/**
 * @brief Define a spinlock with its hardware field
 */
#define DEFINE_CVI_SPINLOCK(x, y) \
	hw_raw_spinlock_t x = __CVI_RAW_SPIN_LOCK_INITIALIZER(y)

/**
 * @brief Save IRQ state and acquire spinlock
 * @param lock Spinlock to acquire
 * @return IRQ flags
 */
int _hw_raw_spin_lock_irqsave(hw_raw_spinlock_t *lock);

/**
 * @brief Release spinlock and restore IRQ state
 * @param lock Spinlock to release
 * @param flag IRQ flags to restore
 * @return Status code
 */
int _hw_raw_spin_unlock_irqrestore(hw_raw_spinlock_t *lock, int flag);

int _hw_raw_spin_lock_irqsave_ext(hw_raw_spinlock_t *lock);
int _hw_raw_spin_unlock_irqrestore_ext(hw_raw_spinlock_t *lock, int flag);

#define drv_spin_lock_irqsave(lock, flags) \
	{ flags = _hw_raw_spin_lock_irqsave(lock); }

#define drv_spin_unlock_irqrestore(lock, flags) \
	_hw_raw_spin_unlock_irqrestore(lock, flags)

#define drv_spin_lock_irqsave_ext(lock, flags) \
	{ flags = _hw_raw_spin_lock_irqsave_ext(lock); }

#define drv_spin_unlock_irqrestore_ext(lock, flags) \
	_hw_raw_spin_unlock_irqrestore_ext(lock, flags)

void spinlock_base(unsigned long mb_base);
void cvi_spinlock_init(void);
void cvi_spinlock_uninit(void);

#endif  // end of __DRV_SPINLOCK_H__
