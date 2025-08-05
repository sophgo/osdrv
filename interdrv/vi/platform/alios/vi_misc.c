#include "vi_misc.h"
#include "vi_common.h"

#include "module/osal_addr.h"
#include <drv/gpio.h>

static int gpio_base_idx[VI_MISC_GPIO_PORT_MAX] = {480, 448, 416, 384, 352};
static int gpio_invalid_idx[VI_MISC_GPIO_PORT_MAX] = {512, 480, 448, 416, 384};

int vi_misc_gpio_request(void **handle, int port, int pin, char *label)
{
	int ret = 0;
	int gpio_idx = 0;
	csi_gpio_t *gpio_handle = NULL;

	if ((*handle)) {
		vi_pr(VI_DBG, "gpio have been request\n");
		return 0;
	}

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

	gpio_handle = (csi_gpio_t *)osal_zalloc(sizeof(csi_gpio_t));
	if (!gpio_handle) {
		vi_pr(VI_ERR, "Failed to alloc gpio handle\n");
		return -1;
	}

	ret = csi_gpio_init(gpio_handle, port);
	if (ret) {
		vi_pr(VI_ERR, "request for port(%d, %d) gpio_%d failed:%d\n", port, pin, gpio_idx, ret);
		return ret;
	}

	*handle = gpio_handle;

	return ret;
}

int vi_misc_gpio_dir(void **handle, int port, int pin, int dir, int value)
{
	int ret = 0;
	int gpio_idx = 0;

	if (!(*handle)) {
		vi_pr(VI_ERR, "gpio need init before use\n");
		return 0;
	}

	if (port >= VI_MISC_GPIO_PORT_MAX || port < 0) {
		vi_pr(VI_ERR, "Invalid port(%d)\n", port);
		return -1;
	}

	gpio_idx = gpio_base_idx[port] + pin;
	if (gpio_idx >= gpio_invalid_idx[port]) {
		vi_pr(VI_ERR, "Invalid handle or port(%d) pin(%d)\n", port, pin);
		return -1;
	}

	ret = csi_gpio_dir(*handle, 1 << pin, dir);
	if (ret) {
		vi_pr(VI_ERR, "set port(%d, %d) gpio_%d dir failed:%d\n", port, pin, gpio_idx, ret);
		return ret;
	}

	csi_gpio_write(*handle, 1 << pin, value);

	return ret;
}

int vi_misc_gpio_set_value(void **handle, int port, int pin, int value)
{
	int ret = 0;
	int gpio_idx = 0;

	if (!(*handle)) {
		vi_pr(VI_ERR, "gpio need init before use\n");
		return 0;
	}

	if (port >= VI_MISC_GPIO_PORT_MAX || port < 0) {
		vi_pr(VI_ERR, "Invalid port(%d)\n", port);
		return -1;
	}

	gpio_idx = gpio_base_idx[port] + pin;
	if (gpio_idx >= gpio_invalid_idx[port]) {
		vi_pr(VI_ERR, "Invalid handle or port(%d) pin(%d)\n", port, pin);
		return -1;
	}

	csi_gpio_write(*handle, 1 << pin, value);

	return ret;
}

int vi_misc_gpio_free(void **handle, int port, int pin)
{
	int ret = 0;
	int gpio_idx = 0;

	if (!(*handle)) {
		vi_pr(VI_ERR, "gpio need init before use\n");
		return 0;
	}

	if (port >= VI_MISC_GPIO_PORT_MAX || port < 0) {
		vi_pr(VI_ERR, "Invalid port(%d)\n", port);
		return -1;
	}

	gpio_idx = gpio_base_idx[port] + pin;
	if (gpio_idx >= gpio_invalid_idx[port]) {
		vi_pr(VI_ERR, "Invalid handle or port(%d) pin(%d)\n", port, pin);
		return -1;
	}

	csi_gpio_uninit(*handle);

	osal_free(*handle);
	*handle = NULL;

	return ret;
}
