#include "osal_def.h"
#include "vi_dump_register.h"
#include "vi_defines.h"
#include "vi_reg.h"

static struct isp_dump_info m_block[ISP_BLK_ID_MAX] = {0};

#define BLK_INFO(_para, _name, _struct) \
	do {\
		_para[ISP_BLK_ID_##_name].phy_base = ISP_BLK_BA_##_name + ISP_TOP_PHY_REG_BASE;\
		_para[ISP_BLK_ID_##_name].reg_base = ctx->phys_regs[ISP_BLK_ID_##_name];\
		_para[ISP_BLK_ID_##_name].blk_size = sizeof(struct _struct) / 4;\
	} while (0)

#define SET_BITS(addr_ofs, val_ofs)						\
	do {									\
		reg_addr = reg_base + addr_ofs;					\
		data = ISP_RD_REG_BA(reg_addr) | (0x1 << val_ofs);		\
		ISP_WR_REG_BA(reg_addr, data);					\
	} while (0)

#define CLEAR_BITS(addr_ofs, val_ofs)						\
	do {									\
		reg_addr = reg_base + addr_ofs;					\
		data = ISP_RD_REG_BA(reg_addr) & (~(0x1 << val_ofs));		\
		ISP_WR_REG_BA(reg_addr, data);					\
	} while (0)

#define WAIT_IP_DISABLE(addr_ofs, val_ofs)			\
	do {							\
		do {						\
			osal_usleep_range(1, 10);			\
			reg_addr = reg_base + addr_ofs;		\
			data = ISP_RD_REG_BA(reg_addr);		\
		} while (((data >> val_ofs) & 0x1) != 0);	\
	} while (0)

#define SET_REGISTER_COMMON(addr_ofs, val_ofs, on_off)		\
	do {							\
		if (!on_off) {/* off */				\
			CLEAR_BITS(addr_ofs, val_ofs);		\
		} else {/* on */				\
			SET_BITS(addr_ofs, val_ofs);		\
		}						\
	} while (0)

#define GET_REGISTER_COMMON(addr_ofs, val_ofs, ret)		\
	do {							\
		reg_addr = reg_base + addr_ofs;			\
		data = ISP_RD_REG_BA(reg_addr);			\
		ret = (data >> val_ofs) & 0x1;			\
	} while (0)

#define SET_SW_MODE_ENABLE(addr_ofs, val_ofs, on_off)		\
	SET_REGISTER_COMMON(addr_ofs, val_ofs, on_off)

#define SET_SW_MODE_MEM_SEL(addr_ofs, val_ofs, mem_id)		\
	SET_REGISTER_COMMON(addr_ofs, val_ofs, mem_id)

#define DUMP_LUT_BASE(data_tbl, data_mask, r_addr, r_trig, r_data)	\
	do {								\
		for (i = 0 ; i < length; i++) {				\
			reg_addr = reg_base + r_addr;			\
			ISP_WR_REG_BA(reg_addr, i);			\
									\
			reg_addr = reg_base + r_trig;			\
			data = (ISP_RD_REG_BA(reg_addr) | (0x1 << 31));	\
			ISP_WR_REG_BA(reg_addr, data);			\
									\
			reg_addr = reg_base + r_data;			\
			data = ISP_RD_REG_BA(reg_addr);			\
			data_tbl[i] = (data & data_mask);		\
		}							\
	} while (0)

#define DUMP_LUT_COMMON(data_tbl, data_mask, sw_mode, r_addr, r_trig, r_data)	\
	do {									\
		reg_addr = reg_base + sw_mode;					\
		data = 0x1;							\
		ISP_WR_REG_BA(reg_addr, data);					\
										\
		DUMP_LUT_BASE(data_tbl, data_mask, r_addr, r_trig, r_data);	\
										\
		reg_addr = reg_base + sw_mode;					\
		data = 0x0;							\
		ISP_WR_REG_BA(reg_addr, data);					\
	} while (0)

#define FPRINTF_TBL(data_tbl)								\
	do {										\
		pos += sprintf(addr + pos, "\t\"%s\": {\n", name);			\
		pos += sprintf(addr + pos, "\t\t\"length\": %u,\n", length);		\
		pos += sprintf(addr + pos, "\t\t\"lut\": [\n");				\
		pos += sprintf(addr + pos, "\t\t\t");					\
		for (i = 0 ; i < length; i++) {						\
			if (i == length - 1) {						\
				pos += sprintf(addr + pos, "%u\n", data_tbl[i]);	\
			} else if (i % 16 == 15) {					\
				pos += sprintf(addr + pos, "%u,\n\t\t\t", data_tbl[i]); \
			} else {							\
				pos += sprintf(addr + pos, "%u,\t", data_tbl[i]);	\
			}								\
		}									\
		pos += sprintf(addr + pos, "\t\t]\n\t},\n");				\
	} while (0)

void vi_init_dump_register(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;

	BLK_INFO(m_block, PRE_RAW_FE0, reg_pre_raw_fe_t);
	BLK_INFO(m_block, CSIBDG0, reg_isp_csi_bdg_t);
	BLK_INFO(m_block, DMA_CTL_CSI0_BDG0, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_CSI0_BDG1, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_CSI0_BDG2, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_CSI0_BDG3, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, PRE_RAW_FE0_LSC0, reg_isp_lsc_t);
	BLK_INFO(m_block, PRE_RAW_FE0_LSC1, reg_isp_lsc_t);
	BLK_INFO(m_block, DMA_CTL_FE0_CLSC_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, AE_HIST_FE0, reg_isp_ae_hist_t);
	BLK_INFO(m_block, DMA_CTL_FE0_AE_HIST_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_FE0_AE_HIST_SE, reg_isp_dma_ctl_t);

	BLK_INFO(m_block, PRE_RAW_FE1, reg_pre_raw_fe_t);
	BLK_INFO(m_block, CSIBDG1, reg_isp_csi_bdg_t);
	BLK_INFO(m_block, DMA_CTL_CSI1_BDG0, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_CSI1_BDG1, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, PRE_RAW_FE1_LSC0, reg_isp_lsc_t);
	BLK_INFO(m_block, PRE_RAW_FE1_LSC1, reg_isp_lsc_t);
	BLK_INFO(m_block, DMA_CTL_FE1_CLSC_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, AE_HIST_FE1, reg_isp_ae_hist_t);
	BLK_INFO(m_block, DMA_CTL_FE1_AE_HIST_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_FE1_AE_HIST_SE, reg_isp_dma_ctl_t);

	BLK_INFO(m_block, PRE_RAW_FE2, reg_pre_raw_fe_t);
	BLK_INFO(m_block, CSIBDG2, reg_isp_csi_bdg_t);
	BLK_INFO(m_block, DMA_CTL_CSI2_BDG0, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, PRE_RAW_FE2_LSC0, reg_isp_lsc_t);
	BLK_INFO(m_block, DMA_CTL_FE2_CLSC_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, AE_HIST_FE2, reg_isp_ae_hist_t);
	BLK_INFO(m_block, DMA_CTL_FE2_AE_HIST_LE, reg_isp_dma_ctl_t);

	BLK_INFO(m_block, WDMA_CORE1, reg_wdma_core_t);
	BLK_INFO(m_block, WDMA_CORE2, reg_wdma_core_t);
	BLK_INFO(m_block, WDMA_CORE3, reg_wdma_core_t);
	BLK_INFO(m_block, WDMA_CORE4, reg_wdma_core_t);

	BLK_INFO(m_block, RAWTOP0, reg_raw_top0_t);
	BLK_INFO(m_block, RAWTOP1, reg_raw_top1_t);
	BLK_INFO(m_block, BNR, reg_isp_bnr_t);
	BLK_INFO(m_block, FUSION, reg_fusion_t);
	BLK_INFO(m_block, MAPCURVE, reg_map_curve_t);
	BLK_INFO(m_block, BLC_DG_WB0, reg_blc_dg_wb_t);
	BLK_INFO(m_block, BLC_DG_WB1, reg_blc_dg_wb_t);
	BLK_INFO(m_block, DPC, reg_isp_dpc_t);
	BLK_INFO(m_block, AF, reg_isp_af_t);
	BLK_INFO(m_block, DMA_CTL_AF_W, reg_isp_dma_ctl_t);

	BLK_INFO(m_block, LSCR, reg_isp_lscr_t);
	BLK_INFO(m_block, DMA_CTL_LSCR_HIST, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DRC, reg_isp_drc_t);
	BLK_INFO(m_block, DMA_CTL_DRC_POLY_R, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_DRC_POLY_W, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_DRC_HIST, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, CFA, reg_isp_cfa_t);

	BLK_INFO(m_block, RGBTOP, reg_rgb_top_t);
	BLK_INFO(m_block, PRE_EE_EXT, reg_ee_ext_t);
	BLK_INFO(m_block, PFR, reg_pfr_t);
	BLK_INFO(m_block, CCM, reg_isp_ccm_t);
	BLK_INFO(m_block, RGBGAMMA, reg_isp_gamma_t);
	BLK_INFO(m_block, RGB_DITHER, reg_isp_rgb_dither_t);
	BLK_INFO(m_block, CLUT, reg_isp_clut_t);
	BLK_INFO(m_block, CSC, reg_isp_csc_t);

	BLK_INFO(m_block, YUVTOP, reg_yuv_top_t);
	BLK_INFO(m_block, YUV_DITHER, reg_isp_yuv_dither_t);
	BLK_INFO(m_block, PRE_EE_FRONT, reg_ee_add_t);
	BLK_INFO(m_block, PRE_EE_BACK, reg_ee_add_back_t);
	BLK_INFO(m_block, LDCI, reg_ldci_t);
	BLK_INFO(m_block, DMA_CTL_LDCI_W, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_LDCI_R, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_LDCI_HIST, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, LDCI_MAP_CORE, reg_ldci_map_lut_t);
	BLK_INFO(m_block, TNR, reg_isp_444_422_t);
	BLK_INFO(m_block, DMA_CTL_TNR_LD_Y, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_TNR_LD_C, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_TNR_LD_MO, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_TNR_LD_MV, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_TNR_LD_FCB, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_TNR_ST_Y, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_TNR_ST_C, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_TNR_ST_MO, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_TNR_ST_MV, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_TNR_ST_FCB, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_TNR_ST_MSP, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, FBCE, reg_fbce_t);
	BLK_INFO(m_block, FBCD, reg_fbcd_t);
	BLK_INFO(m_block, CNR, reg_cnr_t);
	BLK_INFO(m_block, DMA_CTL_CNR_Y_W, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_CNR_C_W, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_CNR_Y_R, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_CNR_C_R, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, CA, reg_ca_t);
	BLK_INFO(m_block, CA_LITE, reg_ca_lite_t);
	BLK_INFO(m_block, POST_EE, reg_isp_ee_t);
	BLK_INFO(m_block, YCURVE, reg_isp_ycurv_t);
	BLK_INFO(m_block, YUV_CROP_Y, reg_crop_t);
	BLK_INFO(m_block, DMA_CTL_YUV_CROP_Y, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, YUV_CROP_C, reg_crop_t);
	BLK_INFO(m_block, DMA_CTL_YUV_CROP_C, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_YUV_RDMA_Y, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_YUV_RDMA_C, reg_isp_dma_ctl_t);

	BLK_INFO(m_block, ISPTOP, reg_isp_top_t);
	BLK_INFO(m_block, RDMA_CORE1, reg_rdma_core_t);
	BLK_INFO(m_block, RDMA_CORE2, reg_rdma_core_t);
	BLK_INFO(m_block, RDMA_CORE3, reg_rdma_core_t);
	BLK_INFO(m_block, CSIBDG0_LITE, reg_isp_csi_bdg_lite_t);
	BLK_INFO(m_block, DMA_CTL_BT0_LITE0, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_BT0_LITE1, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_BT0_LITE2, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_BT0_LITE3, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, PRE_RAW_VI_SEL, reg_pre_raw_vi_sel_t);
	BLK_INFO(m_block, DMA_CTL_PRE_RAW_VI_SEL_LE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, DMA_CTL_PRE_RAW_VI_SEL_SE, reg_isp_dma_ctl_t);
	BLK_INFO(m_block, PRE_RAW_VI_SEL_CROP_LE, reg_crop_t);
	BLK_INFO(m_block, PRE_RAW_VI_SEL_CROP_SE, reg_crop_t);
}

static void _dump_gamma_table(void *addr, struct gamma_tbl *tbl, int *offset)
{
	uint32_t data;
	uint32_t length = tbl->length;
	char name[32];
	uintptr_t reg_base = tbl->addr;
	uintptr_t reg_addr;
	uint32_t i = 0;
	uint8_t r_sel = 0;
	int pos = *offset;

	uint32_t *data_gamma_r = osal_vmalloc(sizeof(uint32_t) * length);
	uint32_t *data_gamma_g = osal_vmalloc(sizeof(uint32_t) * length);
	uint32_t *data_gamma_b = osal_vmalloc(sizeof(uint32_t) * length);

	SET_REGISTER_COMMON(tbl->prog_en.addr_ofs, tbl->prog_en.val_ofs, 1);

	GET_REGISTER_COMMON(tbl->prog_en.addr_ofs, 4, r_sel);
	SET_REGISTER_COMMON(tbl->raddr.addr_ofs, tbl->raddr.val_ofs, r_sel);

	for (i = 0 ; i < length; i++) {
		reg_addr = reg_base + tbl->raddr.addr_ofs;
		data = (ISP_RD_REG_BA(reg_addr) & (~tbl->raddr.mask)) | i;
		ISP_WR_REG_BA(reg_addr, data);

		reg_addr = reg_base + tbl->rdata_r.addr_ofs;
		data = (ISP_RD_REG_BA(reg_addr) | (0x1 << tbl->rdata_r.val_ofs));
		ISP_WR_REG_BA(reg_addr, data);

		reg_addr = reg_base + tbl->rdata_r.addr_ofs;
		data = ISP_RD_REG_BA(reg_addr);
		data_gamma_r[i] = (data & tbl->rdata_r.mask);

		reg_addr = reg_base + tbl->rdata_gb.addr_ofs;
		data = ISP_RD_REG_BA(reg_addr);
		data_gamma_g[i] = (data & tbl->rdata_gb.mask);
		data_gamma_b[i] = ((data >> tbl->rdata_gb.val_ofs) & tbl->rdata_gb.mask);
	}

	osal_memset(name, 0, sizeof(name));
	strcat(strcat(name, tbl->name), "_r");
	FPRINTF_TBL(data_gamma_r);

	osal_memset(name, 0, sizeof(name));
	strcat(strcat(name, tbl->name), "_g");
	FPRINTF_TBL(data_gamma_g);

	osal_memset(name, 0, sizeof(name));
	strcat(strcat(name, tbl->name), "_b");
	FPRINTF_TBL(data_gamma_b);

	osal_vfree(data_gamma_r);
	osal_vfree(data_gamma_g);
	osal_vfree(data_gamma_b);

	*offset = pos;
}

int vi_dump_register(struct sop_vi_dev *vdev, void *addr, int *size)
{
	uint32_t i = 0, j = 0, k = 0;
	int ret = 0;
	int pos = 0;
	int val = 0;
	char name[32] = {0};
	uint32_t length = 0;
	uintptr_t reg_base;
	uintptr_t reg_addr;
	uint32_t data;

	vi_init_dump_register(vdev);

	pos += sprintf(addr + pos, "{\n");

	for (i = 0; i < ISP_BLK_ID_MAX; i++) {
		if (!m_block[i].reg_base)
			continue;
		pos += sprintf(addr + pos, "\t\"0x%08X\": {\n", (uint32_t)m_block[i].phy_base);

		for (j = 0; j < m_block[i].blk_size; j++) {
			val = ISP_RD_REG_BA((uintptr_t)(m_block[i].reg_base + j * 0x4));
			pos += sprintf(addr + pos, "\t\t\"h%02x\": %u,\n", (j * 4), val);
		}

		pos += sprintf(addr + pos, "\t\t\"size\": %u\n\t},\n", m_block[i].blk_size);
	}

	//DPC
	{
		uint32_t *data_dpc = NULL;

		length = 4095;

		data_dpc = osal_vmalloc(sizeof(uint32_t) * length);

		reg_base = m_block[ISP_BLK_ID_DPC].reg_base;

		DUMP_LUT_BASE(data_dpc, 0xFFFFFF, 0x48, 0x4C, 0x4C);

		snprintf(name, sizeof(name), "dpc_tbl");
		FPRINTF_TBL(data_dpc);

		osal_vfree(data_dpc);

		vi_pr(VI_DBG, "DPC\n");
	}

	//rgbgamma
	{
		struct gamma_tbl rgb_gamma = {
			.name = "rgb_gamma",
			.length = 256,
			.addr = m_block[ISP_BLK_ID_RGBGAMMA].reg_base,
			.enable = {
				.addr_ofs = 0x0,
				.val_ofs = 0,
			},
			.shdw_sel = {
				.addr_ofs = 0x0,
				.val_ofs = 1,
			},
			.force_clk_enable = {
				.addr_ofs = 0x0,
				.val_ofs = 2,
			},
			.prog_en = {
				.addr_ofs = 0x4,
				.val_ofs = 8,
			},
			.raddr = {
				.addr_ofs = 0x14,
				.mask = 0xFF,
				.val_ofs = 12,
			},
			.rdata_r = {
				.addr_ofs = 0x18,
				.val_ofs = 31,
				.mask = 0xFFF,
			},
			.rdata_gb = {
				.addr_ofs = 0x1C,
				.val_ofs = 16,
				.mask = 0xFFF,
			},
		};

		_dump_gamma_table(addr, &rgb_gamma, &pos);

		vi_pr(VI_DBG, "RGB_GAMMA\n");
	}

	//clut
	{
		uint32_t r_idx = 17;
		uint32_t g_idx = 17;
		uint32_t b_idx = 17;
		uint32_t rgb_idx = 0;
		uint32_t *data_clut_r = NULL;
		uint32_t *data_clut_g = NULL;
		uint32_t *data_clut_b = NULL;
		uint8_t enable = 0;
		uint8_t shdw_sel = 0;

		length = r_idx * g_idx * b_idx;
		data_clut_r = osal_vmalloc(sizeof(uint32_t) * length);
		data_clut_g = osal_vmalloc(sizeof(uint32_t) * length);
		data_clut_b = osal_vmalloc(sizeof(uint32_t) * length);

		reg_base = m_block[ISP_BLK_ID_CLUT].reg_base;

		GET_REGISTER_COMMON(0x0, 0, enable);
		GET_REGISTER_COMMON(0x0, 1, shdw_sel);

		SET_REGISTER_COMMON(0x0, 0, 0); // reg_clut_enable
		SET_REGISTER_COMMON(0x0, 1, 0); // reg_clut_shdw_sel
		// SET_REGISTER_COMMON(0x0, 2, 0); // reg_force_clk_enable
		// WAIT_IP_DISABLE(0x0, 0);

		SET_REGISTER_COMMON(0x0, 3, 1); // reg_prog_en

		for (i = 0 ; i < b_idx; i++) {
			for (j = 0 ; j < g_idx; j++) {
				for (k = 0 ; k < r_idx; k++) {
					rgb_idx = i * g_idx * r_idx + j * r_idx + k;

					reg_addr = reg_base + 0x04; // reg_sram_r_idx/reg_sram_g_idx/reg_sram_b_idx
					data = (i << 16) | (j << 8) | k;
					ISP_WR_REG_BA(reg_addr, data);

					reg_addr = reg_base + 0x0C; // reg_sram_rd
					data = (0x1 << 31);
					ISP_WR_REG_BA(reg_addr, data);

					reg_addr = reg_base + 0x0C; // reg_sram_rdata
					data = ISP_RD_REG_BA(reg_addr);

					data_clut_r[rgb_idx] = (data >> 20) & 0x3FF;
					data_clut_g[rgb_idx] = (data >> 10) & 0x3FF;
					data_clut_b[rgb_idx] = data & 0x3FF;
				}
			}
		}

		SET_REGISTER_COMMON(0x0, 3, 0); // reg_prog_en
		SET_REGISTER_COMMON(0x0, 1, shdw_sel); // reg_clut_shdw_sel
		SET_REGISTER_COMMON(0x0, 0, enable); // reg_clut_enable

		snprintf(name, sizeof(name), "clut_r");
		FPRINTF_TBL(data_clut_r);

		snprintf(name, sizeof(name), "clut_g");
		FPRINTF_TBL(data_clut_g);

		snprintf(name, sizeof(name), "clut_b");
		FPRINTF_TBL(data_clut_b);

		osal_vfree(data_clut_r);
		osal_vfree(data_clut_g);
		osal_vfree(data_clut_b);

		vi_pr(VI_DBG, "CLUT\n");
	}

	//LDCI map
	{
		uint32_t *dci_map = NULL;

		length = 256;
		dci_map = osal_vmalloc(sizeof(uint32_t) * length);

		reg_base = m_block[ISP_BLK_ID_LDCI_MAP_CORE].reg_base;

		SET_REGISTER_COMMON(0x0, 16, 1); // reg_prog_en

		SET_REGISTER_COMMON(0x8, 16, 1); // rsel

		for (i = 0 ; i < length; i++) {
			reg_addr = reg_base + 0x8;
			data = (ISP_RD_REG_BA(reg_addr) & ~(0xFF00)) | (i << 8); // reg_lut_dbg_raddr[0,9]
			data = (data | 0x1); // reg_lut_dbg_read_en_1t
			ISP_WR_REG_BA(reg_addr, data);

			reg_addr = reg_base + 0x8; // reg_lut_dbg_rdata
			data = ISP_RD_REG_BA(reg_addr);
			dci_map[i] = (data >> 24) & 0xFF;
		}

		SET_REGISTER_COMMON(0x8, 16, 0); // rsel
		SET_REGISTER_COMMON(0x0, 16, 0); // reg_prog_en

		snprintf(name, sizeof(name), "dci_map");
		FPRINTF_TBL(dci_map);

		osal_vfree(dci_map);

		vi_pr(VI_DBG, "LDCI map\n");
	}

	//cacp
	{
		uint32_t *data_cacp_y = NULL;
		uint32_t *data_cacp_u = NULL;
		uint32_t *data_cacp_v = NULL;
		uint8_t enable = 0;
		uint8_t shdw_sel = 0;
		uint8_t ca_cp_mode = 0;

		length = 256;
		reg_base = m_block[ISP_BLK_ID_CA].reg_base;

		data_cacp_y = osal_vmalloc(sizeof(uint32_t) * length);
		data_cacp_u = osal_vmalloc(sizeof(uint32_t) * length);
		data_cacp_v = osal_vmalloc(sizeof(uint32_t) * length);

		GET_REGISTER_COMMON(0x0, 0, enable);
		GET_REGISTER_COMMON(0x0, 4, shdw_sel);
		GET_REGISTER_COMMON(0x0, 1, ca_cp_mode);

		SET_REGISTER_COMMON(0x0, 0, 0); // reg_cacp_enable
		SET_REGISTER_COMMON(0x0, 4, 0); // reg_cacp_shdw_read_sel
		// WAIT_IP_DISABLE(0x0, 0);
		SET_REGISTER_COMMON(0x0, 3, 1); // reg_cacp_mem_sw_mode

		for (i = 0 ; i < length; i++) {
			reg_addr = reg_base + 0x0C;
			data = (ISP_RD_REG_BA(reg_addr) & ~0xFF) | i;
			ISP_WR_REG_BA(reg_addr, data);

			reg_addr = reg_base + 0x0C;
			data = (ISP_RD_REG_BA(reg_addr) | (0x1 << 31));
			ISP_WR_REG_BA(reg_addr, data);

			reg_addr = reg_base + 0x10;
			data = ISP_RD_REG_BA(reg_addr);

			if (ca_cp_mode) {
				data_cacp_y[i] = ((data >> 16) & 0xFF);
				data_cacp_u[i] = ((data >> 8) & 0xFF);
				data_cacp_v[i] = (data & 0xFF);
			} else {
				data_cacp_y[i] = (data & 0x7FF);
			}
		}

		if (ca_cp_mode) {
			snprintf(name, sizeof(name), "ca_cp_y");
			FPRINTF_TBL(data_cacp_y);
			snprintf(name, sizeof(name), "ca_cp_u");
			FPRINTF_TBL(data_cacp_u);
			snprintf(name, sizeof(name), "ca_cp_v");
			FPRINTF_TBL(data_cacp_v);
		} else {
			snprintf(name, sizeof(name), "ca_y_ratio");
			FPRINTF_TBL(data_cacp_y);
		}

		SET_REGISTER_COMMON(0x0, 3, 0);  // reg_cacp_mem_sw_mode
		SET_REGISTER_COMMON(0x0, 4, shdw_sel); // reg_cacp_shdw_read_sel
		SET_REGISTER_COMMON(0x0, 0, enable); // reg_cacp_enable

		osal_vfree(data_cacp_y);
		osal_vfree(data_cacp_u);
		osal_vfree(data_cacp_v);

		vi_pr(VI_DBG, "cacp\n");
	}

	//YCURVE
	{
		uint8_t r_sel = 0;
		uint32_t *data_ycurve = osal_vmalloc(sizeof(uint32_t) * length);

		reg_base = m_block[ISP_BLK_ID_YCURVE].reg_base;

		SET_REGISTER_COMMON(0x4, 8, 1); // reg_ycur_prog_en

		GET_REGISTER_COMMON(0x4, 4, r_sel);
		vi_pr(VI_DBG, "mem[%d] work, mem[%d] IDLE\n", r_sel, r_sel ^ 0x1);
		//SET_REGISTER_COMMON(0x14, 12, r_sel ^ 0x1);
		SET_REGISTER_COMMON(0x14, 12, r_sel);

		for (i = 0 ; i < length; i++) {
			reg_addr = reg_base + 0x14;
			data = (ISP_RD_REG_BA(reg_addr) & ~(0x3F)) | i;
			ISP_WR_REG_BA(reg_addr, data);

			reg_addr = reg_base + 0x18;
			data = (ISP_RD_REG_BA(reg_addr) | (0x1 << 31));
			ISP_WR_REG_BA(reg_addr, data);

			reg_addr = reg_base + 0x18;
			data = ISP_RD_REG_BA(reg_addr);
			data_ycurve[i] = (data & 0xFF);
		}

		SET_REGISTER_COMMON(0x4, 8, 0); // reg_ycur_prog_en

		snprintf(name, sizeof(name), "ycurve");
		FPRINTF_TBL(data_ycurve);

		osal_vfree(data_ycurve);

		vi_pr(VI_DBG, "ycurve\n");
	}

	pos += sprintf(addr + pos, "\t\"end\": {}\n");
	pos += sprintf(addr + pos, "}");

	*size = pos;

	return ret;
}