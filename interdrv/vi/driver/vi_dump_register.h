#ifndef __VI_DUMP_REGISTER__
#define __VI_DUMP_REGISTER__

#include "vi_defines.h"

struct isp_dump_info {
	uint64_t phy_base;
	uint64_t reg_base;
	uint32_t blk_size;
};

struct reg_tbl {
	int addr_ofs;
	int val_ofs;
	int data;
	int mask;
};

struct gamma_tbl {
	char name[16];
	int length;
	uintptr_t addr;
	struct reg_tbl enable;
	struct reg_tbl shdw_sel;
	struct reg_tbl force_clk_enable;
	struct reg_tbl prog_en;
	struct reg_tbl raddr;
	struct reg_tbl rdata_r;
	struct reg_tbl rdata_gb;
};

int vi_dump_register(struct sop_vi_dev *vdev, int pipe, void *addr, int *size);

#endif
