// SPDX-License-Identifier: GPL-2.0
/*
 * cv84x2_dvfs_pmbus.c - 84x2 DVFS PMBus-based voltage + PLL freq control
 *
 * Supports JWH63782 (JoulWatt) and MP2985B (MPS) PMICs.
 * Auto-detects PMIC type at probe via MFR_ID (0x99) register:
 *   JWH63782: 0x4A57 ("JW")
 *   MP2985B:  0x504D ("MP")
 *
 * Safety: volt-first ordering.
 *   Raise: volt -> freq  (safe: over-volt at low freq is harmless)
 *   Lower: freq -> volt  (safe: high volt at low freq is harmless)
 *
 * Hardware paths (direct ioremap, bypasses Linux i2c subsystem):
 *   - I2C controller 0x05036000 (PMBUS0, Synopsys DesignWare)
 *   - pinmux 0x05027800 (PWR_PMBUS dedicated pin group)
 *   - VOLT_ARB 0x05032000 (reg_power_comb_mode for rail merge info)
 *   - CRG 0x28100000 (mpll0/mpll1 ctrl + PLL status + clk mux, freq path)
 */

#include <linux/err.h>
#include <linux/bitops.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/mutex.h>

/* ===== SoC physical base addresses ===== */
#define CV84X2_PMBUS_I2C_BASE	0x05036000UL	/* PMBUS0 controller */
#define CV84X2_PINMUX_BASE	0x05027800UL
#define CV84X2_VOLT_ARB_BASE	0x05032000UL
#define CV84X2_CRG_BASE		0x28100000UL	/* CRG: mpll ctrl + PLL status + clk mux */
#define CRG_MPLL0_CTRL_OFF	0x0220		/* CPU mpll0 ctrl (fbdiv/refdiv/postdiv) */
#define CRG_MPLL1_CTRL_OFF	0x0224		/* TPU mpll1 ctrl */
#define CRG_PLL_STATUS_OFF	0x001c		/* PLL lock status (mpll0/1 lock bits) */
#define CRG_CLK_SEL_0_OFF	0x2020		/* bit0=CPU mux(mpll0/mpll1), bit2=TPU mux */

/* pinmux offsets for PWR_PMBUS */
#define PINMUX_PMBUS_ALERT_OFF	0x50
#define PINMUX_PMBUS_SCL_OFF	0x54
#define PINMUX_PMBUS_SDA_OFF	0x58
#define PINMUX_VAL_I2C		0x3804

/* VOLT_ARB reg_power_comb_mode offset */
#define VOLT_ARB_POWER_COMB_OFF	0x0

/* ===== DesignWare I2C registers ===== */
#define DW_IC_CON		0x00
#define DW_IC_TAR		0x04
#define DW_IC_DATA_CMD		0x10
#define DW_IC_SS_SCL_HCNT	0x14
#define DW_IC_SS_SCL_LCNT	0x18
#define DW_IC_FS_SCL_HCNT	0x1c
#define DW_IC_FS_SCL_LCNT	0x20
#define DW_IC_INTR_MASK		0x30
#define DW_IC_RAW_INTR_STAT	0x34
#define DW_IC_RX_TL		0x38
#define DW_IC_TX_TL		0x3c
#define DW_IC_CLR_STOP_DET	0x60
#define DW_IC_ENABLE		0x6c
#define DW_IC_STATUS		0x70
#define DW_IC_ENABLE_STATUS	0x9c
#define DW_IC_TX_ABRT_SOURCE	0x80
#define DW_IC_SDA_HOLD		0x7c

#define DW_IC_CON_SD		0x0040
#define DW_IC_CON_RE		0x0020
#define DW_IC_CON_SPD_MSK	0x0006
#define DW_IC_CON_SPD_FS	0x0004
#define DW_IC_CON_MM		0x0001
#define DW_IC_ENABLE_B		0x0001
#define DW_IC_CMD_READ		0x0100
#define DW_IC_STOP		0x0200
#define DW_IC_STATUS_RFNE	0x0008
#define DW_IC_STATUS_TFNF	0x0002
#define DW_IC_STOP_DET		0x0200
#define DW_IC_TX_ABRT		0x0040

/* DesignWare input clock 100MHz */
#define DW_I2C_INPUT_CLK_HZ	100000000UL
#define DW_I2C_BUS_SPEED	400000UL

/* ===== PMBus command codes ===== */
#define PMBUS_PAGE		0x00
#define PMBUS_OPERATION		0x01
#define PMBUS_VOUT_MODE		0x20
#define PMBUS_VOUT_COMMAND	0x21
#define PMBUS_READ_VOUT		0x8B
#define PMBUS_STATUS_VOUT	0x7A
#define PMBUS_MFR_ID		0x99

/* MPS-specific registers */
#define PMBUS_MFR_VR_MULTI_CONFIG	0x0D	/* page2: bit[4] 0=10mV, 1=5mV */
#define PMBUS_MFR_VR_CONFIG_IMON	0x0E	/* page2: bit[0] 0=unity, 1=half gain */
#define PMBUS_VID_MODE_DC_LL		0x03	/* page2: bit[13] PMBUS_BOOT_EN_R1 */
#define VID_MODE_PMBUS_EN_BIT		13

/* JWH-specific registers */
#define JWH_SVID_PROTOCOL_ID_VIDOMAX	0xC2	/* page5: hi byte 0=5mV, 1=10mV */
#define JWH_MFR_VR_CONFIG		0x22	/* page5: bit[7:6]=GRS */

/* PMBus 7-bit address; DTS pmic-addr may override */
#define PMIC_ADDR_DEFAULT	0x20

/* Voltage limits (mV) */
#define VOLT_MIN_MV_DEFAULT	600
#define VOLT_MAX_MV_DEFAULT	1200

/* Default rail voltage applied once at probe (mV). Probe-time write is
 * best-effort: failure only warns, never aborts probe.
 */
#define DEFAULT_VOLT_MV		940

/* PLL: 25MHz step, fbdiv range 16-320 */
#define PLL_MHZ_STEP		25
#define PLL_FBDIV_MIN		16
#define PLL_FBDIV_MAX		320

/* vote table: 4-bit id, 16 entries */
#define DVFS_VOTE_MAX	16
#define DVFS_VOTE_NONE	0xff

enum pmic_type {
	PMIC_JWH63782 = 0,
	PMIC_MP2985B,
	PMIC_RENESAS,		/* TODO: Renesas PMIC, reserved, volt/freq not implemented yet */
};

struct dvfs_vote_point {
	u32 cpu_volt_mv;
	u32 cpu_freq_mhz;
	u32 tpu_freq_mhz;
	u8 valid;
};

struct cv84x2_dvfs_pmbus {
	struct device *dev;
	struct mutex lock;	/* serialize DVFS voltage/freq operations */

	struct clk *clk_cpu;
	struct clk *clk_tpu;

	void __iomem *i2c_base;
	void __iomem *pinmux_base;
	void __iomem *arb_base;
	void __iomem *crg_base;

	u8 pmic_addr;
	enum pmic_type pmic;

	/* JWH voltage params */
	u8 jwh_grs;		/* page5 22h[7:6]: 0=1.0, 1=0.6, 2=0.4 */
	u8 jwh_vid_step_mv;	/* page5 C2h[8]: 0=5mV, 1=10mV, default 5 */

	/* MPS voltage params */
	u8 mps_half_gain;	/* 0Eh page2 bit[0]: 0=unity, 1=half */
	u8 mps_vid_step_mv;	/* 0Dh page2 bit[4]: 0=10mV, 1=5mV, default 10 */
	u8 mps_vout_mode;	/* VOUT_MODE: 0=Linear16, 1=Direct(1mV/LSB), 2=VID */
	int mps_vout_exp;	/* Linear16 exponent, only for vout_mode=0, default -9 */
	u8 mps_pmbus_enabled;	/* true if OPERATION already switched to PMBus mode */

	u8 cpu_rail_page;
	u8 tpu_rail_page;

	struct dvfs_vote_point vote_tbl[DVFS_VOTE_MAX];
	u8 active_id;
};

/* ========== DesignWare I2C (polling) ========== */

static inline void dw_writel(struct cv84x2_dvfs_pmbus *d, u32 off, u32 val)
{
	iowrite32(val, d->i2c_base + off);
}

static inline u32 dw_readl(struct cv84x2_dvfs_pmbus *d, u32 off)
{
	return ioread32(d->i2c_base + off);
}

static void dw_i2c_enable(struct cv84x2_dvfs_pmbus *d, bool enable)
{
	u32 ena = enable ? DW_IC_ENABLE_B : 0;
	int timeout = 100;

	do {
		dw_writel(d, DW_IC_ENABLE, ena);
		if ((dw_readl(d, DW_IC_ENABLE_STATUS) & DW_IC_ENABLE_B) == ena)
			return;
		usleep_range(25, 50);
	} while (timeout--);

	dev_warn_once(d->dev, "I2C enable/disable timeout\n");
}

static void dw_i2c_set_bus_speed(struct cv84x2_dvfs_pmbus *d)
{
	unsigned int clk_mhz = DW_I2C_INPUT_CLK_HZ / 1000000;
	unsigned int hcnt, lcnt, cntl;

	dw_i2c_enable(d, false);

	hcnt = (clk_mhz * 600) / 1000;
	lcnt = (clk_mhz * 1300) / 1000;
	dw_writel(d, DW_IC_FS_SCL_HCNT, hcnt);
	dw_writel(d, DW_IC_FS_SCL_LCNT, lcnt);

	cntl = dw_readl(d, DW_IC_CON) & ~DW_IC_CON_SPD_MSK;
	cntl |= DW_IC_CON_SPD_FS | DW_IC_CON_RE | DW_IC_CON_MM;
	dw_writel(d, DW_IC_CON, cntl);

	dw_i2c_enable(d, true);
}

static void dw_i2c_init(struct cv84x2_dvfs_pmbus *d)
{
	dw_i2c_enable(d, false);
	dw_writel(d, DW_IC_CON, DW_IC_CON_SD | DW_IC_CON_SPD_FS | DW_IC_CON_MM);
	dw_writel(d, DW_IC_RX_TL, 0);
	dw_writel(d, DW_IC_TX_TL, 0);
	dw_i2c_set_bus_speed(d);
	dw_i2c_enable(d, true);
}

static void dw_i2c_set_target(struct cv84x2_dvfs_pmbus *d, u8 addr)
{
	dw_i2c_enable(d, false);
	dw_writel(d, DW_IC_TAR, addr);
	dw_i2c_enable(d, true);
}

#define I2C_M_RD 0x0001

struct dvfs_i2c_msg {
	u8 addr;
	u16 flags;
	u16 len;
	u8 *buf;
};

static int dw_i2c_xfer(struct cv84x2_dvfs_pmbus *d, struct dvfs_i2c_msg *msgs, int nmsgs)
{
	int total_read = 0, total_write = 0;
	int i, j, sent = 0, got = 0, wait = 0;
	u8 rxbuf[16];

	if (nmsgs <= 0)
		return 0;

	for (i = 0; i < nmsgs; i++) {
		if (msgs[i].flags & I2C_M_RD)
			total_read += msgs[i].len;
		else
			total_write += msgs[i].len;
	}
	if (total_read > (int)sizeof(rxbuf))
		return -E2BIG;

	dw_i2c_set_target(d, msgs[0].addr);

	for (i = 0; i < nmsgs; i++) {
		bool is_last = (i == nmsgs - 1);

		for (j = 0; j < msgs[i].len; j++) {
			u32 cmd;

			if (msgs[i].flags & I2C_M_RD) {
				cmd = DW_IC_CMD_READ;
				if (is_last && j == msgs[i].len - 1)
					cmd |= DW_IC_STOP;
			} else {
				cmd = msgs[i].buf[j];
				if (total_read == 0 && is_last &&
				    j == msgs[i].len - 1)
					cmd |= DW_IC_STOP;
			}

			wait = 0;
			while (!(dw_readl(d, DW_IC_STATUS) & DW_IC_STATUS_TFNF)) {
				if (dw_readl(d, DW_IC_RAW_INTR_STAT) & DW_IC_TX_ABRT) {
					u32 abrt = dw_readl(d, DW_IC_TX_ABRT_SOURCE);

					dev_warn(d->dev, "I2C TX_ABRT_SOURCE=0x%08x (addr=0x%02x last_cmd=%d)\n",
						 abrt, msgs[0].addr, sent);
					return -ENODEV;
				}
				if (++wait > 5000)
					return -ETIMEDOUT;
				usleep_range(10, 20);
			}
			dw_writel(d, DW_IC_DATA_CMD, cmd);
			sent++;
		}
	}

	while (got < total_read) {
		if (dw_readl(d, DW_IC_RAW_INTR_STAT) & DW_IC_TX_ABRT) {
			u32 abrt = dw_readl(d, DW_IC_TX_ABRT_SOURCE);

			dev_warn(d->dev, "I2C read TX_ABRT_SOURCE=0x%08x (addr=0x%02x)\n",
				 abrt, msgs[0].addr);
			return -ENODEV;
		}
		if (dw_readl(d, DW_IC_STATUS) & DW_IC_STATUS_RFNE) {
			rxbuf[got++] = (u8)dw_readl(d, DW_IC_DATA_CMD);
			continue;
		}
		if (++wait > 5000)
			return -ETIMEDOUT;
		usleep_range(10, 20);
	}

	wait = 0;
	while (!(dw_readl(d, DW_IC_RAW_INTR_STAT) & DW_IC_STOP_DET)) {
		if (++wait > 5000)
			break;
		usleep_range(10, 20);
	}
	(void)dw_readl(d, DW_IC_CLR_STOP_DET);

	for (i = 0, got = 0; i < nmsgs; i++) {
		if (!(msgs[i].flags & I2C_M_RD))
			continue;
		for (j = 0; j < msgs[i].len; j++)
			msgs[i].buf[j] = rxbuf[got++];
	}

	return 0;
}

/* ========== pinmux ========== */

static void pinmux_to_i2c(struct cv84x2_dvfs_pmbus *d)
{
	iowrite32(PINMUX_VAL_I2C, d->pinmux_base + PINMUX_PMBUS_ALERT_OFF);
	iowrite32(PINMUX_VAL_I2C, d->pinmux_base + PINMUX_PMBUS_SCL_OFF);
	iowrite32(PINMUX_VAL_I2C, d->pinmux_base + PINMUX_PMBUS_SDA_OFF);
}

static void pinmux_to_avs(struct cv84x2_dvfs_pmbus *d)
{
	/* PMBUS0 dedicated pins, no AVS sharing, nothing to restore */
}

/* ========== PMBus read/write helpers ========== */

static int pmbus_read_word(struct cv84x2_dvfs_pmbus *d, u8 cmd, u16 *val)
{
	u8 buf[2] = {0, 0};
	struct dvfs_i2c_msg msgs[2] = {
		{ .addr = d->pmic_addr, .flags = 0,      .len = 1, .buf = &cmd },
		{ .addr = d->pmic_addr, .flags = I2C_M_RD, .len = 2, .buf = buf },
	};
	int ret;

	mutex_lock(&d->lock);
	pinmux_to_i2c(d);
	dw_i2c_init(d);
	ret = dw_i2c_xfer(d, msgs, 2);
	pinmux_to_avs(d);
	mutex_unlock(&d->lock);

	if (ret)
		return ret;
	*val = buf[0] | ((u16)buf[1] << 8);
	return 0;
}

static int pmbus_read_byte(struct cv84x2_dvfs_pmbus *d, u8 cmd, u8 *val)
{
	u8 buf = 0;
	struct dvfs_i2c_msg msgs[2] = {
		{ .addr = d->pmic_addr, .flags = 0,        .len = 1, .buf = &cmd },
		{ .addr = d->pmic_addr, .flags = I2C_M_RD, .len = 1, .buf = &buf },
	};
	int ret;

	mutex_lock(&d->lock);
	pinmux_to_i2c(d);
	dw_i2c_init(d);
	ret = dw_i2c_xfer(d, msgs, 2);
	pinmux_to_avs(d);
	mutex_unlock(&d->lock);

	if (ret)
		return ret;
	*val = buf;
	return 0;
}

static int pmbus_write_page(struct cv84x2_dvfs_pmbus *d, u8 page)
{
	u8 buf[2] = { PMBUS_PAGE, page };
	struct dvfs_i2c_msg msg = {
		.addr = d->pmic_addr, .flags = 0, .len = 2, .buf = buf,
	};
	int ret;

	mutex_lock(&d->lock);
	pinmux_to_i2c(d);
	dw_i2c_init(d);
	ret = dw_i2c_xfer(d, &msg, 1);
	pinmux_to_avs(d);
	mutex_unlock(&d->lock);

	return ret;
}

static int pmbus_write_word(struct cv84x2_dvfs_pmbus *d, u8 cmd, u16 val)
{
	u8 buf[3] = { cmd, (u8)(val & 0xff), (u8)(val >> 8) };
	struct dvfs_i2c_msg msg = {
		.addr = d->pmic_addr, .flags = 0, .len = 3, .buf = buf,
	};
	int ret;

	mutex_lock(&d->lock);
	pinmux_to_i2c(d);
	dw_i2c_init(d);
	ret = dw_i2c_xfer(d, &msg, 1);
	pinmux_to_avs(d);
	mutex_unlock(&d->lock);

	return ret;
}

static int pmbus_write_byte(struct cv84x2_dvfs_pmbus *d, u8 cmd, u8 val)
{
	u8 buf[2] = { cmd, val };
	struct dvfs_i2c_msg msg = {
		.addr = d->pmic_addr, .flags = 0, .len = 2, .buf = buf,
	};
	int ret;

	mutex_lock(&d->lock);
	pinmux_to_i2c(d);
	dw_i2c_init(d);
	ret = dw_i2c_xfer(d, &msg, 1);
	pinmux_to_avs(d);
	mutex_unlock(&d->lock);

	return ret;
}

/* ========== VID <-> mV formulas (per-chip) ========== */

/* JWH: V = (VID+49) * step * GRS, GRS gain_x10 = {10, 6, 4} */
static u16 jwh_mv_to_vid(u32 mv, u8 step_mv, u8 grs)
{
	static const u32 gain_x10[3] = {10, 6, 4};
	u32 g = gain_x10[grs < 3 ? grs : 0];
	u32 denom = step_mv * g;
	int vid;

	if (denom == 0)
		return 0;
	vid = (int)div_u64((u64)mv * 10 + denom / 2, denom) - 49;
	if (vid < 0)
		vid = 0;
	if (vid > 511)
		vid = 511;
	return (u16)vid;
}

static u32 jwh_vid_to_mv(u16 vid, u8 step_mv, u8 grs)
{
	static const u32 gain_x10[3] = {10, 6, 4};
	u32 g = gain_x10[grs < 3 ? grs : 0];

	return (u32)div_u64((u64)(vid + 49) * step_mv * g, 10);
}

/* MPS: V = (VID+49) * step * GRS, GRS = half_gain ? 0.5 : 1.0 */
static u16 mps_mv_to_vid(u32 mv, u8 step_mv, u8 half_gain)
{
	int grs2 = half_gain ? 1 : 2;
	int denom = step_mv * grs2;
	int vid;

	if (denom == 0)
		return 0;
	vid = (int)div_u64((u64)mv * 2 + denom / 2, denom) - 49;
	if (vid < 0)
		vid = 0;
	if (vid > 511)
		vid = 511;
	return (u16)vid;
}

/* ========== MPS mode switch helpers ========== */

/* Switch OPERATION to PMBus mode (0x82) and VID DAC mux to PMBus path.
 * Safe sequence: read current V -> stage VOUT_COMMAND -> switch OPERATION -> set DAC mux bit.
 */
static int mps_enable_pmbus(struct cv84x2_dvfs_pmbus *d)
{
	u8 op;
	u16 vid_mode, raw;
	u32 mv_now;
	u16 vid_now;
	int ret;

	if (d->mps_pmbus_enabled)
		return 0;

	ret = pmbus_write_page(d, d->cpu_rail_page);
	if (ret)
		return ret;

	/* read current voltage (Direct or Linear16, per VOUT_MODE) */
	ret = pmbus_read_word(d, PMBUS_READ_VOUT, &raw);
	if (ret)
		return ret;
	if (d->mps_vout_mode == 1) {
		mv_now = raw;	/* Direct: 1mV/LSB */
	} else {
		u64 tmp;

		if (d->mps_vout_exp >= 0)
			tmp = (u64)raw * (1ULL << d->mps_vout_exp) * 1000ULL;
		else
			tmp = div_u64((u64)raw * 1000ULL, 1ULL << (-d->mps_vout_exp));
		mv_now = (u32)tmp;
		if (mv_now > 20000 && raw <= 2000)
			mv_now = raw;
	}
	vid_now = mps_mv_to_vid(mv_now, d->mps_vid_step_mv, d->mps_half_gain);

	/* set AVS_PMBUS_CTRL bit, keep mode */
	ret = pmbus_read_byte(d, PMBUS_OPERATION, &op);
	if (ret)
		return ret;
	ret = pmbus_write_byte(d, PMBUS_OPERATION, op | 0x02);
	if (ret)
		return ret;

	/* stage current voltage */
	ret = pmbus_write_word(d, PMBUS_VOUT_COMMAND, vid_now);
	if (ret)
		return ret;

	/* switch to Normal on (PMBus override) */
	ret = pmbus_write_byte(d, PMBUS_OPERATION, 0x82);
	if (ret)
		return ret;

	/* set VID_MODE_DC_LL bit[13] PMBUS_BOOT_EN_R1 = 1 */
	ret = pmbus_write_page(d, 2);
	if (ret)
		return ret;
	ret = pmbus_read_word(d, PMBUS_VID_MODE_DC_LL, &vid_mode);
	if (ret) {
		pmbus_write_page(d, d->cpu_rail_page);
		return ret;
	}
	vid_mode |= BIT(VID_MODE_PMBUS_EN_BIT);
	ret = pmbus_write_word(d, PMBUS_VID_MODE_DC_LL, vid_mode);
	if (ret) {
		pmbus_write_page(d, d->cpu_rail_page);
		return ret;
	}
	pmbus_write_page(d, d->cpu_rail_page);

	d->mps_pmbus_enabled = 1;
	dev_info(d->dev, "MPS PMBus mode enabled: cur=%umV vid=0x%04x OPERATION=0x82\n",
		 mv_now, vid_now);
	return 0;
}

/* ========== chip detection ========== */

static int detect_pmic(struct cv84x2_dvfs_pmbus *d)
{
	u16 mfr_id;
	int ret;

	ret = pmbus_read_word(d, PMBUS_MFR_ID, &mfr_id);
	if (ret) {
		dev_err(d->dev, "MFR_ID(0x99) read failed: %d, cannot identify PMIC\n", ret);
		return -ENODEV;
	}

	if (mfr_id == 0x5702) {		/* JWH block-read format: len=2 + 'W' (from "WJ"=reversed "JW") */
		d->pmic = PMIC_JWH63782;
		dev_info(d->dev, "detected JWH63782 (MFR_ID=0x%04x)\n", mfr_id);
	} else if (mfr_id == 0x5303) {		/* MPS block-read format: len=3 + 'S' (from "SPM"=reversed "MPS") */
		d->pmic = PMIC_MP2985B;
		dev_info(d->dev, "detected MP2985B (MFR_ID=0x%04x)\n", mfr_id);
	} else {
		/* TODO: Renesas PMIC not adapted yet, log only, no volt/freq control */
		d->pmic = PMIC_RENESAS;
		dev_info(d->dev, "detected Renesas/unknown PMIC (MFR_ID=0x%04x), voltage/freq not implemented yet\n",
			 mfr_id);
	}
	return 0;
}

/* Read JWH voltage params from page5. */
static void jwh_probe_params(struct cv84x2_dvfs_pmbus *d)
{
	u16 v;
	int ret;

	d->jwh_grs = 0;
	d->jwh_vid_step_mv = 5;

	ret = pmbus_write_page(d, 5);
	if (ret)
		return;

	/* C2h: hi byte 0=5mV, 1=10mV */
	ret = pmbus_read_word(d, JWH_SVID_PROTOCOL_ID_VIDOMAX, &v);
	if (!ret) {
		d->jwh_vid_step_mv = (v >> 8) ? 10 : 5;
		dev_info(d->dev, "JWH vid_step=%umV\n", d->jwh_vid_step_mv);
	}

	/* 22h: bit[7:6]=GRS */
	ret = pmbus_read_word(d, JWH_MFR_VR_CONFIG, &v);
	if (!ret) {
		d->jwh_grs = (v >> 6) & 0x3;
		dev_info(d->dev, "JWH GRS=%s\n",
			 d->jwh_grs == 0 ? "1.0" : d->jwh_grs == 1 ? "0.6" : "0.4");
	}

	pmbus_write_page(d, d->cpu_rail_page);
}

/* Read MPS voltage params from page2. */
static void mps_probe_params(struct cv84x2_dvfs_pmbus *d)
{
	u16 v;
	int ret;

	d->mps_vid_step_mv = 10;
	d->mps_half_gain = 0;
	d->mps_vout_mode = 1;	/* default Direct 1mV/LSB, NVM may override */
	d->mps_vout_exp = -9;	/* Linear16 n=-9, only used if vout_mode=0 */

	ret = pmbus_write_page(d, 2);
	if (ret)
		return;

	/* 0Dh page2 bit[4]: 0=10mV, 1=5mV */
	ret = pmbus_read_word(d, PMBUS_MFR_VR_MULTI_CONFIG, &v);
	if (!ret) {
		if (v & BIT(4))
			d->mps_vid_step_mv = 5;
		dev_info(d->dev, "MPS vid_step=%umV\n", d->mps_vid_step_mv);
	}

	/* 0Eh page2 bit[0]: 0=unity, 1=half gain */
	ret = pmbus_read_word(d, PMBUS_MFR_VR_CONFIG_IMON, &v);
	if (!ret) {
		d->mps_half_gain = (v & 0x01) ? 1 : 0;
		dev_info(d->dev, "MPS gain=%s\n", d->mps_half_gain ? "half(0.5)" : "unity(1.0)");
	}

	pmbus_write_page(d, d->cpu_rail_page);

	/* read VOUT_MODE to determine READ_VOUT decode format */
	ret = pmbus_read_word(d, PMBUS_VOUT_MODE, &v);
	if (!ret) {
		switch (v & 0xff) {
		case 0x40:	/* Direct, 1mV/LSB */
			d->mps_vout_mode = 1;
			dev_info(d->dev, "MPS VOUT_MODE=Direct(1mV/LSB)\n");
			break;
		case 0x17:	/* Linear16, n=-9 */
			d->mps_vout_mode = 0;
			d->mps_vout_exp = -9;
			dev_info(d->dev, "MPS VOUT_MODE=Linear16 n=-9\n");
			break;
		default:
			dev_info(d->dev, "MPS VOUT_MODE=0x%02x, keep default Direct\n", v & 0xff);
			break;
		}
	}
}

/* ========== voltage write/read ========== */

static int volt_write_mv(struct cv84x2_dvfs_pmbus *d, u32 mv)
{
	u16 vid;
	int ret;

	ret = pmbus_write_page(d, d->cpu_rail_page);
	if (ret)
		return ret;

	if (d->pmic == PMIC_JWH63782) {
		u8 st;
		u16 rd_vid;

		/* pre-write STATUS_VOUT check */
		ret = pmbus_read_byte(d, PMBUS_STATUS_VOUT, &st);
		if (ret) {
			dev_warn(d->dev, "volt_write: read STATUS_VOUT failed, proceed\n");
		} else if (st & 0xf0) {
			dev_err(d->dev, "volt_write: STATUS_VOUT=0x%02x OV/UV fault, abort\n", st);
			return -EIO;
		}

		vid = jwh_mv_to_vid(mv, d->jwh_vid_step_mv, d->jwh_grs);
		ret = pmbus_write_word(d, PMBUS_VOUT_COMMAND, vid);
		if (ret)
			return ret;

		/* post-write readback verify */
		ret = pmbus_read_word(d, PMBUS_READ_VOUT, &rd_vid);
		if (ret) {
			dev_warn(d->dev, "volt_write: readback READ_VOUT failed, write may still be ok\n");
			return 0;
		}
		{
			u32 rd_mv = jwh_vid_to_mv(rd_vid, d->jwh_vid_step_mv, d->jwh_grs);

			if (abs((int)rd_mv - (int)mv) > (int)d->jwh_vid_step_mv)
				dev_warn(d->dev, "volt_write: target=%u readback=%u mV mismatch\n",
					 mv, rd_mv);
			else
				dev_info(d->dev, "volt_write: %u mV -> vid 0x%04x OK\n", mv, vid);
		}
	} else if (d->pmic == PMIC_MP2985B) {
		/* MPS: first write enables PMBus mode */
		if (!d->mps_pmbus_enabled) {
			ret = mps_enable_pmbus(d);
			if (ret)
				return ret;
		}
		/* refresh OPERATION bit1 (AVS_PMBUS_CTRL) each write */
		{
			u8 op;

			if (!pmbus_read_byte(d, PMBUS_OPERATION, &op))
				pmbus_write_byte(d, PMBUS_OPERATION, op | 0x02);
		}
		vid = mps_mv_to_vid(mv, d->mps_vid_step_mv, d->mps_half_gain);
		ret = pmbus_write_word(d, PMBUS_VOUT_COMMAND, vid);
		if (ret)
			return ret;
		dev_info(d->dev, "volt_write: %u mV -> vid 0x%04x OK\n", mv, vid);
	} else {
		/* Renesas: not implemented yet */
		dev_warn(d->dev, "volt_write: Renesas PMIC not supported yet\n");
		return -ENODEV;
	}
	return 0;
}

static int volt_read_mv(struct cv84x2_dvfs_pmbus *d, u32 *mv)
{
	u16 raw;
	int ret;

	ret = pmbus_write_page(d, d->cpu_rail_page);
	if (ret)
		return ret;
	ret = pmbus_read_word(d, PMBUS_READ_VOUT, &raw);
	if (ret)
		return ret;

	if (d->pmic == PMIC_JWH63782) {
		*mv = jwh_vid_to_mv(raw, d->jwh_vid_step_mv, d->jwh_grs);
	} else if (d->pmic == PMIC_MP2985B) {
		/* MPS READ_VOUT: Direct (1mV/LSB) or Linear16, per VOUT_MODE */
		if (d->mps_vout_mode == 1) {
			/* Direct: raw value = mV */
			*mv = raw;
		} else {
			/* Linear16: V = Y * 2^N, mV = Y * 1000 / 2^(-N) for N<0 */
			u64 tmp;

			if (d->mps_vout_exp >= 0)
				tmp = (u64)raw * (1ULL << d->mps_vout_exp) * 1000ULL;
			else
				tmp = div_u64((u64)raw * 1000ULL, 1ULL << (-d->mps_vout_exp));
			*mv = (u32)tmp;
			/* sanity: if decode gives garbage, fall back to raw */
			if (*mv > 20000 && raw <= 2000)
				*mv = raw;
		}
	} else {
		*mv = 0;	/* Renesas: not implemented */
	}
	return 0;
}

/* ========== frequency set/read (shared, PLL direct write) ========== */

static int freq_set_mhz(struct cv84x2_dvfs_pmbus *d, bool tpu, u32 mhz)
{
	bool cpu = !tpu;
	void __iomem *pll = d->crg_base + (cpu ? CRG_MPLL0_CTRL_OFF : CRG_MPLL1_CTRL_OFF);
	u32 sel_bit = cpu ? BIT(0) : BIT(2);
	u32 lock_bit = cpu ? BIT(10) : BIT(11);
	u32 fbdiv, base, sel;
	int i;

	if (mhz == 0 || mhz % PLL_MHZ_STEP)
		return -EINVAL;
	fbdiv = mhz / PLL_MHZ_STEP;
	if (fbdiv < PLL_FBDIV_MIN || fbdiv > PLL_FBDIV_MAX)
		return -EINVAL;
	base = 0x1181000eU | (fbdiv << 4);

	/* step 1: switch mux to sibling PLL */
	sel = ioread32(d->crg_base + CRG_CLK_SEL_0_OFF);
	iowrite32(sel & ~sel_bit, d->crg_base + CRG_CLK_SEL_0_OFF);

	/* step 2: reset then release MPLL */
	iowrite32(base, pll);
	iowrite32(base | BIT(22), pll);

	/* step 3: poll lock, non-fatal */
	for (i = 0; i < 1000; i++) {
		if (ioread32(d->crg_base + CRG_PLL_STATUS_OFF) & lock_bit)
			break;
		usleep_range(10, 20);
	}
	if (i >= 1000)
		dev_warn(d->dev, "freq_set: MPLL(%s) lock bit not asserted\n",
			 cpu ? "mpll0" : "mpll1");

	/* step 4: switch mux back */
	sel = ioread32(d->crg_base + CRG_CLK_SEL_0_OFF);
	iowrite32(sel | sel_bit, d->crg_base + CRG_CLK_SEL_0_OFF);
	return 0;
}

static u32 mpll_freq_mhz(struct cv84x2_dvfs_pmbus *d, bool tpu)
{
	u32 val = ioread32(d->crg_base + (tpu ? CRG_MPLL1_CTRL_OFF : CRG_MPLL0_CTRL_OFF));
	u32 fbdiv = (val >> 4) & 0xFFF;

	return fbdiv * PLL_MHZ_STEP;
}

/* ========== vote apply (volt-first ordering) ========== */

static int vote_apply(struct cv84x2_dvfs_pmbus *d, u8 id)
{
	struct dvfs_vote_point *p;
	u32 old_mv, new_mv;
	u32 old_cpu_mhz, new_cpu_mhz;
	u32 old_tpu_mhz, new_tpu_mhz;
	int ret;

	if (id >= DVFS_VOTE_MAX || !d->vote_tbl[id].valid)
		return -EINVAL;
	p = &d->vote_tbl[id];

	new_mv = p->cpu_volt_mv;
	new_cpu_mhz = p->cpu_freq_mhz;
	new_tpu_mhz = p->tpu_freq_mhz;

	/* read current state */
	ret = volt_read_mv(d, &old_mv);
	if (ret)
		return ret;
	old_cpu_mhz = mpll_freq_mhz(d, false);
	old_tpu_mhz = mpll_freq_mhz(d, true);

	if (new_mv >= old_mv) {
		/* raise: volt then freq (safe: over-volt at low freq) */
		ret = volt_write_mv(d, new_mv);
		if (ret)
			return ret;
		ret = freq_set_mhz(d, false, new_cpu_mhz);
		if (ret)
			return ret;
		ret = freq_set_mhz(d, true, new_tpu_mhz);
		if (ret)
			return ret;
	} else {
		/* lower: freq then volt (safe: high volt at low freq) */
		ret = freq_set_mhz(d, false, new_cpu_mhz);
		if (ret)
			return ret;
		ret = freq_set_mhz(d, true, new_tpu_mhz);
		if (ret)
			return ret;
		ret = volt_write_mv(d, new_mv);
		if (ret)
			return ret;
	}
	d->active_id = id;
	return 0;
}

/* ========== sysfs ========== */

static const char *pmic_name(enum pmic_type t)
{
	switch (t) {
	case PMIC_JWH63782: return "JWH63782";
	case PMIC_MP2985B:  return "MP2985B";
	case PMIC_RENESAS:  return "Renesas";
	default:            return "unknown";
	}
}

static ssize_t cpu_volt_mv_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct cv84x2_dvfs_pmbus *d = dev_get_drvdata(dev);
	u32 mv;
	int ret;

	ret = volt_read_mv(d, &mv);
	if (ret)
		return scnprintf(buf, PAGE_SIZE, "read cpu_volt: failed: %d\n", ret);
	return scnprintf(buf, PAGE_SIZE, "%u mV (pmic=%s)\n", mv, pmic_name(d->pmic));
}

static ssize_t cpu_volt_mv_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct cv84x2_dvfs_pmbus *d = dev_get_drvdata(dev);
	u32 mv;
	int ret;

	ret = kstrtou32(buf, 10, &mv);
	if (ret)
		return ret;
	if (mv < VOLT_MIN_MV_DEFAULT || mv > VOLT_MAX_MV_DEFAULT) {
		dev_err(dev, "cpu_volt_mv=%u out of range [%u,%u]\n",
			mv, VOLT_MIN_MV_DEFAULT, VOLT_MAX_MV_DEFAULT);
		return -EINVAL;
	}
	ret = volt_write_mv(d, mv);
	if (ret)
		return ret;
	return count;
}
static DEVICE_ATTR_RW(cpu_volt_mv);

static ssize_t cpu_freq_mhz_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct cv84x2_dvfs_pmbus *d = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%u\n", mpll_freq_mhz(d, false));
}

static ssize_t cpu_freq_mhz_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct cv84x2_dvfs_pmbus *d = dev_get_drvdata(dev);
	unsigned long mhz;
	int ret;

	ret = kstrtoul(buf, 10, &mhz);
	if (ret)
		return ret;
	ret = freq_set_mhz(d, false, (u32)mhz);
	if (ret)
		return ret;
	return count;
}
static DEVICE_ATTR_RW(cpu_freq_mhz);

static ssize_t tpu_freq_mhz_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct cv84x2_dvfs_pmbus *d = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%u\n", mpll_freq_mhz(d, true));
}

static ssize_t tpu_freq_mhz_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct cv84x2_dvfs_pmbus *d = dev_get_drvdata(dev);
	unsigned long mhz;
	int ret;

	ret = kstrtoul(buf, 10, &mhz);
	if (ret)
		return ret;
	ret = freq_set_mhz(d, true, (u32)mhz);
	if (ret)
		return ret;
	return count;
}
static DEVICE_ATTR_RW(tpu_freq_mhz);

/* dvfs_table: 16 rows "<id> <cpu_volt_mv> <cpu_freq_mhz> <tpu_freq_mhz> <state>" */
static ssize_t dvfs_table_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct cv84x2_dvfs_pmbus *d = dev_get_drvdata(dev);
	int n = 0;
	int i;

	n += scnprintf(buf + n, PAGE_SIZE - n,
		       "id  cpu_volt(mV)  cpu_freq(MHz)  tpu_freq(MHz)  state\n");
	for (i = 0; i < DVFS_VOTE_MAX; i++) {
		struct dvfs_vote_point *p = &d->vote_tbl[i];
		const char *state = p->valid ? "filled" : "empty";

		if (d->active_id == i)
			state = "active";
		if (p->valid)
			n += scnprintf(buf + n, PAGE_SIZE - n,
				       "%-2d  %-13u  %-13u  %-13u  %s\n",
				       i, p->cpu_volt_mv, p->cpu_freq_mhz,
				       p->tpu_freq_mhz, state);
		else
			n += scnprintf(buf + n, PAGE_SIZE - n,
				       "%-2d  %-13s  %-13s  %-13s  %s\n",
				       i, "-", "-", "-", state);
	}
	return n;
}

static ssize_t dvfs_table_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct cv84x2_dvfs_pmbus *d = dev_get_drvdata(dev);
	unsigned int id, volt, cf, tf;
	char kw[8];

	if (sscanf(buf, "%u %u %u %u", &id, &volt, &cf, &tf) == 4) {
		if (id >= DVFS_VOTE_MAX)
			return -EINVAL;
		d->vote_tbl[id].cpu_volt_mv = volt;
		d->vote_tbl[id].cpu_freq_mhz = cf;
		d->vote_tbl[id].tpu_freq_mhz = tf;
		d->vote_tbl[id].valid = 1;
		return count;
	}
	if (sscanf(buf, "%u %7s", &id, kw) == 2 && !strncmp(kw, "clear", 5)) {
		if (id >= DVFS_VOTE_MAX)
			return -EINVAL;
		memset(&d->vote_tbl[id], 0, sizeof(d->vote_tbl[id]));
		if (d->active_id == id)
			d->active_id = DVFS_VOTE_NONE;
		return count;
	}
	return -EINVAL;
}
static DEVICE_ATTR_RW(dvfs_table);

static ssize_t vote_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	struct cv84x2_dvfs_pmbus *d = dev_get_drvdata(dev);

	if (d->active_id == DVFS_VOTE_NONE)
		return scnprintf(buf, PAGE_SIZE, "none(boot default)\n");
	return scnprintf(buf, PAGE_SIZE, "%u\n", d->active_id);
}

static ssize_t vote_store(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct cv84x2_dvfs_pmbus *d = dev_get_drvdata(dev);
	long val;
	int ret;

	ret = kstrtol(buf, 10, &val);
	if (ret)
		return ret;
	if (val < 0) {
		d->active_id = DVFS_VOTE_NONE;
		return count;
	}
	if (val >= DVFS_VOTE_MAX)
		return -EINVAL;
	ret = vote_apply(d, (u8)val);
	if (ret)
		return ret;
	return count;
}
static DEVICE_ATTR_RW(vote);

static ssize_t info_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	struct cv84x2_dvfs_pmbus *d = dev_get_drvdata(dev);
	u32 cv = 0;
	char volt_str[16];
	u32 comb = 0;
	int n = 0;

	if (!volt_read_mv(d, &cv))
		scnprintf(volt_str, sizeof(volt_str), "%u mV", cv);
	else
		scnprintf(volt_str, sizeof(volt_str), "N/A");
	if (d->arb_base)
		comb = ioread32(d->arb_base + VOLT_ARB_POWER_COMB_OFF);

	n += scnprintf(buf + n, PAGE_SIZE - n,
		       "cv84x2 DVFS PMBus unified driver\n");
	n += scnprintf(buf + n, PAGE_SIZE - n,
		       "  pmic=%s addr=0x%02x cpu_page=%u tpu_page=%u\n",
		       pmic_name(d->pmic), d->pmic_addr,
		       d->cpu_rail_page, d->tpu_rail_page);
	n += scnprintf(buf + n, PAGE_SIZE - n,
		       "  cpu: freq=%u MHz volt=%s\n",
		       mpll_freq_mhz(d, false), volt_str);
	n += scnprintf(buf + n, PAGE_SIZE - n,
		       "  tpu: freq=%u MHz volt=%s (single rail, shared)\n",
		       mpll_freq_mhz(d, true), volt_str);
	n += scnprintf(buf + n, PAGE_SIZE - n,
		       "  reg_power_comb_mode=0x%x (00=indep 01=cpu+gpu 10=all-merged)\n",
		       comb);
	n += scnprintf(buf + n, PAGE_SIZE - n,
		       "  volt range=[%u,%u] mV, freq step=%u MHz\n",
		       VOLT_MIN_MV_DEFAULT, VOLT_MAX_MV_DEFAULT, PLL_MHZ_STEP);
	if (d->active_id == DVFS_VOTE_NONE)
		n += scnprintf(buf + n, PAGE_SIZE - n,
			       "  active_id=none(boot default)\n");
	else
		n += scnprintf(buf + n, PAGE_SIZE - n,
			       "  active_id=%u\n", d->active_id);
	return n;
}
static DEVICE_ATTR_RO(info);

static struct attribute *cv84x2_dvfs_pmbus_attrs[] = {
	&dev_attr_cpu_volt_mv.attr,
	&dev_attr_cpu_freq_mhz.attr,
	&dev_attr_tpu_freq_mhz.attr,
	&dev_attr_dvfs_table.attr,
	&dev_attr_vote.attr,
	&dev_attr_info.attr,
	NULL,
};
ATTRIBUTE_GROUPS(cv84x2_dvfs_pmbus);

/* ========== probe / remove ========== */

static int cv84x2_dvfs_pmbus_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct cv84x2_dvfs_pmbus *d;
	u32 val;
	int ret;

	d = devm_kzalloc(dev, sizeof(*d), GFP_KERNEL);
	if (!d)
		return -ENOMEM;
	d->dev = dev;
	mutex_init(&d->lock);
	d->active_id = DVFS_VOTE_NONE;

	d->clk_cpu = devm_clk_get(dev, "cpu");
	if (IS_ERR(d->clk_cpu)) {
		dev_err(dev, "failed to get cpu clk\n");
		return PTR_ERR(d->clk_cpu);
	}
	d->clk_tpu = devm_clk_get(dev, "tpu");
	if (IS_ERR(d->clk_tpu)) {
		dev_err(dev, "failed to get tpu clk\n");
		return PTR_ERR(d->clk_tpu);
	}

	d->i2c_base = devm_ioremap(dev, CV84X2_PMBUS_I2C_BASE, 0x1000);
	d->pinmux_base = devm_ioremap(dev, CV84X2_PINMUX_BASE, 0x1000);
	d->arb_base = devm_ioremap(dev, CV84X2_VOLT_ARB_BASE, 0x1000);
	d->crg_base = devm_ioremap(dev, CV84X2_CRG_BASE, 0x30000);
	if (!d->i2c_base || !d->pinmux_base || !d->arb_base || !d->crg_base) {
		dev_err(dev, "ioremap failed\n");
		return -ENOMEM;
	}

	d->pmic_addr = PMIC_ADDR_DEFAULT;
	d->cpu_rail_page = 0;
	d->tpu_rail_page = 1;

	if (!of_property_read_u32(np, "pmic-addr", &val))
		d->pmic_addr = (u8)val;
	if (!of_property_read_u32(np, "cpu-rail-page", &val))
		d->cpu_rail_page = (u8)val;
	if (!of_property_read_u32(np, "tpu-rail-page", &val))
		d->tpu_rail_page = (u8)val;

	/* detect PMIC type via MFR_ID */
	ret = detect_pmic(d);
	if (ret) {
		dev_err(dev, "PMIC detection failed, probe aborted\n");
		return ret;
	}

	/* read chip-specific params */
	if (d->pmic == PMIC_JWH63782)
		jwh_probe_params(d);
	else if (d->pmic == PMIC_MP2985B)
		mps_probe_params(d);
	/* Renesas: no params to probe yet */

	ret = sysfs_create_groups(&dev->kobj, cv84x2_dvfs_pmbus_groups);
	if (ret) {
		dev_err(dev, "failed to create sysfs groups\n");
		return ret;
	}

	platform_set_drvdata(pdev, d);

	/* apply default rail voltage at probe: best-effort, never abort probe.
	 * JWH: STATUS_VOUT pre-check may fail before the rail is enabled;
	 * MPS: this switches the VID DAC mux to PMBus (see mps_enable_pmbus).
	 */
	ret = volt_write_mv(d, DEFAULT_VOLT_MV);
	if (ret)
		dev_warn(dev, "failed to set default %umV: %d (continue)\n",
			 DEFAULT_VOLT_MV, ret);
	else
		dev_info(dev, "default rail voltage set to %umV\n", DEFAULT_VOLT_MV);

	dev_info(dev, "cv84x2-dvfs-pmbus ready: pmic=%s addr=0x%02x\n",
		 pmic_name(d->pmic), d->pmic_addr);
	dev_info(dev, "  /sys/.../cv84x2-dvfs/  cpu_volt_mv|cpu_freq_mhz|tpu_freq_mhz RW, dvfs_table|vote RW, info RO\n");
	return 0;
}

static int cv84x2_dvfs_pmbus_remove(struct platform_device *pdev)
{
	struct cv84x2_dvfs_pmbus *d = platform_get_drvdata(pdev);

	sysfs_remove_groups(&pdev->dev.kobj, cv84x2_dvfs_pmbus_groups);
	if (d) {
		/* MPS: leave PMIC in PMBus mode so next insmod can read MFR_ID.
		 * Restoring AVSBus (0xB0) breaks MFR_ID detection on re-insmod.
		 */
		pinmux_to_avs(d);
	}
	return 0;
}

static const struct of_device_id cv84x2_dvfs_pmbus_match[] = {
	{ .compatible = "cvitek,cv84x2-dvfs" },
	{ },
};
MODULE_DEVICE_TABLE(of, cv84x2_dvfs_pmbus_match);

static struct platform_driver cv84x2_dvfs_pmbus_driver = {
	.driver = {
		.name = "cv84x2-dvfs-pmbus",
		.of_match_table = of_match_ptr(cv84x2_dvfs_pmbus_match),
	},
	.probe  = cv84x2_dvfs_pmbus_probe,
	.remove = cv84x2_dvfs_pmbus_remove,
};
module_platform_driver(cv84x2_dvfs_pmbus_driver);

MODULE_AUTHOR("CVITEK");
MODULE_DESCRIPTION("cv84x2 DVFS unified PMBus driver (JWH63782 + MP2985B)");
MODULE_LICENSE("GPL");
