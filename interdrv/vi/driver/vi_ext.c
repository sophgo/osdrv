#include "osal_def.h"
#include "isp/vi_tun_cfg.h"
#include "vi_ext.h"
#include "defines.h"
#include "vi_common.h"
#include "comm_errno.h"

/* isp sync task */
struct isp_sync_tsk_ctx g_isp_sync_tsk_ctx[VI_MAX_PIPE_NUM];

#define isp_sync_tsk_get_ctx(dev) (&g_isp_sync_tsk_ctx[dev])

static void isp_sync_task_find_and_execute(int vi_pipe, struct osal_list_head *head)
{
	struct osal_list_head *pos = NULL;
	struct osal_list_head *next = NULL;
	struct isp_sync_task_node *sync_tsk_node = NULL;

	if (!osal_list_empty(head)) {
		osal_list_for_each_safe(pos, next, head) {
			sync_tsk_node = osal_list_entry(pos, struct isp_sync_task_node, list);

			if (sync_tsk_node->isp_sync_tsk_call_back) {
				sync_tsk_node->isp_sync_tsk_call_back(sync_tsk_node->data);
			}
		}
	}
}

static void work_queue_handler(osal_workqueue *worker)
{
	struct isp_sync_tsk_ctx *sync_tsk = osal_container_of((void *)worker, struct isp_sync_tsk_ctx, worker);

	if (osal_sem_down_interruptible(&sync_tsk->sem)) {
		return;
	}

	isp_sync_task_find_and_execute(sync_tsk->vi_pipe, &sync_tsk->workqueue_list.head);

	osal_sem_up(&sync_tsk->sem);
}

static struct osal_list_head *search_node(struct osal_list_head *head, const char *id)
{
	struct osal_list_head *pos = NULL;
	struct osal_list_head *next = NULL;
	struct isp_sync_task_node *sync_tsk_node = NULL;

	osal_list_for_each_safe(pos, next, head) {
		sync_tsk_node = osal_list_entry(pos, struct isp_sync_task_node, list);
		if (!strncmp(sync_tsk_node->sz_id, id, ISP_SYNC_TASK_ID_MAX_LENGTH)) {
			return pos;
		}
	}

	return NULL;
}

int isp_sync_task_register(int vi_pipe, struct isp_sync_task_node *new_node)
{
	struct isp_sync_tsk_ctx *sync_tsk = isp_sync_tsk_get_ctx(vi_pipe);
	struct osal_list_head *target_list = NULL;
	struct osal_list_head *pos = NULL;
	struct isp_list_entry *list_entry_tmp = NULL;

	if (new_node == NULL) {
		return -1;
	}

	if (new_node->method == ISP_SYNC_TSK_METHOD_HW_IRQ) {
		target_list = &sync_tsk->hwirq_list.head;
	} else {
		target_list = &sync_tsk->workqueue_list.head;
	}

	list_entry_tmp = osal_list_entry(target_list, struct isp_list_entry, head);
	if (list_entry_tmp == NULL) {
		return -1;
	}

	if (list_entry_tmp->num >= ISP_SYNC_TSK_MAX_NUM) {
		return -1;
	}

	pos = search_node(target_list, new_node->sz_id);
	if (pos) {
		return -1;
	}

	if (osal_sem_down_interruptible(&sync_tsk->sem)) {
		return ERR_VI_SYS_NOTREADY;
	}

	osal_list_add_tail(&new_node->list, target_list);

	list_entry_tmp->num++;

	osal_sem_up(&sync_tsk->sem);

	return 0;
}
osal_module_export(isp_sync_task_register);

int isp_sync_task_unregister(int vi_pipe, struct isp_sync_task_node *del_node)
{
	struct isp_sync_tsk_ctx *sync_tsk = isp_sync_tsk_get_ctx(vi_pipe);
	struct osal_list_head *target_list = NULL;
	struct isp_list_entry *list_entry_tmp = NULL;
	struct osal_list_head *pos = NULL;
	int del_success = -1;

	if (del_node == NULL) {
		return -1;
	}

	if (del_node->method == ISP_SYNC_TSK_METHOD_HW_IRQ) {
		target_list = &sync_tsk->hwirq_list.head;
	} else {
		target_list = &sync_tsk->workqueue_list.head;
	}

	list_entry_tmp = osal_list_entry(target_list, struct isp_list_entry, head);
	if (list_entry_tmp == NULL) {
		return -1;
	}

	if (osal_sem_down_interruptible(&sync_tsk->sem)) {
		return ERR_VI_SYS_NOTREADY;
	}

	pos = search_node(target_list, del_node->sz_id);

	if (pos) {
		osal_list_del(pos);
		if (list_entry_tmp->num > 0) {
			list_entry_tmp->num = list_entry_tmp->num - 1;
		}

		del_success = 0;
	}

	osal_sem_up(&sync_tsk->sem);

	return del_success;
}
osal_module_export(isp_sync_task_unregister);

int isp_sync_task_process(int vi_pipe)
{
	struct isp_sync_tsk_ctx *sync_tsk = isp_sync_tsk_get_ctx(vi_pipe);

	if (sync_tsk->hwirq_list.num) {
		isp_sync_task_find_and_execute(vi_pipe, &sync_tsk->hwirq_list.head);
	}

	if (sync_tsk->workqueue_list.num) {
		osal_workqueue_schedule(&sync_tsk->worker);
	}

	return 0;
}

void sync_task_init(int vi_pipe)
{
	struct isp_sync_tsk_ctx *sync_tsk = isp_sync_tsk_get_ctx(vi_pipe);

	OSAL_INIT_LIST_HEAD(&sync_tsk->hwirq_list.head);
	OSAL_INIT_LIST_HEAD(&sync_tsk->workqueue_list.head);

	sync_tsk->hwirq_list.num     = 0;
	sync_tsk->workqueue_list.num = 0;
	osal_sem_init(&sync_tsk->sem, 1);

	osal_workqueue_init(&sync_tsk->worker, work_queue_handler);

	vi_pr(VI_INFO, "vi_pipe %d\n", vi_pipe);
}

void sync_task_exit(int vi_pipe)
{
	struct isp_sync_tsk_ctx *sync_tsk = isp_sync_tsk_get_ctx(vi_pipe);

	sync_tsk->hwirq_list.num     = 0;
	sync_tsk->workqueue_list.num = 0;

	osal_sem_destroy(&sync_tsk->sem);
	osal_workqueue_destroy(&sync_tsk->worker);

	vi_pr(VI_INFO, "vi_pipe %d\n", vi_pipe);
}
