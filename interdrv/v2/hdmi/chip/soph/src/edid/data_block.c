#include "edid/data_block.h"
#include "util/util.h"

typedef struct speaker_alloc_code {
	unsigned char byte;
	unsigned char code;
} speaker_alloc_code_t;

static speaker_alloc_code_t alloc_codes[] = {
		{1,  0},
		{3,  1},
		{5,  2},
		{7,  3},
		{17, 4},
		{19, 5},
		{21, 6},
		{23, 7},
		{9,  8},
		{11, 9},
		{13, 10},
		{15, 11},
		{25, 12},
		{27, 13},
		{29, 14},
		{31, 15},
		{73, 16},
		{75, 17},
		{77, 18},
		{79, 19},
		{33, 20},
		{35, 21},
		{37, 22},
		{39, 23},
		{49, 24},
		{51, 25},
		{53, 26},
		{55, 27},
		{41, 28},
		{43, 29},
		{45, 30},
		{47, 31},
		{0, 0}
};

void speaker_alloc_data_block_reset(hdmi_tx_dev_t *dev, speakerAllocationDataBlock_t * sadb)
{
	sadb->mByte1 = 0;
	sadb->mValid = FALSE;
}

int speaker_alloc_data_block_parse(hdmi_tx_dev_t *dev, speakerAllocationDataBlock_t * sadb, u8 * data)
{
	speaker_alloc_data_block_reset(dev, sadb);
	if ((data != 0) && (bit_field(data[0], 0, 5) == 0x03) && (bit_field(data[0], 5, 3) == 0x04)) {
		sadb->mByte1 = data[1];
		sadb->mValid = TRUE;
		return TRUE;
	}
	return FALSE;
}

int get_channell_alloc_code(hdmi_tx_dev_t *dev, speakerAllocationDataBlock_t * sadb)
{
	int i = 0;
	for(i = 0; alloc_codes[i].byte != 0; i++){
		if(sadb->mByte1 == alloc_codes[i].byte){
			return alloc_codes[i].code;
		}
	}
	return -1;
}

void colorimetry_data_block_reset(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb)
{
	cdb->mByte3 = 0;
	cdb->mByte4 = 0;
	cdb->mValid = FALSE;
}

int colorimetry_data_block_parse(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb, u8 * data)
{
	colorimetry_data_block_reset(dev, cdb);
	if ((data != 0) && (bit_field(data[0], 0, 5) == 0x03) &&
		(bit_field(data[0], 5, 3) == 0x07) && (bit_field(data[1], 0, 7) == 0x05)) {
		cdb->mByte3 = data[2];
		cdb->mByte4 = data[3];
		cdb->mValid = TRUE;
		return TRUE;
	}
	return FALSE;
}

int supports_xv_ycc709(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb)
{
	return (bit_field(cdb->mByte3, 1, 1) == 1) ? TRUE : FALSE;
}

int supports_xv_ycc601(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb)
{
	return (bit_field(cdb->mByte3, 0, 1) == 1) ? TRUE : FALSE;
}

int supports_s_ycc601(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb)
{
	return (bit_field(cdb->mByte3, 2, 1) == 1) ? TRUE : FALSE;
}

int supports_adobe_ycc601(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb)
{
	return (bit_field(cdb->mByte3, 3, 1) == 1) ? TRUE : FALSE;
}

int supports_adobe_rgb(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb)
{
	return (bit_field(cdb->mByte3, 4, 1) == 1) ? TRUE : FALSE;
}

int supports_metadata0(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb)
{
	return (bit_field(cdb->mByte4, 0, 1) == 1) ? TRUE : FALSE;
}

int supports_metadata1(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb)
{
	return (bit_field(cdb->mByte4, 1, 1) == 1) ? TRUE : FALSE;
}

int supports_metadata2(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb)
{
	return (bit_field(cdb->mByte4, 2, 1) == 1) ? TRUE : FALSE;
}

int supports_metadata3(hdmi_tx_dev_t *dev, colorimetryDataBlock_t * cdb)
{
	return (bit_field(cdb->mByte4, 3, 1) == 1) ? TRUE : FALSE;
}

void video_cap_data_block_reset(hdmi_tx_dev_t *dev, videoCapabilityDataBlock_t * vcdb)
{
	vcdb->mQuantizationRangeSelectable = FALSE;
	vcdb->mPreferredTimingScanInfo = 0;
	vcdb->mItScanInfo = 0;
	vcdb->mCeScanInfo = 0;
	vcdb->mValid = FALSE;
}

int video_cap_data_block_parse(hdmi_tx_dev_t *dev, videoCapabilityDataBlock_t * vcdb, u8 * data)
{
	video_cap_data_block_reset(dev, vcdb);
	/* check tag code and extended tag */
	if ((data != 0) && (bit_field(data[0], 5, 3) == 0x7) &&
		(bit_field(data[1], 0, 8) == 0x0) && (bit_field(data[0], 0, 5) == 0x2)) {	/* so far VCDB is 2 bytes long */
		vcdb->mCeScanInfo = bit_field(data[2], 0, 2);
		vcdb->mItScanInfo = bit_field(data[2], 2, 2);
		vcdb->mPreferredTimingScanInfo = bit_field(data[2], 4, 2);
		vcdb->mQuantizationRangeSelectable = (bit_field(data[2], 6, 1) == 1) ? TRUE : FALSE;
		vcdb->mValid = TRUE;
		return TRUE;
	}
	return FALSE;
}