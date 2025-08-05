#ifndef __DRIVER_VI_H__
#define __DRIVER_VI_H__

enum poll_type {
	POLL_TYPE_EVENT = 0,
	POLL_TYPE_DBG,
	POLL_TYPE_MAX,
};

int driver_vi_open(void);
int driver_vi_release(void);
int driver_vi_poll(enum poll_type type);
int driver_vi_init(void);
int driver_vi_exit(void);
int vi_core_suspend(void);
int vi_core_resume(void);
long driver_vi_ioctl(unsigned int cmd, unsigned long arg);

#endif
