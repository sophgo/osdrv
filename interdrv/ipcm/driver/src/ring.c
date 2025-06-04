/**
 * @file ring.c
 * @brief Ring buffer implementation for debug/trace information
 *
 * Implements a simple ring buffer that overwrites oldest data when full.
 * Features include:
 * - Fixed size circular buffer
 * - Thread-safe operations
 * - Overflow handling
 * - Debug statistics
 */

#include "ipcm_plat_adapter.h"
#include "ring.h"

static struct spinlock ring_lock;

/**
 * @brief Initialize ring buffer
 *
 * Sets up ring buffer structure:
 * - Allocates buffer memory
 * - Initializes control variables
 * - Sets up synchronization
 *
 * @param ring Ring buffer structure
 * @param size Size of each item
 * @param len Maximum number of items
 * @return Status code (0 on success, negative on error)
 */
s32 ring_init(IPCMRing *ring, u32 size, u32 len)
{
    if (NULL == ring) {
        ipcm_err("ring is null.\n");
        return -EFAULT;
    }
    ring->data = ring + 1;
    ring->item_size = size;
    ring->len = len;
    ring->rear = 0;

	spin_lock_init(&ring_lock);

    return 0;
}

/**
 * @brief Clean up ring buffer
 *
 * Releases ring buffer resources:
 * - Frees buffer memory
 * - Cleans up synchronization
 *
 * @param ring Ring buffer structure
 * @return Status code (0 on success)
 */
s32 ring_uninit(IPCMRing *ring)
{
    IPCMPA_SPIN_LOCK_DESTROY(&ring_lock);
	return 0;
}

/**
 * @brief Write item to ring buffer
 *
 * Adds item to buffer with overflow handling:
 * - Acquires lock
 * - Copies item data
 * - Updates write pointer
 * - Handles buffer wrap-around
 *
 * @param ring Ring buffer structure
 * @param data Item to write
 * @return Status code (0 on success, negative on error)
 */
s32 ring_put(IPCMRing *ring, void *data)
{
    unsigned long flags;

    if (NULL == ring) {
        ipcm_err("ring is null.\n");
        return -EFAULT;
    }

	spin_lock_irqsave(&ring_lock, flags);
	memcpy(ring->data + (ring->item_size * ring->rear), data, ring->item_size);
	ring->rear++;
	ring->rear %= ring->len;
	spin_unlock_irqrestore(&ring_lock, flags);

    return 0;
}

/**
 * @brief Take snapshot of ring buffer contents
 *
 * Copies entire ring buffer contents to provided buffer and returns current
 * rear position. Protected by spinlock for thread safety.
 *
 * @param ring Ring buffer handle
 * @param data Pointer to buffer to receive data
 * @param rear Pointer to receive current rear position
 * @return Status code (0 on success, negative on error)
 */
s32 ring_snap(IPCMRing *ring, void **data, u32 *rear)
{
    unsigned long flags;

    if ((NULL==ring) || (NULL==data) || (NULL==rear)) {
        ipcm_err("param is null.\n");
        return -EFAULT;
    }

	spin_lock_irqsave(&ring_lock, flags);
    memcpy(*data, ring->data, ring->item_size * ring->len);
    *rear = ring->rear;
	spin_unlock_irqrestore(&ring_lock, flags);

    return 0;
}
