// $Module: reg_isp_dma_ctl $
// $RegisterBank Version: V 1.0.00 $
// $Author: brian $
// $Date: Wed, 03 Nov 2021 05:09:34 PM $
//

//GEN REG ADDR/OFFSET/MASK
#define  ISP_DMA_CTL_SYS_CONTROL  0x0
#define  ISP_DMA_CTL_BASE_ADDR  0x4
#define  ISP_DMA_CTL_DMA_SEGLEN  0x8
#define  ISP_DMA_CTL_DMA_STRIDE  0xc
#define  ISP_DMA_CTL_DMA_SEGNUM  0x10
#define  ISP_DMA_CTL_DMA_STATUS  0x14
#define  ISP_DMA_CTL_DMA_SLICESIZE  0x18
#define  ISP_DMA_CTL_DMA_DUMMY  0x1c
#define  ISP_DMA_CTL_REG_QOS_SEL   0x0
#define  ISP_DMA_CTL_REG_QOS_SEL_OFFSET 0
#define  ISP_DMA_CTL_REG_QOS_SEL_MASK   0x1
#define  ISP_DMA_CTL_REG_QOS_SEL_BITS   0x1
#define  ISP_DMA_CTL_REG_SW_QOS   0x0
#define  ISP_DMA_CTL_REG_SW_QOS_OFFSET 1
#define  ISP_DMA_CTL_REG_SW_QOS_MASK   0x2
#define  ISP_DMA_CTL_REG_SW_QOS_BITS   0x1
#define  ISP_DMA_CTL_REG_BASEH   0x0
#define  ISP_DMA_CTL_REG_BASEH_OFFSET 8
#define  ISP_DMA_CTL_REG_BASEH_MASK   0xff00
#define  ISP_DMA_CTL_REG_BASEH_BITS   0x8
#define  ISP_DMA_CTL_REG_BASE_SEL   0x0
#define  ISP_DMA_CTL_REG_BASE_SEL_OFFSET 16
#define  ISP_DMA_CTL_REG_BASE_SEL_MASK   0x10000
#define  ISP_DMA_CTL_REG_BASE_SEL_BITS   0x1
#define  ISP_DMA_CTL_REG_STRIDE_SEL   0x0
#define  ISP_DMA_CTL_REG_STRIDE_SEL_OFFSET 17
#define  ISP_DMA_CTL_REG_STRIDE_SEL_MASK   0x20000
#define  ISP_DMA_CTL_REG_STRIDE_SEL_BITS   0x1
#define  ISP_DMA_CTL_REG_SEGLEN_SEL   0x0
#define  ISP_DMA_CTL_REG_SEGLEN_SEL_OFFSET 18
#define  ISP_DMA_CTL_REG_SEGLEN_SEL_MASK   0x40000
#define  ISP_DMA_CTL_REG_SEGLEN_SEL_BITS   0x1
#define  ISP_DMA_CTL_REG_SEGNUM_SEL   0x0
#define  ISP_DMA_CTL_REG_SEGNUM_SEL_OFFSET 19
#define  ISP_DMA_CTL_REG_SEGNUM_SEL_MASK   0x80000
#define  ISP_DMA_CTL_REG_SEGNUM_SEL_BITS   0x1
#define  ISP_DMA_CTL_REG_SLICE_ENABLE   0x0
#define  ISP_DMA_CTL_REG_SLICE_ENABLE_OFFSET 20
#define  ISP_DMA_CTL_REG_SLICE_ENABLE_MASK   0x100000
#define  ISP_DMA_CTL_REG_SLICE_ENABLE_BITS   0x1
#define  ISP_DMA_CTL_REG_DBG_SEL   0x0
#define  ISP_DMA_CTL_REG_DBG_SEL_OFFSET 28
#define  ISP_DMA_CTL_REG_DBG_SEL_MASK   0x70000000
#define  ISP_DMA_CTL_REG_DBG_SEL_BITS   0x3
#define  ISP_DMA_CTL_REG_BASEL   0x4
#define  ISP_DMA_CTL_REG_BASEL_OFFSET 0
#define  ISP_DMA_CTL_REG_BASEL_MASK   0xffffffff
#define  ISP_DMA_CTL_REG_BASEL_BITS   0x20
#define  ISP_DMA_CTL_REG_SEGLEN   0x8
#define  ISP_DMA_CTL_REG_SEGLEN_OFFSET 0
#define  ISP_DMA_CTL_REG_SEGLEN_MASK   0xffffff
#define  ISP_DMA_CTL_REG_SEGLEN_BITS   0x18
#define  ISP_DMA_CTL_REG_STRIDE   0xc
#define  ISP_DMA_CTL_REG_STRIDE_OFFSET 0
#define  ISP_DMA_CTL_REG_STRIDE_MASK   0xffffff
#define  ISP_DMA_CTL_REG_STRIDE_BITS   0x18
#define  ISP_DMA_CTL_REG_SEGNUM   0x10
#define  ISP_DMA_CTL_REG_SEGNUM_OFFSET 0
#define  ISP_DMA_CTL_REG_SEGNUM_MASK   0x1fff
#define  ISP_DMA_CTL_REG_SEGNUM_BITS   0xd
#define  ISP_DMA_CTL_REG_STATUS   0x14
#define  ISP_DMA_CTL_REG_STATUS_OFFSET 0
#define  ISP_DMA_CTL_REG_STATUS_MASK   0xffffffff
#define  ISP_DMA_CTL_REG_STATUS_BITS   0x20
#define  ISP_DMA_CTL_REG_SLICE_SIZE   0x18
#define  ISP_DMA_CTL_REG_SLICE_SIZE_OFFSET 0
#define  ISP_DMA_CTL_REG_SLICE_SIZE_MASK   0x3f
#define  ISP_DMA_CTL_REG_SLICE_SIZE_BITS   0x6
#define  ISP_DMA_CTL_REG_DUMMY   0x1c
#define  ISP_DMA_CTL_REG_DUMMY_OFFSET 0
#define  ISP_DMA_CTL_REG_DUMMY_MASK   0xffff
#define  ISP_DMA_CTL_REG_DUMMY_BITS   0x10
#define  ISP_DMA_CTL_REG_PERF_PATCH_ENABLE   0x1c
#define  ISP_DMA_CTL_REG_PERF_PATCH_ENABLE_OFFSET   16
#define  ISP_DMA_CTL_REG_PERF_PATCH_ENABLE_MASK 0x10000
#define  ISP_DMA_CTL_REG_PERF_PATCH_ENABLE_BITS 0x1
#define  ISP_DMA_CTL_REG_SEGLEN_LESS16_ENABLE   0x1c
#define  ISP_DMA_CTL_REG_SEGLEN_LESS16_ENABLE_OFFSET    17
#define  ISP_DMA_CTL_REG_SEGLEN_LESS16_ENABLE_MASK  0x20000
#define  ISP_DMA_CTL_REG_SEGLEN_LESS16_ENABLE_BITS  0x1
#define  ISP_DMA_CTL_REG_SYNC_PATCH_ENABLE  0x1c
#define  ISP_DMA_CTL_REG_SYNC_PATCH_ENABLE_OFFSET   18
#define  ISP_DMA_CTL_REG_SYNC_PATCH_ENABLE_MASK 0x40000
#define  ISP_DMA_CTL_REG_SYNC_PATCH_ENABLE_BITS 0x1
#define  ISP_DMA_CTL_REG_TRIG_PATCH_ENABLE  0x1c
#define  ISP_DMA_CTL_REG_TRIG_PATCH_ENABLE_OFFSET   19
#define  ISP_DMA_CTL_REG_TRIG_PATCH_ENABLE_MASK 0x80000
#define  ISP_DMA_CTL_REG_TRIG_PATCH_ENABLE_BITS 0x1
