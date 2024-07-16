#ifndef _LDC_CFG_H_
#define _LDC_CFG_H_

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
	unsigned char pix_fmt;       // 0: Y only, 1: NV21
	bool map_bypass;

	unsigned short ras_width;      // output width
	unsigned short ras_height;     // output height
	unsigned long long map_base;

	unsigned long long src_y_base;
	unsigned long long src_c_base;
	unsigned short src_width;      // src width, including padding
	unsigned short src_height;     // src height, including padding
	unsigned short src_xstart;
	unsigned short src_xend;
	unsigned short bgcolor;        // data outside start/end if used in opeartion

	unsigned long long dst_y_base;
	unsigned long long dst_c_base;
	unsigned int extend_haddr;//dram bit[35:33]
	enum ldc_dst_mode dst_mode;
	bool use_cmdq;
};

#endif // _LDC_CFG_H_
