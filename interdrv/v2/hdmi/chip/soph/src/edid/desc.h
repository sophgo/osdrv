#ifndef _DESC_H_
#define _DESC_H_

#include "core/hdmitx_dev.h"
#include "util/util.h"

typedef struct {
	u8 mFormat;

	u8 mMaxChannels;

	u8 mSampleRates;

	u8 mByte3;
} shortAudioDesc_t;

typedef struct {
	int 	mNative;

	unsigned mCode;

	unsigned mLimitedToYcc420;

	unsigned mYcc420;

} shortVideoDesc_t;

typedef struct {
	u8 mMinVerticalRate;

	u8 mMaxVerticalRate;

	u8 mMinHorizontalRate;

	u8 mMaxHorizontalRate;

	u8 mMaxPixelClock;

	int mValid;
} monitorRangeLimits_t;

void monitor_range_limits_reset(hdmi_tx_dev_t *dev, monitorRangeLimits_t * mrl);

int svd_parse(hdmi_tx_dev_t *dev, shortVideoDesc_t * svd, u8 data);

void sad_reset(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad);

int sad_parse(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad, u8 * data);

//u8 sad_GetSampleRates(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad);

int sad_support32k(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad);

int sad_support44k1(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad);

int sad_support48k(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad);

int sad_support88k2(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad);

int sad_support96k(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad);

int sad_support176k4(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad);

int sad_support192k(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad);

int sad_support16bit(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad);

int sad_support20bit(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad);

int sad_support24bit(hdmi_tx_dev_t *dev, shortAudioDesc_t * sad);

#endif	/* _DESC_H_ */
