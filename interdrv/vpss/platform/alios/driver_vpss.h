#ifndef __DRIVER_VPSS_H__
#define __DRIVER_VPSS_H__

int driver_vpss_init(void);
void driver_vpss_exit(void);
long driver_vpss_ioctl(unsigned int cmd, unsigned long arg);

#endif