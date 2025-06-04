/**
 * @brief Message queue implementation for IPCM
 *
 * Implements a circular buffer queue for message storage and transmission
 * between processors
 */

#ifndef __MSG_QUEUE__
#define __MSG_QUEUE__

#include "ipcm_common.h"

/**
 * @brief Message queue structure
 *
 * Implements a circular buffer for storing messages with fixed-size items
 */
typedef struct _MsgQueue {
	void *data;      // Queue data buffer
	u32 item_size;   // Size of each queue item
	u32 len;         // Maximum queue length
	u32 front;       // Front index (read position)
	u32 rear;        // Rear index (write position)
	u32 cnt;         // Current number of items in queue
} MsgQueue;

/**
 * @brief Initialize message queue
 * @param queue Queue handle
 * @param size Size of each item
 * @param len Maximum queue length
 * @return Status code
 */
s32 queue_init(MsgQueue *queue, u32 size, u32 len);

/**
 * @brief Uninitialize message queue and free resources
 * @param queue Queue handle
 * @return Status code
 */
s32 queue_uninit(MsgQueue *queue);

/**
 * @brief Put an item into the queue
 * @param queue Queue handle
 * @param data Data to enqueue
 * @return Status code (EQFULL if queue is full)
 */
s32 queue_put(MsgQueue *queue, void *data);

/**
 * @brief Get an item from the queue
 * @param queue Queue handle
 * @param data Buffer to store dequeued data
 * @return Status code (EQEMPTY if queue is empty)
 */
s32 queue_get(MsgQueue *queue, void* data);

/**
 * @brief Get item pointer without copying data
 * @param queue Queue handle
 * @return Pointer to item data or NULL if queue is empty
 */
void *queue_get_no_cpy(MsgQueue *queue);

/**
 * @brief Check if queue is empty
 * @param queue Queue handle
 * @return 1 if empty, 0 if not empty
 */
s8 queue_is_empty(MsgQueue *queue);

/**
 * @brief Check if queue is full
 * @param queue Queue handle
 * @return 1 if full, 0 if not full
 */
s8 queue_is_full(MsgQueue *queue);

#endif
