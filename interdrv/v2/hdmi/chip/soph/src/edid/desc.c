#include "edid/desc.h"
#include "util/util.h"

void sad_reset(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad)
{
	sad->mFormat = 0;
	sad->mMaxChannels = 0;
	sad->mSampleRates = 0;
	sad->mByte3 = 0;
}

int sad_parse(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad, u8 * data)
{
	sad_reset(dev, sad);
	if (data != 0) {
		sad->mFormat = bit_field(data[0], 3, 4);
		sad->mMaxChannels = bit_field(data[0], 0, 3) + 1;
		sad->mSampleRates = bit_field(data[1], 0, 7);
		sad->mByte3 = data[2];
		return TRUE;
	}
	return FALSE;
}

int sad_support32k(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad)
{
	return (bit_field(sad->mSampleRates, 0, 1) == 1) ? TRUE : FALSE;
}

int sad_support44k1(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad)
{
	return (bit_field(sad->mSampleRates, 1, 1) == 1) ? TRUE : FALSE;
}

int sad_support48k(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad)
{
	return (bit_field(sad->mSampleRates, 2, 1) == 1) ? TRUE : FALSE;
}

int sad_support88k2(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad)
{
	return (bit_field(sad->mSampleRates, 3, 1) == 1) ? TRUE : FALSE;
}

int sad_support96k(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad)
{
	return (bit_field(sad->mSampleRates, 4, 1) == 1) ? TRUE : FALSE;
}

int sad_support176k4(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad)
{
	return (bit_field(sad->mSampleRates, 5, 1) == 1) ? TRUE : FALSE;
}

int sad_support192k(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad)
{
	return (bit_field(sad->mSampleRates, 6, 1) == 1) ? TRUE : FALSE;
}

int sad_support16bit(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad)
{
	if (sad->mFormat == 1) {
		return (bit_field(sad->mByte3, 0, 1) == 1) ? TRUE : FALSE;
	}
	pr_err("Information is not valid for this format");
	return FALSE;
}

int sad_support20bit(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad)
{
	if (sad->mFormat == 1) {
		return (bit_field(sad->mByte3, 1, 1) == 1) ? TRUE : FALSE;
	}
	pr_err("Information is not valid for this format");
	return FALSE;
}

int sad_support24bit(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad)
{
	if (sad->mFormat == 1) {
		return (bit_field(sad->mByte3, 2, 1) == 1) ? TRUE : FALSE;
	}
	pr_err("Information is not valid for this format");
	return FALSE;
}

void svd_reset(hdmi_tx_dev_t *dev, shortVideoDesc_t * svd)
{
	svd->mNative = FALSE;
	svd->mCode = 0;
}

int svd_parse(hdmi_tx_dev_t *dev, shortVideoDesc_t * svd, u8 data)
{
	svd_reset(dev, svd);
	svd->mNative = (bit_field(data, 7, 1) == 1) ? TRUE : FALSE;
	svd->mCode = bit_field(data, 0, 7);
	svd->mLimitedToYcc420 = 0;
	svd->mYcc420 = 0;
	return TRUE;
}

void monitor_range_limits_reset(hdmi_tx_dev_t *dev, monitorRangeLimits_t * mrl)
{
	mrl->mMinVerticalRate = 0;
	mrl->mMaxVerticalRate = 0;
	mrl->mMinHorizontalRate = 0;
	mrl->mMaxHorizontalRate = 0;
	mrl->mMaxPixelClock = 0;
	mrl->mValid = FALSE;
}
