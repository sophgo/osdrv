#ifndef _LDC_COMM_LAYER_H_
#define _LDC_COMM_LAYER_H_

#include "ldc_ctx.h"

//dev core init
void ldc_core_init(int top_id);
void ldc_core_deinit(int top_id);
void ldc_dev_init(struct ldc_ctx *dev);
void ldc_dev_deinit(struct ldc_ctx *dev);
void ldc_clk_init(struct ldc_ctx *dev);
void ldc_clk_deinit(struct ldc_ctx *dev);

//proc update
struct ldc_proc_ctx *ldc_get_proc_ctx(void);
struct ldc_proc_ctx **ldc_get_proc_ctx_addr(void);
void ldc_proc_record_hw_tsk_start(struct ldc_job *job, struct ldc_task *tsk, unsigned char top_id);
void ldc_proc_record_hw_tsk_done(struct ldc_job *job, struct ldc_task *tsk);
void ldc_proc_record_job_start(struct ldc_job *job);
void ldc_proc_record_job_done(struct ldc_job *job);
void ldc_proc_commit_job(struct ldc_job *job);
void ldc_proc_update_timer_proc(bool suspend);

#endif
