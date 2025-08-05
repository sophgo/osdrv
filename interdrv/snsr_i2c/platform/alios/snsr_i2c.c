#include <drv/iic.h>

#include "osal.h"
#include "vi_snsr.h"
#include "snsr_i2c.h"


enum {
	SNSR_I2C_DBG_DISABLE = 0,
	SNSR_I2C_DBG_VERIFY,
	SNSR_I2C_DBG_PRINT,
};

static struct i2c_dev *g_i2c_dev;

static int snsr_i2c_write(struct i2c_dev *dev, struct isp_i2c_data *i2c)
{
	csi_iic_t *master_iic = NULL;
	csi_iic_mem_addr_size_t reg_addr_len;
	u8 i2c_dev = i2c->i2c_dev;
	u8 tx[4];
	int ret = 0, idx = 0;

	if (!dev)
		return OSAL_ENODEV;

	/* check i2c device number. */
	if (i2c_dev >= I2C_MAX_NUM)
		return OSAL_EINVAL;

	/* Get i2c client */
	master_iic = dev->ctx[i2c_dev].client;

	reg_addr_len = (i2c->addr_bytes == 0x2) ? IIC_MEM_ADDR_SIZE_16BIT : IIC_MEM_ADDR_SIZE_8BIT;

	/* Config data */
	if (i2c->data_bytes == 1) {
		tx[idx++] = i2c->data & 0xff;
	} else {
		tx[idx++] = i2c->data >> 8;
		tx[idx++] = i2c->data & 0xff;
	}

	ret = csi_iic_mem_send(master_iic, i2c->dev_addr, i2c->reg_addr,
				reg_addr_len, tx, i2c->data_bytes, 1000);
	if (ret != i2c->data_bytes) {
		osal_printk("I2C_WRITE error %d!\n", ret);
		return OSAL_EINVAL;
	}

	return OSAL_SUCCESS;
}

//TODO maybe need to implement this function
#ifdef BURST_MODE

static int snsr_i2c_burst_queue(struct i2c_dev *dev, struct isp_i2c_data *i2c)
{
	u8 i2c_dev = i2c->i2c_dev;
	struct i2c_ctx *ctx;
	int idx = 0;
	u8 *tx;

	if (!dev)
		return OSAL_ENODEV;

	/* check i2c device number. */
	if (i2c_dev >= I2C_MAX_NUM)
		return OSAL_EINVAL;

	/* Get i2c client */
	ctx = &dev->ctx[i2c_dev];
	if (ctx->msg_idx >= I2C_MAX_MSG_NUM)
		return OSAL_EINVAL;

	tx = &ctx->buf[4 * ctx->msg_idx];

	/* Config data */
	if (i2c->data_bytes == 1) {
		tx[idx++] = i2c->data & 0xff;
	} else {
		tx[idx++] = i2c->data >> 8;
		tx[idx++] = i2c->data & 0xff;
	}

	/* Config msg */
	ctx->addr_bytes = i2c->addr_bytes;
	ctx->data_bytes = i2c->data_bytes;
	ctx->msg_idx++;

	return 0;
}

static void snsr_i2c_verify(struct i2c_client *client, struct i2c_ctx *ctx, uint32_t size)
{
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	int i, step, ret;
	u8 tx[4];

	msg.addr = ctx->msg[0].addr;
	msg.buf = tx;

	/*
	 * In burst mode we have no idea when the transfer completes,
	 * therefore we wait for 1ms before verifying the results in debug mode.
	 */
	usleep_range(1000, 2000);
	step = ctx->addr_bytes + ctx->data_bytes;
	for (i = 0; i < size; i++) {
		if (step == 2) {
			/* 1 byte address, 1 byte data*/
			tx[0] = ctx->msg[i].buf[0];
			msg.buf = tx;
			msg.len = 1;
			msg.flags = 0;
			ret = i2c_transfer(adap, &msg, 1);
			if (ret != 1) {
				osal_printk("%s, i2c xfer ng\n", __func__);
				break;
			}
			tx[0] = 0;
			msg.flags = I2C_M_RD;
			ret = i2c_transfer(adap, &msg, 1);
			if (ret != 1) {
				osal_printk("%s, i2c xfer ng\n", __func__);
				break;
			}
			if (ctx->msg[i].buf[1] != tx[0]) {
				osal_printk("%s, addr 0x%02x, w: 0x%02x, r: 0x%02x\n",
						__func__, ctx->msg[i].buf[0], ctx->msg[i].buf[1], tx[0]);
			}
		} else if (step == 3) {
			/* 2 byte address, 1 byte data*/
			tx[0] = ctx->msg[i].buf[0];
			tx[1] = ctx->msg[i].buf[1];
			msg.buf = tx;
			msg.len = 2;
			msg.flags = 0;
			ret = i2c_transfer(adap, &msg, 1);
			if (ret != 1) {
				osal_printk("%s, i2c xfer ng\n", __func__);
				break;
			}
			tx[0] = 0;
			msg.len = 1;
			msg.flags = I2C_M_RD;
			ret = i2c_transfer(adap, &msg, 1);
			if (ret != 1) {
				osal_printk("%s, i2c xfer ng\n", __func__);
				break;
			}
			if (ctx->msg[i].buf[2] != tx[0]) {
				osal_printk("%s, addr 0x%02x,0x%02x w: 0x%02x, r: 0x%02x\n",
						__func__, ctx->msg[i].buf[0], ctx->msg[i].buf[1],
						ctx->msg[i].buf[2], tx[0]);
			}
		} else {
			/* 2 byte address, 2 byte data*/
			tx[0] = ctx->msg[i].buf[0];
			tx[1] = ctx->msg[i].buf[1];
			msg.buf = tx;
			msg.len = 2;
			msg.flags = 0;
			ret = i2c_transfer(adap, &msg, 1);
			if (ret != 1) {
				osal_printk("%s, i2c xfer ng\n", __func__);
				break;
			}
			tx[0] = 0;
			tx[1] = 0;
			msg.len = 2;
			msg.flags = I2C_M_RD;
			ret = i2c_transfer(adap, &msg, 1);
			if (ret != 1) {
				osal_printk("%s, i2c xfer ng\n", __func__);
				break;
			}
			if ((ctx->msg[i].buf[2] != tx[0]) || (ctx->msg[i].buf[3] != tx[1])) {
				osal_printk("%s, addr 0x%02x 0x%02x, w: 0x%02x 0x%02x, r: 0x%02x 0x%02x\n",
						__func__, ctx->msg[i].buf[0], ctx->msg[i].buf[1],
						ctx->msg[i].buf[2], ctx->msg[i].buf[3],
						tx[0], tx[1]);
			}
		}
	}
}

static void snsr_i2c_print(struct i2c_ctx *ctx, uint32_t size)
{
	int i, step;

	step = ctx->addr_bytes + ctx->data_bytes;

	for (i = 0; i < size; i++) {
		if (step == 2)
			osal_printk("a: 0x%02x, d: 0x%02x",
					ctx->msg[i].buf[0], ctx->msg[i].buf[1]);
		else if (step == 3)
			osal_printk("a: 0x%02x, 0x%02x, d: 0x%02x",
					ctx->msg[i].buf[0], ctx->msg[i].buf[1], ctx->msg[i].buf[2]);
		else
			osal_printk("a: 0x%02x, 0x%02x, d: 0x%02x, 0x%02x",
					ctx->msg[i].buf[0], ctx->msg[i].buf[1],
					ctx->msg[i].buf[2], ctx->msg[i].buf[3]);
	}
}

static int snsr_i2c_burst_fire(struct i2c_dev *dev, uint32_t i2c_dev)
{
	csi_iic_t *master_iic;
	struct i2c_adapter *adap;
	struct i2c_ctx *ctx;
	osal_timeval tv;
	uint64_t t1, t2;
	int ret, retry = 5;

	if (!dev)
		return OSAL_ENODEV;

	/* check i2c device number. */
	if (i2c_dev >= I2C_MAX_NUM)
		return OSAL_EINVAL;

	/* Get i2c client */
	ctx = &dev->ctx[i2c_dev];
	if (!ctx->msg_idx)
		return 0;

	master_iic = ctx->client;

	while (retry--) {
		osal_gettimeofday(&tv);
		t1 = ts.tv_sec * 1000000 + ts.tv_usec;

		ret = i2c_transfer(adap, ctx->msg, ctx->msg_idx);
		if (ret == ctx->msg_idx) {

			osal_gettimeofday(&tv);
			t2 = ts.tv_sec * 1000000 + ts.tv_usec;

			osal_printk("burst [%d] success %lld us\n", ret, (t2 - t1));
			if (snsr_i2c_dbg == SNSR_I2C_DBG_VERIFY)
				snsr_i2c_verify(client, ctx, ret);
			else if (snsr_i2c_dbg == SNSR_I2C_DBG_PRINT)
				snsr_i2c_print(ctx, ret);
			break;
		} else if (ret == OSAL_EAGAIN) {
			osal_printk("retry\n");
		} else {
			osal_printk("fail to send burst\n");
		}
	}

	ctx->msg_idx = 0;

	return ret == 1 ? 0 : OSAL_EIO;
}

#endif

static long snsr_i2c_ioctl(void *hdlr, unsigned int cmd, void *arg)
{
	struct i2c_dev *dev = (struct i2c_dev *)hdlr;
	//uint32_t *argp = (uint32_t *)arg;
	//uint32_t i2c_dev;

	switch (cmd) {
	case CVI_SNS_I2C_WRITE:
		return snsr_i2c_write(dev, (struct isp_i2c_data *)arg);
	//case CVI_SNS_I2C_BURST_QUEUE:
	//	return snsr_i2c_burst_queue(dev, (struct isp_i2c_data *)arg);
	//case CVI_SNS_I2C_BURST_FIRE:
	//	i2c_dev = *argp;
	//	return snsr_i2c_burst_fire(dev, i2c_dev);
	default:
		return OSAL_ENOIOCTLCMD;
	}

	return 0;
}

static int i2c_init(struct i2c_ctx *ctx, u8 i2c_id)
{
	s32 ret = CSI_OK;

	ctx->client = (csi_iic_t *)osal_zalloc(sizeof(csi_iic_t));
	if (!ctx->client)
		return OSAL_ENOMEM;

	ret = csi_iic_init(ctx->client, i2c_id);
	if (ret != CSI_OK) {
		osal_printk("csi_iic_initialize error\n");
		goto free_master;
	}

	/* config iic master mode */
	ret = csi_iic_mode(ctx->client, IIC_MODE_MASTER);
	if (ret != CSI_OK) {
		osal_printk("csi_iic_set_mode error\n");
		goto i2c_deinit;
	}

	/* config iic 7bit address mode */
	ret = csi_iic_addr_mode(ctx->client, IIC_ADDRESS_7BIT);
	if (ret != CSI_OK) {
		osal_printk("csi_iic_set_addr_mode error\n");
		goto i2c_deinit;
	}

	/* config iic standard speed*/
	ret = csi_iic_speed(ctx->client, IIC_BUS_SPEED_FAST);
	if (ret != CSI_OK) {
		osal_printk("csi_iic_set_speed error\n");
		goto i2c_deinit;
	}

	return ret;

i2c_deinit:
	csi_iic_uninit(ctx->client);

free_master:
	if (ctx->client) {
		osal_free(ctx->client);
		ctx->client = NULL;
	}

	return ret;
}

static int i2c_uninit(struct i2c_ctx *ctx)
{
	csi_iic_uninit(ctx->client);
	osal_free(ctx->client);
	ctx->client = NULL;

	return 0;
}

static int _init_resource(struct i2c_dev *dev)
{
	int rc = 0;
	struct i2c_ctx *ctx = NULL;
	int i = 0;

	if (!dev)
		return OSAL_ENODEV;

	for (i = 0; i < I2C_MAX_NUM; i++) {
		ctx = &dev->ctx[i];
		if (!ctx->client) {
			rc = i2c_init(ctx, i);
			if (rc < 0)
				continue;
			ctx->buf = osal_malloc(I2C_BUF_SIZE);
			if (!ctx->buf)
				return OSAL_ENOMEM;
		} else
			osal_printf("duplicate i2c_adpa idx %d\n", i);
	}

	return 0;
}

int snsr_i2c_probe(void)
{
	int rc = 0;
	struct i2c_dev *dev;

	/* allocate main snsr state structure */
	g_i2c_dev = osal_zalloc(sizeof(*dev));
	if (!g_i2c_dev) {
		return OSAL_ENOMEM;
	}

	dev = g_i2c_dev;

	/* initialize locks */
	osal_spin_lock_init(&dev->lock);
	osal_mutex_init(&dev->mutex);

	rc = vi_sys_register_cmm_cb(0, dev, snsr_i2c_ioctl);
	if (rc < 0) {
		osal_printk("Failed to register cmm for snsr, %d\n", rc);
		return rc;
	}

	rc = _init_resource(dev);
	if (rc < 0) {
		osal_printk("Failed to init res for snsr, %d\n", rc);
		return rc;
	}

	return 0;
}

static int snsr_i2c_remove(void)
{
	struct i2c_dev *dev = g_i2c_dev;
	int i = 0;

	if (!dev) {
		osal_printk("invalid param");
		return OSAL_EINVAL;
	}

	osal_spin_lock_destroy(&dev->lock);
	osal_mutex_destroy(&dev->mutex);

	for (i = 0; i < I2C_MAX_NUM; i++) {
		struct i2c_ctx *ctx;

		ctx = &dev->ctx[i];
		if (ctx->client)
			i2c_uninit(ctx->client);
		osal_free(ctx->buf);
		ctx->buf = NULL;
	}

	osal_free(dev);
	g_i2c_dev = NULL;

	return 0;
}

int driver_snsr_i2c_init(void)
{
	return snsr_i2c_probe();
}

void driver_snsr_i2c_exit(void)
{
	snsr_i2c_remove();
}
