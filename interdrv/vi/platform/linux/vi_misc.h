#ifndef __VI_MISC_H__

#define VI_MISC_GPIO_PORT_MAX            (5)

int vi_misc_gpio_request(void **handle, int port, int pin, char *label);
int vi_misc_gpio_dir(void **handle, int port, int pin, int dir, int value);
int vi_misc_gpio_set_value(void **handle, int port, int pin, int value);
int vi_misc_gpio_free(void **handle, int port, int pin);

#endif
