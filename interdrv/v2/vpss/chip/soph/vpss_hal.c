#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/kthread.h>
#include <linux/clk-provider.h>

#include "vpss_hal.h"
#include "vpss_debug.h"
#include "vpss_platform.h"
#include "ion.h"
#include "base_common.h"

#define FBD_MAX_CHN (2)

enum vpss_online_state {
	VPSS_ONLINE_UNEXIST,
	VPSS_ONLINE_READY,
	VPSS_ONLINE_RUN,
	VPSS_ONLINE_CONTINUE,
};

struct vpss_task_ctx {
	struct list_head job_wait_queue;
	struct list_head job_online_queue;
	spinlock_t job_lock;
	u8 online_dev_max_num;
	u8 online_status[VPSS_ONLINE_NUM];
	struct vpss_cmdq_buf stCmdqBuf[2];
};

struct vpss_convert_to_bak {
	u8 enable;
	struct cvi_vpss_frmsize grp_src_size;
	unsigned int grp_bytesperline[2];
	struct cvi_rect grp_crop;
	struct cvi_vpss_frmsize chn_src_size;
	struct cvi_vpss_frmsize chn_dst_size;
	unsigned int chn_bytesperline[2];
	struct cvi_rect chn_crop;
	struct cvi_rect chn_rect;
};

static struct vpss_task_ctx stTaskCtx;
static struct cvi_vpss_device *vpss_dev;
struct vpss_convert_to_bak convert_to_bak[VPSS_MAX];

extern int work_mask;

void show_hw_state(void)
{
	u8 state[CVI_VPSS_MAX];
	u8 i;

	for (i = 0; i < CVI_VPSS_MAX; i++)
		state[i] = atomic_read(&vpss_dev->vpss_cores[i].state);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "v(%d %d %d %d) t(%d %d %d %d) d(%d %d).\n",
		state[0], state[1], state[2], state[3], state[4], state[5], state[6],
		state[7], state[8], state[9]);
}

int find_available_dev(u8 chn_num)
{
	int i;
	int start_idx = CVI_VPSS_V0;
	int end_idx = CVI_VPSS_V3;
	//int dev_num = 4;
	u8 mask = 0, mask_tmp;
	u8 user_mask = BIT(chn_num) - 1;

	for (i = start_idx; i <= end_idx; i++)
		if ((work_mask & BIT(i)) && (atomic_read(&vpss_dev->vpss_cores[i].state) == CVI_VIP_IDLE))
			mask |= BIT(i);

	for (i = start_idx; i <= end_idx; i++) {
		mask_tmp = mask;
		mask_tmp &= user_mask;
		if (!(user_mask ^ mask_tmp))
			return i;
		//if (mask & 0x1) //v3 -> v0
		//	mask |= BIT(dev_num);
		mask = mask >> 1;
	}

	return -1;
}

int job_cheack_hw_ready(bool isFBD, u8 chn_num, bool vpss_v_priority)
{
	int i, start_core;

	if (isFBD && (chn_num > FBD_MAX_CHN)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "FBD maximum number of channels(%d), job chn num(%d).\n",
			FBD_MAX_CHN, chn_num);
		return -1;
	}

	if(vpss_v_priority && (!isFBD))
		start_core = CVI_VPSS_V0; //vpss_v priority
	else
		start_core = CVI_VPSS_T0; //vpss_t -> vpss_v

	if (chn_num == 1) {
		for (i = start_core; i < CVI_VPSS_MAX; ++i) {
			if ((work_mask & BIT(i)) && (atomic_read(&vpss_dev->vpss_cores[i].state) == CVI_VIP_IDLE))
				return i;
		}

		if (isFBD)
			return -1;
	} else if (chn_num == 2) {
		if ((atomic_read(&vpss_dev->vpss_cores[CVI_VPSS_T0].state) == CVI_VIP_IDLE)
			&& (atomic_read(&vpss_dev->vpss_cores[CVI_VPSS_T1].state) == CVI_VIP_IDLE))
			return CVI_VPSS_T0;
		if ((atomic_read(&vpss_dev->vpss_cores[CVI_VPSS_T2].state) == CVI_VIP_IDLE)
			&& (atomic_read(&vpss_dev->vpss_cores[CVI_VPSS_T3].state) == CVI_VIP_IDLE))
			return CVI_VPSS_T2;
		if ((work_mask & BIT(CVI_VPSS_D0))  && (work_mask & BIT(CVI_VPSS_D1))
			&& (atomic_read(&vpss_dev->vpss_cores[CVI_VPSS_D0].state) == CVI_VIP_IDLE)
			&& (atomic_read(&vpss_dev->vpss_cores[CVI_VPSS_D1].state) == CVI_VIP_IDLE))
			return CVI_VPSS_D0;

		if (isFBD)
			return -1;
	}

	return find_available_dev(chn_num);
}

int online_get_dev(struct cvi_vpss_job *pstJob)
{
	u8 i;
	u8 chn_num = pstJob->cfg.chn_num;
	unsigned long flags;

	if (stTaskCtx.online_dev_max_num >= chn_num)
		return CVI_SUCCESS;

	for (i = stTaskCtx.online_dev_max_num; i < chn_num; ++i)
		if (!(atomic_read(&vpss_dev->vpss_cores[i].state) == CVI_VIP_IDLE)) {
			return CVI_FAILURE;
		}

	spin_lock_irqsave(&vpss_dev->lock, flags);
	for (i = stTaskCtx.online_dev_max_num; i < chn_num; i++) {
		atomic_set(&vpss_dev->vpss_cores[i].state, CVI_VIP_ONLINE);
		vpss_dev->vpss_cores[i].isOnline = 1;
	}
	stTaskCtx.online_dev_max_num = chn_num;
	spin_unlock_irqrestore(&vpss_dev->lock, flags);

	return CVI_SUCCESS;
}

bool check_convert_to_addr(struct cvi_vpss_hw_cfg *cfg, u8 chn_num){
	u8 i;
	for(i = 0; i < chn_num; i++)
		if(((cfg->astChnCfg[i].addr[0] & 0xf) != 0 ||
			(cfg->astChnCfg[i].addr[1] & 0xf) != 0 ||
			(cfg->astChnCfg[i].addr[2] & 0xf) != 0) && cfg->astChnCfg[i].convert_to_cfg.enable)
			return true;
	return false;
};

int job_convert_to_min_update(struct cvi_vpss_job *pstJob, int dev_idx, u8 *chn_idx){
	struct cvi_vpss_hw_cfg *cfg = &pstJob->cfg;
	u8 i, chn_id;

	convert_to_bak[dev_idx].enable = true;
	convert_to_bak[dev_idx].grp_src_size = cfg->stGrpCfg.src_size;
	convert_to_bak[dev_idx].grp_bytesperline[0] = cfg->stGrpCfg.bytesperline[0];
	convert_to_bak[dev_idx].grp_bytesperline[1] = cfg->stGrpCfg.bytesperline[1];
	convert_to_bak[dev_idx].grp_crop = cfg->stGrpCfg.crop;

	cfg->stGrpCfg.src_size.width = 16;
	cfg->stGrpCfg.src_size.height = 16;
	cfg->stGrpCfg.bytesperline[0] = 16;
	cfg->stGrpCfg.bytesperline[1] = 16;
	cfg->stGrpCfg.crop.top = 0;
	cfg->stGrpCfg.crop.left = 0;
	cfg->stGrpCfg.crop.width = 16;
	cfg->stGrpCfg.crop.height = 16;

	for (i = dev_idx; i < (dev_idx + cfg->chn_num); i++) {
		chn_id = chn_idx[i - dev_idx];
		convert_to_bak[i].chn_src_size = cfg->astChnCfg[chn_id].src_size;
		convert_to_bak[i].chn_crop = cfg->astChnCfg[chn_id].crop;
		convert_to_bak[i].chn_bytesperline[0] = cfg->astChnCfg[chn_id].bytesperline[0];
		convert_to_bak[i].chn_bytesperline[1] = cfg->astChnCfg[chn_id].bytesperline[1];
		convert_to_bak[i].chn_dst_size = cfg->astChnCfg[chn_id].dst_size;
		convert_to_bak[i].chn_rect = cfg->astChnCfg[chn_id].dst_rect;

		cfg->astChnCfg[chn_id].src_size = cfg->stGrpCfg.src_size;
		cfg->astChnCfg[chn_id].crop = cfg->stGrpCfg.crop;
		cfg->astChnCfg[chn_id].bytesperline[0] = cfg->stGrpCfg.bytesperline[0];
		cfg->astChnCfg[chn_id].bytesperline[1] = cfg->stGrpCfg.bytesperline[1];
		cfg->astChnCfg[chn_id].dst_rect = cfg->stGrpCfg.crop;
		cfg->astChnCfg[chn_id].dst_size = cfg->stGrpCfg.src_size;
	}
	return CVI_SUCCESS;
}

int job_try_schedule(struct cvi_vpss_job *pstJob)
{
	u8 i, chn_id, j = 0;
	u8 chn_idx[VPSS_MAX_CHN_NUM];
	u8 dev_list[VPSS_MAX_CHN_NUM] = {false, false, false, false};
	struct cvi_vpss_hw_cfg *cfg = &pstJob->cfg;
	bool isFBD = cfg->stGrpCfg.fbd_enable;
	u8 chn_num = cfg->chn_num;
	unsigned long flags;
	int dev_idx, dev_idx_max;

	for (i = 0; i < VPSS_MAX_CHN_NUM; i++)
		if (cfg->chn_enable[i]) {
			chn_idx[j] = i;
			j++;
		}

	spin_lock_irqsave(&vpss_dev->lock, flags);
	if (pstJob->is_online)
		dev_idx = 0;
	else
		dev_idx = job_cheack_hw_ready(isFBD, chn_num, cfg->stGrpCfg.vpss_v_priority);
	if (dev_idx < 0) {
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d), hw not ready.\n", pstJob->grp_id);
		show_hw_state();
		spin_unlock_irqrestore(&vpss_dev->lock, flags);
		return CVI_FAILURE;
	}

	convert_to_bak[dev_idx].enable = false;
	if (check_convert_to_addr(cfg, chn_num)){
		job_convert_to_min_update(pstJob, dev_idx, chn_idx);
	}

	if (cfg->stGrpCfg.crop.width > VPSS_HW_LIMIT_WIDTH) { //output > 4608 ?
		pstJob->is_tile = true;
		pstJob->tile_mode = 0;
	} else {
		pstJob->is_tile = false;
	}

	if (cfg->stGrpCfg.crop.height > VPSS_HW_LIMIT_HEIGHT) {
		pstJob->is_v_tile = true;
		pstJob->tile_mode &= SCL_TILE_BOTH;
	} else {
		pstJob->is_v_tile = false;
	}
	pstJob->vpss_dev_mask = 0;
	pstJob->dev_idx_start = dev_idx;
	dev_idx_max = dev_idx + chn_num;
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Scheduling Grp(%d), tile(%d).\n",
		pstJob->grp_id, pstJob->is_tile);

	for (i = dev_idx; i < dev_idx_max; i++) {
		chn_id = chn_idx[i - dev_idx];
		dev_list[chn_id] = i;
		atomic_set(&vpss_dev->vpss_cores[i].state, CVI_VIP_RUNNING);
		vpss_dev->vpss_cores[i].job = (void *)pstJob;
		vpss_dev->vpss_cores[i].map_chn = chn_id;
		vpss_dev->vpss_cores[i].u32StartCnt++;
		//pstJob->dev_idx[i] = dev_idx;
		pstJob->vpss_dev_mask |= BIT(i);

		if (vpss_dev->vpss_cores[i].clk_vpss)
			clk_enable(vpss_dev->vpss_cores[i].clk_vpss);

		if (i == dev_idx)
			cvi_img_update(i, true, &cfg->stGrpCfg);
		else
			cvi_img_update(i, false, &cfg->stGrpCfg); //use dma share
		cvi_sc_update(i, &cfg->astChnCfg[chn_id]);

		if (pstJob->is_tile) {
			vpss_dev->vpss_cores[i].tile_mode = sclr_tile_cal_size(i,
				(pstJob->cfg.astChnCfg[chn_id].src_size.width >> 1) - 1);
			pstJob->tile_mode |= vpss_dev->vpss_cores[i].tile_mode;
		}

		if (pstJob->is_v_tile) {
			vpss_dev->vpss_cores[i].tile_mode |= (sclr_v_tile_cal_size(i,
				(pstJob->cfg.astChnCfg[chn_id].src_size.height >> 1) - 1)) << SCL_V_TILE_OFFSET;
			pstJob->tile_mode |= vpss_dev->vpss_cores[i].tile_mode;
			if((pstJob->tile_mode & SCL_TILE_BOTH) == SCL_TILE_BOTH)
				pstJob->tile_mode |= (pstJob->tile_mode & SCL_V_TILE_BOTH) << SCL_V_TILE_OFFSET;
		}

		if (i == (dev_idx_max - 1))
			cvi_top_update(i, false, isFBD); //last chn,not share
		else
			cvi_top_update(i, true, isFBD);
	}

	if (pstJob->is_tile) {
		if (pstJob->tile_mode & SCL_TILE_LEFT) {
			pstJob->tile_mode &= ~SCL_TILE_LEFT;
			pstJob->is_work_on_r_tile = false;
			for (i = dev_idx; i < dev_idx_max; i++)
				if (!cvi_img_left_tile_cfg(i))
					atomic_set(&vpss_dev->vpss_cores[i].state, CVI_VIP_END);
		} else if (pstJob->tile_mode & SCL_TILE_RIGHT) {
			pstJob->tile_mode &= ~SCL_TILE_RIGHT;
			pstJob->is_work_on_r_tile = true;
			for (i = dev_idx; i < dev_idx_max; i++)
				if (!cvi_img_right_tile_cfg(i))
					atomic_set(&vpss_dev->vpss_cores[i].state, CVI_VIP_END);
		}
	}

	if (pstJob->is_v_tile) {
		if ((pstJob->tile_mode) & SCL_TILE_TOP) {
			pstJob->tile_mode &= ~(SCL_TILE_TOP);
			for (i = dev_idx; i < dev_idx_max; i++)
				if (!cvi_img_top_tile_cfg(i, false))
					atomic_set(&vpss_dev->vpss_cores[i].state, CVI_VIP_END);
		} else if ((pstJob->tile_mode) & SCL_TILE_DOWN) {
			pstJob->tile_mode &= ~(SCL_TILE_DOWN);
			for (i = dev_idx; i < dev_idx_max; i++)
				if (!cvi_img_down_tile_cfg(i, false))
					atomic_set(&vpss_dev->vpss_cores[i].state, CVI_VIP_END);
		}
	}
	spin_unlock_irqrestore(&vpss_dev->lock, flags);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) job working, chn enable(%d %d %d %d), dev(%d %d %d %d).\n",
		pstJob->grp_id, cfg->chn_enable[0], cfg->chn_enable[1],
		cfg->chn_enable[2], cfg->chn_enable[3], dev_list[0], dev_list[1],
		dev_list[2], dev_list[3]);

	atomic_set(&pstJob->enJobState, CVI_JOB_WORKING);

	for (i = dev_idx; i < dev_idx_max; i++)
		ktime_get_ts64(&vpss_dev->vpss_cores[i].ts_start);

	cvi_img_start(dev_idx, chn_num);

	return CVI_SUCCESS;
}


void vpss_hal_reset(struct cvi_vpss_job *pstJob)
{
	unsigned long flags;
	int i;

	spin_lock_irqsave(&vpss_dev->lock, flags);
	for (i = 0; i < CVI_VPSS_MAX; i++) {
		if (!(pstJob->vpss_dev_mask & BIT(i)))
			continue;
		cvi_img_reset(i);
		vpss_dev->vpss_cores[i].job = NULL;
		vpss_dev->vpss_cores[i].intr_status = 0;
		if (pstJob->is_online)
			atomic_set(&vpss_dev->vpss_cores[i].state, CVI_VIP_ONLINE);
		else
			atomic_set(&vpss_dev->vpss_cores[i].state, CVI_VIP_IDLE);
	}
	spin_unlock_irqrestore(&vpss_dev->lock, flags);
}

int cvi_vpss_hal_init(struct cvi_vpss_device *dev)
{
	int i;

	spin_lock_init(&stTaskCtx.job_lock);
	INIT_LIST_HEAD(&stTaskCtx.job_wait_queue);
	INIT_LIST_HEAD(&stTaskCtx.job_online_queue);
	stTaskCtx.online_dev_max_num = 0;
	vpss_dev = dev;

	for (i = 0; i < VPSS_ONLINE_NUM; i++)
		stTaskCtx.online_status[i] = VPSS_ONLINE_UNEXIST;

	for (i = 0; i < 2; i++) {
		stTaskCtx.stCmdqBuf[i].cmdq_phy_addr = 0;
		stTaskCtx.stCmdqBuf[i].cmdq_vir_addr = NULL;
		stTaskCtx.stCmdqBuf[i].cmdq_buf_size = 0;
	}

	return CVI_SUCCESS;
}

void cvi_vpss_hal_deinit(void)
{
	struct cvi_vpss_job *pstJob;
	unsigned long flags;
	int i;

	spin_lock_irqsave(&stTaskCtx.job_lock, flags);
	while (!list_empty(&stTaskCtx.job_wait_queue)) {
		pstJob = list_first_entry(&stTaskCtx.job_wait_queue,
				struct cvi_vpss_job, list);
		list_del_init(&pstJob->list);
	}
	while (!list_empty(&stTaskCtx.job_online_queue)) {
		pstJob = list_first_entry(&stTaskCtx.job_online_queue,
				struct cvi_vpss_job, list);
		list_del_init(&pstJob->list);
	}
	spin_unlock_irqrestore(&stTaskCtx.job_lock, flags);

	for (i = 0; i < 2; i++) {
		if (stTaskCtx.stCmdqBuf[i].cmdq_phy_addr) {
			base_ion_free(stTaskCtx.stCmdqBuf[i].cmdq_phy_addr);
			stTaskCtx.stCmdqBuf[i].cmdq_phy_addr = 0;
			stTaskCtx.stCmdqBuf[i].cmdq_vir_addr = NULL;
			stTaskCtx.stCmdqBuf[i].cmdq_buf_size = 0;
		}
	}

	vpss_dev = NULL;
}

int cvi_vpss_hal_push_job(struct cvi_vpss_job *pstJob)
{
	unsigned long flags;

	if (!pstJob || !pstJob->cfg.chn_num)
		return CVI_FAILURE;

	spin_lock_irqsave(&stTaskCtx.job_lock, flags);
	list_add_tail(&pstJob->list, &stTaskCtx.job_wait_queue);
	atomic_set(&pstJob->enJobState, CVI_JOB_WAIT);
	spin_unlock_irqrestore(&stTaskCtx.job_lock, flags);

	return CVI_SUCCESS;
}

int cvi_vpss_hal_push_online_job(struct cvi_vpss_job *pstJob)
{
	unsigned long flags;

	if (!pstJob || !pstJob->cfg.chn_num)
		return CVI_FAILURE;

	spin_lock_irqsave(&stTaskCtx.job_lock, flags);
	list_add_tail(&pstJob->list, &stTaskCtx.job_online_queue);
	atomic_set(&pstJob->enJobState, CVI_JOB_WAIT);
	spin_unlock_irqrestore(&stTaskCtx.job_lock, flags);

	return CVI_SUCCESS;
}

int cvi_vpss_hal_remove_job(struct cvi_vpss_job *pstJob)
{
	int i, count = 10;
	unsigned long flags;
	struct cvi_vpss_job *jobItem;
	enum cvi_job_state enJobState;

	enJobState = atomic_read(&pstJob->enJobState);

	if ((enJobState == CVI_JOB_WORKING) || (enJobState == CVI_JOB_HALF)) {
		while (--count > 0) {
			if (atomic_read(&pstJob->enJobState) == CVI_JOB_END)
				break;
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "wait count(%d)\n", count);
			usleep_range(5000, 6000);
		}
		if (count == 0) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Wait timeout, HW hang.\n", pstJob->grp_id);
			for (i = 0; i < CVI_VPSS_MAX; i++) {
				if (!(pstJob->vpss_dev_mask & BIT(i)))
					continue;
				cvi_vpss_error_stauts(i);
				vpss_hal_reset(pstJob);
				// cvi_vpss_stauts(i);
				// work_mask &= (~BIT(i));
				if (vpss_dev->vpss_cores[i].clk_vpss &&
					__clk_is_enabled(vpss_dev->vpss_cores[i].clk_vpss))
					clk_disable(vpss_dev->vpss_cores[i].clk_vpss);
			}
		}
	} else if (enJobState == CVI_JOB_WAIT) {
		spin_lock_irqsave(&stTaskCtx.job_lock, flags);
		list_for_each_entry(jobItem, &stTaskCtx.job_wait_queue, list) {
			if (jobItem == pstJob) {
				atomic_set(&pstJob->enJobState, CVI_JOB_INVALID);
				list_del_init(&pstJob->list);
				spin_unlock_irqrestore(&stTaskCtx.job_lock, flags);
				return CVI_SUCCESS;
			}
		}
		list_for_each_entry(jobItem, &stTaskCtx.job_online_queue, list) {
			if (jobItem == pstJob) {
				atomic_set(&pstJob->enJobState, CVI_JOB_INVALID);
				list_del_init(&pstJob->list);
				spin_unlock_irqrestore(&stTaskCtx.job_lock, flags);
				return CVI_SUCCESS;
			}
		}
		spin_unlock_irqrestore(&stTaskCtx.job_lock, flags);
	}

	return CVI_SUCCESS;
}

int cvi_vpss_hal_try_schedule(void)
{
	struct cvi_vpss_job *pstJob = NULL;
	unsigned long flags;
	int ret = CVI_FAILURE;

	spin_lock_irqsave(&stTaskCtx.job_lock, flags);
	while (!list_empty(&stTaskCtx.job_wait_queue)) {
		pstJob = list_first_entry(&stTaskCtx.job_wait_queue,
			struct cvi_vpss_job, list);
		if (pstJob->is_online) {
			if (online_get_dev(pstJob))
				break;
			list_move_tail(&pstJob->list, &stTaskCtx.job_online_queue);
			stTaskCtx.online_status[pstJob->grp_id] = VPSS_ONLINE_READY;
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "online max chn num:%d.\n", stTaskCtx.online_dev_max_num);
		} else {
			if (job_try_schedule(pstJob)) {
				//CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d), try schedule fail, wait for next time.\n",
				//	pstJob->grp_id);
				break;
			}
			list_del_init(&pstJob->list);
		}

		ret = CVI_SUCCESS;
	}
	spin_unlock_irqrestore(&stTaskCtx.job_lock, flags);

	return ret;
}

int cvi_vpss_hal_stitch_schedule(struct vpss_stitch_cfg *pstCfg)
{
	u8 dev_idx;
	unsigned long flags;
	int ret;
	struct vpss_cmdq_buf *cmdq_buf;

	spin_lock_irqsave(&vpss_dev->lock, flags);
	if (atomic_cmpxchg(&vpss_dev->vpss_cores[CVI_VPSS_D0].state,
		CVI_VIP_IDLE, CVI_VIP_RUNNING) == CVI_VIP_IDLE) {
		dev_idx = CVI_VPSS_D0;
	} else if (atomic_cmpxchg(&vpss_dev->vpss_cores[CVI_VPSS_D1].state,
		CVI_VIP_IDLE, CVI_VIP_RUNNING) == CVI_VIP_IDLE) {
		dev_idx = CVI_VPSS_D1;
	} else {
		spin_unlock_irqrestore(&vpss_dev->lock, flags);
		CVI_TRACE_VPSS(CVI_DBG_WARN, "vpss device busy.\n");
		return CVI_FAILURE;
	}

	vpss_dev->vpss_cores[dev_idx].job = (void *)pstCfg;
	vpss_dev->vpss_cores[dev_idx].u32StartCnt++;
	spin_unlock_irqrestore(&vpss_dev->lock, flags);

	cmdq_buf = &stTaskCtx.stCmdqBuf[dev_idx - CVI_VPSS_D0];
	if (!cmdq_buf->cmdq_phy_addr) {
		ret = base_ion_alloc(&cmdq_buf->cmdq_phy_addr, &cmdq_buf->cmdq_vir_addr,
				"vpss_cmdq_buf", VPSS_CMDQ_BUF_SIZE, true);
		if (ret) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "ion alloc fail! ret(%d)\n", ret);
			cmdq_buf->cmdq_phy_addr = 0;
			atomic_set(&vpss_dev->vpss_cores[dev_idx].state, CVI_VIP_IDLE);
			return ret;
		}
		cmdq_buf->cmdq_buf_size = VPSS_CMDQ_BUF_SIZE;
	}

	ktime_get_ts64(&vpss_dev->vpss_cores[dev_idx].ts_start);
	if (vpss_dev->vpss_cores[dev_idx].clk_vpss)
		clk_enable(vpss_dev->vpss_cores[dev_idx].clk_vpss);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "stitch dev(%d) pstCfg(%px) cb(%px) data(%px).\n",
		dev_idx, pstCfg, pstCfg->pfnJobCB, pstCfg->data);

	return cvi_vpss_stitch(dev_idx, cmdq_buf, pstCfg);
}

void cvi_vpss_hal_online_release_dev(void)
{
	unsigned long flags;
	int i;

	spin_lock_irqsave(&vpss_dev->lock, flags);
	for (i = 0; i < stTaskCtx.online_dev_max_num; i++) {
		atomic_set(&vpss_dev->vpss_cores[i].state, CVI_VIP_IDLE);
		vpss_dev->vpss_cores[i].isOnline = 0;
	}
	spin_unlock_irqrestore(&vpss_dev->lock, flags);

	spin_lock_irqsave(&stTaskCtx.job_lock, flags);
	stTaskCtx.online_dev_max_num = 0;
	for (i = 0; i < VPSS_ONLINE_NUM; i++)
		stTaskCtx.online_status[i] = VPSS_ONLINE_UNEXIST;
	spin_unlock_irqrestore(&stTaskCtx.job_lock, flags);
}

int cvi_vpss_hal_online_run(struct cvi_vpss_online_cb *param)
{
	int i;
	unsigned long flags;
	struct cvi_vpss_job *pstWorkJob = NULL;
	struct cvi_vpss_job *jobItem;

	if (param->snr_num >= VPSS_ONLINE_NUM) {
		CVI_TRACE_VPSS(CVI_DBG_WARN, "param->snr_num(%d) err.\n", param->snr_num);
		return CVI_FAILURE;
	}

	spin_lock_irqsave(&stTaskCtx.job_lock, flags);
	if (stTaskCtx.online_status[param->snr_num] == VPSS_ONLINE_UNEXIST) {
		CVI_TRACE_VPSS(CVI_DBG_WARN, "vpss(%d) not ready.\n", param->snr_num);
		goto err;
	} else if (stTaskCtx.online_status[param->snr_num] == VPSS_ONLINE_RUN) {
		CVI_TRACE_VPSS(CVI_DBG_NOTICE, "vpss(%d) is running.\n", param->snr_num);
		stTaskCtx.online_status[param->snr_num] = VPSS_ONLINE_CONTINUE;
		goto err;
	} else if (stTaskCtx.online_status[param->snr_num] == VPSS_ONLINE_CONTINUE) {
		CVI_TRACE_VPSS(CVI_DBG_WARN, "vpss(%d) is hang.\n", param->snr_num);
		goto err;
	}

	list_for_each_entry(jobItem, &stTaskCtx.job_online_queue, list) {
		if (jobItem->grp_id == param->snr_num) {
			pstWorkJob = jobItem;
			break;
		}
	}

	if (!pstWorkJob) {
		CVI_TRACE_VPSS(CVI_DBG_WARN, "online grp(%d), Empty job.\n", param->snr_num);
		goto err;
	}
	for (i = 0; i < VPSS_MAX_CHN_NUM; i++) {
		if (!pstWorkJob->cfg.chn_enable[i])
			continue;
		if (pstWorkJob->cfg.astChnCfg[i].addr[0] == 0) {
			CVI_TRACE_VPSS(CVI_DBG_WARN, "online grp(%d), Not buffer.\n", param->snr_num);
			goto err;
		}
	}
	job_try_schedule(pstWorkJob);
	list_del_init(&pstWorkJob->list);
	stTaskCtx.online_status[param->snr_num] = VPSS_ONLINE_RUN;
	spin_unlock_irqrestore(&stTaskCtx.job_lock, flags);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "online trigger vpss, snr_num(%d).\n", param->snr_num);

	return CVI_SUCCESS;

err:
	spin_unlock_irqrestore(&stTaskCtx.job_lock, flags);
	return CVI_FAILURE;
}

int vpss_job_restart(struct cvi_vpss_job *pstJob){
	u8 chn_idx[VPSS_MAX_CHN_NUM];
	u8 i, chn_id, j = 0;
	int dev_idx = pstJob->dev_idx_start;
	int dev_idx_max = dev_idx + pstJob->cfg.chn_num;
	struct cvi_vpss_hw_cfg *cfg = &pstJob->cfg;
	unsigned long flags;

	for (i = 0; i < VPSS_MAX_CHN_NUM; i++)
		if (cfg->chn_enable[i]) {
			chn_idx[j] = i;
			j++;
		}

	cfg->stGrpCfg.src_size = convert_to_bak[dev_idx].grp_src_size;
	cfg->stGrpCfg.bytesperline[0] = convert_to_bak[dev_idx].grp_bytesperline[0];
	cfg->stGrpCfg.bytesperline[1] = convert_to_bak[dev_idx].grp_bytesperline[1];
	cfg->stGrpCfg.crop = convert_to_bak[dev_idx].grp_crop;

	if (cfg->stGrpCfg.crop.width > VPSS_HW_LIMIT_WIDTH) { //output > 4608 ?
		pstJob->is_tile = true;
		pstJob->tile_mode = 0;
	} else {
		pstJob->is_tile = false;
	}

	if (cfg->stGrpCfg.crop.height > VPSS_HW_LIMIT_HEIGHT) {
		pstJob->is_v_tile = true;
		pstJob->tile_mode &= SCL_TILE_BOTH;
	} else {
		pstJob->is_v_tile = false;
	}

	spin_lock_irqsave(&vpss_dev->lock, flags);
	for (i = dev_idx; i < dev_idx_max; i++) {
		chn_id = chn_idx[i - dev_idx];

		cfg->astChnCfg[chn_id].src_size = convert_to_bak[i].chn_src_size;
		cfg->astChnCfg[chn_id].crop = convert_to_bak[i].chn_crop;
		cfg->astChnCfg[chn_id].bytesperline[0] = convert_to_bak[i].chn_bytesperline[0];
		cfg->astChnCfg[chn_id].bytesperline[1] = convert_to_bak[i].chn_bytesperline[1];
		cfg->astChnCfg[chn_id].dst_size = convert_to_bak[i].chn_dst_size;
		cfg->astChnCfg[chn_id].dst_rect = convert_to_bak[i].chn_rect;

		atomic_set(&vpss_dev->vpss_cores[i].state, CVI_VIP_RUNNING);

		if (i == dev_idx)
			cvi_img_update(i, true, &cfg->stGrpCfg);
		else
			cvi_img_update(i, false, &cfg->stGrpCfg); //use dma share
		cvi_sc_update(i, &cfg->astChnCfg[chn_id]);

		if (pstJob->is_tile) {
			vpss_dev->vpss_cores[i].tile_mode = sclr_tile_cal_size(i,
				(pstJob->cfg.astChnCfg[chn_id].src_size.width >> 1) - 1);
			pstJob->tile_mode |= vpss_dev->vpss_cores[i].tile_mode;
		}

		if (pstJob->is_v_tile) {
			vpss_dev->vpss_cores[i].tile_mode |= (sclr_v_tile_cal_size(i,
				(pstJob->cfg.astChnCfg[chn_id].src_size.height >> 1) - 1)) << SCL_V_TILE_OFFSET;
			pstJob->tile_mode |= vpss_dev->vpss_cores[i].tile_mode;
			if((pstJob->tile_mode & SCL_TILE_BOTH) == SCL_TILE_BOTH)
				pstJob->tile_mode |= (pstJob->tile_mode & SCL_V_TILE_BOTH) << SCL_V_TILE_OFFSET;
		}
	}
	if (pstJob->is_tile) {
		if (pstJob->tile_mode & SCL_TILE_LEFT) {
			pstJob->tile_mode &= ~SCL_TILE_LEFT;
			pstJob->is_work_on_r_tile = false;
			for (i = dev_idx; i < dev_idx_max; i++)
				if (!cvi_img_left_tile_cfg(i))
					atomic_set(&vpss_dev->vpss_cores[i].state, CVI_VIP_END);
		} else if (pstJob->tile_mode & SCL_TILE_RIGHT) {
			pstJob->tile_mode &= ~SCL_TILE_RIGHT;
			pstJob->is_work_on_r_tile = true;
			for (i = dev_idx; i < dev_idx_max; i++)
				if (!cvi_img_right_tile_cfg(i))
					atomic_set(&vpss_dev->vpss_cores[i].state, CVI_VIP_END);
		}
	}

	if (pstJob->is_v_tile) {
		if ((pstJob->tile_mode) & SCL_TILE_TOP) {
			pstJob->tile_mode &= ~(SCL_TILE_TOP);
			for (i = dev_idx; i < dev_idx_max; i++)
				if (!cvi_img_top_tile_cfg(i, false))
					atomic_set(&vpss_dev->vpss_cores[i].state, CVI_VIP_END);
		} else if ((pstJob->tile_mode) & SCL_TILE_DOWN) {
			pstJob->tile_mode &= ~(SCL_TILE_DOWN);
			for (i = dev_idx; i < dev_idx_max; i++)
				if (!cvi_img_down_tile_cfg(i, false))
					atomic_set(&vpss_dev->vpss_cores[i].state, CVI_VIP_END);
		}
	}
	spin_unlock_irqrestore(&vpss_dev->lock, flags);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) job working, chn enable(%d %d %d %d).\n",
		pstJob->grp_id, cfg->chn_enable[0], cfg->chn_enable[1],
		cfg->chn_enable[2], cfg->chn_enable[3]);

	convert_to_bak[dev_idx].enable = false;
	atomic_set(&pstJob->enJobState, CVI_JOB_WORKING);

	cvi_img_start(dev_idx, cfg->chn_num);

	return CVI_SUCCESS;
}

static void vpss_job_finish(struct cvi_vpss_job *job)
{
	u8 i, chn_id;
	u32 u32MaxDuration = 0;
	struct cvi_vpss_device *dev = vpss_dev;

	for (i = 0; i < CVI_VPSS_MAX; i++) {
		if (!(job->vpss_dev_mask & BIT(i)))
			continue;

		if (atomic_read(&dev->vpss_cores[i].state) != CVI_VIP_END)
			return;
	}

	if (atomic_cmpxchg(&job->enJobState, CVI_JOB_WORKING, CVI_JOB_HALF) != CVI_JOB_WORKING)
		return;

	if (convert_to_bak[job->dev_idx_start].enable == true){
		vpss_job_restart(job);
		return;
	}

	if ((job->is_v_tile) && (job->tile_mode & 0xc)) {
		job->tile_mode &= ~(SCL_TILE_DOWN);

		for (i = 0; i < CVI_VPSS_MAX; i++) {
			if (!(job->vpss_dev_mask & BIT(i)))
				continue;
			if (cvi_img_down_tile_cfg(i, (job->tile_mode & SCL_RIGHT_DOWN_TILE_FLAG)))
				atomic_set(&dev->vpss_cores[i].state, CVI_VIP_RUNNING);
			else
				atomic_set(&dev->vpss_cores[i].state, CVI_VIP_END);
		}
		job->tile_mode &= ~SCL_RIGHT_DOWN_TILE_FLAG;
		atomic_set(&job->enJobState, CVI_JOB_WORKING);
		cvi_img_start(job->dev_idx_start, job->cfg.chn_num);
		return;
	}

	if ((job->is_tile) && job->tile_mode) { //right tile
		job->tile_mode &= ~SCL_TILE_RIGHT;
		job->is_work_on_r_tile = true;

		for (i = 0; i < CVI_VPSS_MAX; i++) {
			if (!(job->vpss_dev_mask & BIT(i)))
				continue;
			if (cvi_img_right_tile_cfg(i))
				atomic_set(&dev->vpss_cores[i].state, CVI_VIP_RUNNING);
			else
				atomic_set(&dev->vpss_cores[i].state, CVI_VIP_END);
		}
		if(job->is_v_tile){
			job->tile_mode = job->tile_mode >> SCL_V_TILE_OFFSET;
			if ((job->tile_mode) & SCL_TILE_TOP) {
				job->tile_mode &= ~(SCL_TILE_TOP);
				for (i = 0; i < CVI_VPSS_MAX; i++){
					if (!(job->vpss_dev_mask & BIT(i)))
						continue;
					if (!cvi_img_top_tile_cfg(i, false))
						atomic_set(&dev->vpss_cores[i].state, CVI_VIP_END);
					else
						atomic_set(&dev->vpss_cores[i].state, CVI_VIP_RUNNING);
				}
				if(job->tile_mode)
					job->tile_mode |= SCL_RIGHT_DOWN_TILE_FLAG;
			} else if (job->tile_mode & SCL_TILE_DOWN) {
				job->tile_mode &= ~(SCL_TILE_DOWN);
				for (i = 0; i < CVI_VPSS_MAX; i++){
					if (!(job->vpss_dev_mask & BIT(i)))
						continue;
					if (!cvi_img_down_tile_cfg(i, true))
						atomic_set(&dev->vpss_cores[i].state, CVI_VIP_END);
					else
						atomic_set(&dev->vpss_cores[i].state, CVI_VIP_RUNNING);
				}
			}
		}

		atomic_set(&job->enJobState, CVI_JOB_WORKING);
		cvi_img_start(job->dev_idx_start, job->cfg.chn_num);
		return;
	}

	//job finish
	if ((!job->is_tile) || (!job->is_v_tile) || ((job->is_tile || job->is_v_tile) && !job->tile_mode)) {

		for (i = 0; i < CVI_VPSS_MAX; ++i) {
			if (!(job->vpss_dev_mask & BIT(i)))
				continue;

			u32MaxDuration = dev->vpss_cores[i].u32HwDuration > u32MaxDuration ?
							dev->vpss_cores[i].u32HwDuration : u32MaxDuration;
			// disable clk
			if (dev->vpss_cores[i].clk_vpss)
				clk_disable(dev->vpss_cores[i].clk_vpss);

			if (!job->is_online)
				atomic_set(&dev->vpss_cores[i].state, CVI_VIP_IDLE);
			else
				stTaskCtx.online_status[job->grp_id] = VPSS_ONLINE_READY;

			chn_id = dev->vpss_cores[i].map_chn;
			if (chn_id >= VPSS_MAX_CHN_NUM)
				CVI_TRACE_VPSS(CVI_DBG_ERR, "map_chn err.\n");
			else
				job->checksum[chn_id] = dev->vpss_cores[i].checksum;
		}
		job->u32HwDuration = u32MaxDuration;
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "vpss grp(%d) Hw Duration (%d).\n", job->grp_id, u32MaxDuration);

		atomic_set(&job->enJobState, CVI_JOB_END);
		job->vpss_dev_mask = 0;
		job->pfnJobCB(&job->data);

		cvi_vpss_hal_try_schedule();
	}
}


void cvi_vpss_irq_handler(struct vpss_core *core)
{
	u8 vpss_idx = core->enVpssType;
	struct cvi_vpss_job *job = (struct cvi_vpss_job *)core->job;

	if (!job) {
		CVI_TRACE_VPSS(CVI_DBG_INFO, "vpss(%d), job canceled.\n", vpss_idx);
		atomic_set(&core->state, CVI_VIP_IDLE);
		return;
	}
	if (!(job->vpss_dev_mask & BIT(vpss_idx))) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "vpss_dev_mask err.\n");
		return;
	}

	atomic_set(&core->state, CVI_VIP_END);
	ktime_get_ts64(&core->ts_end);
	core->u32HwDuration = get_diff_in_us(core->ts_start, core->ts_end);
	core->u32HwDurationTotal += core->u32HwDuration;
	core->u32IntCnt++;

	if (job->is_tile) {
		if (job->is_work_on_r_tile)
			core->tile_mode &= ~(SCL_TILE_RIGHT);
		else
			core->tile_mode &= ~(SCL_TILE_LEFT);

		if (!core->tile_mode) { //work finish
			core->job = NULL;
		}
	}

	vpss_job_finish(job);
}

void cvi_vpss_cmdq_irq_handler(struct vpss_core *core)
{
	u8 vpss_idx = core->enVpssType;
	struct vpss_stitch_cfg *cfg = (struct vpss_stitch_cfg *)core->job;
	unsigned long flags;

	if (!cfg) {
		CVI_TRACE_VPSS(CVI_DBG_INFO, "vpss(%d), job NULL.\n", vpss_idx);
		atomic_set(&core->state, CVI_VIP_IDLE);
		return;
	}
	if (core->clk_vpss)
		clk_disable(core->clk_vpss);

	spin_lock_irqsave(&vpss_dev->lock, flags);
	atomic_set(&core->state, CVI_VIP_IDLE);
	ktime_get_ts64(&core->ts_end);
	core->u32HwDuration = get_diff_in_us(core->ts_start, core->ts_end);
	core->u32HwDurationTotal += core->u32HwDuration;
	core->u32IntCnt++;
	core->job = NULL;
	spin_unlock_irqrestore(&vpss_dev->lock, flags);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "vpss(%d) Hw Duration (%d).\n", vpss_idx, core->u32HwDuration);

	cfg->pfnJobCB(cfg->data);
}

