#include "comm_errno.h"
#include "tde_debug.h"
#include "tde_core.h"

static bool _job_hash_find(struct tde_core *core, tde_handle hdl, struct tde_job **job)
{
	bool is_found = false;
	struct tde_job *obj = NULL;

	osal_spin_lock(&core->lock);
	obj = osal_hash_get(core->job_hash, hdl);
	osal_spin_unlock(&core->lock);

	if (obj) {
		*job = obj;
		is_found = true;
	}
	return is_found;
}

static int _job_done_cond(const void *param)
{
	struct tde_job *job = (struct tde_job *)param;

	return !osal_atomic_read(&job->task_cnt);
}


tde_handle tde_begin_job(struct tde_core *core)
{
	struct tde_job *job;

	job = osal_vzalloc(sizeof(*job));
	if (job == NULL) {
		TRACE_TDE(DBG_ERR, "osal_vzalloc failed.\n");
		return TDE_INVALID_HANDLE;
	}
	job->hdl = osal_atomic_inc_return(&core->cur_hdl);
	osal_mutex_init(&job->lock);
	osal_wait_init(&job->wait);
	osal_atomic_set(&job->task_cnt, 0);
	OSAL_INIT_LIST_HEAD(&job->task_list);

	osal_spin_lock(&core->lock);
	osal_hash_add(core->job_hash, job->hdl, job);
	osal_spin_unlock(&core->lock);

	TRACE_TDE(DBG_INFO, "begin job, handle(%d).\n", job->hdl);

	return job->hdl;
}

int tde_end_job(struct tde_core *core, tde_handle handle, bool is_sync, bool is_block, unsigned int timeout)
{
	struct tde_job *job;
	int ret = 0;

	if (!_job_hash_find(core, handle, &job)) {
		TRACE_TDE(DBG_ERR, "Not find job, check the handle(%d).\n", handle);
		return ERR_TDE_UNEXIST;
	}

	osal_mutex_lock(&job->lock);
	job->is_sync = is_sync;
	job->is_block = is_block;
	osal_mutex_unlock(&job->lock);

	TRACE_TDE(DBG_INFO, "add job, handle(%d).\n", job->hdl);

	osal_spin_lock(&core->lock);
	osal_hash_del(core->job_hash, handle);
	osal_list_add_tail(&job->item, &core->job_list);
	osal_atomic_inc(&core->job_num);
	osal_sem_up(&core->sem_job);
	osal_spin_unlock(&core->lock);

	if (is_block) {
		ret = osal_wait_timeout_uninterruptible(&job->wait, _job_done_cond, job, timeout);
		if (ret <= 0) {
			osal_mutex_lock(&job->lock);
			job->is_block = false;
			osal_mutex_unlock(&job->lock);
			ret = ERR_TDE_BUSY;
			TRACE_TDE(DBG_ERR, "TDE job timeout.\n");
		} else {
			if (osal_atomic_read(&job->task_cnt)) {
				ret = ERR_TDE_NOT_PERM;
			} else {
				ret = 0;
			}
			osal_mutex_destroy(&job->lock);
			osal_wait_destroy(&job->wait);
			osal_vfree(job);
		}
	}

	return ret;
}

int tde_wait_all_done(struct tde_core *core)
{
	while (osal_atomic_read(&core->job_num) > 0) {
		osal_msleep(1);
	}

	TRACE_TDE(DBG_INFO, "all done.\n");

	return 0;
}

int tde_cancel_job(struct tde_core *core, tde_handle handle)
{
	struct tde_job *job;

	if (!_job_hash_find(core, handle, &job)) {
		TRACE_TDE(DBG_ERR, "Not find job, check the handle(%d).\n", handle);
		return ERR_TDE_UNEXIST;
	}

	osal_spin_lock(&core->lock);
	osal_hash_del(core->job_hash, handle);
	osal_spin_unlock(&core->lock);

	osal_mutex_destroy(&job->lock);
	osal_wait_destroy(&job->wait);
	osal_vfree(job);

	TRACE_TDE(DBG_INFO, "cancel job, handle(%d).\n", job->hdl);

	return 0;
}

int tde_rotate(struct tde_core *core, tde_handle handle, tde_surface_s *src,
	tde_surface_s *dst, tde_rotate_angle_e angle)
{
	struct tde_job *job;
	struct tde_task *task;

	if (!TDE_SUPPORT_FMT(src->color_fmt) || !TDE_SUPPORT_FMT(dst->color_fmt)) {
		TRACE_TDE(DBG_ERR, "Not support format, only argb888.\n");
		return ERR_TDE_ILLEGAL_PARAM;
	}

	if (!_job_hash_find(core, handle, &job)) {
		TRACE_TDE(DBG_ERR, "Not find job, check the handle(%d).\n", handle);
		return ERR_TDE_UNEXIST;
	}

	task = osal_vzalloc(sizeof(*task));
	if (task == NULL) {
		TRACE_TDE(DBG_ERR, "osal_vzalloc failed.\n");
		return ERR_TDE_NOMEM;
	}

	if (angle == TDE_ROTATE_90)
		task->mode = TDE_TASK_ROTATE_90;
	else if (angle == TDE_ROTATE_270)
		task->mode = TDE_TASK_ROTATE_270;
	else {
		osal_vfree(task);
		TRACE_TDE(DBG_ERR, "angle invalid.\n");
		return ERR_TDE_ILLEGAL_PARAM;
	}
	task->src = *src;
	task->dst = *dst;

	osal_mutex_lock(&job->lock);
	osal_list_add_tail(&task->item, &job->task_list);
	osal_atomic_inc(&job->task_cnt);
	osal_mutex_unlock(&job->lock);
	TRACE_TDE(DBG_INFO, "add rotate(%d) task, handle(%d).\n", angle, job->hdl);

	return 0;
}

int tde_draw_line(struct tde_core *core, tde_handle handle, tde_surface_s *src,
	tde_surface_s *dst, tde_line_s *line)
{
	struct tde_job *job;
	struct tde_task *task;

	if (!TDE_SUPPORT_FMT(src->color_fmt) || !TDE_SUPPORT_FMT(dst->color_fmt)) {
		TRACE_TDE(DBG_ERR, "Not support format, only argb888.\n");
		return ERR_TDE_ILLEGAL_PARAM;
	}
	if ((line->start_x >= src->width) || (line->end_x > src->width)
		|| (line->start_y >= src->height) || (line->end_y > src->height)) {
		TRACE_TDE(DBG_ERR, "line param invalid, out of range.\n");
		return ERR_TDE_ILLEGAL_PARAM;
	}

	if ((line->start_x != line->end_x) && (line->start_y != line->end_y)) {
		TRACE_TDE(DBG_ERR, "Slashes are not supported.\n");
		return ERR_TDE_ILLEGAL_PARAM;
	}
	if ((line->start_x == line->end_x) && (line->start_y == line->end_y)) {
		TRACE_TDE(DBG_ERR, "Point not supported.\n");
		return ERR_TDE_ILLEGAL_PARAM;
	}
	if (!_job_hash_find(core, handle, &job)) {
		TRACE_TDE(DBG_ERR, "Not find job, check the handle(%d).\n", handle);
		return ERR_TDE_UNEXIST;
	}

	task = osal_vzalloc(sizeof(*task));
	if (task == NULL) {
		TRACE_TDE(DBG_ERR, "osal_vzalloc failed.\n");
		return ERR_TDE_NOMEM;
	}

	task->mode = TDE_TASK_DRAW_LINE;
	task->src = *src;
	task->dst = *dst;
	task->line = *line;
	if (line->start_x == line->end_x) {
		task->line.end_x += line->thick;
	}
	if (line->start_y == line->end_y) {
		task->line.end_y += line->thick;
	}
	if ((task->line.end_x > src->width) || (task->line.end_y > src->height)) {
		TRACE_TDE(DBG_ERR, "line param invalid, out of range.\n");
		osal_vfree(task);
		return ERR_TDE_ILLEGAL_PARAM;
	}

	osal_mutex_lock(&job->lock);
	osal_list_add_tail(&task->item, &job->task_list);
	osal_atomic_inc(&job->task_cnt);
	osal_mutex_unlock(&job->lock);
	TRACE_TDE(DBG_INFO, "add draw line task, handle(%d).\n", job->hdl);

	return 0;
}

int tde_quick_copy(struct tde_core *core, tde_handle handle, tde_surface_s *src, tde_surface_s *dst)
{
	struct tde_job *job;
	struct tde_task *task;

	if (!TDE_SUPPORT_FMT(src->color_fmt) || !TDE_SUPPORT_FMT(dst->color_fmt)) {
		TRACE_TDE(DBG_ERR, "Not support format, only argb888.\n");
		return ERR_TDE_ILLEGAL_PARAM;
	}

	if (!_job_hash_find(core, handle, &job)) {
		TRACE_TDE(DBG_ERR, "Not find job, check the handle(%d).\n", handle);
		return ERR_TDE_UNEXIST;
	}

	task = osal_vzalloc(sizeof(*task));
	if (task == NULL) {
		TRACE_TDE(DBG_ERR, "osal_vzalloc failed.\n");
		return ERR_TDE_NOMEM;
	}

	task->mode = TDE_TASK_COPY;
	task->src = *src;
	task->dst = *dst;

	osal_mutex_lock(&job->lock);
	osal_list_add_tail(&task->item, &job->task_list);
	osal_atomic_inc(&job->task_cnt);
	osal_mutex_unlock(&job->lock);
	TRACE_TDE(DBG_INFO, "add quick copy task, handle(%d).\n", job->hdl);

	return 0;
}

