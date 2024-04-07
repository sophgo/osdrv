#ifndef __CVIAUDIO_IOCTL_CMD_H__
#define __CVIAUDIO_IOCTL_CMD_H__



#include <linux/fs.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_graph.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/wait.h>
#include <linux/streamline_annotate.h>
//#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/signal.h>
//#endif
#include <generated/compile.h>
#include <linux/cvi_defines.h>
#include <cvi_comm_aio.h>
//#include <cvi_defines.h>
//watch out for corresponding device node minor id
//minor id should map to channel id
#define CVIAUDIO_DRV_MAX_ALL_NUM  (1)


#define CVIAUDIO_DRV_PLATFORM_DEVICE_NAME	"cvi-audio"
#define CVIAUDIO_DRV_CLASS_NAME			"cvi-audio"
#define CVIAUDIO_CEO_DEV_NAME			"cviaudio_core"

typedef struct _st_ssp_msg {
	AI_TALKVQE_CONFIG_S stVqeConfig;
	int bytes_per_period;
	int channel_cnt; //only support 1 or 2 channel(2nd channe should be ref-data)
	//channel should align the setting of the pcm_open setting
} ST_SSP_MSG;

typedef struct _st_ssp_pcm_msg {
	unsigned int channels;
	unsigned int rate;
	unsigned int period_size;
} ST_SSP_PCM_MSG;

typedef struct _st_ssp_data_msg {
	uint64_t data_phyaddr;
	char	*data_addr;
	int	required_size_bytes;
	int	data_valid;
} ST_SSP_DATA_MSG;

typedef struct _st_spk_ssp_msg {
	AO_VQE_CONFIG_S stAoVqeConfig;
} ST_SPK_SSP_MSG;


typedef struct RESAMPLE_INFO_T {
	int inSampleRate;
	int outSampleRate;
	double stepDist;
	uint64_t fixedFraction;
	double normFixed;
	uint64_t step;
	int16_t *last_input;
	int16_t *output_buf;
	uint64_t curOffset;
	uint32_t inputsamples;
	uint32_t prev_inputsamples; //user may not always send the same size
	uint32_t channels;
	uint32_t output_index;
	int last_delta;
} RESAMPLE_INFO;


typedef struct _st_ain_msg {
	int AiDev;
	int AiChnId;
	//for channel buffer
	uint64_t cycbufread_addr;
	int cycbufread_size;
	uint64_t vqebuf_addr;
	int vqebuf_size;
	uint64_t tinyalsaReadBuf;
	int tinyalsaReadBuf_size;
	//for resample handler
	uint64_t resample_handler_addr;
	int resample_handler_size;
	//for share memory
	int sharebuf_index;
	int bsharebuf_first;
	RESAMPLE_INFO ResInfo;
} ST_AIN_MSG;

typedef struct _st_aout_msg {
	int AoDev;
	int AoChn;
	//for channel buffer
	//uint64_t cycbufwrite_addr;
	//int cycbufwrite_size;
	int sizebytes;
	//for share memory
	int sharebuf_index;
	int bsharebuf_first;
	//for resample handler
	uint64_t resample_handler_addr;
	int resample_handler_size;
	RESAMPLE_INFO ResInfo;
} ST_AOUT_MSG;

typedef struct _st_aenc_msg {
	int AenChn;
	//for userspace mem buffer
	uint64_t EncBuff;
	uint64_t EncBuff_aec;
	uint64_t EncInBuff;
	uint64_t EncVqeBuff;

	int EncBuff_size;
	int EncBuff_aec_size;
	int EncInBuff_size;
	int EncVqeBuff_size;
	//for share buffer
	int sharebuf_index;
	int bsharebuf_first;
} ST_AENC_MSG;

typedef struct _st_adec_msg {
	int AdecChn;
	//for userspace mem buffer
	uint64_t DecBuff;
	uint64_t DecReadBuff;

	int DecBuff_size;
	int DecReadBuff_size;
	//for share memory
	int sharebuf_index;
	int bsharebuf_first;
} ST_ADEC_MSG;
struct cviaudio_drv_ops {
//	void	(*clk_get)(struct cvi_vcodec_device *vdev);
};
#define CVIAUDIO_DRV_IOCTL_MAGIC			'A'
#define CVIAUDIO_CMD_MASK	400

//define for cviaudio_adec_interface  commands
//for share buffer
#define CVIAUDIO_INIT_CHECK				_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 1))
#define CVIAUDIO_DEINIT_CHECK				_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 2))
#define CVIAUDIO_GET_MMAP_SIZE				_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 3))
#define CVIAUDIO_AIN_CHN_BUFFER_ACQUIRE			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 6))
#define CVIAUDIO_AIN_ALSA_READ_BUFFER_ACQUIRE		_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 7))
#define CVIAUDIO_AIN_SHAREBUFFER_INIT			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 8))
#define CVIAUDIO_AIN_RESAMPLE_HANDLER_ACQUIRE		_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 9))
#define CVIAUDIO_AIN_RESAMPLE_HANDLER_ATTACH		_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 10))
#define CVIAUDIO_AENC_SHAREBUFFER_INIT			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 11))
#define CVIAUDIO_AENC_CHN_BUFFER_ACQUIRE		_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 12))
#define CVIAUDIO_AOUT_SHAREBUFFER_INIT			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 13))
#define CVIAUDIO_ADEC_SHAREBUFFER_INIT			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 16))
#define CVIAUDIO_ADEC_CHN_BUFFER_ACQUIRE		_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 17))
#define CVIAUDIO_AOUT_RESAMPLE_HANDLER_ACQUIRE		_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 18))
#define CVIAUDIO_AIN_CHN_BUFFER_RELEASE			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 19))
#define CVIAUDIO_AIN_ALSA_READ_BUFFER_RELEASE		_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 20))
#define CVIAUDIO_AENC_CLEARCHNBUF			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 21))
#define CVIAUDIO_ADEC_CLEARCHNBUF			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 22))
//add SSP RTOS cmd
//for SSP algorithm in RTOS
#define CVIAUDIO_IOCTL_SSP_INIT				_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 30))
#define CVIAUDIO_IOCTL_SSP_DEINIT			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 31))
#define CVIAUDIO_IOCTL_SSP_PROC				_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 32))
#define CVIAUDIO_IOCTL_SSP_SPK_INIT			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 33))
#define CVIAUDIO_IOCTL_SSP_SPK_DEINIT			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 34))
#define CVIAUDIO_IOCTL_SSP_SPK_PROC			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 35))
#define CVIAUDIO_IOCTL_SSP_DEBUG			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 36))
#define CVIAUDIO_IOCTL_SSP_UNIT_TEST			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 37))
#define CVIAUDIO_IOCTL_SSP_UNIT_TEST_STOP		_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 38))
#define CVIAUDIO_IOCTL_SSP_UPDATE_PCM_RECORD_CFG	_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 39))
//additional RTOS command for test only
#define CVIAUDIO_RTOS_CMD_SSP_UNIT_TEST_BLOCK_MODE_INIT	0x09
#define CVIAUDIO_RTOS_CMD_SSP_UNIT_TEST_BLOCK_MODE_GET	0x0A



#endif	/* __CVIAUDIO_IOCTL_CMD_H__ */
