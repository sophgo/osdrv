#ifndef _MIPI_TX_H_
#define _MIPI_TX_H_

int driver_mipi_tx_ioctl(unsigned int cmd, unsigned long arg);
int driver_mipi_tx_init();
int driver_mipi_tx_exit();

#endif // _MIPI_TX_H_
