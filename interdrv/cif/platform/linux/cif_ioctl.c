#include <linux/types.h>
#include <linux/delay.h>
#include "cif_ioctl.h"
#include "cif_l.h"
#include "cif.h"

struct link *ctx_to_link(const struct cif_ctx *ctx)
{
	return container_of(ctx, struct link, cif_ctx);
}

struct cif_dev *file_cif_dev(struct file *file)
{
	return container_of(file->private_data, struct cif_dev, miscdev);
}

static long _cif_ioctl(struct cif_dev *dev, unsigned int cmd,
		       unsigned long arg, unsigned int from_user)
{
	struct device *_dev = dev->miscdev.this_device;
	struct cif_ctx *ctx = NULL;
	uint32_t devno;

	dev_dbg(_dev, "%s\n", _to_string_cmd(cmd));

	if (arg == 0) {
		dev_err(_dev, "null pointer\n");
		return -EINVAL;
	}

	switch (cmd) {
	case MIPI_SET_DEV_ATTR:
	{
		struct combo_dev_attr_s attr;

		if (from_user) {
			if (copy_from_user(&attr, (void *)arg, sizeof(attr))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&attr, (void *)arg, sizeof(attr));

		return cif_set_dev_attr(dev, &attr);
	}
	case MIPI_SET_OUTPUT_CLK_EDGE:
	{
		struct clk_edge_s clk;

		if (from_user) {
			if (copy_from_user(&clk, (void *)arg, sizeof(clk))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&clk, (void *)arg, sizeof(clk));

		return cif_set_output_clk_edge(dev, &clk);
	}
	case MIPI_RESET_MIPI:
		if (from_user) {
			if (copy_from_user(&devno, (void *)arg, sizeof(devno))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			devno = *(uint32_t *)arg;

		return cif_reset_mipi(dev, devno);
	case MIPI_SET_CROP_TOP: // remove info line
	{
		struct crop_top_s crop;

		if (from_user) {
			if (copy_from_user(&crop, (void *)arg, sizeof(crop))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&crop, (void *)arg, sizeof(crop));

		return cif_set_crop_top(dev, &crop);
	}
	case MIPI_SET_CROP_WINDOW: // crop input image
	{
		struct cif_crop_win_s win;

		if (from_user) {
			if (copy_from_user(&win, (void *)arg, sizeof(win))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&win, (void *)arg, sizeof(win));

		return cif_set_windowing(dev, &win);
	}
	case MIPI_SET_WDR_MANUAL:
	{
		struct manual_wdr_s wdr_manu;

		if (from_user) {
			if (copy_from_user(&wdr_manu, (void *)arg, sizeof(wdr_manu))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&wdr_manu, (void *)arg, sizeof(wdr_manu));

		return cif_set_wdr_manual(dev, &wdr_manu);
	}
	case MIPI_SET_LVDS_FP_VS:
	{
		struct vsync_gen_s vsync;

		if (from_user) {
			if (copy_from_user(&vsync, (void *)arg, sizeof(vsync))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&vsync, (void *)arg, sizeof(vsync));

		return cif_set_lvds_fp_vs(dev, &vsync);
	}
	case MIPI_RESET_SENSOR:
	{
		sns_rst_config rst_config;

		if (from_user) {
			if (copy_from_user(&rst_config, (void *)arg, sizeof(rst_config))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&rst_config, (void *)arg, sizeof(rst_config));
		return cif_reset_snsr_gpio(dev, &rst_config, 1);
	}
	case MIPI_UNRESET_SENSOR:
	{
		sns_rst_config rst_config;

		if (from_user) {
			if (copy_from_user(&rst_config, (void *)arg, sizeof(rst_config))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&rst_config, (void *)arg, sizeof(rst_config));
		return cif_reset_snsr_gpio(dev, &rst_config, 0);
	}
	case MIPI_ENABLE_SENSOR_CLOCK:
		if (from_user) {
			if (copy_from_user(&devno, (void *)arg, sizeof(devno))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			devno = *(uint32_t *)arg;
		return cif_enable_snsr_clk(dev, devno, 1);
	case MIPI_DISABLE_SENSOR_CLOCK:
		if (from_user) {
			if (copy_from_user(&devno, (void *)arg, sizeof(devno))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			devno = *(uint32_t *)arg;
		return cif_enable_snsr_clk(dev, devno, 0);
	case MIPI_RESET_LVDS:
	case CIF_CB_RESET_LVDS:
		if (from_user) {
			if (copy_from_user(&devno, (void *)arg, sizeof(devno))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			devno = *(uint32_t *)arg;
		return cif_reset_lvds(dev, devno);
	case MIPI_GET_CIF_ATTR:
	case CIF_CB_GET_CIF_ATTR:
	{
		struct cif_attr_s cif_attr;

		if (from_user) {
			if (copy_from_user(&cif_attr, (void *)arg, sizeof(cif_attr))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&cif_attr, (void *)arg, sizeof(cif_attr));

		cif_get_cif_attr(dev, &cif_attr);

		if (from_user) {
			if (copy_to_user((void *)arg, &cif_attr, sizeof(cif_attr))) {
				dev_err(_dev, "copy_to_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy((void *)arg, &cif_attr, sizeof(cif_attr));
		return 0;
	}
	case MIPI_SET_BT_FMT_OUT:
	{
		struct bt_fmt_out_s bt_fmt;

		if (from_user) {
			if (copy_from_user(&bt_fmt, (void *)arg, sizeof(bt_fmt))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&bt_fmt, (void *)arg, sizeof(bt_fmt));

		return cif_bt_fmt_out(dev, &bt_fmt);
	}
	case MIPI_SET_SENSOR_CLOCK:
	{
		struct mclk_pll_s mclk;

		if (from_user) {
			if (copy_from_user(&mclk, (void *)arg, sizeof(mclk))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&mclk, (void *)arg, sizeof(mclk));

		/* sensor enable */
		if (mclk.cam >= MAX_LINK_NUM)
			return -EINVAL;
		if (mclk.cam < MAX_CAM_CLK_NUM) {
			snsr_mclk[mclk.cam] = mclk.freq;
		} else {
			dev_err(_dev, "cam clk out of max clk list.\n");
		}

		return _cif_enable_snsr_clk(_dev, dev, mclk.cam, mclk.freq != CAMPLL_FREQ_NONE);
	}
	case MIPI_SET_MAX_MAC_CLOCK:
		if (from_user) {
			if (copy_from_user(&dev->max_mac_clk, (void *)arg, sizeof(u32))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			dev->max_mac_clk = *(uint32_t *)arg;

		if (dev->max_mac_clk <= 400)
			dev->max_mac_clk = 396;
		else if (dev->max_mac_clk <= 500)
			dev->max_mac_clk = 500;
		else
			dev->max_mac_clk = 594;
		break;
	case MIPI_SET_YUV_SWAP:
	{
		struct cif_yuv_swap_s swap;

		if (from_user) {
			if (copy_from_user(&swap, (void *)arg, sizeof(swap))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&swap, (void *)arg, sizeof(swap));

		ctx = &dev->link[swap.devno].cif_ctx;

		return cif_swap_yuv(ctx, swap.uv_swap, swap.yc_swap);
	}
	default:
		return -ENOIOCTLCMD;
	}
	return 0;
}

long cif_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct cif_dev *dev = file_cif_dev(file);

	return _cif_ioctl(dev, cmd, arg, 1);
}

#ifdef CONFIG_COMPAT
long cif_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static int cif_cb(void *dev, enum _cb_modules_id caller, u32 cmd, void *arg)
{
	return _cif_ioctl((struct cif_dev *)dev, cmd, (unsigned long)arg, 0);
}

int cif_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_CIF);
}

int cif_register_cb(struct cif_dev *dev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_CIF;
	reg_cb.dev		= (void *)dev;
	reg_cb.cb		= cif_cb;

	return base_reg_module_cb(&reg_cb);
}