#ifndef _LDC_CORE_H_
#define _LDC_CORE_H_

#include "ldc_uapi.h"
#include "ldc_ctx.h"
#include <time.h>
#include "osal.h"

#ifdef DEFAULT_STACK_SIZE
#undef DEFAULT_STACK_SIZE
#endif

#define DEFAULT_STACK_SIZE 4096

struct cvi_ldc_data {
	__u32 bytesperline[VIP_MAX_PLANES];
	__u32 sizeimage[VIP_MAX_PLANES];
	__u16 w;
	__u16 h;
};

struct cvi_ldc_job {
	enum cvi_ldc_op op;
	struct cvi_ldc_data cap_data, out_data;

	__u64 mesh_id_addr;
	__u32 bgcolor;
	struct osal_list_head node;
	struct osal_list_head task_list;
	bool sync_io;
	mod_id_e enModId;
	struct timespec hw_start_time;
};


struct ldc_vdev {
	char dev_name[64];
	struct ldc_ctx ctx;
};

int ldc_open(void);
int ldc_release(void);
// int ldc_isr(int irq, void *data);

#endif /* _LDC_CORE_H_ */
