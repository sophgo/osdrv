#ifndef _CVI_VIP_MIPI_TX_PROC_H_
#define _CVI_VIP_MIPI_TX_PROC_H_

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <generated/compile.h>
#include <linux/slab.h>
#include <linux/version.h>
#include "mw/vpu_base.h"
#include "cvi_vip_mipi_tx.h"

int mipi_tx_proc_init(void);
int mipi_tx_proc_remove(void);

#endif // _CVI_VIP_MIPI_TX_PROC_H_
