
// #define _DEBUG

#ifdef __LINUX_DRV__
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/slab.h>
// #include <asm-generic/io.h>
#include <asm/io.h>
#endif

#ifdef __ALIOS__
#include <errno.h>
#include "vip_spinlock.h"
#endif
#include "msg_queue.h"

static struct spinlock lock;

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

s32 queue_uninit(MsgQueue *queue)
{
#ifdef __ALIOS__
	spin_lock_destroy(&lock);
#endif
	return 0;
}

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

s8 queue_is_empty(MsgQueue *queue)
{
	if (queue == NULL) {
		ipcm_err("queue is null.\n");
		return -EFAULT;
	}
	return queue->cnt == 0;
}

s8 queue_is_full(MsgQueue *queue)
{
	if (queue == NULL) {
		ipcm_err("queue is null.\n");
		return -EFAULT;
	}
	return queue->cnt == queue->len;
}
