#include "api.h"
#include "core/main_controller.h"
#include "core/video.h"
#include "core/audio.h"
#include "core/packets.h"
#include "core/irq.h"
#include "core/hdmi_core.h"
#include "core/hdmi_reg.h"
#include "hdcp/hdcp.h"
#include "edid/edid.h"
#include "util/util.h"
#include "identification/identification.h"

void api_set_hdmi_ctrl(hdmi_tx_dev_t *dev, videoParams_t * video, hdcpParams_t * hdcp)
{
	struct hdmi_tx_ctrl *tx_ctrl = &dev->snps_hdmi_ctrl;
	tx_ctrl->data_enable_polarity = 1;

	if (video->mEncodingOut == YCC422)
		tx_ctrl->color_resolution = 8;
	else
		tx_ctrl->color_resolution = video->mColorResolution;
	tx_ctrl->encoding = video->mEncodingOut;

}

int api_configure(hdmi_tx_dev_t *dev, videoParams_t * video, audioParams_t * audio, hdcpParams_t * hdcp)
{
	int success = TRUE;

	if (!dev) {
		pr_err("%s: Device pointer invalid", __func__);
		return FALSE;
	}

	if (!video || !audio || !hdcp) {
		pr_err("%s: Argument Invalid received", __func__);
		return FALSE;
	}

	api_set_hdmi_ctrl(dev, video, hdcp);

	if(!dev->snps_hdmi_ctrl.pixel_clock) {
		dev->snps_hdmi_ctrl.pixel_clock = video->mDtd.mPixelClock;
	}

	cvitek_hdmi_clk_set(dev->snps_hdmi_ctrl.pixel_clock);
	disp_hdmi_gen(&(video->mDtd));
	udelay(10);

	phy_standby(dev);
	api_avmute(dev, TRUE);
	irq_mute(dev);

	success = video_configure(dev, video);
	if (success == FALSE) {
		pr_err("%s:Could not configure video", __func__);
	}

	// Audio
	audio_Initialize(dev);
	success = audio_configure(dev, audio);
	if(success == FALSE){
		pr_err("%s:Audio not configured", __func__);
	}

	// Packets
	success = packets_configure(dev, video);
	if (success == FALSE) {
		pr_err("%s:Could not configure packets\n", __func__);
	}

	mc_enable_all_clocks(dev);
	udelay(10);

	success = phy_configure(dev);
	if (success == FALSE) {
		pr_debug("%s:Could not configure PHY", __func__);
	}

	// Disable blue screen transmission after turning on all necessary blocks (e.g. HDCP)
	fc_force_output(dev, FALSE);

	// HDCP is PHY independent
	if (hdcp_initialize(dev) != TRUE) {
		pr_err("%s:Could not initialize HDCP", __func__);
	}
	udelay(20);

	success = hdcp_configure(dev, hdcp, video);
	if (success == FALSE) {
		pr_err("%s:Could not configure HDCP", __func__);
	}

	api_avmute(dev, FALSE);

	return success;
}

int api_standby(hdmi_tx_dev_t *dev)
{
	control_standby(dev);
	phy_standby(dev);

	return TRUE;
}

void api_avmute(hdmi_tx_dev_t *dev, int enable)
{
	packets_av_mute(dev, enable);
	hdcp_av_mute(dev, enable);
	dev->snps_hdmi_ctrl.avmute = enable ? TRUE : FALSE;
}
