#ifndef __VO_PLATFORM_H__
#define __VO_PLATFORM_H__

#ifdef __cplusplus
	extern "C" {
#endif

int vo_open();
int vo_release();

int vo_create_instance();
int vo_destroy_instance();
void vo_config_pinmux(unsigned int pad);

#ifdef __cplusplus
}
#endif

#endif /* __VO_PLATFORM_H__ */
