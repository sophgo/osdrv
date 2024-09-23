
#ifndef __MSG_QUEUE__
#define __MSG_QUEUE__

#include "ipcm_common.h"

typedef struct _MsgQueue {
	void *data;
	u32 item_size;
	u32 len;
	u32 front;
	u32 rear;
	u32 cnt;
} MsgQueue;

s32 queue_init(MsgQueue *queue, u32 size, u32 len);

s32 queue_uninit(MsgQueue *queue);

s32 queue_put(MsgQueue *queue, void *data);

s32 queue_get(MsgQueue *queue, void* data);

void *queue_get_no_cpy(MsgQueue *queue);

s8 queue_is_empty(MsgQueue *queue);

s8 queue_is_full(MsgQueue *queue);

#endif
