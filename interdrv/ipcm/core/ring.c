
#ifdef __LINUX_DRV__
#include <asm/io.h>
#endif

#ifdef __ALIOS__
#include <errno.h>
#include "vip_spinlock.h"
#endif

#include "ring.h"

static struct spinlock ring_lock;

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

s32 ring_uninit(IPCMRing *ring)
{
#ifdef __ALIOS__
	spin_lock_destroy(&ring_lock);
#endif
	return 0;
}

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
