/* Copyright (C) 2023 MacroSilicon Technology Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * usb_hal_interface.h -- Framebuffer driver for MacroSilicon chip 913x and 912x
 */

#ifndef __USB_HAL_INTERFACE_H__
#define __USB_HAL_INTERFACE_H__

#include <linux/types.h>
#include <linux/mutex.h>

struct usb_interface;
struct usb_device_id;
struct device;
struct kfifo;

struct usb_hal_video_mode
{
	u16 width;
	u16 height;
	u16 rate;
	u8 vic;
};

struct usb_hal
{
	struct usb_interface *interface;
	void *private;

	u8 chip_id;
	u8 port_type;
	u8 sdram_type;
};

struct usb_hal *usb_intf_device_to_hal_func(struct device *dev);

struct usb_hal *usb_hal_init(struct usb_interface *interface, const struct usb_device_id *id, struct kfifo *fifo, u32 index);
void usb_hal_destroy(struct usb_hal *hal);

int usb_hal_get_hpd_status(struct usb_hal *hal, u32 *status);
int usb_hal_get_edid(struct usb_hal *hal, int block, u8 *buf, u32 len);
int usb_hal_video_mode_valid(struct usb_hal *hal, struct usb_hal_video_mode *mode);
int usb_hal_get_vic(struct usb_hal *hal, u16 width, u16 height, u8 rate, u8 *vic);
int usb_hal_enable(struct usb_hal *hal, struct usb_hal_video_mode *mode, u32 fourcc);
int usb_hal_disable(struct usb_hal *hal);
int usb_hal_is_disabled(struct usb_hal *hal);
int usb_hal_update_frame(struct usb_hal *hal, u8 *buf, int pitch, u32 len, u32 fourcc, int try_lock);
int usb_hal_is_support_fourcc(u32 fourcc);
unsigned int usb_hal_get_bpp_by_fourcc(u32 fourcc);
int usb_hal_add_custom_mode(struct usb_hal *hal, int width, int height, int rate, unsigned char vic);

#endif
