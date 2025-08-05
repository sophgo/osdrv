#include "vi_misc.h"
#include "vi_common.h"

#include <linux/of_gpio.h>

static int gpio_base_idx[VI_MISC_GPIO_PORT_MAX] = {480, 448, 416, 384, 352};
static int gpio_invalid_idx[VI_MISC_GPIO_PORT_MAX] = {512, 480, 448, 416, 384};

int vi_misc_gpio_request(void **handle, int port, int pin, char *label)
{
	int ret = 0;
	int gpio_idx = 0;

	if (port >= VI_MISC_GPIO_PORT_MAX || port < 0) {
		vi_pr(VI_ERR, "Invalid port(%d)\n", port);
		return -1;
	}

	if (label == NULL) {
		vi_pr(VI_ERR, "Invalid label\n");
		return -1;
	}

	gpio_idx = gpio_base_idx[port] + pin;
	if (gpio_idx >= gpio_invalid_idx[port]) {
		vi_pr(VI_ERR, "Invalid handle or port(%d) pin(%d)\n", port, pin);
		return -1;
	}

	ret = gpio_request(gpio_idx, label);
	if (ret) {
		vi_pr(VI_ERR, "request for port(%d, %d) gpio_%d failed:%d\n", port, pin, gpio_idx, ret);
		return ret;
	}

	return ret;
}

int vi_misc_gpio_dir(void **handle, int port, int pin, int dir, int value)
{
	int ret = 0;
	int gpio_idx = 0;

	if (port >= VI_MISC_GPIO_PORT_MAX || port < 0) {
		vi_pr(VI_ERR, "Invalid port(%d)\n", port);
		return -1;
	}

	gpio_idx = gpio_base_idx[port] + pin;
	if (gpio_idx >= gpio_invalid_idx[port]) {
		vi_pr(VI_ERR, "Invalid handle or port(%d) pin(%d)\n", port, pin);
		return -1;
	}

	if (dir) {
		ret = gpio_direction_output(gpio_idx, value);
	} else {
		ret = gpio_direction_input(gpio_idx);
	}

	return ret;
}

int vi_misc_gpio_set_value(void **handle, int port, int pin, int value)
{
	int ret = 0;
	int gpio_idx = 0;

	if (port >= VI_MISC_GPIO_PORT_MAX || port < 0) {
		vi_pr(VI_ERR, "Invalid port(%d)\n", port);
		return -1;
	}

	gpio_idx = gpio_base_idx[port] + pin;
	if (gpio_idx >= gpio_invalid_idx[port]) {
		vi_pr(VI_ERR, "Invalid handle or port(%d) pin(%d)\n", port, pin);
		return -1;
	}

	gpio_set_value(gpio_idx, value);

	return ret;
}

int vi_misc_gpio_free(void **handle, int port, int pin)
{
	int ret = 0;
	int gpio_idx = 0;

	if (port >= VI_MISC_GPIO_PORT_MAX || port < 0) {
		vi_pr(VI_ERR, "Invalid port(%d)\n", port);
		return -1;
	}

	gpio_idx = gpio_base_idx[port] + pin;
	if (gpio_idx >= gpio_invalid_idx[port]) {
		vi_pr(VI_ERR, "Invalid handle or port(%d) pin(%d)\n", port, pin);
		return -1;
	}

	gpio_free(gpio_idx);

	return ret;
}
