#ifndef _DATA_BLOCK_H_
#define _DATA_BLOCK_H_

#include "core/hdmitx_dev.h"
#include "util/util.h"

typedef struct {
	u8 mByte3;

	u8 mByte4;

	int mValid;

} colorimetryDataBlock_t;

typedef struct {
	u8 mByte1;

	int mValid;
} speakerAllocationDataBlock_t;

typedef struct {
	int mQuantizationRangeSelectable;

	u8 mPreferredTimingScanInfo;

	u8 mItScanInfo;

	u8 mCeScanInfo;

	int mValid;
} videoCapabilityDataBlock_t;

void video_cap_data_block_reset(hdmi_tx_dev_t *dev, videoCapabilityDataBlock_t * vcdb);

int video_cap_data_block_parse(hdmi_tx_dev_t *dev, videoCapabilityDataBlock_t * vcdb, u8 * data);

void colorimetry_data_block_reset(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb);

int colorimetry_data_block_parse(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb, u8 * data);

int supports_xv_ycc709(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb);

int supports_xv_ycc601(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb);

int supports_s_ycc601(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb);

int supports_adobe_ycc601(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb);

int supports_adobe_rgb(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb);

int supports_metadata0(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb);

int supports_metadata1(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb);

int supports_metadata2(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb);

int supports_metadata3(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb);

void speaker_alloc_data_block_reset(hdmi_tx_dev_t *dev, speakerAllocationDataBlock_t * sadb);

int speaker_alloc_data_block_parse(hdmi_tx_dev_t *dev, speakerAllocationDataBlock_t * sadb, u8 * data);

int get_channell_alloc_code(hdmi_tx_dev_t *dev, speakerAllocationDataBlock_t * sadb);

#endif	/* _DATA_BLOCK_H_ */
