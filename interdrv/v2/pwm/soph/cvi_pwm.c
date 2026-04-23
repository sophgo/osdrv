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
#include <linux/mutex.h>
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
#define REG_PCOUNT		0x50
#define REG_PCOUNT_STATUS	0x60
#define REG_SHIFTCOUNT		0x80
#define REG_SHIFTSTART		0x90
#define REG_FREQEN		0x9C
#define REG_FREQ_DONE_NUM	0xC0
#define REG_PWM_OE		0xD0

#define PWM_REG_NUM		0x80
#define CV_PWM_COUNT_CH_NUM	4

#define CVI_PWM_COUNTMODE_POS(_ch)	(8 + (_ch))
#define CVI_PWM_COUNTMODE_MASK(_ch)	BIT(CVI_PWM_COUNTMODE_POS(_ch))
#define CVI_PWM_SHIFTMODE_MASK		BIT(16)

#define VDDC_PWM_ID		0x8

#define REG_GPIO0_PINCTRL	0x28104C64

struct cv_pwm_channel {
	u32 period;
	u32 hlperiod;
};

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
struct cv_pwm_chip {
	struct pwm_chip chip;
	void __iomem *base;
	struct clk *base_clk;
	u8 polarity_mask;
	bool no_polarity;
	bool count_mode_en[CV_PWM_COUNT_CH_NUM];
	u32 pulse_count[CV_PWM_COUNT_CH_NUM];
	bool shift_mode_en;
	u64 shift_ns[CV_PWM_COUNT_CH_NUM];
	u32 shift_count[CV_PWM_COUNT_CH_NUM];
	bool shift_configured[CV_PWM_COUNT_CH_NUM];
	/* protects register read-modify-write sequences */
	struct mutex lock;
	uint32_t pwm_saved_regs[PWM_REG_NUM];
};
#else
struct cv_pwm_chip_priv {
	struct pwm_chip *chip;
	void __iomem *base;
	struct clk *base_clk;
	u8 polarity_mask;
	bool no_polarity;
	bool count_mode_en[CV_PWM_COUNT_CH_NUM];
	u32 pulse_count[CV_PWM_COUNT_CH_NUM];
	bool shift_mode_en;
	u64 shift_ns[CV_PWM_COUNT_CH_NUM];
	u32 shift_count[CV_PWM_COUNT_CH_NUM];
	bool shift_configured[CV_PWM_COUNT_CH_NUM];
	/* protects register read-modify-write sequences */
	struct mutex lock;
	uint32_t pwm_saved_regs[PWM_REG_NUM];
	/* flexible array member must be last */
	struct cv_pwm_channel *channels[];
};
#endif

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
typedef struct cv_pwm_chip cv_pwm_chip;
static inline
cv_pwm_chip *to_cv_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct cv_pwm_chip, chip);
}

static inline unsigned int cv_pwm_npwm(cv_pwm_chip *c)
{
	return c->chip.npwm;
}

static inline struct pwm_chip *cv_pwm_pwmchip(cv_pwm_chip *c)
{
	return &c->chip;
}
#else
typedef struct cv_pwm_chip_priv cv_pwm_chip;
static inline
cv_pwm_chip *to_cv_pwm_chip(struct pwm_chip *chip)
{
	return pwmchip_get_drvdata(chip);
}

static inline unsigned int cv_pwm_npwm(cv_pwm_chip *c)
{
	return c->chip->npwm;
}

static inline struct pwm_chip *cv_pwm_pwmchip(cv_pwm_chip *c)
{
	return c->chip;
}
#endif

static bool pwm_cv_count_ch_valid(cv_pwm_chip *chip, unsigned int ch)
{
	return ch < cv_pwm_npwm(chip) && ch < CV_PWM_COUNT_CH_NUM;
}

static unsigned int pwm_cv_valid_ch_count(cv_pwm_chip *chip)
{
	unsigned int cnt = cv_pwm_npwm(chip);

	if (cnt > CV_PWM_COUNT_CH_NUM)
		cnt = CV_PWM_COUNT_CH_NUM;

	return cnt;
}

static u32 pwm_cv_shift_configured_mask(cv_pwm_chip *chip)
{
	u32 mask = 0;
	unsigned int i;
	unsigned int ch_cnt;

	ch_cnt = pwm_cv_valid_ch_count(chip);
	for (i = 0; i < ch_cnt; i++) {
		if (chip->shift_configured[i])
			mask |= BIT(i);
	}

	return mask;
}

static void pwm_cv_set_count_mode(cv_pwm_chip *chip, unsigned int ch, bool enable)
{
	u32 value;

	value = readl(chip->base + REG_POLARITY);
	if (enable)
		value |= CVI_PWM_COUNTMODE_MASK(ch);
	else
		value &= ~CVI_PWM_COUNTMODE_MASK(ch);
	writel(value, chip->base + REG_POLARITY);
}

static void pwm_cv_set_pulse_count(cv_pwm_chip *chip, unsigned int ch, u32 count)
{
	writel(count, chip->base + REG_PCOUNT + ch * 4);
}

static void pwm_cv_set_shift_mode(cv_pwm_chip *chip, bool enable)
{
	u32 value;

	value = readl(chip->base + REG_POLARITY);
	if (enable)
		value |= CVI_PWM_SHIFTMODE_MASK;
	else
		value &= ~CVI_PWM_SHIFTMODE_MASK;
	writel(value, chip->base + REG_POLARITY);
}

static void pwm_cv_set_shift_count(cv_pwm_chip *chip, unsigned int ch, u32 count)
{
	writel(count, chip->base + REG_SHIFTCOUNT + ch * 4);
}

static void pwm_cv_set_shift_start(cv_pwm_chip *chip, bool enable)
{
	u32 value;

	value = readl(chip->base + REG_SHIFTSTART);
	if (enable)
		value |= 0x1;
	else
		value &= ~0x1;
	writel(value, chip->base + REG_SHIFTSTART);
}

static int pwm_cv_ns_to_cycles(cv_pwm_chip *chip, u64 ns, u32 *cycles)
{
	u64 clk_rate;
	u64 cyc;

	clk_rate = clk_get_rate(chip->base_clk);
	if (!clk_rate)
		return -EINVAL;

	cyc = ns * clk_rate;
	do_div(cyc, NSEC_PER_SEC);
	if (cyc > 0xffffffffULL)
		return -ERANGE;

	*cycles = (u32)cyc;
	return 0;
}

static int check_vddc_pwm(void)
{
	void __iomem *ptr;
	u32 value, sel;

	ptr = ioremap(REG_GPIO0_PINCTRL, PAGE_SIZE);
	value = readl(ptr);
	sel = value >> 4 & 0xf;
	iounmap(ptr);

	if (sel == 0x7)
		return 1;
	else
		return 0;
}

static int pwm_cv_request(struct pwm_chip *chip, struct pwm_device *pwm_dev)
{
	struct cv_pwm_channel *channel;

	if (pwm_dev->hwpwm == VDDC_PWM_ID && check_vddc_pwm())
		return -EBUSY;

	channel = kzalloc(sizeof(*channel), GFP_KERNEL);
	if (!channel)
		return -ENOMEM;

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
	return pwm_set_chip_data(pwm_dev, channel);
#else
	to_cv_pwm_chip(chip)->channels[pwm_dev->hwpwm] = channel;
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
	unsigned int ch = pwm_dev->hwpwm;

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
	channel = pwm_get_chip_data(pwm_dev);
#else
	if (ch >= chip->npwm)
		return -EINVAL;

	channel = our_chip->channels[ch];
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

	if (pwm_cv_count_ch_valid(our_chip, ch)) {
		writel(channel->period, our_chip->base + REG_GROUP * ch + REG_PERIOD);
		if (channel->hlperiod != 0)
			writel(channel->hlperiod, our_chip->base + REG_GROUP * ch + REG_HLPERIOD);
	}

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
	unsigned int ch = pwm_dev->hwpwm;

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
	channel = pwm_get_chip_data(pwm_dev);
#else
	if (ch >= chip->npwm)
		return -EINVAL;

	channel = our_chip->channels[ch];
	if (!channel)
		return -EINVAL;
#endif

	mutex_lock(&our_chip->lock);

	if (pwm_cv_count_ch_valid(our_chip, ch)) {
		pwm_cv_set_count_mode(our_chip, ch, our_chip->count_mode_en[ch]);
		if (our_chip->count_mode_en[ch])
			pwm_cv_set_pulse_count(our_chip, ch, our_chip->pulse_count[ch]);

		if (our_chip->shift_mode_en) {
			pwm_cv_set_shift_mode(our_chip, true);
			pwm_cv_set_shift_count(our_chip, ch, our_chip->shift_count[ch]);
		}
	}

	writel(channel->period, our_chip->base + REG_GROUP * ch + REG_PERIOD);
	if (channel->hlperiod != 0)
		writel(channel->hlperiod, our_chip->base + REG_GROUP * ch + REG_HLPERIOD);

	if (our_chip->shift_mode_en) {
		mutex_unlock(&our_chip->lock);
		return 0;
	}

	pwm_start_value = readl(our_chip->base + REG_PWMSTART);

	writel(pwm_start_value & (~(1 << ch)), our_chip->base + REG_PWMSTART);

	value = pwm_start_value | (1 << ch);
	pr_debug("pwm_cv_enable: value = %x\n", value);

	writel(value, our_chip->base + REG_PWM_OE);
	writel(value, our_chip->base + REG_PWMSTART);
	mutex_unlock(&our_chip->lock);

	return 0;
}

static void pwm_cv_disable(struct pwm_chip *chip,
			       struct pwm_device *pwm_dev)
{
	cv_pwm_chip *our_chip = to_cv_pwm_chip(chip);
	uint32_t value;
	u32 oe_value;
	unsigned int ch = pwm_dev->hwpwm;

	mutex_lock(&our_chip->lock);

	value = readl(our_chip->base + REG_PWMSTART) & (~(1 << ch));
	pr_debug("pwm_cv_disable: value = %x\n", value);
	writel(value, our_chip->base + REG_PWMSTART);

	oe_value = readl(our_chip->base + REG_PWM_OE) & (~(1 << ch));
	writel(oe_value, our_chip->base + REG_PWM_OE);

	mutex_unlock(&our_chip->lock);
}

static int pwm_cv_set_polarity(struct pwm_chip *chip,
				    struct pwm_device *pwm_dev,
				    enum pwm_polarity polarity)
{
	cv_pwm_chip *our_chip = to_cv_pwm_chip(chip);
	u32 value;

	if (our_chip->no_polarity) {
#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
		dev_err(chip->dev, "no polarity\n");
#else
		dev_err(&chip->dev, "no polarity\n");
#endif
		return -ENOTSUPP;
	}

	mutex_lock(&our_chip->lock);

	value = readl(our_chip->base + REG_POLARITY);
	if (polarity == PWM_POLARITY_NORMAL)
		value &= ~(1 << pwm_dev->hwpwm);
	else
		value |= 1 << pwm_dev->hwpwm;

	writel(value, our_chip->base + REG_POLARITY);
	our_chip->polarity_mask = value & 0xff;

	mutex_unlock(&our_chip->lock);

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
	unsigned int hwpwm = pwm_dev->hwpwm;

	value = readl(our_chip->base + REG_PWM_OE) & (~(1 << hwpwm));
	writel(value, our_chip->base + REG_PWM_OE);
	pr_debug("pwm_cv_capture: REG_PWM_OE = %x\n", value);

	writel(1, our_chip->base + REG_GROUP * hwpwm + REG_FREQNUM);
	writel(1 << hwpwm, our_chip->base + REG_FREQEN);
	pr_debug("pwm_cv_capture: REG_FREQEN = %x\n", readl(our_chip->base + REG_FREQEN));

	while (timeout--) {
		mdelay(1);
		pr_debug("delay 1ms\n");
		value = readl(our_chip->base + REG_FREQ_DONE_NUM + hwpwm * 4);
		if (value != 0)
			break;
	}

	if (value == 0) {
		result->period = 0;
		result->duty_cycle = 0;
	} else {
		cycle_cnt = readl(our_chip->base + REG_GROUP * hwpwm + REG_FREQDATA) + 1;
		pr_debug("%s: cycle_cnt = %llu\n", __func__, cycle_cnt);

		cycles = clk_get_rate(our_chip->base_clk);
		cycle_cnt *= NSEC_PER_SEC;
		do_div(cycle_cnt, cycles);

		result->period = cycle_cnt;
		result->duty_cycle = 0;
	}

	value = readl(our_chip->base + REG_FREQEN) & (~(1 << hwpwm));
	writel(value, our_chip->base + REG_FREQEN);

	return 0;
}

static ssize_t count_pulse_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	cv_pwm_chip *chip = dev_get_drvdata(dev);
	unsigned int i, cnt;
	ssize_t len = 0;

	cnt = pwm_cv_valid_ch_count(chip);

	mutex_lock(&chip->lock);
	for (i = 0; i < cnt; i++)
		len += scnprintf(buf + len, PAGE_SIZE - len, "%u:%u ", i,
				 chip->count_mode_en[i] ? chip->pulse_count[i] : 0);
	mutex_unlock(&chip->lock);
	len += scnprintf(buf + len, PAGE_SIZE - len, "\n");

	return len;
}

static ssize_t count_pulse_store(struct device *dev, struct device_attribute *attr,
				 const char *buf, size_t count)
{
	cv_pwm_chip *chip = dev_get_drvdata(dev);
	unsigned int ch;
	u32 count_pulse;

	if (sscanf(buf, "%u %u", &ch, &count_pulse) != 2)
		return -EINVAL;
	if (!pwm_cv_count_ch_valid(chip, ch))
		return -EINVAL;

	mutex_lock(&chip->lock);
	chip->pulse_count[ch] = count_pulse;
	chip->count_mode_en[ch] = (count_pulse != 0);
	pwm_cv_set_pulse_count(chip, ch, count_pulse);
	pwm_cv_set_count_mode(chip, ch, chip->count_mode_en[ch]);
	mutex_unlock(&chip->lock);

	return count;
}

static ssize_t shift_ns_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	cv_pwm_chip *chip = dev_get_drvdata(dev);
	unsigned int i, cnt;
	ssize_t len = 0;

	cnt = pwm_cv_valid_ch_count(chip);

	mutex_lock(&chip->lock);
	for (i = 0; i < cnt; i++)
		len += scnprintf(buf + len, PAGE_SIZE - len, "%u:%llu ", i,
				 (unsigned long long)chip->shift_ns[i]);
	mutex_unlock(&chip->lock);
	len += scnprintf(buf + len, PAGE_SIZE - len, "\n");

	return len;
}

static ssize_t shift_ns_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	cv_pwm_chip *chip = dev_get_drvdata(dev);
	unsigned int ch;
	unsigned long long shift_ns;
	u32 shift_cnt;
	int ret;

	if (sscanf(buf, "%u %llu", &ch, &shift_ns) != 2)
		return -EINVAL;
	if (!pwm_cv_count_ch_valid(chip, ch))
		return -EINVAL;

	mutex_lock(&chip->lock);
	ret = pwm_cv_ns_to_cycles(chip, shift_ns, &shift_cnt);
	if (ret) {
		mutex_unlock(&chip->lock);
		return ret;
	}

	chip->shift_ns[ch] = shift_ns;
	chip->shift_count[ch] = shift_cnt;
	chip->shift_configured[ch] = true;
	pwm_cv_set_shift_count(chip, ch, shift_cnt);
	mutex_unlock(&chip->lock);

	return count;
}

static ssize_t shift_start_store(struct device *dev, struct device_attribute *attr,
				 const char *buf, size_t count)
{
	cv_pwm_chip *chip = dev_get_drvdata(dev);
	unsigned int start;
	u32 mask;
	u32 value;

	if (kstrtouint(buf, 0, &start))
		return -EINVAL;
	if (start > 1)
		return -EINVAL;

	mutex_lock(&chip->lock);
	mask = pwm_cv_shift_configured_mask(chip);
	if (start && mask == 0) {
		mutex_unlock(&chip->lock);
		return -EINVAL;
	}

	if (!start) {
		pwm_cv_set_shift_start(chip, false);
		value = readl(chip->base + REG_PWMSTART);
		writel(value & ~mask, chip->base + REG_PWMSTART);
		chip->shift_mode_en = false;
		pwm_cv_set_shift_mode(chip, false);
	} else {
		chip->shift_mode_en = true;
		pwm_cv_set_shift_mode(chip, true);
		pwm_cv_set_shift_start(chip, false);
		value = readl(chip->base + REG_PWMSTART);
		writel(value & ~mask, chip->base + REG_PWMSTART);

		value = readl(chip->base + REG_PWM_OE);
		writel(value | mask, chip->base + REG_PWM_OE);

		value = readl(chip->base + REG_PWMSTART);
		writel(value | mask, chip->base + REG_PWMSTART);
		pwm_cv_set_shift_start(chip, true);
	}
	mutex_unlock(&chip->lock);

	return count;
}

static ssize_t pwm_status_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	cv_pwm_chip *chip = dev_get_drvdata(dev);
	unsigned int i, cnt;
	ssize_t len = 0;
	u32 shiftstart;
	u32 pwmstart;
	u32 pwmoe;
	u32 polarity;
	u32 mask;

	mutex_lock(&chip->lock);
	shiftstart = readl(chip->base + REG_SHIFTSTART);
	pwmstart = readl(chip->base + REG_PWMSTART);
	pwmoe = readl(chip->base + REG_PWM_OE);
	polarity = readl(chip->base + REG_POLARITY);
	mask = pwm_cv_shift_configured_mask(chip);
	cnt = pwm_cv_valid_ch_count(chip);

	len += scnprintf(buf + len, PAGE_SIZE - len,
			 "Global\n");
	len += scnprintf(buf + len, PAGE_SIZE - len,
			 "  shift_mode      : %u\n", chip->shift_mode_en ? 1 : 0);
	len += scnprintf(buf + len, PAGE_SIZE - len,
			 "  shift_start     : %u\n", shiftstart & 0x1);
	len += scnprintf(buf + len, PAGE_SIZE - len,
			 "  configured_mask : 0x%08x\n", mask);
	len += scnprintf(buf + len, PAGE_SIZE - len,
			 "  pwmstart_reg    : 0x%08x\n", pwmstart);
	len += scnprintf(buf + len, PAGE_SIZE - len,
			 "  pwmoe_reg       : 0x%08x\n", pwmoe);
	len += scnprintf(buf + len, PAGE_SIZE - len,
			 "  polarity_reg    : 0x%08x\n\n", polarity);

	len += scnprintf(buf + len, PAGE_SIZE - len,
			 "+----+----------+------------+----------+------------+------------+----------+----------+---------+\n");
	len += scnprintf(buf + len, PAGE_SIZE - len,
			 "| ch | shift_en  | shift_ns   | count_en | count_pulse| count_done | period   | hlperiod | running |\n");
	len += scnprintf(buf + len, PAGE_SIZE - len,
			 "+----+----------+------------+----------+------------+------------+----------+----------+---------+\n");

	for (i = 0; i < cnt; i++) {
		len += scnprintf(buf + len, PAGE_SIZE - len,
				 "| %2u | %8u | %10llu | %8u | %10u | %10u | %8u | %8u | %7u |\n",
				 i,
				 chip->shift_configured[i] ? 1 : 0,
				 (unsigned long long)chip->shift_ns[i],
				 chip->count_mode_en[i] ? 1 : 0,
				 chip->pulse_count[i],
				 readl(chip->base + REG_PCOUNT_STATUS + i * 4),
				 readl(chip->base + REG_GROUP * i + REG_PERIOD),
				 readl(chip->base + REG_GROUP * i + REG_HLPERIOD),
				 (pwmstart >> i) & 0x1);
	}
	len += scnprintf(buf + len, PAGE_SIZE - len,
			 "+----+----------+------------+----------+------------+------------+----------+----------+---------+\n");
	mutex_unlock(&chip->lock);

	return len;
}

static DEVICE_ATTR_RW(count_pulse);
static DEVICE_ATTR_RW(shift_ns);
static DEVICE_ATTR_WO(shift_start);
static DEVICE_ATTR_RO(pwm_status);

static struct attribute *cv_pwm_attrs[] = {
	&dev_attr_count_pulse.attr,
	&dev_attr_shift_ns.attr,
	&dev_attr_shift_start.attr,
	&dev_attr_pwm_status.attr,
	NULL,
};

static const struct attribute_group cv_pwm_attr_group = {
	.attrs = cv_pwm_attrs,
};

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
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

	if (of_property_read_bool(pdev->dev.of_node, "pwm-num"))
		device_property_read_u32(&pdev->dev, "pwm-num", &npwm);
	else
		npwm = 4;

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;

	chip->chip.dev = &pdev->dev;
	chip->chip.base = -1;
	chip->chip.of_pwm_n_cells = 3;
	chip->chip.npwm = npwm;
#else
	{
	struct pwm_chip *pwm_chip;
	size_t chip_size = struct_size((struct cv_pwm_chip_priv *)0, channels, npwm);

	pwm_chip = devm_pwmchip_alloc(dev, npwm, chip_size);
	if (IS_ERR(pwm_chip))
		return PTR_ERR(pwm_chip);

	chip = pwmchip_get_drvdata(pwm_chip);
	chip->chip = pwm_chip;
	}
#endif

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
	chip->chip.ops = &pwm_cv_ops;
	chip->chip.of_xlate = of_pwm_xlate_with_flags;
#else
	chip->chip->ops = &pwm_cv_ops;
	chip->chip->of_xlate = of_pwm_xlate_with_flags;
#endif
	chip->polarity_mask = 0;
	mutex_init(&chip->lock);

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

	if (of_property_read_bool(pdev->dev.of_node, "no-polarity"))
		chip->no_polarity = true;
	else
		chip->no_polarity = false;

	platform_set_drvdata(pdev, chip);

	ret = pwmchip_add(cv_pwm_pwmchip(chip));
	if (ret < 0) {
		dev_err(dev, "failed to register PWM chip\n");
		clk_disable_unprepare(chip->base_clk);
		return ret;
	}

	ret = sysfs_create_group(&pdev->dev.kobj, &cv_pwm_attr_group);
	if (ret < 0) {
		dev_err(dev, "failed to create sysfs group\n");
		pwmchip_remove(cv_pwm_pwmchip(chip));
		clk_disable_unprepare(chip->base_clk);
		return ret;
	}

	return 0;
}

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
static int pwm_cv_remove(struct platform_device *pdev)
{
	struct cv_pwm_chip *chip = platform_get_drvdata(pdev);
	int ret;

	sysfs_remove_group(&pdev->dev.kobj, &cv_pwm_attr_group);

	ret = pwmchip_remove(cv_pwm_pwmchip(chip));
	if (ret < 0)
		return ret;

	clk_disable_unprepare(chip->base_clk);

	return 0;
}
#else
static void pwm_cv_remove(struct platform_device *pdev)
{
	cv_pwm_chip *chip = platform_get_drvdata(pdev);

	sysfs_remove_group(&pdev->dev.kobj, &cv_pwm_attr_group);

	pwmchip_remove(cv_pwm_pwmchip(chip));

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
