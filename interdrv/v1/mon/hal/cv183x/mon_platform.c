#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/streamline_annotate.h>
#include <linux/clk.h>
#include "mon_platform.h"

struct AXIMON_INFO_PORT {
	uint8_t port_name[16];
	uint32_t bcnts_min;
	uint32_t bcnts_acc;
	uint32_t bcnts_max;
	uint32_t bcnts_avg;
	uint32_t bw_min;
	uint32_t bw_acc;
	uint32_t bw_max;
	uint64_t bw_avg_sum;
	uint64_t count;
	uint32_t time_avg;
};

struct AXIMON_INFO {
	struct AXIMON_INFO_PORT m1;
	struct AXIMON_INFO_PORT m2;
	struct AXIMON_INFO_PORT m3;
	struct AXIMON_INFO_PORT m4;
	struct AXIMON_INFO_PORT m5;
	struct AXIMON_INFO_PORT m6;
	struct AXIMON_INFO_PORT total;
};

#define DRAMTYPE_STRLEN 30
struct DRAM_INFO {
	uint32_t bus_width;
	uint32_t data_rate;
	char type[DRAMTYPE_STRLEN];
};

static struct DRAM_INFO dram_info;
static struct AXIMON_INFO aximon_info;
static void __iomem *iomem_aximon_base;

//#define AXIMON_BASE 0x08008000
#define REMAPPING_BASE 0
#define AXIMON_M1_0_BASE (REMAPPING_BASE + 0x0)
#define AXIMON_M1_1_BASE (REMAPPING_BASE + 0x80)
#define AXIMON_M2_0_BASE (REMAPPING_BASE + 0x100)
#define AXIMON_M2_1_BASE (REMAPPING_BASE + 0x180)
#define AXIMON_M3_0_BASE (REMAPPING_BASE + 0x200)
#define AXIMON_M3_1_BASE (REMAPPING_BASE + 0x280)
#define AXIMON_M4_0_BASE (REMAPPING_BASE + 0x300)
#define AXIMON_M4_1_BASE (REMAPPING_BASE + 0x380)
#define AXIMON_M5_0_BASE (REMAPPING_BASE + 0x400)
#define AXIMON_M5_1_BASE (REMAPPING_BASE + 0x480)
#define AXIMON_M6_0_BASE (REMAPPING_BASE + 0x500)
#define AXIMON_M6_1_BASE (REMAPPING_BASE + 0x580)

#define AXIMON_OFFSET_CYCLE 0x40
#define AXIMON_OFFSET_INPUTSELECT 0x04
#define AXIMON_OFFSET_BYTECNTS 0x48
//#define AXIMON_OFFSET_LATCNTS 0x4C

#define AXIMON_SNAPSHOT_REGVALUE_1 0x40004
#define AXIMON_SNAPSHOT_REGVALUE_2 0x40000

#define AXIMON_START_REGVALUE 0x30001
#define AXIMON_STOP_REGVALUE 0x30002

#define AXIMON_SELECT_READ 0x1
#define AXIMON_SELECT_WRITE 0x2


static void axi_mon_snapshot(uint32_t base_register)
{
	writel(AXIMON_SNAPSHOT_REGVALUE_1, (iomem_aximon_base + base_register));
	writel(AXIMON_SNAPSHOT_REGVALUE_2, (iomem_aximon_base + base_register));
}

static void axi_mon_start(uint32_t base_register)
{
	writel(AXIMON_START_REGVALUE, (iomem_aximon_base + base_register));
}

static void axi_mon_stop(uint32_t base_register)
{
	writel(AXIMON_STOP_REGVALUE, (iomem_aximon_base + base_register));
}

static uint32_t axi_mon_get_byte_cnts(uint32_t base_register)
{
	return readl(iomem_aximon_base + base_register + AXIMON_OFFSET_BYTECNTS);
}

static void axi_mon_count_port_info(uint32_t duration, uint32_t byte_cnt, struct AXIMON_INFO_PORT *axi_info)
{
	uint32_t bw = byte_cnt / duration;

	if (bw) {
		if (!axi_info->bw_min)
			axi_info->bw_min = bw;
		else if (bw < axi_info->bw_min)
			axi_info->bw_min = bw;
	}

	if (bw > axi_info->bw_max)
		axi_info->bw_max = bw;

	axi_info->bw_avg_sum += bw;
	axi_info->count += 1;

	if (axi_info->time_avg)
		axi_info->time_avg = (axi_info->time_avg + duration) >> 1;
	else
		axi_info->time_avg = duration;

#if 0
	pr_debug("%s bw=%d, bw_min=%d, bw_max=%d, bw_avg=%d\n",
								axi_info->port_name, bw,
								axi_info->bw_min, axi_info->bw_max,
								axi_info->bw_avg);

#endif

}

void axi_mon_reset_all(void)
{
	//reset aximon info
	memset(&aximon_info, 0, sizeof(struct AXIMON_INFO));

	//input source select configure
	writel(AXIMON_SELECT_READ, (iomem_aximon_base + AXIMON_M1_0_BASE + AXIMON_OFFSET_INPUTSELECT));
	writel(AXIMON_SELECT_WRITE, (iomem_aximon_base + AXIMON_M1_1_BASE + AXIMON_OFFSET_INPUTSELECT));
	writel(AXIMON_SELECT_READ, (iomem_aximon_base + AXIMON_M2_0_BASE + AXIMON_OFFSET_INPUTSELECT));
	writel(AXIMON_SELECT_WRITE, (iomem_aximon_base + AXIMON_M2_1_BASE + AXIMON_OFFSET_INPUTSELECT));
	writel(AXIMON_SELECT_READ, (iomem_aximon_base + AXIMON_M3_0_BASE + AXIMON_OFFSET_INPUTSELECT));
	writel(AXIMON_SELECT_WRITE, (iomem_aximon_base + AXIMON_M3_1_BASE + AXIMON_OFFSET_INPUTSELECT));
	writel(AXIMON_SELECT_READ, (iomem_aximon_base + AXIMON_M4_0_BASE + AXIMON_OFFSET_INPUTSELECT));
	writel(AXIMON_SELECT_WRITE, (iomem_aximon_base + AXIMON_M4_1_BASE + AXIMON_OFFSET_INPUTSELECT));
	writel(AXIMON_SELECT_READ, (iomem_aximon_base + AXIMON_M5_0_BASE + AXIMON_OFFSET_INPUTSELECT));
	writel(AXIMON_SELECT_WRITE, (iomem_aximon_base + AXIMON_M5_1_BASE + AXIMON_OFFSET_INPUTSELECT));
	writel(AXIMON_SELECT_READ, (iomem_aximon_base + AXIMON_M6_0_BASE + AXIMON_OFFSET_INPUTSELECT));
	writel(AXIMON_SELECT_WRITE, (iomem_aximon_base + AXIMON_M6_1_BASE + AXIMON_OFFSET_INPUTSELECT));

	//configure port name
	strcpy(aximon_info.m1.port_name, "vip_rt");
	strcpy(aximon_info.m2.port_name, "vip_off");
	strcpy(aximon_info.m3.port_name, "cpu");
	strcpy(aximon_info.m4.port_name, "tpu");
	strcpy(aximon_info.m5.port_name, "vc");
	strcpy(aximon_info.m6.port_name, "hsperi");
	strcpy(aximon_info.total.port_name, "total");
}

void axi_mon_start_all(void)
{
	//aximon start trigger
	axi_mon_start(AXIMON_M1_0_BASE);
	axi_mon_start(AXIMON_M1_1_BASE);
	axi_mon_start(AXIMON_M2_0_BASE);
	axi_mon_start(AXIMON_M2_1_BASE);
	axi_mon_start(AXIMON_M3_0_BASE);
	axi_mon_start(AXIMON_M3_1_BASE);
	axi_mon_start(AXIMON_M4_0_BASE);
	axi_mon_start(AXIMON_M4_1_BASE);
	axi_mon_start(AXIMON_M5_0_BASE);
	axi_mon_start(AXIMON_M5_1_BASE);
	axi_mon_start(AXIMON_M6_0_BASE);
	axi_mon_start(AXIMON_M6_1_BASE);
}

void axi_mon_stop_all(void)
{
	axi_mon_stop(AXIMON_M1_0_BASE);
	axi_mon_stop(AXIMON_M1_1_BASE);
	axi_mon_stop(AXIMON_M2_0_BASE);
	axi_mon_stop(AXIMON_M2_1_BASE);
	axi_mon_stop(AXIMON_M3_0_BASE);
	axi_mon_stop(AXIMON_M3_1_BASE);
	axi_mon_stop(AXIMON_M4_0_BASE);
	axi_mon_stop(AXIMON_M4_1_BASE);
	axi_mon_stop(AXIMON_M5_0_BASE);
	axi_mon_stop(AXIMON_M5_1_BASE);
	axi_mon_stop(AXIMON_M6_0_BASE);
	axi_mon_stop(AXIMON_M6_1_BASE);
}

void axi_mon_snapshot_all(void)
{
	axi_mon_snapshot(AXIMON_M1_0_BASE);
	axi_mon_snapshot(AXIMON_M1_1_BASE);
	axi_mon_snapshot(AXIMON_M2_0_BASE);
	axi_mon_snapshot(AXIMON_M2_1_BASE);
	axi_mon_snapshot(AXIMON_M3_0_BASE);
	axi_mon_snapshot(AXIMON_M3_1_BASE);
	axi_mon_snapshot(AXIMON_M4_0_BASE);
	axi_mon_snapshot(AXIMON_M4_1_BASE);
	axi_mon_snapshot(AXIMON_M5_0_BASE);
	axi_mon_snapshot(AXIMON_M5_1_BASE);
	axi_mon_snapshot(AXIMON_M6_0_BASE);
	axi_mon_snapshot(AXIMON_M6_1_BASE);
}

void axi_mon_get_info_all(uint32_t duration)
{
	uint32_t cur_byte_cnt = 0, sum_byte_cnt = 0;
	uint32_t snapshot_tick = 0;

	snapshot_tick = readl(iomem_aximon_base + AXIMON_OFFSET_CYCLE);
	pr_debug("snapshot_tick=%d, dram_rate=%d\n", snapshot_tick, dram_info.data_rate);

	if (dram_info.data_rate) {
		duration = snapshot_tick / (dram_info.data_rate >> 2);

		cur_byte_cnt = axi_mon_get_byte_cnts(AXIMON_M1_0_BASE) + axi_mon_get_byte_cnts(AXIMON_M1_1_BASE);
		axi_mon_count_port_info(duration, cur_byte_cnt, &aximon_info.m1);
		sum_byte_cnt += cur_byte_cnt;

		cur_byte_cnt = axi_mon_get_byte_cnts(AXIMON_M2_0_BASE) + axi_mon_get_byte_cnts(AXIMON_M2_1_BASE);
		axi_mon_count_port_info(duration, cur_byte_cnt, &aximon_info.m2);
		sum_byte_cnt += cur_byte_cnt;

		cur_byte_cnt = axi_mon_get_byte_cnts(AXIMON_M3_0_BASE) + axi_mon_get_byte_cnts(AXIMON_M3_1_BASE);
		axi_mon_count_port_info(duration, cur_byte_cnt, &aximon_info.m3);
		sum_byte_cnt += cur_byte_cnt;

		cur_byte_cnt = axi_mon_get_byte_cnts(AXIMON_M4_0_BASE) + axi_mon_get_byte_cnts(AXIMON_M4_1_BASE);
		axi_mon_count_port_info(duration, cur_byte_cnt, &aximon_info.m4);
		sum_byte_cnt += cur_byte_cnt;

		cur_byte_cnt = axi_mon_get_byte_cnts(AXIMON_M5_0_BASE) + axi_mon_get_byte_cnts(AXIMON_M5_1_BASE);
		axi_mon_count_port_info(duration, cur_byte_cnt, &aximon_info.m5);
		sum_byte_cnt += cur_byte_cnt;

		cur_byte_cnt = axi_mon_get_byte_cnts(AXIMON_M6_0_BASE) + axi_mon_get_byte_cnts(AXIMON_M6_1_BASE);
		axi_mon_count_port_info(duration, cur_byte_cnt, &aximon_info.m6);
		sum_byte_cnt += cur_byte_cnt;

		axi_mon_count_port_info(duration, sum_byte_cnt, &aximon_info.total);
	} else {
		pr_err("read dram_rate=%d\n", dram_info.data_rate);
	}
}

void axi_mon_dump_single(struct AXIMON_INFO_PORT *port_info)
{
	uint64_t dividend = port_info->bw_avg_sum;

	do_div(dividend, port_info->count);
	pr_err("%-8s bw_avg=%5dMB/s, bw_min=%5dMB/s, bw_max=%5dMB/s\n",
							port_info->port_name, (uint32_t)dividend,
							port_info->bw_min, port_info->bw_max);
}

void axi_mon_dump(void)
{
	pr_err("==============================\n");
	pr_err("%s profiling window time_avg=%3dms\n",
		dram_info.type, aximon_info.total.time_avg / 1000);

	axi_mon_dump_single(&aximon_info.m1);
	axi_mon_dump_single(&aximon_info.m2);
	axi_mon_dump_single(&aximon_info.m3);
	axi_mon_dump_single(&aximon_info.m4);
	axi_mon_dump_single(&aximon_info.m5);
	axi_mon_dump_single(&aximon_info.m6);
	axi_mon_dump_single(&aximon_info.total);
	pr_err("==============================\n");
}

#define CLK_DIVIDEND_MAGIC 0x1770000000
#define CLK_RATE_OFFSET 0x34
static void axi_mon_get_dram_freq(struct cvi_mon_device *ndev)
{
	uint32_t reg = 0;
	char tmp_str[DRAMTYPE_STRLEN];
	uint64_t dividend = CLK_DIVIDEND_MAGIC;

	reg = readl(ndev->ddr_phyd_vaddr + CLK_RATE_OFFSET);
	pr_debug("axi_mon_get_dram_freq() reg=%x\n", reg);

	do_div(dividend, reg);
	dram_info.data_rate = ((uint32_t)dividend) << 4;

	snprintf(tmp_str, DRAMTYPE_STRLEN, "_%dMhz", dram_info.data_rate);
	strncat(dram_info.type, tmp_str, DRAMTYPE_STRLEN);
}

static void axi_mon_get_ddr_bus_width(struct cvi_mon_device *ndev)
{
	uint32_t reg = 0;

	reg = readl(ndev->ddr_ctrl_vaddr);

	if (((reg >> 12) & 0x3) == 0) {
		dram_info.bus_width = 32;
		strncat(dram_info.type, "_32bit", DRAMTYPE_STRLEN);
	} else if (((reg >> 12) & 0x3) == 1) {
		dram_info.bus_width = 16;
		strncat(dram_info.type, "_16bit", DRAMTYPE_STRLEN);
	} else
		pr_err("get DDR bus width error, value=(0x%08X)\n", reg);
}

static void axi_mon_get_dram_type(struct cvi_mon_device *ndev)
{
	uint32_t reg = 0;

	reg = readl(ndev->ddr_ctrl_vaddr);

	if (reg & 0x1)
		strncpy(dram_info.type, "DDR3", DRAMTYPE_STRLEN);
	else if (reg & 0x10)
		strncpy(dram_info.type, "DDR4", DRAMTYPE_STRLEN);
	else if (reg & 0x20)
		strncpy(dram_info.type, "LPDDR4", DRAMTYPE_STRLEN);
	else
		pr_err("get DDR type error, value=(0x%08X)\n", reg);
}

void axi_mon_init(struct cvi_mon_device *ndev)
{
	iomem_aximon_base = ndev->ddr_aximon_vaddr;

	memset(&dram_info, 0, sizeof(struct DRAM_INFO));
	axi_mon_get_dram_type(ndev);
	axi_mon_get_dram_freq(ndev);
	axi_mon_get_ddr_bus_width(ndev);
}

