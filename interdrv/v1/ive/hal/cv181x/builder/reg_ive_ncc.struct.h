// $Module: ive_ncc $
// $RegisterBank Version: V 1.0.00 $
// $Author: andy.tsao $
// $Date: Thu, 25 Nov 2021 03:58:50 PM $
//

#ifndef __REG_IVE_NCC_STRUCT_H__
#define __REG_IVE_NCC_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*numerator[31:0], read_only;*/
		uint32_t reg_numerator_l:32;
	};
	uint32_t val;
} IVE_NCC_REG_NCC_00_C;
typedef union {
	struct {
		/*numerator[39:32], read_only;*/
		uint32_t reg_numerator_h:8;
	};
	uint32_t val;
} IVE_NCC_REG_NCC_01_C;
typedef union {
	struct {
		/*quadsum0[31:0], read_only;*/
		uint32_t reg_quadsum0_l:32;
	};
	uint32_t val;
} IVE_NCC_REG_NCC_02_C;
typedef union {
	struct {
		/*quadsum0[39:32], read_only;*/
		uint32_t reg_quadsum0_h:8;
	};
	uint32_t val;
} IVE_NCC_REG_NCC_03_C;
typedef union {
	struct {
		/*quadsum1[31:0], read_only;*/
		uint32_t reg_quadsum1_l:32;
	};
	uint32_t val;
} IVE_NCC_REG_NCC_04_C;
typedef union {
	struct {
		/*quadsum1[39:32], read_only;*/
		uint32_t reg_quadsum1_h:8;
	};
	uint32_t val;
} IVE_NCC_REG_NCC_05_C;
typedef union {
	struct {
		/*reg_crop_enable;*/
		uint32_t reg_crop_enable:1;
	};
	uint32_t val;
} IVE_NCC_REG_NCC_06_C;
typedef union {
	struct {
		/*reg_crop_start_x;*/
		uint32_t reg_crop_start_x:16;
	};
	uint32_t val;
} IVE_NCC_REG_NCC_07_C;
typedef union {
	struct {
		/*reg_crop_start_y;*/
		uint32_t reg_crop_start_y:16;
	};
	uint32_t val;
} IVE_NCC_REG_NCC_08_C;
typedef union {
	struct {
		/*reg_crop_end_x;*/
		uint32_t reg_crop_end_x:16;
	};
	uint32_t val;
} IVE_NCC_REG_NCC_09_C;
typedef union {
	struct {
		/*reg_crop_end_y;*/
		uint32_t reg_crop_end_y:16;
	};
	uint32_t val;
} IVE_NCC_REG_NCC_10_C;
typedef union {
	struct {
		/*[0:1];*/
		uint32_t reg_shdw_sel:1;
	};
	uint32_t val;
} IVE_NCC_REG_NCC_11_C;
typedef struct {
	volatile IVE_NCC_REG_NCC_00_C REG_NCC_00;
	volatile IVE_NCC_REG_NCC_01_C REG_NCC_01;
	volatile IVE_NCC_REG_NCC_02_C REG_NCC_02;
	volatile IVE_NCC_REG_NCC_03_C REG_NCC_03;
	volatile IVE_NCC_REG_NCC_04_C REG_NCC_04;
	volatile IVE_NCC_REG_NCC_05_C REG_NCC_05;
	volatile IVE_NCC_REG_NCC_06_C REG_NCC_06;
	volatile IVE_NCC_REG_NCC_07_C REG_NCC_07;
	volatile IVE_NCC_REG_NCC_08_C REG_NCC_08;
	volatile IVE_NCC_REG_NCC_09_C REG_NCC_09;
	volatile IVE_NCC_REG_NCC_10_C REG_NCC_10;
	volatile IVE_NCC_REG_NCC_11_C REG_NCC_11;
} IVE_NCC_C;
#ifdef __cplusplus 

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void ive_ncc_dump_ini(FILE* fp, IVE_NCC_C* p) {
	fprintf(fp, "reg_numerator_l = 0x%x\n",p->REG_NCC_00.reg_numerator_l);
	fprintf(fp, "reg_numerator_h = 0x%x\n",p->REG_NCC_01.reg_numerator_h);
	fprintf(fp, "reg_quadsum0_l = 0x%x\n",p->REG_NCC_02.reg_quadsum0_l);
	fprintf(fp, "reg_quadsum0_h = 0x%x\n",p->REG_NCC_03.reg_quadsum0_h);
	fprintf(fp, "reg_quadsum1_l = 0x%x\n",p->REG_NCC_04.reg_quadsum1_l);
	fprintf(fp, "reg_quadsum1_h = 0x%x\n",p->REG_NCC_05.reg_quadsum1_h);
	fprintf(fp, "reg_crop_enable = 0x%x\n",p->REG_NCC_06.reg_crop_enable);
	fprintf(fp, "reg_crop_start_x = 0x%x\n",p->REG_NCC_07.reg_crop_start_x);
	fprintf(fp, "reg_crop_start_y = 0x%x\n",p->REG_NCC_08.reg_crop_start_y);
	fprintf(fp, "reg_crop_end_x = 0x%x\n",p->REG_NCC_09.reg_crop_end_x);
	fprintf(fp, "reg_crop_end_y = 0x%x\n",p->REG_NCC_10.reg_crop_end_y);
	fprintf(fp, "reg_shdw_sel = 0x%x\n",p->REG_NCC_11.reg_shdw_sel);

}
static void ive_ncc_print(IVE_NCC_C* p) {
    fprintf(stderr, "ive_ncc\n");
	fprintf(stderr, "\tREG_NCC_00.reg_numerator_l = 0x%x\n", p->REG_NCC_00.reg_numerator_l);
	fprintf(stderr, "\tREG_NCC_01.reg_numerator_h = 0x%x\n", p->REG_NCC_01.reg_numerator_h);
	fprintf(stderr, "\tREG_NCC_02.reg_quadsum0_l = 0x%x\n", p->REG_NCC_02.reg_quadsum0_l);
	fprintf(stderr, "\tREG_NCC_03.reg_quadsum0_h = 0x%x\n", p->REG_NCC_03.reg_quadsum0_h);
	fprintf(stderr, "\tREG_NCC_04.reg_quadsum1_l = 0x%x\n", p->REG_NCC_04.reg_quadsum1_l);
	fprintf(stderr, "\tREG_NCC_05.reg_quadsum1_h = 0x%x\n", p->REG_NCC_05.reg_quadsum1_h);
	fprintf(stderr, "\tREG_NCC_06.reg_crop_enable = 0x%x\n", p->REG_NCC_06.reg_crop_enable);
	fprintf(stderr, "\tREG_NCC_07.reg_crop_start_x = 0x%x\n", p->REG_NCC_07.reg_crop_start_x);
	fprintf(stderr, "\tREG_NCC_08.reg_crop_start_y = 0x%x\n", p->REG_NCC_08.reg_crop_start_y);
	fprintf(stderr, "\tREG_NCC_09.reg_crop_end_x = 0x%x\n", p->REG_NCC_09.reg_crop_end_x);
	fprintf(stderr, "\tREG_NCC_10.reg_crop_end_y = 0x%x\n", p->REG_NCC_10.reg_crop_end_y);
	fprintf(stderr, "\tREG_NCC_11.reg_shdw_sel = 0x%x\n", p->REG_NCC_11.reg_shdw_sel);

}
#pragma GCC diagnostic pop
#define DEFINE_IVE_NCC_C(X) \
	 IVE_NCC_C X = \
{\
	{	/* REG_NCC_00.reg_numerator_l = */0x0,\
	},\
	{	/*.REG_NCC_01.reg_numerator_h = */0x0,\
	},\
	{	/*.REG_NCC_02.reg_quadsum0_l = */0x0,\
	},\
	{	/*.REG_NCC_03.reg_quadsum0_h = */0x0,\
	},\
	{	/*.REG_NCC_04.reg_quadsum1_l = */0x0,\
	},\
	{	/*.REG_NCC_05.reg_quadsum1_h = */0x0,\
	},\
	{	/*.REG_NCC_06.reg_crop_enable = */0x0,\
	},\
	{	/*.REG_NCC_07.reg_crop_start_x = */0x0,\
	},\
	{	/*.REG_NCC_08.reg_crop_start_y = */0x0,\
	},\
	{	/*.REG_NCC_09.reg_crop_end_x = */0x0,\
	},\
	{	/*.REG_NCC_10.reg_crop_end_y = */0x0,\
	},\
	{	/*.REG_NCC_11.reg_shdw_sel = */0x1,\
	}\
}; 
#else /* !ifdef __cplusplus */ 
#define DEFINE_IVE_NCC_C(X) \
	 IVE_NCC_C X = \
{\
	.REG_NCC_00.reg_numerator_l = 0x0,\
	.REG_NCC_01.reg_numerator_h = 0x0,\
	.REG_NCC_02.reg_quadsum0_l = 0x0,\
	.REG_NCC_03.reg_quadsum0_h = 0x0,\
	.REG_NCC_04.reg_quadsum1_l = 0x0,\
	.REG_NCC_05.reg_quadsum1_h = 0x0,\
	.REG_NCC_06.reg_crop_enable = 0x0,\
	.REG_NCC_07.reg_crop_start_x = 0x0,\
	.REG_NCC_08.reg_crop_start_y = 0x0,\
	.REG_NCC_09.reg_crop_end_x = 0x0,\
	.REG_NCC_10.reg_crop_end_y = 0x0,\
	.REG_NCC_11.reg_shdw_sel = 0x1,\
};
#endif /* ifdef __cplusplus */ 
#endif //__REG_IVE_NCC_STRUCT_H__
