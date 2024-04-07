#ifndef __CVIAUDIO_INTERFACE_H__
#define __CVIAUDIO_INTERFACE_H__

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
#include <cvi_comm_aio.h>
#include <cyclebuffer.h>
//#endif
#include <base_cb.h>
#include <cviaudio_rtos_cmd.h>
#ifndef CVIAUDIO_SSP_CHUNK_NUMBERS
#define CVIAUDIO_SSP_CHUNK_NUMBERS 25
#endif

#ifndef CVIAUDIO_MAX_BUFFER_SIZE
#define CVIAUDIO_MAX_BUFFER_SIZE (160 * 2 * CVIAUDIO_SSP_CHUNK_NUMBERS)
#endif

#ifndef DEFAULT_BYTES_PER_SAMPLE
#define DEFAULT_BYTES_PER_SAMPLE 2//16bit = 2 bytes
#endif
//4000bytes 0.5 sec for 8k, 0.25 sec for 16k
typedef struct _ain_chn_mem_ctx {
	uint64_t vqebuff_addr;
	uint64_t cycbufread_addr;
	uint64_t resample_handler_addr;
	int vqebuf_size;
	int cycbufread_size;
	int resample_handler_size;
	//for sharebuffer
	int sharebuf_index;
	int sharebuf_first;
} ST_AIN_CHN_MEM_CTX;

typedef struct _aout_chn_mem_ctx {
	uint64_t resample_handler_addr;
	int resample_handler_size;
	//for sharebuffer
	int sharebuf_index;
	int sharebuf_first;
} ST_AOUT_CHN_MEM_CTX;

typedef struct _adec_chn_mem_ctx {
	uint64_t DecBuff_addr;
	uint64_t DecReadBuff_addr;

	int DecBuff_size;
	int DecReadBuff_size;
	//for sharebuffer
	int sharebuf_index;
	int sharebuf_first;
} ST_ADEC_CHN_MEM_CTX;

typedef struct _ain_mem_ctx {
	uint64_t tinyalsaReadBuf_addr;
	unsigned char pad1;
	unsigned char pad2;
	unsigned char pad3;
	unsigned char pad4;
	int tinyalsaReadBuf_size;
	ST_AIN_CHN_MEM_CTX chn_mem_ctx[CVI_AUD_MAX_CHANNEL_NUM];
} ST_AIN_MEM_CTX;

typedef struct _aout_mem_ctx {
	ST_AOUT_CHN_MEM_CTX chn_mem_ctx[CVI_AUD_MAX_CHANNEL_NUM];
} ST_AOUT_MEM_CTX;

typedef struct _adec_mem_ctx {
	ST_ADEC_CHN_MEM_CTX chn_mem_ctx[CVI_AUD_MAX_CHANNEL_NUM];
} ST_ADEC_MEM_CTX;

typedef struct _aenc_chn_mem_ctx {
	uint64_t EncInBuff_addr;
	uint64_t EncBuff_addr;
	uint64_t EncBuff_aec_addr;
	uint64_t EncVqeBuff_addr;
	int EncInBuff_size;
	int EncBuff_size;
	int EncBuff_aec_size;
	int EncVqeBuff_size;
	int sharebuf_index;
	int sharebuf_first;
} ST_AENC_CHN_MEM_CTX;

typedef struct _aenc_mem_ctx {
	ST_AENC_CHN_MEM_CTX chn_mem_ctx[CVI_AUD_MAX_CHANNEL_NUM];
} ST_AENC_MEM_CTX;

typedef struct _cviaudio_buf_usage_status {
	u8 in_used;
} ST_AUD_USAGE_STATUS;

#if 1
#ifdef CONFIG_SDK_VER_32BIT
#define CVITYPE_PT uint32_t
#define CVIPRT_PT "%x"
#else
#define CVITYPE_PT uint64_t
#define CVIPRT_PT "%llx"
#endif
#else
#define CVITYPE_PT unsigned long
#define CVIPRT_PT "%lx"
#endif

int cviaudio_create_instance(struct platform_device *pdev);
int cviaudio_destroy_instance(struct platform_device *pdev);
int cviaudio_device_open(struct inode *inode, struct file *filp);
long cviaudio_device_ioctl(struct file *filp, u_int cmd, u_long arg);
int cviaudio_device_release(struct inode *inode, struct file *filp);
unsigned int cviaudio_device_poll(struct file *filp, struct poll_table_struct *wait);
int cviaudio_device_mmap(struct file *filp, struct vm_area_struct *vm);
int cviaudio_device_cb(void *dev, enum ENUM_MODULES_ID caller, u32 cmd, void *arg);
#ifdef __cplusplus
}
#endif

#endif /* __CVIAUDIO_INTERFACE_H__ */
