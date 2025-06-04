/*
 * Copyright (C) 2015-2021 Alibaba Group Holding Limited
 */

#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include <stdio.h>

#ifdef CONFIG_PTHREAD_SPINLOCK_SUPPORT
#include <pthread.h>

// Spinlock structure using pthread spinlock
typedef struct spinlock {
	unsigned int flag;  // Lock initialization flag
	pthread_spinlock_t ps;  // pthread spinlock
} spinlock_t;

// Initialize spinlock to unlocked state
#define __SPIN_LOCK_UNLOCKED(lock) { .flag = 0 }

// Define and initialize a spinlock
#define DEFINE_SPINLOCK(lock) struct spinlock lock = { \
	.flag = 0 \
}

// Initialize spinlock if not already initialized
#define spin_lock_init(lock) do { \
	int ret = -1; \
	if ((lock)->flag == 1) \
		break; \
	ret = pthread_spin_init(&((lock)->ps), PTHREAD_PROCESS_PRIVATE); \
	if (ret) { \
		printf("pthread_spin_init failed, ret:%d\r\n", ret); \
	} \
	(lock)->flag = 1; \
} while (0)

//TODO: how to guarantee this procedure is atomic?
#define spin_lock_irqsave(lock, flags) do { \
	(void)flags; \
	if (!(lock)->flag) { \
		spin_lock_init(lock); \
		(lock)->flag = 1; \
	} \
	pthread_spin_lock(&(lock)->ps); \
} while (0)

// Release spinlock and restore IRQ state
#define spin_unlock_irqrestore(lock, flags) do { \
	(void)flags; \
	pthread_spin_unlock(&(lock)->ps); \
} while (0)

// Acquire spinlock
#define spin_lock(lock) do { \
	pthread_spin_lock(&((lock)->ps)); \
} while (0)

// Release spinlock
#define spin_unlock(lock) do { \
	pthread_spin_unlock(&((lock)->ps)); \
} while (0)

// Acquire spinlock with IRQ disabled
#define spin_lock_irq(lock) \
unsigned long flags; \
do { \
	pthread_spin_lock(&((lock)->ps)); \
} while (0)

// Release spinlock with IRQ enabled
#define spin_unlock_irq(lock) do { \
	pthread_spin_unlock(&((lock)->ps)); \
} while (0)

// Destroy spinlock
#define spin_lock_destroy(lock) do { \
	pthread_spin_destroy(&((lock)->ps)); \
} while (0)

#else // Use AliOS kernel spinlock implementation

#include <aos/kernel.h>
#include <k_api.h>

// Spinlock structure using AliOS kernel spinlock
typedef struct spinlock {
	unsigned int flag;  // Lock initialization flag
	kspinlock_t as;    // AliOS spinlock
} spinlock_t;

// Initialize spinlock to unlocked state
#define __SPIN_LOCK_UNLOCKED(lock) { .flag = 0 }

// Define and initialize a spinlock
#define DEFINE_SPINLOCK(lock) struct spinlock lock = { \
	.flag = 0 \
}

// Initialize spinlock if not already initialized
#define spin_lock_init(lock) do { \
	if ((lock)->flag == 1) \
		break; \
	krhino_spin_lock_init(&((lock)->as)); \
	(lock)->flag = 1; \
} while (0)

// Acquire spinlock and save IRQ state
#define spin_lock_irqsave(lock, flags) do { \
	if (!(lock)->flag) { \
		krhino_spin_lock_init(&((lock)->as)); \
		(lock)->flag = 1; \
	} \
	krhino_spin_lock_irq_save(&(lock)->as, flags); \
} while (0)

// Release spinlock and restore IRQ state
#define spin_unlock_irqrestore(lock, flags) do { \
	(void)flags; \
	krhino_spin_unlock_irq_restore(&(lock)->as, flags); \
} while (0)

// Acquire spinlock
#define spin_lock(lock) do { \
	krhino_spin_lock(&((lock)->as)); \
} while (0)

// Release spinlock
#define spin_unlock(lock) do { \
	krhino_spin_unlock(&((lock)->as)); \
} while (0)

// Acquire spinlock with IRQ disabled
#define spin_lock_irq(lock) \
unsigned long flags; \
do { \
	krhino_spin_lock(&((lock)->as)); \
} while (0)

// Release spinlock with IRQ enabled
#define spin_unlock_irq(lock) do { \
	krhino_spin_unlock(&((lock)->as)); \
} while (0)

// Destroy spinlock
#define spin_lock_destroy(lock) do { \
	(lock)->flag = 0; \
} while (0)
#endif

// Raw spinlock type definition
typedef spinlock_t raw_spinlock_t;

// Initialize raw spinlock to unlocked state
#define __RAW_SPIN_LOCK_UNLOCKED(lock) { \
	.flag = 0 \
}

// Raw spinlock operation macros that map to regular spinlock operations
#define raw_spin_lock_init(lock) spin_lock_init(lock)

#define raw_spin_lock_irqsave(lock, flags) spin_lock_irqsave(lock, flags)
#define raw_spin_unlock_irqrestore(lock, flags)  spin_unlock_irqrestore(lock, flags)

#define raw_spin_lock_irq(lock) spin_lock_irq(lock)
#define raw_spin_unlock_irq(lock)  spin_unlock_irq(lock)


#endif //_SPINLOCK_H_
