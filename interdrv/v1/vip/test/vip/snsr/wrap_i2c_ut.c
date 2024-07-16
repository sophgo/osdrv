#include <ut_common.h>
#include <linux/cvi_vip.h>
#include <linux/cvi_vip_snsr.h>
#include <linux/cvi_vip_cif.h>
#include <linux/v4l2-subdev.h>
#include "../common/vip_msg_com.h"
#include <unistd.h>
#include "wrap_i2c_ut.h"
#include <errno.h>

const char *dev_wrap_i2c = "/dev/sensor-i2c";
int ut_fd_wrap_i2c;

/*************************************************************
 *	Private	functions
 *************************************************************/

static enum cvi_errno wrap_i2c_manual(void *priv)
{
	enum cvi_errno ret = ERR_NONE;
	struct isp_i2c_data i2c;
	int cmd, run = 1;
	char str[20];

	while (run) {
		ut_pr(UT_INFO, "0-Run I2C write test\n");
		ut_pr(UT_INFO, "255-Exit\n");
		scanf("%d", (int *)&cmd);

		if (cmd == 255)
			break;

		ut_pr(UT_INFO, "input i2c dev\n");
		scanf("%d", (int *)&cmd);
		i2c.i2c_dev = cmd;
		ut_pr(UT_INFO, "input dev addr\n");
		scanf("%x", (unsigned int *)&cmd);
		i2c.dev_addr = cmd;
		ut_pr(UT_INFO, "input reg addr\n");
		scanf("%x", (unsigned int *)&cmd);
		i2c.reg_addr = cmd;
		ut_pr(UT_INFO, "input data\n");
		scanf("%x", (unsigned int *)&cmd);
		i2c.data = cmd;
		ut_pr(UT_INFO, "input addr btyes\n");
		scanf("%d", (int *)&cmd);
		i2c.addr_bytes = cmd;
		ut_pr(UT_INFO, "input data btyes\n");
		scanf("%d", (int *)&cmd);
		i2c.data_bytes = cmd;
		if (ioctl(ut_fd_wrap_i2c, CVI_SNS_I2C_WRITE, (void *)&i2c) < 0)
			ret = ERR_IOCTL;
	}

	return ret;

}

/*************************************************************
 *	Public	functions
 *************************************************************/
enum cvi_errno wrap_i2c_init(void *priv)
{
	enum cvi_errno ret = ERR_NONE;

	UT_DEV_INIT(dev_wrap_i2c, ut_fd_wrap_i2c);

	return ret;
}

enum cvi_errno wrap_i2c_config(void *priv)
{
	enum cvi_errno ret = ERR_NONE;

	return ret;
}

enum cvi_errno wrap_i2c_start(void *priv)
{
	const struct inparam *param = (const struct inparam *)priv;
	enum cvi_errno ret = ERR_NONE;

	if (param->manual_mode)
		wrap_i2c_manual(priv);

	return ret;
}

enum cvi_errno wrap_i2c_stop(void *priv)
{
	enum cvi_errno ret = ERR_NONE;

	return ret;
}

enum cvi_errno wrap_i2c_msgrcv(void *priv)
{
	enum cvi_errno ret = ERR_NONE;
	struct msgpack *msg = (struct msgpack *)priv;
	struct wrap_i2c_ctx *msg_ctx = (struct wrap_i2c_ctx *)msg->msg_ctx;

	ut_pr(UT_INFO, "wrap_i2c rev frm src(%s)\n",
			msg->src_name);

	switch (msg_ctx->cmd) {
	case I2C_WRITE:
		if (!ut_fd_wrap_i2c)
			UT_DEV_INIT(dev_wrap_i2c, ut_fd_wrap_i2c);
		ut_pr(UT_INFO, "call ioctl\n");
		if (ioctl(ut_fd_wrap_i2c, CVI_SNS_I2C_WRITE, msg_ctx->indata) < 0) {
			ut_pr(UT_INFO, "errno %d\n", errno);
			ret = ERR_IOCTL;
		}
		break;
	default:
		break;
	}

	return ret;
}

/*************************************************************
 *	Instance	Creation
 *************************************************************/
MODULE_DECL("wrap_i2c", wrap_i2c);

struct module_op *wrap_i2c_get_instance(void)
{
	return &wrap_i2c_op;
}

