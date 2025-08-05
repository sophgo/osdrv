#ifndef _CVI_SNSR_I2C_H_
#define _CVI_SNSR_I2C_H_

#ifdef __KERNEL__
#include <linux/i2c.h>
#endif

#include "module/osal_spinlock.h"
#include "module/osal_mutex.h"
#include "osal_types.h"
#include "vi_snsr.h"
#include "vi_sys.h"
#include "snsr_cb.h"

#ifndef __KERNEL__
struct i2c_msg {
	__u16 addr;		/* slave address			*/
	__u16 flags;
#define I2C_M_RD		0x0001	/* read data, from slave to master */
					/* I2C_M_RD is guaranteed to be 0x0001! */
#define I2C_M_TEN		0x0010	/* this is a ten bit chip address */
#define I2C_M_DMA_SAFE		0x0200	/* the buffer of this message is DMA safe */
					/* makes only sense in kernelspace */
					/* userspace buffers are copied anyway */
#define I2C_M_RECV_LEN		0x0400	/* length will be first received byte */
#define I2C_M_NO_RD_ACK		0x0800	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_IGNORE_NAK	0x1000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_REV_DIR_ADDR	0x2000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_NOSTART		0x4000	/* if I2C_FUNC_NOSTART */
#define I2C_M_STOP		0x8000	/* if I2C_FUNC_PROTOCOL_MANGLING */
	__u16 len;		/* msg length				*/
	__u8 *buf;		/* pointer to msg data			*/
};
#endif

struct i2c_ctx {
#ifdef __KERNEL__
	struct i2c_client	*client;
#else
	void			*client;
#endif
	struct i2c_msg		msg[I2C_MAX_MSG_NUM];
	uint8_t			*buf;
	uint32_t		msg_idx;
	uint16_t		addr_bytes;
	uint16_t		data_bytes;
};

struct i2c_dev {
	osal_spinlock		lock;
	osal_mutex		mutex;
	struct i2c_ctx	ctx[I2C_MAX_NUM];
};


#endif
