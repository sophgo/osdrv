// $Module: isp_wdma $
// $RegisterBank Version: V 1.0.00 $
// $Author: brian $
// $Date: Sun, 26 Sep 2021 04:20:44 PM $
//

#ifndef __REG_ISP_WDMA_STRUCT_H__
#define __REG_ISP_WDMA_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*Shadow reg read select
		0 : read active reg
		1 : read shadow reg;*/
		uint32_t reg_shadow_rd_sel:1;
	};
	uint32_t val;
} ISP_WDMA_SHADOW_RD_SEL_C;
typedef union {
	struct {
		/*Disable IP;*/
		uint32_t reg_ip_disable:32;
	};
	uint32_t val;
} ISP_WDMA_IP_DISABLE_C;
typedef union {
	struct {
		/*Abort done flag;*/
		uint32_t reg_abort_done:1;
		uint32_t rsv_1_3:3;
		/*Error flag : AXI response error;*/
		uint32_t reg_error_axi:1;
		/*Error flag : DMI transfer size mismatch;*/
		uint32_t reg_error_dmi:1;
		/*Warning flag : WDMA Slot buffer full;*/
		uint32_t reg_slot_full:1;
		uint32_t rsv_7_7:1;
		/*Error client ID;*/
		uint32_t reg_error_id:5;
		uint32_t rsv_13_15:3;
		/*Date of latest update;*/
		uint32_t reg_dma_version:16;
	};
	uint32_t val;
} ISP_WDMA_NORM_STATUS0_C;
typedef union {
	struct {
		/*reg_id_done[id] : done status of client id 
		(WDMA has received all data from the client);*/
		uint32_t reg_id_done:32;
	};
	uint32_t val;
} ISP_WDMA_NORM_STATUS1_C;
typedef union {
	struct {
		/*Bandwidth limiter window size;*/
		uint32_t reg_bwlwin:10;
		/*Bandwidth limiter transaction number;*/
		uint32_t reg_bwltxn:6;
		/*QoS only mode threshold;*/
		uint32_t reg_qoso_th:4;
		/*QoS only mode enable;*/
		uint32_t reg_qoso_en:1;
	};
	uint32_t val;
} ISP_WDMA_NORM_PERF_C;
typedef struct {
	volatile ISP_WDMA_SHADOW_RD_SEL_C SHADOW_RD_SEL;
	volatile ISP_WDMA_IP_DISABLE_C IP_DISABLE;
	volatile uint32_t _NORM_STATUS0_0; // 0x08
	volatile uint32_t _NORM_STATUS0_1; // 0x0C
	volatile ISP_WDMA_NORM_STATUS0_C NORM_STATUS0;
	volatile ISP_WDMA_NORM_STATUS1_C NORM_STATUS1;
	volatile uint32_t _NORM_PERF_0; // 0x18
	volatile uint32_t _NORM_PERF_1; // 0x1C
	volatile ISP_WDMA_NORM_PERF_C NORM_PERF;
} ISP_WDMA_C;
#ifdef __cplusplus 

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void isp_wdma_dump_ini(FILE* fp, ISP_WDMA_C* p) {
	fprintf(fp, "reg_shadow_rd_sel = 0x%x\n",p->SHADOW_RD_SEL.reg_shadow_rd_sel);
	fprintf(fp, "reg_ip_disable = 0x%x\n",p->IP_DISABLE.reg_ip_disable);
	fprintf(fp, "reg_abort_done = 0x%x\n",p->NORM_STATUS0.reg_abort_done);
	fprintf(fp, "reg_error_axi = 0x%x\n",p->NORM_STATUS0.reg_error_axi);
	fprintf(fp, "reg_error_dmi = 0x%x\n",p->NORM_STATUS0.reg_error_dmi);
	fprintf(fp, "reg_slot_full = 0x%x\n",p->NORM_STATUS0.reg_slot_full);
	fprintf(fp, "reg_error_id = 0x%x\n",p->NORM_STATUS0.reg_error_id);
	fprintf(fp, "reg_dma_version = 0x%x\n",p->NORM_STATUS0.reg_dma_version);
	fprintf(fp, "reg_id_done = 0x%x\n",p->NORM_STATUS1.reg_id_done);
	fprintf(fp, "reg_bwlwin = 0x%x\n",p->NORM_PERF.reg_bwlwin);
	fprintf(fp, "reg_bwltxn = 0x%x\n",p->NORM_PERF.reg_bwltxn);
	fprintf(fp, "reg_qoso_th = 0x%x\n",p->NORM_PERF.reg_qoso_th);
	fprintf(fp, "reg_qoso_en = 0x%x\n",p->NORM_PERF.reg_qoso_en);

}
static void isp_wdma_print(ISP_WDMA_C* p) {
    fprintf(stderr, "isp_wdma\n");
	fprintf(stderr, "\tSHADOW_RD_SEL.reg_shadow_rd_sel = 0x%x\n", p->SHADOW_RD_SEL.reg_shadow_rd_sel);
	fprintf(stderr, "\tIP_DISABLE.reg_ip_disable = 0x%x\n", p->IP_DISABLE.reg_ip_disable);
	fprintf(stderr, "\tNORM_STATUS0.reg_abort_done = 0x%x\n", p->NORM_STATUS0.reg_abort_done);
	fprintf(stderr, "\tNORM_STATUS0.reg_error_axi = 0x%x\n", p->NORM_STATUS0.reg_error_axi);
	fprintf(stderr, "\tNORM_STATUS0.reg_error_dmi = 0x%x\n", p->NORM_STATUS0.reg_error_dmi);
	fprintf(stderr, "\tNORM_STATUS0.reg_slot_full = 0x%x\n", p->NORM_STATUS0.reg_slot_full);
	fprintf(stderr, "\tNORM_STATUS0.reg_error_id = 0x%x\n", p->NORM_STATUS0.reg_error_id);
	fprintf(stderr, "\tNORM_STATUS0.reg_dma_version = 0x%x\n", p->NORM_STATUS0.reg_dma_version);
	fprintf(stderr, "\tNORM_STATUS1.reg_id_done = 0x%x\n", p->NORM_STATUS1.reg_id_done);
	fprintf(stderr, "\tNORM_PERF.reg_bwlwin = 0x%x\n", p->NORM_PERF.reg_bwlwin);
	fprintf(stderr, "\tNORM_PERF.reg_bwltxn = 0x%x\n", p->NORM_PERF.reg_bwltxn);
	fprintf(stderr, "\tNORM_PERF.reg_qoso_th = 0x%x\n", p->NORM_PERF.reg_qoso_th);
	fprintf(stderr, "\tNORM_PERF.reg_qoso_en = 0x%x\n", p->NORM_PERF.reg_qoso_en);

}
#pragma GCC diagnostic pop
#define DEFINE_ISP_WDMA_C(X) \
	 ISP_WDMA_C X = \
{\
	{	/* SHADOW_RD_SEL.reg_shadow_rd_sel = */0x1,\
	},\
	{	/*.IP_DISABLE.reg_ip_disable = */0x0,\
	},\
	{	/*.NORM_STATUS0.reg_abort_done = */0x0,\
	/*uint32_t rsv_1_3:3;=*/0,\
	/*.NORM_STATUS0.reg_error_axi = */0x0,\
	/*.NORM_STATUS0.reg_error_dmi = */0x0,\
	/*.NORM_STATUS0.reg_slot_full = */0x0,\
	/*uint32_t rsv_7_7:1;=*/0,\
	/*.NORM_STATUS0.reg_error_id = */0x1f,\
	/*uint32_t rsv_13_15:3;=*/0,\
	/*.NORM_STATUS0.reg_dma_version = */0x0703,\
	},\
	{	/*.NORM_STATUS1.reg_id_done = */0x0,\
	},\
	{	/*.NORM_PERF.reg_bwlwin = */0x0,\
	/*.NORM_PERF.reg_bwltxn = */0x0,\
	/*.NORM_PERF.reg_qoso_th = */0x0,\
	/*.NORM_PERF.reg_qoso_en = */0x0,\
	}\
}; 
#else /* !ifdef __cplusplus */ 
#define DEFINE_ISP_WDMA_C(X) \
	 ISP_WDMA_C X = \
{\
	.SHADOW_RD_SEL.reg_shadow_rd_sel = 0x1,\
	.IP_DISABLE.reg_ip_disable = 0x0,\
	.NORM_STATUS0.reg_abort_done = 0x0,\
	.NORM_STATUS0.reg_error_axi = 0x0,\
	.NORM_STATUS0.reg_error_dmi = 0x0,\
	.NORM_STATUS0.reg_slot_full = 0x0,\
	.NORM_STATUS0.reg_error_id = 0x1f,\
	.NORM_STATUS0.reg_dma_version = 0x0703,\
	.NORM_STATUS1.reg_id_done = 0x0,\
	.NORM_PERF.reg_bwlwin = 0x0,\
	.NORM_PERF.reg_bwltxn = 0x0,\
	.NORM_PERF.reg_qoso_th = 0x0,\
	.NORM_PERF.reg_qoso_en = 0x0,\
};
#endif /* ifdef __cplusplus */ 
#endif //__REG_ISP_WDMA_STRUCT_H__
