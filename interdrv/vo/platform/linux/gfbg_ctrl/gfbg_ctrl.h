#ifndef _GFBG_CTRL_H_
#define _GFBG_CTRL_H_

#include "vo_debug.h"
#include "common.h"

int vo_gfbg_get_bind_dev_id(vo_layer layer, vo_dev *dev);
int vo_gfbg_get_hw_layer_id(vo_layer layer, vo_layer *hw_layer);
int vo_gfbg_dev_is_enable(vo_dev dev, bool *is_enable);
int vo_gfbg_set_layer_enable(vo_layer layer, bool enable);
int vo_gfbg_get_layer_enable(vo_layer layer, bool *enable);

#endif /* _VO_GFBG_CTRL_H_ */
