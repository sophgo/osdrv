#ifndef __VB_H__
#define __VB_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include "linux/cvi_common.h"
#include "linux/cvi_comm_vb.h"
#include "base_ctx.h"

/*
 *
 * frame_crop: for dis
 * s16OffsetXXX: equal to the offset member in VIDEO_FRAME_S.
 *               to show the invalid area in size.
 */
struct cvi_buffer {
	uint64_t phy_addr[NUM_OF_PLANES];
	size_t length[NUM_OF_PLANES];
	uint32_t stride[NUM_OF_PLANES];
	SIZE_S size;
	uint64_t u64PTS;
	uint8_t dev_num;
	PIXEL_FORMAT_E enPixelFormat;
	COMPRESS_MODE_E enCompressMode;
	uint64_t compress_expand_addr;
	uint32_t frm_num;
	struct crop_size frame_crop;

	int16_t s16OffsetTop;
	int16_t s16OffsetBottom;
	int16_t s16OffsetLeft;
	int16_t s16OffsetRight;

	uint8_t  motion_lv;
	uint8_t  motion_table[MO_TBL_SIZE];

	uint32_t u32FrameFlag;
	uint32_t sequence;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct timespec64 timestamp;
#else
	struct timeval timestamp;
#endif
};

/*
 * vb_pool: the pool this blk belongs to.
 * phy_addr: physical address of the blk.
 * vir_addr: virtual address of the blk.
 * usr_cnt: ref-count of the blk.
 * buf: the usage which define planes of buffer.
 * magic: magic number to avoid wrong reference.
 * mod_ids: the users of this blk. BIT(MOD_ID) will be set is MOD using.
 */
struct vb_s {
	VB_POOL vb_pool;
	uint64_t phy_addr;
	void *vir_addr;
	atomic_t usr_cnt;
	struct cvi_buffer buf;
	uint32_t magic;
	atomic64_t mod_ids;
	CVI_BOOL external;
	struct hlist_node node;
};

FIFO_HEAD(vbq, vb_s*);

typedef int32_t(*vb_acquire_fp)(MMF_CHN_S, void *);

struct vb_req {
	STAILQ_ENTRY(vb_req) stailq;
	vb_acquire_fp fp;
	MMF_CHN_S chn;
	VB_POOL VbPool;
	void *data;
};

STAILQ_HEAD(vb_req_q, vb_req);


struct vb_pool {
	VB_POOL poolID;
	int16_t ownerID;
	uint64_t memBase;
	void *vmemBase;
	struct vbq freeList;
	struct vb_req_q reqQ;
	uint32_t blk_size;
	uint32_t blk_cnt;
	VB_REMAP_MODE_E remap_mode;
	CVI_BOOL bIsCommPool;
	uint32_t u32FreeBlkCnt;
	uint32_t u32MinFreeBlkCnt;
	char acPoolName[VB_POOL_NAME_LEN];
	struct mutex lock;
	struct mutex reqQ_lock;
};


/* Generall common pool use this owner id, module common pool use VB_UID as owner id */
#define POOL_OWNER_COMMON -1
/* Private pool use this owner id */
#define POOL_OWNER_PRIVATE -2

#define CVI_VB_MAGIC 0xbabeface

int32_t vb_get_pool_info(struct vb_pool **pool_info, uint32_t *pMaxPool, uint32_t *pMaxBlk);
void vb_cleanup(void);
int32_t vb_get_config(struct cvi_vb_cfg *pstVbConfig);

int32_t vb_create_pool(struct cvi_vb_pool_cfg *config);
int32_t vb_destroy_pool(uint32_t poolId);

VB_BLK vb_phys_addr2handle(uint64_t u64PhyAddr);
uint64_t vb_handle2phys_addr(VB_BLK blk);
void *vb_handle2virt_addr(VB_BLK blk);
VB_POOL vb_handle2pool_id(VB_BLK blk);
int32_t vb_inquire_user_cnt(VB_BLK blk, uint32_t *pCnt);

VB_BLK vb_get_block_with_id(VB_POOL poolId, uint32_t u32BlkSize, MOD_ID_E modId);
VB_BLK vb_create_block(uint64_t phyAddr, void *virAddr, VB_POOL VbPool, bool isExternal);
int32_t vb_release_block(VB_BLK blk);
VB_POOL find_vb_pool(uint32_t u32BlkSize);
void vb_acquire_block(vb_acquire_fp fp, MMF_CHN_S chn, VB_POOL VbPool, void *data);
void vb_cancel_block(MMF_CHN_S chn, VB_POOL VbPool);
long vb_ctrl(unsigned long arg);
int32_t vb_create_instance(void);
void vb_destroy_instance(void);
void vb_release(void);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __VB_H__ */
