/**
 * @brief Ring buffer implementation for IPCM
 *
 * Provides a simple ring buffer for storing debug/trace information.
 * Unlike queue, ring buffer overwrites oldest data when full.
 */

#ifndef __RING__
#define __RING__

#include "ipcm_common.h"

/**
 * @brief Ring buffer structure
 */
typedef struct _IPCMRing {
    void *data;      // Ring buffer data
    u32 item_size;   // Size of each item
    u32 len;         // Total buffer length
    u32 rear;        // Current write position
} IPCMRing;

/**
 * @brief Initialize ring buffer
 * @param ring Ring buffer handle
 * @param size Size of each item
 * @param len Total buffer length
 * @return Status code
 */
s32 ring_init(IPCMRing *ring, u32 size, u32 len);

s32 ring_uninit(IPCMRing *ring);

s32 ring_put(IPCMRing *ring, void *data);

s32 ring_snap(IPCMRing *ring, void **data, u32 *rear);

#endif
