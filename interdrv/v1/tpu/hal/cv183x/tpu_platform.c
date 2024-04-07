/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: tpu_platform.c
 * Description: tpu hw control driver code
 */

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/streamline_annotate.h>
#include <linux/clk.h>
#include <linux/version.h>
#include <asm/cacheflush.h>

#include "cvi_tpu_interface.h"
#include "reg_tiu.h"
#include "reg_tdma.h"
#include "tpu_platform.h"
#include "tpu_pmu.h"

//for security usage
#include "tee_cv_private.h"


#define TPU_DMABUF_HEADER_M		0xB5B5

typedef struct {
	uint32_t vld;
	uint32_t compress_en;
	uint32_t eod;
	uint32_t intp_en;
	uint32_t bar_en;
	uint32_t check_bf16_value;
	uint32_t trans_dir;
	uint32_t rsv00;
	uint32_t trans_fmt;
	uint32_t transpose_md;
	uint32_t rsv01;
	uint32_t outstanding_en;
	uint32_t cmd_id;
	uint32_t spec_func;
	uint32_t dst_fmt;
	uint32_t src_fmt;
	uint32_t cmprs_fmt;
	uint32_t sys_dtype;
	uint32_t rsv2_1;
	uint32_t int8_sign;
	uint32_t compress_zero_guard;
	uint32_t int8_rnd_mode;
	uint32_t wait_id_tpu;
	uint32_t wait_id_other_tdma;
	uint32_t wait_id_sdma;
	uint32_t const_val;
	uint32_t src_base_reg_sel;
	uint32_t mv_lut_idx;
	uint32_t dst_base_reg_sel;
	uint32_t mv_lut_base;
	uint32_t rsv4_5;
	uint32_t dst_h_stride;
	uint32_t dst_c_stride_low;
	uint32_t dst_n_stride;
	uint32_t src_h_stride;
	uint32_t src_c_stride_low;
	uint32_t src_n_stride;
	uint32_t dst_c;
	uint32_t src_c;
	uint32_t dst_w;
	uint32_t dst_h;
	uint32_t src_w;
	uint32_t src_h;
	uint32_t dst_base_addr_low;
	uint32_t src_base_addr_low;
	uint32_t src_n;
	uint32_t dst_base_addr_high;
	uint32_t src_base_addr_high;
	uint32_t src_c_stride_high;
	uint32_t dst_c_stride_high;
	uint32_t compress_bias0;
	uint32_t compress_bias1;
	uint32_t layer_ID;
} tdma_reg_t;

#if 1	//default enable ANNOTATE function
#define PLATTAG_CHANNEL_COLOR	ANNOTATE_CHANNEL_COLOR
#define PLATTAG_CHANNEL_END		ANNOTATE_CHANNEL_END
#define PLATTAG_NAME_CHANNEL	ANNOTATE_NAME_CHANNEL
#else
#define PLATTAG_CHANNEL_COLOR(...)
#define PLATTAG_CHANNEL_END(...)
#define PLATTAG_NAME_CHANNEL(...)
#endif

#define TIMEOUT_MS			(60 * 1000)

#define TDMA_MASK_INIT	0x20	//ignore descriptor nchw/stride=0 error
#define TDMA_INT_EOD		0x1
#define TDMA_INT_EOPMU	0x8000
#define TDMA_ALL_IDLE		0X1F

#define TEE_FIREWALL_MAX 6
static uint64_t tee_firewall_addr[TEE_FIREWALL_MAX] = {0};

void platform_clear_int(struct cvi_tpu_device *ndev)
{
	u32 reg_value, int_status;

	//get interrupt status
	reg_value = RAW_READ32(ndev->tdma_vaddr + TDMA_INT_MASK);
	int_status = (reg_value >> 16) & ~(TDMA_MASK_INIT);
	pr_debug("platform_tdma_irq()=0x%x int_status=0x%x\n", reg_value, int_status);

	if (int_status != TDMA_INT_EOD && int_status != TDMA_INT_EOPMU) {
		pr_err("platform_tdma_irq() got error = 0x%x\n", reg_value);
		pr_err("TDMA_SYNC_STATUS reg=0x%x\n", RAW_READ32(ndev->tdma_vaddr + TDMA_SYNC_STATUS));
	}

	RAW_WRITE32(ndev->tdma_vaddr + TDMA_INT_MASK, 0xFFFF0000);
	pr_debug("platform_clear_int() done\n");
}

irqreturn_t platform_tdma_irq(struct cvi_tpu_device *ndev)
{
	pr_debug("got platform_tdma_irq() callback\n");
	complete(&ndev->tdma_done);
	return IRQ_HANDLED;
}

static void set_tdma_descriptor_Fire(struct TPU_PLATFORM_CFG *pCfg, u64 desc_offset,
				     u32 num_tdma)
{
	u8 *iomem_tdmaBase = pCfg->iomem_tdmaBase;

	//set TDMA descriptor address
	RAW_WRITE32(iomem_tdmaBase + TDMA_DES_BASE, desc_offset);

	//make sure that debug mode is disable
	RAW_WRITE32(iomem_tdmaBase + TDMA_DEBUG_MODE, 0x0);

	//tdma dcm enable
	RAW_WRITE32(iomem_tdmaBase + TDMA_DCM_DISABLE, 0x0);

	//init interrupt mask
	RAW_WRITE32(iomem_tdmaBase + TDMA_INT_MASK, TDMA_MASK_INIT);

	RAW_WRITE32(iomem_tdmaBase + TDMA_CTRL,
			(0x1 << TDMA_CTRL_ENABLE_BIT) |
			(0x1 << TDMA_CTRL_MODESEL_BIT) |
			(num_tdma << TDMA_CTRL_DESNUM_BIT) |
			(0x3 << TDMA_CTRL_BURSTLEN_BIT) |
//		(0x1 << TDMA_CTRL_FORCE_1ARRAY) |	//set 1 array
			(0x1 << TDMA_CTRL_64BYTE_ALIGN_EN));
}

static void set_tiu_descriptor(struct TPU_PLATFORM_CFG *pCfg, u64 desc_offset,
			       u32 num_bd)
{
	u32 regVal = 0;
	u64 desc_addr = 0;
	u8 *iomem_tiuBase = pCfg->iomem_tiuBase;

	desc_addr = (u64)(desc_offset << BDC_ENGINE_CMD_ALIGNED_BIT);

	//set TIU descriptor address
	RAW_WRITE32(iomem_tiuBase + BD_CTRL_BASE_ADDR + 0x4, desc_addr & 0xFFFFFFFF);
	regVal = RAW_READ32(iomem_tiuBase + BD_CTRL_BASE_ADDR + 0x8);
	RAW_WRITE32(iomem_tiuBase + BD_CTRL_BASE_ADDR + 0x8, (regVal & 0xFFFFFF00) | (desc_addr >> 32 & 0xFF));

	//enable TIU dcm
	regVal = RAW_READ32(iomem_tiuBase + BD_CTRL_BASE_ADDR + 0xC);
	RAW_WRITE32(iomem_tiuBase + BD_CTRL_BASE_ADDR + 0xC, (regVal & ~0x1E));

	//set 1 array, lane=8
//regVal = RAW_READ32(iomem_tiuBase + BD_CTRL_BASE_ADDR);
//regVal &= ~0x3FC00000;
//RAW_WRITE32(iomem_tiuBase + BD_CTRL_BASE_ADDR, regVal | (3 << 22));

	//fire TIU
	regVal = RAW_READ32(iomem_tiuBase + BD_CTRL_BASE_ADDR);
	RAW_WRITE32(iomem_tiuBase + BD_CTRL_BASE_ADDR, regVal | (0x1 << BD_DES_ADDR_VLD) | (0x1 << BD_INTR_ENABLE));
}

static void resync_cmd_id(struct TPU_PLATFORM_CFG *pCfg)
{
	u32 regVal = 0;
	u8 *iomem_tiuBase = pCfg->iomem_tiuBase;
	u8 *iomem_tdmaBase = pCfg->iomem_tdmaBase;

	//reset TIU ID
	regVal = RAW_READ32(iomem_tiuBase + BD_CTRL_BASE_ADDR + 0xC);
	RAW_WRITE32(iomem_tiuBase + BD_CTRL_BASE_ADDR + 0xC, (regVal | 0x1));
	RAW_WRITE32(iomem_tiuBase + BD_CTRL_BASE_ADDR + 0xC, (regVal & ~0x1));

	//reset TIU interrupt status
	regVal = RAW_READ32(iomem_tiuBase + BD_CTRL_BASE_ADDR);
	RAW_WRITE32(iomem_tiuBase + BD_CTRL_BASE_ADDR, (regVal | (0x1 << 1)));

	//reset DMA ID
	RAW_WRITE32(iomem_tdmaBase + TDMA_CTRL, (0x1 << TDMA_CTRL_RESET_SYNCID_BIT));
	RAW_WRITE32(iomem_tdmaBase + TDMA_CTRL, 0x0);

	//reset DMA interrupt status
	RAW_WRITE32(iomem_tdmaBase + TDMA_INT_MASK, 0xFFFF0000);
}

static int poll_cmdbuf_done(struct TPU_PLATFORM_CFG *pCfg, struct CMD_ID_NODE *id_node)
{
	u32 regVal = 0, regVal2 = 0;
	u8 *iomem_tdmaBase = pCfg->iomem_tdmaBase;
	u8 *iomem_tiuBase = pCfg->iomem_tiuBase;
	unsigned long expire = jiffies + msecs_to_jiffies(TIMEOUT_MS);

	if (id_node->tdma_cmd_id > 0) {
		regVal = RAW_READ32(iomem_tdmaBase + TDMA_INT_MASK);
		regVal2 = RAW_READ32(iomem_tdmaBase + TDMA_SYNC_STATUS);

		pr_debug("regVal=0x%x, regVal2=0x%x, tdma_cmd_id=0x%x\n"
					, regVal, regVal2, id_node->tdma_cmd_id);

		if ((regVal2 >> 16) < id_node->tdma_cmd_id) {
			pr_err("got tdma int but tdma id not the last one, maybe not finished\n");
			return -1;
		}
	}

	if (id_node->bd_cmd_id > 0) {

		//pool until bd done
		while (1) {
			regVal = RAW_READ32(iomem_tiuBase + BD_CTRL_BASE_ADDR);

			if ((((regVal >> 6) & 0xFFFF) >= id_node->bd_cmd_id) && ((regVal & (0x1 << 1)) != 0)) {
				RAW_WRITE32(iomem_tiuBase + BD_CTRL_BASE_ADDR,	 regVal | (0x1 << 1));
				break;
			}

			cpu_relax();

			if (time_after(jiffies, expire)) {
				pr_err("poll tiu timeout, BD_CTRL_BASE_ADDR=0x%x\n", regVal);
				return -ETIMEDOUT;
			}
		}
	}

	return 0;
}

static void platform_run_dmabuf_setArrayBase(struct TPU_PLATFORM_CFG *pCfg, struct dma_hdr_t *header)
{
	uint8_t *iomem_tdmaBase = pCfg->iomem_tdmaBase;

	pr_debug("base0L=0x%x, base1L=0x%x, base2L=0x%x, base3L=0x%x, base0H=0x%x, base1H=0x%x, base2H=0x%x, base3H=0x%x\n",
							header->arraybase_0_L, header->arraybase_1_L,
							header->arraybase_2_L, header->arraybase_3_L,
							header->arraybase_0_H, header->arraybase_1_H,
							header->arraybase_2_H, header->arraybase_3_H);

	pr_debug("base4L=0x%x, base5L=0x%x, base6L=0x%x, base7L=0x%x, base4H=0x%x, base5H=0x%x, base6H=0x%x, base7H=0x%x\n",
							header->arraybase_4_L, header->arraybase_5_L,
							header->arraybase_6_L, header->arraybase_7_L,
							header->arraybase_4_H, header->arraybase_5_H,
							header->arraybase_6_H, header->arraybase_7_H);

	RAW_WRITE32(iomem_tdmaBase + TDMA_ARRAYBASE0_L, header->arraybase_0_L);
	RAW_WRITE32(iomem_tdmaBase + TDMA_ARRAYBASE1_L, header->arraybase_1_L);
	RAW_WRITE32(iomem_tdmaBase + TDMA_ARRAYBASE2_L, header->arraybase_2_L);
	RAW_WRITE32(iomem_tdmaBase + TDMA_ARRAYBASE3_L, header->arraybase_3_L);
	RAW_WRITE32(iomem_tdmaBase + TDMA_ARRAYBASE4_L, header->arraybase_4_L);
	RAW_WRITE32(iomem_tdmaBase + TDMA_ARRAYBASE5_L, header->arraybase_5_L);
	RAW_WRITE32(iomem_tdmaBase + TDMA_ARRAYBASE6_L, header->arraybase_6_L);
	RAW_WRITE32(iomem_tdmaBase + TDMA_ARRAYBASE7_L, header->arraybase_7_L);

	//assume high bit always 0
	RAW_WRITE32(iomem_tdmaBase + TDMA_ARRAYBASE0_H, 0);
	RAW_WRITE32(iomem_tdmaBase + TDMA_ARRAYBASE1_H, 0);
}

int platform_run_dmabuf(struct cvi_tpu_device *ndev, void *dmabuf_v, uint64_t dmabuf_p)
{
	int i = 0, ret = -1;
	u8 u8pmu_enable = 0;

	struct dma_hdr_t *header = (struct dma_hdr_t *)dmabuf_v;
	struct cvi_cpu_sync_desc_t *desc = (struct cvi_cpu_sync_desc_t *)(dmabuf_v + sizeof(struct dma_hdr_t));

	struct TPU_PLATFORM_CFG cfg;
	struct CMD_ID_NODE id_node = {0};

	pr_debug("dmabuf_v=0x%p, dmabuf_p=0x%llx\n", dmabuf_v, dmabuf_p);

	if (header->dmabuf_magic_m != TPU_DMABUF_HEADER_M) {
		pr_err("err dmabuf\n");
		return -1;
	}

	//the first part tag point
	PLATTAG_CHANNEL_COLOR(1, ANNOTATE_GREEN, "tpu_SW_pre");

	//assign iomem base related
	cfg.iomem_tdmaBase = ndev->tdma_vaddr;
	cfg.iomem_tiuBase = ndev->tiu_vaddr;
	cfg.pmubuf_addr_p = dmabuf_p + (uint64_t)(header->pmubuf_offset);
	cfg.pmubuf_size = header->pmubuf_size;

	pr_debug("iomem_tdmaBase=%p, iomem_tiuBase=%p, pmubuf_addr_p=%llx\n",
		cfg.iomem_tdmaBase, cfg.iomem_tiuBase, cfg.pmubuf_addr_p);

	platform_run_dmabuf_setArrayBase(&cfg, header);

	//check if enable pmu
	if ((header->pmubuf_offset) && (header->pmubuf_size))
		u8pmu_enable = 1;

	if (u8pmu_enable)
		TPUPMU_Enable(&cfg, 1, TPU_PMUEVENT_TDMABW);

	for (i = 0; i < header->cpu_desc_count; i++, desc++) {
		int bd_num = desc->num_bd & 0xFFFF;
		int tdma_num = desc->num_gdma & 0xFFFF;
		u32 bd_offset = desc->offset_bd;
		u32 tdma_offset = desc->offset_gdma;

		reinit_completion(&ndev->tdma_done);
		resync_cmd_id(&cfg);

		id_node.bd_cmd_id = bd_num;
		id_node.tdma_cmd_id = tdma_num;

		pr_debug("num <bd: %d, gdma: %d>, offset <0x%08x, 0x%08x>\n", bd_num, tdma_num, bd_offset, tdma_offset);

		if (bd_num > 0)
			set_tiu_descriptor(&cfg, bd_offset, bd_num);

		if (tdma_num > 0)
			set_tdma_descriptor_Fire(&cfg, tdma_offset, tdma_num);

		//the second part tag point
		PLATTAG_CHANNEL_END(1);
		PLATTAG_CHANNEL_COLOR(2, ANNOTATE_GREEN, "tpu_HW");

		/////////////////wait
		if (tdma_num > 0) {
			ret = wait_for_completion_interruptible_timeout(
				&ndev->tdma_done, msecs_to_jiffies(TIMEOUT_MS));

			//clear int, tdma int must be edge_triiger, for security compatible support
			platform_clear_int(ndev);

			if (ret == 0) {
				dev_err(ndev->dev, "run dmabuf timeout\n");
				return -ETIMEDOUT;
			}

			if (ret < 0) {
				dev_err(ndev->dev, "run dmabuf interrupted\n");
				return ret;
			}
		}

		//the third part tag point
		PLATTAG_CHANNEL_END(2);
		PLATTAG_CHANNEL_COLOR(1, ANNOTATE_GREEN, "tpu_SW_post");

		//check tdma/tiu current descriptor
		ret = poll_cmdbuf_done(&cfg, &id_node);
		if (ret < 0) {
			dev_err(ndev->dev, "pool dmabuf timeout\n");
			return -ETIMEDOUT;
		}

	}

	//disable PMU
	if (u8pmu_enable) {
		reinit_completion(&ndev->tdma_done);

		TPUPMU_Enable(&cfg, 0, TPU_PMUEVENT_TDMABW);

		ret = wait_for_completion_interruptible_timeout(
			&ndev->tdma_done, msecs_to_jiffies(TIMEOUT_MS));

		//clear int, tdma int must be edge_triiger, for security compatible support
		platform_clear_int(ndev);

		if (ret == 0) {
			dev_err(ndev->dev, "stop pmu timeout\n");
			return -ETIMEDOUT;
		}

		if (ret < 0) {
			dev_err(ndev->dev, "stop pmu interrupted\n");
			return ret;
		}

		//parsing pmubuf content, depracated way, move to user space
		//TPUPMU_ParsingResult((u8 *)((u64)dmabuf_v + (u64)(header->pmubuf_offset)));
	}

	if (ndev->clk_tpu_axi) {
		header->tpu_clk_rate = clk_get_rate(ndev->clk_tpu_axi);
		//need not flush, because we have cvi_tpu_cleanup_buffer() unmap
		//__dma_map_area(phys_to_virt((uint64_t)dmabuf_p), sizeof(struct dma_hdr_t), DMA_TO_DEVICE);
	}

	PLATTAG_CHANNEL_END(1);
	PLATTAG_NAME_CHANNEL(1, 1, "tpu_SW");
	PLATTAG_NAME_CHANNEL(2, 1, "tpu_HW");
	return 0;
}

int platform_loadcmdbuf_tee(struct cvi_tpu_device *ndev, struct tpu_tee_load_info *p_info)
{
	struct arm_smccc_res retParam = {0};
	int i = 0, firewall_hit = 0;
	bool decrypt_cmd = false, decrypt_weight = false;

	for (i = 0; i < TEE_FIREWALL_MAX; i++) {
		if (tee_firewall_addr[i] == p_info->weight_addr_ree) {
			firewall_hit = 1;
			break;
		}
	}

	if (!firewall_hit) {
		for (i = 0; i < TEE_FIREWALL_MAX; i++) {
			if (!tee_firewall_addr[i]) {
				tee_firewall_addr[i] = p_info->weight_addr_ree;
				firewall_hit = 1;
				decrypt_weight = true;
				tee_cv_aimodel_set_prot_range(p_info->weight_addr_ree,
					p_info->weight_len_ree, &retParam);
				if (retParam.a0) {
					pr_err("err tee_cv_aimodel_set_prot_range()\n");
					return -1;
				}
				break;
			}
		}
	}

	if (!firewall_hit) {
		pr_err("tee only supported %d firewall, not enough\n", TEE_FIREWALL_MAX);
		return -1;
	}

	//assume we always need decrypt cmdbuf
	decrypt_cmd = true;
	tee_cv_aimodel_load(p_info->cmdbuf_addr_ree, p_info->cmdbuf_len_ree,
			p_info->weight_addr_ree, p_info->weight_len_ree,
			p_info->neuron_addr_ree, decrypt_cmd,
			decrypt_weight, &retParam);

	if (retParam.a0) {
		pr_err("err tee_cv_aimodel_load()\n");
		return -1;
	}

	//got secure dma buffer address
	p_info->dmabuf_addr_tee = retParam.a1;
	pr_debug("platform_loadcmdbuf_tee() dmabuf_addr_tee=0x%llx\n", p_info->dmabuf_addr_tee);
	return 0;
}

int platform_run_dmabuf_tee(struct cvi_tpu_device *ndev, struct tpu_tee_submit_info *p_info)
{
	int ret = -1, next_loop = 0;
	struct arm_smccc_res retParam = {0};

	do {
		pr_debug("platform_run_dmabuf_tee() dmabuf_paddr=0x%llx\n", p_info->dmabuf_paddr);

		reinit_completion(&ndev->tdma_done);

		//running dma buffer
		tee_cv_aimodel_run((uint64_t)p_info->dmabuf_paddr, (uint64_t)p_info->gaddr_base2,
			(uint64_t)p_info->gaddr_base3, (uint64_t)p_info->gaddr_base4,
			(uint64_t)p_info->gaddr_base5, (uint64_t)p_info->gaddr_base6,
			(uint64_t)p_info->gaddr_base7, &retParam);

		if (retParam.a0) {
			pr_err("platform_run_dmabuf_tee() err dmabuf\n");
			return -1;
		}

		//chcecking if next loop
		next_loop = retParam.a3;
		pr_debug("next_loop=%d\n", next_loop);

		//checking tdma interrupt
		if (retParam.a1) {
			ret = wait_for_completion_interruptible_timeout(
				&ndev->tdma_done, msecs_to_jiffies(TIMEOUT_MS));
			if (ret == 0) {
				dev_err(ndev->dev, "run dmabuf timeout\n");
				return -ETIMEDOUT;
			}

			if (ret < 0) {
				dev_err(ndev->dev, "run dmabuf interrupted\n");
				return ret;
			}
		}

		//checking if dmabuf finished
		if (retParam.a1 || retParam.a2) {
			tee_cv_aimodel_wait((uint64_t)p_info->dmabuf_paddr, &retParam);

			if (retParam.a0) {
				dev_err(ndev->dev, "pool dmabuf timeout\n");
				return -ETIMEDOUT;
			}
		}
	} while (next_loop);
	return 0;

}

int platform_unload_tee(struct cvi_tpu_device *ndev, uint64_t paddr, uint64_t size)
{
	struct arm_smccc_res retParam = {0};
	int i = 0, firewall_hit = 0;

	pr_debug("platform_unload_tee() addr=0x%llx, size=0x%llx\n", paddr, size);

	for (i = 0; i < TEE_FIREWALL_MAX; i++) {
		if (tee_firewall_addr[i] == paddr) {
			firewall_hit = 1;
			break;
		}
	}

	if (!firewall_hit) {
		pr_err("platform_unload_tee() compare failed\n");
		return -1;
	}

	tee_cv_aimodel_clear_prot_range(paddr, size, &retParam);
	if (retParam.a0) {
		pr_err("err platform_unload_tee()\n");
		return -1;
	}

	tee_firewall_addr[i] = 0;
	return 0;
}

int platform_tpu_suspend(struct cvi_tpu_device *ndev)
{
	return 0;
}

int platform_tpu_resume(struct cvi_tpu_device *ndev)
{
	return 0;
}

int platform_tpu_open(struct cvi_tpu_device *ndev)
{
	return 0;
}

int platform_tpu_reset(struct cvi_tpu_device *ndev)
{
	if (ndev->rst_tdma && ndev->rst_tpu && ndev->rst_tpusys) {
		pr_debug("tpu reset()\n");
		reset_control_assert(ndev->rst_tdma);
		reset_control_assert(ndev->rst_tpu);
		reset_control_assert(ndev->rst_tpusys);

		reset_control_deassert(ndev->rst_tdma);
		reset_control_deassert(ndev->rst_tpu);
		reset_control_deassert(ndev->rst_tpusys);
	}
	return 0;
}

int platform_tpu_init(struct cvi_tpu_device *ndev)
{
	//enable clock
	if (ndev->clk_tpu_axi && ndev->clk_tpu_fab) {
		pr_debug("tpu enable clock()\n");
		clk_prepare_enable(ndev->clk_tpu_axi);
		clk_prepare_enable(ndev->clk_tpu_fab);
	}

	//reset
	platform_tpu_reset(ndev);
	return 0;
}

void platform_tpu_deinit(struct cvi_tpu_device *ndev)
{
	//disable clock
	if (ndev->clk_tpu_axi && ndev->clk_tpu_fab) {
		pr_debug("tpu disable clock()\n");
		clk_disable_unprepare(ndev->clk_tpu_axi);
		clk_disable_unprepare(ndev->clk_tpu_fab);
	}
}

static void emit_tdma_reg(const tdma_reg_t *r, uint32_t *_p)
{
	uint32_t *p = _p;

	p[15] = (r->compress_bias0 & ((1u << 8) - 1)) |
		((r->compress_bias1 & ((1u << 8) - 1)) << 8) |
		((r->layer_ID & ((1u << 16) - 1)) << 16);
	p[14] = (r->src_c_stride_high & ((1u << 16) - 1)) |
		((r->dst_c_stride_high & ((1u << 16) - 1)) << 16);
	p[13] = (r->src_n & ((1u << 16) - 1)) |
		((r->dst_base_addr_high & ((1u << 8) - 1)) << 16) |
		((r->src_base_addr_high & ((1u << 8) - 1)) << 24);
	p[12] = (r->src_base_addr_low & (((uint64_t)1 << 32) - 1));
	p[11] = (r->dst_base_addr_low & (((uint64_t)1 << 32) - 1));
	p[10] = (r->src_w & ((1u << 16) - 1)) |
		((r->src_h & ((1u << 16) - 1)) << 16);
	p[9] = (r->dst_w & ((1u << 16) - 1)) |
		((r->dst_h & ((1u << 16) - 1)) << 16);
	p[8] = (r->dst_c & ((1u << 16) - 1)) |
		((r->src_c & ((1u << 16) - 1)) << 16);
	p[7] = (r->src_n_stride & (((uint64_t)1 << 32) - 1));
	p[6] = (r->src_h_stride & ((1u << 16) - 1)) |
		((r->src_c_stride_low & ((1u << 16) - 1)) << 16);
	p[5] = (r->dst_n_stride & (((uint64_t)1 << 32) - 1));
	p[4] = (r->dst_h_stride & ((1u << 16) - 1)) |
		((r->dst_c_stride_low & ((1u << 16) - 1)) << 16);
	p[3] = (r->const_val & ((1u << 16) - 1)) |
		((r->src_base_reg_sel & ((1u << 3) - 1)) << 16) |
		((r->mv_lut_idx & 1) << 19) |
		((r->dst_base_reg_sel & ((1u << 3) - 1)) << 20) |
		((r->mv_lut_base & 1) << 23) |
		((r->rsv4_5 & ((1u << 8) - 1)) << 24);
	p[2] = (r->wait_id_other_tdma & ((1u << 16) - 1)) |
		((r->wait_id_sdma & ((1u << 16) - 1)) << 16);
	p[1] = (r->spec_func & ((1u << 3) - 1)) |
		((r->dst_fmt & ((1u << 2) - 1)) << 3) |
		((r->src_fmt & ((1u << 2) - 1)) << 5) |
		((r->cmprs_fmt & 1) << 7) |
		((r->sys_dtype & 1) << 8) |
		((r->rsv2_1 & ((1u << 4) - 1)) << 9) |
		((r->int8_sign & 1) << 13) |
		((r->compress_zero_guard & 1) << 14) |
		((r->int8_rnd_mode & 1) << 15) |
		((r->wait_id_tpu & ((1u << 16) - 1)) << 16);
	p[0] = (r->vld & 1) |
		((r->compress_en & 1) << 1) |
		((r->eod & 1) << 2) |
		((r->intp_en & 1) << 3) |
		((r->bar_en & 1) << 4) |
		((r->check_bf16_value & 1) << 5) |
		((r->trans_dir & ((1u << 2) - 1)) << 6) |
		((r->rsv00 & ((1u << 2) - 1)) << 8) |
		((r->trans_fmt & 1) << 10) |
		((r->transpose_md & ((1u << 2) - 1)) << 11) |
		((r->rsv01 & ((1u << 2) - 1)) << 13) |
		((r->outstanding_en & 1) << 15) |
		((r->cmd_id & ((1u << 16) - 1)) << 16);
}

static void reset_tdma_reg(tdma_reg_t *r)
{
	r->vld = 0x0;
	r->compress_en = 0x0;
	r->eod = 0x0;
	r->intp_en = 0x0;
	r->bar_en = 0x0;
	r->check_bf16_value = 0x0;
	r->trans_dir = 0x0;
	r->rsv00 = 0x0;
	r->trans_fmt = 0x0;
	r->transpose_md = 0x0;
	r->rsv01 = 0x0;
	r->outstanding_en = 0x0;
	r->cmd_id = 0x0;
	r->spec_func = 0x0;
	r->dst_fmt = 0x1;
	r->src_fmt = 0x1;
	r->cmprs_fmt = 0x0;
	r->sys_dtype = 0x0;
	r->rsv2_1 = 0x0;
	r->int8_sign = 0x0;
	r->compress_zero_guard = 0x0;
	r->int8_rnd_mode = 0x0;
	r->wait_id_tpu = 0x0;
	r->wait_id_other_tdma = 0x0;
	r->wait_id_sdma = 0x0;
	r->const_val = 0x0;
	r->src_base_reg_sel = 0x0;
	r->mv_lut_idx = 0x0;
	r->dst_base_reg_sel = 0x0;
	r->mv_lut_base = 0x0;
	r->rsv4_5 = 0x0;
	r->dst_h_stride = 0x1;
	r->dst_c_stride_low = 0x1;
	r->dst_n_stride = 0x1;
	r->src_h_stride = 0x1;
	r->src_c_stride_low = 0x1;
	r->src_n_stride = 0x1;
	r->dst_c = 0x1;
	r->src_c = 0x1;
	r->dst_w = 0x1;
	r->dst_h = 0x1;
	r->src_w = 0x1;
	r->src_h = 0x1;
	r->dst_base_addr_low = 0x0;
	r->src_base_addr_low = 0x0;
	r->src_n = 0x1;
	r->dst_base_addr_high = 0x0;
	r->src_base_addr_high = 0x0;
	r->src_c_stride_high = 0x0;
	r->dst_c_stride_high = 0x0;
	r->compress_bias0 = 0x0;
	r->compress_bias1 = 0x0;
	r->layer_ID = 0x0;
}

static void set_tdma_pio(struct cvi_tpu_device *ndev, uint32_t *pio_array)
{
	uint32_t i = 0;
	u8 *iomem_tdmaBase = ndev->tdma_vaddr;

	//reset DMA ID
	RAW_WRITE32(iomem_tdmaBase + TDMA_CTRL, 0x1 << TDMA_CTRL_RESET_SYNCID_BIT);
	RAW_WRITE32(iomem_tdmaBase + TDMA_CTRL, 0x0);

	//reset DMA interrupt status
	RAW_WRITE32(iomem_tdmaBase + TDMA_INT_MASK, 0xFFFF0000);

	for (i = 0; i < 16; i++) {
		//pr_debug("addr=0x%px, value=0x%x\n", iomem_tdmaBase + TDMA_CMD_ACCP0 + (i << 2), pio_array[i]);
		RAW_WRITE32(iomem_tdmaBase + TDMA_CMD_ACCP0 + (i << 2), pio_array[i]);
	}

	//make sure that debug mode is disable
	RAW_WRITE32(iomem_tdmaBase + TDMA_DEBUG_MODE, 0x0);

	//tdma dcm enable
	RAW_WRITE32(iomem_tdmaBase + TDMA_DCM_DISABLE, 0x0);

	//init interrupt mask
	RAW_WRITE32(iomem_tdmaBase + TDMA_INT_MASK, TDMA_MASK_INIT);

	RAW_WRITE32(iomem_tdmaBase + TDMA_CTRL,
			(0x1 << TDMA_CTRL_ENABLE_BIT) |
//		(0x1 << TDMA_CTRL_MODESEL_BIT) |
//		(0x1 << TDMA_CTRL_DESNUM_BIT) |
			(0x3 << TDMA_CTRL_BURSTLEN_BIT) |
//		(0x1 << TDMA_CTRL_FORCE_1ARRAY) | //set 1 array
			(0x1 << TDMA_CTRL_64BYTE_ALIGN_EN));

#if 0
	for (i = 0; i < 2; i++) {
		pr_debug("addr=0x%px, value=0x%x\n",
			iomem_tdmaBase + TDMA_CTRL + (i << 2),
			RAW_READ32(iomem_tdmaBase + TDMA_CTRL + (i << 2)));
	}
#endif
}

int platform_run_pio(struct cvi_tpu_device *ndev, struct tpu_tdma_pio_info *info)
{
	tdma_reg_t reg;
	uint32_t pio_array[16] = {0};
	int32_t ret = -1;
	unsigned long tdma_expire = jiffies + msecs_to_jiffies(TIMEOUT_MS);
	uint32_t reg_value = 0;

	reset_tdma_reg(&reg);
	reinit_completion(&ndev->tdma_done);

	reg.vld = 1;
	reg.trans_dir = 2; // 0:tg2l, 1:l2tg, 2:g2g, 3:l2l
	reg.src_base_addr_low = (uint32_t)(info->paddr_src);
	reg.src_base_addr_high = (info->paddr_src) >> 32;
	reg.dst_base_addr_low = (uint32_t)(info->paddr_dst);
	reg.dst_base_addr_high = (info->paddr_dst) >> 32;
	reg.eod = 1;
	reg.intp_en = 1;

	if (info->enable_2d) {
		reg.trans_fmt = 0; // 0:tensor, 1:common
	  reg.src_n = 1;
	  reg.src_c = 1;
	  reg.src_h = info->h;
	  reg.src_w = info->w_bytes;

	  reg.dst_c = 1;
	  reg.dst_h = reg.src_h;
	  reg.dst_w = reg.src_w;

	  reg.src_n_stride = info->stride_bytes_src * info->h;
	  reg.src_h_stride = info->stride_bytes_src;

	  reg.dst_n_stride = info->stride_bytes_dst * info->h;
	  reg.dst_h_stride = info->stride_bytes_dst;
	} else {
		reg.trans_fmt = 1; // 0:tensor, 1:common
		reg.src_n_stride = info->leng_bytes;
	}
	emit_tdma_reg(&reg, pio_array);
	set_tdma_pio(ndev, pio_array);

	ret = wait_for_completion_interruptible_timeout(
		&ndev->tdma_done, msecs_to_jiffies(TIMEOUT_MS));

	//clear int, tdma int must be edge_triiger, for security compatible support
	platform_clear_int(ndev);

	//PIO MODE tdma_done INT is not supported, have to polling tdma_idle
	while (1) {
		reg_value = RAW_READ32(ndev->tdma_vaddr + TDMA_STATUS);

		if (reg_value == TDMA_ALL_IDLE)
			break;

		cpu_relax();
		if (time_after(jiffies, tdma_expire)) {
			pr_err("platform_run_pio() timeout\n");
			return -ETIMEDOUT;
		}
	}

	if (ret == 0) {
		pr_err("run dmabuf timeout\n");
		return -ETIMEDOUT;
	}

	if (ret < 0) {
		pr_err("run dmabuf interrupted\n");
		return ret;
	}
	return 0;
}

void platform_tpu_spll_divide(struct cvi_tpu_device *ndev, u32 div)
{
	pr_err("not support platform_tpu_spll_divide()\n");
}

#define CV1835TOP								0x03000000
#define TPU_SRAM_LP_CTRL_0			0x1C0
#define REG_TPU_SRAM_SD_AUTO		BIT(3)
#define REG_TDMA_SRAM_SD_AUTO		BIT(7)

int platform_tpu_probe_setting(void)
{
	void __iomem *tpreg;
	unsigned int val;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	tpreg = ioremap(CV1835TOP, PAGE_SIZE);
#else
	tpreg = ioremap_nocache(CV1835TOP, PAGE_SIZE);
#endif
	if (IS_ERR(tpreg))
		return PTR_ERR(tpreg);

	val = readl((tpreg + TPU_SRAM_LP_CTRL_0));
	val |= (REG_TPU_SRAM_SD_AUTO | REG_TDMA_SRAM_SD_AUTO);
	writel(val, (tpreg + TPU_SRAM_LP_CTRL_0));
	iounmap(tpreg);
	return 0;
}

