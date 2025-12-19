#include <aos/cli.h>
#include <debug/dbg.h>

#include "osal.h"
#include <base_cb.h>
#include <ldc_cb.h>
#include "ldc_debug.h"
#include "ldc_core.h"
#include "driver_ldc.h"
#include "ldc_uapi.h"
#include "ldc_proc.h"
#include "ldc.h"
#include "mesh.h"
#include "ldc_sdk.h"
#include "drv/cvi_irq.h"
#include "ldc_comm_layer.h"

#define LDC_DEV_NAME "cvi-ldc"

#define GDC_SHARE_MEM_SIZE (0x8000)
#define REG_LDC_BASE_ADDR (void *)0x0a096000

u32 ldc_log_lv = DBG_WARN/*DBG_INFO*/;
static struct ldc_vdev *ldc_dev;

bool ldc_dump_reg = false;

int ldc_open(void)
{
	return 0;
}
int ldc_release(void)
{
	return 0;
}

long driver_ldc_ioctl(unsigned int cmd, void *arg)
{
	struct ldc_vdev *wdev = ldc_dev;
	int ret = 0;

	switch (cmd) {
		case LDC_BEGIN_JOB: {
			struct gdc_handle_data *data = (struct gdc_handle_data *)arg;

			TRACE_LDC(DBG_DEBUG, "CVILDC_BEGIN_JOB\n");
			CHECK_IOCTL_CMD(cmd, struct gdc_handle_data);
			ret = ldc_begin_job(&wdev->ctx, data);
			break;
		}

		case LDC_END_JOB: {
			struct gdc_handle_data *data = (struct gdc_handle_data *)arg;

			TRACE_LDC(DBG_DEBUG, "CVILDC_END_JOB, handle=0x%llx\n",
				      (unsigned long long)data->handle);
			CHECK_IOCTL_CMD(cmd, struct gdc_handle_data);
			ret = ldc_end_job(&wdev->ctx, data->handle);
			break;
		}

		case LDC_CANCEL_JOB: {
			struct gdc_handle_data *data = (struct gdc_handle_data *)arg;

			TRACE_LDC(DBG_DEBUG, "CVILDC_END_JOB, handle=0x%llx\n",
				      (unsigned long long)data->handle);
			CHECK_IOCTL_CMD(cmd, struct gdc_handle_data);
			ret = ldc_cancel_job(&wdev->ctx, data->handle);
			break;
		}

		case LDC_ADD_ROT_TASK: {
			struct gdc_task_attr *attr = (struct gdc_task_attr *)arg;

			TRACE_LDC(DBG_DEBUG, "CVILDC_ADD_ROT_TASK, handle=0x%llx\n",
				      (unsigned long long)attr->handle);
			CHECK_IOCTL_CMD(cmd, struct gdc_task_attr);
			ret = ldc_add_rotation_task(&wdev->ctx, attr);
			break;
		}

		case LDC_ADD_LDC_TASK: {
			struct gdc_task_attr *attr = (struct gdc_task_attr *)arg;

			TRACE_LDC(DBG_DEBUG, "CVILDC_ADD_LDC_TASK, handle=0x%llx\n",
				      (unsigned long long)attr->handle);
			CHECK_IOCTL_CMD(cmd, struct gdc_task_attr);
			ret = ldc_add_ldc_task(&wdev->ctx, attr);
			break;
		}

		case LDC_SET_JOB_IDENTITY: {
			struct gdc_identity_attr *identity = (struct gdc_identity_attr *)arg;

			TRACE_LDC(DBG_DEBUG, "LDC_SET_JOB_IDENTITY, handle=0x%llx\n",
					(unsigned long long)identity->handle);
			CHECK_IOCTL_CMD(cmd, struct gdc_identity_attr);

			ret = ldc_set_identity(&wdev->ctx, identity);
			break;
		}
		case LDC_ATTACH_VB_POOL: {
			struct ldc_vb_pool_cfg *cfg = (struct ldc_vb_pool_cfg *)arg;

			CHECK_IOCTL_CMD(cmd, struct ldc_vb_pool_cfg);

			ret = ldc_attach_vb_pool(cfg);
			break;
		}
		case LDC_DETACH_VB_POOL: {
			struct ldc_vb_pool_cfg *cfg = (struct ldc_vb_pool_cfg *)arg;

			CHECK_IOCTL_CMD(cmd, struct ldc_vb_pool_cfg);

			ret = ldc_detach_vb_pool(cfg);
			break;
		}
		case LDC_SUSPEND: {
			ret = ldc_suspend();
			break;
		}

		case LDC_RESUME: {
			ret = ldc_resume();
			break;
		}

		case LDC_GET_INTER_CHN_ATTR: {
			struct ldc_internal_chn_attr *attr = (struct ldc_internal_chn_attr *)arg;

			CHECK_IOCTL_CMD(cmd, struct ldc_internal_chn_attr);

			ret = ldc_get_internal_chn_attr(&wdev->ctx, attr);
			break;
		}

		case CVI_LDC_SET_CHN_LDC_CFG: {
			struct ldc_internal_chn_ldc_cfg *cfg = (struct ldc_internal_chn_ldc_cfg *)arg;

			CHECK_IOCTL_CMD(cmd, struct ldc_internal_chn_ldc_cfg);

			ret = ldc_set_internal_chn_ldc_cfg(&wdev->ctx, cfg);
			break;
		}

		case LDC_GET_CHN_FRM: {
			struct gdc_chn_frm_cfg *cfg = (struct gdc_chn_frm_cfg *)arg;
			video_frame_info_s *video_frame = &cfg->video_frame;
			struct gdc_identity_attr *identity = &cfg->identity;
			int milli_sec = cfg->milli_sec;

			CHECK_IOCTL_CMD(cmd, struct gdc_chn_frm_cfg);

			ret = ldc_get_chn_frame(&wdev->ctx, identity, video_frame, milli_sec);
			break;
		}

		case LDC_GET_WORK_JOB: {
			struct gdc_handle_data *data = (struct gdc_handle_data *)arg;

			CHECK_IOCTL_CMD(cmd, struct gdc_handle_data);

			ret = ldc_get_work_job(&wdev->ctx, data);
			break;
		}

		default: {
			TRACE_LDC(DBG_ERR, "bad ioctl cmd((%d))\n", cmd);
			ret = -1;
			break;
		}
	}

	return ret;
}

/*************************************************************************
 *	General functions
 *************************************************************************/
int ldc_create_instance(struct ldc_vdev *wdev)
{
	int rc = 0;
	int i;

	// clk_ldc_src_sel default 1(clk_src_vip_sys_2), 600 MHz
	// set 0(clk_src_vip_sys_4), 400MHz for ND
	// vip_sys_reg_write_mask(VIP_SYS_VIP_CLK_CTRL1, BIT(20), 1);

	// wdev->align = LDC_ADDR_ALIGN;
	osal_strcpy(wdev->dev_name, LDC_DEV_NAME);

	wdev->ctx.shared_mem = osal_zalloc(sizeof(struct ldc_proc_ctx));
	if (!wdev->ctx.shared_mem) {
		TRACE_LDC(DBG_ERR, "gdc shared mem alloc fail\n");
		return -1;
	}
#if CONFIG_LDC_SUPPORT_PROC
	if (ldc_proc_init(wdev->ctx.shared_mem) < 0) {
		TRACE_LDC(DBG_ERR, "gdc proc init failed\n");
		goto err_proc;
	}
#endif

	for (i = 0; i < LDC_DEV_MAX_CNT; i++) {
		osal_atomic_set(&wdev->ctx.core[i].clk_en, false);
	}

	ldc_dev_init(&wdev->ctx);

	if (ldc_sw_init(&wdev->ctx)) {
		TRACE_LDC(DBG_ERR, "gdc init fail\n");
		goto err_sw_init;
	}

	return rc;

err_sw_init:
	ldc_sw_deinit(&wdev->ctx);

#if CONFIG_LDC_SUPPORT_PROC
err_proc:
	osal_kfree(wdev->ctx.shared_mem);
#endif
	return rc;
}

int ldc_destroy_instance(struct ldc_vdev *wdev)
{
	ldc_sw_deinit(&wdev->ctx);
	ldc_dev_deinit(&wdev->ctx);
#if CONFIG_LDC_SUPPORT_PROC
	ldc_proc_remove();
#endif
	if (wdev->ctx.shared_mem)
		osal_free(wdev->ctx.shared_mem);

	return 0;
}

static void ldc_tsk_finish(struct ldc_ctx *ctx, int top_id)
{
	struct ldc_core *core = &ctx->core[top_id];
	ldc_wkup_frm_done_work(core);
}

static void ldc_irq_handler(unsigned char intr_status, struct ldc_ctx *ctx, int top_id, struct ldc_job *done_job)
{
	if (!ctx || !done_job)
		return;

	if (top_id < 0 || top_id >= LDC_DEV_MAX_CNT) {
		TRACE_LDC(DBG_ERR, "invalid ldc_dev_%d\n", top_id);
		return;
	}

	if (done_job->use_cmdq) {
		TRACE_LDC(DBG_DEBUG, "core(%d) cmdq status(%#x)\n", top_id, intr_status);
	} else {
		if (!(intr_status & BIT(0))) {
			TRACE_LDC(DBG_ERR, "core(%d) axi read err, status(%#x)\n", top_id, intr_status);
			return;
		}
	}

	osal_atomic_set(&ctx->core[top_id].state, LDC_CORE_STATE_END);
	TRACE_LDC(DBG_INFO, "core(%d) state set(%d)\n", top_id, LDC_CORE_STATE_END);

	ldc_tsk_finish(ctx, top_id);
}

static int ldc_isr(int irq, void *data)
{
	struct ldc_ctx *ctx = (struct ldc_ctx *)data;
	struct ldc_core *core;
	int top_id, i;
	unsigned char intr_status;
	struct ldc_job *done_job;

	if (!ctx) {
		TRACE_LDC(DBG_ERR, "invalid ldc_dev\n");
		return -1;
	}

	for (i = 0; i < ctx->core_num; i++) {
		core = &ctx->core[i];
		if (core->irq_num == irq) {
			top_id = i;
			osal_spin_lock(&core->core_lock);
			done_job = osal_list_first_entry(&core->list, struct ldc_job, node);
			osal_spin_unlock(&core->core_lock);

			if ((!done_job)) {
				TRACE_LDC(DBG_NOTICE, "null done job\n");
				goto CMDQ_STATUS_UNKOWN;
			}

			if ((done_job->coreid != top_id)) {
				TRACE_LDC(DBG_NOTICE, "done job core[%d] not match with [%d]\n"
					, done_job->coreid, top_id);
				goto CMDQ_STATUS_UNKOWN;
			}

			if (done_job->use_cmdq) {
				intr_status = ldc_cmdq_intr_status(top_id);
				if (intr_status) {
					ldc_cmdq_intr_clr(top_id, intr_status);
					TRACE_LDC(DBG_DEBUG, "ldc_%d irqn(%d) cmdq-status(0x%x)\n"
						, top_id, irq, intr_status);

					if ((intr_status & 0x04) && ldc_cmdq_is_sw_restart(top_id)) {
						TRACE_LDC(DBG_DEBUG, "cmdq-sw restart\n");
						ldc_cmdq_sw_restart(top_id);
					}
				} else {
					goto CMDQ_STATUS_UNKOWN;
				}
			} else {
CMDQ_STATUS_UNKOWN:
				intr_status = ldc_intr_status(top_id);
				TRACE_LDC(DBG_INFO, "ldc_%d, irq(%d), status(0x%x)\n"
					, top_id, irq, intr_status);
				ldc_intr_clr(intr_status, top_id);
			}
			ldc_intr_ctrl(0x00, top_id);
			ldc_disable(top_id);
			ldc_irq_handler(intr_status, ctx, top_id, done_job);
		}
	}

	return 0;
}

static int ldc_init_resources(struct ldc_vdev *wdev)
{
	int rc = 0;
	int i;
	int irq_num = 25;
	const char *const irq_name = "ldc";
	const char ldc_clk_name[LDC_DEV_MAX_CNT][20] = {"reg_clk_ldc_vip_en"};
	void *reg_base = REG_LDC_BASE_ADDR;

	ldc_set_base_addr(reg_base, 0);

	if (osal_irq_request(irq_num, (int (*)(int,  void *))ldc_isr, 0, irq_name, (void *)&wdev->ctx)) {
		TRACE_LDC(DBG_ERR, "Unable to request ldc IRQ(%d)\n", irq_num);
		return -1;
	}

	for (i = 0; i < LDC_DEV_MAX_CNT; i++) {
		wdev->ctx.core[i].ldc_clk = osal_clk_get(NULL, ldc_clk_name[i]);
		if (!(wdev->ctx.core[i].ldc_clk)) {
			TRACE_LDC(DBG_ERR, "Cannot get clk for clk_ldc[%d]\n", i);
			wdev->ctx.core[i].ldc_clk = NULL;
		}

		wdev->ctx.core[i].irq_num = irq_num;
	}

	TRACE_LDC(DBG_DEBUG, "irq(%d) for %s get from vip ldc driver.\n", irq_num, irq_name);
	return rc;
}

int driver_ldc_init(void)
{
	int rc = 0;
	struct ldc_vdev *wdev;
	struct ldc_ctx **ctx = get_ldc_ctx_addr();

	TRACE_LDC(DBG_INFO, "done with rc(%d).\n", rc);
	TRACE_LDC(DBG_WARN, "+\n");

	wdev = osal_zalloc(sizeof(*wdev));
	if (!wdev) {
		TRACE_LDC(DBG_ERR, "Can not get ldc_vdev\n");
		return -1;
	}

	ldc_dev = wdev;
	*ctx = &ldc_dev->ctx;

	// get hw-resources
	rc = ldc_init_resources(wdev);
	if (rc)
		goto err_ldc_reg;

	// for ldc
	rc = ldc_create_instance(wdev);
	if (rc) {
		TRACE_LDC(DBG_ERR, "Failed to create ldc instance\n");
		goto err_ldc_reg;
	}

	/* ldc register cb */
	if (ldc_reg_cb(&wdev->ctx)) {
		TRACE_LDC(DBG_ERR, "Failed to register ldc cb, err %d\n", rc);
		return rc;
	}

	TRACE_LDC(DBG_INFO, "done with rc(%d).\n", rc);
	TRACE_LDC(DBG_WARN, "-\n");

	return rc;

err_ldc_reg:
	TRACE_LDC(DBG_ERR, "failed with rc(%d).\n", rc);
	return rc;
}

/*
 * driver_ldc_exit - device remove method.
 * @pdev: Pointer of platform device.
 */
int driver_ldc_exit(void)
{
	struct ldc_vdev *wdev = ldc_dev;
	int i;

	if (!wdev) {
		TRACE_LDC(DBG_ERR, "Can not get ldc_vdev\n");
		return -1;
	}
	ldc_destroy_instance(wdev);

	for (i = 0; i < LDC_DEV_MAX_CNT; i++) {
		if (wdev->ctx.core[i].ldc_clk) {
			osal_clk_put(NULL, wdev->ctx.core[i].ldc_clk);
			wdev->ctx.core[i].ldc_clk = NULL;
		}
	}
	/* ldc rm cb */
	if (ldc_rm_cb()) {
		TRACE_LDC(DBG_ERR, "Failed to rm ldc cb\n");
		return -1;
	}
	osal_free(wdev);

	return 0;
}

static void set_ldc_log_level(int32_t argc, char **argv)
{
	int level;

	if (argc >= 2) {
		level = atoi(argv[1]);
		aos_debug_printf("Set ldc_log_lv, (%d) -> (%d).\n", ldc_log_lv, level);
		ldc_log_lv = level;
	} else {
		aos_debug_printf("ldc_log_lv = %d.\n", ldc_log_lv);
	}
}

ALIOS_CLI_CMD_REGISTER(set_ldc_log_level, ldc_log_lv, set ldc_log_lv);
