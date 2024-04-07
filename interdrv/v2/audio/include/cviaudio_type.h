#ifndef __CVIAUDIO_TYPE_H__
#define __CVIAUDIO_TYPE_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/clk.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include <linux/streamline_annotate.h>
#include <linux/version.h>
//#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
#include <uapi/linux/sched/types.h>
//#endif
//include CVI header type ----------------------start
#include <cvi_comm_aio.h>
//include CVI header type ----------------------stop
#define CVIAUDIO_SHARE_MEM_SIZE	(0x32000)
//define VQE_DEBUG
//define error check macro for audio channels number / device numbers
#define CVIAUDIO_CHECK_CHN_VALID(Y) ((((Y) >= CVI_AUD_MAX_CHANNEL_NUM) || ((Y) < 0)) ? -1:1)
#define CVIAUDIO_CHECK_DEV_VALID(DEV_ID) (((DEV_ID) >= CVI_MAX_AI_DEVICE_ID_NUM) ? -1:1)

#ifndef SAFE_FREE_BUF
#define SAFE_FREE_BUF(OBJ) {if (OBJ != NULL) {kfree(OBJ); OBJ = NULL; } }
#endif

#ifndef SAFE_FREE_BUF_PHY
#define SAFE_FREE_BUF_PHY(VAR)  {if ((*VAR) != 0) {kfree(phys_to_virt((*VAR))); (*VAR) = 0; } }
#endif

//define log print for kernel mode ------------------------start

extern u32 audio_log_lv;
enum aud_msg_pri {
	AUDIO_ERR		= 0x1,
	AUDIO_WARN		= 0x2,
	AUDIO_NOTICE		= 0x4,
	AUDIO_INFO		= 0x8,
	AUDIO_DBG		= 0x10,
};
extern u32 audio_log_lv;
#define audio_pr(level, fmt, arg...) \
	do { \
		if (audio_log_lv & level) { \
			if (level == AUDIO_ERR) \
				pr_err("[cviaudio_core]%s:%d(): " fmt, __func__, __LINE__, ## arg); \
			else if (level == AUDIO_WARN) \
				pr_warn("[cviaudio_core]%s:%d(): " fmt, __func__, __LINE__, ## arg); \
			else if (level == AUDIO_NOTICE) \
				pr_notice("[cviaudio_core]%s:%d(): " fmt, __func__, __LINE__, ## arg); \
			else if (level == AUDIO_INFO) \
				pr_err("[cviaudio_core][info]%s:%d(): " fmt, __func__, __LINE__, ## arg); \
			else if (level == AUDIO_DBG) \
				pr_info("[cviaudio_core]%s:%d(): " fmt, __func__, __LINE__, ## arg); \
		} \
	} while (0)
//define log print for kernel mode---------------------------stop
//global structure for cviaudio device -----------------------start
struct cviaudio_dev {
	struct device			*dev;
	struct class			*cviaudio_class;
	struct cdev			cdev_ceo;
	dev_t				cdev_id;
	void				*shared_mem;
	int				cviaudio_major;
	const struct vcodec_drv_ops	*pdata;
};

struct cviaudio_status {
	int counter;
};

typedef struct cviaudio_status st_cviaudio_status;
//global structure for cviaudio device -----------------------stop


#ifdef __cplusplus
}
#endif

#endif /* __CVIAUDIO_INTERFACE_H__ */
