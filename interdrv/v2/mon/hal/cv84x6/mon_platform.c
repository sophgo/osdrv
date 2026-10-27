// SPDX-License-Identifier: GPL-2.0-only
/* CV84X6 AXI Monitor Platform Driver
 *
 * This driver provides bandwidth profiling and latency monitoring
 * for CV84X6 SoC AXI bus fabric.
 *
 * Monitor instances:
 *   - Fabric S1: F0S1, F1S1, F2S1, F3S1, F5S1 (TOP_NORTH)
 *   - Video Encoder: VE
 *   - Video Decoder: VD0-VD3, each with MON0 and MON1
 *
 * Each monitor has 2 channels:
 *   - Write channel: base + 0x00
 *   - Read channel:  base + 0x80
 */
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/timex.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <asm/div64.h>
#include "mon_platform.h"

/*
 * DDR subsystem top (ddr_sys) register: CFG_CLK_EN
 * Each DDR subsystem has a ddr_sys block at (monitor_paddr - 0x6000).
 * CFG_CLK_EN[2] = cfg_cgen_axi_mon: clock gate control of axi monitor
 *   0: clock disable, 1: clock enable
 */
#define DDR_SYS_CFG_CLK_EN		0x0000
#define DDR_SYS_CGEN_AXI_MON		BIT(2)
#define DDR_MON_OFFSET_IN_SYS		0x6000

/* All CV84X6 AXI Monitor instances with subsystem descriptions
 *
 * axi_clk: AXI clock frequency in MHz for duration calculation
 * has_his: Support latency histogram (VO monitors only)
 * no_irq:  No IRQ in DTS (VO/TPU monitors)
 */
static struct aximon_instance mon_instances[] = {
	/* Fabric S1 monitors (1200 MHz) */
	{ .name = "F0S1", .id_name = "f0s1", .axi_clk = 1200, .desc = "VD0~3_MON1" },
	{ .name = "F1S1", .id_name = "f1s1", .axi_clk = 1200, .desc = "VD0~3_MON0/VE" },
	{ .name = "F2S1", .id_name = "f2s1", .axi_clk = 1200, .desc = "USB_SYS/VO_RT" },
	{ .name = "F3S1", .id_name = "f3s1", .axi_clk = 1200, .desc = "AP_SYS/VO_OFF/VO_IGA/SE/HESPERI" },
	{ .name = "F5S1", .id_name = "f5s1", .axi_clk = 1200, .desc = "Fabric4/TPU_SYS" },
	/* VE monitor (650 MHz), VD monitors (800 MHz) */
	{ .name = "VE", .id_name = "ve", .axi_clk = 650, .desc = "Video Encoder" },
	{ .name = "VD0_MON0", .id_name = "vd0_m0", .axi_clk = 800, .desc = "VD0 pri/v0_pri/bwb/VPSS_T2" },
	{ .name = "VD0_MON1", .id_name = "vd0_m1", .axi_clk = 800, .desc = "VD0 VPSS_T0/T1" },
	{ .name = "VD1_MON0", .id_name = "vd1_m0", .axi_clk = 800, .desc = "VD1 pri/v0_pri/bwb/VPSS_T2" },
	{ .name = "VD1_MON1", .id_name = "vd1_m1", .axi_clk = 800, .desc = "VD1 VPSS_T0/T1" },
	{ .name = "VD2_MON0", .id_name = "vd2_m0", .axi_clk = 800, .desc = "VD2 pri/v0_pri/bwb/VPSS_T2" },
	{ .name = "VD2_MON1", .id_name = "vd2_m1", .axi_clk = 800, .desc = "VD2 VPSS_T0/T1" },
	{ .name = "VD3_MON0", .id_name = "vd3_m0", .axi_clk = 800, .desc = "VD3 pri/v0_pri/bwb/VPSS_T2" },
	{ .name = "VD3_MON1", .id_name = "vd3_m1", .axi_clk = 800, .desc = "VD3 VPSS_T0/T1" },
	/* VO monitors (no IRQ, has latency histogram) */
	{ .name = "VO_IGA", .id_name = "vo_iga", .axi_clk = 1200, .no_irq = true, .has_his = true, .desc = "IGA0~2" },
	{ .name = "VO_OFF", .id_name = "vo_off", .axi_clk = 650,  .no_irq = true, .has_his = true, .desc = "LDC/2DE0/OENC/VPSS" },
	{ .name = "VO_RT",  .id_name = "vo_rt",  .axi_clk = 650,  .no_irq = true, .has_his = true, .desc = "VPSS/DISP0" },
	/* TPU monitors (no IRQ, 1200 MHz) */
	{ .name = "TPU_MON0", .id_name = "tpu_m0", .axi_clk = 1200, .no_irq = true, .desc = "TXP_SYS0 dat" },
	{ .name = "TPU_MON1", .id_name = "tpu_m1", .axi_clk = 1200, .no_irq = true, .desc = "TXP_SYS1 dat" },
	{ .name = "TPU_MON2", .id_name = "tpu_m2", .axi_clk = 1200, .no_irq = true, .desc = "TXP_SYS2 dat" },
	{ .name = "TPU_MON3", .id_name = "tpu_m3", .axi_clk = 1200, .no_irq = true, .desc = "TXP_SYS3 dat" },
	{ .name = "TPU_MON4", .id_name = "tpu_m4", .axi_clk = 1200, .no_irq = true, .desc = "HAU D0" },
	{ .name = "TPU_MON5", .id_name = "tpu_m5", .axi_clk = 1200, .no_irq = true, .desc = "HAU D1" },
	/* DDR MON0 (no IRQ) - 0x70c06000, 0x74c06000, 0x78c06000, 0x7cc06000 */
	{ .name = "DDR0_MON0", .id_name = "ddr0m0",
	  .axi_clk = 1200, .no_irq = true, .desc = "DDR SYS0 MON0" },
	{ .name = "DDR1_MON0", .id_name = "ddr1m0",
	  .axi_clk = 1200, .no_irq = true, .desc = "DDR SYS1 MON0" },
	{ .name = "DDR2_MON0", .id_name = "ddr2m0",
	  .axi_clk = 1200, .no_irq = true, .desc = "DDR SYS2 MON0" },
	{ .name = "DDR3_MON0", .id_name = "ddr3m0",
	  .axi_clk = 1200, .no_irq = true, .desc = "DDR SYS3 MON0" },
	/* DDR MON1 (no IRQ) - 0x70c06100, 0x74c06100, 0x78c06100, 0x7cc06100 */
	{ .name = "DDR0_MON1", .id_name = "ddr0m1",
	  .axi_clk = 1200, .no_irq = true, .desc = "DDR SYS0 MON1" },
	{ .name = "DDR1_MON1", .id_name = "ddr1m1",
	  .axi_clk = 1200, .no_irq = true, .desc = "DDR SYS1 MON1" },
	{ .name = "DDR2_MON1", .id_name = "ddr2m1",
	  .axi_clk = 1200, .no_irq = true, .desc = "DDR SYS2 MON1" },
	{ .name = "DDR3_MON1", .id_name = "ddr3m1",
	  .axi_clk = 1200, .no_irq = true, .desc = "DDR SYS3 MON1" },
};

#define MON_INST_COUNT	ARRAY_SIZE(mon_instances)

/*
 * Mapping table: mon_instances index -> vaddr field offset in cvi_mon_device
 * This eliminates hardcoded index assignments in axi_mon_init()
 */
static const size_t mon_vaddr_offsets[] = {
	offsetof(struct cvi_mon_device, mon_f0s1_vaddr),
	offsetof(struct cvi_mon_device, mon_f1s1_vaddr),
	offsetof(struct cvi_mon_device, mon_f2s1_vaddr),
	offsetof(struct cvi_mon_device, mon_f3s1_vaddr),
	offsetof(struct cvi_mon_device, mon_f5s1_vaddr),
	offsetof(struct cvi_mon_device, mon_ve_vaddr),
	offsetof(struct cvi_mon_device, mon_vd0_mon0_vaddr),
	offsetof(struct cvi_mon_device, mon_vd0_mon1_vaddr),
	offsetof(struct cvi_mon_device, mon_vd1_mon0_vaddr),
	offsetof(struct cvi_mon_device, mon_vd1_mon1_vaddr),
	offsetof(struct cvi_mon_device, mon_vd2_mon0_vaddr),
	offsetof(struct cvi_mon_device, mon_vd2_mon1_vaddr),
	offsetof(struct cvi_mon_device, mon_vd3_mon0_vaddr),
	offsetof(struct cvi_mon_device, mon_vd3_mon1_vaddr),
	offsetof(struct cvi_mon_device, mon_vo_iga_vaddr),
	offsetof(struct cvi_mon_device, mon_vo_off_vaddr),
	offsetof(struct cvi_mon_device, mon_vo_rt_vaddr),
	offsetof(struct cvi_mon_device, mon_tpu_mon0_vaddr),
	offsetof(struct cvi_mon_device, mon_tpu_mon1_vaddr),
	offsetof(struct cvi_mon_device, mon_tpu_mon2_vaddr),
	offsetof(struct cvi_mon_device, mon_tpu_mon3_vaddr),
	offsetof(struct cvi_mon_device, mon_tpu_mon4_vaddr),
	offsetof(struct cvi_mon_device, mon_tpu_mon5_vaddr),
	/* DDR MON0 */
	offsetof(struct cvi_mon_device, mon_ddr0_mon0_vaddr),
	offsetof(struct cvi_mon_device, mon_ddr1_mon0_vaddr),
	offsetof(struct cvi_mon_device, mon_ddr2_mon0_vaddr),
	offsetof(struct cvi_mon_device, mon_ddr3_mon0_vaddr),
	/* DDR MON1 */
	offsetof(struct cvi_mon_device, mon_ddr0_mon1_vaddr),
	offsetof(struct cvi_mon_device, mon_ddr1_mon1_vaddr),
	offsetof(struct cvi_mon_device, mon_ddr2_mon1_vaddr),
	offsetof(struct cvi_mon_device, mon_ddr3_mon1_vaddr),
};

/* Timing for bandwidth calculation */
static struct timespec64 ts_start, ts_end, ts_delta;

/*
 * Hit filter configuration
 *
 * Global module parameters set default for ALL monitors.
 * Use procfs interface to override specific monitors:
 *   echo "<name> <sel> <id> <mask>" > /proc/mon/hit_filter
 *
 * Match logic: (Ax_id & mask) == (hit_id & mask)
 *
 * ID bit fields vary by Monitor (see hit_ID_values.txt):
 *   - F0S1: bit[7:6] selects VD0~VD3 MON1
 *   - F1S1: bit[13:11] selects VD0~VD3 MON0 / VE MON
 *   - F2S1: bit[8] selects USB_SYS / VO RT MON
 *   - F3S1: bit[14:12] selects AP_SYS / VO OFF / VO IGA / SE_SYS / HESPERI_SYS
 *   - F5S1: bit[12] selects Fabric4 / TPU_SYS
 *
 * Examples:
 *   # Set global default (all monitors use same filter)
 *   insmod soph_mon.ko hit_sel=2 hit_id=0x4000 hit_id_mask=0x7000
 *
 *   # Override specific monitor via procfs
 *   echo "F3S1 2 0x2000 0x7000" > /proc/mon/hit_filter
 *   echo "F1S1 2 0x2000 0x3800" > /proc/mon/hit_filter
 *   echo "F0S1 0 0 0" > /proc/mon/hit_filter  # disable filter on F0S1
 */
static uint hit_sel;
static uint hit_id;
static uint hit_id_mask;
module_param(hit_sel, uint, 0644);
module_param(hit_id, uint, 0644);
module_param(hit_id_mask, uint, 0644);
MODULE_PARM_DESC(hit_sel, "Default hit select: 0=all traffic, 2=ID filter");
MODULE_PARM_DESC(hit_id, "Default AXI ID value for filtering");
MODULE_PARM_DESC(hit_id_mask, "Default AXI ID mask (1=compare, 0=ignore)");

/*
 * Low-level register access functions
 */
static inline void mon_writel(struct aximon_instance *inst, uint32_t ch_offset,
			      uint32_t reg_offset, uint32_t val)
{
	if (inst->vaddr)
		writel(val, inst->vaddr + ch_offset + reg_offset);
}

static inline uint32_t mon_readl(struct aximon_instance *inst, uint32_t ch_offset,
				 uint32_t reg_offset)
{
	if (inst->vaddr)
		return readl(inst->vaddr + ch_offset + reg_offset);
	return 0;
}

/*
 * Snapshot a single channel
 */
static void axi_mon_snapshot_channel(struct aximon_instance *inst, uint32_t ch)
{
	mon_writel(inst, ch, REG_MISC, AXIMON_SNAPSHOT_VALUE_1);
	mon_writel(inst, ch, REG_MISC, AXIMON_SNAPSHOT_VALUE_2);
}

/*
 * Start monitoring on a single channel
 */
static void axi_mon_start_channel(struct aximon_instance *inst, uint32_t ch)
{
	mon_writel(inst, ch, REG_MISC, AXIMON_START_VALUE);
}

/*
 * Stop monitoring on a single channel
 */
static void axi_mon_stop_channel(struct aximon_instance *inst, uint32_t ch)
{
	mon_writel(inst, ch, REG_MISC, AXIMON_STOP_VALUE);
}

/*
 * Clear counters on a single channel
 */
static void axi_mon_clear_channel(struct aximon_instance *inst, uint32_t ch)
{
	mon_writel(inst, ch, REG_MISC, AXIMON_CLEAR_VALUE);
	mon_writel(inst, ch, REG_MISC, AXIMON_CLEAR_DONE);
}

/*
 * Get byte count from a channel
 */
static uint32_t axi_mon_get_byte_cnt(struct aximon_instance *inst, uint32_t ch)
{
	return mon_readl(inst, ch, REG_BYTE_CNT);
}

/*
 * Get hit count from a channel
 */
static uint32_t axi_mon_get_hit_cnt(struct aximon_instance *inst, uint32_t ch)
{
	return mon_readl(inst, ch, REG_HIT_CNT);
}

/*
 * Get latency count from a channel
 */
static uint32_t axi_mon_get_latency_cnt(struct aximon_instance *inst, uint32_t ch)
{
	return mon_readl(inst, ch, REG_LATENCY_CNT);
}

/*
 * Get latency histogram count
 */
static uint32_t axi_mon_get_latency_his_cnt(struct aximon_instance *inst,
					    uint32_t ch, uint32_t bin)
{
	return mon_readl(inst, ch, REG_LATENCY_HIS_BASE + 4 * bin);
}

/*
 * Get cycle count from a channel (for duration calculation)
 *
 * NOTE: cycle_count register is only 28 bits [27:0].
 * Max value = 0x0FFFFFFF = 268,435,455 cycles.
 * Overflow times by axi_clk:
 *   1200 MHz (Fabric) -> ~224ms
 *   1066 MHz (DDR)    -> ~252ms
 *   1000 MHz (TPU)    -> ~268ms
 *    650 MHz (VE/VD)  -> ~413ms
 * Ensure profiling_window_ms stays below these limits.
 */
static uint32_t axi_mon_get_cycle_cnt(struct aximon_instance *inst, uint32_t ch)
{
	return mon_readl(inst, ch, REG_CYCLE_CNT);
}

/*
 * Set latency histogram bin size for instances that support it
 */
static void axi_mon_set_lat_bin_size(uint32_t val)
{
	int i;

	for (i = 0; i < MON_INST_COUNT; i++) {
		if (!mon_instances[i].vaddr || !mon_instances[i].has_his)
			continue;
		mon_writel(&mon_instances[i], AXIMON_CH_WRITE,
			   REG_CYC_LAT_BIN_SIZE_SEL, val & 0xF);
		mon_writel(&mon_instances[i], AXIMON_CH_READ,
			   REG_CYC_LAT_BIN_SIZE_SEL, val & 0xF);
	}
}

/*
 * Configure hit filter settings for a channel (using per-instance config)
 *
 * REG_HIT_MISC layout:
 *   bit[9:0]   = hit_sel (0=all traffic, 2=ID filter)
 *   bit[17:16] = byte_align_sel (default 0)
 *
 * Since byte_align_sel is typically 0, we write hit_sel directly.
 */
static void axi_mon_config_hit_filter(struct aximon_instance *inst, uint32_t ch)
{
	/* Write hit_sel directly to bit[9:0] */
	mon_writel(inst, ch, REG_HIT_MISC, inst->hit_sel);

	/* Set hit_id and hit_id_mask if ID filtering enabled (hit_sel=2) */
	if (inst->hit_sel == 2) {
		mon_writel(inst, ch, REG_HIT_ID, inst->hit_id);
		mon_writel(inst, ch, REG_HIT_ID_MASK, inst->hit_id_mask);
	}
}

/*
 * Configure input/clock source for a channel
 */
static void axi_mon_config_input_sel(struct aximon_instance *inst, uint32_t ch,
				     uint32_t sel_val)
{
	mon_writel(inst, ch, REG_MISC, sel_val);
}

/*
 * Calculate and accumulate bandwidth statistics (write + read separately)
 *
 * avg: only counts non-zero periods (byte_cnt > 0)
 * avg_tot: counts all periods (including zero)
 */
static void axi_mon_count_port_info(uint32_t duration_us,
				    uint32_t byte_cnt_wr, uint32_t byte_cnt_rd,
				    struct aximon_port_info *info)
{
	uint32_t bw_wr, bw_rd, bw_tot;
	uint32_t total_byte = byte_cnt_wr + byte_cnt_rd;

	if (duration_us == 0)
		return;

	/* BW (MB/s) = byte_cnt / duration_us */
	bw_wr = byte_cnt_wr / duration_us;
	bw_rd = byte_cnt_rd / duration_us;
	bw_tot = total_byte / duration_us;

	/* Write bandwidth: avg_tot always accumulates, avg only for non-zero */
	info->bw_wr_avg_tot_sum += bw_wr;
	if (byte_cnt_wr > 0) {
		if (info->bw_wr_count == 0) {
			info->bw_wr_min = bw_wr;
			info->bw_wr_max = bw_wr;
		} else {
			if (bw_wr < info->bw_wr_min)
				info->bw_wr_min = bw_wr;
			if (bw_wr > info->bw_wr_max)
				info->bw_wr_max = bw_wr;
		}
		info->bw_wr_avg_sum += bw_wr;
		info->bw_wr_count++;
	}

	/* Read bandwidth: avg_tot always accumulates, avg only for non-zero */
	info->bw_rd_avg_tot_sum += bw_rd;
	if (byte_cnt_rd > 0) {
		if (info->bw_rd_count == 0) {
			info->bw_rd_min = bw_rd;
			info->bw_rd_max = bw_rd;
		} else {
			if (bw_rd < info->bw_rd_min)
				info->bw_rd_min = bw_rd;
			if (bw_rd > info->bw_rd_max)
				info->bw_rd_max = bw_rd;
		}
		info->bw_rd_avg_sum += bw_rd;
		info->bw_rd_count++;
	}

	/* Total bandwidth (for compatibility) */
	if (total_byte > 0) {
		if (info->bw_count == 0) {
			info->bw_min = bw_tot;
			info->bw_max = bw_tot;
		} else {
			if (bw_tot < info->bw_min)
				info->bw_min = bw_tot;
			if (bw_tot > info->bw_max)
				info->bw_max = bw_tot;
		}
		info->bw_avg_sum += bw_tot;
		info->bw_count++;
	}

	info->count++;

	/* Accumulate duration for arithmetic average (computed at dump time) */
	info->time_avg_sum += duration_us;
}

/*
 * Calculate and accumulate latency statistics (in ns)
 */
static void axi_mon_count_latency_info(uint32_t latency_cnt,
				       uint32_t hit_cnt,
				       uint32_t axi_clk,
				       uint64_t *avg_sum,
				       uint32_t *min,
				       uint32_t *max,
				       uint64_t *lat_count)
{
	uint32_t avg_latency;

	if (hit_cnt == 0)
		return;

	avg_latency = (uint32_t)(1000ULL * latency_cnt / (axi_clk * hit_cnt));

	/* Update min/max/avg only for valid samples (hit_cnt > 0) */
	if (*lat_count == 0) {
		*min = avg_latency;
		*max = avg_latency;
	} else {
		if (avg_latency < *min)
			*min = avg_latency;
		if (avg_latency > *max)
			*max = avg_latency;
	}

	*avg_sum += avg_latency;
	(*lat_count)++;
}

/*
 * Accumulate latency histogram data
 */
static void axi_mon_count_latency_his_info(uint64_t write_his[LATENCY_HIS_COUNT],
					   uint64_t read_his[LATENCY_HIS_COUNT],
					   struct aximon_port_info *info)
{
	int i;

	for (i = 0; i < LATENCY_HIS_COUNT; i++) {
		info->write_latency_his_cnt[i] += write_his[i];
		info->read_latency_his_cnt[i] += read_his[i];
	}
}

/*
 * Apply global default hit filter to all instances (unless already overridden)
 */
static void axi_mon_sync_hit_params(void)
{
	int i;

	for (i = 0; i < MON_INST_COUNT; i++) {
		/* Only apply global default if not explicitly set via procfs */
		if (mon_instances[i].hit_sel == 0 &&
		    mon_instances[i].hit_id == 0 &&
		    mon_instances[i].hit_id_mask == 0) {
			mon_instances[i].hit_sel = hit_sel;
			mon_instances[i].hit_id = hit_id;
			mon_instances[i].hit_id_mask = hit_id_mask;
		}
	}
}

/*
 * Set hit filter for a specific monitor by name (called from procfs)
 * Returns 0 on success, -1 if monitor not found
 */
int axi_mon_set_hit_filter(const char *name, uint32_t sel, uint32_t id,
			   uint32_t mask)
{
	int i;

	for (i = 0; i < MON_INST_COUNT; i++) {
		if (strcasecmp(mon_instances[i].name, name) == 0) {
			mon_instances[i].hit_sel = sel;
			mon_instances[i].hit_id = id;
			mon_instances[i].hit_id_mask = mask;
			pr_info("[CV84X6 MON] %s: hit_sel=%u, hit_id=0x%x, hit_mask=0x%x\n",
				name, sel, id, mask);
			return 0;
		}
	}
	return -1;
}

/*
 * Get current hit filter config for all monitors (for procfs read)
 */
int axi_mon_get_hit_filter_info(char *buf, int size)
{
	int i, len = 0;

	len += scnprintf(buf + len, size - len,
			 "# Format: <name> <hit_sel> <hit_id> <hit_mask>\n");
	len += scnprintf(buf + len, size - len,
			 "# Write: echo \"<name> <sel> <id> <mask>\" > hit_filter\n\n");

	for (i = 0; i < MON_INST_COUNT; i++) {
		len += scnprintf(buf + len, size - len,
				 "%-10s %u 0x%04x 0x%04x\n",
				 mon_instances[i].name,
				 mon_instances[i].hit_sel,
				 mon_instances[i].hit_id,
				 mon_instances[i].hit_id_mask);
	}
	return len;
}

void axi_mon_reset_all(void)
{
	int i;

	/* Sync module params to per-instance config */
	axi_mon_sync_hit_params();

	for (i = 0; i < MON_INST_COUNT; i++) {
		memset(&mon_instances[i].info, 0, sizeof(struct aximon_port_info));
		strncpy(mon_instances[i].info.port_name, mon_instances[i].name,
			MON_NAME_LEN - 1);

		/* Skip monitors without valid vaddr */
		if (!mon_instances[i].vaddr)
			continue;

		/* Reset sequence: Stop -> Clear -> Enable (Input Sel + Hit Sel 0) */
		axi_mon_stop_channel(&mon_instances[i], AXIMON_CH_WRITE);
		axi_mon_stop_channel(&mon_instances[i], AXIMON_CH_READ);

		axi_mon_clear_channel(&mon_instances[i], AXIMON_CH_WRITE);
		axi_mon_clear_channel(&mon_instances[i], AXIMON_CH_READ);

		/* hit_sel = 0 */
		mon_writel(&mon_instances[i], AXIMON_CH_WRITE, REG_HIT_MISC, 0);
		mon_writel(&mon_instances[i], AXIMON_CH_READ, REG_HIT_MISC, 0);

		/* Configure input select and hit filter from per-instance config */
		axi_mon_config_input_sel(&mon_instances[i], AXIMON_CH_WRITE,
					 AXIMON_SELECT_CLK);
		axi_mon_config_input_sel(&mon_instances[i], AXIMON_CH_READ,
					 AXIMON_SELECT_CLK);

		axi_mon_config_hit_filter(&mon_instances[i], AXIMON_CH_WRITE);
		axi_mon_config_hit_filter(&mon_instances[i], AXIMON_CH_READ);
	}
}

/*
 * Clear hardware counters only (preserve software statistics)
 * Used between sampling periods to get per-period measurements
 */
void axi_mon_clear_hw_counters(void)
{
	int i;

	for (i = 0; i < MON_INST_COUNT; i++) {
		if (!mon_instances[i].vaddr)
			continue;

		axi_mon_stop_channel(&mon_instances[i], AXIMON_CH_WRITE);
		axi_mon_stop_channel(&mon_instances[i], AXIMON_CH_READ);

		axi_mon_clear_channel(&mon_instances[i], AXIMON_CH_WRITE);
		axi_mon_clear_channel(&mon_instances[i], AXIMON_CH_READ);

		axi_mon_start_channel(&mon_instances[i], AXIMON_CH_WRITE);
		axi_mon_start_channel(&mon_instances[i], AXIMON_CH_READ);
	}
}

/*
 * Public API: Start all monitors
 */
void axi_mon_start_all(void)
{
	int i;

	/* Reset all counters and filters before starting */
	axi_mon_reset_all();

	for (i = 0; i < MON_INST_COUNT; i++) {
		if (!mon_instances[i].vaddr)
			continue;
		axi_mon_start_channel(&mon_instances[i], AXIMON_CH_WRITE);
		axi_mon_start_channel(&mon_instances[i], AXIMON_CH_READ);
	}

	ktime_get_boottime_ts64(&ts_start);
}

/*
 * Public API: Stop all monitors
 */
void axi_mon_stop_all(void)
{
	int i;

	for (i = 0; i < MON_INST_COUNT; i++) {
		if (!mon_instances[i].vaddr)
			continue;
		axi_mon_stop_channel(&mon_instances[i], AXIMON_CH_WRITE);
		axi_mon_stop_channel(&mon_instances[i], AXIMON_CH_READ);
	}

	ktime_get_boottime_ts64(&ts_end);
	ts_delta = timespec64_sub(ts_end, ts_start);

	pr_info("[CV84X6 MON] Monitor time: %lld ns = %lld us\n",
		timespec64_to_ns(&ts_delta), timespec64_to_ns(&ts_delta) / 1000);
}

/*
 * Public API: Snapshot all monitors
 */
void axi_mon_snapshot_all(void)
{
	int i;

	for (i = 0; i < MON_INST_COUNT; i++) {
		if (!mon_instances[i].vaddr)
			continue;
		axi_mon_snapshot_channel(&mon_instances[i], AXIMON_CH_WRITE);
		axi_mon_snapshot_channel(&mon_instances[i], AXIMON_CH_READ);
	}
}

/*
 * Public API: Get info from all monitors
 * Note: duration_us parameter is ignored, we use hardware CYCLE_CNT instead
 *       (consistent with soph platform)
 */
void axi_mon_get_info_all(uint32_t duration_us)
{
	int i, j;
	struct aximon_instance *inst;
	uint32_t byte_cnt_wr, byte_cnt_rd;
	uint32_t hit_cnt_wr, hit_cnt_rd;
	uint32_t lat_cnt_wr, lat_cnt_rd;
	uint32_t cycle_cnt, hw_duration_us;
	uint64_t write_his[LATENCY_HIS_COUNT];
	uint64_t read_his[LATENCY_HIS_COUNT];

	for (i = 0; i < MON_INST_COUNT; i++) {
		inst = &mon_instances[i];

		/* Skip monitors without valid vaddr */
		if (!inst->vaddr)
			continue;

		/* Get cycle count from hardware and calculate duration */
		cycle_cnt = axi_mon_get_cycle_cnt(inst, AXIMON_CH_WRITE);
		if (inst->axi_clk > 0)
			hw_duration_us = cycle_cnt / inst->axi_clk;
		else
			hw_duration_us = duration_us; /* fallback */

		/* Get byte counts */
		byte_cnt_wr = axi_mon_get_byte_cnt(inst, AXIMON_CH_WRITE);
		byte_cnt_rd = axi_mon_get_byte_cnt(inst, AXIMON_CH_READ);

		/* Save byte counts for later filtering in dump */
		inst->info.byte_cnt_wr += byte_cnt_wr;
		inst->info.byte_cnt_rd += byte_cnt_rd;

		/* Calculate bandwidth statistics (write/read separately) */
		axi_mon_count_port_info(hw_duration_us, byte_cnt_wr, byte_cnt_rd,
					&inst->info);

		/* Get hit and latency counts */
		hit_cnt_wr = axi_mon_get_hit_cnt(inst, AXIMON_CH_WRITE);
		hit_cnt_rd = axi_mon_get_hit_cnt(inst, AXIMON_CH_READ);
		lat_cnt_wr = axi_mon_get_latency_cnt(inst, AXIMON_CH_WRITE);
		lat_cnt_rd = axi_mon_get_latency_cnt(inst, AXIMON_CH_READ);

		/* Calculate latency */
		axi_mon_count_latency_info(lat_cnt_wr, hit_cnt_wr, inst->axi_clk,
					   &inst->info.latency_write_avg,
					   &inst->info.latency_write_avg_min,
					   &inst->info.latency_write_avg_max,
					   &inst->info.latency_write_count);
		axi_mon_count_latency_info(lat_cnt_rd, hit_cnt_rd, inst->axi_clk,
					   &inst->info.latency_read_avg,
					   &inst->info.latency_read_avg_min,
					   &inst->info.latency_read_avg_max,
					   &inst->info.latency_read_count);

		/* Get latency histogram if supported */
		if (inst->has_his) {
			for (j = 0; j < LATENCY_HIS_COUNT; j++) {
				write_his[j] = axi_mon_get_latency_his_cnt(inst,
								AXIMON_CH_WRITE, j);
				read_his[j] = axi_mon_get_latency_his_cnt(inst,
								AXIMON_CH_READ, j);
			}
			axi_mon_count_latency_his_info(write_his, read_his, &inst->info);
		}
	}
}

/*
 * Dump statistics for a single monitor instance
 */
static void axi_mon_dump_single(struct aximon_instance *inst)
{
	struct aximon_port_info *info = &inst->info;
	uint64_t bw_wr_avg, bw_wr_avg_tot, bw_rd_avg, bw_rd_avg_tot;
	uint64_t bw_tot_avg, bw_tot_avg_tot;
	uint64_t lat_w_avg, lat_w_avg_tot, lat_r_avg, lat_r_avg_tot;

	if (info->count == 0)
		return;

	/* Skip monitor if both channels have no data */
	if (info->byte_cnt_wr == 0 && info->byte_cnt_rd == 0) {
		pr_err("%-10s (%s) byte_cnt=0, skipped\n",
		       info->port_name, inst->desc ? inst->desc : "");
		return;
	}

	pr_err("\n");
	pr_err("%-10s (%s)\n", info->port_name, inst->desc ? inst->desc : "");

	/* Print write bandwidth if write channel has data */
	if (info->byte_cnt_wr > 0) {
		/* avg: average of non-zero samples */
		bw_wr_avg = info->bw_wr_avg_sum;
		if (info->bw_wr_count > 0)
			do_div(bw_wr_avg, info->bw_wr_count);

		/* avg_tot: average of all samples */
		bw_wr_avg_tot = info->bw_wr_avg_tot_sum;
		do_div(bw_wr_avg_tot, info->count);

		pr_err(" W_BW: avg=%5uMB/s, avg_tot=%5uMB/s, min=%5uMB/s, max=%5uMB/s\n",
		       (uint32_t)bw_wr_avg, (uint32_t)bw_wr_avg_tot,
		       info->bw_wr_min, info->bw_wr_max);
	}

	/* Print read bandwidth if read channel has data */
	if (info->byte_cnt_rd > 0) {
		/* avg: average of non-zero samples */
		bw_rd_avg = info->bw_rd_avg_sum;
		if (info->bw_rd_count > 0)
			do_div(bw_rd_avg, info->bw_rd_count);

		/* avg_tot: average of all samples */
		bw_rd_avg_tot = info->bw_rd_avg_tot_sum;
		do_div(bw_rd_avg_tot, info->count);

		pr_err(" R_BW: avg=%5uMB/s, avg_tot=%5uMB/s, min=%5uMB/s, max=%5uMB/s\n",
		       (uint32_t)bw_rd_avg, (uint32_t)bw_rd_avg_tot,
		       info->bw_rd_min, info->bw_rd_max);
	}

	/* Print total bandwidth */
	if (info->bw_count > 0) {
		/* avg: average of non-zero samples */
		bw_tot_avg = info->bw_avg_sum;
		do_div(bw_tot_avg, info->bw_count);

		/* avg_tot: average of all samples */
		bw_tot_avg_tot = info->bw_avg_sum;
		do_div(bw_tot_avg_tot, info->count);

		pr_err(" Total: avg=%5uMB/s, avg_tot=%5uMB/s, min=%5uMB/s, max=%5uMB/s\n",
		       (uint32_t)bw_tot_avg, (uint32_t)bw_tot_avg_tot,
		       info->bw_min, info->bw_max);
	}

	/* Print write latency only if write channel has data */
	if (info->byte_cnt_wr > 0) {
		/* avg: average of non-zero samples */
		lat_w_avg = info->latency_write_avg;
		if (info->latency_write_count > 0)
			do_div(lat_w_avg, info->latency_write_count);

		/* avg_tot: average of all samples */
		lat_w_avg_tot = info->latency_write_avg;
		do_div(lat_w_avg_tot, info->count);

		pr_err(" W_Lat: avg=%5lluns, avg_tot=%5lluns, min=%5uns, max=%5uns\n",
		       lat_w_avg, lat_w_avg_tot,
		       info->latency_write_avg_min, info->latency_write_avg_max);
	}

	/* Print read latency only if read channel has data */
	if (info->byte_cnt_rd > 0) {
		/* avg: average of non-zero samples */
		lat_r_avg = info->latency_read_avg;
		if (info->latency_read_count > 0)
			do_div(lat_r_avg, info->latency_read_count);

		/* avg_tot: average of all samples */
		lat_r_avg_tot = info->latency_read_avg;
		do_div(lat_r_avg_tot, info->count);

		pr_err(" R_Lat: avg=%5lluns, avg_tot=%5lluns, min=%5uns, max=%5uns\n",
		       lat_r_avg, lat_r_avg_tot,
		       info->latency_read_avg_min, info->latency_read_avg_max);
	}

	/* Dump histogram for VO modules (if data exists) */
	if (info->byte_cnt_wr > 0 && info->write_latency_his_cnt[0]) {
		pr_err("W_Lat_his 0~10: %llu, %llu, %llu, %llu, %llu, %llu, %llu, %llu, %llu, %llu, %llu\n",
			info->write_latency_his_cnt[0], info->write_latency_his_cnt[1],
			info->write_latency_his_cnt[2], info->write_latency_his_cnt[3],
			info->write_latency_his_cnt[4], info->write_latency_his_cnt[5],
			info->write_latency_his_cnt[6], info->write_latency_his_cnt[7],
			info->write_latency_his_cnt[8], info->write_latency_his_cnt[9],
			info->write_latency_his_cnt[10]);
	}
	if (info->byte_cnt_rd > 0 && info->read_latency_his_cnt[0]) {
		pr_err("R_Lat_his 0~10: %llu, %llu, %llu, %llu, %llu, %llu, %llu, %llu, %llu, %llu, %llu\n",
			info->read_latency_his_cnt[0], info->read_latency_his_cnt[1],
			info->read_latency_his_cnt[2], info->read_latency_his_cnt[3],
			info->read_latency_his_cnt[4], info->read_latency_his_cnt[5],
			info->read_latency_his_cnt[6], info->read_latency_his_cnt[7],
			info->read_latency_his_cnt[8], info->read_latency_his_cnt[9],
			info->read_latency_his_cnt[10]);
	}
}

/*
 * Public API: Dump all monitor statistics
 */
void axi_mon_dump(void)
{
	int i;
	uint64_t time_avg = 0;
	uint64_t count = 0;

	/* Find first instance with valid data to compute avg hw period */
	for (i = 0; i < MON_INST_COUNT; i++) {
		if (mon_instances[i].info.count > 0) {
			time_avg = mon_instances[i].info.time_avg_sum;
			count = mon_instances[i].info.count;
			do_div(time_avg, count);
			break;
		}
	}

	if (count == 0) {
		pr_err("==============================\n");
		pr_err("CV84X6: No complete sampling period collected.\n");
		pr_err("  Profiling duration too short or period too long.\n");
		pr_err("  Try increasing profiling duration or decreasing profiling_window_ms\n");
		return;
	}

	pr_err("==============================\n");
	pr_err("CV84X6 profiling window time_avg=%3lluus, %llu counts\n",
	       time_avg, count);

	/* Show hit filter config for Fabric monitors */
	pr_err("Hit filter config:\n");
	for (i = 0; i < MON_INST_COUNT; i++) {
		if (mon_instances[i].hit_sel != 0) {
			pr_err("  %s: hit_sel=%d, hit_id=0x%x, hit_mask=0x%x\n",
			       mon_instances[i].name,
			       mon_instances[i].hit_sel,
			       mon_instances[i].hit_id,
			       mon_instances[i].hit_id_mask);
		}
	}

	for (i = 0; i < MON_INST_COUNT; i++)
		axi_mon_dump_single(&mon_instances[i]);

	pr_err("==============================\n");
}

/*
 * Interrupt handler for AXI Monitor
 */
static irqreturn_t axi_mon_irq_handler(int irq, void *dev_id)
{
	struct aximon_instance *inst = dev_id;
	uint32_t status;

	/* Check and clear Write channel interrupt */
	status = mon_readl(inst, AXIMON_CH_WRITE, REG_MISC);
	if (status & MISC_IRQ_STATUS) {
		mon_writel(inst, AXIMON_CH_WRITE, REG_MISC,
			   AXIMON_IRQ_CLR_VALUE);
		mon_writel(inst, AXIMON_CH_WRITE, REG_MISC,
			   AXIMON_IRQ_CLR_DONE);
	}

	/* Check and clear Read channel interrupt */
	status = mon_readl(inst, AXIMON_CH_READ, REG_MISC);
	if (status & MISC_IRQ_STATUS) {
		mon_writel(inst, AXIMON_CH_READ, REG_MISC,
			   AXIMON_IRQ_CLR_VALUE);
		mon_writel(inst, AXIMON_CH_READ, REG_MISC,
			   AXIMON_IRQ_CLR_DONE);
	}

	return IRQ_HANDLED;
}

/*
 * Enable DDR monitor clock gate in ddr_sys top register.
 * Without this, the monitor counter clock runs but AXI bus
 * traffic is not captured (byte_cnt stays 0).
 */
static void ddr_mon_cg_enable(void)
{
	int i;

	for (i = 0; i < MON_INST_COUNT; i++) {
		phys_addr_t ddr_sys_base;
		void __iomem *reg;
		uint32_t val;
		size_t len;

		if (strncmp(mon_instances[i].id_name, "ddr", 3) != 0)
			continue;
		/* Only process MON0 (id_name ends with "m0"); MON1 shares the same
		 * ddr_sys block and would compute a wrong ddr_sys_base address
		 */
		len = strlen(mon_instances[i].id_name);
		if (len < 2 || strncmp(mon_instances[i].id_name + len - 2, "m0", 2) != 0)
			continue;
		if (!mon_instances[i].vaddr || !mon_instances[i].paddr)
			continue;

		ddr_sys_base = mon_instances[i].paddr - DDR_MON_OFFSET_IN_SYS;
		reg = ioremap(ddr_sys_base, PAGE_SIZE);
		if (IS_ERR_OR_NULL(reg)) {
			pr_err("%s: ioremap ddr_sys 0x%llx failed\n",
			       mon_instances[i].name, (u64)ddr_sys_base);
			continue;
		}

		val = readl(reg + DDR_SYS_CFG_CLK_EN);
		if (!(val & DDR_SYS_CGEN_AXI_MON)) {
			writel(val | DDR_SYS_CGEN_AXI_MON,
			       reg + DDR_SYS_CFG_CLK_EN);
			pr_info("%s: enabled axi_mon CG at ddr_sys 0x%llx\n",
				mon_instances[i].name, (u64)ddr_sys_base);
		}

		iounmap(reg);
	}
}

/*
 * Public API: Initialize monitor driver
 */
void axi_mon_init(struct cvi_mon_device *ndev)
{
	int i;
	struct platform_device *pdev = to_platform_device(ndev->dev);
	char irq_name[MON_NAME_LEN + 4];

	pr_info("[CV84X6 MON] Initializing %d AXI Monitor instances\n",
		(int)MON_INST_COUNT);

	/* Assign vaddr/paddr using offset mapping table (no hardcoded indices) */
	for (i = 0; i < MON_INST_COUNT; i++) {
		void __iomem **vaddr_ptr;

		vaddr_ptr = (void __iomem **)((char *)ndev + mon_vaddr_offsets[i]);
		mon_instances[i].vaddr = *vaddr_ptr;
		mon_instances[i].paddr = ndev->mon_paddr[i];
		mon_instances[i].irq_wr = -1;
		mon_instances[i].irq_rd = -1;
	}

	/* Enable DDR monitor clock gate before any monitor operation */
	ddr_mon_cg_enable();

	/* axi_clk and has_his are pre-configured in mon_instances[] static init */

	for (i = 0; i < MON_INST_COUNT; i++) {
		struct aximon_instance *inst = &mon_instances[i];
		char name_lower[MON_NAME_LEN];
		int j;

		if (!inst->vaddr) {
			dev_info(ndev->dev, "Monitor %s not present (no DTS resource)\n",
				 inst->name);
			continue;
		}

		/* Skip IRQ registration for monitors without IRQ (VO/TPU) */
		if (inst->no_irq) {
			inst->irq_wr = -1;
			inst->irq_rd = -1;
			goto init_port_info;
		}

		/* Get IRQs from DTS by name (DTS uses lowercase) */
		strncpy(name_lower, inst->name, sizeof(name_lower) - 1);
		for (j = 0; name_lower[j]; j++)
			name_lower[j] = tolower(name_lower[j]);

		snprintf(irq_name, sizeof(irq_name), "%s_wr", name_lower);
		inst->irq_wr = platform_get_irq_byname(pdev, irq_name);
		if (inst->irq_wr >= 0) {
			int ret;

			ret = devm_request_irq(ndev->dev, inst->irq_wr,
					       axi_mon_irq_handler, 0,
					       irq_name, inst);
			if (ret) {
				pr_warn("  %s: IRQ %s request failed (%d)\n",
					inst->name, irq_name, ret);
				inst->irq_wr = -1;
			}
		}

		snprintf(irq_name, sizeof(irq_name), "%s_rd", name_lower);
		inst->irq_rd = platform_get_irq_byname(pdev, irq_name);
		if (inst->irq_rd >= 0) {
			int ret;

			ret = devm_request_irq(ndev->dev, inst->irq_rd,
					       axi_mon_irq_handler, 0,
					       irq_name, inst);
			if (ret) {
				pr_warn("  %s: IRQ %s request failed (%d)\n",
					inst->name, irq_name, ret);
				inst->irq_rd = -1;
			}
		}

init_port_info:

		/* Initialize port info */
		memset(&inst->info, 0, sizeof(struct aximon_port_info));
		strncpy(inst->info.port_name, inst->name, MON_NAME_LEN - 1);
		strncpy(inst->info.id_name, inst->id_name, MON_NAME_LEN - 1);

		/* Print monitor info with physical address for debugging */
		if (inst->irq_wr >= 0 && inst->irq_rd >= 0)
			pr_info("  [%2d] %-12s paddr=0x%08llx  irq=[%d, %d]\n",
				i, inst->name, (u64)inst->paddr, inst->irq_wr, inst->irq_rd);
		else if (inst->irq_wr >= 0)
			pr_info("  [%2d] %-12s paddr=0x%08llx  irq=[%d, N/A]\n",
				i, inst->name, (u64)inst->paddr, inst->irq_wr);
		else if (inst->irq_rd >= 0)
			pr_info("  [%2d] %-12s paddr=0x%08llx  irq=[N/A, %d]\n",
				i, inst->name, (u64)inst->paddr, inst->irq_rd);
		else
			pr_info("  [%2d] %-12s paddr=0x%08llx  irq=[N/A, N/A]\n",
				i, inst->name, (u64)inst->paddr);
	}

	/* Set latency histogram bin size for VO monitors */
	axi_mon_set_lat_bin_size(0xF);

	pr_info("[CV84X6 MON] Initialization complete\n");
}
