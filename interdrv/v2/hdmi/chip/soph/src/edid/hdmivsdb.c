#include "edid/hdmivsdb.h"
#include "util/util.h"

void hdmivsdb_reset(hdmi_tx_dev_t *dev, hdmivsdb_t * vsdb)
{
	int i, j = 0;
	vsdb->mPhysicalAddress = 0;
	vsdb->mSupportsAi = FALSE;
	vsdb->mDeepColor30 = FALSE;
	vsdb->mDeepColor36 = FALSE;
	vsdb->mDeepColor48 = FALSE;
	vsdb->mDeepColorY444 = FALSE;
	vsdb->mDviDual = FALSE;
	vsdb->mMaxTmdsClk = 0;
	vsdb->mVideoLatency = 0;
	vsdb->mAudioLatency = 0;
	vsdb->mInterlacedVideoLatency = 0;
	vsdb->mInterlacedAudioLatency = 0;
	vsdb->mId = 0;
	vsdb->mContentTypeSupport = 0;
	vsdb->mHdmiVicCount = 0;
	for (i = 0; i < MAX_HDMI_VIC; i++) {
		vsdb->mHdmiVic[i] = 0;
	}
	vsdb->m3dPresent = FALSE;
	for (i = 0; i < MAX_VIC_WITH_3D; i++) {
		for (j = 0; j < MAX_HDMI_3DSTRUCT; j++) {
			vsdb->mVideo3dStruct[i][j] = 0;
		}
	}
	for (i = 0; i < MAX_VIC_WITH_3D; i++) {
		for (j = 0; j < MAX_HDMI_3DSTRUCT; j++) {
			vsdb->mDetail3d[i][j] = ~0;
		}
	}
	vsdb->mValid = FALSE;
}

int hdmivsdb_parse(hdmi_tx_dev_t *dev, hdmivsdb_t * vsdb, u8 * data)
{
	u8 blockLength = 0;
	unsigned videoInfoStart = 0;
	unsigned hdmi3dStart = 0;
	unsigned hdmiVicLen = 0;
	unsigned hdmi3dLen = 0;
	unsigned spanned3d = 0;
	unsigned i = 0;
	unsigned j = 0;

	hdmivsdb_reset(dev, vsdb);
	if (data == 0) {
		return FALSE;
	}
	if (bit_field(data[0], 5, 3) != 0x3) {
		pr_err("Invalid datablock tag");
		return FALSE;
	}
	blockLength = bit_field(data[0], 0, 5);
	if (blockLength < 5) {
		pr_err("%s Invalid minimum length (%d)", __func__, blockLength);
		return FALSE;
	}
	if (byte_to_dword(0x00, data[3], data[2], data[1]) != HDMI_LICENSING_LLC_OUI) {
		pr_err("HDMI IEEE registration identifier not valid");
		return FALSE;
	}
	hdmivsdb_reset(dev, vsdb);
	vsdb->mId = HDMI_LICENSING_LLC_OUI;
	vsdb->mPhysicalAddress = byte_to_word(data[4], data[5]);
	/* parse extension fields if they exist */
	if (blockLength > 5) {
		vsdb->mSupportsAi = bit_field(data[6], 7, 1) == 1;
		vsdb->mDeepColor48 = bit_field(data[6], 6, 1) == 1;
		vsdb->mDeepColor36 = bit_field(data[6], 5, 1) == 1;
		vsdb->mDeepColor30 = bit_field(data[6], 4, 1) == 1;
		vsdb->mDeepColorY444 = bit_field(data[6], 3, 1) == 1;
		vsdb->mDviDual = bit_field(data[6], 0, 1) == 1;
	} else {
		vsdb->mSupportsAi = FALSE;
		vsdb->mDeepColor48 = FALSE;
		vsdb->mDeepColor36 = FALSE;
		vsdb->mDeepColor30 = FALSE;
		vsdb->mDeepColorY444 = FALSE;
		vsdb->mDviDual = FALSE;
	}
	vsdb->mMaxTmdsClk = (blockLength > 6) ? data[7] : 0;
	vsdb->mVideoLatency = 0;
	vsdb->mAudioLatency = 0;
	vsdb->mInterlacedVideoLatency = 0;
	vsdb->mInterlacedAudioLatency = 0;
	if (blockLength > 7) {
		if (bit_field(data[8], 7, 1) == 1) {
			if (blockLength < 10) {
				pr_err("Invalid length - latencies are not valid");
				return FALSE;
			}
			if (bit_field(data[8], 6, 1) == 1) {
				if (blockLength < 12) {
					pr_err("Invalid length - Interlaced latencies are not valid");
					return FALSE;
				} else {
					vsdb->mVideoLatency = data[9];
					vsdb->mAudioLatency = data[10];
					vsdb->mInterlacedVideoLatency = data[11];
					vsdb->mInterlacedAudioLatency = data[12];
					videoInfoStart = 13;
				}
			} else {
				vsdb->mVideoLatency = data[9];
				vsdb->mAudioLatency = data[10];
				vsdb->mInterlacedVideoLatency = 0;
				vsdb->mInterlacedAudioLatency = 0;
				videoInfoStart = 11;
			}
		} else {	/* no latency data */
			vsdb->mVideoLatency = 0;
			vsdb->mAudioLatency = 0;
			vsdb->mInterlacedVideoLatency = 0;
			vsdb->mInterlacedAudioLatency = 0;
			videoInfoStart = 9;
		}
		vsdb->mContentTypeSupport = bit_field(data[8], 0, 4);
	}
	if (bit_field(data[8], 5, 1) == 1) {	/* additional video format capabilities are described */
		vsdb->mImageSize = bit_field(data[videoInfoStart], 3, 2);
		hdmiVicLen = bit_field(data[videoInfoStart + 1], 5, 3);
		hdmi3dLen = bit_field(data[videoInfoStart + 1], 0, 5);
		for (i = 0; i < hdmiVicLen; i++) {
			vsdb->mHdmiVic[i] = data[videoInfoStart + 2 + i];
		}
		pr_debug("HDMI VIC Count %d", hdmiVicLen);
		pr_debug("HDMI 3D Present %d", bit_field(data[videoInfoStart], 7, 1));
		vsdb->mHdmiVicCount = hdmiVicLen;
		if (bit_field(data[videoInfoStart], 7, 1) == 1) {	/* 3d present */
			vsdb->m3dPresent = TRUE;
			vsdb->hdmi3dCount = hdmi3dLen;
			hdmi3dStart = videoInfoStart + hdmiVicLen + 2;
			/* 3d multi 00 -> both 3d_structure_all and 3d_mask_15 are NOT present */
			/* 3d mutli 11 -> reserved */
			if (bit_field(data[videoInfoStart], 5, 2) == 1) {	/* 3d multi 01 */
				/* 3d_structure_all is present but 3d_mask_15 not present */
				for (j = 0; j < 16; j++) {	/* j spans 3d structures */
					if (bit_field(data[hdmi3dStart + (j / 8)], (j % 8), 1) == 1) {
						int struct3d = (j < 8)	? j + 8 : j - 8;
						for (i = 0; i < 16; i++) {	/* go through 2 registers, [videoInfoStart + hdmiVicLen + 1] & [videoInfoStart + hdmiVicLen + 2]  */
							vsdb->mVideo3dStruct[i][struct3d] = 1;
						}
					}
				}
				spanned3d = 2;
				/*hdmi3dStart += 2;
				   hdmi3dLen -= 2; */
			} else if (bit_field(data[videoInfoStart], 5, 2) == 2) {	/* 3d multi 10 */
				/* 3d_structure_all and 3d_mask_15 are present */
				for (j = 0; j < 16; j++) {
					for (i = 0; i < 16; i++) {	/* assign according to mask, through 2 registers, [videoInfoStart + hdmiVicLen + 3] & [videoInfoStart + hdmiVicLen + 4] */
						if (bit_field(data[hdmi3dStart + 2 + (i / 8)], (i % 8), 1) == 1) {	/* go through 2 registers, [videoInfoStart + hdmiVicLen + 1] & [videoInfoStart + hdmiVicLen + 2]  */
							vsdb->mVideo3dStruct[(i < 8) ? i + 8 : i - 8][(j < 8) ? j + 8 : j - 8] = bit_field(data[hdmi3dStart + (j / 8)], (j % 8), 1);
						}
					}
				}
				spanned3d = 4;
			}
			if (hdmi3dLen > spanned3d) {
				hdmi3dStart += spanned3d;
				for (i = 0, j = 0; i < (hdmi3dLen - spanned3d); i++) {
#if 0
					if(i == 0){
						vsdb->hdmi3dfirst = bit_field(data[hdmi3dStart + i + j], 4, 4);
						vsdb->hdmi3dfirststrc = bit_field(data[hdmi3dStart + i + j], 0, 4);
					}
#endif
					pr_debug("3D struct %d - %d", bit_field(data[hdmi3dStart + i + j], 4, 4), bit_field(data[hdmi3dStart + i + j], 0, 4));
					vsdb->mVideo3dStruct[bit_field(data[hdmi3dStart + i + j], 4, 4)][bit_field(data[hdmi3dStart + i + j], 0, 4)] = 1;

					if (bit_field(data[hdmi3dStart + i + j], 0, 4) > 7) {	/* bytes with 3D_Detail_X and Reserved(0) are present only when 3D_Structure_X > b'1000 - side-by-side(half) */
						j++;
						pr_debug("3D_Detail_X struct %d - %d - %d", bit_field(data[hdmi3dStart + i + j], 4, 4), bit_field(data[hdmi3dStart + i + j], 4, 4), bit_field(data[hdmi3dStart + i + j], 4, 4));
						vsdb->mDetail3d[bit_field(data[hdmi3dStart + i + j], 4, 4)][bit_field(data[hdmi3dStart + i + j], 4, 4)] = bit_field(data[hdmi3dStart + i + j], 4, 4);
					}
				}
			}
		} else {	/* 3d NOT present */
			vsdb->m3dPresent = FALSE;
		}
	}
	vsdb->mValid = TRUE;
	return TRUE;
}

u16 get_index_supported_3dstructs(hdmi_tx_dev_t *dev, hdmivsdb_t * vsdb, u8 index)
{
	u16 structs3d = 0;
	int i;
	for (i = 0; i < MAX_HDMI_3DSTRUCT; i++) {
		structs3d |= (vsdb->mVideo3dStruct[index][i] & 1) << i;
	}
	return structs3d;
}

u16 get_3dstruct_indexes(hdmi_tx_dev_t *dev, hdmivsdb_t * vsdb, u8 struct3d)
{
	u16 indexes = 0;
	int i;
	for (i = 0; i < MAX_HDMI_3DSTRUCT; i++) {
		indexes |= (vsdb->mVideo3dStruct[i][struct3d] & 1) << i;
	}
	return indexes;
}

void hdmiforumvsdb_reset(hdmi_tx_dev_t *dev, hdmiforumvsdb_t * forumvsdb)
{
	forumvsdb->mValid = FALSE;
	forumvsdb->mIeee_Oui = 0;
	forumvsdb->mVersion = 0;
	forumvsdb->mMaxTmdsCharRate = 0;
	forumvsdb->mSCDC_Present = FALSE;
	forumvsdb->mRR_Capable = FALSE;
	forumvsdb->mLTS_340Mcs_scramble = FALSE;
	forumvsdb->mIndependentView = FALSE;
	forumvsdb->mDualView = FALSE;
	forumvsdb->m3D_OSD_Disparity = FALSE;
	forumvsdb->mDC_30bit_420 = FALSE;
	forumvsdb->mDC_36bit_420 = FALSE;
	forumvsdb->mDC_48bit_420 = FALSE;
}

int hdmiforumvsdb_parse(hdmi_tx_dev_t *dev, hdmiforumvsdb_t * forumvsdb, u8 * data)
{
	u16 blockLength;
	hdmiforumvsdb_reset(dev, forumvsdb);
	if (data == 0) {
		return FALSE;
	}
	if (bit_field(data[0], 5, 3) != 0x3) {
		pr_err("Invalid datablock tag");
		return FALSE;
	}
	pr_debug("block TAG ok");
	blockLength = bit_field(data[0], 0, 5);
	if (blockLength < 7) {
		pr_err("Invalid minimum length");
		return FALSE;
	}
	pr_debug("size ok");
	if (byte_to_dword(0x00, data[3], data[2], data[1]) != 0xC45DD8) {
		pr_err("HDMI IEEE registration identifier not valid");
		return FALSE;
	}
	pr_debug("ieeeid TAG ok");
	forumvsdb->mVersion = bit_field(data[4], 0, 7);
	forumvsdb->mMaxTmdsCharRate = bit_field(data[5], 0, 7);
	forumvsdb->mSCDC_Present = bit_field(data[6], 7, 1);
	forumvsdb->mRR_Capable = bit_field(data[6], 6, 1);
	forumvsdb->mLTS_340Mcs_scramble = bit_field(data[6], 3, 1);
	forumvsdb->mIndependentView = bit_field(data[6], 2, 1);
	forumvsdb->mDualView = bit_field(data[6], 1, 1);
	forumvsdb->m3D_OSD_Disparity = bit_field(data[6], 0, 1);
	forumvsdb->mDC_48bit_420 = bit_field(data[7], 2, 1);
	forumvsdb->mDC_36bit_420 = bit_field(data[7], 1, 1);
	forumvsdb->mDC_30bit_420 = bit_field(data[7], 0, 1);
	forumvsdb->mValid = TRUE;

#if 1
	pr_debug("version %d", bit_field(data[4], 0, 7));
	pr_debug("Max_TMDS_Charater_rate %d", bit_field(data[5], 0, 7));
	pr_debug("SCDC_Present %d", bit_field(data[6], 7, 1));
	pr_debug("RR_Capable %d", bit_field(data[6], 6, 1));
	pr_debug("LTE_340Mcsc_scramble %d", bit_field(data[6], 3, 1));
	pr_debug("Independent_View %d", bit_field(data[6], 2, 1));
	pr_debug("Dual_View %d", bit_field(data[6], 1, 1));
	pr_debug("3D_OSD_Disparity %d", bit_field(data[6], 0, 1));
	pr_debug("DC_48bit_420 %d", bit_field(data[7], 2, 1));
	pr_debug("DC_36bit_420 %d", bit_field(data[7], 1, 1));
	pr_debug("DC_30bit_420 %d", bit_field(data[7], 0, 1));
#endif
	return TRUE;
}
