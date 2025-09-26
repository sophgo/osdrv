#ifndef __VI_RAW_DUMP_H__
#define __VI_RAW_DUMP_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "vi_defines.h"

enum RAWDUMP_STATE {
	RAWDUMP_IDLE,
	RAWDUMP_START,
	RAWDUMP_PREPARE,
	RAWDUMP_PREPARE_DONE,
	RAWDUMP_DONE,
};

enum SMOOTH_RAWDUMP_STATE {
	SMOOTH_RAWDUMP_IDLE,
	SMOOTH_RAWDUMP_START,
	SMOOTH_RAWDUMP_STOP,
};

struct raw_dump_memblock {
	u8  raw_num;
	u64 phy_addr;
	u32 size;
};

struct raw_dump_info {
	struct raw_dump_memblock raw_dump;

	u64 pts;
	u32 frm_num;
	u32 time_out;//msec
	u16 src_w;
	u16 src_h;
	u16 crop_x;
	u16 crop_y;
	u8  is_b_not_rls;
	u8  is_timeout;
	u8  is_sig_int;
};

/*******************************************************************************
 *	rawdump interfaces
 ******************************************************************************/
void _isp_fe_raw_dump_cfg(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num, const u8 chn_num);
int isp_raw_dump(struct sop_vi_dev *vdev, struct raw_dump_info *dump);
void free_isp_byr(struct sop_vi_dev *vdev, u8 pipe);

int isp_start_smooth_raw_dump(struct sop_vi_dev *vdev, struct sop_vip_isp_smooth_raw_param *pstSmoothRawParam);
int isp_stop_smooth_raw_dump(struct sop_vi_dev *vdev, struct sop_vip_isp_smooth_raw_param *pstSmoothRawParam);
int isp_get_smooth_raw_dump(struct sop_vi_dev *vdev, struct raw_dump_info *dump);
int isp_put_smooth_raw_dump(struct sop_vi_dev *vdev, struct raw_dump_info *dump);

void _isp_raw_dump_chk(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num, const u32 frm_num);
void isp_raw_dump_init(struct sop_vi_dev *vdev);
void isp_raw_dump_deinit(struct sop_vi_dev *vdev);
void isp_raw_dump_vb_queue(struct sop_vi_dev *vdev, struct isp_buffer *buf, bool try2sched);

#ifdef __cplusplus
}
#endif

#endif //__VI_RAW_DUMP_H__
