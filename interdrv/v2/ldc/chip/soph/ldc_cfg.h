#ifndef _CVI_LDC_CFG_H_
#define _CVI_LDC_CFG_H_

#define LDC_ADDR_ALIGN 64
#define LDC_SIZE_ALIGN 64
#define LDC_BASE_ADDR_SHIFT 6
#define LDC_MAX_WIDTH 4608
#define LDC_MAX_HEIGHT 4608

#define YUV400		0
#define NV21		1
#define LDC_CMDQ_MAX_REG_CNT (100)

enum ldc_dst_mode {
	LDC_DST_FLAT = 0,
	LDC_DST_XY_FLIP,
	LDC_DST_ROT_90,
	LDC_DST_ROT_270,
	LDC_DST_MAX,
};

struct ldc_cfg {
	u8 pix_fmt;       // 0: Y only, 1: NV21
	bool map_bypass;

	u16 ras_width;      // output width
	u16 ras_height;     // output height
	u64 map_base;

	u64 src_y_base;
	u64 src_c_base;
	u16 src_width;      // src width, including padding
	u16 src_height;     // src height, including padding
	u16 src_xstart;
	u16 src_xend;
	u16 bgcolor;        // data outside start/end if used in opeartion

	u64 dst_y_base;
	u64 dst_c_base;
	u32 extend_haddr;//dram bit[35:33]
	enum ldc_dst_mode dst_mode;
	bool use_cmdq;
};

#endif // _CVI_LDC_CFG_H_
