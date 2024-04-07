//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
//
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
//
// The entire notice above must be reproduced on all authorized copies.
//
// Description  :
//-----------------------------------------------------------------------------
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "jpu_helper.h"

Queue* Queue_Create(
    Uint32      itemCount,
    Uint32      itemSize
    )
{
    Queue* queue = NULL;
    Uint32 size  = itemCount*itemSize;

    if ((queue=(Queue *)vmalloc(sizeof(Queue))) == NULL)
        return NULL;
    queue->size   = itemCount;
    queue->itemSize = itemSize;
    queue->count  = 0;
    queue->front  = 0;
    queue->rear   = 0;
    queue->buffer = (Uint8*)vmalloc(size);
    queue->lock = NULL;

    return queue;
}

Queue* Queue_Create_With_Lock(
    Uint32      itemCount,
    Uint32      itemSize
    )
{
    Queue* queue = NULL;
    Uint32 size  = itemCount*itemSize;

    if ((queue=(Queue *)vmalloc(sizeof(Queue))) == NULL)
        return NULL;
    queue->size   = itemCount;
    queue->itemSize = itemSize;
    queue->count  = 0;
    queue->front  = 0;
    queue->rear   = 0;
    queue->buffer = (Uint8*)vmalloc(size);
    queue->lock   = JpuMutex_Create();

    return queue;
}

void Queue_Destroy(
    Queue*      queue
    )
{
    if (queue == NULL)
        return;

    if (queue->buffer)
        vfree(queue->buffer);
    if (queue->lock)
        JpuMutex_Destroy(queue->lock);
    vfree(queue);
}

BOOL Queue_Enqueue(
    Queue*      queue,
    void*       data
    )
{
    Uint8*      ptr;
    Uint32      offset;

    if (queue == NULL) return FALSE;

    /* Queue is full */
    if (queue->count == queue->size)
        return -1;

    if (queue->lock)
        JpuMutex_Lock(queue->lock);
    offset = queue->rear * queue->itemSize;

    ptr = &queue->buffer[offset];
    memcpy(ptr, data, queue->itemSize);
    queue->rear++;
    queue->rear %= queue->size;
    queue->count++;
    if (queue->lock)
        JpuMutex_Unlock(queue->lock);

    return TRUE;
}

void* Queue_Dequeue(
    Queue*      queue
    )
{
    void* data;
    Uint32   offset;

    if (queue == NULL)
        return NULL;
    /* Queue is empty */
    if (queue->count == 0)
        return NULL;
    if (queue->lock)
        JpuMutex_Lock(queue->lock);
    offset = queue->front * queue->itemSize;
    data   = (void*)&queue->buffer[offset];
    queue->front++;
    queue->front %= queue->size;
    queue->count--;
    if (queue->lock)
        JpuMutex_Unlock(queue->lock);
    return data;
}

void Queue_Flush(
    Queue*      queue
    )
{
    if (queue == NULL)
        return;
    if (queue->lock)
        JpuMutex_Lock(queue->lock);
    queue->count = 0;
    queue->front = 0;
    queue->rear  = 0;
    if (queue->lock)
        JpuMutex_Unlock(queue->lock);
    return;
}

void* Queue_Peek(
    Queue*      queue
    )
{
    Uint32      offset;
    void*       temp;

    if (queue == NULL)
        return NULL;
    /* Queue is empty */
    if (queue->count == 0)
        return NULL;
    if (queue->lock)
        JpuMutex_Lock(queue->lock);
    offset = queue->front * queue->itemSize;
    temp = (void*)&queue->buffer[offset];
    if (queue->lock)
        JpuMutex_Unlock(queue->lock);
    return  temp;
}

Uint32   Queue_Get_Cnt(
    Queue*      queue
    )
{
    Uint32      cnt;

    if (queue == NULL)
        return 0;
    if (queue->lock)
        JpuMutex_Lock(queue->lock);
    cnt = queue->count;
    if (queue->lock)
        JpuMutex_Unlock(queue->lock);
    return cnt;
}

Queue* Queue_Copy(
    Queue*  dstQ,
    Queue*  srcQ
    )
{
    Queue*   queue = NULL;
    Uint32   bufferSize;

    if (dstQ == NULL) {
        if ((queue=(Queue *)vmalloc(sizeof(Queue))) == NULL)
            return NULL;
        memset((void*)queue, 0x00, sizeof(Queue));
    }
    else {
        queue = dstQ;
    }

    bufferSize      = srcQ->size * srcQ->itemSize;
    queue->size     = srcQ->size;
    queue->itemSize = srcQ->itemSize;
    queue->count    = srcQ->count;
    queue->front    = srcQ->front;
    queue->rear     = srcQ->rear;
    if (queue->buffer) {
        vfree(queue->buffer);
    }
    queue->buffer   = (Uint8*)vmalloc(bufferSize);

    memcpy(queue->buffer, srcQ->buffer, bufferSize);

    return queue;
}

