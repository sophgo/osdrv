#ifndef __VO_PLATFORM_H__
#define __VO_PLATFORM_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/platform_device.h>

long vo_ioctl(struct file *filp, u_int cmd, u_long arg);
int vo_open(struct inode *inode, struct file *filp);
int vo_release(struct inode *inode, struct file *filp);
unsigned int vo_poll(struct file *filp, struct poll_table_struct *wait);
void vo_config_pinmux(unsigned int pad);
int vo_create_instance(struct platform_device *pdev);
int vo_destroy_instance(struct platform_device *pdev);

#ifdef __cplusplus
}
#endif

#endif /* __VO_PLATFORM_H__ */
