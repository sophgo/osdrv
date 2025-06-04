/**
 * @file msg_queue.c
 * @brief Message queue implementation for IPCM
 *
 * Implements a fixed-size circular queue for message passing between processors.
 * Features include:
 * - Thread-safe operations
 * - Zero-copy message handling
 * - Overflow protection
 * - Queue status tracking
 */

// #define _DEBUG
#include "ipcm_plat_adapter.h"
#include "msg_queue.h"

static struct spinlock lock;

/**
 * @brief Initialize message queue
 *
 * Sets up queue structure:
 * - Validates parameters
 * - Initializes control variables
 * - Sets up synchronization
 *
 * @param queue Queue structure
 * @param size Size of each message
 * @param len Maximum number of messages
 * @return Status code (0 on success, negative on error)
 */
s32 queue_init(MsgQueue *queue, u32 size, u32 len)
{
	if (queue == NULL) {
		ipcm_err("queue is null.\n");
		return -EFAULT;
	}

	queue->data = queue + 1;
	queue->item_size = size;
	queue->len = len;
	queue->front = 0;
	queue->rear = 0;
	queue->cnt = 0;

	spin_lock_init(&lock);

	return 0;
}

/**
 * @brief Clean up message queue
 *
 * Releases queue resources:
 * - Cleans up synchronization
 * - Resets control variables
 *
 * @param queue Queue structure
 * @return Status code (0 on success)
 */
s32 queue_uninit(MsgQueue *queue)
{
	IPCMPA_SPIN_LOCK_DESTROY(&lock);
	return 0;
}


/**
 * @brief Add message to queue
 *
 * Copies message into queue:
 * - Acquires lock
 * - Checks for space
 * - Copies message data
 * - Updates write pointer
 *
 * @param queue Queue structure
 * @param data Message to add
 * @return Status code (0 on success, negative on error)
 */
s32 queue_put(MsgQueue *queue, void *data)
{
	unsigned long flags;

	if (queue == NULL) {
		ipcm_err("queue is null.\n");
		return -EFAULT;
	}

	ipcm_debug("%s addr(%lx), size(%u), len(%u), front(%u), rear(%u), cnt(%u)\n", __func__,
		(unsigned long)queue->data, queue->item_size, queue->len, queue->front, queue->rear, queue->cnt);

	if (data == NULL) {
		ipcm_err("data is null.\n");
		return -EFAULT;
	}
	spin_lock_irqsave(&lock, flags);
	if (queue->cnt == queue->len) {
		spin_unlock_irqrestore(&lock, flags);
		ipcm_err("queue is full.\n");
		return -EQFULL;
	}
	memcpy(queue->data + (queue->item_size * queue->rear), data, queue->item_size);
	queue->rear++;
	queue->rear %= queue->len;
	queue->cnt++;
	spin_unlock_irqrestore(&lock, flags);

	return 0;
}

s32 queue_get(MsgQueue *queue, void* data)
{
	unsigned long flags;
	// void *data;

	if (queue == NULL) {
		ipcm_err("queue is null.\n");
		return -EFAULT;
	}

	if (data == NULL) {
		ipcm_err("data is null.\n");
		return -EFAULT;
	}

	ipcm_debug("queue_get addr(%lx), size(%u), len(%u), front(%u), rear(%u), cnt(%u)\n",
		(unsigned long)queue->data, queue->item_size, queue->len, queue->front, queue->rear, queue->cnt);

	spin_lock_irqsave(&lock, flags);
	if (queue->cnt == 0) {
		spin_unlock_irqrestore(&lock, flags);
		ipcm_err("queue is empty.\n");
		return -EQEMPTY;
	}
	// data = queue->data + (queue->front * queue->item_size);
	memcpy(data, queue->data + (queue->front * queue->item_size), queue->item_size);
	queue->front++;
	queue->front %= queue->len;
	queue->cnt--;
	spin_unlock_irqrestore(&lock, flags);
	return 0;
}

/**
 * @brief Get message from queue without copying
 *
 * Returns pointer to next message:
 * - Acquires lock
 * - Checks for available message
 * - Updates read pointer
 * - Returns direct pointer to message
 *
 * @param queue Queue structure
 * @return Pointer to message or NULL if queue empty
 */
void *queue_get_no_cpy(MsgQueue *queue)
{
	unsigned long flags;
	void *data;

	if (queue == NULL) {
		ipcm_err("queue is null.\n");
		return NULL;
	}

	ipcm_debug("queue_get_no_cpy addr(%lx), size(%u), len(%u), front(%u), rear(%u), cnt(%u)\n",
		(unsigned long)queue->data, queue->item_size, queue->len, queue->front, queue->rear, queue->cnt);

	spin_lock_irqsave(&lock, flags);
	if (queue->cnt == 0) {
		spin_unlock_irqrestore(&lock, flags);
		ipcm_err("queue is empty.\n");
		return NULL;
	}
	data = queue->data + (queue->front * queue->item_size);
	queue->front++;
	queue->front %= queue->len;
	queue->cnt--;
	spin_unlock_irqrestore(&lock, flags);
	return data;
}

/**
 * @brief Check if queue is empty
 * @param queue Queue handle
 * @return 1 if empty, 0 if not empty, negative on error
 */
s8 queue_is_empty(MsgQueue *queue)
{
	if (queue == NULL) {
		ipcm_err("queue is null.\n");
		return -EFAULT;
	}
	return queue->cnt == 0;
}

/**
 * @brief Check if queue is full
 * @param queue Queue handle
 * @return 1 if full, 0 if not full, negative on error
 */
s8 queue_is_full(MsgQueue *queue)
{
	if (queue == NULL) {
		ipcm_err("queue is null.\n");
		return -EFAULT;
	}
	return queue->cnt == queue->len;
}
