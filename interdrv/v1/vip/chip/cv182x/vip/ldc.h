/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: ldc.h
 * Description:
 */

#ifndef _CVI_LDC_H_
#define _CVI_LDC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ldc_cfg.h"

void ldc_set_base_addr(void *base);

/**
 * ldc_init - setup ldc, mainly interpolation settings.
 *
 */
void ldc_init(void);

/**
 * ldc_reset - do reset. This can be activated only if dma stop to avoid hang
 *	       fabric.
 *
 */
void ldc_reset(void);

/**
 * ldc_check_param - check if config param is valid.
 *
 * @param cfg: settings for this ldc's operation
 * @return: true for valid.
 */
bool ldc_check_param(const struct ldc_cfg *cfg);

/**
 * ldc_engine - start a ldc operation, wait frame_done intr after this.
 *              If output target is scaler, scaler's config should be done
 *              before this.
 *
 * @param cfg: settings for this ldc's operation
 */
void ldc_engine(const struct ldc_cfg *cfg);

/**
 * ldc_intr_ctrl - ldc's interrupt on(1)/off(0)
 *                 bit0: frame_done, bit1: mesh_id axi read err,
 *                 bit2: mesh_table axi read err
 *
 * @param intr_mask: On/Off ctrl of the interrupt.
 */
void ldc_intr_ctrl(u8 intr_mask);

/**
 * ldc_intr_ctrl - clear ldc's interrupt
 *                 bit0: frame_done, bit1: mesh_id axi read err,
 *                 bit2: mesh_table axi read err
 *
 * @param intr_mask: On/Off ctrl of the interrupt.
 */
void ldc_intr_clr(u8 intr_mask);

/**
 * ldc_intr_status - ldc's interrupt status
 *                 bit0: frame_done, bit1: mesh_id axi read err,
 *                 bit2: mesh_table axi read err
 *
 * @return: The interrupt's status. 1 if active.
 */
u8 ldc_intr_status(void);

/**
 * ldc_is_busy - check if ldc's operation is finished.
 *              ldc_start can only be toggled if only dma done(frame_done intr),
 *              ow dma won't finished.
 */
bool ldc_is_busy(void);

#ifdef __cplusplus
}
#endif

#endif /* _CVI_LDC_H_ */
