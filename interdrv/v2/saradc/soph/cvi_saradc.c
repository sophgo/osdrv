/*
 * Cvitek SoCs saradc driver
 *
 * Copyright (c) 2023 Cvitek Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/iopoll.h>
#include <linux/version.h>
#include <linux/iio/iio.h>

#include "./cvi_saradc.h"

/* ------------------------------------------------------------------ */
/*  Channel topology                                                   */
/* ------------------------------------------------------------------ */

enum ADCChannel {
	ADC1 = 1,
	ADC2,
	ADC3,
	PWR_ADC1,
	PWR_ADC2,
};

static inline void __iomem *get_domain_base(struct cvi_saradc_device *ndev,
					    int channel)
{
	return (channel > ADC3) ? ndev->rtcsys_saradc_base_addr
				: ndev->top_saradc_base_addr;
}

static inline int get_hw_channel(int channel)
{
	return (channel > ADC3) ? (channel - ADC3) : channel;
}

/* ------------------------------------------------------------------ */
/*  Clock helpers                                                      */
/* ------------------------------------------------------------------ */

static int platform_saradc_clk_init(struct cvi_saradc_device *ndev)
{
	if (ndev->clk_saradc)
		clk_prepare_enable(ndev->clk_saradc);
	return 0;
}

static void platform_saradc_clk_deinit(struct cvi_saradc_device *ndev)
{
	if (ndev->clk_saradc)
		clk_disable_unprepare(ndev->clk_saradc);
}

/* ------------------------------------------------------------------ */
/*  Core measurement – used by read_raw and self-test                  */
/* ------------------------------------------------------------------ */

static int saradc_do_measurement(void __iomem *base, int hw_chan,
				 uint32_t *result)
{
	uint32_t sel, status;
	int ret;

	writel(0x0, base + SARADC_INTR_EN);

	sel = (1 << (SARADC_CTRL_SEL_SHIFT + hw_chan));
	writel(sel, base + SARADC_CTRL);
	writel(sel | SARADC_CTRL_EN, base + SARADC_CTRL);

	ret = readl_poll_timeout(base + SARADC_STATUS, status,
				 !(status & SARADC_STA_BUSY),
				 SARADC_POLL_US, SARADC_TIMEOUT_US);
	if (ret)
		return -ETIMEDOUT;

	*result = readl(base + SARADC_CH1_RESULT + (hw_chan - 1) * 4);
	return 0;
}

/* ------------------------------------------------------------------ */
/*  Chip-specific pin definitions                                      */
/* ------------------------------------------------------------------ */

struct saradc_pin_info {
	uint32_t    reg_offset;
	uint32_t    adc_mux_val;
	const char *pin_name;
};

#ifdef SARADC_CHIP_BM1688

static const struct saradc_pin_info top_pins[] = {
	{ 0x2c, 0, "ADC1" },
	{ 0x30, 0, "ADC2" },
	{ 0x34, 0, "ADC3" },
};

static const struct saradc_pin_info rtc_pins[] = {
	{ 0x30, 0, "PWR_GPIO0" },
	{ 0x34, 3, "PWR_GPIO1" },
};

#else /* !SARADC_CHIP_BM1688  —  cv84x6 / cv84x2 */

/*
 * Top-domain: ADC1/2/3 in pinmux group G3.
 *   g3_pins index 51/52/53 → offset 0xCC/0xD0/0xD4, PIN_SEL=0 for ADC.
 *
 * RTC-domain: PWR_SAR1/PWR_SAR2 on PWR_GPIO1/PWR_GPIO2 in group G8.
 *   g8_pins index 10/11 → offset 0x28/0x2C, PIN_SEL=1 for PWR_SARx.
 */
static const struct saradc_pin_info top_pins[] = {
	{ 0xCC, 0, "ADC1" },
	{ 0xD0, 0, "ADC2" },
	{ 0xD4, 0, "ADC3" },
};

static const struct saradc_pin_info rtc_pins[] = {
	{ 0x28, 1, "PWR_GPIO1" },
	{ 0x2C, 1, "PWR_GPIO2" },
};

#endif /* SARADC_CHIP_BM1688 */

/* ------------------------------------------------------------------ */
/*  IO config (common logic, chip-specific masks/pins from above)      */
/* ------------------------------------------------------------------ */

static int saradc_io_channel_set_locked(struct cvi_saradc_device *ndev,
					bool is_top, int hw_chan, bool enable)
{
	void __iomem *pinmux_base;
	const struct saradc_pin_info *pins;
	uint32_t *saved_regs;
	bool *saved_valid;
	bool *io_on;
	int max_chans;
	int idx;

	if (is_top) {
		pinmux_base = ndev->top_pinmux_base;
		pins = top_pins;
		saved_regs = ndev->top_pinmux_saved;
		saved_valid = ndev->top_pinmux_saved_valid;
		io_on = ndev->top_pinmux_io_on;
		max_chans = SARADC_TOP_CHANS;
	} else {
		pinmux_base = ndev->rtc_pinmux_base;
		pins = rtc_pins;
		saved_regs = ndev->rtc_pinmux_saved;
		saved_valid = ndev->rtc_pinmux_saved_valid;
		io_on = ndev->rtc_pinmux_io_on;
		max_chans = SARADC_RTC_CHANS;
	}

	if (!pinmux_base)
		return -ENODEV;
	if (hw_chan < 1 || hw_chan > max_chans)
		return -EINVAL;

	idx = hw_chan - 1;
	if (enable) {
		uint32_t reg, new_val;

		if (!saved_valid[idx]) {
			saved_regs[idx] = readl(pinmux_base + pins[idx].reg_offset);
			saved_valid[idx] = true;
		}

		reg = readl(pinmux_base + pins[idx].reg_offset);
		new_val = reg & ~PINMUX_ADC_CFG_MASK;
		new_val |= (pins[idx].adc_mux_val << PINMUX_PIN_SEL_SHIFT);
		writel(new_val, pinmux_base + pins[idx].reg_offset);
		io_on[idx] = true;
	} else {
		if (!saved_valid[idx])
			return -EINVAL;
		writel(saved_regs[idx], pinmux_base + pins[idx].reg_offset);
		io_on[idx] = false;
	}

	return 0;
}

/* ------------------------------------------------------------------ */
/*  IIO read_raw                                                       */
/* ------------------------------------------------------------------ */

static int saradc_read_raw(struct iio_dev *indio_dev,
			   struct iio_chan_spec const *chan,
			   int *val, int *val2, long info)
{
	struct cvi_saradc_device *ndev = iio_priv(indio_dev);
	void __iomem *base;
	int hw_chan;
	uint32_t raw;
	int ret;

	if (info != IIO_CHAN_INFO_RAW)
		return -EINVAL;

	base = get_domain_base(ndev, chan->channel);
	hw_chan = get_hw_channel(chan->channel);

	mutex_lock(&ndev->lock);

	ret = saradc_do_measurement(base, hw_chan, &raw);

	if (ret) {
		mutex_unlock(&ndev->lock);
		dev_err(ndev->dev, "ch%d measurement timeout\n",
			chan->channel);
		return ret;
	}

	if (!(raw & SARADC_RESULT_VALID)) {
		mutex_unlock(&ndev->lock);
		dev_warn(ndev->dev, "ch%d result not valid\n",
			 chan->channel);
		return -ENODATA;
	}

	*val = raw & SARADC_RESULT_MASK;
	mutex_unlock(&ndev->lock);

	return IIO_VAL_INT;
}

/* ------------------------------------------------------------------ */
/*  Continuous / Periodic mode skeletons (not wired to sysfs yet)      */
/* ------------------------------------------------------------------ */

static void __maybe_unused
cvi_saradc_set_cont_mode(void __iomem *base, bool enable)
{
	uint32_t ctrl = readl(base + SARADC_CTRL);

	if (enable)
		ctrl |= SARADC_CTRL_CONT;
	else
		ctrl &= ~SARADC_CTRL_CONT;
	writel(ctrl, base + SARADC_CTRL);
}

static void __maybe_unused
cvi_saradc_set_periodic_mode(void __iomem *base, bool enable,
			     uint32_t period_cycle, uint8_t prediv)
{
	uint32_t ctrl = readl(base + SARADC_CTRL);

	if (enable) {
		writel((period_cycle & SARADC_PERIOD_CYCLE_MASK) |
		       ((uint32_t)prediv << SARADC_PERIOD_PREDIV_SHIFT),
		       base + SARADC_PERIOD);
		ctrl |= SARADC_CTRL_PERIODIC;
	} else {
		ctrl &= ~SARADC_CTRL_PERIODIC;
	}
	writel(ctrl, base + SARADC_CTRL);
}

static void saradc_gap_delay_ns(u64 gap_ns)
{
	u64 us, ms;

	if (!gap_ns)
		return;

	if (gap_ns <= 1000) {
		ndelay(gap_ns);
		return;
	}

	us = DIV_ROUND_UP_ULL(gap_ns, 1000);
	if (us <= 10) {
		udelay(us);
		return;
	}

	if (us < 20000) {
		usleep_range(us, us + 50);
		return;
	}

	ms = DIV_ROUND_UP_ULL(us, 1000);
	msleep(ms);
}

static int saradc_parse_time_to_ns(const char *input, u64 *out_ns)
{
	char tok[20];
	size_t n, num_len;
	u64 val, mul = 0;

	n = strnlen(input, sizeof(tok));
	if (!n || n >= sizeof(tok))
		return -EINVAL;

	memcpy(tok, input, n);
	tok[n] = '\0';
	strim(tok);
	n = strlen(tok);
	if (!n)
		return -EINVAL;

	if (n >= 2 && !strcmp(tok + n - 2, "ns")) {
		mul = 1;
		num_len = n - 2;
	} else if (n >= 2 && !strcmp(tok + n - 2, "us")) {
		mul = 1000ULL;
		num_len = n - 2;
	} else if (n >= 2 && !strcmp(tok + n - 2, "ms")) {
		mul = 1000000ULL;
		num_len = n - 2;
	} else if (n >= 1 && tok[n - 1] == 's') {
		mul = 1000000000ULL;
		num_len = n - 1;
	} else {
		return -EINVAL;
	}

	if (!num_len || num_len >= sizeof(tok))
		return -EINVAL;
	tok[num_len] = '\0';

	if (kstrtoull(tok, 0, &val))
		return -EINVAL;
	if (val > U64_MAX / mul)
		return -ERANGE;

	*out_ns = val * mul;
	return 0;
}

/* ------------------------------------------------------------------ */
/*  ext_info: status (with live measurement)                           */
/* ------------------------------------------------------------------ */

static int saradc_measure_all(void __iomem *base, int num_chans, u64 gap_ns)
{
	uint32_t result;
	int i, ret;

	for (i = 1; i <= num_chans; i++) {
		ret = saradc_do_measurement(base, i, &result);
		if (ret)
			return ret;
		if (i < num_chans)
			saradc_gap_delay_ns(gap_ns);
	}
	return 0;
}

static void saradc_print_domain(struct cvi_saradc_device *ndev,
				void __iomem *base,
				const char *name, int num_chans,
				const struct saradc_pin_info *pins,
				int n_pins,
				void __iomem *pinmux_base,
				u64 gap_ns,
				char *buf, ssize_t *len)
{
	uint32_t version, ctrl, status, cyc_set, force;
	uint32_t settling, sample, clkdiv, compare;
	uint32_t s, sa, co, d, t_1ch_ns, t_all_ns;
	int i, meas_ok;

	meas_ok = (saradc_measure_all(base, num_chans, gap_ns) == 0);

	version  = readl(base + SARADC_VERSION);
	ctrl     = readl(base + SARADC_CTRL);
	status   = readl(base + SARADC_STATUS);
	cyc_set  = readl(base + SARADC_CYC_SET);
	force    = readl(base + SARADC_TEST_FORCE);

	settling = (cyc_set >> SARADC_CYC_SETTLING_SHIFT) & 0x1F;
	sample   = (cyc_set >> SARADC_CYC_SAMP_SHIFT) & 0xF;
	clkdiv   = (cyc_set >> SARADC_CYC_CLKDIV_SHIFT) & 0xF;
	compare  = (cyc_set >> SARADC_CYC_COMP_SHIFT) & 0xF;

	s  = settling + 1;
	sa = sample + 1;
	co = compare + 1;
	d  = clkdiv + 1;
	t_1ch_ns  = (s + (sa + co) * 1) * 40 * d;
	t_all_ns  = (s + (sa + co) * num_chans) * 40 * d;

	*len += scnprintf(buf + *len, PAGE_SIZE - *len,
		"\n=== %s (version: 0x%08X) ===\n", name, version);
	*len += scnprintf(buf + *len, PAGE_SIZE - *len,
		"  force_mode      : %s",
		(force & SARADC_FORCE_EN) ? "on" : "off");
	if (force & SARADC_FORCE_EN)
		*len += scnprintf(buf + *len, PAGE_SIZE - *len,
			" (value=%u)", force & SARADC_FORCE_RESULT_MASK);
	*len += scnprintf(buf + *len, PAGE_SIZE - *len, "\n");
	*len += scnprintf(buf + *len, PAGE_SIZE - *len,
		"  mode            : %s\n",
		(ctrl & SARADC_CTRL_CONT) ? "continuous" :
		(ctrl & SARADC_CTRL_PERIODIC) ? "periodic" : "one-shot");
	*len += scnprintf(buf + *len, PAGE_SIZE - *len,
		"  fsm_state       : 0x%X\n",
		(status >> SARADC_STA_FSM_SHIFT) & 0xF);
	*len += scnprintf(buf + *len, PAGE_SIZE - *len,
		"  meas_time       : %u ns/ch, %u ns/%dch"
		"  T=(S+(sa+co)*N)*40ns*D  S=%u sa=%u co=%u D=%u\n",
		t_1ch_ns, t_all_ns, num_chans, s, sa, co, d);
	*len += scnprintf(buf + *len, PAGE_SIZE - *len,
		"  inter_ch_gap    : %llu ns\n",
		(unsigned long long)gap_ns);
	if (!meas_ok)
		*len += scnprintf(buf + *len, PAGE_SIZE - *len,
			"  WARNING: measurement timeout!\n");

	*len += scnprintf(buf + *len, PAGE_SIZE - *len,
		"+------+-------+-------+------+------------------+------+---------+\n");
	*len += scnprintf(buf + *len, PAGE_SIZE - *len,
		"|  ch  | value | valid | busy |  pin(cur/exp)    | pull |  state  |\n");
	*len += scnprintf(buf + *len, PAGE_SIZE - *len,
		"+------+-------+-------+------+------------------+------+---------+\n");

	for (i = 0; i < num_chans; i++) {
		uint32_t raw = readl(base + SARADC_CH1_RESULT + i * 4);
		uint32_t val   = raw & SARADC_RESULT_MASK;
		uint32_t valid = (raw >> 15) & 1;
		uint32_t busy  = (status >> (SARADC_STA_CH_BUSY_SHIFT + i + 1)) & 1;
		uint32_t mux_val = 0;
		const char *pull_str = "?";
		const char *state_str = "UNKNOWN";
		char pin_str[20];

		if (pinmux_base && i < n_pins) {
			uint32_t pmux = readl(pinmux_base + pins[i].reg_offset);

			mux_val = (pmux & PINMUX_PIN_SEL_MASK) >> PINMUX_PIN_SEL_SHIFT;

#ifdef SARADC_CHIP_BM1688
			{
				uint32_t p_en  = (pmux & PINMUX_P_EN) ? 1 : 0;
				uint32_t pu_sel = (pmux & PINMUX_PU_SEL) ? 1 : 0;

				pull_str = p_en ? (pu_sel ? "up" : "down") : "none";
			}
#else
			{
				uint32_t pu_en = (pmux & PINMUX_PU_EN) ? 1 : 0;
				uint32_t pd_en = (pmux & PINMUX_PD_EN) ? 1 : 0;

				pull_str = pu_en ? "up" : (pd_en ? "down" : "none");
			}
#endif
			state_str = (mux_val == pins[i].adc_mux_val
#ifdef SARADC_CHIP_BM1688
				     && !(pmux & PINMUX_P_EN)
#else
				     && !(pmux & PINMUX_PU_EN) && !(pmux & PINMUX_PD_EN)
#endif
				    ) ? "  READY" : "NOT_RDY";
		}

		snprintf(pin_str, sizeof(pin_str), "%s(%u/%u)",
			 (i < n_pins) ? pins[i].pin_name : "?",
			 mux_val, (i < n_pins) ? pins[i].adc_mux_val : 0);

		*len += scnprintf(buf + *len, PAGE_SIZE - *len,
			"| %4d | %5u | %5u | %4u | %-16s | %4s | %7s |\n",
			i + 1, val, valid, busy,
			pin_str, pull_str, state_str);
	}
	*len += scnprintf(buf + *len, PAGE_SIZE - *len,
		"+------+-------+-------+------+------------------+------+---------+\n");
}

static ssize_t saradc_status_read(struct iio_dev *indio_dev,
				  uintptr_t private,
				  struct iio_chan_spec const *chan, char *buf)
{
	struct cvi_saradc_device *ndev = iio_priv(indio_dev);
	ssize_t len = 0;

	len += scnprintf(buf + len, PAGE_SIZE - len,
		"SARADC Driver Status\n");

	mutex_lock(&ndev->lock);
	saradc_print_domain(ndev, ndev->top_saradc_base_addr,
			    "Top Domain", SARADC_TOP_CHANS,
			    top_pins, ARRAY_SIZE(top_pins),
			    ndev->top_pinmux_base,
			    ndev->top_gap_ns,
			    buf, &len);
	saradc_print_domain(ndev, ndev->rtcsys_saradc_base_addr,
			    "RTC Domain", SARADC_RTC_CHANS,
			    rtc_pins, ARRAY_SIZE(rtc_pins),
			    ndev->rtc_pinmux_base,
			    ndev->rtc_gap_ns,
			    buf, &len);
	mutex_unlock(&ndev->lock);

	return len;
}

/* ------------------------------------------------------------------ */
/*  ext_info: self_test                                                */
/* ------------------------------------------------------------------ */

static int saradc_run_self_test(void __iomem *base, uint32_t vrtsel,
				uint32_t *result)
{
	uint32_t test_val, status;
	int ret;

	test_val = SARADC_TEST_ENTINT | (vrtsel << SARADC_TEST_VRTSEL_SHIFT);
	writel(test_val, base + SARADC_TEST);

	writel(1 << SARADC_CTRL_SEL_SHIFT, base + SARADC_CTRL);
	writel((1 << SARADC_CTRL_SEL_SHIFT) | SARADC_CTRL_EN,
	       base + SARADC_CTRL);

	ret = readl_poll_timeout(base + SARADC_STATUS, status,
				 !(status & SARADC_STA_BUSY),
				 SARADC_POLL_US, SARADC_TIMEOUT_US);

	*result = readl(base + SARADC_CH0_RESULT) & SARADC_RESULT_MASK;

	writel(0, base + SARADC_TEST);

	return ret;
}

static ssize_t saradc_self_test_read(struct iio_dev *indio_dev,
				     uintptr_t private,
				     struct iio_chan_spec const *chan,
				     char *buf)
{
	struct cvi_saradc_device *ndev = iio_priv(indio_dev);
	ssize_t len = 0;
	uint32_t result;
	int ret, i, diff;
	static const struct {
		uint32_t vrtsel;
		uint32_t expected;
		const char *label;
	} tests[] = {
		{ 1, 1024, "0.25*VREF" },
		{ 2, 2048, "0.50*VREF" },
		{ 3, 3072, "0.75*VREF" },
	};

	mutex_lock(&ndev->lock);

	len += scnprintf(buf + len, PAGE_SIZE - len,
		"Self-Test (Top Domain):\n");

	for (i = 0; i < ARRAY_SIZE(tests); i++) {
		ret = saradc_run_self_test(ndev->top_saradc_base_addr,
					  tests[i].vrtsel, &result);
		if (ret) {
			len += scnprintf(buf + len, PAGE_SIZE - len,
				"  %s: TIMEOUT\n", tests[i].label);
			continue;
		}
		diff = (int)result - (int)tests[i].expected;
		len += scnprintf(buf + len, PAGE_SIZE - len,
			"  %s: expected=%u measured=%u diff=%d [%s]\n",
			tests[i].label, tests[i].expected, result, diff,
			(diff >= -50 && diff <= 50) ? "PASS" : "FAIL");
	}

	mutex_unlock(&ndev->lock);
	return len;
}

/* ------------------------------------------------------------------ */
/*  ext_info: config (unified cyc + trim, per-domain)                  */
/* ------------------------------------------------------------------ */

static void saradc_print_config(void __iomem *base, const char *name, u64 gap_ns,
				char *buf, ssize_t *len)
{
	uint32_t cyc_set, trim, force;
	uint32_t settling, sample, clkdiv, compare, freq_khz;

	cyc_set = readl(base + SARADC_CYC_SET);
	trim    = readl(base + SARADC_TRIM_REG) & SARADC_TRIM_MASK;
	force   = readl(base + SARADC_TEST_FORCE);

	settling = (cyc_set >> SARADC_CYC_SETTLING_SHIFT) & 0x1F;
	sample   = (cyc_set >> SARADC_CYC_SAMP_SHIFT) & 0xF;
	clkdiv   = (cyc_set >> SARADC_CYC_CLKDIV_SHIFT) & 0xF;
	compare  = (cyc_set >> SARADC_CYC_COMP_SHIFT) & 0xF;
	freq_khz = 25000 / (1 + clkdiv);

	*len += scnprintf(buf + *len, PAGE_SIZE - *len,
		"\n%s:\n", name);
	*len += scnprintf(buf + *len, PAGE_SIZE - *len,
		"  settling  : %-2u  (%u cycles, range 0..31)\n",
		settling, settling + 1);
	*len += scnprintf(buf + *len, PAGE_SIZE - *len,
		"  sample    : %-2u  (%u cycles, range 0..15)\n",
		sample, sample + 1);
	*len += scnprintf(buf + *len, PAGE_SIZE - *len,
		"  clkdiv    : %-2u  (freq=%u kHz, %u-divide, range 0..15)\n",
		clkdiv, freq_khz, clkdiv + 1);
	*len += scnprintf(buf + *len, PAGE_SIZE - *len,
		"  compare   : %-2u  (%u cycles, range 0..15)\n",
		compare, compare + 1);
	*len += scnprintf(buf + *len, PAGE_SIZE - *len,
		"  trim      : %-2u  (range 0..31)\n", trim);
	*len += scnprintf(buf + *len, PAGE_SIZE - *len,
		"  gap       : %llu ns  (range 0..1s)\n",
		(unsigned long long)gap_ns);
	*len += scnprintf(buf + *len, PAGE_SIZE - *len,
		"  force     : %s", (force & SARADC_FORCE_EN) ? "on" : "off");
	if (force & SARADC_FORCE_EN)
		*len += scnprintf(buf + *len, PAGE_SIZE - *len,
			" (value=%u)", force & SARADC_FORCE_RESULT_MASK);
	*len += scnprintf(buf + *len, PAGE_SIZE - *len, "\n");
}

static void saradc_print_io_config(struct cvi_saradc_device *ndev,
				   char *buf, ssize_t *len)
{
	int i;

	*len += scnprintf(buf + *len, PAGE_SIZE - *len, "\nIO Config:\n");
	*len += scnprintf(buf + *len, PAGE_SIZE - *len, "  top io    :");
	for (i = 0; i < SARADC_TOP_CHANS; i++)
		*len += scnprintf(buf + *len, PAGE_SIZE - *len, " ch%d=%s",
				  i + 1, ndev->top_pinmux_io_on[i] ? "on" : "off");
	*len += scnprintf(buf + *len, PAGE_SIZE - *len, "\n");

	*len += scnprintf(buf + *len, PAGE_SIZE - *len, "  rtc io    :");
	for (i = 0; i < SARADC_RTC_CHANS; i++)
		*len += scnprintf(buf + *len, PAGE_SIZE - *len, " ch%d=%s",
				  i + 1, ndev->rtc_pinmux_io_on[i] ? "on" : "off");
	*len += scnprintf(buf + *len, PAGE_SIZE - *len, "\n");
}

static ssize_t saradc_config_read(struct iio_dev *indio_dev,
				  uintptr_t private,
				  struct iio_chan_spec const *chan, char *buf)
{
	struct cvi_saradc_device *ndev = iio_priv(indio_dev);
	ssize_t len = 0;

	len += scnprintf(buf + len, PAGE_SIZE - len,
		"Usage: echo \"{top|rtc|all} {settling|sample|clkdiv|compare|trim} <value>\" > config\n");
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"       echo \"{top|rtc} io {1|2|3|all} {on|off}\" > config\n");
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"       echo \"{top|rtc|all} gap <time>{ns|us|ms|s}\" > config\n");
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"       echo \"{top|rtc|all} force {off|<0..4095>}\" > config\n");

	mutex_lock(&ndev->lock);
	saradc_print_config(ndev->top_saradc_base_addr, "Top Domain",
			    ndev->top_gap_ns, buf, &len);
	saradc_print_config(ndev->rtcsys_saradc_base_addr, "RTC Domain",
			    ndev->rtc_gap_ns, buf, &len);
	saradc_print_io_config(ndev, buf, &len);
	mutex_unlock(&ndev->lock);

	return len;
}

static ssize_t saradc_config_write(struct iio_dev *indio_dev,
				   uintptr_t private,
				   struct iio_chan_spec const *chan,
				   const char *buf, size_t len)
{
	struct cvi_saradc_device *ndev = iio_priv(indio_dev);
	char domain[8], param[16];
	char io_ch[8], io_sw[8];
	char time_str[20];
	unsigned int val;
	void __iomem *targets[2];
	int n_targets = 0, i, ret;
	uint32_t shift, width, max_val, mask;
	int is_trim = 0;
	u64 gap_ns;

	/* {top|rtc} io {1|2|3|all} {on|off} */
	ret = sscanf(buf, "%7s io %7s %7s", domain, io_ch, io_sw);
	if (ret == 3) {
		bool do_top = false, do_rtc = false, enable;
		int ch;

		if (!strcmp(domain, "top"))
			do_top = true;
		else if (!strcmp(domain, "rtc"))
			do_rtc = true;
		else if (!strcmp(domain, "all")) {
			do_top = true;
			do_rtc = true;
		} else
			return -EINVAL;

		if (!strcmp(io_sw, "on"))
			enable = true;
		else if (!strcmp(io_sw, "off"))
			enable = false;
		else
			return -EINVAL;

		mutex_lock(&ndev->lock);
		if (!strcmp(io_ch, "all")) {
			if (do_top) {
				for (ch = 1; ch <= SARADC_TOP_CHANS; ch++) {
					ret = saradc_io_channel_set_locked(ndev, true, ch,
									   enable);
					if (ret)
						break;
				}
			}

			if (!ret && do_rtc) {
				for (ch = 1; ch <= SARADC_RTC_CHANS; ch++) {
					ret = saradc_io_channel_set_locked(ndev, false, ch,
									   enable);
					if (ret)
						break;
				}
			}
		} else {
			ret = kstrtoint(io_ch, 0, &ch);
			if (!ret && ch <= 0)
				ret = -EINVAL;
			if (!ret && do_top)
				ret = saradc_io_channel_set_locked(ndev, true, ch,
								   enable);
			if (!ret && do_rtc)
				ret = saradc_io_channel_set_locked(ndev, false, ch,
								   enable);
		}
		mutex_unlock(&ndev->lock);
		return ret ? ret : len;
	}

	/* {top|rtc|all} force {off|<0..4095>} */
	ret = sscanf(buf, "%7s force %15s", domain, param);
	if (ret == 2) {
		uint32_t force_reg;

		if (!strcmp(domain, "top")) {
			targets[n_targets++] = ndev->top_saradc_base_addr;
		} else if (!strcmp(domain, "rtc")) {
			targets[n_targets++] = ndev->rtcsys_saradc_base_addr;
		} else if (!strcmp(domain, "all")) {
			targets[n_targets++] = ndev->top_saradc_base_addr;
			targets[n_targets++] = ndev->rtcsys_saradc_base_addr;
		} else {
			return -EINVAL;
		}

		if (sysfs_streq(param, "off")) {
			force_reg = 0;
		} else {
			ret = kstrtouint(param, 0, &val);
			if (ret)
				return -EINVAL;
			if (val > SARADC_FORCE_RESULT_MASK)
				return -EINVAL;
			force_reg = SARADC_FORCE_EN | val;
		}

		mutex_lock(&ndev->lock);
		for (i = 0; i < n_targets; i++)
			writel(force_reg, targets[i] + SARADC_TEST_FORCE);
		mutex_unlock(&ndev->lock);
		return len;
	}

	/* {top|rtc|all} gap <time>{ns|us|ms|s} */
	ret = sscanf(buf, "%7s %15s %19s", domain, param, time_str);
	if (ret == 3 && !strcmp(param, "gap")) {
		ret = saradc_parse_time_to_ns(time_str, &gap_ns);
		if (ret)
			return -EINVAL;
		if (gap_ns > NSEC_PER_SEC)
			return -ERANGE;

		mutex_lock(&ndev->lock);
		if (!strcmp(domain, "top")) {
			ndev->top_gap_ns = gap_ns;
		} else if (!strcmp(domain, "rtc")) {
			ndev->rtc_gap_ns = gap_ns;
		} else if (!strcmp(domain, "all")) {
			ndev->top_gap_ns = gap_ns;
			ndev->rtc_gap_ns = gap_ns;
		} else {
			mutex_unlock(&ndev->lock);
			return -EINVAL;
		}
		mutex_unlock(&ndev->lock);
		return len;
	}

	/* {top|rtc|all} {settling|sample|clkdiv|compare|trim} <value> */
	ret = sscanf(buf, "%7s %15s %u", domain, param, &val);
	if (ret != 3)
		return -EINVAL;

	if (!strcmp(domain, "top")) {
		targets[n_targets++] = ndev->top_saradc_base_addr;
	} else if (!strcmp(domain, "rtc")) {
		targets[n_targets++] = ndev->rtcsys_saradc_base_addr;
	} else if (!strcmp(domain, "all")) {
		targets[n_targets++] = ndev->top_saradc_base_addr;
		targets[n_targets++] = ndev->rtcsys_saradc_base_addr;
	} else {
		return -EINVAL;
	}

	if (!strcmp(param, "settling")) {
		shift = SARADC_CYC_SETTLING_SHIFT; width = 5;
	} else if (!strcmp(param, "sample")) {
		shift = SARADC_CYC_SAMP_SHIFT; width = 4;
	} else if (!strcmp(param, "clkdiv")) {
		shift = SARADC_CYC_CLKDIV_SHIFT; width = 4;
	} else if (!strcmp(param, "compare")) {
		shift = SARADC_CYC_COMP_SHIFT; width = 4;
	} else if (!strcmp(param, "trim")) {
		is_trim = 1; width = 5;
	} else {
		return -EINVAL;
	}

	max_val = (1u << width) - 1;
	if (val > max_val)
		return -EINVAL;

	mutex_lock(&ndev->lock);
	for (i = 0; i < n_targets; i++) {
		if (is_trim) {
			writel(val, targets[i] + SARADC_TRIM_REG);
		} else {
			uint32_t cyc_set;

			mask = max_val << shift;
			cyc_set = readl(targets[i] + SARADC_CYC_SET);
			cyc_set = (cyc_set & ~mask) | (val << shift);
			writel(cyc_set, targets[i] + SARADC_CYC_SET);
		}
	}
	mutex_unlock(&ndev->lock);

	return len;
}

/* ------------------------------------------------------------------ */
/*  IIO channel / info definitions                                     */
/* ------------------------------------------------------------------ */

static const struct iio_chan_spec_ext_info saradc_ext_info[] = {
	{
		.name = "status",
		.read = saradc_status_read,
		.shared = IIO_SHARED_BY_ALL,
	},
	{
		.name = "config",
		.read = saradc_config_read,
		.write = saradc_config_write,
		.shared = IIO_SHARED_BY_ALL,
	},
	{
		.name = "self_test",
		.read = saradc_self_test_read,
		.shared = IIO_SHARED_BY_ALL,
	},
	{ },
};

#define	SARADC_CHAN_VOLTAGE(lval, idx, addr)			\
	{							\
		lval.type = IIO_VOLTAGE;			\
		lval.channel = idx;				\
		lval.indexed = 1;				\
		lval.address = addr;				\
		lval.info_mask_separate = BIT(IIO_CHAN_INFO_RAW);\
		lval.scan_index = idx;				\
		lval.scan_type.sign = 'u';			\
		lval.scan_type.realbits = 12;			\
		lval.scan_type.storagebits = 16;		\
		lval.scan_type.endianness = IIO_LE;		\
		lval.ext_info = saradc_ext_info;		\
	}

static const struct iio_info saradc_info = {
	.read_raw = saradc_read_raw,
};

/* ------------------------------------------------------------------ */
/*  Probe / Remove                                                     */
/* ------------------------------------------------------------------ */

static int cvi_saradc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cvi_saradc_device *ndev;
	struct resource *res;
	struct iio_dev *indio_dev;
	int ret, i;

	indio_dev = devm_iio_device_alloc(dev, sizeof(*ndev));
	if (!indio_dev)
		return -ENOMEM;

	ndev = iio_priv(indio_dev);
	ndev->dev = dev;
	ndev->private_data = pdev;

	platform_set_drvdata(pdev, indio_dev);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
					   "top_domain_saradc");
	if (!res) {
		dev_err(dev, "missing top_domain_saradc resource\n");
		return -ENXIO;
	}
	ndev->top_saradc_base_addr = devm_ioremap_resource(dev, res);
	if (IS_ERR(ndev->top_saradc_base_addr))
		return PTR_ERR(ndev->top_saradc_base_addr);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
					   "rtc_domain_saradc");
	if (!res) {
		dev_err(dev, "missing rtc_domain_saradc resource\n");
		return -ENXIO;
	}
	ndev->rtcsys_saradc_base_addr = devm_ioremap_resource(dev, res);
	if (IS_ERR(ndev->rtcsys_saradc_base_addr))
		return PTR_ERR(ndev->rtcsys_saradc_base_addr);

	for (i = 1; i <= SARADC_CHAN_NUM; ++i) {
		int hw = get_hw_channel(i);

		SARADC_CHAN_VOLTAGE(ndev->iio_channels[i - 1], i,
				   SARADC_CH1_RESULT + (hw - 1) * 4);
	}

	indio_dev->name = "cvi_saradc";
	indio_dev->info = &saradc_info;
	indio_dev->dev.parent = dev;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = ndev->iio_channels;
	indio_dev->num_channels = SARADC_CHAN_NUM;

	ndev->clk_saradc = devm_clk_get(dev, "clk_saradc");
	if (IS_ERR(ndev->clk_saradc)) {
		dev_err(dev, "failed to get clk_saradc\n");
		ndev->clk_saradc = NULL;
	}
	platform_saradc_clk_init(ndev);

	ndev->rst_saradc = devm_reset_control_get(dev, "res_saradc");
	if (IS_ERR(ndev->rst_saradc)) {
		dev_err(dev, "failed to get res_saradc\n");
		ndev->rst_saradc = NULL;
	}

	ndev->top_gap_ns = 10ULL * 1000ULL;
	ndev->rtc_gap_ns = 10ULL * 1000ULL;

	if (PINMUX_G_TOP_SARADC_BASE) {
		ndev->top_pinmux_base = devm_ioremap(dev, PINMUX_G_TOP_SARADC_BASE,
						     PINMUX_MAP_SIZE);
		if (!ndev->top_pinmux_base)
			dev_warn(dev, "cannot map top pinmux regs, io_config disabled\n");
	}

	if (PINMUX_G_RTC_SARADC_BASE) {
		ndev->rtc_pinmux_base = devm_ioremap(dev, PINMUX_G_RTC_SARADC_BASE,
						     PINMUX_MAP_SIZE);
		if (!ndev->rtc_pinmux_base)
			dev_warn(dev, "cannot map rtc pinmux regs, io_config disabled\n");
	}

	mutex_init(&ndev->lock);

	ret = iio_device_register(indio_dev);
	if (ret) {
		dev_err(dev, "failed to register iio device: %d\n", ret);
		return ret;
	}

	dev_info(dev, "SARADC probed (version top=0x%08X rtc=0x%08X)\n",
		 readl(ndev->top_saradc_base_addr + SARADC_VERSION),
		 readl(ndev->rtcsys_saradc_base_addr + SARADC_VERSION));
	return 0;
}

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
static int cvi_saradc_remove(struct platform_device *pdev)
{
	struct iio_dev *indio_dev = platform_get_drvdata(pdev);
	struct cvi_saradc_device *ndev = iio_priv(indio_dev);

	iio_device_unregister(indio_dev);
	platform_saradc_clk_deinit(ndev);
	return 0;
}
#else
static void cvi_saradc_remove(struct platform_device *pdev)
{
	struct iio_dev *indio_dev = platform_get_drvdata(pdev);
	struct cvi_saradc_device *ndev = iio_priv(indio_dev);

	iio_device_unregister(indio_dev);
	platform_saradc_clk_deinit(ndev);
}
#endif

/* ------------------------------------------------------------------ */
/*  PM                                                                 */
/* ------------------------------------------------------------------ */

#ifdef CONFIG_PM_SLEEP
static int saradc_cv_suspend(struct device *dev)
{
	struct cvi_saradc_device *ndev = iio_priv(dev_get_drvdata(dev));

	memcpy_fromio(ndev->saradc_saved_top_regs,
		      ndev->top_saradc_base_addr, SARADC_SAVE_SIZE);
	memcpy_fromio(ndev->saradc_saved_rtc_regs,
		      ndev->rtcsys_saradc_base_addr, SARADC_SAVE_SIZE);
	platform_saradc_clk_deinit(ndev);
	return 0;
}

static int saradc_cv_resume(struct device *dev)
{
	struct cvi_saradc_device *ndev = iio_priv(dev_get_drvdata(dev));

	platform_saradc_clk_init(ndev);
	memcpy_toio(ndev->top_saradc_base_addr,
		    ndev->saradc_saved_top_regs, SARADC_SAVE_SIZE);
	memcpy_toio(ndev->rtcsys_saradc_base_addr,
		    ndev->saradc_saved_rtc_regs, SARADC_SAVE_SIZE);
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(saradc_cv_pm_ops, saradc_cv_suspend,
			 saradc_cv_resume);

/* ------------------------------------------------------------------ */
/*  Platform driver                                                    */
/* ------------------------------------------------------------------ */

static const struct of_device_id cvi_saradc_match[] = {
	{ .compatible = "cvitek,saradc" },
	{},
};
MODULE_DEVICE_TABLE(of, cvi_saradc_match);

static struct platform_driver cvi_saradc_driver = {
	.probe  = cvi_saradc_probe,
	.remove = cvi_saradc_remove,
	.driver = {
		.owner          = THIS_MODULE,
		.name           = "cvi-saradc",
		.pm             = &saradc_cv_pm_ops,
		.of_match_table = cvi_saradc_match,
	},
};
module_platform_driver(cvi_saradc_driver);

MODULE_AUTHOR("zixun.li <zixun.li@sophgo.com>");
MODULE_DESCRIPTION("Cvitek SoC saradc driver");
MODULE_LICENSE("GPL");
