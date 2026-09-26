/* SPDX-License-Identifier: GPL-2.0-only*/
#ifndef __MON_PLATFORM_H__
#define __MON_PLATFORM_H__

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include "cvi_mon_interface.h"

/* CV84X6 AXI Monitor Register Offsets (per channel) */
#define REG_MISC			0x00
#define REG_HIT_MISC			0x04
#define REG_HIT_ADDR_ST_L		0x08
#define REG_HIT_ADDR_ST_H		0x0C
#define REG_HIT_ADDR_SP_L		0x10
#define REG_HIT_ADDR_SP_H		0x14
#define REG_HIT_ID_MASK			0x18
#define REG_HIT_ID			0x1C
#define REG_HIT_AXI_ATTR		0x20	/* hit_len/size/burst/lock/cache/prot/qos */
#define REG_CYCLE_CNT			0x24	/* [27:0] 28-bit, max ~224ms @1200MHz */
#define REG_HIT_CNT			0x28	/* [23:0] 24-bit */
#define REG_BYTE_CNT			0x2C	/* [31:0] 32-bit */
#define REG_LATENCY_CNT			0x34	/* [31:0] 32-bit, += outstanding/cycle */
#define REG_OUTSTANDING_CNT		0x38	/* [7:0]  8-bit, current outstanding */
#define REG_IRQ_ADDR_L			0x3C
#define REG_IRQ_ADDR_H			0x40
#define REG_IRQ_ID			0x44
#define REG_IRQ_AXI_ATTR		0x48	/* irq_len/size/burst/lock/cache/prot/qos */
#define REG_CYC_LAT_BIN_SIZE_SEL	0x50
#define REG_LATENCY_HIS_BASE		0x54

/* Channel offsets within each monitor */
#define AXIMON_CH_WRITE			0x00
#define AXIMON_CH_READ			0x80

/* MISC register bits */
#define MISC_CLEAR			BIT(1)
#define MISC_IRQ_EN			BIT(4)
#define MISC_IRQ_CLEAR			BIT(5)
#define MISC_IRQ_STATUS			BIT(7)
#define MISC_INPUT_SEL_SHIFT		8
#define MISC_INPUT_SEL_MASK		(0x3F << MISC_INPUT_SEL_SHIFT)

/*
 * Control values with mask mechanism
 * bits[31:16] = mask, bits[15:0] = value
 */
#define AXIMON_SELECT_CLK		0x01000100	/* input_sel=1, mask=input_sel */
#define AXIMON_START_VALUE		0x00010001	/* func_en=1, mask=bit[0] */
#define AXIMON_STOP_VALUE		0x00010000	/* func_en=0, mask=bit[0] */
#define AXIMON_SNAPSHOT_VALUE_1		0x00040004	/* snapshot=1, mask=bit[2] */
#define AXIMON_SNAPSHOT_VALUE_2		0x00040000	/* snapshot=0, mask=bit[2] */
#define AXIMON_CLEAR_VALUE		0x00020002	/* clear=1, mask=bit[1] */
#define AXIMON_CLEAR_DONE		0x00020000	/* clear=0, mask=bit[1] */
#define AXIMON_IRQ_EN_VALUE		0x00100010	/* irq_en=1, mask=bit[4] */
#define AXIMON_IRQ_DIS_VALUE		0x00100000	/* irq_en=0, mask=bit[4] */
#define AXIMON_IRQ_CLR_VALUE		0x00200020	/* irq_clear=1, mask=bit[5] */
#define AXIMON_IRQ_CLR_DONE		0x00200000	/* irq_clear=0, mask=bit[5] */

/* Number of latency histogram bins */
#define LATENCY_HIS_COUNT		11

/* Maximum name length */
#define MON_NAME_LEN			16

/* Per-port bandwidth and latency statistics */
struct aximon_port_info {
	char port_name[MON_NAME_LEN];
	char id_name[MON_NAME_LEN];
	/* Byte count for write/read channels */
	uint64_t byte_cnt_wr;
	uint64_t byte_cnt_rd;
	/* Write bandwidth statistics (MB/s) */
	uint32_t bw_wr_min;
	uint32_t bw_wr_max;
	uint64_t bw_wr_avg_sum;		/* Sum for non-zero samples only */
	uint64_t bw_wr_avg_tot_sum;	/* Sum for all samples */
	uint64_t bw_wr_count;		/* Valid samples with byte_cnt_wr > 0 */
	/* Read bandwidth statistics (MB/s) */
	uint32_t bw_rd_min;
	uint32_t bw_rd_max;
	uint64_t bw_rd_avg_sum;		/* Sum for non-zero samples only */
	uint64_t bw_rd_avg_tot_sum;	/* Sum for all samples */
	uint64_t bw_rd_count;		/* Valid samples with byte_cnt_rd > 0 */
	/* Total bandwidth statistics (kept for compatibility) */
	uint32_t bw_min;
	uint32_t bw_max;
	uint64_t bw_avg_sum;
	uint64_t bw_count;	/* Valid samples with byte_cnt > 0 */
	uint64_t count;		/* Total samples including zero */
	uint64_t time_avg_sum;	/* Sum of hw_duration_us for arithmetic average */
	/* Latency statistics (in ns) */
	uint64_t latency_write_avg;
	uint32_t latency_write_avg_max;
	uint32_t latency_write_avg_min;
	uint64_t latency_write_count;	/* Valid samples with hit_cnt > 0 */
	uint64_t latency_read_avg;
	uint32_t latency_read_avg_max;
	uint32_t latency_read_avg_min;
	uint64_t latency_read_count;	/* Valid samples with hit_cnt > 0 */
	/* Latency histogram (VO only) */
	uint64_t write_latency_his_cnt[LATENCY_HIS_COUNT];
	uint64_t read_latency_his_cnt[LATENCY_HIS_COUNT];
};

/* Monitor instance descriptor */
struct aximon_instance {
	const char *name;
	const char *id_name;
	const char *desc;		/* Subsystem description for display */
	void __iomem *vaddr;
	phys_addr_t paddr;		/* Physical base address */
	int irq_wr;			/* Write channel IRQ */
	int irq_rd;			/* Read channel IRQ */
	uint32_t axi_clk;		/* AXI clock frequency in MHz */
	bool has_his;			/* Support latency histogram */
	bool no_irq;			/* No IRQ in DTS (VO/TPU monitors) */
	/* Per-instance hit filter configuration */
	uint32_t hit_sel;		/* 0=all, 2=ID filter */
	uint32_t hit_id;		/* AXI ID value */
	uint32_t hit_id_mask;		/* AXI ID mask */
	struct aximon_port_info info;
};

void axi_mon_reset_all(void);
void axi_mon_start_all(void);
void axi_mon_stop_all(void);
void axi_mon_snapshot_all(void);
void axi_mon_get_info_all(uint32_t duration);
void axi_mon_clear_hw_counters(void);
void axi_mon_dump(void);
void axi_mon_init(struct cvi_mon_device *ndev);

/* Hit filter configuration API (for procfs) */
int axi_mon_set_hit_filter(const char *name, uint32_t sel, uint32_t id,
			   uint32_t mask);
int axi_mon_get_hit_filter_info(char *buf, int size);

#endif
