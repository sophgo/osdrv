#ifndef __TDE_CORE_H__
#define __TDE_CORE_H__

#include "osal.h"
#include "comm_tde.h"


#ifndef FPGA_PORTING
#define TDE_JOB_WAIT_TIMEOUT_MS  1000
#define TDE_IDLE_WAIT_TIMEOUT_MS  1000
#define TDE_INTR_WAIT_TIMEOUT_MS  1000
#else
#define TDE_JOB_WAIT_TIMEOUT_MS  6000
#define TDE_IDLE_WAIT_TIMEOUT_MS  6000
#define TDE_INTR_WAIT_TIMEOUT_MS  5000
#endif

#define TDE_JOB_HASH_SIZE (8)

#define TDE_SUPPORT_FMT(fmt) (fmt == PIXEL_FORMAT_ARGB_8888)

enum tde_task_mode_e {
	TDE_TASK_ROTATE_90, /* ratate 90 */
	TDE_TASK_ROTATE_270, /* ratate 270 */
	TDE_TASK_DRAW_LINE,
	TDE_TASK_COPY,
	TDE_TASK_MAX
};

struct tde_task {
	enum tde_task_mode_e mode;
	tde_surface_s src;
	tde_surface_s dst;
	tde_line_s line;
	struct osal_list_head item;
};

struct tde_job {
	tde_handle hdl;
	osal_atomic task_cnt;
	osal_mutex lock;
	osal_wait wait; //for block
	bool is_sync;
	bool is_block;
	struct osal_list_head task_list;
	struct osal_list_head item;
};

struct tde_core {
	osal_atomic cur_hdl;
	osal_atomic job_num;
	osal_hash *job_hash;
	osal_semaphore sem_job; //for job
	osal_semaphore sem_intr; //for isr
	osal_spinlock lock;
	osal_clk *clk;
	osal_clk *isp_top_clk;
	osal_task *thread;
	unsigned int stop_flag;
	struct osal_list_head job_list;
};

int tde_core_init(struct tde_core *core);
int tde_core_deinit(struct tde_core *core);

int tde_core_open(struct tde_core *core);
int tde_core_release(struct tde_core *core);

int tde_core_suspend(struct tde_core *core);
int tde_core_resume(struct tde_core *core);

int tde_core_isr(struct tde_core *core);

int tde_reg_cb(struct tde_core *core);
int tde_rm_cb(void);

#endif
