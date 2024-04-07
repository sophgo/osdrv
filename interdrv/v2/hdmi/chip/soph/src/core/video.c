#include "core/video.h"
#include "core/packets.h"
#include "core/fc.h"
#include "core/main_controller.h"
#include "core/hdmi_reg.h"
#include "bsp/access.h"
#include "edid/edid.h"


int video_initialize(hdmi_tx_dev_t *dev, videoParams_t * video, u8 dataEnablePolarity)
{
	return TRUE;
}

int video_configure(hdmi_tx_dev_t *dev, videoParams_t * video)
{
	/* DVI mode does not support pixel repetition */
	if ((video->mHdmi == DVI) && video_params_is_pixel_repetition(dev, video)) {
		pr_err("DVI mode with pixel repetition: video not transmitted");
		return FALSE;
	}

	fc_force_output(dev, 1);

	if (fc_video_config(dev, video) == FALSE)
		return FALSE;
	if (video_video_packetizer(dev, video) == FALSE)
		return FALSE;
	if (video_color_space_converter(dev, video) == FALSE)
		return FALSE;
	if (video_video_sampler(dev, video) == FALSE)
		return FALSE;

	return TRUE;
}

int video_color_space_converter(hdmi_tx_dev_t *dev, videoParams_t * video)
{
	unsigned interpolation = 0;
	unsigned decimation = 0;
	unsigned color_depth = 0;

	if (video_params_is_color_space_interpolation(dev, video)) {
		if (video->mCscFilter > 1) {
			pr_err("invalid chroma interpolation filter: %d", video->mCscFilter);
			return FALSE;
		}
		interpolation = 1 + video->mCscFilter;
	}
	else if (video_params_is_color_space_decimation(dev, video)) {
		if (video->mCscFilter > 2) {
			pr_err("invalid chroma decimation filter: %d", video->mCscFilter);
			return FALSE;
		}
		decimation = 1 + video->mCscFilter;
	}

	if ((video->mColorResolution == COLOR_DEPTH_24) || (video->mColorResolution == 0))
		color_depth = 4;
	else if (video->mColorResolution == COLOR_DEPTH_30)
		color_depth = 5;
	else if (video->mColorResolution == COLOR_DEPTH_36)
		color_depth = 6;
	else if (video->mColorResolution == COLOR_DEPTH_48)
		color_depth = 7;
	else {
		pr_err("invalid color depth: %d", video->mColorResolution);
		return FALSE;
	}

	csc_config(dev, video, interpolation, decimation, color_depth);

	return TRUE;
}

int video_video_packetizer(hdmi_tx_dev_t *dev, videoParams_t * video)
{
	unsigned color_depth = 0;
	unsigned remap_size = 0;
	unsigned output_select = 0;

	if ((video->mEncodingOut == RGB) || (video->mEncodingOut == YCC444) || (video->mEncodingOut == YCC420)) {
		if (video->mColorResolution == 0)
			output_select = 3;
		else if (video->mColorResolution == COLOR_DEPTH_24) {
			color_depth = 0;
			output_select = 3;
		} else if (video->mColorResolution == COLOR_DEPTH_30)
			color_depth = 5;
		else if (video->mColorResolution == COLOR_DEPTH_36)
			color_depth = 6;
		else if (video->mColorResolution == COLOR_DEPTH_48)
			color_depth = 7;
		else {
			pr_err("invalid color depth: %d", video->mColorResolution);
			return FALSE;
		}
	}
	else if (video->mEncodingOut == YCC422) {
		if ((video->mColorResolution == COLOR_DEPTH_24) || (video->mColorResolution == 0))
			remap_size = 0;
		else if (video->mColorResolution == COLOR_DEPTH_30)
			remap_size = 1;
		else if (video->mColorResolution == COLOR_DEPTH_36)
			remap_size = 2;
		else {
			pr_err("invalid color remap size: %d", video->mColorResolution);
			return FALSE;
		}
		output_select = 1;
	}
	else {
		pr_err("invalid output encoding type: %d", video->mEncodingOut);
		return FALSE;
	}

	vp_pixel_repetition_factor(dev, video->mPixelRepetitionFactor);
	vp_color_depth(dev, color_depth);
	vp_pixel_packing_default_phase(dev, video->mPixelPackingDefaultPhase);
	vp_ycc422_remap_size(dev, remap_size);
	vp_output_selector(dev, output_select);
	return TRUE;
}

int video_video_sampler(hdmi_tx_dev_t *dev, videoParams_t * video)
{
	unsigned map_code = 0;

	if (video->mEncodingIn == RGB || video->mEncodingIn == YCC444 || video->mEncodingIn == YCC420) {
		if ((video->mColorResolution == COLOR_DEPTH_24) || (video->mColorResolution == 0))
			map_code = 1;
		else if (video->mColorResolution == COLOR_DEPTH_30)
			map_code = 3;
		else if (video->mColorResolution == COLOR_DEPTH_36)
			map_code = 5;
		else if (video->mColorResolution == COLOR_DEPTH_48)
			map_code = 7;
		else {
			pr_err("invalid color depth: %d", video->mColorResolution);
			return FALSE;
		}
		map_code += (video->mEncodingIn != RGB) ? 8 : 0;
	} else if (video->mEncodingIn == YCC422) {
		/* YCC422 mapping is discontinued - only map 1 is supported */
		if (video->mColorResolution == COLOR_DEPTH_36)
			map_code = 18;
		else if (video->mColorResolution == COLOR_DEPTH_30)
			map_code = 20;
		else if ((video->mColorResolution == COLOR_DEPTH_24) || (video->mColorResolution == 0))
			map_code = 22;
		else {
			pr_err("invalid color remap size: %d", video->mColorResolution);
			return FALSE;
		}
	} else {
		pr_err("invalid input encoding type: %d", video->mEncodingIn);
		return FALSE;
	}

	video_sampler_config(dev, map_code);

	return TRUE;
}

int video_params_get_cea_vic_code(int hdmi_vic_code)
{
	switch(hdmi_vic_code)
	{
	case 1:
		return 95;
		break;
	case 2:
		return 94;
		break;
	case 3:
		return 93;
		break;
	case 4:
		return 98;
		break;
	default:
		return -1;
		break;
	}
	return -1;
}

int video_params_get_hhdmi_vic_code(int cea_code)
{
	switch(cea_code)
	{
	case 95:
		return 1;
		break;
	case 94:
		return 2;
		break;
	case 93:
		return 3;
		break;
	case 98:
		return 4;
		break;
	default:
		return -1;
		break;
	}
	return -1;
}

void video_params_reset(hdmi_tx_dev_t *dev, videoParams_t * params)
{
	params->mHdmi = MODE_UNDEFINED;
	params->mEncodingOut = RGB;
	params->mEncodingIn = RGB;
	params->mColorResolution = COLOR_DEPTH_24;
	params->mPixelRepetitionFactor = 0;
	params->mRgbQuantizationRange = 0;
	params->mPixelPackingDefaultPhase = 0;
	params->mColorimetry = 0;
	params->mScanInfo = 0;
	params->mActiveFormatAspectRatio = 8;
	params->mNonUniformScaling = 0;
	params->mExtColorimetry = ~0;
	params->mItContent = 0;
	params->mEndTopBar = ~0;
	params->mStartBottomBar = ~0;
	params->mEndLeftBar = ~0;
	params->mStartRightBar = ~0;
	params->mCscFilter = 0;
	params->mHdmiVideoFormat = UNDEFINED_FORMAT;
	params->m3dStructure = UNDEFINED_3D;
	params->m3dExtData = UNDEFINED_EXTDATA_3D;
	params->mHdmiVic = 0;
	params->mHdmi20 = 0;

#if 0
	params->mDtd.mCode = 0;
	params->mDtd.mLimitedToYcc420 = 0xFF;
	params->mDtd.mYcc420 = 0xFF;
	params->mDtd.mPixelRepetitionInput = 0xFF;
	params->mDtd.mPixelClock = 0.0;
	params->mDtd.mInterlaced = 0xFF;
	params->mDtd.mHActive = 0;
	params->mDtd.mHBlanking = 0;
	params->mDtd.mHBorder = 0xFFFF;
	params->mDtd.mHImageSize = 0;
	params->mDtd.mHSyncOffset = 0;
	params->mDtd.mHSyncPulseWidth = 0;
	params->mDtd.mHSyncPolarity = 0xFF;
	params->mDtd.mVActive = 0;
	params->mDtd.mVBlanking = 0;
	params->mDtd.mVBorder = 0xFFFF;
	params->mDtd.mVImageSize = 0;
	params->mDtd.mVSyncOffset = 0;
	params->mDtd.mVSyncPulseWidth = 0;
	params->mDtd.mVSyncPolarity = 0xFF;
#endif

}

u16 *video_params_get_csc_a(hdmi_tx_dev_t *dev, videoParams_t * params)
{
	video_params_update_csc_coefficients(dev, params);
	return params->mCscA;
}

void video_params_set_csc_a(hdmi_tx_dev_t *dev, videoParams_t * params, u16 value[4])
{
	u16 i = 0;
	for (i = 0; i < sizeof(params->mCscA) / sizeof(params->mCscA[0]); i++) {
		params->mCscA[i] = value[i];
	}
}

u16 *video_params_get_csc_b(hdmi_tx_dev_t *dev, videoParams_t * params)
{
	video_params_update_csc_coefficients(dev, params);
	return params->mCscB;
}

void video_params_set_csc_b(hdmi_tx_dev_t *dev, videoParams_t * params, u16 value[4])
{
	u16 i = 0;
	for (i = 0; i < sizeof(params->mCscB) / sizeof(params->mCscB[0]); i++) {
		params->mCscB[i] = value[i];
	}
}

u16 *video_params_get_csc_c(hdmi_tx_dev_t *dev, videoParams_t * params)
{
	video_params_update_csc_coefficients(dev, params);
	return params->mCscC;
}

void video_params_set_csc_c(hdmi_tx_dev_t *dev, videoParams_t * params, u16 value[4])
{
	u16 i = 0;
	for (i = 0; i < sizeof(params->mCscC) / sizeof(params->mCscC[0]); i++) {
		params->mCscC[i] = value[i];
	}
}

void video_params_set_csc_scale(hdmi_tx_dev_t *dev, videoParams_t * params, u16 value)
{
	params->mCscScale = value;
}

/* [0.01 MHz] */
u32 video_params_get_pixel_clock(hdmi_tx_dev_t *dev, videoParams_t * params)
{
	u32 pixelClock = 0;
	pixelClock = params->mDtd.mPixelClock;

	pr_debug("Pixel clock %xMHz", pixelClock);

	return pixelClock;
}

/* 0.01 */
u32 video_params_get_ratio_clock(hdmi_tx_dev_t *dev, videoParams_t * params)
{
	u32 ratio = 1000;

	if (params->mEncodingOut != YCC422) {
		if (params->mColorResolution == COLOR_DEPTH_24) {
			ratio = 1000;
		} else if (params->mColorResolution == COLOR_DEPTH_30) {
			ratio = 1250;
		} else if (params->mColorResolution == COLOR_DEPTH_36) {
			ratio = 1500;
		} else if (params->mColorResolution == COLOR_DEPTH_48) {
			ratio = 2000;
		}
	}
	return ratio * (params->mPixelRepetitionFactor + 1);
}

int video_params_is_color_space_conversion(hdmi_tx_dev_t *dev, videoParams_t * params)
{
	return params->mEncodingIn != params->mEncodingOut;
}

int video_params_is_color_space_decimation(hdmi_tx_dev_t *dev, videoParams_t * params)
{
	return params->mEncodingOut == YCC422 && (params->mEncodingIn == RGB
			|| params->mEncodingIn ==
					YCC444);
}

int video_params_is_color_space_interpolation(hdmi_tx_dev_t *dev, videoParams_t * params)
{
	return params->mEncodingIn == YCC422 && (params->mEncodingOut == RGB
			|| params->mEncodingOut ==
					YCC444);
}

int video_params_is_pixel_repetition(hdmi_tx_dev_t *dev, videoParams_t * params)
{
	return (params->mPixelRepetitionFactor > 0) || (params->mDtd.mPixelRepetitionInput > 0);
}

void video_params_update_csc_coefficients(hdmi_tx_dev_t *dev, videoParams_t * params)
{
	u16 i = 0;
	if (!video_params_is_color_space_conversion(dev, params)) {
		for (i = 0; i < 4; i++) {
			params->mCscA[i] = 0;
			params->mCscB[i] = 0;
			params->mCscC[i] = 0;
		}
		params->mCscA[0] = 0x2000;
		params->mCscB[1] = 0x2000;
		params->mCscC[2] = 0x2000;
		params->mCscScale = 1;
	} else if (video_params_is_color_space_conversion(dev, params) && params->mEncodingOut == RGB) {
		if (params->mColorimetry == ITU601) {
			params->mCscA[0] = 0x2000;
			params->mCscA[1] = 0x6926;
			params->mCscA[2] = 0x74fd;
			params->mCscA[3] = 0x010e;

			params->mCscB[0] = 0x2000;
			params->mCscB[1] = 0x2cdd;
			params->mCscB[2] = 0x0000;
			params->mCscB[3] = 0x7e9a;

			params->mCscC[0] = 0x2000;
			params->mCscC[1] = 0x0000;
			params->mCscC[2] = 0x38b4;
			params->mCscC[3] = 0x7e3b;

			params->mCscScale = 1;
		} else if (params->mColorimetry == ITU709) {
			params->mCscA[0] = 0x2000;
			params->mCscA[1] = 0x7106;
			params->mCscA[2] = 0x7a02;
			params->mCscA[3] = 0x00a7;

			params->mCscB[0] = 0x2000;
			params->mCscB[1] = 0x3264;
			params->mCscB[2] = 0x0000;
			params->mCscB[3] = 0x7e6d;

			params->mCscC[0] = 0x2000;
			params->mCscC[1] = 0x0000;
			params->mCscC[2] = 0x3b61;
			params->mCscC[3] = 0x7e25;

			params->mCscScale = 1;
		}
	} else if (video_params_is_color_space_conversion(dev, params) && params->mEncodingIn == RGB) {
		if (params->mColorimetry == ITU601) {
			params->mCscA[0] = 0x2591;
			params->mCscA[1] = 0x1322;
			params->mCscA[2] = 0x074b;
			params->mCscA[3] = 0x0000;

			params->mCscB[0] = 0x6535;
			params->mCscB[1] = 0x2000;
			params->mCscB[2] = 0x7acc;
			params->mCscB[3] = 0x0200;

			params->mCscC[0] = 0x6acd;
			params->mCscC[1] = 0x7534;
			params->mCscC[2] = 0x2000;
			params->mCscC[3] = 0x0200;

			params->mCscScale = 0;
		} else if (params->mColorimetry == ITU709) {
			params->mCscA[0] = 0x2dc5;
			params->mCscA[1] = 0x0d9b;
			params->mCscA[2] = 0x049e;
			params->mCscA[3] = 0x0000;

			params->mCscB[0] = 0x62f0;
			params->mCscB[1] = 0x2000;
			params->mCscB[2] = 0x7d11;
			params->mCscB[3] = 0x0200;

			params->mCscC[0] = 0x6756;
			params->mCscC[1] = 0x78ab;
			params->mCscC[2] = 0x2000;
			params->mCscC[3] = 0x0200;

			params->mCscScale = 0;
		}
	}
	/* else use user coefficients */
}

void video_params_set_ycc420_support(hdmi_tx_dev_t *dev, dtd_t * paramsDtd, shortVideoDesc_t * paramsSvd)
{
	paramsDtd->mLimitedToYcc420 = paramsSvd->mLimitedToYcc420;
	paramsDtd->mYcc420 = paramsSvd->mYcc420;
	pr_debug("set ParamsDtd->limite %d", paramsDtd->mLimitedToYcc420);
	pr_debug("set ParamsDtd->supports %d", paramsDtd->mYcc420);
	pr_debug("set ParamsSvd->limite %d", paramsSvd->mLimitedToYcc420);
	pr_debug("set ParamsSvd->supports %d", paramsSvd->mYcc420);
}


char * get_encoding_string(encoding_t encoding)
{
	switch (encoding){
		case 	RGB: return "RGB";
		case	YCC444: return "YCbCr-444";
		case	YCC422: return "YCbCr-422";
		case	YCC420: return "YCbCr-420";
		default :break;
	}
	return "Undefined";
}

void hal_video_sampler_internal_data_enable_generator(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(TX_INVID0, TX_INVID0_INTERNAL_DE_GENERATOR_MASK, (bit ? 1 : 0));
}

void hal_video_sampler_video_mapping(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write_mask(TX_INVID0, TX_INVID0_VIDEO_MAPPING_MASK, value);
}

void hal_video_sampler_stuffing_gy(hdmi_tx_dev_t *dev, u16 value)
{
	dev_write((TX_GYDATA0), (u8) (value >> 0));
	dev_write((TX_GYDATA1), (u8) (value >> 8));
	dev_write_mask(TX_INSTUFFING, TX_INSTUFFING_GYDATA_STUFFING_MASK, 1);
}

void hal_video_sampler_stuffing_rcr(hdmi_tx_dev_t *dev, u16 value)
{

	dev_write((TX_RCRDATA0), (u8) (value >> 0));
	dev_write((TX_RCRDATA1), (u8) (value >> 8));
	dev_write_mask(TX_INSTUFFING, TX_INSTUFFING_RCRDATA_STUFFING_MASK, 1);
}

void hal_video_sampler_stuffing_bcb(hdmi_tx_dev_t *dev, u16 value)
{
	dev_write((TX_BCBDATA0), (u8) (value >> 0));
	dev_write((TX_BCBDATA1), (u8) (value >> 8));
	dev_write_mask(TX_INSTUFFING, TX_INSTUFFING_BCBDATA_STUFFING_MASK, 1);
}

void video_sampler_config(hdmi_tx_dev_t *dev, u8 map_code)
{
	hal_video_sampler_internal_data_enable_generator(dev, 0);
	hal_video_sampler_video_mapping(dev, map_code);
	hal_video_sampler_stuffing_gy(dev, 0);
	hal_video_sampler_stuffing_rcr(dev, 0);
	hal_video_sampler_stuffing_bcb(dev, 0);
}