#ifndef _HDMIVSDB_H_
#define _HDMIVSDB_H_

#include "core/hdmitx_dev.h"
#include "util/util.h"

#define MAX_HDMI_VIC		16
#define MAX_HDMI_3DSTRUCT	16
#define MAX_VIC_WITH_3D		16

/** For detailed handling of this structure, refer to documentation of the functions */
typedef struct {
	u16 mPhysicalAddress;

	int mSupportsAi;

	int mDeepColor30;

	int mDeepColor36;

	int mDeepColor48;

	int mDeepColorY444;

	int mDviDual;

	u16 mMaxTmdsClk;

	u16 mVideoLatency;

	u16 mAudioLatency;

	u16 mInterlacedVideoLatency;

	u16 mInterlacedAudioLatency;

	u32 mId;

	u8 mContentTypeSupport;

	u8 mImageSize;

	int mHdmiVicCount;

	u8 mHdmiVic[MAX_HDMI_VIC];

	int m3dPresent;

	int hdmi3dCount; //number of 3d entries

	int hdmi3dfirst; //number of 3d entries

	int hdmi3dfirststrc; //number of 3d entries

	int mVideo3dStruct[MAX_VIC_WITH_3D][MAX_HDMI_3DSTRUCT];	/* row index is the VIC number */

	int mDetail3d[MAX_VIC_WITH_3D][MAX_HDMI_3DSTRUCT];	/* index is the VIC number */

	int mValid;

} hdmivsdb_t;

/* HDMI 2.0 HF_VSDB */
typedef struct {
	u32 mIeee_Oui;

	u8 mValid;

	u8 mVersion;

	u8 mMaxTmdsCharRate;

	u8 m3D_OSD_Disparity;

	u8 mDualView;

	u8 mIndependentView;

	u8 mLTS_340Mcs_scramble;

	u8 mRR_Capable;

	u8 mSCDC_Present;

	u8 mDC_30bit_420;

	u8 mDC_36bit_420;

	u8 mDC_48bit_420;

} hdmiforumvsdb_t;

void hdmiforumvsdb_reset(hdmi_tx_dev_t *dev, hdmiforumvsdb_t * forumvsdb);

int hdmiforumvsdb_parse(hdmi_tx_dev_t *dev, hdmiforumvsdb_t * forumvsdb, u8 * data);

void hdmivsdb_reset(hdmi_tx_dev_t *dev, hdmivsdb_t * vsdb);

int hdmivsdb_parse(hdmi_tx_dev_t *dev, hdmivsdb_t * vsdb, u8 * data);

u16 get_index_supported_3dstructs(hdmi_tx_dev_t *dev, hdmivsdb_t * vsdb, u8 index);

u16 get_3dstruct_indexes(hdmi_tx_dev_t *dev, hdmivsdb_t * vsdb, u8 struct3d);

#endif	/* _HDMIVSDB_H_ */
