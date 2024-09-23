// $Module: isp_dma_ctl $
// $RegisterBank Version: V 1.0.00 $
// $Author: brian $
// $Date: Wed, 03 Nov 2021 05:09:34 PM $
//

#ifndef __REG_ISP_DMA_CTL_STRUCT_H__
#define __REG_ISP_DMA_CTL_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*Client QoS source selection
		0 : HW QoS (from client)
		1 : SW QoS (from DMA register);*/
		uint32_t reg_qos_sel:1;
		/*SW QoS setting;*/
		uint32_t reg_sw_qos:1;
		uint32_t rsv_2_7:6;
		/*Base address of memory layout (H part = bit32) , unit = byte;*/
		uint32_t reg_baseh:8;
		/*Base address selection
		0 : hw mode
		1 : sw mode;*/
		uint32_t reg_base_sel:1;
		/*Stride select:
		0 : hw mode
		1 : sw mode;*/
		uint32_t reg_stride_sel:1;
		/*seglen sel
		0 : hw mode
		1 : sw mode;*/
		uint32_t reg_seglen_sel:1;
		/*segnum select:
		0 : hw mode
		1 : sw mode;*/
		uint32_t reg_segnum_sel:1;
		/*slice mode enable;*/
		uint32_t reg_slice_enable:1;
		uint32_t rsv_21_27:7;
		/*reg_dbg_sel;*/
		uint32_t reg_dbg_sel:3;
	};
	uint32_t val;
} ISP_DMA_CTL_SYS_CONTROL_C;
typedef union {
	struct {
		/*Base address of memory layout (L part = bit31~0) , unit = byte
		limitation = 16-byte aligned;*/
		uint32_t reg_basel:32;
	};
	uint32_t val;
} ISP_DMA_CTL_BASE_ADDR_C;
typedef union {
	struct {
		/*Segment length, unit : byte
		limitation = 16-byte aligned;*/
		uint32_t reg_seglen:24;
	};
	uint32_t val;
} ISP_DMA_CTL_DMA_SEGLEN_C;
typedef union {
	struct {
		/*Stride length, the distance between two segments, unit : byte
		limitation = 16-byte aligned;*/
		uint32_t reg_stride:24;
	};
	uint32_t val;
} ISP_DMA_CTL_DMA_STRIDE_C;
typedef union {
	struct {
		/*Total segment number;*/
		uint32_t reg_segnum:13;
	};
	uint32_t val;
} ISP_DMA_CTL_DMA_SEGNUM_C;
typedef union {
	struct {
		/*DMA Status;*/
		uint32_t reg_status:32;
	};
	uint32_t val;
} ISP_DMA_CTL_DMA_STATUS_C;
typedef union {
	struct {
		/*slice buffer Size;*/
		uint32_t reg_slice_size:6;
	};
	uint32_t val;
} ISP_DMA_CTL_DMA_SLICESIZE_C;
typedef union {
	struct {
		/*dummy register;*/
		uint32_t reg_dummy:16;
	};
	uint32_t val;
} ISP_DMA_CTL_DMA_DUMMY_C;



typedef struct {
	ISP_DMA_CTL_SYS_CONTROL_C SYS_CONTROL;
	ISP_DMA_CTL_BASE_ADDR_C BASE_ADDR;
	ISP_DMA_CTL_DMA_SEGLEN_C DMA_SEGLEN;
	ISP_DMA_CTL_DMA_STRIDE_C DMA_STRIDE;
	ISP_DMA_CTL_DMA_SEGNUM_C DMA_SEGNUM;
	ISP_DMA_CTL_DMA_STATUS_C DMA_STATUS;
	ISP_DMA_CTL_DMA_SLICESIZE_C DMA_SLICESIZE;
	ISP_DMA_CTL_DMA_DUMMY_C DMA_DUMMY;
} ISP_DMA_CTL_C;



static void isp_dma_ctl_printk(ISP_DMA_CTL_C *p)
{
	pr_info("isp_dma_ctl\n");
	pr_info("\tSYS_CONTROL.reg_qos_sel = 0x%x\n",
		p->SYS_CONTROL.reg_qos_sel);
	pr_info("\tSYS_CONTROL.reg_sw_qos = 0x%x\n", p->SYS_CONTROL.reg_sw_qos);
	pr_info("\tSYS_CONTROL.reg_baseh = 0x%x\n", p->SYS_CONTROL.reg_baseh);
	pr_info("\tSYS_CONTROL.reg_base_sel = 0x%x\n",
		p->SYS_CONTROL.reg_base_sel);
	pr_info("\tSYS_CONTROL.reg_stride_sel = 0x%x\n",
		p->SYS_CONTROL.reg_stride_sel);
	pr_info("\tSYS_CONTROL.reg_seglen_sel = 0x%x\n",
		p->SYS_CONTROL.reg_seglen_sel);
	pr_info("\tSYS_CONTROL.reg_segnum_sel = 0x%x\n",
		p->SYS_CONTROL.reg_segnum_sel);
	pr_info("\tSYS_CONTROL.reg_slice_enable = 0x%x\n",
		p->SYS_CONTROL.reg_slice_enable);
	pr_info("\tSYS_CONTROL.reg_dbg_sel = 0x%x\n",
		p->SYS_CONTROL.reg_dbg_sel);
	pr_info("\tBASE_ADDR.reg_basel = 0x%x\n", p->BASE_ADDR.reg_basel);
	pr_info("\tDMA_SEGLEN.reg_seglen = 0x%x\n", p->DMA_SEGLEN.reg_seglen);
	pr_info("\tDMA_STRIDE.reg_stride = 0x%x\n", p->DMA_STRIDE.reg_stride);
	pr_info("\tDMA_SEGNUM.reg_segnum = 0x%x\n", p->DMA_SEGNUM.reg_segnum);
	pr_info("\tDMA_STATUS.reg_status = 0x%x\n", p->DMA_STATUS.reg_status);
	pr_info("\tDMA_SLICESIZE.reg_slice_size = 0x%x\n",
		p->DMA_SLICESIZE.reg_slice_size);
	pr_info("\tDMA_DUMMY.reg_dummy = 0x%x\n", p->DMA_DUMMY.reg_dummy);
}

#ifdef __cplusplus
#error "removed."
#else /* !ifdef __cplusplus */
#define _DEFINE_ISP_DMA_CTL_C \
	{\
		.SYS_CONTROL.reg_qos_sel = 0x0,\
		.SYS_CONTROL.reg_sw_qos = 0x0,\
		.SYS_CONTROL.reg_baseh = 0x0,\
		.SYS_CONTROL.reg_base_sel = 0x0,\
		.SYS_CONTROL.reg_stride_sel = 0x0,\
		.SYS_CONTROL.reg_seglen_sel = 0x0,\
		.SYS_CONTROL.reg_segnum_sel = 0x0,\
		.SYS_CONTROL.reg_slice_enable = 0x0,\
		.SYS_CONTROL.reg_dbg_sel = 0x0,\
		.BASE_ADDR.reg_basel = 0x0,\
		.DMA_SEGLEN.reg_seglen = 0x0,\
		.DMA_STRIDE.reg_stride = 0x0,\
		.DMA_SEGNUM.reg_segnum = 0x0,\
		.DMA_STATUS.reg_status = 0x0,\
		.DMA_SLICESIZE.reg_slice_size = 0x0,\
		.DMA_DUMMY.reg_dummy = 0x5a5a,\
	}
//#define DEFINE_ISP_DMA_CTL_C(X) ISP_DMA_CTL_C X = _DEFINE_ISP_DMA_CTL_C
#endif /* ifdef __cplusplus */
#endif //__REG_ISP_DMA_CTL_STRUCT_H__
