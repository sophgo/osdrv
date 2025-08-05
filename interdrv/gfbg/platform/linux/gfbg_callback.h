#ifndef GFBG_CALLBACK_H
#define GFBG_CALLBACK_H

#include "base_cb.h"
#include "disp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

int gfbg_cb(void *dev, cb_modules_id caller, unsigned int cmd, void *arg);
void gfbg_vo_int_cb(vo_dev dev_id, vo_layer layer_id, union disp_intr intr_status);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
