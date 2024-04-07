// $Module: ive_hist $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Wed, 15 Dec 2021 10:10:49 AM $
//

#ifndef __REG_IVE_HIST_STRUCT_H__
#define __REG_IVE_HIST_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*soft reset for pipe engine;*/
		uint32_t reg_ive_hist_enable:1;
		/*reg reg sel;*/
		uint32_t reg_shdw_sel:1;
		/*soft reset for pipe engine;*/
		uint32_t reg_softrst:1;
		uint32_t rsv_3_3:1;
		/*tile number, 0 for no tile;*/
		uint32_t reg_ive_hist_tile_nm:4;
		/*force clock enable;*/
		uint32_t reg_force_clk_enable:1;
		/*force dma diable;*/
		uint32_t reg_force_dma_disable:1;
	};
	uint32_t val;
} IVE_HIST_REG_0_C;
typedef union {
	struct {
		/*stride;*/
		uint32_t reg_ive_hist_stride:16;
	};
	uint32_t val;
} IVE_HIST_REG_1_C;
typedef union {
	struct {
		/*memory address;*/
		uint32_t reg_ive_hist_mem_addr:32;
	};
	uint32_t val;
} IVE_HIST_REG_2_C;
typedef struct {
	volatile IVE_HIST_REG_0_C REG_0;
	volatile IVE_HIST_REG_1_C REG_1;
	volatile IVE_HIST_REG_2_C REG_2;
} IVE_HIST_C;
#ifdef __cplusplus

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void ive_hist_dump_ini(FILE* fp, IVE_HIST_C* p) {
	fprintf(fp, "reg_ive_hist_enable = 0x%x\n",p->REG_0.reg_ive_hist_enable);
	fprintf(fp, "reg_shdw_sel = 0x%x\n",p->REG_0.reg_shdw_sel);
	fprintf(fp, "reg_softrst = 0x%x\n",p->REG_0.reg_softrst);
	fprintf(fp, "reg_ive_hist_tile_nm = 0x%x\n",p->REG_0.reg_ive_hist_tile_nm);
	fprintf(fp, "reg_force_clk_enable = 0x%x\n",p->REG_0.reg_force_clk_enable);
	fprintf(fp, "reg_force_dma_disable = 0x%x\n",p->REG_0.reg_force_dma_disable);
	fprintf(fp, "reg_ive_hist_stride = 0x%x\n",p->REG_1.reg_ive_hist_stride);
	fprintf(fp, "reg_ive_hist_mem_addr = 0x%x\n",p->REG_2.reg_ive_hist_mem_addr);

}
static void ive_hist_print(IVE_HIST_C* p) {
    fprintf(stderr, "ive_hist\n");
	fprintf(stderr, "\tREG_0.reg_ive_hist_enable = 0x%x\n", p->REG_0.reg_ive_hist_enable);
	fprintf(stderr, "\tREG_0.reg_shdw_sel = 0x%x\n", p->REG_0.reg_shdw_sel);
	fprintf(stderr, "\tREG_0.reg_softrst = 0x%x\n", p->REG_0.reg_softrst);
	fprintf(stderr, "\tREG_0.reg_ive_hist_tile_nm = 0x%x\n", p->REG_0.reg_ive_hist_tile_nm);
	fprintf(stderr, "\tREG_0.reg_force_clk_enable = 0x%x\n", p->REG_0.reg_force_clk_enable);
	fprintf(stderr, "\tREG_0.reg_force_dma_disable = 0x%x\n", p->REG_0.reg_force_dma_disable);
	fprintf(stderr, "\tREG_1.reg_ive_hist_stride = 0x%x\n", p->REG_1.reg_ive_hist_stride);
	fprintf(stderr, "\tREG_2.reg_ive_hist_mem_addr = 0x%x\n", p->REG_2.reg_ive_hist_mem_addr);

}
#pragma GCC diagnostic pop
#define DEFINE_IVE_HIST_C(X) \
	 IVE_HIST_C X = \
{\
	{	/* REG_0.reg_ive_hist_enable = */0x0,\
	/*.REG_0.reg_shdw_sel = */0x1,\
	/*.REG_0.reg_softrst = */0x0,\
	/*uint32_t rsv_3_3:1;=*/0,\
	/*.REG_0.reg_ive_hist_tile_nm = */0x0,\
	/*.REG_0.reg_force_clk_enable = */0x1,\
	/*.REG_0.reg_force_dma_disable = */0x0,\
	},\
	{	/*.REG_1.reg_ive_hist_stride = */0x0,\
	},\
	{	/*.REG_2.reg_ive_hist_mem_addr = */0x1,\
	}\
};
#else /* !ifdef __cplusplus */
#define _DEFINE_IVE_HIST_C \
{\
	.REG_0.reg_ive_hist_enable = 0x0,\
	.REG_0.reg_shdw_sel = 0x1,\
	.REG_0.reg_softrst = 0x0,\
	.REG_0.reg_ive_hist_tile_nm = 0x0,\
	.REG_0.reg_force_clk_enable = 0x1,\
	.REG_0.reg_force_dma_disable = 0x0,\
	.REG_1.reg_ive_hist_stride = 0x0,\
	.REG_2.reg_ive_hist_mem_addr = 0x1,\
}
#endif /* ifdef __cplusplus */
#endif //__REG_IVE_HIST_STRUCT_H__
