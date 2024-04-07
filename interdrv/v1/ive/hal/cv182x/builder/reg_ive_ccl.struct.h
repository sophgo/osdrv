// $Module: ive_ccl $
// $RegisterBank Version: V 1.0.00 $
// $Author: andy.tsao $
// $Date: Tue, 07 Dec 2021 11:00:20 AM $
//

#ifndef __REG_IVE_CCL_STRUCT_H__
#define __REG_IVE_CCL_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*0: ccl 4c ; 1: ccl 8c;*/
		uint32_t reg_ccl_mode:1;
		uint32_t rsv_1_3:3;
		/*[0:1];*/
		uint32_t reg_ccl_shdw_sel:1;
	};
	uint32_t val;
} IVE_CCL_REG_CCL_00_C;
typedef union {
	struct {
		/*unsigned 16b; min area = 1;*/
		uint32_t reg_ccl_area_thr:16;
		/*unsigned 16b; min area = 1;*/
		uint32_t reg_ccl_area_step:16;
	};
	uint32_t val;
} IVE_CCL_REG_CCL_01_C;
typedef union {
	struct {
		/*0: no force; 1:force clk;*/
		uint32_t reg_force_clk_enable:1;
	};
	uint32_t val;
} IVE_CCL_REG_CCL_02_C;
typedef union {
	struct {
		/*total region number,[0:254];*/
		uint32_t reg_ccl_region_num:8;
		/*8h0: label successful; 8hff: label failed ;*/
		uint32_t reg_ccl_label_status:8;
		/*current region area threshold;*/
		uint32_t reg_ccl_cur_area_thr:16;
	};
	uint32_t val;
} IVE_CCL_REG_CCL_03_C;
typedef struct {
	volatile IVE_CCL_REG_CCL_00_C REG_CCL_00;
	volatile IVE_CCL_REG_CCL_01_C REG_CCL_01;
	volatile IVE_CCL_REG_CCL_02_C REG_CCL_02;
	volatile IVE_CCL_REG_CCL_03_C REG_CCL_03;
} IVE_CCL_C;
#ifdef __cplusplus 

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void ive_ccl_dump_ini(FILE* fp, IVE_CCL_C* p) {
	fprintf(fp, "reg_ccl_mode = 0x%x\n",p->REG_CCL_00.reg_ccl_mode);
	fprintf(fp, "reg_ccl_shdw_sel = 0x%x\n",p->REG_CCL_00.reg_ccl_shdw_sel);
	fprintf(fp, "reg_ccl_area_thr = 0x%x\n",p->REG_CCL_01.reg_ccl_area_thr);
	fprintf(fp, "reg_ccl_area_step = 0x%x\n",p->REG_CCL_01.reg_ccl_area_step);
	fprintf(fp, "reg_force_clk_enable = 0x%x\n",p->REG_CCL_02.reg_force_clk_enable);
	fprintf(fp, "reg_ccl_region_num = 0x%x\n",p->REG_CCL_03.reg_ccl_region_num);
	fprintf(fp, "reg_ccl_label_status = 0x%x\n",p->REG_CCL_03.reg_ccl_label_status);
	fprintf(fp, "reg_ccl_cur_area_thr = 0x%x\n",p->REG_CCL_03.reg_ccl_cur_area_thr);

}
static void ive_ccl_print(IVE_CCL_C* p) {
    fprintf(stderr, "ive_ccl\n");
	fprintf(stderr, "\tREG_CCL_00.reg_ccl_mode = 0x%x\n", p->REG_CCL_00.reg_ccl_mode);
	fprintf(stderr, "\tREG_CCL_00.reg_ccl_shdw_sel = 0x%x\n", p->REG_CCL_00.reg_ccl_shdw_sel);
	fprintf(stderr, "\tREG_CCL_01.reg_ccl_area_thr = 0x%x\n", p->REG_CCL_01.reg_ccl_area_thr);
	fprintf(stderr, "\tREG_CCL_01.reg_ccl_area_step = 0x%x\n", p->REG_CCL_01.reg_ccl_area_step);
	fprintf(stderr, "\tREG_CCL_02.reg_force_clk_enable = 0x%x\n", p->REG_CCL_02.reg_force_clk_enable);
	fprintf(stderr, "\tREG_CCL_03.reg_ccl_region_num = 0x%x\n", p->REG_CCL_03.reg_ccl_region_num);
	fprintf(stderr, "\tREG_CCL_03.reg_ccl_label_status = 0x%x\n", p->REG_CCL_03.reg_ccl_label_status);
	fprintf(stderr, "\tREG_CCL_03.reg_ccl_cur_area_thr = 0x%x\n", p->REG_CCL_03.reg_ccl_cur_area_thr);

}
#pragma GCC diagnostic pop
#define DEFINE_IVE_CCL_C(X) \
	 IVE_CCL_C X = \
{\
	{	/* REG_CCL_00.reg_ccl_mode = */0x0,\
	/*uint32_t rsv_1_3:3;=*/0,\
	/*.REG_CCL_00.reg_ccl_shdw_sel = */0x1,\
	},\
	{	/*.REG_CCL_01.reg_ccl_area_thr = */0x1,\
	/*.REG_CCL_01.reg_ccl_area_step = */0x1,\
	},\
	{	/*.REG_CCL_02.reg_force_clk_enable = */0x0,\
	},\
	{	/*.REG_CCL_03.reg_ccl_region_num = */0x0,\
	/*.REG_CCL_03.reg_ccl_label_status = */0x0,\
	/*.REG_CCL_03.reg_ccl_cur_area_thr = */0x0,\
	}\
}; 
#else /* !ifdef __cplusplus */ 
#define DEFINE_IVE_CCL_C(X) \
	 IVE_CCL_C X = \
{\
	.REG_CCL_00.reg_ccl_mode = 0x0,\
	.REG_CCL_00.reg_ccl_shdw_sel = 0x1,\
	.REG_CCL_01.reg_ccl_area_thr = 0x1,\
	.REG_CCL_01.reg_ccl_area_step = 0x1,\
	.REG_CCL_02.reg_force_clk_enable = 0x0,\
	.REG_CCL_03.reg_ccl_region_num = 0x0,\
	.REG_CCL_03.reg_ccl_label_status = 0x0,\
	.REG_CCL_03.reg_ccl_cur_area_thr = 0x0,\
};
#endif /* ifdef __cplusplus */ 
#endif //__REG_IVE_CCL_STRUCT_H__
