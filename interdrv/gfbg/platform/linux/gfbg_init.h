#ifndef GFBG_INIT_H
#define GFBG_INIT_H

#include <linux/atomic.h>
#include "gfbg_main.h"
#include "gfbg_disp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

typedef struct {
	unsigned long compre_paddr;	/* Start of compress buffer mem */
	void *compre_vaddr;
	unsigned long compre_size;
	struct oenc_cfg oenc_cfg; /* oenc config */
} gfbg_compre_info;

typedef struct {
	struct fb_info *info;
	unsigned long layer_size;       /* layer_size = fb.smem_len, For display buf, KB */
	gfbg_compre_info compre_info[2];
	int index; /* index for compress buffer */
} gfbg_layer;

struct gfbg_vo_dev {
	int irq;
	int irq_oenc;
	atomic_t irq_requested;
	vo_dev dev_id;
	vo_layer layer_id;
};

extern gfbg_layer g_layer[GFBG_MAX_LAYER_NUM];
extern struct gfbg_vo_dev *gfbg_vdev;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
