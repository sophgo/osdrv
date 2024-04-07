#include "dpu.h"
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <uapi/linux/sched/types.h>

#include <linux/cvi_comm_video.h>

#include "base_common.h"
#include <base_cb.h>
#include <vb.h>
#include <sys.h>
#include "dpu.h"
#include "vbq.h"
#include "bind.h"
#include "ion.h"

#include <linux/delay.h>

#include <linux/kernel.h>

#define IDLE_TIMEOUT_MS      30000
#define EOF_WAIT_TIMEOUT_MS  10000
#define HW_WAIT_TIMEOUT_MS  35


#define CTX_EVENT_WKUP       0x0001
#define CTX_EVENT_EOF        0x0002
#define CTX_EVENT_ERR        0x0004
#define ALIGN_4                  4
#define ALIGN_8                  8
#define ALIGN_16                 16
#define ALIGN_32                 32
#define ALIGN_64                 64
extern s32 hw_wait_time;
static u64 reg_base;
static u64 reg_base_sgbm_ld1_dma;
static u64 reg_base_sgbm_ld2_dma;
static u64 reg_base_sgbm_median_dma;
static u64 reg_base_sgbm_bf_dma;
static u64 reg_base_fgs_gx_dma;
static u64 reg_base_fgs_ux_dma;
static u64 reg_base_fgs_chfh_st_dma;
static u64 reg_base_fgs_chfh_ld_dma;

static u32 dram_base_out_l;
static u32 dram_base_out_h;
static u32 dram_base_out_btcost_l;
static u32 dram_base_out_btcost_h;
static u32 dram_base_left_l;
static u32 dram_base_left_h;
static u32 dram_base_right_l;
static u32 dram_base_right_h;
static u32 dram_base_chfh_l;
static u32 dram_base_chfh_h;

static unsigned long timeout;

static struct cvi_dpu_dev *dpu_dev;
static CVI_DPU_REG cvi_dpu_reg;
static struct cvi_dpu_ctx *dpuCtx[DPU_MAX_GRP_NUM] = { [0 ... DPU_MAX_GRP_NUM - 1] = NULL };

//Get Available Grp lock
static struct mutex dpuGetGrpLock;
static u8 dpuGrpUsed[DPU_MAX_GRP_NUM];

struct dpu_jobs_ctx {
	struct vb_jobs_t ins[DPU_PIPE_IN_NUM];
	struct vb_jobs_t outs[DPU_MAX_CHN_NUM];
	struct mutex lock;
} gstDpuJobs[DPU_MAX_GRP_NUM];

struct dpu_handler_ctx {
	u8 u8DpuDev;	// index of handler_ctx
	enum handler_state enHdlState;

	DPU_GRP workingGrp;
	u8 workingMask;
	u8 IntMask;
	u32 events;
	struct timespec64 time;
	struct mutex mutex;
	DPU_MODE_E enDpuMode;
	// struct cvi_dpu_ctx dpuCtxWork[1];

} handler_ctx[DPU_IP_NUM];

struct cvi_dpu_dev *dpu_get_dev(void)
{
	return dpu_dev;
}
//EXPORT_SYMBOL_GPL(dpu_get_dev);

static void print_vbq_size(DPU_GRP dpuGrp)
{
	CVI_TRACE_DPU(CVI_DBG_INFO,"gstDpuJobs.ins[0].vbqSize: waitq(%d),workq(%d),doneq(%d)\n",
				FIFO_SIZE(&gstDpuJobs[dpuGrp].ins[0].waitq),
				FIFO_SIZE(&gstDpuJobs[dpuGrp].ins[0].workq),
				FIFO_SIZE(&gstDpuJobs[dpuGrp].ins[0].doneq));
	CVI_TRACE_DPU(CVI_DBG_INFO,"gstDpuJobs.ins[1].vbqSize: waitq(%d),workq(%d),doneq(%d)\n",
				FIFO_SIZE(&gstDpuJobs[dpuGrp].ins[1].waitq),
				FIFO_SIZE(&gstDpuJobs[dpuGrp].ins[1].workq),
				FIFO_SIZE(&gstDpuJobs[dpuGrp].ins[1].doneq));
	CVI_TRACE_DPU(CVI_DBG_INFO,"gstDpuJobs.outs[0].vbqSize: waitq(%d),workq(%d),doneq(%d)\n",
				FIFO_SIZE(&gstDpuJobs[dpuGrp].outs[0].waitq),
				FIFO_SIZE(&gstDpuJobs[dpuGrp].outs[0].workq),
				FIFO_SIZE(&gstDpuJobs[dpuGrp].outs[0].doneq));
}

static u32 get_mask(u32 src,u32 bits ,u32 start_LSB){
    return ((((1 << bits) - 1) << start_LSB) & src) >> start_LSB;
}

s32 dpu_fill_videoframe2buffer(MMF_CHN_S chn, const VIDEO_FRAME_INFO_S *pstVideoFrame,
	struct cvi_buffer *buf)
{
	u32 plane_size;
	VB_CAL_CONFIG_S stVbCalConfig;
	u8 i = 0;

	COMMON_GetPicBufferConfig(pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.u32Height,
		pstVideoFrame->stVFrame.enPixelFormat, DATA_BITWIDTH_8, COMPRESS_MODE_NONE,
		ALIGN_16, &stVbCalConfig);

	buf->size.u32Width = pstVideoFrame->stVFrame.u32Width;
	buf->size.u32Height = pstVideoFrame->stVFrame.u32Height;
	buf->enPixelFormat = pstVideoFrame->stVFrame.enPixelFormat;
	buf->s16OffsetLeft = pstVideoFrame->stVFrame.s16OffsetLeft;
	buf->s16OffsetTop = pstVideoFrame->stVFrame.s16OffsetTop;
	buf->s16OffsetRight = pstVideoFrame->stVFrame.s16OffsetRight;
	buf->s16OffsetBottom = pstVideoFrame->stVFrame.s16OffsetBottom;
	buf->frm_num = pstVideoFrame->stVFrame.u32TimeRef;
	buf->u64PTS = pstVideoFrame->stVFrame.u64PTS;
	memset(&buf->frame_crop, 0, sizeof(buf->frame_crop));

	for (i = 0; i < NUM_OF_PLANES; ++i) {
		if (i >= stVbCalConfig.plane_num) {
			buf->phy_addr[i] = 0;
			buf->length[i] = 0;
			buf->stride[i] = 0;
			continue;
		}

		plane_size = (i == 0) ? stVbCalConfig.u32MainYSize : stVbCalConfig.u32MainCSize;
		buf->phy_addr[i] = pstVideoFrame->stVFrame.u64PhyAddr[i];
		buf->length[i] = pstVideoFrame->stVFrame.u32Length[i];
		buf->stride[i] = pstVideoFrame->stVFrame.u32Stride[i];
		if (buf->length[i] < plane_size) {
			pr_err("Mod(%s) Dev(%d) Chn(%d) Plane[%d]\n"
				, sys_get_modname(chn.enModId), chn.s32DevId, chn.s32ChnId, i);
			pr_err(" length(%zu) less than expected(%d).\n"
				, buf->length[i], plane_size);
			return CVI_FAILURE;
		}
		if (buf->stride[i] % ALIGN_16) {
			pr_err("Mod(%s) Dev(%d) Chn(%d) Plane[%d]\n"
				, sys_get_modname(chn.enModId), chn.s32DevId, chn.s32ChnId, i);
			pr_err(" stride(%d) not aligned(%d).\n"
				, buf->stride[i], ALIGN_16);
			return CVI_FAILURE;
		}
		if (buf->phy_addr[i] % ALIGN_64) {
			pr_err("Mod(%s) Dev(%d) Chn(%d) Plane[%d]\n"
				, sys_get_modname(chn.enModId), chn.s32DevId, chn.s32ChnId, i);
			pr_err(" address(%llx) not aligned(%d).\n"
				, buf->phy_addr[i], ALIGN_64);
			return CVI_FAILURE;
		}
	}
	// [WA-01]
	if (stVbCalConfig.plane_num > 1) {
		if (((buf->phy_addr[0] & (stVbCalConfig.u16AddrAlign - 1))
		    != (buf->phy_addr[1] & (stVbCalConfig.u16AddrAlign - 1)))
		 || ((buf->phy_addr[0] & (stVbCalConfig.u16AddrAlign - 1))
		    != (buf->phy_addr[2] & (stVbCalConfig.u16AddrAlign - 1)))) {
			pr_err("Mod(%s) Dev(%d) Chn(%d)\n"
				, sys_get_modname(chn.enModId), chn.s32DevId, chn.s32ChnId);
			pr_err("plane address offset (%llx-%llx-%llx)"
				, buf->phy_addr[0], buf->phy_addr[1], buf->phy_addr[2]);
			pr_err("not aligned to %#x.\n", stVbCalConfig.u16AddrAlign);
			return CVI_FAILURE;
		}
	}
	return CVI_SUCCESS;
}

void register_sgbm_ld1_ld(u32 seg_len, u32 seg_num,u32 SRAM_DPU_BASE_H, \
							u32 SGBM_LEFT_IMG_ADDR,u64 dmaBaseAddr, u64 regBaseAddr)
{
    //u32 addr =DPU_ALIGN(SGBM_LEFT_IMG_ADDR,ADDR_ALIGN);
	u32 addr=SGBM_LEFT_IMG_ADDR;
    u32 width =seg_len;
    u32 width_align = DPU_ALIGN(seg_len,ALIGN_16);
    u32 height = seg_num ;

    u32 sgbm_ld_crop_h_end = height -1;
    u32 sgbm_ld_crop_h_str = 0;
    u32 sgbm_ld_crop_w_end = width -1;
    u32 sgbm_ld_crop_w_str = 0;
    u32 sgbm_ld_crop_height = height -1 ;
    u32 sgbm_ld_crop_width = width_align -1;

    u32 reg_2C = sgbm_ld_crop_h_end | (sgbm_ld_crop_h_str << 16) ;
    u32 reg_30 = sgbm_ld_crop_w_end | (sgbm_ld_crop_w_str << 16) ;
    u32 reg_34 = sgbm_ld_crop_height | (sgbm_ld_crop_width << 16) ;

	reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<8)-1) <<8,SRAM_DPU_BASE_H << 8);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<16,1 << 16);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<17,1 << 17);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<18,1 << 18);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<19,1 << 19);

	write_reg(dmaBaseAddr + DMA_BASE_ADDR_OFS, addr);
    // bit reg_seglen
    write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, width_align);
    //write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, seg_len);
    // reg_segnum
    write_reg(dmaBaseAddr + DMA_SEGNUM_OFS, height);
    // reg_stride
    write_reg(dmaBaseAddr + DMA_STRIDE_OFS, width_align);

    write_reg(regBaseAddr + DPU_REG_2C_OFS,reg_2C);
    write_reg(regBaseAddr + DPU_REG_30_OFS,reg_30);
    write_reg(regBaseAddr + DPU_REG_34_OFS,reg_34);

}

void register_sgbm_ld2_ld(u32 seg_len, u32 seg_num,u32 SRAM_DPU_BASE_H,\
						    u32 SGBM_RIGHT_IMG_ADDR,u64 dmaBaseAddr,u64 regBaseAddr)
{
	u32 stride;

    //u32 addr =DPU_ALIGN(SGBM_RIGHT_IMG_ADDR,ADDR_ALIGN);
    u32 addr= SGBM_RIGHT_IMG_ADDR;
    u32 width_align = DPU_ALIGN(seg_len,ALIGN_16);
	reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<8)-1) <<8,SRAM_DPU_BASE_H << 8);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<16,1 << 16);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<17,1 << 17);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<18,1 << 18);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<19,1 << 19);

    write_reg(dmaBaseAddr + DMA_BASE_ADDR_OFS, addr);

    // bit reg_seglen
    write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, width_align);
    //write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, seg_len);

    // reg_segnum
    write_reg(dmaBaseAddr + DMA_SEGNUM_OFS, seg_num);

    // reg_stride
    stride =DPU_ALIGN(seg_len,ALIGN_16);
    write_reg(dmaBaseAddr + DMA_STRIDE_OFS, stride);
}

void register_sgbm_bf_st_ld(u32 seg_len, u32 seg_num,u32 SRAM_DPU_BASE_H, \
								u32 SGBM_BTCOST_ST_ADDR,u64 dmaBaseAddr,u64 regBaseAddr)
{
	u32 reg_20 ;
    u32 reg_24 ;
    u32 reg_28 ;


    //u32 addr =DPU_ALIGN(SGBM_BTCOST_ST_ADDR,ADDR_ALIGN);
	u32 addr=SGBM_BTCOST_ST_ADDR;

    u32 width = (seg_len * 16);
    u32 height = seg_num ;
    u32 sgbm_bf_st_crop_h_end = height-1;
    u32 sgbm_bf_st_crop_h_str = 0;
    u32 sgbm_bf_st_crop_w_end = width-1;
    u32 sgbm_bf_st_crop_w_str = 0;
    u32 sgbm_bf_st_crop_height = height-1;
    u32 sgbm_bf_st_crop_width = width-1;
    seg_len = seg_len * 256;
	write_reg(dmaBaseAddr + DMA_BASE_ADDR_OFS, addr);
	reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<8)-1) <<8,SRAM_DPU_BASE_H << 8);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<16,1 << 16);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<17,1 << 17);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<18,1 << 18);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<19,1 << 19);
    // bit reg_seglen
    write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, seg_len);

    // reg_segnum
    write_reg(dmaBaseAddr + DMA_SEGNUM_OFS, seg_num);

    // reg_stride
    write_reg(dmaBaseAddr + DMA_STRIDE_OFS, seg_len);

    reg_20 = sgbm_bf_st_crop_h_end | (sgbm_bf_st_crop_h_str << 16) ;
    reg_24 = sgbm_bf_st_crop_w_end | (sgbm_bf_st_crop_w_str << 16) ;
    reg_28 = sgbm_bf_st_crop_height | (sgbm_bf_st_crop_width << 16) ;

    write_reg(regBaseAddr + DPU_REG_20_OFS,reg_20);
    write_reg(regBaseAddr + DPU_REG_24_OFS,reg_24);
    write_reg(regBaseAddr + DPU_REG_28_OFS,reg_28);

}

void register_sgbm_median_st_ld(u32 seg_len, u32 seg_num,u32 chooseDma,u32 SRAM_DPU_BASE_H, \
									u32 SGBM_MEDIAN_ST_ADDR,u64 dmaBaseAddr,u64 regBaseAddr)
{
	u32 reg_38 ;
    u32 reg_3C ;
    u32 reg_40 ;

    //u32 addr =DPU_ALIGN(SGBM_MEDIAN_ST_ADDR,ADDR_ALIGN);
	u32 addr=SGBM_MEDIAN_ST_ADDR;

    u32 width = seg_len;
    u32 width_align =DPU_ALIGN(seg_len,ALIGN_16);
    u32 height = seg_num ;
    u32 sgbm_mux_st_crop_h_end = height -1;
    u32 sgbm_mux_st_crop_h_str = 0;
    u32 sgbm_mux_st_crop_w_end = width -1;
    u32 sgbm_mux_st_crop_w_str = 0;
    u32 sgbm_mux_st_crop_height = height -1;
    u32 sgbm_mux_st_crop_width = width_align -1;

	write_reg(dmaBaseAddr + DMA_BASE_ADDR_OFS, addr);
	reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<8)-1) <<8,SRAM_DPU_BASE_H << 8);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<16,1 << 16);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<17,1 << 17);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<18,1 << 18);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<19,1 << 19);
    if(chooseDma == 0){
        sgbm_mux_st_crop_width = width_align/2 -1;
        sgbm_mux_st_crop_w_end = seg_len/2 -1;
    } else if(chooseDma == 1) {
        width_align= width_align*2;

    } else if(chooseDma == 2){
        sgbm_mux_st_crop_width = width_align/2 -1;
        sgbm_mux_st_crop_w_end = seg_len/2 -1;
    } else{
        CVI_TRACE_DPU(CVI_DBG_ERR,"data_sel: not in 1-3");
    }
    CVI_TRACE_DPU(CVI_DBG_INFO,"align_width:[%d]\n",width_align);
    CVI_TRACE_DPU(CVI_DBG_INFO,"data_sel:[%d]\n",chooseDma);
    // bit reg_seglen
    write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, width_align);
    //write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, seg_len);

    // reg_segnum
    write_reg(dmaBaseAddr + DMA_SEGNUM_OFS, seg_num);

    // reg_stride
    write_reg(dmaBaseAddr + DMA_STRIDE_OFS, width_align);

    reg_38 = sgbm_mux_st_crop_h_end | (sgbm_mux_st_crop_h_str << 16) ;
    reg_3C = sgbm_mux_st_crop_w_end | (sgbm_mux_st_crop_w_str << 16) ;
    reg_40 = sgbm_mux_st_crop_height | (sgbm_mux_st_crop_width << 16) ;

    write_reg(regBaseAddr + DPU_REG_38_OFS,reg_38);
    write_reg(regBaseAddr + DPU_REG_3C_OFS,reg_3C);
    write_reg(regBaseAddr + DPU_REG_40_OFS,reg_40);
}


//rdma--read from bottom to top
void register_fgs_chfh_ld(u32 seg_len, u32 seg_num, u32 SRAM_DPU_BASE_H, \
							u32 FGS_CHFH_LD_ADDR,u64 dmaBaseAddr,u64 regBaseAddr){
    //reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS, REG_ENABLE_INV_MASK, reg_enable_inv);
	u32 reg_44 ;
    u32 reg_48 ;
    u32 reg_4C ;
	u32 stride ;

    //u32 addr =DPU_ALIGN(FGS_CHFH_LD_ADDR,ADDR_ALIGN);
	u32 addr =FGS_CHFH_LD_ADDR;

    u32 width =seg_len;
    u32 width_align = DPU_ALIGN(seg_len,ALIGN_16);
    u32 height = seg_num ;

    u32 fgs_chfh_crop_h_end = height -1;
    u32 fgs_chfh_crop_h_str = 0;
    u32 fgs_chfh_crop_w_end = width -1;
    u32 fgs_chfh_crop_w_str = 0;
    u32 fgs_chfh_crop_height = height -1;
    u32 fgs_chfh_crop_width = width_align -1;
	write_reg(dmaBaseAddr + DMA_BASE_ADDR_OFS, addr);
	reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<8)-1) <<8,SRAM_DPU_BASE_H << 8);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<16,0 << 16);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<17,0 << 17);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<18,0 << 18);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<19,0 << 19);

    // bit reg_seglen
    write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, width_align);
    //write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, seg_len);

    // reg_segnum
    write_reg(dmaBaseAddr + DMA_SEGNUM_OFS, seg_num);

    // reg_stride
    stride =DPU_ALIGN(seg_len,ALIGN_16);
    write_reg(dmaBaseAddr + DMA_STRIDE_OFS, stride);

    reg_44 = fgs_chfh_crop_h_end | (fgs_chfh_crop_h_str << 16) ;
    reg_48 = fgs_chfh_crop_w_end | (fgs_chfh_crop_w_str << 16) ;
    reg_4C = fgs_chfh_crop_height | (fgs_chfh_crop_width << 16) ;

    write_reg(regBaseAddr + DPU_REG_44_OFS,reg_44);  //fgs_chfh
    write_reg(regBaseAddr + DPU_REG_48_OFS,reg_48);
    write_reg(regBaseAddr + DPU_REG_4C_OFS,reg_4C);

    write_reg(regBaseAddr + DPU_REG_7C_OFS,reg_44);  //independent
    write_reg(regBaseAddr + DPU_REG_80_OFS,reg_48);
    write_reg(regBaseAddr + DPU_REG_84_OFS,reg_4C);

    reg_write_mask(regBaseAddr + DPU_REG_98_OFS,((1<<4)-1) <<8,SRAM_DPU_BASE_H << 8);
    write_reg(regBaseAddr + DPU_REG_9C_OFS, addr);

}

//rdma
void register_fgs_gx_ld(u32 seg_len, u32 seg_num, u32 SRAM_DPU_BASE_H, \
						u32 FGS_GX_LD_ADDR,u64 dmaBaseAddr,u64 regBaseAddr){
    //u32 addr =DPU_ALIGN(FGS_GX_LD_ADDR,ADDR_ALIGN);
	u32 reg_50 ;
    u32 reg_54 ;
    u32 reg_58 ;
	u32 stride ;
	u32 addr=FGS_GX_LD_ADDR;
    u32 width =seg_len;
    u32 width_align = DPU_ALIGN(seg_len,ALIGN_16);
    u32 height = seg_num ;

    u32 fgs_gx_crop_h_end = height-1;
    u32 fgs_gx_crop_h_str = 0;
    u32 fgs_gx_crop_w_end = width-1;
    u32 fgs_gx_crop_w_str = 0;
    u32 fgs_gx_crop_height = height-1;
    u32 fgs_gx_crop_width = width_align-1;
	write_reg(dmaBaseAddr + DMA_BASE_ADDR_OFS, addr);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<16,1 << 16);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<17,0 << 17);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<18,0 << 18);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<19,0 << 19);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<8)-1) <<8,SRAM_DPU_BASE_H << 8);
    // bit reg_seglen
    write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, width_align);
    //write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, seg_len);

    // reg_segnum
    write_reg(dmaBaseAddr + DMA_SEGNUM_OFS, seg_num);

    // reg_stride
    stride =DPU_ALIGN(seg_len,ALIGN_16);
    write_reg(dmaBaseAddr + DMA_STRIDE_OFS, stride);

    reg_50 = fgs_gx_crop_h_end | (fgs_gx_crop_h_str << 16) ;
    reg_54 = fgs_gx_crop_w_end | (fgs_gx_crop_w_str << 16) ;
    reg_58 = fgs_gx_crop_height | (fgs_gx_crop_width << 16) ;

    write_reg(regBaseAddr + DPU_REG_50_OFS,reg_50);
    write_reg(regBaseAddr + DPU_REG_54_OFS,reg_54);
    write_reg(regBaseAddr + DPU_REG_58_OFS,reg_58);
}

//wdma
void register_fgs_chfh_st(u32 seg_len, u32 seg_num, u32 SRAM_DPU_BASE_H, \
							u32 FGS_CHFH_ST_ADDR,u64 dmaBaseAddr,u64 regBaseAddr){
    //u32 addr =DPU_ALIGN(FGS_CHFH_ST_ADDR,ADDR_ALIGN);
	u32 stride;
	u32 addr=FGS_CHFH_ST_ADDR;
    write_reg(dmaBaseAddr + DMA_BASE_ADDR_OFS, addr);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<8)-1) <<8,SRAM_DPU_BASE_H << 8);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<16,1 << 16);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<17,0 << 17);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<18,0 << 18);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<19,0 << 19);
    stride =DPU_ALIGN(seg_len,ALIGN_16);
    // bit reg_seglen
    write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, stride);
    //write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, seg_len);

    // reg_segnum
    write_reg(dmaBaseAddr + DMA_SEGNUM_OFS, seg_num);

    // reg_stride

    write_reg(dmaBaseAddr + DMA_STRIDE_OFS, stride);

    reg_write_mask(regBaseAddr + DPU_REG_98_OFS,((1<<4)-1) <<12,SRAM_DPU_BASE_H << 12);
    write_reg(regBaseAddr + DPU_REG_A0_OFS, addr);
}

//wdma
void register_fgs_ux_st(u32 seg_len, u32 seg_num,u32 chooseChn, u32 SRAM_DPU_BASE_H,\
							u32 FGS_UX_ST_ADDR,u64 dmaBaseAddr,u64 regBaseAddr)
{
    //u32 addr =DPU_ALIGN(FGS_UX_ST_ADDR,ADDR_ALIGN);
	u32 stride;
	u32 reg_5C ;
    u32 reg_60 ;
    u32 reg_64 ;
	u32 addr=FGS_UX_ST_ADDR;

    u32 width =seg_len;
    u32 height = seg_num ;

    u32 fgs_ux_crop_h_end = height -1;
    u32 fgs_ux_crop_h_str = 0;
    u32 fgs_ux_crop_w_end = width/2 -1;
    u32 fgs_ux_crop_w_str = 0;
    u32 fgs_ux_crop_height = height-1 ;
    u32 fgs_ux_crop_width = DPU_ALIGN(width/2,16) -1;
    if(chooseChn == 1){
        fgs_ux_crop_w_end = width -1;
        fgs_ux_crop_width= DPU_ALIGN(width,32) -1;
    }

    stride =DPU_ALIGN(seg_len,32);

	write_reg(dmaBaseAddr + DMA_BASE_ADDR_OFS, addr);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<8)-1) <<8,SRAM_DPU_BASE_H << 8);

    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<16,1 << 16);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<17,0 << 17);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<18,0 << 18);
    reg_write_mask(dmaBaseAddr + SYS_CONTROL_OFS,((1<<1)-1) <<19,0 << 19);
    // bit reg_seglen
    //write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, seg_len);
    write_reg(dmaBaseAddr + DMA_SEGLEN_OFS, stride);

    // reg_segnum
    write_reg(dmaBaseAddr + DMA_SEGNUM_OFS, seg_num);

    // reg_stride

    write_reg(dmaBaseAddr + DMA_STRIDE_OFS, stride);

    reg_5C = fgs_ux_crop_h_end | (fgs_ux_crop_h_str << 16) ;
    reg_60 = fgs_ux_crop_w_end | (fgs_ux_crop_w_str << 16) ;
    reg_64 = fgs_ux_crop_height | (fgs_ux_crop_width << 16) ;

    write_reg(regBaseAddr + DPU_REG_5C_OFS,reg_5C);
    write_reg(regBaseAddr + DPU_REG_60_OFS,reg_60);
    write_reg(regBaseAddr + DPU_REG_64_OFS,reg_64);
}

void getsgbm_status(void)
{
	u32 reg_18,reg_68;
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print]Read sgbm debug status ...\n");
    reg_18 = read_reg(reg_base + DPU_REG_18_OFS);
	reg_68 = read_reg(reg_base + DPU_REG_68_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_18]:  %d\n", reg_18);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][sgbm_line_cnt]:  %d\n", get_mask(reg_18, 11, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][sgbm_pre_boxfilter_busy]:  %d\n", get_mask(reg_18, 1, 11));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][sgbm_boxfilter_busy]:  %d\n", get_mask(reg_18, 1, 12));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][sgbm_dcc_busy]:  %d\n", get_mask(reg_18, 1, 13));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][sgbm_wta_busy]:  %d\n", get_mask(reg_18, 1, 14));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][sgbm_dispinterp_busy]:  %d\n", get_mask(reg_18, 1, 15));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][sgbm_median_busy]:  %d\n", get_mask(reg_18, 1, 16));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][sgbm_busy]:  %d\n", get_mask(reg_18, 1, 17));

    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][sgbm_ld1_dma_idle]:  %d\n", get_mask(reg_18, 1, 18));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][sgbm_ld2_dma_idle]:  %d\n", get_mask(reg_18, 1, 19));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][sgbm_mux_st_dma_idle]:  %d\n", get_mask(reg_18, 1, 20));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][sgbm_boxfilter_st_dma_idle]:  %d\n", get_mask(reg_18, 1, 21));

	CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_sgbm_frame_done]:  %d\n", get_mask(reg_68, 1, 1));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print]Read sgbm debug status done ...\n");
}

void getfgs_status(void)
{
	u32 reg_18;
	u32 reg_8c,reg_68;
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print]Read fgs debug status ...\n");
    reg_18 = read_reg(reg_base + DPU_REG_18_OFS);
    reg_8c = read_reg(reg_base + DPU_REG_8C_OFS);
	reg_68 = read_reg(reg_base + DPU_REG_68_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_18]:  %d\n", reg_18);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][fgs_dma_idle_r_gx]:  %d\n", get_mask(reg_18, 1, 22));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][fgs_dma_idle_r_chfh]:  %d\n", get_mask(reg_18, 1, 23));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][fgs_dma_idle_w_ux]:  %d\n", get_mask(reg_18, 1, 24));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][fgs_dma_idle_w_chfh]:  %d\n", get_mask(reg_18, 1, 25));

    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][fgs_circle_cnt]:  %d\n", get_mask(reg_8c, 8, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][fgs_line_cnt]:  %d\n", get_mask(reg_8c, 14, 8));
	CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_fgs_frame_done]:  %d\n", get_mask(reg_68, 1, 11));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print]Read fgs debug status done ...\n");
}


static inline s32 check_dpu_grp_created(DPU_GRP grp)
{
	if (!dpuCtx[grp] || !dpuCtx[grp]->isCreated) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) isn't created yet.\n", grp);
		return CVI_ERR_DPU_UNEXIST;
	}
	return CVI_SUCCESS;
}

static inline s32 check_dpu_grp_valid(DPU_GRP DpuGrp)
{
	if ((DpuGrp >= DPU_MAX_GRP_NUM) || (DpuGrp < 0)) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) invalid .\n",
					DpuGrp);
		return CVI_ERR_DPU_ILLEGAL_PARAM;
	}
	return CVI_SUCCESS;
}

static inline s32 check_dpu_chn_valid(DPU_GRP DpuGrp, DPU_CHN DpuChn)
{
	if ((DpuChn >= DPU_MAX_CHN_NUM) || (DpuChn < 0)) {
	CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) Chn-out(%d) invalid .\n",
				DpuGrp, DpuChn);
	return CVI_ERR_DPU_ILLEGAL_PARAM;
	}

	return CVI_SUCCESS;
}

s32 check_dpu_id(DPU_GRP DpuGrp, DPU_CHN DpuChn)
{
	s32 ret = CVI_SUCCESS;

	ret = check_dpu_grp_valid(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_grp_created(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_chn_valid(DpuGrp, DpuChn);
	if (ret != CVI_SUCCESS)
		return ret;
	return ret;
}

void dpu_notify_wkup_evt(u8 u8DpuDev)
{
	if (u8DpuDev >= DPU_IP_NUM) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "invalid dev(%d)\n", u8DpuDev);
		return;
	}
	// mutex_lock(&handler_ctx[u8DpuDev].mutex);
	handler_ctx[u8DpuDev].events |= CTX_EVENT_WKUP;
	// mutex_unlock(&handler_ctx[u8DpuDev].mutex);
	CVI_TRACE_DPU(CVI_DBG_INFO, "wake up IP(%d) event(%d)\n",u8DpuDev,handler_ctx[u8DpuDev].events);
	wake_up_interruptible(&dpu_dev->wait);
}

void dpu_notify_isr_evt(void)
{
	u8 i;

	for (i = 0; i < DPU_IP_NUM; i++) {
		if (handler_ctx[i].enHdlState == HANDLER_STATE_RUN) {
			// mutex_lock(&handler_ctx[i].mutex);
			handler_ctx[i].events |= CTX_EVENT_EOF;
			// mutex_unlock(&handler_ctx[i].mutex);
			CVI_TRACE_DPU(CVI_DBG_INFO, "handler_ctx[%d] state=%d, event=0x%x\n",
					i, handler_ctx[i].enHdlState,
					handler_ctx[i].events);
			break;
		} else
			CVI_TRACE_DPU(CVI_DBG_INFO, "handler_ctx[%d] state=%d, event=0x%x\n",
					i, handler_ctx[i].enHdlState,
					handler_ctx[i].events);
	}

	wake_up_interruptible(&dpu_dev->wait);
}

// static s32 _vb_dqbuf(MMF_CHN_S chn, enum CHN_TYPE_E chn_type, VB_BLK *blk)
// {
// 	struct vb_s *p;
// 	*blk = VB_INVALID_HANDLE;

// 	// get vb from workq which is done.
// 	if (base_mod_jobs_workq_empty(chn, chn_type)) {
// 		CVI_TRACE_DPU(CVI_DBG_ERR, "Mod(%d) ChnId(%d) No more vb for dequeue.\n",
// 			       chn.enModId, chn.s32ChnId);
// 		return CVI_FAILURE;
// 	}
// 	p = (struct vb_s *)base_mod_jobs_workq_pop(chn, chn_type);
// 	if (!p)
// 		return CVI_FAILURE;

// 	*blk = (VB_BLK)p;
// 	p->mod_ids &= ~BIT(chn.enModId);

// 	return CVI_SUCCESS;
// }

int32_t _vb_qbuf(MMF_CHN_S chn, enum CHN_TYPE_E chn_type, struct vb_jobs_t *jobs, VB_BLK blk)
{
	struct vb_s *vb = (struct vb_s *)blk;
	s32 ret = CVI_SUCCESS;
	struct vb_s *vb_tmp;

	pr_debug("%s dev(%d) chn(%d) chnType(%d): phy-addr(%lld) cnt(%d)\n",
		     sys_get_modname(chn.enModId), chn.s32DevId, chn.s32ChnId, chn_type,
		     vb->phy_addr, vb->usr_cnt.counter);

	if (!jobs) {
		pr_err("mod(%s), vb_qbuf fail, error, empty jobs\n", sys_get_modname(chn.enModId));
		return CVI_FAILURE;
	}
	if (!jobs->inited) {
		pr_err("mod(%s), vb_qbuf fail, jobs not initialized yet\n", sys_get_modname(chn.enModId));
		return CVI_FAILURE;
	}

	mutex_lock(&jobs->lock);
	if (chn_type == CHN_TYPE_OUT) {
		if (FIFO_FULL(&jobs->workq)) {
			mutex_unlock(&jobs->lock);
			pr_err("%s workq is full. drop new one.\n"
				     , sys_get_modname(chn.enModId));
			return -ENOBUFS;
		}
		vb->buf.dev_num = chn.s32ChnId;
		FIFO_PUSH(&jobs->workq, vb);
	} else {
		if (FIFO_FULL(&jobs->waitq)) {
			FIFO_POP(&jobs->waitq, &vb_tmp);
			vb_release_block((VB_BLK)vb_tmp);
		}
		FIFO_PUSH(&jobs->waitq, vb);
		up(&jobs->sem);
	}
	mutex_unlock(&jobs->lock);

	atomic_fetch_add(1, &vb->usr_cnt);
	atomic_long_fetch_or(BIT(chn.enModId), &vb->mod_ids);
	return ret;
}

static void release_buffers_working(DPU_GRP DpuGrp)
{
	struct DPU_CHN_CFG *stChnCfg;
	VB_BLK blk_left;
	VB_BLK blk_right;
	VB_BLK blk_out[2] ;
	DPU_CHN DpuChn;
	MMF_CHN_S chn_left = {.enModId = CVI_ID_DPU, .s32DevId = DpuGrp, .s32ChnId = 0};
	MMF_CHN_S chn_right = {.enModId = CVI_ID_DPU, .s32DevId = DpuGrp, .s32ChnId = 1};
	MMF_CHN_S chn_out[2] ={chn_left,chn_right};

	vb_dqbuf(chn_left, &gstDpuJobs[DpuGrp].ins[0], &blk_left);
	if (blk_left != VB_INVALID_HANDLE)
		vb_release_block(blk_left);

	vb_dqbuf(chn_right, &gstDpuJobs[DpuGrp].ins[1], &blk_right);
	if (blk_right != VB_INVALID_HANDLE)
		vb_release_block(blk_right);

	for (DpuChn = 0; DpuChn < dpuCtx[DpuGrp]->chnNum; ++DpuChn) {
		stChnCfg = &dpuCtx[DpuGrp]->stChnCfgs[DpuChn];
		chn_out[DpuChn].enModId = CVI_ID_DPU;
		chn_out[DpuChn].s32DevId = DpuGrp;
		chn_out[DpuChn].s32ChnId = DpuChn;
		if (!stChnCfg->isEnabled)
			continue;

		// vb_cancel_block(chn_out[DpuChn]);
		while (!base_mod_jobs_workq_empty(&gstDpuJobs[DpuGrp].outs[DpuChn])) {
			vb_dqbuf(chn_out[DpuChn], &gstDpuJobs[DpuGrp].outs[DpuChn], &blk_out[DpuChn]);
			if (blk_out[DpuChn] != VB_INVALID_HANDLE)
				vb_release_block(blk_out[DpuChn]);
		}
	}
}


static void hw_reset(DPU_GRP DpuGrp)
{
	CVI_TRACE_DPU(CVI_DBG_INFO,"dpu reset !\n");
	release_buffers_working(DpuGrp);
	//dpu_reset();
}

s32 get_dev_info_by_chn(MMF_CHN_S chn, enum CHN_TYPE_E chn_type)
{
	if (chn.enModId != CVI_ID_DPU)
		return 0;
	if (dpuCtx[chn.s32DevId] == NULL) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) isn't created yet.\n", chn.s32DevId);
		return 0;
	}

	return dpuCtx[chn.s32DevId]->u8DpuDev;

}

static DPU_GRP find_next_en_grp(DPU_GRP workingGrp, u8 u8DpuDev)
{
	DPU_GRP i = workingGrp;
	u8 count = 0;
	struct vb_jobs_t *jobs_L;
	struct vb_jobs_t *jobs_R;

	do {
		++i;
		if (i >= DPU_MAX_GRP_NUM)
			i = 0;

		if (dpuCtx[i] && dpuCtx[i]->isCreated && dpuCtx[i]->isStarted
			&& (dpuCtx[i]->u8DpuDev == u8DpuDev)) {
			// CVI_TRACE_DPU(CVI_DBG_INFO, "isCreated(%d)  isStarted(%d)  dev(%d)  i(%d)\n",
			// 		dpuCtx[i]->isCreated,dpuCtx[i]->isStarted,dpuCtx[i]->u8DpuDev,i);
			if(!(&gstDpuJobs[i].ins[0]) || !(&gstDpuJobs[i].ins[1]) ){
				CVI_TRACE_DPU(CVI_DBG_INFO,"Grp(%d) pipe(%d)job ptr is null\n",workingGrp,0);
				continue;
			}
			jobs_L = &gstDpuJobs[i].ins[0];
			jobs_R = &gstDpuJobs[i].ins[1];
			if ((!jobs_L) || (!jobs_R)) {
				CVI_TRACE_DPU(CVI_DBG_INFO, "get jobs failed\n");
				continue;
			}

			// if (!down_trylock(&jobs_L->sem) && !down_trylock(&jobs_R->sem))
			// 	return i;

			if ( (!base_mod_jobs_waitq_empty(&gstDpuJobs[i].ins[0])) &&
					(!base_mod_jobs_waitq_empty(&gstDpuJobs[i].ins[1]))){
						CVI_TRACE_DPU(CVI_DBG_INFO,"next enable grp(%d)\n",i);
						return i;
			}
		}
	} while (++count < DPU_MAX_GRP_NUM);

	return DPU_MAX_GRP_NUM;
}

static u8 get_work_mask(struct cvi_dpu_ctx *ctx)
{
	u8 mask = 0;
	DPU_CHN DpuChn;

	if (!ctx->isCreated || !ctx->isStarted)
		return 0;

	for (DpuChn = 0; DpuChn < ctx->chnNum; ++DpuChn) {
		if (!ctx->stChnCfgs[DpuChn].isEnabled)
			continue;
		mask |= BIT(DpuChn);
	}
	if (mask == 0)
		return 0;

	return mask;
}

void dpu_irq_handler(u8 intr_status, struct cvi_dpu_dev *wdev)
{
	CVI_TRACE_DPU(CVI_DBG_INFO," irq num(%d)\n",wdev->irq_num );
	if(intr_status == DPU_INTR_STATE_DONE){
		CVI_TRACE_DPU(CVI_DBG_INFO, "%s: intr_status(%d)\n", __func__,DPU_INTR_STATE_DONE);
		dpu_notify_isr_evt();
	} else {
		CVI_TRACE_DPU(CVI_DBG_ERR, "%s: intr_status(%d),Frame No Done!  \n", __func__,DPU_INTR_STATE_OTHERS);
		handler_ctx[0].events = 0;
		handler_ctx[0].workingMask =0;
		dpuCtx[handler_ctx->workingGrp]->grp_state = GRP_STATE_IDLE;
		hw_reset(handler_ctx->workingGrp);

		handler_ctx[0].workingGrp
			= find_next_en_grp(handler_ctx[0].workingGrp, 0);

		// unfinished job found, need to re-trig event handler
		if (handler_ctx[0].workingGrp != DPU_MAX_GRP_NUM)
			dpu_notify_wkup_evt(handler_ctx[0].u8DpuDev);

	}
}

static u8 dpu_enable_handler_ctx(struct dpu_handler_ctx *ctx)
{
	u8 u8DpuDev = ctx->u8DpuDev;
	int i;

	for (i = 0; i < DPU_MAX_GRP_NUM; i++)
		if (dpuCtx[i] && dpuCtx[i]->isCreated && dpuCtx[i]->isStarted &&
		    dpuCtx[i]->u8DpuDev == u8DpuDev)
			return CVI_TRUE;

	return CVI_FALSE;
}

static struct vbq *_get_doneq(MMF_CHN_S chn)
{
	struct vb_jobs_t *jobs = &gstDpuJobs[chn.s32DevId].outs[chn.s32ChnId];
	return &jobs->doneq;
}


// s32 dpu_get_chn_buffer(MMF_CHN_S chn, VB_BLK *blk,enum CHN_TYPE_E eChnType ,s32 timeout_ms)
// {
// 	s32 ret = CVI_FAILURE;
// 	struct vb_jobs_t *jobs = base_get_jobs_by_chn(chn, eChnType);
// 	struct vb_s *vb;
// 	struct vbq *doneq = _get_doneq(chn);
// 	struct snap_s *s;

// 	if (!jobs) {
// 		pr_err("mod(%s), get chn buf fail, jobs NULL\n", sys_get_modname(chn.enModId));
// 		return CVI_FAILURE;
// 	}

s32 dpu_get_chn_buffer(MMF_CHN_S chn, VB_BLK *blk,enum CHN_TYPE_E eChnType ,s32 timeout_ms)
{
	s32 ret = CVI_FAILURE;
	struct vb_jobs_t *jobs = (eChnType == CHN_TYPE_OUT) ? &gstDpuJobs[chn.s32DevId].outs[chn.s32ChnId] : \
								&gstDpuJobs[chn.s32DevId].ins[chn.s32ChnId];
	struct vb_s *vb;
	struct vbq *doneq = _get_doneq(chn);
	struct snap_s *s;

	if (!jobs) {
		pr_err("mod(%s), get chn buf fail, jobs NULL\n", sys_get_modname(chn.enModId));
		return CVI_FAILURE;
	}

	if (!jobs->inited) {
		pr_err("mod(%s) get chn buf fail, not inited yet\n", sys_get_modname(chn.enModId));
		return CVI_FAILURE;
	}

	mutex_lock(&jobs->dlock);
	if (!FIFO_EMPTY(doneq)) {
		FIFO_POP(doneq, &vb);
		atomic_long_fetch_and(~BIT(chn.enModId), &vb->mod_ids);
		atomic_long_fetch_or(BIT(CVI_ID_USER), &vb->mod_ids);
		mutex_unlock(&jobs->dlock);
		*blk = (VB_BLK)vb;
		return CVI_SUCCESS;
	}

	s = vmalloc(sizeof(*s));
	if (!s) {
		mutex_unlock(&jobs->dlock);
		return CVI_FAILURE;
	}

	// if (!jobs->inited) {
	// 	pr_err("mod(%s) get chn buf fail, not inited yet\n", sys_get_modname(chn.enModId));
	// 	return CVI_FAILURE;
	// }

	// mutex_lock(&jobs->dlock);
	// if (!FIFO_EMPTY(doneq)) {
	// 	FIFO_POP(doneq, &vb);
	// 	vb->mod_ids &= ~BIT(chn.enModId);
	// 	vb->mod_ids |= BIT(CVI_ID_USER);
	// 	mutex_unlock(&jobs->dlock);
	// 	*blk = (VB_BLK)vb;
	// 	return CVI_SUCCESS;
	// }

	// s = vmalloc(sizeof(*s));
	// if (!s) {
	// 	mutex_unlock(&jobs->dlock);
	// 	return CVI_FAILURE;
	// }

	init_waitqueue_head(&s->cond_queue);

	s->chn = chn;
	s->blk = VB_INVALID_HANDLE;
	s->avail = CVI_FALSE;

	if (timeout_ms < 0) {
		TAILQ_INSERT_TAIL(&jobs->snap_jobs, s, tailq);
		mutex_unlock(&jobs->dlock);
		ret = wait_event_interruptible(s->cond_queue, s->avail);
		// ret < 0, interrupt by a signal
		// ret = 0, condition true
	} else {
		TAILQ_INSERT_TAIL(&jobs->snap_jobs, s, tailq);
		mutex_unlock(&jobs->dlock);
		ret = wait_event_interruptible_timeout(s->cond_queue, s->avail, msecs_to_jiffies(timeout_ms));
		// ret < 0, interrupted by a signal
		// ret = 0, timeout
		// ret = 1, condition true
	}

	if (s->avail)
		ret = 0;
	else
		ret = -1;

	if (!ret) {
		*blk = s->blk;
	} else {
		mutex_lock(&jobs->dlock);
		TAILQ_REMOVE(&jobs->snap_jobs, s, tailq);
		mutex_unlock(&jobs->dlock);
		pr_err("Mod(%s) Grp(%d) Chn(%d), jobs wait(%d) work(%d) done(%d)\n"
			, sys_get_modname(chn.enModId), chn.s32DevId, chn.s32ChnId
			, FIFO_SIZE(&jobs->waitq), FIFO_SIZE(&jobs->workq), FIFO_SIZE(&jobs->doneq));
	}

	vfree(s);
	return ret;
}

s32 cvi_dpu_get_assist_buf_size(u16 u16_disp_num,u32 u32_dst_height,u32 * pu32_size)
{
	return 0;
}

s32 cvi_dpu_create_grp(DPU_GRP DpuGrp,DPU_GRP_ATTR_S *pstGrpAttr)
{
	s32 ret;
	u8 u8DevUsed;
	int i;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_DPU, pstGrpAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_grp_valid(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	if (dpuCtx[DpuGrp]){
		CVI_TRACE_DPU(CVI_DBG_INFO, "DPU GRP existed!!!\n");
		return CVI_FAILURE;
	}


	u8DevUsed = 0;

	if (u8DevUsed >= DPU_IP_NUM) {
		u8DevUsed = DPU_IP_NUM - 1 ;
		CVI_TRACE_DPU(CVI_DBG_WARN, "DPU only allow DpuDev 0.\n");
	}

	dpuCtx[DpuGrp] = kzalloc(sizeof(struct cvi_dpu_ctx), GFP_ATOMIC);
	if (!dpuCtx[DpuGrp]) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "cvi_dpu_ctx kzalloc fail.\n");
		return CVI_ERR_DPU_NOMEM;
	}

	if (pstGrpAttr->stFrameRate.s32SrcFrameRate < pstGrpAttr->stFrameRate.s32DstFrameRate) {
		CVI_TRACE_DPU(CVI_DBG_WARN, "Grp(%d) FrameRate ctrl, src(%d) < dst(%d), not support\n"
				, DpuGrp, pstGrpAttr->stFrameRate.s32SrcFrameRate
				, pstGrpAttr->stFrameRate.s32DstFrameRate);
		return CVI_ERR_DPU_ILLEGAL_PARAM;
	}

	mutex_init(&dpuCtx[DpuGrp]->lock);
	// mutex_init(&gstDpuJobs[DpuGrp].lock);
	dpuCtx[DpuGrp]->isCreated = CVI_TRUE;
	dpuCtx[DpuGrp]->chnNum = 1;
	dpuCtx[DpuGrp]->enPixelFormat = PIXEL_FORMAT_YUV_400 ;
	dpuCtx[DpuGrp]->costTimeForSec = 0 ;
	dpuCtx[DpuGrp]->frameNum = 0;
	dpuCtx[DpuGrp]->u8DpuDev =0;
	dpuCtx[DpuGrp]->chfhBlk = VB_INVALID_HANDLE;
	dpuCtx[DpuGrp]->phyAddr_chfh = 0;
	dpuCtx[DpuGrp]->virAddr_chfh =NULL;
	memcpy(&dpuCtx[DpuGrp]->stGrpAttr, pstGrpAttr, sizeof(dpuCtx[DpuGrp]->stGrpAttr));

	for(i=0; i<DPU_PIPE_IN_NUM; ++i){
		base_mod_jobs_init(&gstDpuJobs[DpuGrp].ins[i], DPU_WAITQ_DEPTH_IN, DPU_WORKQ_DEPTH_IN, DPU_DONEQ_DEPTH_IN);
		CVI_TRACE_DPU(CVI_DBG_INFO, "Grp(%d) ChnIn(%d) Dev(%d)waitqDepth(%d) workqDepth(%d) doneqDepth(%d)\n",
		DpuGrp, i,dpuCtx[DpuGrp]->u8DpuDev, DPU_WAITQ_DEPTH_IN, DPU_WORKQ_DEPTH_IN, DPU_DONEQ_DEPTH_IN);
	}

	memset(&dpuCtx[DpuGrp]->stChnCfgs, 0, sizeof(dpuCtx[DpuGrp]->stChnCfgs));

	memset(&dpuCtx[DpuGrp]->stGrpWorkStatus, 0, sizeof(dpuCtx[DpuGrp]->stGrpWorkStatus));

	memset(&dpuCtx[DpuGrp]->stInputJobStatus, 0, sizeof(dpuCtx[DpuGrp]->stInputJobStatus));
	memset(&dpuCtx[DpuGrp]->stWorkingJobStatus, 0, sizeof(dpuCtx[DpuGrp]->stWorkingJobStatus));
	memset(&dpuCtx[DpuGrp]->stOutputJobStatus, 0, sizeof(dpuCtx[DpuGrp]->stOutputJobStatus));

	mutex_lock(&dpuGetGrpLock);
	dpuGrpUsed[DpuGrp] = CVI_TRUE;
	mutex_unlock(&dpuGetGrpLock);

	CVI_TRACE_DPU(CVI_DBG_INFO, "Grp(%d)  chnNum(%d) isCreated(%d)\n",
		DpuGrp, dpuCtx[DpuGrp]->chnNum,dpuCtx[DpuGrp]->isCreated);

	return CVI_SUCCESS;
}
//EXPORT_SYMBOL_GPL(cvi_dpu_create_grp);

s32 cvi_dpu_destroy_grp(DPU_GRP DpuGrp)
{
	s32 ret;
	int i;

	ret = check_dpu_grp_valid(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!dpuCtx[DpuGrp])
		return CVI_SUCCESS;

	if (dpuCtx[DpuGrp]->isCreated) {
		mutex_lock(&dpuCtx[DpuGrp]->lock);
		dpuCtx[DpuGrp]->isCreated = CVI_FALSE;
		for(i=0; i< DPU_PIPE_IN_NUM; ++i)
			base_mod_jobs_exit(&gstDpuJobs[DpuGrp].ins[i]);
		// if(dpuCtx[DpuGrp]->chfhBlk != VB_INVALID_HANDLE)
		// 	vb_release_block(dpuCtx[DpuGrp]->chfhBlk);
		// CVI_TRACE_DPU(CVI_DBG_INFO, "chfhBlk(%llx)", dpuCtx[DpuGrp]->chfhBlk);

		mutex_unlock(&dpuCtx[DpuGrp]->lock);
	}
	kfree(dpuCtx[DpuGrp]);
	dpuCtx[DpuGrp] = NULL;

	mutex_lock(&dpuGetGrpLock);
	dpuGrpUsed[DpuGrp] = CVI_FALSE;
	mutex_unlock(&dpuGetGrpLock);

	CVI_TRACE_DPU(CVI_DBG_INFO, "Grp(%d)\n", DpuGrp);

	return CVI_SUCCESS;
}
//EXPORT_SYMBOL_GPL(cvi_dpu_destroy_grp);

DPU_GRP cvi_dpu_get_available_grp(void){
	DPU_GRP grp = 0;
	DPU_GRP ret = -1;

	mutex_lock(&dpuGetGrpLock);
	for(grp = 0; grp < DPU_MAX_GRP_NUM; grp++){
		if(!dpuGrpUsed[grp]){
			dpuGrpUsed[grp] = CVI_TRUE;
			ret = grp;
			break;
		}
	}
	mutex_unlock(&dpuGetGrpLock);

	if(dpuCtx[ret]){
		cvi_dpu_disable_chn(ret, 0);
		cvi_dpu_stop_grp(ret);
		cvi_dpu_destroy_grp(ret);
		mutex_lock(&dpuGetGrpLock);
		dpuGrpUsed[ret] = CVI_TRUE;
		mutex_unlock(&dpuGetGrpLock);
	}

	return ret;
}

s32 cvi_dpu_set_grp_attr(DPU_GRP DpuGrp,const DPU_GRP_ATTR_S *pstGrpAttr)
{
	s32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_DPU, pstGrpAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_grp_valid(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_grp_created(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	if (pstGrpAttr->stFrameRate.s32SrcFrameRate < pstGrpAttr->stFrameRate.s32DstFrameRate) {
		CVI_TRACE_DPU(CVI_DBG_WARN, "Grp(%d) FrameRate ctrl, src(%d) < dst(%d), not support\n"
				, DpuGrp, pstGrpAttr->stFrameRate.s32SrcFrameRate
				, pstGrpAttr->stFrameRate.s32DstFrameRate);
		return CVI_ERR_DPU_ILLEGAL_PARAM;
	}

	mutex_lock(&dpuCtx[DpuGrp]->lock);
	memcpy(&dpuCtx[DpuGrp]->stGrpAttr,pstGrpAttr,sizeof(dpuCtx[DpuGrp]->stGrpAttr));
	//dpuCtx[DpuGrp]->stGrpAttr = *pstGrpAttr;
	if(dpuCtx[DpuGrp]->stGrpAttr.bIsBtcostOut ){
		dpuCtx[DpuGrp]->chnNum=2;
	}else{
		dpuCtx[DpuGrp]->chnNum=1;
	}
	mutex_unlock(&dpuCtx[DpuGrp]->lock);

	CVI_TRACE_DPU(CVI_DBG_INFO, "Grp(%d) DpuDev(%d) chnNum(%d) \n",
		DpuGrp, dpuCtx[DpuGrp]->u8DpuDev, dpuCtx[DpuGrp]->chnNum);
	return CVI_SUCCESS;
}
//EXPORT_SYMBOL_GPL(cvi_dpu_set_grp_attr);

s32 cvi_dpu_get_grp_attr(DPU_GRP DpuGrp,DPU_GRP_ATTR_S *pstGrpAttr)
{
	s32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_DPU, pstGrpAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_grp_valid(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_grp_created(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&dpuCtx[DpuGrp]->lock);
	*pstGrpAttr = dpuCtx[DpuGrp]->stGrpAttr;
	mutex_unlock(&dpuCtx[DpuGrp]->lock);

	return CVI_SUCCESS;
}
//EXPORT_SYMBOL_GPL(cvi_dpu_get_grp_attr);

s32 cvi_dpu_start_grp(DPU_GRP DpuGrp)
{
	u8 u8DpuDev;
	s32 ret;

	ret = check_dpu_grp_valid(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_grp_created(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	// if (dpuCtx[DpuGrp]->isStarted) {
	// 	CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) already started.\n", DpuGrp);
	// 	return CVI_SUCCESS;
	// }

	u8DpuDev=dpuCtx[DpuGrp]->u8DpuDev;
	mutex_lock(&dpuCtx[DpuGrp]->lock);
	dpuCtx[DpuGrp]->isStarted = CVI_TRUE;
	dpuCtx[DpuGrp]->grp_state = GRP_STATE_IDLE;

	/* Only change state from stop to run */
	if (handler_ctx[u8DpuDev].enHdlState == HANDLER_STATE_STOP)
		handler_ctx[u8DpuDev].enHdlState = HANDLER_STATE_RUN;
	mutex_unlock(&dpuCtx[DpuGrp]->lock);

	CVI_TRACE_DPU(CVI_DBG_INFO, "Grp(%d) isStart(%d)\n", DpuGrp,dpuCtx[DpuGrp]->isStarted);

	return CVI_SUCCESS;
}
//EXPORT_SYMBOL_GPL(cvi_dpu_start_grp);

s32 cvi_dpu_stop_grp(DPU_GRP DpuGrp)
{
	s32 ret;
	u8 u8DpuDev;
	u8 enabled;
	enum handler_state state;

	ret = check_dpu_grp_valid(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!dpuCtx[DpuGrp])
		return CVI_SUCCESS;
	if (!dpuCtx[DpuGrp]->isStarted)
		return CVI_SUCCESS;

	u8DpuDev = dpuCtx[DpuGrp]->u8DpuDev;

	mutex_lock(&dpuCtx[DpuGrp]->lock);
	if(handler_ctx[u8DpuDev].workingGrp == DpuGrp)
		dpu_reset();
	dpuCtx[DpuGrp]->isStarted = CVI_FALSE;

	dpuCtx[DpuGrp]->grp_state = GRP_STATE_IDLE;

	/* Only change state from run to stop */
	enabled = dpu_enable_handler_ctx(&handler_ctx[u8DpuDev]);
	state = handler_ctx[u8DpuDev].enHdlState;
	CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_enable_handler_ctx(%d)\n", enabled);
	CVI_TRACE_DPU(CVI_DBG_INFO, "handler_ctx.enHdlState (%d)\n", state);
	if (!enabled && state == HANDLER_STATE_RUN) {
		handler_ctx[u8DpuDev].enHdlState = HANDLER_STATE_STOP;
		handler_ctx[u8DpuDev].workingGrp = DPU_MAX_GRP_NUM;
		handler_ctx[u8DpuDev].workingMask = 0;
		handler_ctx[u8DpuDev].events = 0;

		dpu_dev->bBusy = CVI_FALSE;
	}
	mutex_unlock(&dpuCtx[DpuGrp]->lock);
	CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_dev->bBusy(%d)\n", dpu_dev->bBusy);
	CVI_TRACE_DPU(CVI_DBG_INFO, "Grp(%d)\n", DpuGrp);

	return CVI_SUCCESS;
}
//EXPORT_SYMBOL_GPL(cvi_dpu_stop_grp);

s32 cvi_dpu_set_chn_attr(DPU_GRP DpuGrp,DPU_CHN  DpuChn,const DPU_CHN_ATTR_S *pstChnAttr)
{
	VB_CAL_CONFIG_S stVbCalConfig_out;
	VB_CAL_CONFIG_S stVbCalConfig_btcost;
	s32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_DPU, pstChnAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_grp_valid(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_grp_created(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_chn_valid(DpuGrp, DpuChn);
	if (ret != CVI_SUCCESS)
		return ret;


	mutex_lock(&dpuCtx[DpuGrp]->lock);

	if(DpuChn == 0){
		if(dpuCtx[DpuGrp]->stGrpAttr.enDpuMode == DPU_MODE_DEFAULT ||
		dpuCtx[DpuGrp]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_MUX0 ||
		dpuCtx[DpuGrp]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_MUX2 ) {
			COMMON_GetPicBufferConfig(pstChnAttr->stImgSize.u32Width, pstChnAttr->stImgSize.u32Height, \
			PIXEL_FORMAT_YUV_PLANAR_420, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, ALIGN_16, &stVbCalConfig_out);
			dpuCtx[DpuGrp]->stChnCfgs[DpuChn].enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;
			dpuCtx[DpuGrp]->stChnCfgs[0].align = ALIGN_16;
			dpuCtx[DpuGrp]->stChnCfgs[DpuChn].blk_size = stVbCalConfig_out.u32VBSize;
			dpuCtx[DpuGrp]->stChnCfgs[DpuChn].stride=stVbCalConfig_out.u32MainStride;

		}else if(dpuCtx[DpuGrp]->stGrpAttr.enDpuMode == DPU_MODE_FGS_MUX0 ||
				dpuCtx[DpuGrp]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_FGS_ONLINE_MUX0){
					COMMON_GetPicBufferConfig(pstChnAttr->stImgSize.u32Width, pstChnAttr->stImgSize.u32Height, \
					PIXEL_FORMAT_YUV_PLANAR_420, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, ALIGN_32, &stVbCalConfig_out);
					dpuCtx[DpuGrp]->stChnCfgs[DpuChn].enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;
					dpuCtx[DpuGrp]->stChnCfgs[0].align = ALIGN_32;
					dpuCtx[DpuGrp]->stChnCfgs[DpuChn].blk_size = stVbCalConfig_out.u32VBSize;
					dpuCtx[DpuGrp]->stChnCfgs[DpuChn].stride=stVbCalConfig_out.u32MainStride;
		} else if(dpuCtx[DpuGrp]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_MUX1 ){
			COMMON_GetPicBufferConfig(pstChnAttr->stImgSize.u32Width, pstChnAttr->stImgSize.u32Height, \
			PIXEL_FORMAT_YUV_400, DATA_BITWIDTH_8
			, COMPRESS_MODE_NONE, ALIGN_16, &stVbCalConfig_out);
			dpuCtx[DpuGrp]->stChnCfgs[DpuChn].enPixelFormat = PIXEL_FORMAT_YUV_400;
			dpuCtx[DpuGrp]->stChnCfgs[0].align = ALIGN_16;
			dpuCtx[DpuGrp]->stChnCfgs[DpuChn].blk_size = stVbCalConfig_out.u32VBSize*2;
			dpuCtx[DpuGrp]->stChnCfgs[DpuChn].stride=stVbCalConfig_out.u32MainStride*2;

		} else if(dpuCtx[DpuGrp]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_FGS_ONLINE_MUX1 ||
			dpuCtx[DpuGrp]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_FGS_ONLINE_MUX2 ||
			dpuCtx[DpuGrp]->stGrpAttr.enDpuMode == DPU_MODE_FGS_MUX1) {
			COMMON_GetPicBufferConfig(pstChnAttr->stImgSize.u32Width, pstChnAttr->stImgSize.u32Height, \
			PIXEL_FORMAT_YUV_400, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, ALIGN_32, &stVbCalConfig_out);
			dpuCtx[DpuGrp]->stChnCfgs[DpuChn].enPixelFormat = PIXEL_FORMAT_YUV_400;
			dpuCtx[DpuGrp]->stChnCfgs[0].align = ALIGN_32;
			dpuCtx[DpuGrp]->stChnCfgs[DpuChn].blk_size = stVbCalConfig_out.u32VBSize*2;
			dpuCtx[DpuGrp]->stChnCfgs[DpuChn].stride=stVbCalConfig_out.u32MainStride*2;
		}else {
		CVI_TRACE_DPU(CVI_DBG_ERR,"[%s] INVALID DPU MODE!\n",__func__);
		return CVI_FAILURE;
		}

		memcpy(&dpuCtx[DpuGrp]->stChnCfgs[0].stChnAttr, pstChnAttr,
		sizeof(dpuCtx[DpuGrp]->stChnCfgs[0].stChnAttr));

		// dpuCtx[DpuGrp]->stChnCfgs[DpuChn].enPixelFormat = PIXEL_FORMAT_YUV_400;
		dpuCtx[DpuGrp]->stChnCfgs[DpuChn].is_cfg_changed = CVI_TRUE;
		dpuCtx[DpuGrp]->stChnCfgs[DpuChn].VbPool=VB_INVALID_POOLID;
		memcpy(&dpuCtx[DpuGrp]->stChnCfgs[0].stVbCalConfig,&stVbCalConfig_out,sizeof(VB_CAL_CONFIG_S));
	}else if(DpuChn == 1){
		COMMON_GetPicBufferConfig(pstChnAttr->stImgSize.u32Width * 128, pstChnAttr->stImgSize.u32Height,
			PIXEL_FORMAT_YUV_400, DATA_BITWIDTH_16
			, COMPRESS_MODE_NONE, ALIGN_16, &stVbCalConfig_btcost);
		memcpy(&dpuCtx[DpuGrp]->stChnCfgs[1].stChnAttr, pstChnAttr,
		sizeof(dpuCtx[DpuGrp]->stChnCfgs[1].stChnAttr));
		dpuCtx[DpuGrp]->stChnCfgs[DpuChn].blk_size = stVbCalConfig_btcost.u32VBSize;

		dpuCtx[DpuGrp]->stChnCfgs[DpuChn].enPixelFormat = PIXEL_FORMAT_YUV_400;
		dpuCtx[DpuGrp]->stChnCfgs[DpuChn].stride=stVbCalConfig_btcost.u32MainStride;
		dpuCtx[DpuGrp]->stChnCfgs[DpuChn].is_cfg_changed = CVI_TRUE;
		dpuCtx[DpuGrp]->stChnCfgs[DpuChn].VbPool = VB_INVALID_POOLID;
		memcpy(&dpuCtx[DpuGrp]->stChnCfgs[1].stVbCalConfig,&stVbCalConfig_btcost,sizeof(VB_CAL_CONFIG_S));
	}

	mutex_unlock(&dpuCtx[DpuGrp]->lock);

	CVI_TRACE_DPU(CVI_DBG_INFO, "Grp(%d) Chn(%d) u32Width(%d), u32Height(%d)\n"
		, DpuGrp, DpuChn, pstChnAttr->stImgSize.u32Width, pstChnAttr->stImgSize.u32Height);

	return CVI_SUCCESS;
}
//EXPORT_SYMBOL_GPL(cvi_dpu_set_chn_attr);

s32 cvi_dpu_get_chn_attr(DPU_GRP DpuGrp,DPU_CHN DpuChn,DPU_CHN_ATTR_S *pstChnAttr)
{
	s32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_DPU, pstChnAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_grp_valid(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_grp_created(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_chn_valid(DpuGrp, DpuChn);
	if (ret != CVI_SUCCESS)
		return ret;

	memcpy(pstChnAttr, &dpuCtx[DpuGrp]->stChnCfgs[DpuChn].stChnAttr, sizeof(*pstChnAttr));
	return CVI_SUCCESS;
}
//EXPORT_SYMBOL_GPL(cvi_dpu_get_chn_attr);

s32 cvi_dpu_enable_chn(DPU_GRP DpuGrp,DPU_CHN DpuChn)
{
	struct DPU_CHN_CFG *chn_cfg;
	s32 ret;
	u8 u8DpuDev;

	ret = check_dpu_grp_valid(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_grp_created(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_chn_valid(DpuGrp, DpuChn);
	if (ret != CVI_SUCCESS)
		return ret;

	chn_cfg = &dpuCtx[DpuGrp]->stChnCfgs[DpuChn];
	if (chn_cfg->isEnabled) {
		CVI_TRACE_DPU(CVI_DBG_INFO, "Grp(%d) Chn(%d) already enabled\n", DpuGrp, DpuChn);
		return CVI_SUCCESS;
	}
	u8DpuDev = dpuCtx[DpuGrp]->u8DpuDev;
	mutex_lock(&dpuCtx[DpuGrp]->lock);
	base_mod_jobs_init(&gstDpuJobs[DpuGrp].outs[DpuChn], DPU_WAITQ_DEPTH_OUT,  DPU_WORKQ_DEPTH_OUT,  DPU_DONEQ_DEPTH_OUT);

	chn_cfg->isEnabled = CVI_TRUE;
	mutex_unlock(&dpuCtx[DpuGrp]->lock);
	CVI_TRACE_DPU(CVI_DBG_INFO, "Grp(%d) Chn(%d)\n", DpuGrp, DpuChn);

	return CVI_SUCCESS;
}
//EXPORT_SYMBOL_GPL(cvi_dpu_enable_chn);

s32 cvi_dpu_disable_chn(DPU_GRP DpuGrp,DPU_CHN DpuChn)
{
	s32 ret;

	ret = check_dpu_grp_valid(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_grp_created(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_chn_valid(DpuGrp, DpuChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!dpuCtx[DpuGrp]->isCreated) {
		CVI_TRACE_DPU(CVI_DBG_INFO, "Grp(%d) not created yet\n", DpuGrp);
		return CVI_SUCCESS;
	}
	if (!dpuCtx[DpuGrp]->stChnCfgs[DpuChn].isEnabled) {
		CVI_TRACE_DPU(CVI_DBG_INFO, "Grp(%d) Chn(%d) not enabled yet\n", DpuGrp, DpuChn);
		return CVI_SUCCESS;
	}

	mutex_lock(&dpuCtx[DpuGrp]->lock);
	dpuCtx[DpuGrp]->stChnCfgs[DpuChn].isEnabled = CVI_FALSE;

	base_mod_jobs_exit(&gstDpuJobs[DpuGrp].outs[DpuChn]);

	mutex_unlock(&dpuCtx[DpuGrp]->lock);
	CVI_TRACE_DPU(CVI_DBG_INFO, "Grp(%d) Chn(%d)\n", DpuGrp, DpuChn);

	return CVI_SUCCESS;
}
//EXPORT_SYMBOL_GPL(cvi_dpu_disable_chn);

s32 cvi_dpu_send_frame(DPU_GRP DpuGrp,\
                                const VIDEO_FRAME_INFO_S *pstLeftFrame,\
                                const VIDEO_FRAME_INFO_S *pstRightFrame,\
                                s32 s32_millisec)
{

	MMF_CHN_S chn_left = {.enModId = CVI_ID_DPU, .s32DevId = DpuGrp, .s32ChnId = 0};
	MMF_CHN_S chn_right = {.enModId = CVI_ID_DPU, .s32DevId = DpuGrp, .s32ChnId = 1};
	VB_BLK blk_right;
	VB_BLK blk_left;
	s32 ret;
	struct vb_s *vb_l;
	struct vb_s *vb_r;
	// ret = wait_event_interruptible_timeout(dpu_dev->sendFrame_wait,
	// 		!dpu_dev->bBusy ,msecs_to_jiffies(3000));
	CVI_TRACE_DPU(CVI_DBG_INFO, " cvi_dpu_send_frame          +\n");
	ret = MOD_CHECK_NULL_PTR(CVI_ID_DPU, pstLeftFrame);
	if (ret != CVI_SUCCESS){
		CVI_TRACE_DPU(CVI_DBG_ERR, "left Frame null ptr.\n");
		return ret;
	}

	ret = MOD_CHECK_NULL_PTR(CVI_ID_DPU, pstRightFrame);
	if (ret != CVI_SUCCESS){
		CVI_TRACE_DPU(CVI_DBG_ERR, "right Frame null ptr.\n");
		return ret;
	}

	ret = check_dpu_grp_valid(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_grp_created(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!DPU_GRP_SUPPORT_FMT(pstLeftFrame->stVFrame.enPixelFormat)) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) leftFrame PixelFormat(%d) mismatch.\n"
			, DpuGrp, pstLeftFrame->stVFrame.enPixelFormat);
		return CVI_ERR_DPU_ILLEGAL_PARAM;
	}
	if (!DPU_GRP_SUPPORT_FMT(pstRightFrame->stVFrame.enPixelFormat)) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) RightFrame PixelFormat(%d) mismatch.\n"
			, DpuGrp, pstRightFrame->stVFrame.enPixelFormat);
		return CVI_ERR_DPU_ILLEGAL_PARAM;
	}
	if ((dpuCtx[DpuGrp]->stGrpAttr.stLeftImageSize.u32Width  != pstLeftFrame->stVFrame.u32Width)
	 || (dpuCtx[DpuGrp]->stGrpAttr.stLeftImageSize.u32Height  != pstLeftFrame->stVFrame.u32Height)) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) leftFrame Size(%d * %d) mismatch.\n"
			, DpuGrp, pstLeftFrame->stVFrame.u32Width, pstLeftFrame->stVFrame.u32Height);
		return CVI_ERR_DPU_ILLEGAL_PARAM;
	}
	if ((dpuCtx[DpuGrp]->stGrpAttr.stRightImageSize.u32Width  != pstRightFrame->stVFrame.u32Width)
	 || (dpuCtx[DpuGrp]->stGrpAttr.stRightImageSize.u32Height  != pstRightFrame->stVFrame.u32Height)) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) rightFrame Size(%d * %d) mismatch.\n"
			, DpuGrp, pstRightFrame->stVFrame.u32Width, pstRightFrame->stVFrame.u32Height);
		return CVI_ERR_DPU_ILLEGAL_PARAM;
	}
	if (IS_FRAME_OFFSET_INVALID(pstLeftFrame->stVFrame)) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) Leftframe offset (%d %d %d %d) invalid\n",
			DpuGrp, pstLeftFrame->stVFrame.s16OffsetLeft, pstLeftFrame->stVFrame.s16OffsetRight,
			pstLeftFrame->stVFrame.s16OffsetTop, pstLeftFrame->stVFrame.s16OffsetBottom);
		return CVI_ERR_DPU_ILLEGAL_PARAM;
	}
	if (IS_FRAME_OFFSET_INVALID(pstRightFrame->stVFrame)) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) Rightframe offset (%d %d %d %d) invalid\n",
			DpuGrp, pstRightFrame->stVFrame.s16OffsetLeft, pstRightFrame->stVFrame.s16OffsetRight,
			pstRightFrame->stVFrame.s16OffsetTop, pstRightFrame->stVFrame.s16OffsetBottom);
		return CVI_ERR_DPU_ILLEGAL_PARAM;
	}
	// mutex_lock(&gstDpuJobs[DpuGrp].lock);
	blk_left = vb_phys_addr2handle(pstLeftFrame->stVFrame.u64PhyAddr[0]);
	if (blk_left == VB_INVALID_HANDLE) {
		// CVI_TRACE_DPU(CVI_DBG_WARN, " blk_left VB_INVALID_HANDLE Grp(%d) .\n", DpuGrp);
		blk_left = vb_create_block(pstLeftFrame->stVFrame.u64PhyAddr[0], NULL, VB_EXTERNAL_POOLID, CVI_TRUE);
		if (blk_left == VB_INVALID_HANDLE) {
			CVI_TRACE_DPU(CVI_DBG_ERR, "left frame Grp(%d) no space for malloc.\n", DpuGrp);
			return CVI_ERR_DPU_NOMEM;
		}
	}

	blk_right = vb_phys_addr2handle(pstRightFrame->stVFrame.u64PhyAddr[0]);
	if (blk_right == VB_INVALID_HANDLE) {
		// CVI_TRACE_DPU(CVI_DBG_WARN, " blk_left VB_INVALID_HANDLE Grp(%d) .\n", DpuGrp);
		blk_right = vb_create_block(pstRightFrame->stVFrame.u64PhyAddr[0], NULL, VB_EXTERNAL_POOLID, CVI_TRUE);
		if (blk_right == VB_INVALID_HANDLE) {
			CVI_TRACE_DPU(CVI_DBG_ERR, "Right frame Grp(%d) no space for malloc.\n", DpuGrp);
			return CVI_ERR_DPU_NOMEM;
		}
	}

	if (dpu_fill_videoframe2buffer(chn_left, pstLeftFrame, &((struct vb_s *)blk_left)->buf) != CVI_SUCCESS) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Left frame Grp(%d) Invalid parameter\n", DpuGrp);
		return CVI_ERR_DPU_ILLEGAL_PARAM;
	}

	if (dpu_fill_videoframe2buffer(chn_right, pstRightFrame, &((struct vb_s *)blk_right)->buf) != CVI_SUCCESS) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Right frame Grp(%d) Invalid parameter\n", DpuGrp);
		return CVI_ERR_DPU_ILLEGAL_PARAM;
	}

	if (_vb_qbuf(chn_left, CHN_TYPE_IN, &gstDpuJobs[DpuGrp].ins[0], blk_left) != CVI_SUCCESS) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Left frame Grp(%d) qbuf failed\n", DpuGrp);
		return CVI_ERR_DPU_BUSY;
	}
	vb_l =(struct vb_s *)blk_left;
	CVI_TRACE_DPU(CVI_DBG_INFO, "vb_l cnt(%d) \n",vb_l->usr_cnt.counter);
	vb_release_block(blk_left);
	CVI_TRACE_DPU(CVI_DBG_INFO, "vb_l cnt(%d) \n",vb_l->usr_cnt.counter);

	if (_vb_qbuf(chn_right, CHN_TYPE_IN, &gstDpuJobs[DpuGrp].ins[1], blk_right) != CVI_SUCCESS) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Right frame Grp(%d) qbuf failed\n", DpuGrp);
		if(!base_mod_jobs_waitq_empty(&gstDpuJobs[DpuGrp].ins[0])){
			base_mod_jobs_waitq_pop(&gstDpuJobs[DpuGrp].ins[0]);
			if(!down_trylock(&gstDpuJobs[DpuGrp].ins[0].sem)){
				CVI_TRACE_DPU(CVI_DBG_ERR, "down try lock sem of left buf\n");
			}
		}
		return CVI_ERR_DPU_BUSY;
	}
	print_vbq_size(DpuGrp);
	vb_r =(struct vb_s *)blk_right;
	CVI_TRACE_DPU(CVI_DBG_INFO, "vb_r cnt(%d) \n",vb_r->usr_cnt.counter);
	vb_release_block(blk_right);
	// mutex_unlock(&gstDpuJobs[DpuGrp].lock);
	CVI_TRACE_DPU(CVI_DBG_INFO, "vb_r cnt(%d) \n",vb_r->usr_cnt.counter);

	CVI_TRACE_DPU(CVI_DBG_INFO, "Left frame Grp(%d), phy-address(0x%llx)\n",
			DpuGrp, pstLeftFrame->stVFrame.u64PhyAddr[0]);

	CVI_TRACE_DPU(CVI_DBG_INFO, "Right frame Grp(%d), phy-address(0x%llx)\n",
			DpuGrp, pstRightFrame->stVFrame.u64PhyAddr[0]);

	dpuCtx[DpuGrp]->u32Stride[0]= pstLeftFrame->stVFrame.u32Stride[0];
	dpuCtx[DpuGrp]->u32Stride[1]= pstRightFrame->stVFrame.u32Stride[0];

	//dpu_notify_wkup_evt(handler_ctx[0].u8DpuDev);
	if(dpu_dev->bBusy == CVI_FALSE){
		dpu_notify_wkup_evt(handler_ctx[0].u8DpuDev);
	}
	CVI_TRACE_DPU(CVI_DBG_INFO, " dpu_dev->bBusy(%d) \n",dpu_dev->bBusy);
	CVI_TRACE_DPU(CVI_DBG_INFO, " cvi_dpu_send_frame          -\n");
	return CVI_SUCCESS;
}
//EXPORT_SYMBOL_GPL(cvi_dpu_send_frame);

s32 cvi_dpu_send_chn_frame(DPU_GRP DpuGrp, DPU_CHN DpuChn
	, const VIDEO_FRAME_INFO_S *pstFrameInfo, s32 s32MilliSec)
{
	s32 ret;
	MMF_CHN_S chn = {.enModId = CVI_ID_DPU, .s32DevId = DpuGrp, .s32ChnId = DpuChn};
	VB_BLK blk;
	struct vb_s *vb;
	struct vb_jobs_t *jobs;

	CVI_TRACE_DPU(CVI_DBG_INFO, " cvi_dpu_send_chn_frame \n");

	ret = MOD_CHECK_NULL_PTR(CVI_ID_DPU, pstFrameInfo);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_grp_valid(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_grp_created(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_chn_valid(DpuGrp, DpuChn);
	if (ret != CVI_SUCCESS)
		return ret;

	UNUSED(s32MilliSec);

	blk = vb_phys_addr2handle(pstFrameInfo->stVFrame.u64PhyAddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		blk = vb_create_block(pstFrameInfo->stVFrame.u64PhyAddr[0], NULL, VB_EXTERNAL_POOLID, CVI_TRUE);
		if (blk == VB_INVALID_HANDLE) {
			CVI_TRACE_DPU(CVI_DBG_ERR, "DpuChn frame Chn(%d) no space for malloc.\n", DpuChn);
			return CVI_ERR_DPU_NOMEM;
		}
	}

	if(base_fill_videoframe2buffer(chn, pstFrameInfo, &((struct vb_s *)blk)->buf) != CVI_SUCCESS){
		CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) Chn(%d) Invalid parameter\n", DpuGrp, DpuChn);
		return CVI_ERR_DPU_NOMEM;
	}

	jobs = &gstDpuJobs[DpuGrp].outs[DpuChn];
	if (!jobs) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) Chn(%d) get job failed\n",
				DpuGrp, DpuChn);
		return CVI_FAILURE;
	}

	vb = (struct vb_s *)blk;
	mutex_lock(&jobs->lock);
	if (FIFO_FULL(&jobs->waitq)) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) Chn(%d) waitq is full\n", DpuGrp, DpuChn);
		mutex_unlock(&jobs->lock);
		return CVI_FAILURE;
	}
	FIFO_PUSH(&jobs->waitq, vb);
	atomic_fetch_add(1, &vb->usr_cnt);
	mutex_unlock(&jobs->lock);

	CVI_TRACE_DPU(CVI_DBG_DEBUG, "Grp(%d) Chn(%d)\n", DpuGrp, DpuChn);
	CVI_TRACE_DPU(CVI_DBG_INFO, "Output frame Grp(%d), phy-address(0x%llx)\n",
			DpuGrp, pstFrameInfo->stVFrame.u64PhyAddr[0]);
	CVI_TRACE_DPU(CVI_DBG_INFO, " cvi_dpu_send_chn_frame \n");
	return ret;
}

s32 cvi_dpu_get_frame(DPU_GRP DpuGrp,\
						 DPU_CHN DpuChn,\
						 VIDEO_FRAME_INFO_S *pstFrameInfo,\
						 s32 s32Millisec)
{
	s32 ret, i;
	VB_BLK blk = VB_INVALID_HANDLE;
	struct vb_s *vb;
	MMF_CHN_S chn = {.enModId = CVI_ID_DPU, .s32DevId = DpuGrp, .s32ChnId = DpuChn};
	CVI_TRACE_DPU(CVI_DBG_INFO, " cvi_dpu_get_frame          +\n");
	ret = MOD_CHECK_NULL_PTR(CVI_ID_DPU, pstFrameInfo);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_grp_valid(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_grp_created(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_chn_valid(DpuGrp, DpuChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!dpuCtx[DpuGrp]->isStarted) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) not yet started.\n", DpuGrp);
		return CVI_ERR_DPU_NOTREADY;
	}
	if (!dpuCtx[DpuGrp]->stChnCfgs[DpuChn].isEnabled) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) Chn(%d) not yet enabled.\n", DpuGrp, DpuChn);
		return CVI_ERR_DPU_NOTREADY;
	}

	memset(pstFrameInfo, 0, sizeof(*pstFrameInfo));
	ret = base_get_chn_buffer(chn, &gstDpuJobs[DpuGrp].outs[DpuChn],&blk,s32Millisec);

	if (ret != 0 || blk == VB_INVALID_HANDLE) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) Chn(%d) get chn frame fail, s32MilliSec=%d, ret=%d\n",
				DpuGrp, DpuChn, s32Millisec, ret);

		dpu_check_reg_read();

		getsgbm_status();

		getfgs_status();
		return CVI_ERR_DPU_BUF_EMPTY;
	}

	vb = (struct vb_s *)blk;
	if (!vb->buf.phy_addr[0] || !vb->buf.size.u32Width) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "buf already released\n");
		return CVI_ERR_DPU_BUF_EMPTY;
	}

	pstFrameInfo->stVFrame.enPixelFormat = dpuCtx[DpuGrp]->stChnCfgs[DpuChn].enPixelFormat;
	pstFrameInfo->stVFrame.u32Width = vb->buf.size.u32Width;
	pstFrameInfo->stVFrame.u32Height = vb->buf.size.u32Height;
	pstFrameInfo->stVFrame.u32TimeRef = vb->buf.frm_num;
	pstFrameInfo->stVFrame.u64PTS = vb->buf.u64PTS;
	for (i = 0; i < 3; ++i) {
		if(vb->buf.length[i] ==0 )
			continue;

		pstFrameInfo->stVFrame.u64PhyAddr[i] = vb->buf.phy_addr[i];
		pstFrameInfo->stVFrame.u32Length[i] = vb->buf.length[i];
		pstFrameInfo->stVFrame.u32Stride[i] = vb->buf.stride[i];
	}

	pstFrameInfo->stVFrame.s16OffsetTop = vb->buf.s16OffsetTop;
	pstFrameInfo->stVFrame.s16OffsetBottom = vb->buf.s16OffsetBottom;
	pstFrameInfo->stVFrame.s16OffsetLeft = vb->buf.s16OffsetLeft;
	pstFrameInfo->stVFrame.s16OffsetRight = vb->buf.s16OffsetRight;
	pstFrameInfo->stVFrame.pPrivateData = vb;

	CVI_TRACE_DPU(CVI_DBG_INFO, "Grp(%d) Chn(%d) end to set pstFrameInfo width:%d height:%d buf:0x%llx\n"
			, DpuGrp, DpuChn, pstFrameInfo->stVFrame.u32Width, pstFrameInfo->stVFrame.u32Height,
			pstFrameInfo->stVFrame.u64PhyAddr[0]);
	print_vbq_size(DpuGrp);
	CVI_TRACE_DPU(CVI_DBG_INFO, " cvi_dpu_get_frame          -\n");
	return CVI_SUCCESS;
}

s32 cvi_dpu_release_frame(DPU_GRP DpuGrp,\
							DPU_CHN DpuChn,\
                            const VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	VB_BLK blk;
	s32 ret;
	CVI_TRACE_DPU(CVI_DBG_INFO, " cvi_dpu_release_frame          +\n");
	ret = MOD_CHECK_NULL_PTR(CVI_ID_DPU, pstVideoFrame);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_grp_valid(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_grp_created(DpuGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_dpu_chn_valid(DpuGrp, DpuChn);
	if (ret != CVI_SUCCESS)
		return ret;

	blk = vb_phys_addr2handle(pstVideoFrame->stVFrame.u64PhyAddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		if (pstVideoFrame->stVFrame.pPrivateData == 0) {
			CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) Chn(%d) phy-address(0x%llx) invalid to locate.\n"
				      , DpuGrp, DpuChn, (unsigned long long)pstVideoFrame->stVFrame.u64PhyAddr[0]);
			return CVI_ERR_DPU_ILLEGAL_PARAM;
		}
		blk = (VB_BLK)pstVideoFrame->stVFrame.pPrivateData;
	}

	if (vb_release_block(blk) != CVI_SUCCESS)
		return CVI_FAILURE;

	CVI_TRACE_DPU(CVI_DBG_INFO, "Grp(%d) Chn(%d) buf:0x%llx\n",
			DpuGrp, DpuChn, pstVideoFrame->stVFrame.u64PhyAddr[0]);

	CVI_TRACE_DPU(CVI_DBG_INFO, " cvi_dpu_release_frame          -\n");
	return CVI_SUCCESS;
}

void dpu_set_base_addr(void *base)
{
	reg_base = (uintptr_t)base;
}


void dpu_set_base_addr_sgbm_dma(void *base1,void *base2,void *base3,void *base4)
{
	reg_base_sgbm_ld1_dma    = (uintptr_t)base1;
	reg_base_sgbm_ld2_dma    = (uintptr_t)base2;
	reg_base_sgbm_median_dma = (uintptr_t)base3;
	reg_base_sgbm_bf_dma     = (uintptr_t)base4;

}

void dpu_set_base_addr_fgs_dma(void *base1,void *base2,void *base3,void *base4)
{
	reg_base_fgs_gx_dma      = (uintptr_t)base1;
	reg_base_fgs_chfh_ld_dma = (uintptr_t)base2;
	reg_base_fgs_chfh_st_dma = (uintptr_t)base3;
	reg_base_fgs_ux_dma      = (uintptr_t)base4;
}

void dpu_reset(void)
{
	s32 cnt = 0;
	u32 reg_68 = read_reg(reg_base + DPU_REG_68_OFS);
	CVI_TRACE_DPU(CVI_DBG_INFO, " dpu_reset_done(%d) \n",get_mask(reg_68, 2, 7));
	CVI_TRACE_DPU(CVI_DBG_INFO, " dpu_reset          +\n");
	reg_write_mask(reg_base + DPU_REG_68_OFS,((1 << 1)-1) << 6,1<< 6);
	while(cnt < 50){
		reg_68 = read_reg(reg_base + DPU_REG_68_OFS);
		if(get_mask(reg_68, 2, 7) ==3)
			break;
		//CVI_TRACE_DPU(CVI_DBG_INFO, " dpu_reset_done(%d) \n",get_mask(reg_68, 2, 7));
		reg_write_mask(reg_base + DPU_REG_68_OFS,((1 << 1)-1) << 6,1<< 6);
		cnt++;
		udelay(20);
	}
	CVI_TRACE_DPU(CVI_DBG_INFO, " dpu_reset          -\n");
}

void dpu_intr_clr()
{
	CVI_TRACE_DPU(CVI_DBG_INFO, " dpu_intr_clr          +\n");
	reg_write_mask(reg_base + DPU_REG_68_OFS, ((1 << 1)-1) << 9, 1 << 9);
	CVI_TRACE_DPU(CVI_DBG_INFO, " dpu_intr_clr          -\n");
}

u8 dpu_intr_status(void)
{
	u8 intr_status ;
	u32 value;
	u32 reg_68 = read_reg(reg_base + DPU_REG_68_OFS);
	CVI_TRACE_DPU(CVI_DBG_INFO, " dpu_intr_status          +\n");
	if(handler_ctx[0].enDpuMode == DPU_MODE_DEFAULT  ||
		handler_ctx[0].enDpuMode == DPU_MODE_SGBM_MUX0 ||
		handler_ctx[0].enDpuMode == DPU_MODE_SGBM_MUX1 ||
		handler_ctx[0].enDpuMode == DPU_MODE_SGBM_MUX2 ){
			value = get_mask(reg_68, 1, 1);
	}else {
		value = get_mask(reg_68, 1, 11);
	}

	if(value ==1)
		intr_status = DPU_INTR_STATE_DONE;
	else
		intr_status = DPU_INTR_STATE_OTHERS;

	dpu_intr_clr();
	CVI_TRACE_DPU(CVI_DBG_INFO, " dpu_intr_status          -\n");
	return intr_status;
}

void dpu_sel_intr(u32 intr_type)
{
	CVI_TRACE_DPU(CVI_DBG_INFO,"dpu_sel_intr(%d)\n",intr_type);
    reg_write_mask(reg_base+DPU_REG_6C_OFS,((1 << 8)-1) << 24,intr_type<< 24);
}

void dpu_engine(DPU_GRP workingGrpID)
{
	// start dpu
	if(dpuCtx[workingGrpID]->stGrpAttr.enDpuMode == DPU_MODE_DEFAULT  ||
		dpuCtx[workingGrpID]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_MUX0 ||
		dpuCtx[workingGrpID]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_MUX1 ||
		dpuCtx[workingGrpID]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_MUX2  ){
		dpu_sel_intr(1);
		reg_write_mask(reg_base+DPU_REG_68_OFS,((1 << 1)-1) << 0,1 << 0);
		CVI_TRACE_DPU(CVI_DBG_INFO,"triger sgbm frame start\n");
	}else if(dpuCtx[workingGrpID]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_FGS_ONLINE_MUX0 ||
		dpuCtx[workingGrpID]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_FGS_ONLINE_MUX1 ||
		dpuCtx[workingGrpID]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_FGS_ONLINE_MUX2 ){
		dpu_sel_intr(2);
		reg_write_mask(reg_base+DPU_REG_68_OFS,((1 << 1)-1) << 10,1 << 10);
		reg_write_mask(reg_base+DPU_REG_68_OFS,((1 << 1)-1) << 0,1 << 0);
		CVI_TRACE_DPU(CVI_DBG_INFO,"triger sgbm frame start\n");
	}else if(dpuCtx[workingGrpID]->stGrpAttr.enDpuMode == DPU_MODE_FGS_MUX0 ||
	dpuCtx[workingGrpID]->stGrpAttr.enDpuMode == DPU_MODE_FGS_MUX1){
		dpu_sel_intr(2);
		reg_write_mask(reg_base+DPU_REG_68_OFS,((1 << 1)-1) << 10,1 << 10);
		CVI_TRACE_DPU(CVI_DBG_INFO,"triger fgs frame start\n");
	}else{
		CVI_TRACE_DPU(CVI_DBG_INFO,"unexpected DPU MODE\n");
	}

}

u8 dpu_is_busy(void)
{
	return CVI_FALSE;
}

struct cvi_dpu_ctx **dpu_get_shdw_ctx(void)
{
	return dpuCtx;
}

struct dpu_handler_ctx *dpu_get_handler_ctx(void)
{
	return handler_ctx;
}

s32 dpu_check_param(DPU_GRP grp)
{

	u32 width;
	u32 height;
	u32 min_d;
	u32 dispRange;
	u32 stride_left;
	u32 stride_right;
	u32 width_left =dpuCtx[grp]->stGrpAttr.stLeftImageSize.u32Width;
	u32 width_right =dpuCtx[grp]->stGrpAttr.stRightImageSize.u32Width;
	u32 height_left =dpuCtx[grp]->stGrpAttr.stLeftImageSize.u32Height;
	u32 height_right =dpuCtx[grp]->stGrpAttr.stRightImageSize.u32Height;
	CVI_TRACE_DPU(CVI_DBG_INFO, " dpu_check_param          +\n");
	if(width_left != width_right || height_left != height_right){
		CVI_TRACE_DPU(CVI_DBG_INFO,"[%s]the size of Left image not the same as Right image!\n ",__func__);
		CVI_TRACE_DPU(CVI_DBG_INFO,"left width(%d),left height(%d),right width(%d) right height(%d)",width_left,height_left,width_right,height_right);
		return CVI_FAILURE;
	}

	width =width_left;
	height =height_left;
	if(width <64 || width > 1920 || height < 64 || height >1080){
		CVI_TRACE_DPU(CVI_DBG_INFO,"[%s]the size of image out of the range!\n ",__func__);
		CVI_TRACE_DPU(CVI_DBG_INFO,"width(%d),height(%d)",width,height);
		return CVI_FAILURE;
	}
	if((width % ALIGN_4) != 0){
		CVI_TRACE_DPU(CVI_DBG_INFO,"[%s]the width is not 4 ailgn !\n ",__func__);
		CVI_TRACE_DPU(CVI_DBG_INFO,"width(%d)\n",width);
		return CVI_FAILURE;
	}
	stride_left=dpuCtx[grp]->u32Stride[0];
	stride_right=dpuCtx[grp]->u32Stride[1];
	if(((stride_left % ALIGN_16) != 0) || ((stride_right % ALIGN_16) != 0)){
		CVI_TRACE_DPU(CVI_DBG_INFO,"[%s]the stride is not 16 ailgn !\n ",__func__);
		CVI_TRACE_DPU(CVI_DBG_INFO,"stride left(%d),stride right(%d)",stride_left,stride_right);
		return CVI_FAILURE;
	}

	min_d =dpuCtx[grp]->stGrpAttr.u16DispStartPos;
	if(min_d >239){
		CVI_TRACE_DPU(CVI_DBG_INFO,"[%s]the min_d out of the range !\n ",__func__);
		CVI_TRACE_DPU(CVI_DBG_INFO,"min_d(%d)",min_d);
		return CVI_FAILURE;
	}
	if(dpuCtx[grp]->stGrpAttr.enDispRange == DPU_DISP_RANGE_DEFAULT ||
		dpuCtx[grp]->stGrpAttr.enDispRange == DPU_DISP_RANGE_16) {
		dispRange = 16;
	}else if(dpuCtx[grp]->stGrpAttr.enDispRange == DPU_DISP_RANGE_32){
		dispRange = 32;
	}else if(dpuCtx[grp]->stGrpAttr.enDispRange == DPU_DISP_RANGE_48){
		dispRange = 48;
	}else if(dpuCtx[grp]->stGrpAttr.enDispRange == DPU_DISP_RANGE_64){
		dispRange = 64;
	}else if(dpuCtx[grp]->stGrpAttr.enDispRange == DPU_DISP_RANGE_80){
		dispRange = 80;
	}else if(dpuCtx[grp]->stGrpAttr.enDispRange == DPU_DISP_RANGE_96){
		dispRange = 96;
	}else if(dpuCtx[grp]->stGrpAttr.enDispRange == DPU_DISP_RANGE_112){
		dispRange = 112;
	}else if(dpuCtx[grp]->stGrpAttr.enDispRange == DPU_DISP_RANGE_128){
		dispRange = 128;
	}else{
		CVI_TRACE_DPU(CVI_DBG_INFO,"[%s]the dispRange enum idex out of the range !\n ",__func__);
		return CVI_FAILURE;
	}

	if((min_d +dispRange)>255 || (min_d+dispRange)>(width-48)) {
		CVI_TRACE_DPU(CVI_DBG_INFO,"[%s]min_d +dispRange out of the range !\n ",__func__);
		CVI_TRACE_DPU(CVI_DBG_INFO,"min_dt(%d),dispRange(%d),width(%d)",min_d,dispRange,width);
		return CVI_FAILURE;
	}

	CVI_TRACE_DPU(CVI_DBG_INFO, " dpu_check_param          -\n");
	return CVI_SUCCESS;
}

s32 dpu_reg_config(DPU_GRP grp)
{
	s32 ret;
	CVI_TRACE_DPU(CVI_DBG_INFO, " dpu_reg_config          +\n");
	ret = dpu_check_param(grp);
	if(ret != CVI_SUCCESS){
		CVI_TRACE_DPU(CVI_DBG_INFO,"[%s]dou check param fail !\n ",__func__);
		return CVI_FAILURE;
	}
	handler_ctx[0].enDpuMode = dpuCtx[grp]->stGrpAttr.enDpuMode;
	cvi_dpu_reg.reg_dpu_enable =1;
	cvi_dpu_reg.reg_dpu_img_width = dpuCtx[grp]->stGrpAttr.stLeftImageSize.u32Width;
	cvi_dpu_reg.reg_dpu_img_height = dpuCtx[grp]->stGrpAttr.stLeftImageSize.u32Height;
	cvi_dpu_reg.reg_dpu_fgs_img_width = dpuCtx[grp]->stGrpAttr.stLeftImageSize.u32Width;
	cvi_dpu_reg.reg_dpu_fgs_img_height = dpuCtx[grp]->stGrpAttr.stLeftImageSize.u32Height;

	if(dpuCtx[grp]->stGrpAttr.enMaskMode == DPU_MASK_MODE_DEFAULT ||
		dpuCtx[grp]->stGrpAttr.enMaskMode == DPU_MASK_MODE_7x7){
		cvi_dpu_reg.reg_dpu_bfw_size =3;
	} else if(dpuCtx[grp]->stGrpAttr.enMaskMode == DPU_MASK_MODE_1x1){
		cvi_dpu_reg.reg_dpu_bfw_size =0;
	} else if(dpuCtx[grp]->stGrpAttr.enMaskMode == DPU_MASK_MODE_3x3){
		cvi_dpu_reg.reg_dpu_bfw_size =1;
	} else if(dpuCtx[grp]->stGrpAttr.enMaskMode == DPU_MASK_MODE_5x5){
		cvi_dpu_reg.reg_dpu_bfw_size =2;
	} else {
		CVI_TRACE_DPU(CVI_DBG_INFO,"[%s]the index of enMaskMode out of the range !\n ",__func__);
		return CVI_FAILURE;
	}

	if(dpuCtx[grp]->stGrpAttr.enDispRange == DPU_DISP_RANGE_DEFAULT ||
		dpuCtx[grp]->stGrpAttr.enDispRange == DPU_DISP_RANGE_16) {
		cvi_dpu_reg.reg_dpu_disp_range = 0;
	}else if(dpuCtx[grp]->stGrpAttr.enDispRange == DPU_DISP_RANGE_32){
		cvi_dpu_reg.reg_dpu_disp_range = 1;
	}else if(dpuCtx[grp]->stGrpAttr.enDispRange == DPU_DISP_RANGE_48){
		cvi_dpu_reg.reg_dpu_disp_range = 2;
	}else if(dpuCtx[grp]->stGrpAttr.enDispRange == DPU_DISP_RANGE_64){
		cvi_dpu_reg.reg_dpu_disp_range = 3;
	}else if(dpuCtx[grp]->stGrpAttr.enDispRange == DPU_DISP_RANGE_80){
		cvi_dpu_reg.reg_dpu_disp_range = 4;
	}else if(dpuCtx[grp]->stGrpAttr.enDispRange == DPU_DISP_RANGE_96){
		cvi_dpu_reg.reg_dpu_disp_range = 5;
	}else if(dpuCtx[grp]->stGrpAttr.enDispRange == DPU_DISP_RANGE_112){
		cvi_dpu_reg.reg_dpu_disp_range = 6;
	}else if(dpuCtx[grp]->stGrpAttr.enDispRange == DPU_DISP_RANGE_128){
		cvi_dpu_reg.reg_dpu_disp_range = 7;
	}else{
		CVI_TRACE_DPU(CVI_DBG_INFO,"[%s]the index of enDispRange out of the range !\n ",__func__);
		return CVI_FAILURE;
	}

	if(dpuCtx[grp]->stGrpAttr.enDccDir == DPU_DCC_DIR_DEFAULT ||
		dpuCtx[grp]->stGrpAttr.enDccDir == DPU_DCC_DIR_A12){
		cvi_dpu_reg.reg_dpu_dcc_a234 = 0;
	}else if(dpuCtx[grp]->stGrpAttr.enDccDir == DPU_DCC_DIR_A13) {
		cvi_dpu_reg.reg_dpu_dcc_a234 = 1;
	}else if(dpuCtx[grp]->stGrpAttr.enDccDir == DPU_DCC_DIR_A14) {
		cvi_dpu_reg.reg_dpu_dcc_a234 = 2;
	}else{
		CVI_TRACE_DPU(CVI_DBG_INFO,"[%s]the index of enDccDir out of the range !\n ",__func__);
		return CVI_FAILURE;
	}

	cvi_dpu_reg.reg_dpu_min_d = dpuCtx[grp]->stGrpAttr.u16DispStartPos;
	cvi_dpu_reg.reg_dpu_rshift1 = dpuCtx[grp]->stGrpAttr.u32Rshift1;
	cvi_dpu_reg.reg_dpu_rshift2 = dpuCtx[grp]->stGrpAttr.u32Rshift2;
	cvi_dpu_reg.reg_dpu_ca_p1 =  dpuCtx[grp]->stGrpAttr.u32CaP1;
	cvi_dpu_reg.reg_dpu_ca_p2 =  dpuCtx[grp]->stGrpAttr.u32CaP2;
	cvi_dpu_reg.reg_dpu_uniq_ratio = dpuCtx[grp]->stGrpAttr.u32UniqRatio;
	cvi_dpu_reg.reg_dpu_disp_shift = dpuCtx[grp]->stGrpAttr.u32DispShift;
	cvi_dpu_reg.reg_dpu_census_shift = dpuCtx[grp]->stGrpAttr.u32CensusShift;


	cvi_dpu_reg.reg_dpu_fxbaseline = dpuCtx[grp]->stGrpAttr.u32FxBaseline;
	cvi_dpu_reg.reg_dpu_fgs_max_count = dpuCtx[grp]->stGrpAttr.u32FgsMaxCount;
	cvi_dpu_reg.reg_dpu_fgs_max_t= dpuCtx[grp]->stGrpAttr.u32FgsMaxT;

	if(dpuCtx[grp]->stGrpAttr.enDpuDepthUnit == DPU_DEPTH_UNIT_DEFAULT ||
		dpuCtx[grp]->stGrpAttr.enDpuDepthUnit == DPU_DEPTH_UNIT_MM){
			cvi_dpu_reg.reg_dpu_fgs_output_unit_choose = 0;
	}else if(dpuCtx[grp]->stGrpAttr.enDpuDepthUnit == DPU_DEPTH_UNIT_CM){
		cvi_dpu_reg.reg_dpu_fgs_output_unit_choose = 1;
	}else if(dpuCtx[grp]->stGrpAttr.enDpuDepthUnit == DPU_DEPTH_UNIT_DM){
		cvi_dpu_reg.reg_dpu_fgs_output_unit_choose = 2;
	}else if(dpuCtx[grp]->stGrpAttr.enDpuDepthUnit == DPU_DEPTH_UNIT_M){
		cvi_dpu_reg.reg_dpu_fgs_output_unit_choose = 3;
	}else{
		CVI_TRACE_DPU(CVI_DBG_INFO,"[%s]the index of enDpuDepthUnit out of the range !\n ",__func__);
		return CVI_FAILURE;
	}

	if(dpuCtx[grp]->stGrpAttr.enDpuMode == DPU_MODE_DEFAULT ||
		dpuCtx[grp]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_MUX0){
		cvi_dpu_reg.reg_sgbm2fgs_online = 0;
		cvi_dpu_reg.reg_dpu_sgbm_enable =1;
		cvi_dpu_reg.reg_dpu_fgs_enable =0;

		cvi_dpu_reg.reg_dpu_data_sel = 0;

		cvi_dpu_reg.reg_sgbm_ld1_dma_enable =1;
		cvi_dpu_reg.reg_sgbm_ld2_dma_enable =1;
		cvi_dpu_reg.reg_sgbm_mux_st_dma_enable =1;
		cvi_dpu_reg.reg_sgbm_bf_st_dma_enable =0;

		cvi_dpu_reg.reg_dma_enable_fgs1 =0;
		cvi_dpu_reg.reg_dma_enable_fgs2 =0;
		cvi_dpu_reg.reg_dma_enable_fgs3 =0;
		cvi_dpu_reg.reg_dma_enable_fgs4 =0;

		cvi_dpu_reg.reg_sgbm_ld1_crop_enable =1;
		cvi_dpu_reg.reg_sgbm_ld2_crop_enable =1;
		cvi_dpu_reg.reg_sgbm_mux_st_crop_enable =1;
		cvi_dpu_reg.reg_sgbm_bf_st_crop_enable =0;

		cvi_dpu_reg.reg_crop_enable_fgs_gx = 0;
		cvi_dpu_reg.reg_crop_enable_fgs_chfh =0;
		cvi_dpu_reg.reg_crop_enable_fgs_independent =0;
		cvi_dpu_reg.reg_crop_enable_fgs_ux =0;

		cvi_dpu_reg.reg_dpu_fgs_output_bit_choose =0;
		cvi_dpu_reg.reg_dpu_src_disp_mux =0;
	}else if(dpuCtx[grp]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_MUX1){
		cvi_dpu_reg.reg_sgbm2fgs_online = 0;
		cvi_dpu_reg.reg_dpu_sgbm_enable =1;
		cvi_dpu_reg.reg_dpu_fgs_enable =0;

		cvi_dpu_reg.reg_dpu_data_sel = 1;

		cvi_dpu_reg.reg_sgbm_ld1_dma_enable =1;
		cvi_dpu_reg.reg_sgbm_ld2_dma_enable =1;
		cvi_dpu_reg.reg_sgbm_mux_st_dma_enable =1;
		cvi_dpu_reg.reg_sgbm_bf_st_dma_enable =0;

		cvi_dpu_reg.reg_dma_enable_fgs1 =0;
		cvi_dpu_reg.reg_dma_enable_fgs2 =0;
		cvi_dpu_reg.reg_dma_enable_fgs3 =0;
		cvi_dpu_reg.reg_dma_enable_fgs4 =0;

		cvi_dpu_reg.reg_sgbm_ld1_crop_enable =1;
		cvi_dpu_reg.reg_sgbm_ld2_crop_enable =1;
		cvi_dpu_reg.reg_sgbm_mux_st_crop_enable =1;
		cvi_dpu_reg.reg_sgbm_bf_st_crop_enable =0;

		cvi_dpu_reg.reg_crop_enable_fgs_gx = 0;
		cvi_dpu_reg.reg_crop_enable_fgs_chfh =0;
		cvi_dpu_reg.reg_crop_enable_fgs_independent =0;
		cvi_dpu_reg.reg_crop_enable_fgs_ux =0;

		cvi_dpu_reg.reg_dpu_fgs_output_bit_choose =0;
		cvi_dpu_reg.reg_dpu_src_disp_mux =0;
	}else if(dpuCtx[grp]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_MUX2){
		cvi_dpu_reg.reg_sgbm2fgs_online = 0;
		cvi_dpu_reg.reg_dpu_sgbm_enable =1;
		cvi_dpu_reg.reg_dpu_fgs_enable =0;

		cvi_dpu_reg.reg_dpu_data_sel = 2;

		cvi_dpu_reg.reg_sgbm_ld1_dma_enable =1;
		cvi_dpu_reg.reg_sgbm_ld2_dma_enable =1;
		cvi_dpu_reg.reg_sgbm_mux_st_dma_enable =1;
		cvi_dpu_reg.reg_sgbm_bf_st_dma_enable =0;

		cvi_dpu_reg.reg_dma_enable_fgs1 =0;
		cvi_dpu_reg.reg_dma_enable_fgs2 =0;
		cvi_dpu_reg.reg_dma_enable_fgs3 =0;
		cvi_dpu_reg.reg_dma_enable_fgs4 =0;

		cvi_dpu_reg.reg_sgbm_ld1_crop_enable =1;
		cvi_dpu_reg.reg_sgbm_ld2_crop_enable =1;
		cvi_dpu_reg.reg_sgbm_mux_st_crop_enable =1;
		cvi_dpu_reg.reg_sgbm_bf_st_crop_enable =0;

		cvi_dpu_reg.reg_crop_enable_fgs_gx = 0;
		cvi_dpu_reg.reg_crop_enable_fgs_chfh =0;
		cvi_dpu_reg.reg_crop_enable_fgs_independent =0;
		cvi_dpu_reg.reg_crop_enable_fgs_ux =0;

		cvi_dpu_reg.reg_dpu_fgs_output_bit_choose =0;
		cvi_dpu_reg.reg_dpu_src_disp_mux =0;
	}else if(dpuCtx[grp]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_FGS_ONLINE_MUX0){
		cvi_dpu_reg.reg_sgbm2fgs_online = 1;
		cvi_dpu_reg.reg_dpu_sgbm_enable =1;
		cvi_dpu_reg.reg_dpu_fgs_enable =1;

		cvi_dpu_reg.reg_dpu_data_sel = 2;

		cvi_dpu_reg.reg_sgbm_ld1_dma_enable =1;
		cvi_dpu_reg.reg_sgbm_ld2_dma_enable =1;
		cvi_dpu_reg.reg_sgbm_mux_st_dma_enable =0;
		cvi_dpu_reg.reg_sgbm_bf_st_dma_enable =0;

		cvi_dpu_reg.reg_dma_enable_fgs1 =1;
		cvi_dpu_reg.reg_dma_enable_fgs2 =1;
		cvi_dpu_reg.reg_dma_enable_fgs3 =1;
		cvi_dpu_reg.reg_dma_enable_fgs4 =1;

		cvi_dpu_reg.reg_sgbm_ld1_crop_enable =1;
		cvi_dpu_reg.reg_sgbm_ld2_crop_enable =1;
		cvi_dpu_reg.reg_sgbm_mux_st_crop_enable =0;
		cvi_dpu_reg.reg_sgbm_bf_st_crop_enable =0;

		cvi_dpu_reg.reg_crop_enable_fgs_gx = 0;
		cvi_dpu_reg.reg_crop_enable_fgs_chfh =1;
		cvi_dpu_reg.reg_crop_enable_fgs_independent =1;
		cvi_dpu_reg.reg_crop_enable_fgs_ux =1;

		cvi_dpu_reg.reg_dpu_fgs_output_bit_choose =0;
		cvi_dpu_reg.reg_dpu_src_disp_mux =0;
	}else if(dpuCtx[grp]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_FGS_ONLINE_MUX1){
		cvi_dpu_reg.reg_sgbm2fgs_online = 1;
		cvi_dpu_reg.reg_dpu_sgbm_enable =1;
		cvi_dpu_reg.reg_dpu_fgs_enable =1;

		cvi_dpu_reg.reg_dpu_data_sel = 2;

		cvi_dpu_reg.reg_sgbm_ld1_dma_enable =1;
		cvi_dpu_reg.reg_sgbm_ld2_dma_enable =1;
		cvi_dpu_reg.reg_sgbm_mux_st_dma_enable =0;
		cvi_dpu_reg.reg_sgbm_bf_st_dma_enable =0;

		cvi_dpu_reg.reg_dma_enable_fgs1 =1;
		cvi_dpu_reg.reg_dma_enable_fgs2 =1;
		cvi_dpu_reg.reg_dma_enable_fgs3 =1;
		cvi_dpu_reg.reg_dma_enable_fgs4 =1;

		cvi_dpu_reg.reg_sgbm_ld1_crop_enable =1;
		cvi_dpu_reg.reg_sgbm_ld2_crop_enable =1;
		cvi_dpu_reg.reg_sgbm_mux_st_crop_enable =0;
		cvi_dpu_reg.reg_sgbm_bf_st_crop_enable =0;

		cvi_dpu_reg.reg_crop_enable_fgs_gx = 0;
		cvi_dpu_reg.reg_crop_enable_fgs_chfh =1;
		cvi_dpu_reg.reg_crop_enable_fgs_independent =1;
		cvi_dpu_reg.reg_crop_enable_fgs_ux =1;

		cvi_dpu_reg.reg_dpu_fgs_output_bit_choose =1;
		cvi_dpu_reg.reg_dpu_src_disp_mux =0;
	}else if(dpuCtx[grp]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_FGS_ONLINE_MUX2){
		cvi_dpu_reg.reg_sgbm2fgs_online = 1;
		cvi_dpu_reg.reg_dpu_sgbm_enable =1;
		cvi_dpu_reg.reg_dpu_fgs_enable =1;

		cvi_dpu_reg.reg_dpu_data_sel = 2;

		cvi_dpu_reg.reg_sgbm_ld1_dma_enable =1;
		cvi_dpu_reg.reg_sgbm_ld2_dma_enable =1;
		cvi_dpu_reg.reg_sgbm_mux_st_dma_enable =0;
		cvi_dpu_reg.reg_sgbm_bf_st_dma_enable =0;

		cvi_dpu_reg.reg_dma_enable_fgs1 =1;
		cvi_dpu_reg.reg_dma_enable_fgs2 =1;
		cvi_dpu_reg.reg_dma_enable_fgs3 =1;
		cvi_dpu_reg.reg_dma_enable_fgs4 =1;

		cvi_dpu_reg.reg_sgbm_ld1_crop_enable =1;
		cvi_dpu_reg.reg_sgbm_ld2_crop_enable =1;
		cvi_dpu_reg.reg_sgbm_mux_st_crop_enable =0;
		cvi_dpu_reg.reg_sgbm_bf_st_crop_enable =0;

		cvi_dpu_reg.reg_crop_enable_fgs_gx = 0;
		cvi_dpu_reg.reg_crop_enable_fgs_chfh =1;
		cvi_dpu_reg.reg_crop_enable_fgs_independent =1;
		cvi_dpu_reg.reg_crop_enable_fgs_ux =1;

		cvi_dpu_reg.reg_dpu_fgs_output_bit_choose =1;
		cvi_dpu_reg.reg_dpu_src_disp_mux =1;
	}else if(dpuCtx[grp]->stGrpAttr.enDpuMode == DPU_MODE_FGS_MUX0){
		cvi_dpu_reg.reg_sgbm2fgs_online = 0;
		cvi_dpu_reg.reg_dpu_sgbm_enable =0;
		cvi_dpu_reg.reg_dpu_fgs_enable =1;

		cvi_dpu_reg.reg_dpu_data_sel = 2;

		cvi_dpu_reg.reg_sgbm_ld1_dma_enable =0;
		cvi_dpu_reg.reg_sgbm_ld2_dma_enable =0;
		cvi_dpu_reg.reg_sgbm_mux_st_dma_enable =0;
		cvi_dpu_reg.reg_sgbm_bf_st_dma_enable =0;

		cvi_dpu_reg.reg_dma_enable_fgs1 =1;
		cvi_dpu_reg.reg_dma_enable_fgs2 =1;
		cvi_dpu_reg.reg_dma_enable_fgs3 =1;
		cvi_dpu_reg.reg_dma_enable_fgs4 =1;

		cvi_dpu_reg.reg_sgbm_ld1_crop_enable =0;
		cvi_dpu_reg.reg_sgbm_ld2_crop_enable =0;
		cvi_dpu_reg.reg_sgbm_mux_st_crop_enable =0;
		cvi_dpu_reg.reg_sgbm_bf_st_crop_enable =0;

		cvi_dpu_reg.reg_crop_enable_fgs_gx = 0;
		cvi_dpu_reg.reg_crop_enable_fgs_chfh =1;
		cvi_dpu_reg.reg_crop_enable_fgs_independent =1;
		cvi_dpu_reg.reg_crop_enable_fgs_ux =1;

		cvi_dpu_reg.reg_dpu_fgs_output_bit_choose =0;
		cvi_dpu_reg.reg_dpu_src_disp_mux =0;
	}else if(dpuCtx[grp]->stGrpAttr.enDpuMode == DPU_MODE_FGS_MUX1){
		cvi_dpu_reg.reg_sgbm2fgs_online = 0;
		cvi_dpu_reg.reg_dpu_sgbm_enable =0;
		cvi_dpu_reg.reg_dpu_fgs_enable =1;

		cvi_dpu_reg.reg_dpu_data_sel = 2;

		cvi_dpu_reg.reg_sgbm_ld1_dma_enable =0;
		cvi_dpu_reg.reg_sgbm_ld2_dma_enable =0;
		cvi_dpu_reg.reg_sgbm_mux_st_dma_enable =0;
		cvi_dpu_reg.reg_sgbm_bf_st_dma_enable =0;

		cvi_dpu_reg.reg_dma_enable_fgs1 =1;
		cvi_dpu_reg.reg_dma_enable_fgs2 =1;
		cvi_dpu_reg.reg_dma_enable_fgs3 =1;
		cvi_dpu_reg.reg_dma_enable_fgs4 =1;

		cvi_dpu_reg.reg_sgbm_ld1_crop_enable =0;
		cvi_dpu_reg.reg_sgbm_ld2_crop_enable =0;
		cvi_dpu_reg.reg_sgbm_mux_st_crop_enable =0;
		cvi_dpu_reg.reg_sgbm_bf_st_crop_enable =0;

		cvi_dpu_reg.reg_crop_enable_fgs_gx = 0;
		cvi_dpu_reg.reg_crop_enable_fgs_chfh =1;
		cvi_dpu_reg.reg_crop_enable_fgs_independent =1;
		cvi_dpu_reg.reg_crop_enable_fgs_ux =1;

		cvi_dpu_reg.reg_dpu_fgs_output_bit_choose =1;
		cvi_dpu_reg.reg_dpu_src_disp_mux =0;
	}

	if(dpuCtx[grp]->stGrpAttr.bIsBtcostOut){
		cvi_dpu_reg.reg_sgbm_bf_st_dma_enable =1;
		cvi_dpu_reg.reg_sgbm_bf_st_crop_enable =1;
	}
	CVI_TRACE_DPU(CVI_DBG_INFO, " dpu_reg_config          -\n");
	return CVI_SUCCESS;
}

void dpu_write_sgbm_all_reg(void)
{
	u32 reg_04;
	u32 reg_08;
	u32 reg_0C;
	u32 reg_10;
	u32 reg_14;
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Write] Write dpu sgbm Reg configurations ...\n");
    //reg_00
    reg_write_mask(reg_base + DPU_REG_00_OFS,((1 << 1)-1) << 0,1);
    //reg_04
    reg_04 = (cvi_dpu_reg.reg_dpu_img_width-1) | ((cvi_dpu_reg.reg_dpu_img_height-1) << 16);
    write_reg(reg_base + DPU_REG_04_OFS ,reg_04);
    //reg_08
    reg_08 = cvi_dpu_reg.reg_dpu_min_d ;
    write_reg(reg_base + DPU_REG_08_OFS ,reg_08);
    //reg_0C
    reg_0C = (cvi_dpu_reg.reg_dpu_rshift1 << 8) \
                    | (cvi_dpu_reg.reg_dpu_rshift2 << 12);
    write_reg(reg_base+DPU_REG_0C_OFS ,reg_0C);
    //reg_10
    reg_10 = cvi_dpu_reg.reg_dpu_ca_p1 | (cvi_dpu_reg.reg_dpu_ca_p2 << 16);
    write_reg(reg_base+DPU_REG_10_OFS ,reg_10);
    //reg_14
    reg_14 = cvi_dpu_reg.reg_dpu_uniq_ratio | (cvi_dpu_reg.reg_dpu_disp_shift << 8) \
                    | (cvi_dpu_reg.reg_dpu_bfw_size << 16) \
                    | (cvi_dpu_reg.reg_dpu_census_shift << 18);
    write_reg(reg_base + DPU_REG_14_OFS ,reg_14);
    //reg_1C
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 20,cvi_dpu_reg.reg_sgbm_bf_st_dma_enable << 20 );
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 21,cvi_dpu_reg.reg_sgbm_ld1_dma_enable << 21 );
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 22,cvi_dpu_reg.reg_sgbm_ld2_dma_enable << 22 );
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 23,cvi_dpu_reg.reg_sgbm_mux_st_dma_enable << 23 );
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 29,cvi_dpu_reg.reg_sgbm_bf_st_crop_enable << 29 );
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 30,cvi_dpu_reg.reg_sgbm_ld1_crop_enable << 30 );
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 31,cvi_dpu_reg.reg_sgbm_ld2_crop_enable << 31 );
    //reg_68
    reg_write_mask(reg_base + DPU_REG_68_OFS, ((1 << 1)-1) << 2,cvi_dpu_reg.reg_sgbm_mux_st_crop_enable << 2 );
    reg_write_mask(reg_base + DPU_REG_68_OFS, ((1 << 1)-1) << 12,cvi_dpu_reg.reg_dpu_sgbm_enable << 12 );
    reg_write_mask(reg_base + DPU_REG_68_OFS, ((1 << 1)-1) << 16,cvi_dpu_reg.reg_sgbm2fgs_online << 16 );
    //reg_74
    reg_write_mask(reg_base + DPU_REG_74_OFS, ((1 << 3)-1) << 29,cvi_dpu_reg.reg_dpu_disp_range << 29 );
    //reg_78
    reg_write_mask(reg_base + DPU_REG_78_OFS, ((1 << 3)-1) << 0,cvi_dpu_reg.reg_dpu_dcc_a234 << 0 );
    //reg_88
    reg_write_mask(reg_base + DPU_REG_88_OFS, ((1 << 2)-1) << 1,cvi_dpu_reg.reg_dpu_data_sel << 1);
    reg_write_mask(reg_base + DPU_REG_88_OFS, ((1 << 1)-1) << 3,cvi_dpu_reg.reg_sgbm_bf_st_crop_enable << 3);

    CVI_TRACE_DPU(CVI_DBG_INFO,"[Write] Write dpu sgbm Reg Done ...\n");
}

void dpu_write_fgs_all_reg(void)
{
	u32 reg_70;
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Write] Write dpu fgs Reg configurations ...\n");

    //reg_1C
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 5)-1) << 0,cvi_dpu_reg.reg_dpu_fgs_max_count << 0 );
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 24,cvi_dpu_reg.reg_dma_enable_fgs1 << 24 );
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 25,cvi_dpu_reg.reg_dma_enable_fgs2 << 25 );
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 26,cvi_dpu_reg.reg_dma_enable_fgs3 << 26 );
    reg_write_mask(reg_base + DPU_REG_1C_OFS, ((1 << 1)-1) << 27,cvi_dpu_reg.reg_dma_enable_fgs4 << 27 );
    //reg_68
    reg_write_mask(reg_base + DPU_REG_68_OFS, ((1 << 1)-1) << 3,cvi_dpu_reg.reg_crop_enable_fgs_chfh << 3 );
    reg_write_mask(reg_base + DPU_REG_68_OFS, ((1 << 1)-1) << 4,cvi_dpu_reg.reg_crop_enable_fgs_gx << 4 );
    reg_write_mask(reg_base + DPU_REG_68_OFS, ((1 << 1)-1) << 5,cvi_dpu_reg.reg_crop_enable_fgs_ux << 5 );
    reg_write_mask(reg_base + DPU_REG_68_OFS, ((1 << 1)-1) << 13,cvi_dpu_reg.reg_dpu_fgs_enable << 13 );
	reg_write_mask(reg_base + DPU_REG_68_OFS, ((1 << 1)-1) << 16,cvi_dpu_reg.reg_sgbm2fgs_online << 16 );
    //reg_70
    reg_70 = (cvi_dpu_reg.reg_dpu_fgs_img_width-1) | ((cvi_dpu_reg.reg_dpu_fgs_img_height-1) << 16);
    write_reg(reg_base + DPU_REG_70_OFS ,reg_70);

    //reg_74
    reg_write_mask(reg_base + DPU_REG_74_OFS, ((1 << 8)-1),cvi_dpu_reg.reg_dpu_nd_ds);
    reg_write_mask(reg_base + DPU_REG_74_OFS, ((1 << 20)-1) << 8,cvi_dpu_reg.reg_dpu_fxbaseline << 8);
    reg_write_mask(reg_base + DPU_REG_74_OFS, ((1 << 1)-1) << 28,cvi_dpu_reg.reg_dpu_fgs_output_bit_choose << 28);
    //reg_78
    reg_write_mask(reg_base + DPU_REG_78_OFS, ((1 << 2)-1) << 16,cvi_dpu_reg.reg_dpu_src_disp_mux << 16 );
    reg_write_mask(reg_base + DPU_REG_78_OFS, ((1 << 7)-1) << 18,cvi_dpu_reg.reg_dpu_fgs_max_t << 18);
    //reg_88
    reg_write_mask(reg_base + DPU_REG_88_OFS, ((1 << 1)-1) << 0,cvi_dpu_reg.reg_crop_enable_fgs_independent << 0 );
    //reg_98
    reg_write_mask(reg_base + DPU_REG_98_OFS, ((1 << 2)-1) << 16,cvi_dpu_reg.reg_dpu_fgs_output_unit_choose << 16);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Write] Write dpu fgs Reg configurations Done ...\n");
};

static s32 commit_hw_settings(DPU_GRP workingGrp)
{
	u32 seg_len;
	u32 seg_num;
	CVI_TRACE_DPU(CVI_DBG_INFO, " commit_hw_settings          +\n");

	write_reg(reg_base+DPU_REG_90_OFS,0);
    write_reg(reg_base+DPU_REG_94_OFS,0);
    reg_write_mask(reg_base+DPU_REG_98_OFS,((1<<1)-1) <<0,0 << 0);
    reg_write_mask(reg_base+DPU_REG_98_OFS,((1<<1)-1) <<1,0 << 1);
	if(dpuCtx[workingGrp]->stGrpAttr.bIsBtcostOut){
		seg_len= cvi_dpu_reg.reg_dpu_img_width;
		seg_num= cvi_dpu_reg.reg_dpu_img_height;
		register_sgbm_bf_st_ld(seg_len, seg_num,dram_base_out_btcost_h, \
						dram_base_out_btcost_l,reg_base_sgbm_bf_dma, reg_base);
	}

	if(dpuCtx[workingGrp]->stGrpAttr.enDpuMode == DPU_MODE_DEFAULT  ||
		dpuCtx[workingGrp]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_MUX0 ||
		dpuCtx[workingGrp]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_MUX1 ||
		dpuCtx[workingGrp]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_MUX2 ){
		seg_len= cvi_dpu_reg.reg_dpu_img_width;
		seg_num= cvi_dpu_reg.reg_dpu_img_height;
		register_sgbm_ld1_ld(seg_len, seg_num,dram_base_left_h, \
						dram_base_left_l,reg_base_sgbm_ld1_dma, reg_base);
		register_sgbm_ld2_ld(seg_len, seg_num,dram_base_right_h, \
						dram_base_right_l,reg_base_sgbm_ld2_dma, reg_base);
		register_sgbm_median_st_ld(seg_len, seg_num,cvi_dpu_reg.reg_dpu_data_sel,\
								dram_base_out_h,dram_base_out_l,reg_base_sgbm_median_dma, reg_base);
		dpu_write_sgbm_all_reg();
		CVI_TRACE_DPU(CVI_DBG_INFO,"[%s]write sgbm reg\n",__func__);
	}else if(dpuCtx[workingGrp]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_FGS_ONLINE_MUX0 ||
			 dpuCtx[workingGrp]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_FGS_ONLINE_MUX1 ||
			 dpuCtx[workingGrp]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_FGS_ONLINE_MUX2 ){
		seg_len= cvi_dpu_reg.reg_dpu_img_width;
		seg_num= cvi_dpu_reg.reg_dpu_img_height;
		register_sgbm_ld1_ld(seg_len, seg_num,dram_base_left_h, \
						dram_base_left_l,reg_base_sgbm_ld1_dma, reg_base);
		register_sgbm_ld2_ld(seg_len, seg_num,dram_base_right_h, \
						dram_base_right_l,reg_base_sgbm_ld2_dma, reg_base);
		register_fgs_gx_ld(seg_len, seg_num,dram_base_left_h, \
						dram_base_left_l,reg_base_fgs_gx_dma, reg_base);
		register_fgs_chfh_ld(seg_len, seg_num,dram_base_right_h, \
						dram_base_right_l,reg_base_fgs_chfh_ld_dma, reg_base);
		register_fgs_chfh_st(seg_len, seg_num, dram_base_chfh_h, \
						dram_base_chfh_l,reg_base_fgs_chfh_st_dma, reg_base);
		register_fgs_ux_st(seg_len, seg_num,cvi_dpu_reg.reg_dpu_fgs_output_bit_choose,\
						dram_base_out_h, dram_base_out_l,reg_base_fgs_ux_dma, reg_base);
		dpu_write_sgbm_all_reg();
		dpu_write_fgs_all_reg();
		CVI_TRACE_DPU(CVI_DBG_INFO,"[%s]write sgbm reg\n",__func__);
		CVI_TRACE_DPU(CVI_DBG_INFO,"[%s]write fgs reg\n",__func__);
	}else if(dpuCtx[workingGrp]->stGrpAttr.enDpuMode == DPU_MODE_FGS_MUX0 ||
			 dpuCtx[workingGrp]->stGrpAttr.enDpuMode == DPU_MODE_FGS_MUX1 ){
		seg_len= cvi_dpu_reg.reg_dpu_fgs_img_width;
		seg_num= cvi_dpu_reg.reg_dpu_fgs_img_height;
		register_fgs_gx_ld(seg_len, seg_num,dram_base_left_h, \
						dram_base_left_l,reg_base_fgs_gx_dma, reg_base);
		register_fgs_chfh_ld(seg_len, seg_num,dram_base_right_h, \
						dram_base_right_l,reg_base_fgs_chfh_ld_dma, reg_base);
		register_fgs_chfh_st(seg_len, seg_num, dram_base_chfh_h, \
						dram_base_chfh_l,reg_base_fgs_chfh_st_dma, reg_base);
		register_fgs_ux_st(seg_len, seg_num,cvi_dpu_reg.reg_dpu_fgs_output_bit_choose,\
						dram_base_out_h, dram_base_out_l,reg_base_fgs_ux_dma, reg_base);
		dpu_write_fgs_all_reg();
		CVI_TRACE_DPU(CVI_DBG_INFO,"[%s]write fgs reg\n",__func__);
	}
	CVI_TRACE_DPU(CVI_DBG_INFO, " commit_hw_settings          -\n");
	return CVI_SUCCESS;
}

static s32 hw_start(DPU_GRP workgrp)
{
	CVI_TRACE_DPU(CVI_DBG_INFO, "hw_start          +\n");
	ktime_get_ts64(&dpu_dev->timeStart);
	dpu_engine(workgrp);
	CVI_TRACE_DPU(CVI_DBG_INFO, "hw_start          -\n");
	return CVI_SUCCESS;
}

static u8 dpu_handler_is_idle(void)
{
	int i;
	for (i = 0; i < DPU_MAX_GRP_NUM; i++)
		if (dpuCtx[i] && dpuCtx[i]->isCreated && dpuCtx[i]->isStarted)
			return CVI_FALSE;

	return CVI_TRUE;
}

static int32_t dpu_base_get_frame_info(VB_CAL_CONFIG_S stVbCalConfig,PIXEL_FORMAT_E fmt, SIZE_S size, struct cvi_buffer *buf, u64 mem_base)
{
	u8 i = 0;
	memset(buf, 0, sizeof(*buf));
	buf->size = size;
	buf->enPixelFormat = fmt;
	for (i = 0; i < stVbCalConfig.plane_num; ++i) {
		buf->phy_addr[i] = mem_base;
		buf->length[i] = ALIGN((i == 0) ? stVbCalConfig.u32MainYSize : stVbCalConfig.u32MainCSize,
					stVbCalConfig.u16AddrAlign);
		buf->stride[i] = (i == 0) ? stVbCalConfig.u32MainStride : stVbCalConfig.u32CStride;
		mem_base += buf->length[i];

		pr_debug("(%llx-%zu-%d)\n", buf->phy_addr[i], buf->length[i], buf->stride[i]);
	}

	return CVI_SUCCESS;
}

static void _dpu_fill_cvi_buffer(MMF_CHN_S chn, struct vb_s *grp_vb_in,
		uint64_t phy_addr, struct cvi_buffer *buf, struct cvi_dpu_ctx *ctx)
{

	SIZE_S size;
	CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_fill_cvi_buffer          +\n");
	size.u32Width = ctx->stChnCfgs[chn.s32ChnId].stChnAttr.stImgSize.u32Width;
	size.u32Height = ctx->stChnCfgs[chn.s32ChnId].stChnAttr.stImgSize.u32Height;
	dpu_base_get_frame_info( ctx->stChnCfgs[chn.s32ChnId].stVbCalConfig
			   , ctx->stChnCfgs[chn.s32ChnId].enPixelFormat
			   , size
			   , buf
			   , phy_addr);
	buf->s16OffsetTop = 0;
	buf->s16OffsetBottom =0;
	buf->s16OffsetLeft = 0;
	buf->s16OffsetRight =0;

	if (grp_vb_in) {
		buf->u64PTS = grp_vb_in->buf.u64PTS;
		buf->frm_num = grp_vb_in->buf.frm_num;
		buf->motion_lv = grp_vb_in->buf.motion_lv;
		memcpy(buf->motion_table, grp_vb_in->buf.motion_table, MO_TBL_SIZE);
	}
}


static s32 dpu_qbuf(MMF_CHN_S chn, struct vb_s *grp_vb_in,
	VB_BLK chn_vb_blk, struct cvi_dpu_ctx *ctx)
{

	VB_BLK blk = chn_vb_blk;
	CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_qbuf          +\n");
	_dpu_fill_cvi_buffer(chn, grp_vb_in, vb_handle2phys_addr(blk), &((struct vb_s *)blk)->buf, ctx);

	if (vb_qbuf(chn, CHN_TYPE_OUT, &gstDpuJobs[chn.s32DevId].outs[chn.s32ChnId], blk) == -ENOBUFS){
		CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) Chn(%d) qbuf failed\n", chn.s32DevId, chn.s32ChnId);
		return CVI_FAILURE;
	}
	vb_release_block(blk);
	CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_qbuf          -\n");
	return CVI_SUCCESS;
}

static s32 fill_buffers(DPU_GRP DpuGrp)
{
	DPU_CHN DpuChn ;
	VB_CAL_CONFIG_S stVbCalConfig_chfh;
	uint64_t phyAddr_out_arr[DPU_MAX_CHN_NUM] ;
	uint64_t phyAddr_left ;
	uint64_t phyAddr_right ;
	uint64_t phyAddr_out;
	uint64_t phyAddr_out_btcost;
	//VB_BLK blk_chfh = VB_INVALID_HANDLE;
	VB_BLK blk[DPU_MAX_CHN_NUM] = { [0 ... DPU_MAX_CHN_NUM - 1] = VB_INVALID_HANDLE };
	//struct vdev *d;
	MMF_CHN_S chn_left = {.enModId = CVI_ID_DPU, .s32DevId = DpuGrp, .s32ChnId = 0};
	MMF_CHN_S chn_right = {.enModId = CVI_ID_DPU, .s32DevId = DpuGrp, .s32ChnId = 1};
	MMF_CHN_S chn_out[DPU_MAX_CHN_NUM] = {chn_left, chn_right};
	s32 ret = CVI_SUCCESS;
	struct DPU_CHN_CFG *stChnCfg;
	struct vb_s *vb_in = NULL;

	struct cvi_buffer *buf_out[DPU_MAX_CHN_NUM];
	struct cvi_buffer *buf_left;
	struct cvi_buffer *buf_right;

	u8 is_true_left = CVI_FALSE;
	u8 is_true_right = CVI_FALSE;
	u8 is_true_out[DPU_MAX_CHN_NUM] = {CVI_FALSE};

	u8 is_null_out[DPU_MAX_CHN_NUM] = {CVI_TRUE};
	//char chfh_ion_name[10] ="dpuChfh";
	//u8 is_true_chfh = CVI_FALSE;
	CVI_TRACE_DPU(CVI_DBG_INFO, "fill_buffers          +\n");
	if ( base_mod_jobs_waitq_empty(&gstDpuJobs[DpuGrp].ins[0]) ||
		base_mod_jobs_waitq_empty(&gstDpuJobs[DpuGrp].ins[1])){
			CVI_TRACE_DPU(CVI_DBG_ERR, "waitq empty.\n");
			print_vbq_size(DpuGrp);
			return CVI_ERR_DPU_BUF_EMPTY;
		}

	buf_left = base_mod_jobs_enque_work(&gstDpuJobs[DpuGrp].ins[0]);
	if (buf_left == NULL) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "left frame Grp(%d) qbuf failed.\n", DpuGrp);
		ret = CVI_ERR_DPU_NOTREADY;
		goto ERR_FILL_BUF;
	}
	is_true_left=CVI_TRUE;

	buf_right = base_mod_jobs_enque_work(&gstDpuJobs[DpuGrp].ins[1]);
	if (buf_right == NULL) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "left frame Grp(%d) qbuf failed.\n", DpuGrp);
		ret = CVI_ERR_DPU_NOTREADY;
		goto ERR_FILL_BUF;
	}
	is_true_right=CVI_TRUE;
	for (DpuChn = 0; DpuChn < dpuCtx[DpuGrp]->chnNum; ++DpuChn) {
		if (!base_mod_jobs_waitq_empty(&gstDpuJobs[DpuGrp].outs[DpuChn])) {
			is_null_out[DpuChn]=CVI_FALSE;
		} else {
			is_null_out[DpuChn]=CVI_TRUE;
		}
	}
	vb_in = container_of(buf_left, struct vb_s, buf);
	// get buffers.
	for (DpuChn = 0; DpuChn < dpuCtx[DpuGrp]->chnNum; ++DpuChn) {
		stChnCfg = &dpuCtx[DpuGrp]->stChnCfgs[DpuChn];
		if (!stChnCfg->isEnabled)
			continue;

		// chn buffer from user
		if (!base_mod_jobs_waitq_empty(&gstDpuJobs[DpuGrp].outs[DpuChn])) {
			CVI_TRACE_DPU(CVI_DBG_INFO, "Grp(%d) Chn(%d) chn buffer from user.\n", DpuGrp, DpuChn);

			buf_out[DpuChn] = base_mod_jobs_enque_work(&gstDpuJobs[DpuGrp].outs[DpuChn]);
			if (!buf_out[DpuChn]) {
				CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) Chn(%d) qbuf failed.\n", DpuGrp, DpuChn);
				ret = CVI_ERR_DPU_NOTREADY;
				goto ERR_FILL_BUF;
			}
			is_true_out[DpuChn]=CVI_TRUE;
			phyAddr_out_arr[DpuChn] =buf_out[DpuChn]->phy_addr[0];

			continue;
		}

		// chn buffer from pool
		blk[DpuChn] = vb_get_block_with_id(dpuCtx[DpuGrp]->stChnCfgs[DpuChn].VbPool,
						dpuCtx[DpuGrp]->stChnCfgs[DpuChn].blk_size, CVI_ID_DPU);
		if (blk[DpuChn] == VB_INVALID_HANDLE) {
			CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) Chn(%d) Can't acquire VB BLK for DPU\n"
				, DpuGrp, DpuChn);
			ret = CVI_ERR_DPU_NOBUF;
			goto ERR_FILL_BUF;
		}
		phyAddr_out_arr[DpuChn] =vb_handle2phys_addr(blk[DpuChn]);
		if(dpu_qbuf(chn_out[DpuChn], vb_in, blk[DpuChn], dpuCtx[DpuGrp]) != CVI_SUCCESS){
			CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) chn-out(%d) qbuf fail.\n"
				, DpuGrp,DpuChn);
			ret = CVI_FAILURE;
			goto ERR_FILL_BUF;
		}
	}

	if(dpuCtx[DpuGrp]->stGrpAttr.enDpuMode == DPU_MODE_FGS_MUX0 ||
		dpuCtx[DpuGrp]->stGrpAttr.enDpuMode == DPU_MODE_FGS_MUX1 ||
		dpuCtx[DpuGrp]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_FGS_ONLINE_MUX0 ||
		dpuCtx[DpuGrp]->stGrpAttr.enDpuMode == DPU_MODE_SGBM_FGS_ONLINE_MUX1  ){

		COMMON_GetPicBufferConfig(dpuCtx[DpuGrp]->stGrpAttr.stLeftImageSize.u32Width, \
			dpuCtx[DpuGrp]->stGrpAttr.stLeftImageSize.u32Height, \
			PIXEL_FORMAT_YUV_400, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, ALIGN_32, &stVbCalConfig_chfh);
		// ret = base_ion_alloc(&dpuCtx[DpuGrp]->phyAddr_chfh, &dpuCtx[DpuGrp]->virAddr_chfh,
		// 	 (uint8_t *)chfh_ion_name, stVbCalConfig_chfh.u32VBSize*5, CVI_FALSE);
		// if (ret) {
		// 	CVI_TRACE_DPU(CVI_DBG_ERR,"base_ion_alloc fail! ret(%d)\n", ret);
		// 	goto ERR_FILL_BUF;
		// }
		// dram_base_chfh_l = dpuCtx[DpuGrp]->phyAddr_chfh &(0xFFFFFFFF);
		// dram_base_chfh_h = dpuCtx[DpuGrp]->phyAddr_chfh >> 32;
		dram_base_chfh_l = dpu_dev->phyAddr_chfh &(0xFFFFFFFF);
		dram_base_chfh_h = dpu_dev->phyAddr_chfh >> 32;
		CVI_TRACE_DPU(CVI_DBG_INFO, "chfh base(0x%llx) h(0x%x) l(0x%x) \n",
					dpu_dev->phyAddr_chfh,dram_base_chfh_h,dram_base_chfh_l);
	}

	phyAddr_out =phyAddr_out_arr[0];
	phyAddr_out_btcost =phyAddr_out_arr[1];
	phyAddr_left =buf_left->phy_addr[0];
	phyAddr_right =buf_right->phy_addr[0];

	dram_base_out_l = phyAddr_out &(0xFFFFFFFF);
	dram_base_out_h = phyAddr_out >> 32;

	dram_base_out_btcost_l = phyAddr_out_btcost &(0xFFFFFFFF);
	dram_base_out_btcost_h = phyAddr_out_btcost >> 32;

	dram_base_left_l = phyAddr_left &(0xFFFFFFFF);
	dram_base_left_h = phyAddr_left >> 32;

	dram_base_right_l = phyAddr_right &(0xFFFFFFFF);
	dram_base_right_h = phyAddr_right >> 32;

	dpuCtx[DpuGrp]->enPixelFormat = vb_in->buf.enPixelFormat;
	CVI_TRACE_DPU(CVI_DBG_INFO, "left base(0x%llx) h(0x%x) l(0x%x) \n",phyAddr_left,dram_base_left_h,dram_base_left_l);
	CVI_TRACE_DPU(CVI_DBG_INFO, "right base(0x%llx) h(0x%x) l(0x%x) \n",phyAddr_right,dram_base_right_h,dram_base_right_l);
	CVI_TRACE_DPU(CVI_DBG_INFO, "out base(0x%llx) h(0x%x) l(0x%x) \n",phyAddr_out,dram_base_out_h,dram_base_out_l);
	CVI_TRACE_DPU(CVI_DBG_INFO, "btcost out base(0x%llx) h(0x%x) l(0x%x) \n",phyAddr_out_btcost,dram_base_out_btcost_h,dram_base_out_btcost_l);

	CVI_TRACE_DPU(CVI_DBG_INFO, "fill_buffers          -\n");
	return ret;
ERR_FILL_BUF:
	for (DpuChn = 0; DpuChn < dpuCtx[DpuGrp]->chnNum; ++DpuChn) {
		if (blk[DpuChn] != VB_INVALID_HANDLE)
			vb_release_block(blk[DpuChn]);
	}

	if(dpuCtx[DpuGrp]->chfhBlk!= VB_INVALID_HANDLE)
		vb_release_block(dpuCtx[DpuGrp]->chfhBlk);

	if(is_true_left){
		VB_BLK BLK_temp = base_mod_jobs_workq_pop(&gstDpuJobs[DpuGrp].ins[0]);
		if (BLK_temp != VB_INVALID_HANDLE)
			vb_release_block(BLK_temp);
	} else {
		VB_BLK BLK_temp = base_mod_jobs_waitq_pop(&gstDpuJobs[DpuGrp].ins[0]);
		if (BLK_temp != VB_INVALID_HANDLE)
			vb_release_block(BLK_temp);
	}

	if(is_true_right){
		VB_BLK BLK_temp = base_mod_jobs_workq_pop(&gstDpuJobs[DpuGrp].ins[1]);
		if (BLK_temp != VB_INVALID_HANDLE)
			vb_release_block(BLK_temp);
	} else {
		VB_BLK BLK_temp = base_mod_jobs_waitq_pop(&gstDpuJobs[DpuGrp].ins[1]);
		if (BLK_temp != VB_INVALID_HANDLE)
			vb_release_block(BLK_temp);
	}

	for (DpuChn = 0; DpuChn < dpuCtx[DpuGrp]->chnNum; ++DpuChn) {
		if(!is_null_out[DpuChn]){
			if(is_true_out[DpuChn]){
				VB_BLK BLK_temp = base_mod_jobs_workq_pop(&gstDpuJobs[DpuGrp].outs[DpuChn]);
				if (BLK_temp != VB_INVALID_HANDLE)
					vb_release_block(BLK_temp);
			} else {
				VB_BLK BLK_temp = base_mod_jobs_waitq_pop(&gstDpuJobs[DpuGrp].outs[DpuChn]);
				if (BLK_temp != VB_INVALID_HANDLE)
					vb_release_block(BLK_temp);
			}
		}

	}

	return ret;
}

static void _release_dpu_waitq(MMF_CHN_S chn, enum CHN_TYPE_E chn_type)
{
	VB_BLK blk_grp;
	CVI_TRACE_DPU(CVI_DBG_INFO, "release_dpu_waitq          +\n");
	if (chn_type == CHN_TYPE_OUT)
		blk_grp = base_mod_jobs_waitq_pop(&gstDpuJobs[chn.s32DevId].outs[chn.s32ChnId]);
	else
		blk_grp = base_mod_jobs_waitq_pop(&gstDpuJobs[chn.s32DevId].ins[chn.s32ChnId]);

	if (blk_grp != VB_INVALID_HANDLE)
		vb_release_block(blk_grp);

	CVI_TRACE_DPU(CVI_DBG_INFO, "release_dpu_waitq          -\n");
}

static s32 dpu_try_schedule_offline(struct dpu_handler_ctx *ctx)
{

	u32 ret;
	DPU_GRP workingGrp = ctx->workingGrp;
	u8 workingMask = 0;

	u32 leftPipeId=0;
	u32 rightPipeId=1;
	MMF_CHN_S chn = {.enModId = CVI_ID_DPU, .s32DevId = workingGrp, .s32ChnId = 0};

	dpuCtx[workingGrp]->stGrpWorkStatus.StartCnt ++;
	CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_try_schedule_offline          +\n");
	ktime_get_ts64(&ctx->time);
	dpu_reset();

	// sc's mask
	workingMask = get_work_mask(dpuCtx[workingGrp]);

	if (workingMask == 0) {
		CVI_TRACE_DPU(CVI_DBG_NOTICE, "grp(%d) workingMask zero.\n", workingGrp);
		chn.s32ChnId=leftPipeId;
		_release_dpu_waitq(chn, CHN_TYPE_IN);
		chn.s32ChnId=rightPipeId;
		_release_dpu_waitq(chn, CHN_TYPE_IN);
		goto dpu_next_job;
	}

	ret =dpu_reg_config(workingGrp);
	if(ret != CVI_SUCCESS){
		CVI_TRACE_DPU(CVI_DBG_ERR, "grp(%d) dpu para config failed.\n", workingGrp);

		goto dpu_next_job;
	}
	// mutex_lock(&gstDpuJobs[workingGrp].lock);
	if (fill_buffers(workingGrp) != CVI_SUCCESS) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "grp(%d) fill buffer NG.\n", workingGrp);

		goto dpu_next_job;
	}

	// commit hw settings of this dpu-grp.
	CVI_TRACE_DPU(CVI_DBG_INFO, "reg_base(0x%llx) .\n",reg_base );
	CVI_TRACE_DPU(CVI_DBG_INFO, "reg_dma_sgbm_ld1(0x%llx) .\n",reg_base_sgbm_ld1_dma);
	CVI_TRACE_DPU(CVI_DBG_INFO, "reg_dma_sgbm_ld2(0x%llx) .\n",reg_base_sgbm_ld2_dma);
	CVI_TRACE_DPU(CVI_DBG_INFO, "reg_base_sgbm_median_dma(0x%llx) .\n",reg_base_sgbm_median_dma);
	CVI_TRACE_DPU(CVI_DBG_INFO, "reg_base_sgbm_bf_dma(0x%llx) .\n",reg_base_sgbm_bf_dma);
	CVI_TRACE_DPU(CVI_DBG_INFO, "reg_base_fgs_gx_dma(0x%llx) .\n",reg_base_fgs_gx_dma);
	CVI_TRACE_DPU(CVI_DBG_INFO, "reg_base_fgs_ux_dma(0x%llx) .\n",reg_base_fgs_ux_dma);
	CVI_TRACE_DPU(CVI_DBG_INFO, "reg_base_fgs_chfh_st_dma(0x%llx) .\n",reg_base_fgs_chfh_st_dma);
	CVI_TRACE_DPU(CVI_DBG_INFO, "reg_base_fgs_chfh_ld_dma(0x%llx) .\n",reg_base_fgs_chfh_ld_dma);
	commit_hw_settings(workingGrp);

	/* Update state first, isr could occur immediately */
	ctx->workingGrp = workingGrp;
	ctx->workingMask = workingMask;

	dpuCtx[workingGrp]->grp_state = GRP_STATE_HW_STARTED;
	print_vbq_size(workingGrp);
	hw_start(workingGrp);
	// mutex_unlock(&gstDpuJobs[workingGrp].lock);

	/* Should use async sbm, the lock region is too big ! */

	CVI_TRACE_DPU(CVI_DBG_INFO, "ctx[%d] workingGrp=%d\n",
			ctx->u8DpuDev, ctx->workingGrp);

	// wait for h/w done
	CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_try_schedule_offline          -\n");
	return CVI_SUCCESS;

dpu_next_job:
	// job done.
	ctx->workingMask = 0;
	dpuCtx[workingGrp]->grp_state = GRP_STATE_IDLE;

	dpuCtx[workingGrp]->stGrpWorkStatus.StartFailCnt++;
	CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_do_necxt_job         -\n");
	return CVI_FAILURE;
}

static void _dpu_handle_snap(MMF_CHN_S chn, enum CHN_TYPE_E chn_type, VB_BLK blk)
{

	struct vb_jobs_t *jobs = (chn_type == CHN_TYPE_OUT) ? &gstDpuJobs[chn.s32DevId].outs[chn.s32ChnId] : \
								&gstDpuJobs[chn.s32DevId].ins[chn.s32ChnId];
	struct vb_s *p = (struct vb_s *)blk;
	struct vbq *doneq;
	struct snap_s *s, *s_tmp;
	CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_handle_snap          +\n");
	if (chn.enModId != CVI_ID_DPU)
		return;

	if (jobs == NULL) {
		pr_err("handle snap fail, Null parameter\n");
		return;
	}

	if (!jobs->inited) {
		pr_err("handle snap fail, job not inited yet\n");
		return;
	}

	mutex_lock(&jobs->dlock);
	TAILQ_FOREACH_SAFE(s, &jobs->snap_jobs, tailq, s_tmp) {
		if (CHN_MATCH(&s->chn, &chn)) {
			TAILQ_REMOVE(&jobs->snap_jobs, s, tailq);
			s->blk = blk;
			atomic_fetch_add(1, &p->usr_cnt);
			atomic_long_fetch_or(BIT(CVI_ID_USER), &p->mod_ids);
			s->avail = CVI_TRUE;
			wake_up_all(&s->cond_queue);
			mutex_unlock(&jobs->dlock);
			return;
		}
	}

	doneq = _get_doneq(chn);
	// check if there is a snap-queue
	if (FIFO_CAPACITY(doneq)) {
		if (FIFO_FULL(doneq)) {
			struct vb_s *vb = NULL;

			FIFO_POP(doneq, &vb);
			atomic_long_fetch_and(~BIT(chn.enModId), &vb->mod_ids);
			vb_release_block((VB_BLK)vb);
		}
		atomic_fetch_add(1, &p->usr_cnt);
		atomic_long_fetch_or(BIT(chn.enModId), &p->mod_ids);
		FIFO_PUSH(doneq, p);
		CVI_TRACE_DPU(CVI_DBG_INFO, "push doneq(%d)  \n",FIFO_SIZE(doneq));
	}

	mutex_unlock(&jobs->dlock);
	CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_handle_snap          -\n");
}

int32_t dpu_vb_done_handler(MMF_CHN_S chn, enum CHN_TYPE_E chn_type, VB_BLK blk)
{
	MMF_BIND_DEST_S stBindDest;
	u8 i;
	s32 ret=0;
	struct  vb_s* vb;

	struct vb_jobs_t *jobs = (chn_type == CHN_TYPE_OUT) ? &gstDpuJobs[chn.s32DevId].outs[chn.s32ChnId] : \
								&gstDpuJobs[chn.s32DevId].ins[chn.s32ChnId];

	CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_vb_done_handler          +\n");
	CVI_TRACE_DPU(CVI_DBG_INFO, "chn is %d    -\n", chn.s32ChnId);

	if (chn_type == CHN_TYPE_OUT) {
		_dpu_handle_snap(chn, chn_type, blk);
		if (bind_get_dst(&chn, &stBindDest) == CVI_SUCCESS) {
			for (i = 0; i < stBindDest.u32Num; ++i) {
				vb_qbuf(stBindDest.astMmfChn[i], CHN_TYPE_IN, jobs, blk);
				pr_debug(" Mod(%s) chn(%d) dev(%d) -> Mod(%s) chn(%d) dev(%d)\n"
					     , sys_get_modname(chn.enModId), chn.s32ChnId, chn.s32DevId
					     , sys_get_modname(stBindDest.astMmfChn[i].enModId)
					     , stBindDest.astMmfChn[i].s32ChnId
					     , stBindDest.astMmfChn[i].s32DevId);
			}
		} else {
			// release if not found
			pr_debug("Mod(%s) chn(%d) dev(%d) src no dst release\n"
				     , sys_get_modname(chn.enModId), chn.s32ChnId, chn.s32DevId);
		}
	} else {
		pr_debug("Mod(%s) chn(%d) dev(%d) dst out release\n"
			     , sys_get_modname(chn.enModId), chn.s32ChnId, chn.s32DevId);
	}
	ret = vb_release_block(blk);
	vb = (struct  vb_s*)blk;
	CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_vb_done_handler   vb cnt(%d)       -\n",vb->usr_cnt.counter);
	CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_vb_done_handler          -\n");
	return ret;
}

static void dpu_handle_frame_done(struct dpu_handler_ctx *ctx)
{

	struct timespec64 time;
	VB_BLK blk_left;
	VB_BLK blk_right;
	VB_BLK blk_out[DPU_MAX_CHN_NUM];
	DPU_CHN DpuChn;
	u64 duration_s;
	DPU_GRP workingGrp = ctx->workingGrp;
	u8 workingMask = ctx->workingMask;

	MMF_CHN_S chn_left = {.enModId = CVI_ID_DPU, .s32DevId = workingGrp, .s32ChnId = 0};
	MMF_CHN_S chn_right = {.enModId = CVI_ID_DPU, .s32DevId = workingGrp, .s32ChnId = 1};
	MMF_CHN_S chn_out[DPU_MAX_CHN_NUM] = {chn_left, chn_right};
	u32 duration = 0;
	CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_handle_frame_done          +\n");
	CVI_TRACE_DPU(CVI_DBG_INFO, "ctx[%d] grp(%d) eof\n", ctx->u8DpuDev, workingGrp);

	dpuCtx[workingGrp]->grp_state = GRP_STATE_IDLE;

	vb_dqbuf(chn_left, &gstDpuJobs[workingGrp].ins[0], &blk_left);
	if (blk_left == VB_INVALID_HANDLE) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Mod(%d) can't get left vb-blk.\n", chn_left.enModId);
	} else {
		dpu_vb_done_handler(chn_left, CHN_TYPE_IN, blk_left);
	}

	vb_dqbuf(chn_right, &gstDpuJobs[workingGrp].ins[1], &blk_right);
	if (blk_right == VB_INVALID_HANDLE) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Mod(%d) can't get right vb-blk.\n", chn_right.enModId);
	} else {
		dpu_vb_done_handler(chn_right, CHN_TYPE_IN, blk_right);
	}

	DpuChn = 0;
	do {
		if (!(workingMask & BIT(DpuChn)))
			continue;

		workingMask &= ~BIT(DpuChn);

		if (!dpuCtx[workingGrp]->stChnCfgs[DpuChn].isEnabled)
			continue;

		chn_out[DpuChn].s32ChnId = DpuChn;

		vb_dqbuf(chn_out[DpuChn], &gstDpuJobs[workingGrp].outs[DpuChn], &blk_out[DpuChn]);
		if (blk_out[DpuChn] == VB_INVALID_HANDLE) {
			CVI_TRACE_DPU(CVI_DBG_ERR, "Mod(%d) can't get out vb-blk.\n"
				     , chn_out[DpuChn].enModId);
			continue;
		}else {
			dpu_vb_done_handler(chn_out[DpuChn], CHN_TYPE_OUT, blk_out[DpuChn]);
		}

		CVI_TRACE_DPU(CVI_DBG_INFO, "grp(%d) chn(%d) end\n", workingGrp, DpuChn);

	} while (++DpuChn < dpuCtx[workingGrp]->chnNum);

	ctx->workingMask = workingMask;

	dpuCtx[workingGrp]->stGrpWorkStatus.SendPicCnt ++ ;

	ktime_get_ts64(&time);
	duration = get_diff_in_us(ctx->time, time);
	dpuCtx[workingGrp]->costTimeForSec += duration;
	if(dpuCtx[workingGrp]->costTimeForSec <= (1000*1000)){
		dpuCtx[workingGrp]->frameNum ++;
	} else {
		dpuCtx[workingGrp]->stGrpWorkStatus.FrameRate = \
		dpuCtx[workingGrp]->frameNum;
		dpuCtx[workingGrp]->costTimeForSec = 0;
		dpuCtx[workingGrp]->frameNum = 0;
	}

	dpuCtx[workingGrp]->stGrpWorkStatus.CurTaskCostTm = duration;
	if(dpuCtx[workingGrp]->stGrpWorkStatus.CurTaskCostTm \
		> dpuCtx[workingGrp]->stGrpWorkStatus.MaxTaskCostTm){
			dpuCtx[workingGrp]->stGrpWorkStatus.MaxTaskCostTm \
			= dpuCtx[workingGrp]->stGrpWorkStatus.CurTaskCostTm;
	}
	duration_s = duration;

	do_div(duration_s,(1000*1000));
	dpu_dev->stRunTimeInfo.RunTm = duration_s;
	ctx->events = 0;
	// if(dpuCtx[workingGrp]->phyAddr_chfh!=0){
	// 	base_ion_free(dpuCtx[workingGrp]->phyAddr_chfh);
	// 	dpuCtx[workingGrp]->phyAddr_chfh=0;
	// 	CVI_TRACE_DPU(CVI_DBG_INFO, "phyAddr_chfh(0x%llx)", dpuCtx[workingGrp]->phyAddr_chfh);
	// }
	CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_handle_frame_done          -\n");
}

static void dpu_handle_offline(struct dpu_handler_ctx *ctx)
{

	struct timespec64 time;
	u32 state;
	u64 duration64;
	DPU_GRP workingGrp = ctx->workingGrp;
	u8 workingMask = ctx->workingMask;
	u32 events = ctx->events;
	CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_handle_offline           +\n");
	CVI_TRACE_DPU(CVI_DBG_INFO, "workingMask(%d) ,workingGrp(%d) ,u8DpuDev(%d)\n",
			workingMask,workingGrp,ctx->u8DpuDev);
	if (workingMask == 0 && workingGrp == DPU_MAX_GRP_NUM) {
		// find grp has job todo.
		workingGrp = find_next_en_grp(workingGrp, ctx->u8DpuDev);
		ctx->workingGrp = workingGrp;

		if (workingGrp >= DPU_MAX_GRP_NUM) {
			CVI_TRACE_DPU(CVI_DBG_WARN, "Grp(%d) invalid\n", workingGrp);
			return;
		}
	}
	mutex_lock(&dpuCtx[workingGrp]->lock);

	// Sanity check

	if (!dpuCtx[workingGrp]) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) isn't created yet.\n", workingGrp);
		return;
	}
	if (!dpuCtx[workingGrp]->isCreated || !dpuCtx[workingGrp]->isStarted) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "Grp(%d) invalid, isCreated=%d, isStarted=%d\n",
				workingGrp, dpuCtx[workingGrp]->isCreated, dpuCtx[workingGrp]->isStarted);
		return;
	}

	CVI_TRACE_DPU(CVI_DBG_INFO, "ctx[%d] event=0x%x, Grp(%d) , mask=0x%x\n",
			ctx->u8DpuDev, events, workingGrp,workingMask);
	state=dpuCtx[workingGrp]->grp_state;
	if (dpuCtx[workingGrp]->grp_state == GRP_STATE_IDLE) {
		dpu_try_schedule_offline(ctx);
	} else if (dpuCtx[workingGrp]->grp_state == GRP_STATE_HW_STARTED) {
		ktime_get_ts64(&time);
		duration64 = get_diff_in_us(ctx->time, time);
		do_div(duration64, 1000);

		if (ctx->events & CTX_EVENT_EOF) {
			dpu_handle_frame_done(ctx);
		} else {
			if (duration64 > (hw_wait_time-2)) {
				/* timeout */
				CVI_TRACE_DPU(CVI_DBG_INFO, "ctx[%d] event timeout on grp(%d)\n",
						ctx->u8DpuDev, workingGrp);
				//sclr_check_register();
				ctx->events = 0;
				ctx->workingMask =0;
				dpuCtx[workingGrp]->grp_state = GRP_STATE_IDLE;
				hw_reset(workingGrp);

				// if(dpuCtx[workingGrp]->phyAddr_chfh!=0){
				// 	base_ion_free(dpuCtx[workingGrp]->phyAddr_chfh);
				// 	dpuCtx[workingGrp]->phyAddr_chfh=0;
				// }
				// CVI_TRACE_DPU(CVI_DBG_INFO, "phyAddr_chfh(0x%llx)", dpuCtx[workingGrp]->phyAddr_chfh);

			} else {
				// keep waiting
				CVI_TRACE_DPU(CVI_DBG_INFO, "ctx[%d] event no timeout on grp, but not done!(%d)\n",
						ctx->u8DpuDev, workingGrp);
			}
			dpuCtx[workingGrp]->stGrpWorkStatus.StartFailCnt++;

		}
	} else {
		CVI_TRACE_DPU(CVI_DBG_INFO, "ctx[%d] grp(%d) unexpected state=%d, events=0x%x\n",
				ctx->u8DpuDev, workingGrp, dpuCtx[workingGrp]->grp_state,
				ctx->events);
	}
	mutex_unlock(&dpuCtx[workingGrp]->lock);
	CVI_TRACE_DPU(CVI_DBG_INFO, "ctx[%d] evt=0x%x -> %x, mask=0x%x, Grp(%d) state 0x%x->0x%x\n",
			ctx->u8DpuDev, events, ctx->events,ctx->workingMask, workingGrp, state,
			dpuCtx[workingGrp]->grp_state);
	CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_handle_offline           -\n");
}


static int dpu_event_handler(void *arg)
{
	struct timespec64 time;
	u32 hwDuration;
	struct cvi_dpu_dev *dpu_dev = (struct cvi_dpu_dev *)arg;
	unsigned long idle_timeout = msecs_to_jiffies(IDLE_TIMEOUT_MS);
	unsigned long eof_timeout = msecs_to_jiffies(EOF_WAIT_TIMEOUT_MS);
	unsigned long hw_timeout = msecs_to_jiffies(hw_wait_time);
	int i, ret;
	timeout = idle_timeout;
	//CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_event_handler           +\n");
	while (!kthread_should_stop()) {
		ret = wait_event_interruptible_timeout(dpu_dev->wait,
			handler_ctx[0].events ||kthread_should_stop(),timeout);
		/* -%ERESTARTSYS */
		if (ret < 0 || kthread_should_stop())
			break;

		/* timeout */
		if (!ret && dpu_handler_is_idle()){
			timeout = idle_timeout;
			continue;
		}

		if(handler_ctx[0].events == CTX_EVENT_EOF){
			ktime_get_ts64(&time);
			hwDuration = get_diff_in_us(dpu_dev->timeStart, time);
			dpu_dev->costTimeForSec += hwDuration;
			if(dpu_dev->costTimeForSec <=(1000*1000)){
				dpu_dev->IntTimePerSec += hwDuration;
				dpu_dev->IntNumPerSec += 1;
			} else {
				dpu_dev->stRunTimeInfo.CostTmPerSec = dpu_dev->IntTimePerSec;
				dpu_dev->IntTimePerSec = 0;

				if(dpu_dev->stRunTimeInfo.MCostTmPerSec \
				< dpu_dev->stRunTimeInfo.CostTmPerSec){
					dpu_dev->stRunTimeInfo.MCostTmPerSec = \
					dpu_dev->stRunTimeInfo.CostTmPerSec;
				}

				dpu_dev->stRunTimeInfo.CntPerSec = dpu_dev->IntNumPerSec;
				dpu_dev->IntNumPerSec = 0;
				if(dpu_dev->stRunTimeInfo.MaxCntPerSec \
				< dpu_dev->stRunTimeInfo.CntPerSec){
					dpu_dev->stRunTimeInfo.MaxCntPerSec = \
					dpu_dev->stRunTimeInfo.CntPerSec;
				}

				dpu_dev->costTimeForSec = 0;
			}

			dpu_dev->stRunTimeInfo.TotalIntCostTm += hwDuration;
			dpu_dev->stRunTimeInfo.TotalIntCnt ++;
			dpu_dev->stRunTimeInfo.CostTm = hwDuration;
			if(dpu_dev->stRunTimeInfo.CostTm > dpu_dev->stRunTimeInfo.MCostTm)
				dpu_dev->stRunTimeInfo.MCostTm = dpu_dev->stRunTimeInfo.CostTm;
		}

		dpu_dev->bBusy = CVI_TRUE;

		CVI_TRACE_DPU(CVI_DBG_INFO, "ctx[0] state=%d, events=0x%x, bBusy=%d\n",
				handler_ctx[0].enHdlState, handler_ctx[0].events,dpu_dev->bBusy );

		handler_ctx[0].events &= ~CTX_EVENT_WKUP;
		for (i = 0; i < DPU_IP_NUM; i++) {
			if (handler_ctx[i].enHdlState != HANDLER_STATE_RUN)
				continue;
			if(!(&handler_ctx[i])){
				CVI_TRACE_DPU(CVI_DBG_INFO,"handler_ctx[%d] null ptr\n",i);
				continue;
			}
			dpu_handle_offline(&handler_ctx[i]);
			// check if there are still unfinished jobs
			if (handler_ctx[i].workingMask ==0) {
				handler_ctx[i].workingGrp
					= find_next_en_grp(handler_ctx[i].workingGrp, i);

				// unfinished job found, need to re-trig event handler
				if (handler_ctx[i].workingGrp != DPU_MAX_GRP_NUM)
					handler_ctx[i].events = CTX_EVENT_WKUP;

				dpu_dev->bBusy =CVI_FALSE;

			}


		}
		CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_dev->bBusy(%d) \n",dpu_dev->bBusy);
		CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_handler_is_idle(%d) \n",dpu_handler_is_idle());
		/* Adjust timeout */
		timeout = dpu_handler_is_idle() ? idle_timeout : ((handler_ctx[0].workingMask == 1) ? hw_timeout:eof_timeout);
		if(!dpu_enable_handler_ctx(&handler_ctx[0])){
			handler_ctx[0].workingGrp = DPU_MAX_GRP_NUM;
			handler_ctx[0].workingMask = 0;
			handler_ctx[0].events = 0;
		}
		CVI_TRACE_DPU(CVI_DBG_INFO, "timeout(%ld) \n",timeout);
		CVI_TRACE_DPU(CVI_DBG_INFO, "idle_timeout(%ld) \n",idle_timeout);
		CVI_TRACE_DPU(CVI_DBG_INFO, "eof_timeout(%ld) \n",eof_timeout);
		CVI_TRACE_DPU(CVI_DBG_INFO, "hw_timeout(%ld) \n",hw_timeout);

	}
	CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_event_handler           -\n");
	return 0;
}


void dpu_start_handler(struct cvi_dpu_dev *dpu_dev)
{
	int ret;
	u8 u8DpuDev;
	struct sched_param tsk;
	//CVI_TRACE_DPU(CVI_DBG_INFO, "dpu_start_handler          +\n");
	memset(&dpu_dev->stRunTimeInfo,0,sizeof(dpu_dev->stRunTimeInfo));
	dpu_dev->stRunTimeInfo.CntPerSec = 0;
	dpu_dev->stRunTimeInfo.CostTm = 0;
	dpu_dev->stRunTimeInfo.CostTmPerFrm = 0;
	dpu_dev->stRunTimeInfo.CostTmPerSec = 0;
	dpu_dev->stRunTimeInfo.HwCostTmPerFrm = 0;
	dpu_dev->stRunTimeInfo.MaxCntPerSec = 0;
	dpu_dev->stRunTimeInfo.MCostTm = 0;
	dpu_dev->stRunTimeInfo.MCostTmPerSec = 0;
	dpu_dev->stRunTimeInfo.RunTm = 0;
	dpu_dev->stRunTimeInfo.TotalIntCnt = 0;
	dpu_dev->stRunTimeInfo.TotalIntCntLastSec = 0;
	dpu_dev->stRunTimeInfo.TotalIntCostTm = 0;

	dpu_dev->timeForSec = 0;
	dpu_dev->IntNumPerSec = 0;

	dpu_dev->bBusy =CVI_FALSE;

	for (u8DpuDev = 0; u8DpuDev < DPU_IP_NUM; u8DpuDev++) {
		handler_ctx[u8DpuDev].u8DpuDev = u8DpuDev;
		handler_ctx[u8DpuDev].enHdlState = HANDLER_STATE_STOP;
		handler_ctx[u8DpuDev].workingGrp = DPU_MAX_GRP_NUM;
		handler_ctx[u8DpuDev].workingMask = 0;
		handler_ctx[u8DpuDev].events = 0;
		mutex_init(&handler_ctx[u8DpuDev].mutex);

		//FIFO_INIT(&handler_ctx[u8DpuDev].rgnex_jobs.jobq, 16);
	}

	// Same as sched_set_fifo in linux 5.x
	tsk.sched_priority = MAX_USER_RT_PRIO - 10;

	dpu_dev->thread = kthread_run(dpu_event_handler, dpu_dev,
		"cvitask_dpu_hdl");
	if (IS_ERR(dpu_dev->thread)) {
		pr_err("failed to create dpu kthread, u8DpuDev=%d\n", u8DpuDev);
	}

	ret = sched_setscheduler(dpu_dev->thread, SCHED_FIFO, &tsk);
	if (ret)
		pr_warn("dpu thread priority update failed: %d\n", ret);
}

int dpu_get_handle_info(struct cvi_dpu_dev *dpu_wdev, struct file *file,
		struct cvi_dpu_handle_info **f_list)
{
	struct cvi_dpu_handle_info *h_info;

	mutex_lock(&dpu_wdev->dpuLock);

	list_for_each_entry(h_info, &dpu_wdev->handle_list, list) {
		if (h_info->file == file) {
			*f_list = h_info;
			mutex_unlock(&dpu_wdev->dpuLock);
			return 0;
		}
	}
	mutex_unlock(&dpu_wdev->dpuLock);
	return -EINVAL;
}

void dpu_mode_deinit(DPU_GRP DpuGrp){

	mutex_lock(&dpuGetGrpLock);
	dpuGrpUsed[DpuGrp] = CVI_FALSE;
	mutex_unlock(&dpuGetGrpLock);
}

void dpu_init(void *arg)
{
	int ret;
	u8 i;
	char chfh_ion_name[10] ="dpuChfh";
	if (!arg)
		return;

	dpu_dev = (struct cvi_dpu_dev *)arg;

	mutex_init(&dpu_dev->dpuLock);
	mutex_init(&dpuGetGrpLock);

	INIT_LIST_HEAD(&dpu_dev->handle_list);

	mutex_lock(&dpuGetGrpLock);
	for(i = 0; i < DPU_MAX_GRP_NUM; ++i){
		dpuGrpUsed[i] = CVI_FALSE;
	}
	mutex_unlock(&dpuGetGrpLock);

	// CVI_SYS_Init()
	if(dpu_dev->clk_sys[0])
		clk_prepare_enable(dpu_dev->clk_sys[0]);

	if(dpu_dev->clk_sys[1])
		clk_prepare_enable(dpu_dev->clk_sys[1]);
	init_waitqueue_head(&dpu_dev->wait);
	init_waitqueue_head(&dpu_dev->reset_wait);
	init_waitqueue_head(&dpu_dev->sendFrame_wait);
	dpu_dev->reset_done = CVI_FALSE;
	ret = base_ion_alloc(&dpu_dev->phyAddr_chfh, &dpu_dev->virAddr_chfh,
			 (uint8_t *)chfh_ion_name, 1920*1080*5, CVI_FALSE);
	if (ret) {
		CVI_TRACE_DPU(CVI_DBG_ERR,"base_ion_alloc fail! ret(%d)\n", ret);
		return;
	}
	CVI_TRACE_DPU(CVI_DBG_INFO, "phyAddr_chfh(0x%llx)", dpu_dev->phyAddr_chfh);
	dpu_start_handler(dpu_dev);
}

void dpu_deinit(void *arg)
{
	int ret;
	if (!arg)
		return;

	dpu_dev = (struct cvi_dpu_dev *)arg;
	base_ion_free(dpu_dev->phyAddr_chfh);
	CVI_TRACE_DPU(CVI_DBG_INFO, "phyAddr_chfh(0x%llx)", dpu_dev->phyAddr_chfh);

	if(dpu_dev->clk_sys[0])
		clk_disable_unprepare(dpu_dev->clk_sys[0]);

	if(dpu_dev->clk_sys[1])
		clk_disable_unprepare(dpu_dev->clk_sys[1]);
	if (!dpu_dev->thread) {
		pr_err("dpu thread not initialized yet\n");
		return ;
	}

	mutex_destroy(&dpu_dev->dpuLock);
	mutex_destroy(&dpuGetGrpLock);

	ret = kthread_stop(dpu_dev->thread);
	if (ret)
		pr_err("fail to stop dpu thread, err=%d\n", ret);

	dpu_dev->thread = NULL;
}

void dpu_check_reg_read(void)
{
    u32 reg_sys;
	u32 reg_00 ;
	u32 reg_04;
	u32 reg_08;
	u32 reg_0C;
	u32 reg_10;
	u32 reg_14;
	u32 reg_1C;
	u32 reg_20;
	u32 reg_24;
	u32 reg_28;
	u32 reg_2C;
	u32 reg_30;
	u32 reg_34;
	u32 reg_38;
	u32 reg_3C;
	u32 reg_40;
	u32 reg_44;
	u32 reg_48;
	u32 reg_4C;
	u32 reg_50;
	u32 reg_54;
	u32 reg_58;
	u32 reg_5C;
	u32 reg_60;
	u32 reg_64;
	u32 reg_68;
	u32 reg_6C;
	u32 reg_70;
	u32 reg_74;
	u32 reg_78;
	u32 reg_7C;
	u32 reg_80;
	u32 reg_84;
	u32 reg_88;
	u32 reg_90;
	u32 reg_94;
	u32 reg_98;
	u32 reg_9C;
	u32 reg_A0;
	u32 reg_dma_sgbm_ld1_base_addr;
	u32 reg_dma_sgbm_ld1_seglen;
	u32 reg_dma_sgbm_ld1_stride;
	u32 reg_dma_sgbm_ld1_segnum;
	u32 reg_dma_sgbm_ld2_base_addr;
	u32 reg_dma_sgbm_ld2_seglen;
	u32 reg_dma_sgbm_ld2_stride;
	u32 reg_dma_sgbm_ld2_segnum;
	u32 reg_dma_sgbm_bf_base_addr;
	u32 reg_dma_sgbm_bf_seglen;
	u32 reg_dma_sgbm_bf_stride;
	u32 reg_dma_sgbm_bf_segnum;
	u32 reg_dma_sgbm_mux_base_addr;
	u32 reg_dma_sgbm_mux_seglen;
	u32 reg_dma_sgbm_mux_stride;
	u32 reg_dma_sgbm_mux_segnum;

	u32 reg_dma_fgs_chfh_ld_base_addr;
	u32 reg_dma_fgs_chfh_ld_seglen;
	u32 reg_dma_fgs_chfh_ld_stride;
	u32 reg_dma_fgs_chfh_ld_segnum;
	u32 reg_dma_fgs_chfh_st_base_addr;
	u32 reg_dma_fgs_chfh_st_seglen;
	u32 reg_dma_fgs_chfh_st_stride;
	u32 reg_dma_fgs_chfh_st_segnum;
	u32 reg_dma_fgs_gx_ld_base_addr;
	u32 reg_dma_fgs_gx_ld_seglen;
	u32 reg_dma_fgs_gx_ld_stride;
	u32 reg_dma_fgs_gx_ld_segnum;
	u32 reg_dma_fgs_ux_st_base_addr;
	u32 reg_dma_fgs_ux_st_seglen;
	u32 reg_dma_fgs_ux_st_stride;
	u32 reg_dma_fgs_ux_st_segnum;
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print] Print dpu Reg configurations ...\n");
    reg_00 = read_reg(reg_base + DPU_REG_00_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_00]:  %d\n", reg_00);
    reg_04 = read_reg(reg_base + DPU_REG_04_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_04]:  %d\n", reg_04);
    reg_08 = read_reg(reg_base + DPU_REG_08_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_08]:  %d\n", reg_08);
    reg_0C = read_reg(reg_base + DPU_REG_0C_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_0C]:  %d\n", reg_0C);

    reg_10 = read_reg(reg_base + DPU_REG_10_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_10]:  %d\n", reg_10);
    reg_14 = read_reg(reg_base + DPU_REG_14_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_14]:  %d\n", reg_14);
    reg_1C = read_reg(reg_base + DPU_REG_1C_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_1C]:  %d\n", reg_1C);

    reg_20 = read_reg(reg_base + DPU_REG_20_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_20]:  %d\n", reg_20);
    reg_24 = read_reg(reg_base + DPU_REG_24_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_24]:  %d\n", reg_24);
    reg_28 = read_reg(reg_base + DPU_REG_28_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_28]:  %d\n", reg_28);
    reg_2C = read_reg(reg_base + DPU_REG_2C_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_2C]:  %d\n", reg_2C);

    reg_30 = read_reg(reg_base + DPU_REG_30_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_30]:  %d\n", reg_30);
    reg_34 = read_reg(reg_base + DPU_REG_34_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_34]:  %d\n", reg_34);
    reg_38 = read_reg(reg_base + DPU_REG_38_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_38]:  %d\n", reg_38);
    reg_3C = read_reg(reg_base + DPU_REG_3C_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_3C]:  %d\n", reg_3C);

    reg_40 = read_reg(reg_base + DPU_REG_40_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_40]:  %d\n", reg_40);
    reg_44 = read_reg(reg_base + DPU_REG_44_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_44]:  %d\n", reg_44);
    reg_48 = read_reg(reg_base + DPU_REG_48_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_48]:  %d\n", reg_48);
    reg_4C = read_reg(reg_base + DPU_REG_4C_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_4C]:  %d\n", reg_4C);

    reg_50 = read_reg(reg_base + DPU_REG_50_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_50]:  %d\n", reg_50);
    reg_54 = read_reg(reg_base + DPU_REG_54_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_54]:  %d\n", reg_54);
    reg_58 = read_reg(reg_base + DPU_REG_58_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_58]:  %d\n", reg_58);
    reg_5C = read_reg(reg_base + DPU_REG_5C_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_5C]:  %d\n", reg_5C);

    reg_60 = read_reg(reg_base + DPU_REG_60_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_60]:  %d\n", reg_60);
    reg_64 = read_reg(reg_base + DPU_REG_64_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_64]:  %d\n", reg_64);
    reg_68 = read_reg(reg_base + DPU_REG_68_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_64]:  %d\n", reg_64);
    reg_6C = read_reg(reg_base + DPU_REG_6C_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_6C]:  %d\n", reg_6C);

    reg_70 = read_reg(reg_base + DPU_REG_70_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_70]:  %d\n", reg_70);
    reg_74 = read_reg(reg_base + DPU_REG_74_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_74]:  %d\n", reg_74);
    reg_78 = read_reg(reg_base + DPU_REG_78_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_78]:  %d\n", reg_78);
    reg_7C = read_reg(reg_base + DPU_REG_7C_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_7C]:  %d\n", reg_7C);

    reg_80 = read_reg(reg_base + DPU_REG_80_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_80]:  %d\n", reg_80);
    reg_84 = read_reg(reg_base + DPU_REG_84_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_84]:  %d\n", reg_84);
    reg_88 = read_reg(reg_base + DPU_REG_88_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_88]:  %d\n", reg_88);

    reg_sys = read_reg(reg_base_sgbm_ld1_dma + SYS_CONTROL_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_LD1][base_h]: 0x%x\n", get_mask(reg_sys,8,8));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_LD1][base_sel]: 0x%x\n", get_mask(reg_sys,1,16));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_LD1][stride_sel]: 0x%x\n", get_mask(reg_sys,1,17));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_LD1][seglen_sel]: 0x%x\n", get_mask(reg_sys,1,18));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_LD1][segnum_sel]: 0x%x\n", get_mask(reg_sys,1,19));
    reg_dma_sgbm_ld1_base_addr = read_reg(reg_base_sgbm_ld1_dma + DMA_BASE_ADDR_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_LD1][base_addr]: 0x%x\n", reg_dma_sgbm_ld1_base_addr);
    reg_dma_sgbm_ld1_seglen = read_reg(reg_base_sgbm_ld1_dma + DMA_SEGLEN_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_LD1][seg_len]:  %d\n", reg_dma_sgbm_ld1_seglen);
    reg_dma_sgbm_ld1_stride = read_reg(reg_base_sgbm_ld1_dma + DMA_STRIDE_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_LD1][seg_stride]:  %d\n", reg_dma_sgbm_ld1_stride);
    reg_dma_sgbm_ld1_segnum = read_reg(reg_base_sgbm_ld1_dma + DMA_SEGNUM_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_LD1][seg_num]:  %d\n", reg_dma_sgbm_ld1_segnum);

    reg_sys = read_reg(reg_base_sgbm_ld2_dma + SYS_CONTROL_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_LD2][base_h]: 0x%x\n", get_mask(reg_sys,8,8));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_LD2][base_sel]: 0x%x\n", get_mask(reg_sys,1,16));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_LD2][stride_sel]: 0x%x\n", get_mask(reg_sys,1,17));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_LD2][seglen_sel]: 0x%x\n", get_mask(reg_sys,1,18));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_LD2][segnum_sel]: 0x%x\n", get_mask(reg_sys,1,19));
    reg_dma_sgbm_ld2_base_addr = read_reg(reg_base_sgbm_ld2_dma + DMA_BASE_ADDR_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_LD2][base_addr]: 0x%x\n", reg_dma_sgbm_ld2_base_addr);
    reg_dma_sgbm_ld2_seglen = read_reg(reg_base_sgbm_ld2_dma + DMA_SEGLEN_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_LD2][seg_len]:  %d\n", reg_dma_sgbm_ld2_seglen);
    reg_dma_sgbm_ld2_stride = read_reg(reg_base_sgbm_ld2_dma + DMA_STRIDE_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_LD2][seg_stride]:  %d\n", reg_dma_sgbm_ld2_stride);
    reg_dma_sgbm_ld2_segnum = read_reg(reg_base_sgbm_ld2_dma + DMA_SEGNUM_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_LD2][seg_num]:  %d\n", reg_dma_sgbm_ld2_segnum);

    reg_sys = read_reg(reg_base_sgbm_bf_dma + SYS_CONTROL_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_BF][base_h]: 0x%x\n", get_mask(reg_sys,8,8));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_BF][base_sel]: 0x%x\n", get_mask(reg_sys,1,16));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_BF][stride_sel]: 0x%x\n", get_mask(reg_sys,1,17));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_BF][seglen_sel]: 0x%x\n", get_mask(reg_sys,1,18));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_BF][segnum_sel]: 0x%x\n", get_mask(reg_sys,1,19));
    reg_dma_sgbm_bf_base_addr = read_reg(reg_base_sgbm_bf_dma + DMA_BASE_ADDR_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_BF][base_addr]: 0x%x\n", reg_dma_sgbm_bf_base_addr);
    reg_dma_sgbm_bf_seglen = read_reg(reg_base_sgbm_bf_dma + DMA_SEGLEN_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_BF][seg_len]:  %d\n", reg_dma_sgbm_bf_seglen);
    reg_dma_sgbm_bf_stride = read_reg(reg_base_sgbm_bf_dma + DMA_STRIDE_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_BF][seg_stride]:  %d\n", reg_dma_sgbm_bf_stride);
    reg_dma_sgbm_bf_segnum = read_reg(reg_base_sgbm_bf_dma + DMA_SEGNUM_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_BF][seg_num]:  %d\n", reg_dma_sgbm_bf_segnum);

    reg_sys = read_reg(reg_base_sgbm_median_dma + SYS_CONTROL_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_MUX][base_h]: 0x%x\n", get_mask(reg_sys,8,8));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_MUX][base_sel]: 0x%x\n", get_mask(reg_sys,1,16));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_MUX][stride_sel]: 0x%x\n", get_mask(reg_sys,1,17));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_MUX][seglen_sel]: 0x%x\n", get_mask(reg_sys,1,18));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_MUX][segnum_sel]: 0x%x\n", get_mask(reg_sys,1,19));
    reg_dma_sgbm_mux_base_addr = read_reg(reg_base_sgbm_median_dma + DMA_BASE_ADDR_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_MUX][base_addr]: 0x%x\n", reg_dma_sgbm_mux_base_addr);
    reg_dma_sgbm_mux_seglen = read_reg(reg_base_sgbm_median_dma + DMA_SEGLEN_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_MUX][seg_len]:  %d\n", reg_dma_sgbm_mux_seglen);
    reg_dma_sgbm_mux_stride = read_reg(reg_base_sgbm_median_dma + DMA_STRIDE_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_MUX][seg_stride]:  %d\n", reg_dma_sgbm_mux_stride);
    reg_dma_sgbm_mux_segnum = read_reg(reg_base_sgbm_median_dma + DMA_SEGNUM_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][SGBM_MUX][seg_num]:  %d\n", reg_dma_sgbm_mux_segnum);

    reg_sys = read_reg(reg_base_fgs_chfh_ld_dma + SYS_CONTROL_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_CHFH_LD][base_h]: 0x%x\n", get_mask(reg_sys,8,8));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_CHFH_LD][base_sel]: 0x%x\n", get_mask(reg_sys,1,16));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_CHFH_LD][stride_sel]: 0x%x\n", get_mask(reg_sys,1,17));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_CHFH_LD][seglen_sel]: 0x%x\n", get_mask(reg_sys,1,18));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_CHFH_LD][segnum_sel]: 0x%x\n", get_mask(reg_sys,1,19));
    reg_dma_fgs_chfh_ld_base_addr = read_reg(reg_base_fgs_chfh_ld_dma + DMA_BASE_ADDR_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_CHFH_LD][base_addr]: 0x%x\n", reg_dma_fgs_chfh_ld_base_addr);
    reg_dma_fgs_chfh_ld_seglen = read_reg(reg_base_fgs_chfh_ld_dma + DMA_SEGLEN_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_CHFH_LD][seg_len]:  %d\n", reg_dma_fgs_chfh_ld_seglen);
    reg_dma_fgs_chfh_ld_stride = read_reg(reg_base_fgs_chfh_ld_dma + DMA_STRIDE_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_CHFH_LD][seg_stride]:  %d\n", reg_dma_fgs_chfh_ld_stride);
    reg_dma_fgs_chfh_ld_segnum = read_reg(reg_base_fgs_chfh_ld_dma + DMA_SEGNUM_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_CHFH_LD][seg_num]:  %d\n", reg_dma_fgs_chfh_ld_segnum);

    reg_sys = read_reg(reg_base_fgs_chfh_st_dma + SYS_CONTROL_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_CHFH_ST][base_h]: 0x%x\n", get_mask(reg_sys,8,8));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_CHFH_ST][base_sel]: 0x%x\n", get_mask(reg_sys,1,16));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_CHFH_ST][stride_sel]: 0x%x\n", get_mask(reg_sys,1,17));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_CHFH_ST][seglen_sel]: 0x%x\n", get_mask(reg_sys,1,18));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_CHFH_ST][segnum_sel]: 0x%x\n", get_mask(reg_sys,1,19));
    reg_dma_fgs_chfh_st_base_addr = read_reg(reg_base_fgs_chfh_st_dma + DMA_BASE_ADDR_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_CHFH_ST][base_addr]: 0x%x\n", reg_dma_fgs_chfh_st_base_addr);
    reg_dma_fgs_chfh_st_seglen = read_reg(reg_base_fgs_chfh_st_dma + DMA_SEGLEN_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_CHFH_ST][seg_len]:  %d\n", reg_dma_fgs_chfh_st_seglen);
    reg_dma_fgs_chfh_st_stride = read_reg(reg_base_fgs_chfh_st_dma + DMA_STRIDE_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_CHFH_ST][seg_stride]:  %d\n", reg_dma_fgs_chfh_st_stride);
    reg_dma_fgs_chfh_st_segnum = read_reg(reg_base_fgs_chfh_st_dma + DMA_SEGNUM_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_CHFH_ST][seg_num]:  %d\n", reg_dma_fgs_chfh_st_segnum);

    reg_sys = read_reg(reg_base_fgs_gx_dma + SYS_CONTROL_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_GX_LD][base_h]: 0x%x\n", get_mask(reg_sys,8,8));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_GX_LD][base_sel]: 0x%x\n", get_mask(reg_sys,1,16));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_GX_LD][stride_sel]: 0x%x\n", get_mask(reg_sys,1,17));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_GX_LD][seglen_sel]: 0x%x\n", get_mask(reg_sys,1,18));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_GX_LD][segnum_sel]: 0x%x\n", get_mask(reg_sys,1,19));
    reg_dma_fgs_gx_ld_base_addr = read_reg(reg_base_fgs_gx_dma + DMA_BASE_ADDR_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_GX_LD][base_addr]: 0x%x\n", reg_dma_fgs_gx_ld_base_addr);
    reg_dma_fgs_gx_ld_seglen = read_reg(reg_base_fgs_gx_dma + DMA_SEGLEN_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_GX_LD][seg_len]:  %d\n", reg_dma_fgs_gx_ld_seglen);
    reg_dma_fgs_gx_ld_stride = read_reg(reg_base_fgs_gx_dma + DMA_STRIDE_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_GX_LD][seg_stride]:  %d\n", reg_dma_fgs_gx_ld_stride);
    reg_dma_fgs_gx_ld_segnum = read_reg(reg_base_fgs_gx_dma + DMA_SEGNUM_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_GX_LD][seg_num]:  %d\n", reg_dma_fgs_gx_ld_segnum);

    reg_sys = read_reg(	reg_base_fgs_ux_dma + SYS_CONTROL_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_UX_ST][base_h]: 0x%x\n", get_mask(reg_sys,8,8));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_UX_ST][base_sel]: 0x%x\n", get_mask(reg_sys,1,16));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_UX_ST][stride_sel]: 0x%x\n", get_mask(reg_sys,1,17));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_UX_ST][seglen_sel]: 0x%x\n", get_mask(reg_sys,1,18));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_UX_ST][segnum_sel]: 0x%x\n", get_mask(reg_sys,1,19));
    reg_dma_fgs_ux_st_base_addr = read_reg(reg_base_fgs_ux_dma + DMA_BASE_ADDR_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_UX_ST][base_addr]: 0x%x\n", reg_dma_fgs_ux_st_base_addr);
    reg_dma_fgs_ux_st_seglen = read_reg(reg_base_fgs_ux_dma + DMA_SEGLEN_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_UX_ST][seg_len]:  %d\n", reg_dma_fgs_ux_st_seglen);
    reg_dma_fgs_ux_st_stride = read_reg(reg_base_fgs_ux_dma + DMA_STRIDE_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_UX_ST][seg_stride]:  %d\n", reg_dma_fgs_ux_st_stride);
    reg_dma_fgs_ux_st_segnum = read_reg(reg_base_fgs_ux_dma + DMA_SEGNUM_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DMA][FGS_UX_ST][seg_num]:  %d\n", reg_dma_fgs_ux_st_segnum);

    //reg00
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_enable]:  %d\n", get_mask(reg_00, 1, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_debug_mode]:  %d\n", get_mask(reg_00, 4, 4));
    //reg04
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_img_width]:  %d\n", get_mask(reg_04, 16, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_img_height]:  %d\n", get_mask(reg_04, 16, 16));
    //reg08
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_min_d]:  %d\n", get_mask(reg_08, 8, 0));
    //reg0C
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_rshift1]:  %d\n", get_mask(reg_0C, 3, 8));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_rshift2]:  %d\n", get_mask(reg_0C, 3, 12));
    //reg10
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_ca_p1]:  %d\n", get_mask(reg_10, 16, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_ca_p2]:  %d\n", get_mask(reg_10, 16, 16));
    //reg14
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_uniq_ratio]:  %d\n", get_mask(reg_14, 8, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_disp_shift]:  %d\n", get_mask(reg_14, 4, 8));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_bfw_size]:  %d\n", get_mask(reg_14, 2, 16));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_census_shift]:  %d\n", get_mask(reg_14, 8, 18));
    //reg1C
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_fgs_max_count]:  %d\n", get_mask(reg_1C, 5, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_fgs_w_scale]:  %d\n", get_mask(reg_1C, 4, 8));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_fgs_c_scale]:  %d\n", get_mask(reg_1C, 4, 12));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_fgs_f_scale]:  %d\n", get_mask(reg_1C, 4, 16));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_bf_dma_en]:  %d\n", get_mask(reg_1C, 1, 20));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_ld1_dma_en]:  %d\n", get_mask(reg_1C, 1, 21));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_ld2_dma_en]:  %d\n", get_mask(reg_1C, 1, 22));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_mux_dma_en]:  %d\n", get_mask(reg_1C, 1, 23));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs1_dma_en]:  %d\n", get_mask(reg_1C, 1, 24));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs2_dma_en]:  %d\n", get_mask(reg_1C, 1, 25));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs3_dma_en]:  %d\n", get_mask(reg_1C, 1, 26));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs4_dma_en]:  %d\n", get_mask(reg_1C, 1, 27));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_bf_crop_en]:  %d\n", get_mask(reg_1C, 1, 29));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_ld1_crop_en]:  %d\n", get_mask(reg_1C, 1, 30));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_ld2_crop_en]:  %d\n", get_mask(reg_1C, 1, 31));
    //reg20
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_bf_crop_h_en]:  %d\n", get_mask(reg_20, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_bf_crop_h_str]:  %d\n", get_mask(reg_20, 14, 16));
    //reg24
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_bf_crop_w_en]:  %d\n", get_mask(reg_24, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_bf_crop_w_str]:  %d\n", get_mask(reg_24, 14, 16));
    //reg28
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_bf_crop_height]:  %d\n", get_mask(reg_28, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_bf_crop_width]:  %d\n", get_mask(reg_28, 14, 16));
    //reg2C
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_ld_crop_h_en]:  %d\n", get_mask(reg_2C, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_ld_crop_h_str]:  %d\n", get_mask(reg_2C, 14, 16));
    //reg30
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_ld_crop_w_en]:  %d\n", get_mask(reg_30, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_ld_crop_w_str]:  %d\n", get_mask(reg_30, 14, 16));
    //reg34
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_ld_crop_height]:  %d\n", get_mask(reg_34, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_ld_crop_width]:  %d\n", get_mask(reg_34, 14, 16));
    //reg38
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_mux_crop_h_en]:  %d\n", get_mask(reg_38, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_mux_crop_h_str]:  %d\n", get_mask(reg_38, 14, 16));
    //reg3C
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_mux_crop_w_en]:  %d\n", get_mask(reg_3C, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_mux_crop_w_str]:  %d\n", get_mask(reg_3C, 14, 16));
    //reg40
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_mux_crop_height]:  %d\n", get_mask(reg_40, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_mux_crop_width]:  %d\n", get_mask(reg_40, 14, 16));
    //reg44
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_chfh_crop_h_en]:  %d\n", get_mask(reg_44, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_chfh_crop_h_str]:  %d\n", get_mask(reg_44, 14, 16));
    //reg48
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_chfh_crop_w_en]:  %d\n", get_mask(reg_48, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_chfh_crop_w_str]:  %d\n", get_mask(reg_48, 14, 16));
    //reg4C
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_chfh_crop_height]:  %d\n", get_mask(reg_4C, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_chfh_crop_width]:  %d\n", get_mask(reg_4C, 14, 16));
    //reg50
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_gx_crop_h_en]:  %d\n", get_mask(reg_50, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_gx_crop_h_str]:  %d\n", get_mask(reg_50, 14, 16));
    //reg54
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_gx_crop_w_en]:  %d\n", get_mask(reg_54, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_gx_crop_w_str]:  %d\n", get_mask(reg_54, 14, 16));
    //reg58
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_gx_crop_height]:  %d\n", get_mask(reg_58, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_gx_crop_width]:  %d\n", get_mask(reg_58, 14, 16));
    //reg5C
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_ux_crop_h_en]:  %d\n", get_mask(reg_5C, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_ux_crop_h_str]:  %d\n", get_mask(reg_5C, 14, 16));
    //reg60
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_ux_crop_w_en]:  %d\n", get_mask(reg_60, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_ux_crop_w_str]:  %d\n", get_mask(reg_60, 14, 16));
    //reg64
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_ux_crop_height]:  %d\n", get_mask(reg_64, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_ux_crop_width]:  %d\n", get_mask(reg_64, 14, 16));
    //reg68
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_sgbm_frame_done]:  %d\n", get_mask(reg_68, 1, 1));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_mux_crop_en]:  %d\n", get_mask(reg_68, 1, 2));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_chfh_crop_en]:  %d\n", get_mask(reg_68, 1, 3));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_gx_crop_en]:  %d\n", get_mask(reg_68, 1, 4));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_ux_crop_en]:  %d\n", get_mask(reg_68, 1, 5));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_reset_done]:  %d\n", get_mask(reg_68, 2, 7));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_sgbm_enable]:  %d\n", get_mask(reg_68, 1, 12));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_fgs_enable]:  %d\n", get_mask(reg_68, 1, 13));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_sgbm2fgs_online]:  %d\n", get_mask(reg_68, 1, 16));
	//reg68
	CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_interrupt_sel]:  %d\n", get_mask(reg_6C, 8, 24));
    //reg70
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_img_width_fgs]:  %d\n", get_mask(reg_70, 16, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_img_height_fgs]:  %d\n", get_mask(reg_70, 16, 16));
    //reg74
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_nd_ds_fgs]:  %d\n", get_mask(reg_74, 8, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_fxbaseline_fgs]:  %d\n", get_mask(reg_74, 20, 8));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_output_bit_choose]:  %d\n", get_mask(reg_74, 1, 28));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_disp_range]:  %d\n", get_mask(reg_74, 3, 29));
    //reg78
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_dcc_a234]:  %d\n", get_mask(reg_78, 3, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_invalid_def]:  %d\n", get_mask(reg_78, 8, 8));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_src_disp_mux]:  %d\n", get_mask(reg_78, 2, 16));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_fgs_max_t]:  %d\n", get_mask(reg_78, 7, 18));
    //reg7C
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_independent_crop_h_en]:  %d\n", get_mask(reg_7C, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_independent_crop_h_str]:  %d\n", get_mask(reg_7C, 14, 16));
    //reg80
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_independent_crop_w_en]:  %d\n", get_mask(reg_80, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_independent_crop_w_str]:  %d\n", get_mask(reg_80, 14, 16));
    //reg84
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_independent_crop_height]:  %d\n", get_mask(reg_84, 14, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_independent_crop_width]:  %d\n", get_mask(reg_84, 14, 16));
    //reg88
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_independent_crop_en]:  %d\n", get_mask(reg_88, 1, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_data_sel]:  %d\n", get_mask(reg_88, 2, 1));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_sgbm_bf_dma_en]:  %d\n", get_mask(reg_88, 1, 3));
    //reg90
    reg_90 = read_reg(reg_base + DPU_REG_90_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_interrupt_mask_sgbm]:  %d\n", reg_90);
    //reg94
    reg_94 = read_reg(reg_base + DPU_REG_94_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_interrupt_mask_fgs]:  %d\n", reg_94);
    //reg98
    reg_98 = read_reg(reg_base + DPU_REG_98_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_shadow_int_mask_sgbm]:  %d\n", get_mask(reg_98, 1, 0));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_shadow_int_mask_fgs]:  %d\n", get_mask(reg_98, 1, 1));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_shadow_done_sgbm]:  %d\n", get_mask(reg_98, 1, 4));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_shadow_done_fgs]:  %d\n", get_mask(reg_98, 1, 5));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_ux_store_base_h]:  %d\n", get_mask(reg_98, 4, 8));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_chfh_store_base_h]:  %d\n", get_mask(reg_98, 4, 12));
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_dpu_output_unit_choose]:  %d\n", get_mask(reg_98, 2, 16));
    //reg9C
    reg_9C = read_reg(reg_base + DPU_REG_9C_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_ux_store_base_l]:  %x\n", reg_9C);
    //regA0
    reg_A0 = read_reg(reg_base + DPU_REG_A0_OFS);
    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print][DPU][reg_fgs_chfh_store_base_l]:  %x\n", reg_A0);

    CVI_TRACE_DPU(CVI_DBG_INFO,"[Print]Print dpu Reg Done ...\n");
}

void dpu_write_reg_init(void)
{
    cvi_dpu_reg.reg_dpu_enable = 1;
    cvi_dpu_reg.reg_dpu_sgbm_enable = 1;
    cvi_dpu_reg.reg_dpu_img_width = 64;
    cvi_dpu_reg.reg_dpu_img_height = 64;
    cvi_dpu_reg.reg_dpu_min_d = 1;
    cvi_dpu_reg.reg_dpu_rshift1 = 1;
    cvi_dpu_reg.reg_dpu_rshift2 = 1;
    cvi_dpu_reg.reg_dpu_ca_p1 = 1;
    cvi_dpu_reg.reg_dpu_ca_p2 = 1;
    cvi_dpu_reg.reg_dpu_uniq_ratio = 1;
    cvi_dpu_reg.reg_dpu_disp_shift = 1;
    cvi_dpu_reg.reg_dpu_bfw_size = 1;
    cvi_dpu_reg.reg_dpu_census_shift = 1;

    cvi_dpu_reg.reg_dpu_nd_ds = 1;
    cvi_dpu_reg.reg_dpu_fxbaseline = 1;
    cvi_dpu_reg.reg_dpu_disp_range = 1;
    cvi_dpu_reg.reg_dpu_dcc_a234 =1;
    cvi_dpu_reg.reg_dpu_invalid_def =1;
    cvi_dpu_reg.reg_dpu_src_disp_mux =1;
    cvi_dpu_reg.reg_dpu_data_sel=1;
    //cvi_dpu_reg.reg_dpu_fgs_enable=1;

    cvi_dpu_reg.reg_dpu_fgs_enable = 1;
    cvi_dpu_reg.reg_dpu_fgs_max_count = 1;
    cvi_dpu_reg.reg_dpu_fgs_max_t = 1;
    cvi_dpu_reg.reg_dpu_fgs_img_width =64;
    cvi_dpu_reg.reg_dpu_fgs_img_height =64;
    cvi_dpu_reg.reg_dpu_fgs_output_bit_choose =1;
    cvi_dpu_reg.reg_dpu_fgs_output_unit_choose =1;

    cvi_dpu_reg.reg_sgbm_bf_st_dma_enable = 1;
    cvi_dpu_reg.reg_sgbm_ld1_dma_enable=1;
    cvi_dpu_reg.reg_sgbm_ld2_dma_enable=1;
    cvi_dpu_reg.reg_sgbm_mux_st_dma_enable = 1;
    cvi_dpu_reg.reg_dma_enable_fgs1 = 1;
    cvi_dpu_reg.reg_dma_enable_fgs2 = 1;
    cvi_dpu_reg.reg_dma_enable_fgs3 = 1;
    cvi_dpu_reg.reg_dma_enable_fgs4 = 1;
    cvi_dpu_reg.reg_sgbm_bf_st_crop_enable=1;
    cvi_dpu_reg.reg_sgbm_ld1_crop_enable=1;
    cvi_dpu_reg.reg_sgbm_ld2_crop_enable=1;
    cvi_dpu_reg.reg_sgbm_mux_st_crop_enable =1;

    cvi_dpu_reg.reg_crop_enable_fgs_independent=1;
    cvi_dpu_reg.reg_crop_enable_fgs_chfh=1;
    cvi_dpu_reg.reg_crop_enable_fgs_gx=1;
    cvi_dpu_reg.reg_crop_enable_fgs_ux=1;
    cvi_dpu_reg.reg_sgbm2fgs_online=1;
}

void dpu_check_reg_write(void)
{
	u32 seg_len;
	u32 seg_num;
	dpu_write_reg_init();
	dpu_write_sgbm_all_reg();
	dpu_write_fgs_all_reg();
	seg_len= cvi_dpu_reg.reg_dpu_img_width;
	seg_num= cvi_dpu_reg.reg_dpu_img_height;
	register_sgbm_ld1_ld(seg_len, seg_num,dram_base_left_h, \
						dram_base_left_l,reg_base_sgbm_ld1_dma, reg_base);
	register_sgbm_ld2_ld(seg_len, seg_num,dram_base_right_h, \
						dram_base_right_l,reg_base_sgbm_ld2_dma, reg_base);
	register_sgbm_median_st_ld(seg_len, seg_num,cvi_dpu_reg.reg_dpu_data_sel,\
								dram_base_out_h,dram_base_out_l,reg_base_sgbm_median_dma, reg_base);
	register_sgbm_bf_st_ld(seg_len, seg_num,dram_base_out_btcost_h, \
						dram_base_out_btcost_l,reg_base_sgbm_bf_dma, reg_base);

	seg_len= cvi_dpu_reg.reg_dpu_fgs_img_width;
	seg_num= cvi_dpu_reg.reg_dpu_fgs_img_height;
	register_fgs_gx_ld(seg_len, seg_num,dram_base_left_h, \
						dram_base_left_l,reg_base_fgs_gx_dma, reg_base);
	register_fgs_chfh_ld(seg_len, seg_num,dram_base_right_h, \
						dram_base_right_l,reg_base_fgs_chfh_ld_dma, reg_base);
	register_fgs_chfh_st(seg_len, seg_num, dram_base_chfh_h, \
						dram_base_chfh_l,reg_base_fgs_chfh_st_dma, reg_base);
	register_fgs_ux_st(seg_len, seg_num,cvi_dpu_reg.reg_dpu_fgs_output_bit_choose,\
						dram_base_out_h, dram_base_out_l,reg_base_fgs_ux_dma, reg_base);

	dpu_check_reg_read();
}
