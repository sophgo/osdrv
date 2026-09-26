#ifndef	__CVI_SARADC_H__
#define	__CVI_SARADC_H__

#include <linux/cdev.h>
#include <linux/iio/iio.h>
#include <linux/mutex.h>
#include <linux/types.h>

/* Register offsets */
#define	SARADC_VERSION		0x000
#define	SARADC_CTRL		0x004
#define	SARADC_STATUS		0x008
#define	SARADC_CYC_SET		0x00C
#define	SARADC_CH0_RESULT	0x010
#define	SARADC_CH1_RESULT	0x014
#define	SARADC_CH2_RESULT	0x018
#define	SARADC_CH3_RESULT	0x01C
#define	SARADC_INTR_EN		0x020
#define	SARADC_INTR_CLR		0x024
#define	SARADC_INTR_STA		0x028
#define	SARADC_INTR_RAW		0x02C
#define	SARADC_TEST		0x030
#define	SARADC_TRIM_REG		0x034
#define	SARADC_PERIOD		0x038
#define	SARADC_TEST_FORCE	0x040

/* SARADC_CTRL (0x004) */
#define	SARADC_CTRL_EN		BIT(0)
#define	SARADC_CTRL_CONT	BIT(1)
#define	SARADC_CTRL_PERIODIC	BIT(2)
#define	SARADC_CTRL_SEL_SHIFT	4
#define	SARADC_CTRL_SEL_MASK	(0xF << SARADC_CTRL_SEL_SHIFT)

/* SARADC_STATUS (0x008) */
#define	SARADC_STA_BUSY		BIT(0)
#define	SARADC_STA_CH_BUSY_SHIFT  4
#define	SARADC_STA_CH_VALID_SHIFT 8
#define	SARADC_STA_FSM_SHIFT	  16
#define	SARADC_STA_CYCLE_SHIFT	  20

/* SARADC_CYC_SET (0x00C) */
#define	SARADC_CYC_SETTLING_SHIFT 0
#define	SARADC_CYC_SAMP_SHIFT	  8
#define	SARADC_CYC_CLKDIV_SHIFT	  12
#define	SARADC_CYC_COMP_SHIFT	  16

/* SARADC_TEST (0x030) */
#define	SARADC_TEST_ENTINT	BIT(0)
#define	SARADC_TEST_DIFF	BIT(1)
#define	SARADC_TEST_VREFSEL	BIT(2)
#define	SARADC_TEST_TSEL_SHIFT	4
#define	SARADC_TEST_VRTSEL_SHIFT 6
#define	SARADC_TEST_DOUT_SHIFT	16

/* SARADC_TRIM (0x034) */
#define	SARADC_TRIM_MASK	0x1F

/* SARADC_PERIOD (0x038) */
#define	SARADC_PERIOD_CYCLE_MASK	0xFFFFFF
#define	SARADC_PERIOD_PREDIV_SHIFT	24

/* SARADC_TEST_FORCE (0x040) */
#define	SARADC_FORCE_RESULT_MASK 0xFFF
#define	SARADC_FORCE_EN		 BIT(15)

/* Result register common bits */
#define	SARADC_RESULT_MASK	0xFFF
#define	SARADC_RESULT_VALID	BIT(15)

/* Channel topology */
#define	SARADC_CHAN_NUM		5
#define	SARADC_TOP_CHANS	3
#define	SARADC_RTC_CHANS	2

/* Polling / timeout */
#define	SARADC_POLL_US		10
#define	SARADC_TIMEOUT_US	10000

/* Save/restore covers 0x000 – 0x040 inclusive */
#define	SARADC_SAVE_SIZE	0x044
#define	SARADC_SAVE_NUM		(SARADC_SAVE_SIZE / 4)

/* ---- Chip-specific pinmux configuration ---- */
#ifdef SARADC_CHIP_BM1688

#define	PINMUX_G12_BASE		0x28104c00
#define	PINMUX_G7_BASE		0x05027000
#define	PINMUX_G_TOP_SARADC_BASE	PINMUX_G12_BASE
#define	PINMUX_G_RTC_SARADC_BASE	PINMUX_G7_BASE
#define	PINMUX_MAP_SIZE		0x40

#define	PINMUX_P_EN		BIT(0)
#define	PINMUX_PU_SEL		BIT(1)
#define	PINMUX_PIN_SEL_SHIFT	4
#define	PINMUX_PIN_SEL_MASK	(0xF << PINMUX_PIN_SEL_SHIFT)
#define	PINMUX_IE		BIT(14)
#define	PINMUX_ADC_CFG_MASK	(PINMUX_P_EN | PINMUX_PU_SEL | \
				 PINMUX_PIN_SEL_MASK | PINMUX_IE)

#else /* !SARADC_CHIP_BM1688  —  cv84x6 / cv84x2 */

/*
 * Top-domain ADC pins: ADC1/2/3 in pinmux group G3.
 * RTC-domain SAR pins: PWR_SAR1(PWR_GPIO1), PWR_SAR2(PWR_GPIO2) in group G8.
 */
#define	PINMUX_G3_BASE		0x28104300
#define	PINMUX_G8_BASE		0x05027800
#define	PINMUX_G_TOP_SARADC_BASE	PINMUX_G3_BASE
#define	PINMUX_G_RTC_SARADC_BASE	PINMUX_G8_BASE
#define	PINMUX_MAP_SIZE		0x100

/*
 * cv84x6 pinmux IO control register bit fields:
 *   bit  2 : PU_EN  (pull-up enable)
 *   bit  3 : PD_EN  (pull-down enable)
 *   bit 7:4: PIN_SEL (function mux)
 *   bit 14 : IE     (input enable)
 */
#define	PINMUX_PU_EN		BIT(2)
#define	PINMUX_PD_EN		BIT(3)
#define	PINMUX_PIN_SEL_SHIFT	4
#define	PINMUX_PIN_SEL_MASK	(0xF << PINMUX_PIN_SEL_SHIFT)
#define	PINMUX_IE		BIT(14)
#define	PINMUX_ADC_CFG_MASK	(PINMUX_PU_EN | PINMUX_PD_EN | \
				 PINMUX_PIN_SEL_MASK | PINMUX_IE)

#endif /* SARADC_CHIP_BM1688 */

struct cvi_saradc_device {
	struct device *dev;
	struct reset_control *rst_saradc;
	struct iio_chan_spec iio_channels[SARADC_CHAN_NUM];
	struct clk *clk_saradc;
	void __iomem *top_saradc_base_addr;
	void __iomem *rtcsys_saradc_base_addr;
	void __iomem *top_pinmux_base;
	void __iomem *rtc_pinmux_base;
	uint32_t top_pinmux_saved[SARADC_TOP_CHANS];
	uint32_t rtc_pinmux_saved[SARADC_RTC_CHANS];
	bool top_pinmux_saved_valid[SARADC_TOP_CHANS];
	bool rtc_pinmux_saved_valid[SARADC_RTC_CHANS];
	bool top_pinmux_io_on[SARADC_TOP_CHANS];
	bool rtc_pinmux_io_on[SARADC_RTC_CHANS];
	u64 top_gap_ns;
	u64 rtc_gap_ns;
	struct mutex lock;
	void *private_data;
	uint32_t saradc_saved_top_regs[SARADC_SAVE_NUM];
	uint32_t saradc_saved_rtc_regs[SARADC_SAVE_NUM];
};

#endif /* __CVI_SARADC_H__ */
