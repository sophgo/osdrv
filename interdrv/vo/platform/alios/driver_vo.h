#ifndef __DRIVER_VO_H__
#define __DRIVER_VO_H__

#ifdef __cplusplus
	extern "C" {
#endif

long driver_vo_ioctl(unsigned int cmd, unsigned long arg);
int driver_vo_exit();
int driver_vo_init();
int driver_vo_release();

#ifdef __cplusplus
}
#endif

#endif /* __DRIVER_VO_H__ */
