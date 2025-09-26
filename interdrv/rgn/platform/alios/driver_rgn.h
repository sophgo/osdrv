#ifndef __RGN_DRIVER_H__
#define __RGN_DRIVER_H__

int driver_rgn_init(void);
int driver_rgn_exit(void);
long driver_rgn_ioctl(unsigned int cmd, unsigned long arg);
void driver_rgn_release(void);

#endif /* __RGN_DRIVER_H__ */
