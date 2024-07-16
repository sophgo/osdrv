// $Module: ive_match_bg $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Tue, 07 Dec 2021 11:00:50 AM $
//

#ifndef __REG_IVE_MATCH_BG_STRUCT_H__
#define __REG_IVE_MATCH_BG_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*Matchbg enable
		0: disable
		1: enable;*/
		uint32_t reg_matchbg_en:1;
		/*Matchbg bgmodel bypass
		0: normal mode
		1: Read bgmodel from DMA and bypass to Update bg;*/
		uint32_t reg_matchbg_bypass_model:1;
		uint32_t rsv_2_3:2;
		/*Matchbg software reset
		0: normal
		1: reset;*/
		uint32_t reg_matchbg_softrst:1;
	};
	uint32_t val;
} IVE_MATCH_BG_REG_00_C;
typedef union {
	struct {
		/*Current frame timestamp, in frame units;*/
		uint32_t reg_matchbg_curfrmnum:32;
	};
	uint32_t val;
} IVE_MATCH_BG_REG_04_C;
typedef union {
	struct {
		/*Previous frame timestamp, in frame units;*/
		uint32_t reg_matchbg_prefrmnum:32;
	};
	uint32_t val;
} IVE_MATCH_BG_REG_08_C;
typedef union {
	struct {
		/*Potential background replacement time threshold (range: 2 to 100; default: 20);*/
		uint32_t reg_matchbg_timethr:7;
		uint32_t rsv_7_7:1;
		/*Correlation coefficients between differential threshold and gray value (range: 0 to 5; default: 0);*/
		uint32_t reg_matchbg_diffthrcrlcoef:3;
		uint32_t rsv_11_11:1;
		/*Maximum of background differential threshold (range: 3 to 15; default: 6);*/
		uint32_t reg_matchbg_diffmaxthr:4;
		/*Minimum of background differential threshold (range: 3 to 15; default: 4);*/
		uint32_t reg_matchbg_diffminthr:4;
		/*Dynamic Background differential threshold increment (range: 0 to 6; default: 0);*/
		uint32_t reg_matchbg_diffthrinc:3;
		uint32_t rsv_23_23:1;
		/*Quick background learning rate (range: 0 to 4; default: 2);*/
		uint32_t reg_matchbg_fastlearnrate:3;
		uint32_t rsv_27_27:1;
		/*Whether to detect change region (range: 0(no), 1(yes); default: 0);*/
		uint32_t reg_matchbg_detchgregion:1;
	};
	uint32_t val;
} IVE_MATCH_BG_REG_0C_C;
typedef union {
	struct {
		/*Pixel numbers of fg;*/
		uint32_t reg_matchbg_stat_pixnum:32;
	};
	uint32_t val;
} IVE_MATCH_BG_REG_10_C;
typedef union {
	struct {
		/*Summary of all input pixel luminance;*/
		uint32_t reg_matchbg_stat_sumlum:32;
	};
	uint32_t val;
} IVE_MATCH_BG_REG_14_C;
typedef struct {
	volatile IVE_MATCH_BG_REG_00_C REG_00;
	volatile IVE_MATCH_BG_REG_04_C REG_04;
	volatile IVE_MATCH_BG_REG_08_C REG_08;
	volatile IVE_MATCH_BG_REG_0C_C REG_0C;
	volatile IVE_MATCH_BG_REG_10_C REG_10;
	volatile IVE_MATCH_BG_REG_14_C REG_14;
} IVE_MATCH_BG_C;

static void ive_match_bg_printk(IVE_MATCH_BG_C *p)
{
	pr_info("ive_match_bg\n");
	pr_info("\tREG_00.reg_matchbg_en = 0x%x\n", p->REG_00.reg_matchbg_en);
	pr_info("\tREG_00.reg_matchbg_bypass_model = 0x%x\n",
		p->REG_00.reg_matchbg_bypass_model);
	pr_info("\tREG_00.reg_matchbg_softrst = 0x%x\n",
		p->REG_00.reg_matchbg_softrst);
	pr_info("\tREG_04.reg_matchbg_curfrmnum = 0x%x\n",
		p->REG_04.reg_matchbg_curfrmnum);
	pr_info("\tREG_08.reg_matchbg_prefrmnum = 0x%x\n",
		p->REG_08.reg_matchbg_prefrmnum);
	pr_info("\tREG_0C.reg_matchbg_timethr = 0x%x\n",
		p->REG_0C.reg_matchbg_timethr);
	pr_info("\tREG_0C.reg_matchbg_diffthrcrlcoef = 0x%x\n",
		p->REG_0C.reg_matchbg_diffthrcrlcoef);
	pr_info("\tREG_0C.reg_matchbg_diffmaxthr = 0x%x\n",
		p->REG_0C.reg_matchbg_diffmaxthr);
	pr_info("\tREG_0C.reg_matchbg_diffminthr = 0x%x\n",
		p->REG_0C.reg_matchbg_diffminthr);
	pr_info("\tREG_0C.reg_matchbg_diffthrinc = 0x%x\n",
		p->REG_0C.reg_matchbg_diffthrinc);
	pr_info("\tREG_0C.reg_matchbg_fastlearnrate = 0x%x\n",
		p->REG_0C.reg_matchbg_fastlearnrate);
	pr_info("\tREG_0C.reg_matchbg_detchgregion = 0x%x\n",
		p->REG_0C.reg_matchbg_detchgregion);
	pr_info("\tREG_10.reg_matchbg_stat_pixnum = 0x%x\n",
		p->REG_10.reg_matchbg_stat_pixnum);
	pr_info("\tREG_14.reg_matchbg_stat_sumlum = 0x%x\n",
		p->REG_14.reg_matchbg_stat_sumlum);
}

#ifdef __cplusplus

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void ive_match_bg_dump_ini(FILE* fp, IVE_MATCH_BG_C* p) {
	fprintf(fp, "reg_matchbg_en = 0x%x\n",p->REG_00.reg_matchbg_en);
	fprintf(fp, "reg_matchbg_bypass_model = 0x%x\n",p->REG_00.reg_matchbg_bypass_model);
	fprintf(fp, "reg_matchbg_softrst = 0x%x\n",p->REG_00.reg_matchbg_softrst);
	fprintf(fp, "reg_matchbg_curfrmnum = 0x%x\n",p->REG_04.reg_matchbg_curfrmnum);
	fprintf(fp, "reg_matchbg_prefrmnum = 0x%x\n",p->REG_08.reg_matchbg_prefrmnum);
	fprintf(fp, "reg_matchbg_timethr = 0x%x\n",p->REG_0C.reg_matchbg_timethr);
	fprintf(fp, "reg_matchbg_diffthrcrlcoef = 0x%x\n",p->REG_0C.reg_matchbg_diffthrcrlcoef);
	fprintf(fp, "reg_matchbg_diffmaxthr = 0x%x\n",p->REG_0C.reg_matchbg_diffmaxthr);
	fprintf(fp, "reg_matchbg_diffminthr = 0x%x\n",p->REG_0C.reg_matchbg_diffminthr);
	fprintf(fp, "reg_matchbg_diffthrinc = 0x%x\n",p->REG_0C.reg_matchbg_diffthrinc);
	fprintf(fp, "reg_matchbg_fastlearnrate = 0x%x\n",p->REG_0C.reg_matchbg_fastlearnrate);
	fprintf(fp, "reg_matchbg_detchgregion = 0x%x\n",p->REG_0C.reg_matchbg_detchgregion);
	fprintf(fp, "reg_matchbg_stat_pixnum = 0x%x\n",p->REG_10.reg_matchbg_stat_pixnum);
	fprintf(fp, "reg_matchbg_stat_sumlum = 0x%x\n",p->REG_14.reg_matchbg_stat_sumlum);

}
static void ive_match_bg_print(IVE_MATCH_BG_C* p) {
    fprintf(stderr, "ive_match_bg\n");
	fprintf(stderr, "\tREG_00.reg_matchbg_en = 0x%x\n", p->REG_00.reg_matchbg_en);
	fprintf(stderr, "\tREG_00.reg_matchbg_bypass_model = 0x%x\n", p->REG_00.reg_matchbg_bypass_model);
	fprintf(stderr, "\tREG_00.reg_matchbg_softrst = 0x%x\n", p->REG_00.reg_matchbg_softrst);
	fprintf(stderr, "\tREG_04.reg_matchbg_curfrmnum = 0x%x\n", p->REG_04.reg_matchbg_curfrmnum);
	fprintf(stderr, "\tREG_08.reg_matchbg_prefrmnum = 0x%x\n", p->REG_08.reg_matchbg_prefrmnum);
	fprintf(stderr, "\tREG_0C.reg_matchbg_timethr = 0x%x\n", p->REG_0C.reg_matchbg_timethr);
	fprintf(stderr, "\tREG_0C.reg_matchbg_diffthrcrlcoef = 0x%x\n", p->REG_0C.reg_matchbg_diffthrcrlcoef);
	fprintf(stderr, "\tREG_0C.reg_matchbg_diffmaxthr = 0x%x\n", p->REG_0C.reg_matchbg_diffmaxthr);
	fprintf(stderr, "\tREG_0C.reg_matchbg_diffminthr = 0x%x\n", p->REG_0C.reg_matchbg_diffminthr);
	fprintf(stderr, "\tREG_0C.reg_matchbg_diffthrinc = 0x%x\n", p->REG_0C.reg_matchbg_diffthrinc);
	fprintf(stderr, "\tREG_0C.reg_matchbg_fastlearnrate = 0x%x\n", p->REG_0C.reg_matchbg_fastlearnrate);
	fprintf(stderr, "\tREG_0C.reg_matchbg_detchgregion = 0x%x\n", p->REG_0C.reg_matchbg_detchgregion);
	fprintf(stderr, "\tREG_10.reg_matchbg_stat_pixnum = 0x%x\n", p->REG_10.reg_matchbg_stat_pixnum);
	fprintf(stderr, "\tREG_14.reg_matchbg_stat_sumlum = 0x%x\n", p->REG_14.reg_matchbg_stat_sumlum);

}
#pragma GCC diagnostic pop
#define DEFINE_IVE_MATCH_BG_C(X) \
	 IVE_MATCH_BG_C X = \
{\
	{	/* REG_00.reg_matchbg_en = */0x0,\
	/*.REG_00.reg_matchbg_bypass_model = */0x0,\
	/*uint32_t rsv_2_3:2;=*/0,\
	/*.REG_00.reg_matchbg_softrst = */0x0,\
	},\
	{	/*.REG_04.reg_matchbg_curfrmnum = */0x0,\
	},\
	{	/*.REG_08.reg_matchbg_prefrmnum = */0x0,\
	},\
	{	/*.REG_0C.reg_matchbg_timethr = */0x14,\
	/*uint32_t rsv_7_7:1;=*/0,\
	/*.REG_0C.reg_matchbg_diffthrcrlcoef = */0x0,\
	/*uint32_t rsv_11_11:1;=*/0,\
	/*.REG_0C.reg_matchbg_diffmaxthr = */0x6,\
	/*.REG_0C.reg_matchbg_diffminthr = */0x4,\
	/*.REG_0C.reg_matchbg_diffthrinc = */0x0,\
	/*uint32_t rsv_23_23:1;=*/0,\
	/*.REG_0C.reg_matchbg_fastlearnrate = */0x2,\
	/*uint32_t rsv_27_27:1;=*/0,\
	/*.REG_0C.reg_matchbg_detchgregion = */0x0,\
	},\
	{	/*.REG_10.reg_matchbg_stat_pixnum = */0,\
	},\
	{	/*.REG_14.reg_matchbg_stat_sumlum = */0,\
	}\
}; 
#else /* !ifdef __cplusplus */ 
#define DEFINE_IVE_MATCH_BG_C(X) \
	 IVE_MATCH_BG_C X = \
{\
	.REG_00.reg_matchbg_en = 0x0,\
	.REG_00.reg_matchbg_bypass_model = 0x0,\
	.REG_00.reg_matchbg_softrst = 0x0,\
	.REG_04.reg_matchbg_curfrmnum = 0x0,\
	.REG_08.reg_matchbg_prefrmnum = 0x0,\
	.REG_0C.reg_matchbg_timethr = 0x14,\
	.REG_0C.reg_matchbg_diffthrcrlcoef = 0x0,\
	.REG_0C.reg_matchbg_diffmaxthr = 0x6,\
	.REG_0C.reg_matchbg_diffminthr = 0x4,\
	.REG_0C.reg_matchbg_diffthrinc = 0x0,\
	.REG_0C.reg_matchbg_fastlearnrate = 0x2,\
	.REG_0C.reg_matchbg_detchgregion = 0x0,\
};
#endif /* ifdef __cplusplus */ 
#endif //__REG_IVE_MATCH_BG_STRUCT_H__
