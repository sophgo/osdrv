#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/export.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/overflow.h>

#define REG_HLPERIOD		0x0
#define REG_PERIOD		0x4
#define REG_GROUP		0x8
#define REG_FREQNUM		0x20
#define REG_FREQDATA		0x24
#define REG_POLARITY		0x40

#define REG_PWMSTART		0x44
#define REG_PWMUPDATE		0x4C
#define REG_SHIFTCOUNT		0x80
#define REG_SHIFTSTART		0x90
#define REG_FREQEN		0x9C
#define REG_FREQ_DONE_NUM	0xC0
#define REG_PWM_OE		0xD0

#define PWM_REG_NUM		0x80

#define VDDC_PWM_ID		0x8

#define REG_GPIO0_PINCTRL	0x28104C64

/**
 * struct cv_pwm_channel - private data of PWM channel
 * @period_ns:	current period in nanoseconds programmed to the hardware
 * @duty_ns:	current duty time in nanoseconds programmed to the hardware
 * @tin_ns:	time of one timer tick in nanoseconds with current timer rate
 */
struct cv_pwm_channel {
	u32 period;
	u32 hlperiod;
};

/**
 * struct cv_pwm_chip - private data of PWM chip
 * @chip:		generic PWM chip
 * @variant:		local copy of hardware variant data
 * @inverter_mask:	inverter status for all channels - one bit per channel
 * @base:		base address of mapped PWM registers
 * @base_clk:		base clock used to drive the timers
 * @tclk0:		external clock 0 (can be ERR_PTR if not present)
 * @tclk1:		external clock 1 (can be ERR_PTR if not present)
 */
#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
/* Linux 5.10: chip is first member */
struct cv_pwm_chip {
	struct pwm_chip chip;
	void __iomem *base;
	struct clk *base_clk;
	u8 polarity_mask;
	bool no_polarity;
	uint32_t pwm_saved_regs[PWM_REG_NUM];
};
#else
/* Linux 6.12: private data follows pwm_chip (allocated by devm_pwmchip_alloc) */
struct cv_pwm_chip_priv {
	struct pwm_chip *chip; /* Back pointer to pwm_chip */
	void __iomem *base;
	struct clk *base_clk;
	u8 polarity_mask;
	bool no_polarity;
	uint32_t pwm_saved_regs[PWM_REG_NUM];
	/* Channel private data - allocated on request, freed on free */
	struct cv_pwm_channel *channels[];
};
#endif

/* Unified type for both Linux versions */
#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
/* Linux 5.10: cv_pwm_chip is struct cv_pwm_chip */
typedef struct cv_pwm_chip cv_pwm_chip;
static inline
cv_pwm_chip *to_cv_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct cv_pwm_chip, chip);
}
#else
/* Linux 6.12: cv_pwm_chip is struct cv_pwm_chip_priv */
typedef struct cv_pwm_chip_priv cv_pwm_chip;
static inline
cv_pwm_chip *to_cv_pwm_chip(struct pwm_chip *chip)
{
	return pwmchip_get_drvdata(chip);
}
#endif

static int check_vddc_pwm(void)
{
	void __iomem *ptr;
	u32 value, sel;

	ptr = ioremap(REG_GPIO0_PINCTRL, PAGE_SIZE);
	value = readl(ptr);
	sel = value >> 4 & 0xf;
	iounmap(ptr);

	// if pin GPIO0 is multiplexed to PWM8, return 1.
	if (sel == 0x7)
		return 1;
	else
		return 0;
}

static int pwm_cv_request(struct pwm_chip *chip, struct pwm_device *pwm_dev)
{
	struct cv_pwm_channel *channel;
#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
	/* Linux 5.10: use pwm->pwm instead of hwpwm */
	if (pwm_dev->pwm == VDDC_PWM_ID && check_vddc_pwm())
		return -EBUSY;
#else
	/* Linux 6.12: use hwpwm */
	cv_pwm_chip *our_chip = to_cv_pwm_chip(chip);

	if (pwm_dev->hwpwm == VDDC_PWM_ID && check_vddc_pwm())
		return -EBUSY;

	/* Check channel count */
	if (pwm_dev->hwpwm >= chip->npwm)
		return -EINVAL;
#endif

	channel = kzalloc(sizeof(*channel), GFP_KERNEL);
	if (!channel)
		return -ENOMEM;

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
	return pwm_set_chip_data(pwm_dev, channel);
#else
	our_chip->channels[pwm_dev->hwpwm] = channel;
	return 0;
#endif
}

static void pwm_cv_free(struct pwm_chip *chip, struct pwm_device *pwm_dev)
{
#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
	struct cv_pwm_channel *channel = pwm_get_chip_data(pwm_dev);

	pwm_set_chip_data(pwm_dev, NULL);
	kfree(channel);
#else
	cv_pwm_chip *our_chip = to_cv_pwm_chip(chip);

	/* Check channel count before accessing */
	if (pwm_dev->hwpwm < chip->npwm) {
		kfree(our_chip->channels[pwm_dev->hwpwm]);
		our_chip->channels[pwm_dev->hwpwm] = NULL;
	}
#endif
}

static int pwm_cv_config(struct pwm_chip *chip, struct pwm_device *pwm_dev,
			     int duty_ns, int period_ns)
{
	cv_pwm_chip *our_chip = to_cv_pwm_chip(chip);
	struct cv_pwm_channel *channel;
	u64 cycles;

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
	channel = pwm_get_chip_data(pwm_dev);
#else
	/* Check channel count */
	if (pwm_dev->hwpwm >= chip->npwm)
		return -EINVAL;

	channel = our_chip->channels[pwm_dev->hwpwm];
	if (!channel)
		return -EINVAL;
#endif

	cycles = clk_get_rate(our_chip->base_clk);
	pr_debug("clk_get_rate=%llu\n", cycles);

	cycles *= period_ns;
	do_div(cycles, NSEC_PER_SEC);

	channel->period = cycles;
	cycles = cycles * duty_ns;
	do_div(cycles, period_ns);

	if (cycles == 0)
		cycles = 1;
	if (cycles == channel->period)
		cycles = channel->period - 1;

	channel->hlperiod = channel->period - cycles;

	pr_debug("period_ns=%d, duty_ns=%d, period=%d, hlperiod=%d\n",
			period_ns, duty_ns, channel->period, channel->hlperiod);

	return 0;
}

static int pwm_cv_enable(struct pwm_chip *chip, struct pwm_device *pwm_dev)
{
	cv_pwm_chip *our_chip = to_cv_pwm_chip(chip);
	struct cv_pwm_channel *channel;
	uint32_t pwm_start_value;
	uint32_t value;
	unsigned int hwpwm;

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
	hwpwm = pwm_dev->pwm;
	channel = pwm_get_chip_data(pwm_dev);
#else
	hwpwm = pwm_dev->hwpwm;

	/* Check channel count */
	if (hwpwm >= chip->npwm)
		return -EINVAL;

	channel = our_chip->channels[hwpwm];
	if (!channel)
		return -EINVAL;
#endif

	writel(channel->period, our_chip->base + REG_GROUP * hwpwm + REG_PERIOD);
	if (channel->hlperiod != 0)
		writel(channel->hlperiod, our_chip->base + REG_GROUP * hwpwm + REG_HLPERIOD);

	pwm_start_value = readl(our_chip->base + REG_PWMSTART);

	writel(pwm_start_value & (~(1 << hwpwm)), our_chip->base + REG_PWMSTART);

	value = pwm_start_value | (1 << hwpwm);
	pr_debug("pwm_cv_enable: value = %x\n", value);

	writel(value, our_chip->base + REG_PWM_OE);
	writel(value, our_chip->base + REG_PWMSTART);

	return 0;
}

static void pwm_cv_disable(struct pwm_chip *chip,
			       struct pwm_device *pwm_dev)
{
	cv_pwm_chip *our_chip = to_cv_pwm_chip(chip);
	uint32_t value;
#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
	unsigned int hwpwm = pwm_dev->pwm;
#else
	unsigned int hwpwm = pwm_dev->hwpwm;
#endif

	value = readl(our_chip->base + REG_PWMSTART) & (~(1 << hwpwm));
	pr_debug("pwm_cv_disable: value = %x\n", value);
	writel(value, our_chip->base + REG_PWM_OE);
	writel(value, our_chip->base + REG_PWMSTART);

	writel(1, our_chip->base + REG_GROUP * hwpwm + REG_PERIOD);
	writel(2, our_chip->base + REG_GROUP * hwpwm + REG_HLPERIOD);
}

static int pwm_cv_set_polarity(struct pwm_chip *chip,
				    struct pwm_device *pwm_dev,
				    enum pwm_polarity polarity)
{
	cv_pwm_chip *our_chip = to_cv_pwm_chip(chip);
#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
	unsigned int hwpwm = pwm_dev->pwm;
#else
	unsigned int hwpwm = pwm_dev->hwpwm;
#endif

	if (our_chip->no_polarity) {
#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
		dev_err(chip->dev, "no polarity\n");
#else
		dev_err(&chip->dev, "no polarity\n");
#endif
		return -ENOTSUPP;
	}

	if (polarity == PWM_POLARITY_NORMAL)
		our_chip->polarity_mask &= ~(1 << hwpwm);
	else
		our_chip->polarity_mask |= 1 << hwpwm;

	writel(our_chip->polarity_mask, our_chip->base + REG_POLARITY);

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int pwm_cv_apply(struct pwm_chip *chip, struct pwm_device *pwm,
			      const struct pwm_state *state)
#else
static int pwm_cv_apply(struct pwm_chip *chip, struct pwm_device *pwm,
			      struct pwm_state *state)
#endif
{
	int ret;

	ret = pwm_cv_config(chip, pwm, state->duty_cycle, state->period);
	if (ret) {
#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
		dev_err(chip->dev, "pwm apply err\n");
#else
		dev_err(&chip->dev, "pwm apply err\n");
#endif
		return ret;
	}

	ret = pwm_cv_set_polarity(chip, pwm, state->polarity);
	if (ret) {
#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
		dev_err(chip->dev, "pwm apply err\n");
#else
		dev_err(&chip->dev, "pwm apply err\n");
#endif
		return ret;
	}

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
	dev_dbg(chip->dev, "pwm_cv_apply state->enabled = %d\n", state->enabled);
#else
	dev_dbg(&chip->dev, "pwm_cv_apply state->enabled = %d\n", state->enabled);
#endif
	if (state->enabled)
		ret = pwm_cv_enable(chip, pwm);
	else
		pwm_cv_disable(chip, pwm);

	if (ret) {
#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
		dev_err(chip->dev, "pwm apply failed\n");
#else
		dev_err(&chip->dev, "pwm apply failed\n");
#endif
		return ret;
	}
	return ret;
}

static int pwm_cv_capture(struct pwm_chip *chip, struct pwm_device *pwm_dev,
			   struct pwm_capture *result, unsigned long timeout)
{
	cv_pwm_chip *our_chip = to_cv_pwm_chip(chip);
	uint32_t value;
	u64 cycles;
	u64 cycle_cnt;
#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
	unsigned int hwpwm = pwm_dev->pwm;
#else
	unsigned int hwpwm = pwm_dev->hwpwm;
#endif

	// Set corresponding bit in PWM_OE to 0
	value = readl(our_chip->base + REG_PWM_OE) & (~(1 << hwpwm));
	writel(value, our_chip->base + REG_PWM_OE);
	pr_debug("pwm_cv_capture: REG_PWM_OE = %x\n", value);

	// Enable capture
	writel(1, our_chip->base + REG_GROUP * hwpwm + REG_FREQNUM);
	writel(1 << hwpwm, our_chip->base + REG_FREQEN);
	pr_debug("pwm_cv_capture: REG_FREQEN = %x\n", readl(our_chip->base + REG_FREQEN));

	// Wait for done status
	while (timeout--) {
		mdelay(1);
		pr_debug("delay 1ms\n");
		value = readl(our_chip->base + REG_FREQ_DONE_NUM + hwpwm * 4);
		if (value != 0)
			break;
	}

	// Handle timeout
	if (value == 0) {
		result->period = 0;
		result->duty_cycle = 0;
	} else {
		// Read cycle count
		cycle_cnt = readl(our_chip->base + REG_GROUP * hwpwm + REG_FREQDATA) + 1;
		pr_debug("%s: cycle_cnt = %llu\n", __func__, cycle_cnt);

		// Convert from cycle count to period ns
		cycles = clk_get_rate(our_chip->base_clk);
		cycle_cnt *= NSEC_PER_SEC;
		do_div(cycle_cnt, cycles);

		result->period = cycle_cnt;
		result->duty_cycle = 0;
	}

	// Disable capture
	value = readl(our_chip->base + REG_FREQEN) & (~(1 << hwpwm));
	writel(value, our_chip->base + REG_FREQEN);

	return 0;
}

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
/* Linux 5.10: pwm_ops includes enable, disable, config, owner */
static const struct pwm_ops pwm_cv_ops = {
	.request	= pwm_cv_request,
	.free		= pwm_cv_free,
	.enable		= pwm_cv_enable,
	.disable	= pwm_cv_disable,
	.config		= pwm_cv_config,
	.apply		= pwm_cv_apply,
	.capture	= pwm_cv_capture,
	.owner		= THIS_MODULE,
};
#else
/* Linux 6.12: pwm_ops only includes request, free, apply, capture */
static const struct pwm_ops pwm_cv_ops = {
	.request	= pwm_cv_request,
	.free		= pwm_cv_free,
	.apply		= pwm_cv_apply,
	.capture	= pwm_cv_capture,
};
#endif

static const struct of_device_id cv_pwm_match[] = {
	{ .compatible = "cvitek,cvi-pwm" },
	{ },
};
MODULE_DEVICE_TABLE(of, cv_pwm_match);

static int pwm_cv_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	cv_pwm_chip *chip;
	struct resource *res;
	int ret;
	unsigned int npwm;

	// pr_debug("%s\n", __func__);

	//pwm-num default is 4, compatible with bm1682
	if (of_property_read_bool(pdev->dev.of_node, "pwm-num"))
		device_property_read_u32(&pdev->dev, "pwm-num", &npwm);
	else
		npwm = 4;

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
	/* Linux 5.10: allocate chip structure */
	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;

	chip->chip.dev = &pdev->dev;
	chip->chip.base = -1;
	chip->chip.of_pwm_n_cells = 3;
	chip->chip.npwm = npwm;
#else
	/* Linux 6.12: use devm_pwmchip_alloc for flexible array member support
	 * Note: devm_pwmchip_alloc already calls device_initialize and sets dev.parent
	 * dev_set_name will be called by pwmchip_add
	 */
	struct pwm_chip *pwm_chip;
	size_t chip_size = struct_size((struct cv_pwm_chip_priv *)0, channels, npwm);

	pwm_chip = devm_pwmchip_alloc(dev, npwm, chip_size);
	if (IS_ERR(pwm_chip))
		return PTR_ERR(pwm_chip);

	chip = pwmchip_get_drvdata(pwm_chip);
	chip->chip = pwm_chip; /* Save back pointer */
	/* npwm is already set by devm_pwmchip_alloc */
#endif

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
	chip->chip.ops = &pwm_cv_ops;
	chip->polarity_mask = 0;
	chip->chip.of_xlate = of_pwm_xlate_with_flags;
#else
	chip->chip->ops = &pwm_cv_ops;
	chip->polarity_mask = 0;
	chip->chip->of_xlate = of_pwm_xlate_with_flags;
#endif

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	chip->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(chip->base))
		return PTR_ERR(chip->base);

	chip->base_clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(chip->base_clk)) {
		dev_err(dev, "failed to get pwm source clk\n");
		return PTR_ERR(chip->base_clk);
	}

	ret = clk_prepare_enable(chip->base_clk);
	if (ret < 0) {
		dev_err(dev, "failed to enable base clock\n");
		return ret;
	}

	//no_polarity default is false(have polarity) , compatible with bm1682
	if (of_property_read_bool(pdev->dev.of_node, "no-polarity"))
		chip->no_polarity = true;
	else
		chip->no_polarity = false;
	// pr_debug("chip->chip.npwm = %d  chip->no_polarity = %d\n", chip->chip.npwm, chip->no_polarity);

	platform_set_drvdata(pdev, chip);

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
	ret = pwmchip_add(&chip->chip);
#else
	ret = pwmchip_add(chip->chip);
#endif
	if (ret < 0) {
		dev_err(dev, "failed to register PWM chip\n");
		clk_disable_unprepare(chip->base_clk);
		return ret;
	}

	return 0;
}

/* platform_driver.remove return type changed from int to void in Linux 6.12 */
#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
static int pwm_cv_remove(struct platform_device *pdev)
{
	struct cv_pwm_chip *chip = platform_get_drvdata(pdev);
	int ret;

	ret = pwmchip_remove(&chip->chip);
	if (ret < 0)
		return ret;

	clk_disable_unprepare(chip->base_clk);

	return 0;
}
#else
static void pwm_cv_remove(struct platform_device *pdev)
{
	cv_pwm_chip *chip = platform_get_drvdata(pdev);
	int i;

	pwmchip_remove(chip->chip);

	/* Free all allocated channels */
	for (i = 0; i < chip->chip->npwm; i++) {
		kfree(chip->channels[i]);
		chip->channels[i] = NULL;
	}

	clk_disable_unprepare(chip->base_clk);
}
#endif

#ifdef CONFIG_PM_SLEEP
static int pwm_cv_suspend(struct device *dev)
{
	cv_pwm_chip *chip = dev_get_drvdata(dev);

	memcpy_fromio(chip->pwm_saved_regs, chip->base, PWM_REG_NUM * 4);

	return 0;
}

static int pwm_cv_resume(struct device *dev)
{
	cv_pwm_chip *chip = dev_get_drvdata(dev);

	memcpy_toio(chip->base, chip->pwm_saved_regs, PWM_REG_NUM * 4);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(pwm_cv_pm_ops, pwm_cv_suspend,
			 pwm_cv_resume);

static struct platform_driver pwm_cv_driver = {
	.driver		= {
		.name	= "cvtek-pwm",
		.pm	= &pwm_cv_pm_ops,
		.of_match_table = of_match_ptr(cv_pwm_match),
	},
	.probe		= pwm_cv_probe,
	.remove		= pwm_cv_remove,
};
module_platform_driver(pwm_cv_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mark.Hsieh");
MODULE_DESCRIPTION("Cvitek PWM driver");
