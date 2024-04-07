#ifndef _HDMI_CORE_H_
#define _HDMI_CORE_H_

#include "core/hdmi_includes.h"
#include "bsp/access.h"
#include "hdmitx_dev.h"
#include "edid/edid.h"
#include "util/util.h"
#include "core/audio.h"
#include "core/video.h"
#include "core/packets.h"
#include "core/main_controller.h"
#include "scdc/scdc.h"
#include "identification/identification.h"
#include <linux/hdmi_uapi.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>

#define STRING_ARRAY_SIZE 150

#define IS_EQUAL(buffer, text) \
	((strcmp(buffer, text) == 0) ? 1 : 0)

#define GET(buf, argc) \
	strtoul(buf[argc], NULL, 0)

#define SIZE_ARRAY(array, type) \
	(sizeof(array) / sizeof(type))

#define EDID_DTD_ARRAY_SIZE  (32)
#define EDID_SVD_ARRAY_SIZE  (128)
#define EDID_SAD_ARRAY_SIZE  (128)
#define EDID_MONITOR_NAME_SIZE  (13)

#define HDMI_HOTPLUG         (1)
#define HDMI_NOPLUG          (2)
#define HDMI_EDID_FAIL       (3)

typedef struct _sink_cap_info {
	dtd_t sink_cap_info;
	u32 fresh_rate;
} sink_cap_info;

/**
 * Compliance mode structure
 * This structure contains the important information regarding the expert
 * mode: variables and structures
 */
struct hdmi_compliance_mode {
	int 		current_mode;
	int 		re_configure;
	int 		ycc420_support;
};

/**
 * Demo mode structure
 * This structure contains the important information regarding the expert
 * mode: variables and structures
 */
struct hdmi_demo_mode {
	int 		current_3D;
	int 		re_configure;
	int 		ycc420_support;
	u32	current_mode;
	u32 	cea;
	u32 	dcm;
	u32	sfr_clk;
};

typedef struct {
	/**
	 * Array to hold all the parsed Detailed Timing Descriptors.
	 */
	dtd_t edid_mDtd[32];

	unsigned int edid_mDtdIndex;
	/**
	 * array to hold all the parsed Short Video Descriptors.
	 */
	shortVideoDesc_t edid_mSvd[EDID_SVD_ARRAY_SIZE];
	shortVideoDesc_t tmpSvd;

	unsigned int edid_mSvdIndex;
	/**
	 * array to hold all the parsed Short Audio Descriptors.
	 */
	shortAudioDesc_t edid_mSad[EDID_SAD_ARRAY_SIZE];

	unsigned int edid_mSadIndex;
	int native_vic;
	/**
	 * A string to hold the Monitor Name parsed from EDID.
	 */
	char edid_mMonitorName[EDID_MONITOR_NAME_SIZE];
	int edid_mYcc444Support;
	int edid_mYcc422Support;
	int edid_mYcc420Support;
	int edid_mBasicAudioSupport;
	int edid_mUnderscanSupport;
	int xv_ycc709;
	int s_ycc601;
	int xv_ycc601;
	int adobe_ycc601;
	int adobe_rgb;
	/**
	 *  Audio
	 */
	u32 Support_SampleRate[10];
	u8  Support_BitDepth[10];

	/**
	 *  If Sink is HDMI 2.0
	 */
	int edid_m20Sink;

	hdmivsdb_t edid_mHdmivsdb;
	hdmiforumvsdb_t edid_mHdmiForumvsdb;

	monitorRangeLimits_t edid_mMonitorRangeLimits;

	videoCapabilityDataBlock_t edid_mVideoCapabilityDataBlock;

	colorimetryDataBlock_t edid_mColorimetryDataBlock;

	speakerAllocationDataBlock_t edid_mSpeakerAllocationDataBlock;
} sink_edid_t;

struct hdmi_mode {
	bool hdmi_en;

	videoParams_t 		pVideo;
	audioParams_t 		pAudio;
	hdcpParams_t 		pHdcp;
	hdmivsdb_t 			vsdb;
	hdmiforumvsdb_t 	forumvsdb;

	u8			ksv_list_buffer[670];
	u8			ksv_devices;
	u8			dpk_aksv[7];
	u8			sw_enc_key[2];
	u8			dpk_keys[560];

	u8 		      edid_ext[3][128];

	int 			hpd;
	int 			edid_done;

	struct edid 	      edid;
	sink_edid_t  	      * sink_cap;
	sink_cap_info     sink_capinfo[128];

	struct hdmi_compliance_mode compliance;
	struct hdmi_demo_mode		demo;
};

struct hdmi_tx_ctx {
	/** Verbose */
	int 				verbose;

	/** HDMI TX Driver */
	char 				hdmi_tx_name[STRING_ARRAY_SIZE];
	/** File descriptor */
	u32			base_address;

	/* Reserved for API internal use only */
	/** HDMI TX API Internals */
	struct device_access dev_access;
	hdmi_tx_dev_t 		 hdmi_tx;

	/** Application mode configurations */
	struct hdmi_mode	mode;

	/** Mutex to synchronize calls */
	struct mutex  		mutex;

	struct edid 	    * tx_edid;
	u8 		        	* tx_edid_ext;
	sink_edid_t  	    * tx_sink_cap;

	int 				edid_tx_checks;
};

int hdmitx_init(void);

int hdmitx_deinit(void);

int hdmitx_set_phy(int phy, int debug_mode);

int edid_read_cap(void);

int hdmitx_force_get_edid(CVI_HDMI_EDID* edid_raw);

int sink_capability(CVI_HDMI_SINK_CAPABILITY* cvi_sink_cap);

int hdmitx_set_infoframe(CVI_HDMI_INFOFRAME* info_frame);

int hdmitx_get_infoframe(CVI_HDMI_INFOFRAME* info_frame);

int hdmitx_start(void);

int hdmitx_stop(void);

int hdmitx_set_attr(CVI_HDMI_ATTR* attr);

int hdmitx_get_attr(CVI_HDMI_ATTR* attr);

int stop_handler(void);

void hpd_handler(void);

void hdcp_handler(void);

void reset_hdcp_params(void);

int hdmitx_set_avmute(int enable);

int hdmitx_set_audio_mute(int enable);

int edid_parser(hdmi_tx_dev_t *dev, u8 * buffer, sink_edid_t *edidExt, u16 edid_size);

void print_videoinfo(videoParams_t *pVideo);

void cvitek_hdmi_clk_set(u32 pClk);

void disp_hdmi_gen(dtd_t *mdtd);

int edid_tx_supports_cea_code(u32 cea_code);

struct hdmi_tx_ctx* get_hdmi_ctx(void);

int send_signal(hdmi_tx_dev_t *dev, int sig);

int get_current_event_id(u32* event_id);

void hdmitx_create_proc(struct hdmitx_dev * dev);

void hdmitx_destroy_proc(struct hdmitx_dev * dev);

#endif /* _HDMI_CORE_H_*/
