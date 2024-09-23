
#ifndef __RING__
#define __RING__

#include "ipcm_common.h"

typedef struct _IPCMRing {
    void *data;
    u32 item_size;
    u32 len;
    u32 rear;
} IPCMRing;

s32 ring_init(IPCMRing *ring, u32 size, u32 len);

s32 ring_uninit(IPCMRing *ring);

s32 ring_put(IPCMRing *ring, void *data);

s32 ring_snap(IPCMRing *ring, void **data, u32 *rear);

#endif
