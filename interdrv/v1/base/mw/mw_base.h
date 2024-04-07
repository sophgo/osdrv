/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: mw_base.h
 * Description: middleware layer base header file
 */

#ifndef _U_MW_CVI_BASE_H_
#define _U_MW_CVI_BASE_H_

#include <stdatomic.h>

#define NUM_OF_PLANES               (3)

#define BIND_DEST_MAXNUM            32
#define BIND_NODE_MAXNUM            64
#define VERSION_NAME_MAXLEN         128
#define MAX_VB_POOL_NAME_LEN        (32)

#define BASE_LOG_LEVEL_OFFSET       (0x10)
#define BASE_BIND_INFO_OFFSET       (BASE_LOG_LEVEL_OFFSET + LOG_LEVEL_RSV_SIZE)
#define BASE_VERSION_INFO_OFFSET    (BASE_BIND_INFO_OFFSET + BIND_INFO_RSV_SIZE)
#define BASE_VB_COMM_POOL_OFFSET    (BASE_VERSION_INFO_OFFSET + VERSION_INFO_RSV_SIZE)
#define BASE_VB_BLK_MOD_ID_OFFSET   (BASE_VB_COMM_POOL_OFFSET + VB_COMM_POOL_RSV_SIZE)

#define LOG_LEVEL_RSV_SIZE          (sizeof(int) * CVI_ID_BUTT)
#define BIND_INFO_RSV_SIZE          (sizeof(struct BIND_NODE_S) * BIND_NODE_MAXNUM)
#define VERSION_INFO_RSV_SIZE       (sizeof(struct MMF_VERSION_S))
#define VB_COMM_POOL_RSV_SIZE       (sizeof(struct VB_POOL_S) * vb_max_pools)
#define VB_BLK_MOD_ID_RSV_SIZE      (sizeof(uint64_t) * vb_pool_max_blk * vb_max_pools)
#define BASE_SHARE_MEM_SIZE         ALIGN(BASE_VB_BLK_MOD_ID_OFFSET + VB_BLK_MOD_ID_RSV_SIZE, 0x1000)

#ifdef __arm__
#define FIFO_HEAD(name, type)						\
	struct name {							\
		struct type *fifo;					\
		__u32 padding;						\
		int front, tail, capacity;				\
	} __aligned(8)
#else
#define FIFO_HEAD(name, type)						\
	struct name {							\
		struct type *fifo;					\
		int front, tail, capacity;				\
	}
#endif

#define FOREACH_MOD(MOD) {\
		MOD(BASE)	\
		MOD(VB)		\
		MOD(SYS)	\
		MOD(RGN)	\
		MOD(CHNL)	\
		MOD(VDEC)	\
		MOD(VPSS)	\
		MOD(VENC)	\
		MOD(H264E)	\
		MOD(JPEGE)	\
		MOD(MPEG4E) \
		MOD(H265E)	\
		MOD(JPEGD)	\
		MOD(VO)		\
		MOD(VI)		\
		MOD(DIS)	\
		MOD(RC)		\
		MOD(AIO)	\
		MOD(AI)		\
		MOD(AO)		\
		MOD(AENC)	\
		MOD(ADEC)	\
		MOD(AUD)   \
		MOD(VPU)	\
		MOD(ISP)	\
		MOD(IVE)	\
		MOD(USER)	\
		MOD(PROC)	\
		MOD(LOG)	\
		MOD(H264D)	\
		MOD(GDC)	\
		MOD(PHOTO)	\
		MOD(FB)		\
		MOD(BUTT)	\
	}

#define GENERATE_ENUM(ENUM) CVI_ID_ ## ENUM,

typedef enum _MOD_ID_E FOREACH_MOD(GENERATE_ENUM) MOD_ID_E;

struct MMF_CHN_S {
	MOD_ID_E	enModId;
	int32_t		s32DevId;
	int32_t		s32ChnId;
};

struct MMF_VERSION_S {
	char version[VERSION_NAME_MAXLEN];
};

#define CHN_MATCH(x, y) (((x)->enModId == (y)->enModId) && ((x)->s32DevId == (y)->s32DevId)             \
	&& ((x)->s32ChnId == (y)->s32ChnId))

struct MMF_BIND_DEST_S {
	uint32_t u32Num;
	struct MMF_CHN_S astMmfChn[BIND_DEST_MAXNUM];
};

struct BIND_NODE_S {
	bool bUsed;
	struct MMF_CHN_S src;
	struct MMF_BIND_DEST_S dsts;
};

struct SIZE_S {
	uint32_t u32Width;
	uint32_t u32Height;
};

enum PIXEL_FORMAT_E {
	PIXEL_FORMAT_RGB_888 = 0,
	PIXEL_FORMAT_BGR_888,
	PIXEL_FORMAT_RGB_888_PLANAR,
	PIXEL_FORMAT_BGR_888_PLANAR,

	PIXEL_FORMAT_ARGB_1555, // 4,
	PIXEL_FORMAT_ARGB_4444,
	PIXEL_FORMAT_ARGB_8888,

	PIXEL_FORMAT_RGB_BAYER_8BPP, // 7,
	PIXEL_FORMAT_RGB_BAYER_10BPP,
	PIXEL_FORMAT_RGB_BAYER_12BPP,
	PIXEL_FORMAT_RGB_BAYER_14BPP,
	PIXEL_FORMAT_RGB_BAYER_16BPP,

	PIXEL_FORMAT_YUV_PLANAR_422, // 12,
	PIXEL_FORMAT_YUV_PLANAR_420,
	PIXEL_FORMAT_YUV_PLANAR_444,
	PIXEL_FORMAT_YUV_400,

	PIXEL_FORMAT_HSV_888, // 16,
	PIXEL_FORMAT_HSV_888_PLANAR,

	PIXEL_FORMAT_NV12, // 18,
	PIXEL_FORMAT_NV21,
	PIXEL_FORMAT_NV16,
	PIXEL_FORMAT_NV61,
	PIXEL_FORMAT_YUYV,
	PIXEL_FORMAT_UYVY,
	PIXEL_FORMAT_YVYU,
	PIXEL_FORMAT_VYUY,

	PIXEL_FORMAT_FP32_C1 = 32, // 32
	PIXEL_FORMAT_FP32_C3_PLANAR,
	PIXEL_FORMAT_INT32_C1,
	PIXEL_FORMAT_INT32_C3_PLANAR,
	PIXEL_FORMAT_UINT32_C1,
	PIXEL_FORMAT_UINT32_C3_PLANAR,
	PIXEL_FORMAT_BF16_C1,
	PIXEL_FORMAT_BF16_C3_PLANAR,
	PIXEL_FORMAT_INT16_C1,
	PIXEL_FORMAT_INT16_C3_PLANAR,
	PIXEL_FORMAT_UINT16_C1,
	PIXEL_FORMAT_UINT16_C3_PLANAR,
	PIXEL_FORMAT_INT8_C1,
	PIXEL_FORMAT_INT8_C3_PLANAR,
	PIXEL_FORMAT_UINT8_C1,
	PIXEL_FORMAT_UINT8_C3_PLANAR,

	PIXEL_FORMAT_8BIT_MODE = 48, //48

	PIXEL_FORMAT_MAX
};

/*
 * VB_REMAP_MODE_NONE: no remap.
 * VB_REMAP_MODE_NOCACHE: no cache remap.
 * VB_REMAP_MODE_CACHED: cache remap. flush cache is needed.
 */
enum VB_REMAP_MODE_E {
	VB_REMAP_MODE_NONE = 0,
	VB_REMAP_MODE_NOCACHE = 1,
	VB_REMAP_MODE_CACHED = 2,
	VB_REMAP_MODE_BUTT
};

/*
 * u32BlkSize: size of blk in the pool.
 * u32BlkCnt: number of blk in the pool.
 * enRemapMode: remap mode.
 */
struct VB_POOL_CONFIG_S {
	uint32_t u32BlkSize;
	uint32_t u32BlkCnt;
	enum VB_REMAP_MODE_E enRemapMode;
	char acName[MAX_VB_POOL_NAME_LEN];
};

// start point is included.
// end point is excluded.
struct crop_size {
	uint16_t  start_x;
	uint16_t  start_y;
	uint16_t  end_x;
	uint16_t  end_y;
};

struct buffer {
	uint64_t phy_addr[NUM_OF_PLANES];
	size_t length[NUM_OF_PLANES];
	uint32_t stride[NUM_OF_PLANES];
	struct SIZE_S size;
	uint64_t u64PTS;
	uint8_t dev_num;
	enum PIXEL_FORMAT_E enPixelFormat;
	uint32_t frm_num;
	struct crop_size frame_crop;
};

struct VB_S {
	uint32_t vb_pool;
	uint64_t phy_addr;
	void *vir_addr;
	atomic_uint usr_cnt;
	struct buffer buf;
	uint32_t magic;
	uint64_t *mod_ids;
	bool external;
};

FIFO_HEAD(vbq, vb_s*);

struct VB_POOL_S {
	uint32_t poolID;
	int16_t ownerID;
	uint64_t memBase;
	void *vmemBase;
#ifdef __arm__
	__u32 padding; /* padding for keeping same size of this structure */
#endif
	struct vbq freeList;
	struct VB_POOL_CONFIG_S config;
	bool bIsCommPool;
	uint32_t u32FreeBlkCnt;
	uint32_t u32MinFreeBlkCnt;
	char acPoolName[MAX_VB_POOL_NAME_LEN];
} __aligned(8);

#endif // _U_MW_CVI_BASE_H_
