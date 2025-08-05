#ifndef __VO_CORE_H__
#define __VO_CORE_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/cdev.h>
#include "defines.h"

struct vo_core_dev {
	// private data
	struct device *dev;
	struct class *vo_class;
	struct cdev cdev;
	dev_t cdev_id;
	struct clk *clk_vo[2];
	//disp, dsi_mac, dsi_phy, vo_mac
	void __iomem *reg_base[4];
};

#ifdef __cplusplus
}
#endif

#endif /* __VO_CORE_H__ */
