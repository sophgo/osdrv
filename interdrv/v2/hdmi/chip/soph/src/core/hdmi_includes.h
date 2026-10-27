#ifndef __HDMI_INCLUDES_H__
#define __HDMI_INCLUDES_H__

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/of_irq.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/fb.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <asm/io.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/atomic.h>
#include <linux/comm_hdmi.h>

/* Audio frame queue node structure */
struct hdmi_audio_frame_node {
	hdmi_audio_frame frame;  /* Audio frame data */
	struct list_head list;          /* List linkage */
};

struct hdmitx_dev{
	/** Device node */
	struct device *parent_dev;
	/** Device list */
	struct list_head devlist;

	struct class *hdmi_class;
	struct cdev cdev;
	dev_t cdev_id;
	/** Verbose */
	int verbose;
	int msi_enabled;
	/** Interrupts */
	u32 irq[6];

	/** Spinlock */
	spinlock_t slock;
	/** Mutex */
	struct mutex mutex;

	/** Device Tree Information */
	char *device_name;
	/** HDMI TX Controller */
	void __iomem *dwc_hdmi_tx_io;
	void __iomem *base_address;
	u32 address_size;

	/** EDID block of data */
	u8 edid_data[128];

	int hpd;
	int hdcp22;

	spinlock_t lock;

	struct task_struct *thread;

	u8 hpd_flag;
	/* Accumulates interrupt causes until hpd_work runs. */
	atomic_t irq_pending;
	struct delayed_work hpd_work;

	/* Audio frame queue management */
	struct list_head audio_frame_queue;    /* Queue of pending audio frames */
	spinlock_t audio_lock;                  /* Protect queue access */
	struct work_struct audio_work;          /* Work queue for DMA processing */
	bool audio_dma_busy;                    /* DMA busy flag */
	struct hdmi_audio_frame_node *current_frame;  /* Currently playing frame */
	struct hdmi_audio_frame_node *frame_done;    /* Frame just finished; VB release + kfree in work */
};

/**
 * @brief Dynamic memory allocation
 * Instance 0 will have the total size allocated so far and also the number
 * of calls to this function (number of allocated instances)
 */
struct mem_alloc{
	int instance;
	const char *info;
	size_t size;
	void *pointer; // the pointer allocated by this instance
	struct mem_alloc *last;
	struct mem_alloc *prev;
};


#endif /* __HDMI_INCLUDES_H__ */
