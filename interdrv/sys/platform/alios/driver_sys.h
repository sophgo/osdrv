#ifndef __DRIVER_SYS_H__
#define __DRIVER_SYS_H__

int driver_sys_init(void);
void driver_sys_exit(void);
long driver_sys_ioctl(unsigned int cmd, unsigned long arg);

#endif
