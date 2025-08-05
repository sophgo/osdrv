#ifndef _MIPI_TX_H_
#define _MIPI_TX_H_

#include <linux/clk.h>

int mipi_tx_get_combo_dev_cfg(struct combo_dev_cfg_s *dev_cfg, int devno);

#endif // _MIPI_TX_H_
