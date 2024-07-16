/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cv183x_pwm.c
 * Description: cvitek pwm driver
 */

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/of.h>
#include <linux/version.h>

/* CVITEK RTC registers */
#define CVI_RTC_SET_SEC_CNTR_VALUE		0x10
#define CVI_RTC_SET_SEC_CNTR_TRIG		0x14
#define CVI_RTC_SEC_CNTR_VALUE		0x18

#define CVI_RTC_ANA_CALIB				0x0
#define CVI_RTC_SEC_PULSE_GEN			0x4
#define CVI_RTC_POR_DB_MAGIC_KEY		0x68
#define CVI_RTC_PWR_DET_SEL				0x140
#define CVI_RTC_PWR_DET_COMP			0x144

#define CVI_RTC_FC_COARSE_EN			0x40
#define CVI_RTC_FC_COARSE_CAL			0x44
#define CVI_RTC_FC_FINE_EN				0x48
#define CVI_RTC_FC_FINE_CAL				0x50

struct cvi_rtc_info {
	struct platform_device	*pdev;
	struct rtc_device	*rtc_dev;
	void __iomem		*rtc_base; /* NULL if not initialized. */
	void __iomem		*rtc_ctrl_base; /* NULL if not initialized. */
	struct clk		*clk;
	int			cvi_rtc_irq; /* alarm and periodic irq */
	spinlock_t		cvi_rtc_lock;
};

static int cvi_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct cvi_rtc_info *info = dev_get_drvdata(dev);
	unsigned long sec;
	unsigned long sl_irq_flags;

	spin_lock_irqsave(&info->cvi_rtc_lock, sl_irq_flags);

	sec = readl(info->rtc_base + CVI_RTC_SEC_CNTR_VALUE);

	spin_unlock_irqrestore(&info->cvi_rtc_lock, sl_irq_flags);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	rtc_time64_to_tm(sec, tm);
#else
	rtc_time_to_tm(sec, tm);
#endif
	dev_vdbg(dev, "time read as %lu. %d/%d/%d %d:%02u:%02u\n",
		sec,
		tm->tm_mon + 1,
		tm->tm_mday,
		tm->tm_year + 1900,
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec
	);

	return 0;
}

static int cvi_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct cvi_rtc_info *info = dev_get_drvdata(dev);
	unsigned long sec;
	int ret;
	unsigned long sl_irq_flags;

	/* convert tm to seconds. */
	ret = rtc_valid_tm(tm);
	if (ret)
		return ret;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	sec = rtc_tm_to_time64(tm);
#else
	rtc_tm_to_time(tm, &sec);
#endif
	dev_vdbg(dev, "cvi_rtc_set_time %lu\n", sec);

	dev_vdbg(dev, "time set to %lu. %d/%d/%d %d:%02u:%02u\n",
		sec,
		tm->tm_mon+1,
		tm->tm_mday,
		tm->tm_year+1900,
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec
	);

	spin_lock_irqsave(&info->cvi_rtc_lock, sl_irq_flags);

	writel(sec, info->rtc_base + CVI_RTC_SET_SEC_CNTR_VALUE);
	writel(1, info->rtc_base + CVI_RTC_SET_SEC_CNTR_TRIG);

	spin_unlock_irqrestore(&info->cvi_rtc_lock, sl_irq_flags);

	return ret;
}

static int cvi_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	return 0;
}

static int cvi_rtc_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	return 0;
}

static int cvi_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	return 0;
}

static int cvi_rtc_proc(struct device *dev, struct seq_file *seq)
{
	if (!dev || !dev->driver)
		return 0;

	seq_printf(seq, "name\t\t: %s\n", dev_name(dev));

	return 0;
}

static irqreturn_t cvi_rtc_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static const struct rtc_class_ops cvi_rtc_ops = {
	.read_time	= cvi_rtc_read_time,
	.set_time	= cvi_rtc_set_time,
	.read_alarm	= cvi_rtc_read_alarm,
	.set_alarm	= cvi_rtc_set_alarm,
	.proc		= cvi_rtc_proc,
	.alarm_irq_enable = cvi_rtc_alarm_irq_enable,
};

static const struct of_device_id cvi_rtc_dt_match[] = {
	{ .compatible = "cvitek,rtc", },
	{}
};
MODULE_DEVICE_TABLE(of, cvi_rtc_dt_match);

static void rtc_32k_coarse_value_calib(struct cvi_rtc_info *info)
{
	uint32_t analog_calib_value = 0;
	uint32_t fc_coarse_time1 = 0;
	uint32_t fc_coarse_time2 = 0;
	uint32_t fc_coarse_value = 0;
	uint32_t offset = 128;

	writel(0x10100, info->rtc_base + CVI_RTC_ANA_CALIB);
	udelay(200);

	analog_calib_value = readl(info->rtc_base + CVI_RTC_ANA_CALIB);
	// dev_notice(NULL, "RTC_ANA_CALIB: 0x%x\n", analog_calib_value);

	writel(1, info->rtc_ctrl_base + CVI_RTC_FC_COARSE_EN);

	while (1) {
		fc_coarse_time1 = readl(info->rtc_ctrl_base + CVI_RTC_FC_COARSE_CAL);
		fc_coarse_time1 >>= 16;
		// dev_notice(NULL, "fc_coarse_time1 = 0x%x\n", fc_coarse_time1);
		// dev_notice(NULL, "fc_coarse_time2 = 0x%x\n", fc_coarse_time2);

		while (fc_coarse_time2 <= fc_coarse_time1) {
			fc_coarse_time2 = readl(info->rtc_ctrl_base + CVI_RTC_FC_COARSE_CAL);
			fc_coarse_time2 >>= 16;
			// dev_notice(NULL, "fc_coarse_time2 = 0x%x\n", fc_coarse_time2);
		}

		udelay(400);
		fc_coarse_value = readl(info->rtc_ctrl_base + CVI_RTC_FC_COARSE_CAL);
		fc_coarse_value &= 0xFFFF;
		// dev_notice(NULL, "fc_coarse_value = 0x%x\n", fc_coarse_value);

		if (fc_coarse_value > 770) {
			analog_calib_value += offset;
			offset >>= 1;
			writel(analog_calib_value, info->rtc_base + CVI_RTC_ANA_CALIB);
		} else if (fc_coarse_value < 755) {
			analog_calib_value -= offset;
			offset >>= 1;
			writel(analog_calib_value, info->rtc_base + CVI_RTC_ANA_CALIB);
		} else {
			writel(0, info->rtc_ctrl_base + CVI_RTC_FC_COARSE_EN);
			// dev_notice(NULL, "RTC coarse calib done\n");
			break;
		}
		if (offset == 0) {
			dev_err(NULL, "RTC calib failed\n");
			break;
		}
		// dev_notice(NULL, "RTC_ANA_CALIB: 0x%x\n", analog_calib_value);
	}
}

static void rtc_32k_fine_value_calib(struct cvi_rtc_info *info)
{
	uint32_t fc_fine_time1 = 0;
	uint32_t fc_fine_time2 = 0;
	uint32_t fc_fine_value = 0;
	uint64_t freq;
	uint32_t sec_cnt;
	uint32_t frac_ext = 10000;

	writel(1, info->rtc_ctrl_base + CVI_RTC_FC_FINE_EN);

	fc_fine_time1 = readl(info->rtc_ctrl_base + CVI_RTC_FC_FINE_CAL);
	fc_fine_time1 >>= 24;
	// dev_notice(NULL, "fc_fine_time1 = 0x%x\n", fc_fine_time1);

	while (fc_fine_time2 <= fc_fine_time1) {
		fc_fine_time2 = readl(info->rtc_ctrl_base + CVI_RTC_FC_FINE_CAL);
		fc_fine_time2 >>= 24;
		// dev_notice(NULL, "fc_fine_time2 = 0x%x\n", fc_fine_time2);
	}

	fc_fine_value = readl(info->rtc_ctrl_base + CVI_RTC_FC_FINE_CAL);
	fc_fine_value &= 0xFFFFFF;
	// dev_notice(NULL, "fc_fine_value = 0x%x\n", fc_fine_value);

	// Frequency = 256 / (RTC_FC_FINE_VALUE x 40ns)
	freq = 256000000000 / 40;
	freq = (freq * frac_ext) / fc_fine_value;
	// dev_notice(NULL, "freq = %u\n", freq);

	sec_cnt = ((freq / frac_ext) << 8) + (((freq % frac_ext) * 256) / frac_ext & 0xFF);
	// dev_notice(NULL, "sec_cnt = 0x%x\n", sec_cnt);

	writel(sec_cnt, info->rtc_base + CVI_RTC_SEC_PULSE_GEN);
	writel(0, info->rtc_ctrl_base + CVI_RTC_FC_FINE_EN);
}

static int __init cvi_rtc_probe(struct platform_device *pdev)
{
	struct cvi_rtc_info *info;
	struct resource *res;
	int ret;

	info = devm_kzalloc(&pdev->dev, sizeof(struct cvi_rtc_info),
		GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	info->rtc_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(info->rtc_base))
		return PTR_ERR(info->rtc_base);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	info->rtc_ctrl_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(info->rtc_ctrl_base))
		return PTR_ERR(info->rtc_ctrl_base);

	info->cvi_rtc_irq = platform_get_irq(pdev, 0);
	if (info->cvi_rtc_irq <= 0)
		return -EBUSY;

	info->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(info->clk))
		return PTR_ERR(info->clk);

	ret = clk_prepare_enable(info->clk);
	if (ret < 0)
		return ret;

	/* set context info. */
	info->pdev = pdev;
	spin_lock_init(&info->cvi_rtc_lock);

	platform_set_drvdata(pdev, info);

	device_init_wakeup(&pdev->dev, 1);

	info->rtc_dev = devm_rtc_device_register(&pdev->dev,
				dev_name(&pdev->dev), &cvi_rtc_ops,
				THIS_MODULE);
	if (IS_ERR(info->rtc_dev)) {
		ret = PTR_ERR(info->rtc_dev);
		dev_err(&pdev->dev, "Unable to register device (err=%d).\n",
			ret);
		goto disable_clk;
	}

	ret = devm_request_irq(&pdev->dev, info->cvi_rtc_irq,
			cvi_rtc_irq_handler, IRQF_TRIGGER_HIGH,
			dev_name(&pdev->dev), &pdev->dev);
	if (ret) {
		dev_err(&pdev->dev,
			"Unable to request interrupt for device (err=%d).\n",
			ret);
		goto disable_clk;
	}

	rtc_32k_coarse_value_calib(info);
	rtc_32k_fine_value_calib(info);

	dev_notice(&pdev->dev, "CVITEK real time clock\n");

	return 0;

disable_clk:
	clk_disable_unprepare(info->clk);
	return ret;
}

static int cvi_rtc_remove(struct platform_device *pdev)
{
	struct cvi_rtc_info *info = platform_get_drvdata(pdev);

	clk_disable_unprepare(info->clk);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int cvi_rtc_suspend(struct device *dev)
{
	return 0;
}

static int cvi_rtc_resume(struct device *dev)
{
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(cvi_rtc_pm_ops, cvi_rtc_suspend, cvi_rtc_resume);

static void cvi_rtc_shutdown(struct platform_device *pdev)
{
	dev_vdbg(&pdev->dev, "disabling interrupts.\n");
	cvi_rtc_alarm_irq_enable(&pdev->dev, 0);
}

MODULE_ALIAS("platform:cvi_rtc");
static struct platform_driver cvi_rtc_driver = {
	.remove		= cvi_rtc_remove,
	.shutdown	= cvi_rtc_shutdown,
	.driver		= {
		.name	= "cvi_rtc",
		.of_match_table = cvi_rtc_dt_match,
		.pm	= &cvi_rtc_pm_ops,
	},
};

module_platform_driver_probe(cvi_rtc_driver, cvi_rtc_probe);

MODULE_AUTHOR("Mark Hsieh <mark.hsieh@cvitek.com>");
MODULE_DESCRIPTION("driver for CVITEK RTC");
MODULE_LICENSE("GPL");
