//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
//
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
//
// The entire notice above must be reproduced on all authorized copies.
//
// Description  :
//-----------------------------------------------------------------------------
#ifndef __JPU_DRV_H__
#define __JPU_DRV_H__

#include <linux/fs.h>
#include <linux/types.h>
#include "jpuconfig.h"
#define JDI_IOCTL_MAGIC  'J'

#define JDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY          _IO(JDI_IOCTL_MAGIC, 0)
#define JDI_IOCTL_FREE_PHYSICALMEMORY               _IO(JDI_IOCTL_MAGIC, 1)
#define JDI_IOCTL_WAIT_INTERRUPT                    _IO(JDI_IOCTL_MAGIC, 2)
#define JDI_IOCTL_SET_CLOCK_GATE                    _IO(JDI_IOCTL_MAGIC, 3)
#define JDI_IOCTL_RESET                             _IO(JDI_IOCTL_MAGIC, 4)
#define JDI_IOCTL_GET_INSTANCE_POOL                 _IO(JDI_IOCTL_MAGIC, 5)
#define JDI_IOCTL_GET_RESERVED_VIDEO_MEMORY_INFO    _IO(JDI_IOCTL_MAGIC, 6)
#define JDI_IOCTL_GET_REGISTER_INFO                 _IO(JDI_IOCTL_MAGIC, 7)
#define JDI_IOCTL_OPEN_INSTANCE                     _IO(JDI_IOCTL_MAGIC, 8)
#define JDI_IOCTL_CLOSE_INSTANCE                    _IO(JDI_IOCTL_MAGIC, 9)
#define JDI_IOCTL_GET_INSTANCE_NUM                  _IO(JDI_IOCTL_MAGIC, 10)
#define JDI_IOCTL_GET_INSTANCE_CORE_INDEX           _IO(JDI_IOCTL_MAGIC, 11)
#define JDI_IOCTL_CLOSE_INSTANCE_CORE_INDEX      	_IO(JDI_IOCTL_MAGIC, 12)
#define JDI_IOCTL_GET_MAX_NUM_JPU_CORE           	_IO(JDI_IOCTL_MAGIC, 13)


typedef struct jpudrv_buffer_t {
    //unsigned int size;
    unsigned long size;
    unsigned long phys_addr;
    unsigned long base;                     /* kernel logical address in use kernel */
    unsigned long virt_addr;                /* virtual user space address */
    unsigned int  is_cached;
} jpudrv_buffer_t;

typedef struct jpudrv_inst_info_t {
    unsigned int inst_idx;
    int inst_open_count;    /* for output only*/
    unsigned int core_idx;
} jpudrv_inst_info_t;

typedef struct jpudrv_intr_info_t {
    unsigned int    timeout;
    int             intr_reason;
    unsigned int    inst_idx;
    unsigned int    core_idx;
} jpudrv_intr_info_t;

#define MAX_JPU_STAT_WIN_SIZE  10
typedef struct {
    uint64_t jpu_working_time_in_ms[MAX_NUM_JPU_CORE];
    uint64_t jpu_total_time_in_ms[MAX_NUM_JPU_CORE];
    uint64_t jpu_stat_cycles[MAX_NUM_JPU_CORE];
    int jpu_working_array[MAX_NUM_JPU_CORE][MAX_JPU_STAT_WIN_SIZE];
    int jpu_status_index[MAX_NUM_JPU_CORE];
    int jpu_instant_usage[MAX_NUM_JPU_CORE];
    int jpu_instant_count[MAX_NUM_JPU_CORE];
    uint64_t jpu_laster_time[MAX_NUM_JPU_CORE];
} jpu_statistic_info_t;

/* Structure representing JPU instance statistics */
typedef struct jpu_instance_stats {
    int core_id;             // Core identifier
    int instance_id;         // Instance identifier

    enum { DEC = 1, ENC } state;// Current state (1: decoding, 2: encoding)
    int width;               // Frame width
    int height;              // Frame height

    unsigned long long dec_nr;          // Total decoded frames
    unsigned long long dec_err_nr;      // Total decoding errors
    unsigned long long enc_nr;          // Total encoded frames
    unsigned long long enc_err_nr;      // Total encoding errors
    int last_dec_err;                  // Last enc error code
    int last_enc_err;                  // Last enc error code

    int fps;                 // Calculated frames per second
    u64 last_fps_ts;
    int fps_counter;
    u64 last_frame_ts;
} jpu_inst_info_t;
uint64_t jpu_get_current_time(void);
void jpu_clear_stat_info(int coreIdx);
int jpu_get_register_info(int core_idx, jpudrv_buffer_t *arg);
int jpu_reset(int core_idx);
int jpu_wait_interrupt(jpudrv_intr_info_t *arg);
int jpu_free_memory(jpudrv_buffer_t *arg);
int jpu_alloc_memory(jpudrv_buffer_t *arg);
int jpu_invalidate_cache(jpudrv_buffer_t *arg);
int jpu_flush_cache(jpudrv_buffer_t *arg);
int jpu_core_release_resource(int id);
int jpu_core_request_resource(int timeout);
int jpu_open_device(void);
int jpu_get_instancepool(jpudrv_buffer_t* arg);
int jpu_open_instance(jpudrv_inst_info_t *instInfo);
int jpu_close_instance(jpudrv_inst_info_t *instInfo);
int jpu_set_clock_gate(int core_idx, int *enable);
uint32_t jpu_get_extension_address(int core_idx);
void jpu_set_extension_address(int core_idx, uint32_t addr);
void jpu_sw_top_reset(int core_idx);
void jpu_lock(void);
void jpu_unlock(void);

extern jpu_statistic_info_t s_jpu_usage_info;
#endif
