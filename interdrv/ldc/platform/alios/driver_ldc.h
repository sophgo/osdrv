#ifndef _LDC_DRIVER_H_
#define _LDC_DRIVER_H_

int driver_ldc_init(void);
int driver_ldc_exit(void);
long driver_ldc_ioctl(unsigned int cmd, void *arg);

#endif /* _LDC_DRIVER_H_ */
