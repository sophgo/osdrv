/*
 * Copyright (C) Cvitek Co., Ltd. 2021-2022. All rights reserved.
 *
 * File Name: cvi_ive_platform.c
 * Description: cvitek ive driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef CVI_IVE_REG
#define CVI_IVE_REG
// include register
//#include "ive_ccl_reg.h"
//#include "ive_dma_reg.h"
//#include "ive_filterop_reg.h"
//#include "ive_gmm_reg.h"
//#include "ive_map_reg.h"
//#include "ive_match_bg_reg.h"
//#include "ive_sad_reg.h"
//#include "ive_top_reg.h"
//#include "ive_update_bg_reg.h"
// include regBuilder
#include "builder/reg_ive_ccl.h"
#include "builder/reg_ive_ccl.struct.h"
#include "builder/reg_ive_dma.h"
#include "builder/reg_ive_dma.struct.h"
#include "builder/reg_ive_filterop.h"
#include "builder/reg_ive_filterop.struct.h"
#include "builder/reg_ive_gmm.h"
#include "builder/reg_ive_gmm.struct.h"
#include "builder/reg_ive_map.h"
#include "builder/reg_ive_map.struct.h"
#include "builder/reg_ive_match_bg.h"
#include "builder/reg_ive_match_bg.struct.h"
#include "builder/reg_ive_sad.h"
#include "builder/reg_ive_sad.struct.h"
#include "builder/reg_ive_top.h"
#include "builder/reg_ive_top.struct.h"
#include "builder/reg_ive_update_bg.h"
#include "builder/reg_ive_update_bg.struct.h"
#include "builder/reg_ive_ncc.h"
#include "builder/reg_ive_ncc.struct.h"
#include "builder/reg_cmdq.h"
#include "builder/reg_cmdq.struct.h"

#include "builder/reg_isp_dma_ctl.h"
#include "builder/reg_isp_dma_ctl.struct.h"
#include "builder/reg_isp_rdma.h"
#include "builder/reg_isp_rdma.struct.h"
#include "builder/reg_isp_wdma.h"
#include "builder/reg_isp_wdma.struct.h"

#include "builder/reg_img_in.h"
#include "builder/reg_img_in.struct.h"
#include "builder/reg_ive_hist.h"
#include "builder/reg_ive_hist.struct.h"
#include "builder/reg_sc_odma.h"
#include "builder/reg_sc_odma.struct.h"
#include "builder/reg_ive_intg.h"
#include "builder/reg_ive_intg.struct.h"

static void cmdq_printk(CMDQ_C *p)
{
	pr_info("cmdq\n");
	pr_info("\tINT_EVENT.reg_cmdq_int = 0x%x\n", p->INT_EVENT.reg_cmdq_int);
	pr_info("\tINT_EVENT.reg_cmdq_end = 0x%x\n", p->INT_EVENT.reg_cmdq_end);
	pr_info("\tINT_EVENT.reg_cmdq_wait = 0x%x\n", p->INT_EVENT.reg_cmdq_wait);
	pr_info("\tINT_EVENT.reg_isp_pslverr = 0x%x\n", p->INT_EVENT.reg_isp_pslverr);
	pr_info("\tINT_EVENT.reg_task_end = 0x%x\n", p->INT_EVENT.reg_task_end);
	pr_info("\tINT_EN.reg_cmdq_int_en = 0x%x\n", p->INT_EN.reg_cmdq_int_en);
	pr_info("\tINT_EN.reg_cmdq_end_en = 0x%x\n", p->INT_EN.reg_cmdq_end_en);
	pr_info("\tINT_EN.reg_cmdq_wait_en = 0x%x\n", p->INT_EN.reg_cmdq_wait_en);
	pr_info("\tINT_EN.reg_isp_pslverr_en = 0x%x\n", p->INT_EN.reg_isp_pslverr_en);
	pr_info("\tINT_EN.reg_task_end_en = 0x%x\n", p->INT_EN.reg_task_end_en);
	pr_info("\tDMA_ADDR.reg_dma_addr_l = 0x%x\n", p->DMA_ADDR_L.reg_dma_addr_l);
	pr_info("\tDMA_CNT.reg_dma_cnt = 0x%x\n", p->DMA_CNT.reg_dma_cnt);
	pr_info("\tDMA_CONFIG.reg_dma_rsv = 0x%x\n", p->DMA_CONFIG.reg_dma_rsv);
	pr_info("\tDMA_CONFIG.reg_adma_en = 0x%x\n", p->DMA_CONFIG.reg_adma_en);
	pr_info("\tDMA_CONFIG.reg_task_en = 0x%x\n", p->DMA_CONFIG.reg_task_en);
	pr_info("\tAXI_CONFIG.reg_max_burst_len = 0x%x\n", p->AXI_CONFIG.reg_max_burst_len);
	pr_info("\tAXI_CONFIG.reg_ot_enable = 0x%x\n", p->AXI_CONFIG.reg_ot_enable);
	pr_info("\tAXI_CONFIG.reg_sw_overwrite = 0x%x\n", p->AXI_CONFIG.reg_sw_overwrite);
	pr_info("\tJOB_CTL.reg_job_start = 0x%x\n", p->JOB_CTL.reg_job_start);
	pr_info("\tJOB_CTL.reg_cmd_restart = 0x%x\n", p->JOB_CTL.reg_cmd_restart);
	pr_info("\tJOB_CTL.reg_restart_hw_mod = 0x%x\n", p->JOB_CTL.reg_restart_hw_mod);
	pr_info("\tJOB_CTL.reg_cmdq_idle_en = 0x%x\n", p->JOB_CTL.reg_cmdq_idle_en);
	pr_info("\tAPB_PARA.reg_base_addr = 0x%x\n", p->APB_PARA.reg_base_addr);
	pr_info("\tAPB_PARA.reg_apb_pprot = 0x%x\n", p->APB_PARA.reg_apb_pprot);
	pr_info("\tDEBUG_BUS0.reg_debus0 = 0x%x\n", p->DEBUG_BUS0.reg_debus0);
	pr_info("\tDEBUG_BUS1.reg_debus1 = 0x%x\n", p->DEBUG_BUS1.reg_debus1);
	pr_info("\tDEBUG_BUS2.reg_debus2 = 0x%x\n", p->DEBUG_BUS2.reg_debus2);
	pr_info("\tDEBUG_BUS3.reg_debus3 = 0x%x\n", p->DEBUG_BUS3.reg_debus3);
	pr_info("\tDEBUG_BUS_SEL.reg_debus_sel = 0x%x\n", p->DEBUG_BUS_SEL.reg_debus_sel);
	pr_info("\tDUMMY.reg_dummy = 0x%x\n", p->DUMMY.reg_dummy);
	pr_info("\tTASK_DONE_STS.reg_task_done = 0x%x\n", p->TASK_DONE_STS.reg_task_done);
	pr_info("\tDMA_ADDR_TSK0.reg_dma_addr_tsk0 = 0x%x\n", p->DMA_ADDR_TSK0.reg_dma_addr_tsk0);
	pr_info("\tDMA_CNT_TSK0.reg_dma_cnt_tsk0 = 0x%x\n", p->DMA_CNT_TSK0.reg_dma_cnt_tsk0);
	pr_info("\tDMA_ADDR_TSK1.reg_dma_addr_tsk1 = 0x%x\n", p->DMA_ADDR_TSK1.reg_dma_addr_tsk1);
	pr_info("\tDMA_CNT_TSK1.reg_dma_cnt_tsk1 = 0x%x\n", p->DMA_CNT_TSK1.reg_dma_cnt_tsk1);
	pr_info("\tDMA_ADDR_TSK2.reg_dma_addr_tsk2 = 0x%x\n", p->DMA_ADDR_TSK2.reg_dma_addr_tsk2);
	pr_info("\tDMA_CNT_TSK2.reg_dma_cnt_tsk2 = 0x%x\n", p->DMA_CNT_TSK2.reg_dma_cnt_tsk2);
	pr_info("\tDMA_ADDR_TSK3.reg_dma_addr_tsk3 = 0x%x\n", p->DMA_ADDR_TSK3.reg_dma_addr_tsk3);
	pr_info("\tDMA_CNT_TSK3.reg_dma_cnt_tsk3 = 0x%x\n", p->DMA_CNT_TSK3.reg_dma_cnt_tsk3);
	pr_info("\tDMA_ADDR_TSK4.reg_dma_addr_tsk4 = 0x%x\n", p->DMA_ADDR_TSK4.reg_dma_addr_tsk4);
	pr_info("\tDMA_CNT_TSK4.reg_dma_cnt_tsk4 = 0x%x\n", p->DMA_CNT_TSK4.reg_dma_cnt_tsk4);
	pr_info("\tDMA_ADDR_TSK5.reg_dma_addr_tsk5 = 0x%x\n", p->DMA_ADDR_TSK5.reg_dma_addr_tsk5);
	pr_info("\tDMA_CNT_TSK5.reg_dma_cnt_tsk5 = 0x%x\n", p->DMA_CNT_TSK5.reg_dma_cnt_tsk5);
	pr_info("\tDMA_ADDR_TSK6.reg_dma_addr_tsk6 = 0x%x\n", p->DMA_ADDR_TSK6.reg_dma_addr_tsk6);
	pr_info("\tDMA_CNT_TSK6.reg_dma_cnt_tsk6 = 0x%x\n", p->DMA_CNT_TSK6.reg_dma_cnt_tsk6);
	pr_info("\tDMA_ADDR_TSK7.reg_dma_addr_tsk7 = 0x%x\n", p->DMA_ADDR_TSK7.reg_dma_addr_tsk7);
	pr_info("\tDMA_CNT_TSK7.reg_dma_cnt_tsk7 = 0x%x\n", p->DMA_CNT_TSK7.reg_dma_cnt_tsk7);
}

#endif
