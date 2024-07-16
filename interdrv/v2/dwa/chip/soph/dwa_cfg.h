#ifndef _DWA_CFG_H_
#define _DWA_CFG_H_

#define DWA_INTR_NUM 15//16
#define DWA2_INTR_NUM 16
// coordinate in S COOR_N.COOR_M
#define DEWARP_COORD_MBITS 13
#define DEWARP_COORD_NBITS 18
#define NUMBER_Y_LINE_A_TILE 4
#define DIV_0_BIT_LSHIFT 0			// default 16
#define DIV_1_BIT_LSHIFT 18
#define INTERPOLATION_PRECISION_BITS 5
#define COEFFICIENT_PHASE_NUM (1 << INTERPOLATION_PRECISION_BITS)
#define INTERPOLATION_COEF_BITS 8		// SN
#define INTERPOLATION_COEF_FBITS 6

#define MESH_ID_FFS	0xfffa	// frame start
#define MESH_ID_FSS 0xfffb	// slice start
#define MESH_ID_FTS	0xfffc	// tile start
#define MESH_ID_FTE 0xfffd	// tile end
#define MESH_ID_FSE 0xfffe	// slice end
#define MESH_ID_FFE 0xffff	// frame end

#define CACHE_LINE_SIZE 16	// 16 bytes
#define CACHE_DATA_SIZE 128 // 128 bytes i.e., 16 bytes with 8 continue bursts,
#define CACHE_SET_NUM	16	// 16 sets
#define CACHE_WAY_NUM	4	// 4 ways
#define CACHE_X_ADDR	1	// 4 3 2 1
#define CACHE_Y_ADDR	3	// 0 1 2 3
#define CACHE_REPLACE_FIFO
//#define CACHE_REPLACE_LRU

#define YUV400		2
#define YUV420p		0
#define RGB888p		1

#define HTILE_MODE
#define DWA_CMDQ_MAX_REG_CNT (100)
struct dwa_buf {
	unsigned int addrl;
	unsigned int addrh;
	unsigned int pitch;
	unsigned short offset_x;
	unsigned short offset_y;
};

struct dwa_cfg {
	unsigned char pix_fmt;         // 0: YUV420, 1: RGB, 2: Y only
	unsigned char output_target;   // 0: to scaler, 1: to DRAM

	unsigned int bgcolor;        // R[0:7], G[15:8], B[23:16]
	unsigned int bdcolor;        // R[0:7], G[15:8], B[23:16]
	int src_width;
	int src_height;
	struct dwa_buf src_buf[3];
	int dst_width;
	int dst_height;
	struct dwa_buf dst_buf[3];
	unsigned long long mesh_id;
};

#endif // _DWA_CFG_H_
