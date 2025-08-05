#ifndef _SNSR_CB_H_
#define _SNSR_CB_H_

#include "vi_snsr.h"
#include "vi_sys.h"
#include "osal_ioctl.h"

#define I2C_MAX_NUM		5
#define I2C_MAX_MSG_NUM		32
#define I2C_BUF_SIZE		(I2C_MAX_MSG_NUM << 2)

#define CVI_SNS_I2C_IOC_MAGIC	'i'
#define CVI_SNS_I2C_WRITE	_IOWR(CVI_SNS_I2C_IOC_MAGIC, 2, \
					struct isp_i2c_data)
#define CVI_SNS_I2C_BURST_QUEUE	_IOWR(CVI_SNS_I2C_IOC_MAGIC, 3, \
					struct isp_i2c_data)
#define CVI_SNS_I2C_BURST_FIRE	_IOWR(CVI_SNS_I2C_IOC_MAGIC, 4, \
					unsigned int)
#endif
