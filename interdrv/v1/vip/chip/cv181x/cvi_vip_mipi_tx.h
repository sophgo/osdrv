#ifndef _CVI_VIP_MIPI_TX_H_
#define _CVI_VIP_MIPI_TX_H_

#include "uapi/cvi_vip_mipi_tx.h"

extern bool __clk_is_enabled(struct clk *clk);
int mipi_tx_get_combo_dev_cfg(struct combo_dev_cfg_s *dev_cfg);

#endif // _CVI_VIP_MIPI_TX_H_
