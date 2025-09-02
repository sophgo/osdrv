#ifndef __DRIVER_BASE_H__
#define __DRIVER_BASE_H__

int driver_base_init(void);
void driver_base_exit(void);
void driver_base_release(void);
long driver_base_ioctl(unsigned int cmd, unsigned long arg);

#endif
