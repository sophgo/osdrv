#include "tde_debug.h"
#include "tde_core.h"
#include "tde_ip_ctrl.h"
#include "base_ctx.h"

static void _tde_remove_task(struct tde_job *job)
{
	struct tde_task *task;

	osal_mutex_lock(&job->lock);
	while (!osal_list_empty(&job->task_list)) {
		task = osal_list_first_entry(&job->task_list,
			struct tde_task, item);
		osal_list_del_init(&task->item);
		osal_vfree(task);
	}
	osal_mutex_unlock(&job->lock);
}

static void _tde_task_run(struct tde_task *task)
{
	struct tde_buffer src_buffer, dst_buffer;
	struct tde_line_attr attr;

	src_buffer.addr = task->src.phy_addr;
	src_buffer.stride = task->src.stride;
	src_buffer.width = task->src.width;
	src_buffer.height = task->src.height;

	dst_buffer.addr = task->dst.phy_addr;
	dst_buffer.stride = task->dst.stride;
	dst_buffer.width = task->dst.width;
	dst_buffer.height = task->dst.height;

	if (task->mode == TDE_TASK_ROTATE_90) {
		tde_run_rotate90(&src_buffer, &dst_buffer);
	} else if (task->mode == TDE_TASK_ROTATE_270) {
		tde_run_rotate270(&src_buffer, &dst_buffer);
	} else if (task->mode == TDE_TASK_DRAW_LINE) {
		attr.start_x = task->line.start_x;
		attr.start_y = task->line.start_y;
		attr.end_x = task->line.end_x;
		attr.end_y = task->line.end_y;
		attr.color = task->line.color;
		attr.thick = task->line.thick;
		tde_run_draw_line(&attr, &src_buffer, &dst_buffer);
	} else if (task->mode == TDE_TASK_COPY) {
		tde_run_copy(&src_buffer, &dst_buffer);
	}
}

static void _tde_job_process(struct tde_job *job, osal_semaphore *sem)
{
	int ret;
	struct tde_task *task;
	unsigned long timeout = osal_msecs_to_jiffies(TDE_INTR_WAIT_TIMEOUT_MS);

	TRACE_TDE(DBG_INFO, "working job, handle(%d).\n", job->hdl);

	while (osal_atomic_read(&job->task_cnt)) {
		osal_mutex_lock(&job->lock);
		task = osal_list_first_entry(&job->task_list,
			struct tde_task, item);
		osal_list_del_init(&task->item);
		osal_mutex_unlock(&job->lock);

		_tde_task_run(task);
		TRACE_TDE(DBG_INFO, "***running task(%d), handle(%d)***.\n", task->mode, job->hdl);

		osal_vfree(task);
		ret = osal_sem_down_timeout(sem, timeout);
		if (ret != OSAL_SUCCESS) {
			TRACE_TDE(DBG_ERR, "TDE timeout, HW hang.\n");
			tde_ip_reset();
			_tde_remove_task(job);
			break;
		}

		osal_atomic_dec(&job->task_cnt);
	}

	if (job->is_block) {
		TRACE_TDE(DBG_INFO, "wakeup, handle(%d).\n", job->hdl);
		osal_wait_wakeup(&job->wait);
	} else {
		TRACE_TDE(DBG_INFO, "END, handle(%d).\n", job->hdl);
		osal_mutex_destroy(&job->lock);
		osal_wait_destroy(&job->wait);
		osal_vfree(job);
	}
}

static int tde_event_handler(void *arg)
{
	int ret;
	struct tde_core *core = (struct tde_core *)arg;
	unsigned long timeout = osal_msecs_to_jiffies(TDE_IDLE_WAIT_TIMEOUT_MS);
	struct tde_job *job;

	while (!core->stop_flag) {
		ret = osal_sem_down_timeout(&core->sem_job, timeout);

		if (ret != OSAL_SUCCESS)
			continue;

		if (osal_atomic_read(&core->job_num) == 0)
			continue;

		osal_spin_lock(&core->lock);
		if (osal_list_empty(&core->job_list)) {
			osal_spin_unlock(&core->lock);
			continue;
		}

		job = osal_list_first_entry(&core->job_list,
			struct tde_job, item);
		osal_list_del_init(&job->item);
		osal_spin_unlock(&core->lock);

		osal_clk_enable(core->clk);
		_tde_job_process(job, &core->sem_intr);
		osal_clk_disable(core->clk);

		osal_atomic_dec(&core->job_num);
	}

	return 0;
}

int tde_core_init(struct tde_core *core)
{
	osal_atomic_set(&core->cur_hdl, 0);
	osal_atomic_set(&core->job_num, 0);
	osal_sem_init(&core->sem_job, 0);
	osal_sem_init(&core->sem_intr, 0);
	osal_spin_lock_init(&core->lock);
	OSAL_INIT_LIST_HEAD(&core->job_list);
	core->job_hash = osal_hash_create(TDE_JOB_HASH_SIZE);
	if (!core->job_hash) {
		TRACE_TDE(DBG_ERR, "osal_hash_create fail!\n");
	}

	core->stop_flag = 0;
	core->thread = osal_kthread_create(tde_event_handler, core, "task_tde_hdl", 0);
	if (!core->thread) {
		TRACE_TDE(DBG_ERR, "TDE kthread_create failed.\n");
		return -1;
	}
	osal_kthread_set_priority(core->thread, MM_THREAD_PRIO);

	return 0;
}

int tde_core_deinit(struct tde_core *core)
{
	core->stop_flag = 1;
	osal_sem_up(&core->sem_job);
	osal_kthread_destroy(core->thread, core->stop_flag);
	core->thread = NULL;

	osal_spin_lock(&core->lock);
	osal_hash_destroy(core->job_hash);
	core->job_hash = NULL;
	osal_spin_unlock(&core->lock);

	osal_spin_lock_destroy(&core->lock);
	osal_sem_destroy(&core->sem_job);
	osal_sem_destroy(&core->sem_intr);

	return 0;
}

int tde_core_open(struct tde_core *core)
{
	osal_clk_prepare_enable(core->clk);
	tde_ip_init();
	osal_clk_disable(core->clk);
	return 0;
}

int tde_core_release(struct tde_core *core)
{
	while (osal_atomic_read(&core->job_num) > 0) {
		osal_msleep(1);
	}

	tde_ip_deinit();
	osal_clk_unprepare(core->clk);

	return 0;
}

int tde_core_suspend(struct tde_core *core)
{
	core->stop_flag = 1;
	osal_sem_up(&core->sem_job);
	osal_kthread_destroy(core->thread, core->stop_flag);
	core->thread = NULL;

	return 0;
}

int tde_core_resume(struct tde_core *core)
{
	core->stop_flag = 0;
	core->thread = osal_kthread_create(tde_event_handler, core, "task_tde_hdl", 0);
	if (!core->thread) {
		TRACE_TDE(DBG_ERR, "TDE kthread_create failed.\n");
		return -1;
	}
	osal_kthread_set_priority(core->thread, MM_THREAD_PRIO);

	return 0;
}

int tde_core_isr(struct tde_core *core)
{
	if (tde_ip_clear_intr()) {
		osal_sem_up(&core->sem_intr);
		TRACE_TDE(DBG_INFO, "===isr, sem wakeup===\n");
	}

	return 0;
}
