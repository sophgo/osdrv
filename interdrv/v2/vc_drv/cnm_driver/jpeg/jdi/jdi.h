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
#ifndef _JDI_HPI_H_
#define _JDI_HPI_H_

#include <linux/file.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/mm.h>
#include <linux/export.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/spinlock.h>
#include <linux/rcupdate.h>
#include <linux/mutex.h>
#include <linux/gfp.h>
#include <linux/pid.h>
#include <linux/types.h>

#include "../jpuapi/jpuconfig.h"
#include "mm.h"
#include "jputypes.h"

#define MAX_JPU_BUFFER_POOL 512
#define JpuWriteInstReg(INST_IDX, ADDR, DATA) jdi_write_register_ext(pJpgInst->coreIndex, ((unsigned long)(INST_IDX * NPT_REG_SIZE) + ADDR), DATA)
#define JpuReadInstReg( INST_IDX, ADDR )		    jdi_read_register_ext(pJpgInst->coreIndex, ((unsigned long)INST_IDX*NPT_REG_SIZE)+ADDR ) // system register write 	with instance index

#define JpuWriteReg( ADDR, DATA )                   jdi_write_register_ext(pJpgInst->coreIndex, ADDR, DATA ) // system register write
#define JpuReadReg( ADDR )                          jdi_read_register_ext(pJpgInst->coreIndex, ADDR )           // system register write

#define JpuWriteInstRegExt(CORE, INST_IDX, ADDR, DATA )		jdi_write_register_ext(CORE, ((unsigned long)INST_IDX*NPT_REG_SIZE)+ADDR, DATA ) // system register write 	with instance index
#define JpuReadInstRegExt(CORE, INST_IDX, ADDR )		    jdi_read_register_ext(CORE, ((unsigned long)INST_IDX*NPT_REG_SIZE)+ADDR ) // system register write 	with instance index

#define JpuWriteRegExt(CORE, ADDR, DATA )                   jdi_write_register_ext(CORE, ADDR, DATA ) // system register write
#define JpuReadRegExt(CORE, ADDR )                          jdi_read_register_ext(CORE, ADDR )           // system register write

#define JpuWriteMem( ADDR, DATA, LEN, ENDIAN )      jdi_write_memory( ADDR, DATA, LEN, ENDIAN ) // system memory write
#define JpuReadMem( ADDR, DATA, LEN, ENDIAN )       jdi_read_memory( ADDR, DATA, LEN, ENDIAN ) // system memory write

typedef Int32 HANDLE;

typedef struct jpu_buffer_t {
    //unsigned int  size;
    size_t  size;
    unsigned long phys_addr;
    unsigned long base;
    unsigned long virt_addr;
    unsigned int  is_cached;
} jpu_buffer_t;

typedef struct jpu_instance_pool_t {
    unsigned char   jpgInstPool[MAX_JPEG_NUM_INSTANCE][MAX_INST_HANDLE_SIZE];
    Int32           jpu_instance_num;
    BOOL            instance_pool_inited;
    void*           instPendingInst[MAX_JPEG_NUM_INSTANCE];
    jpeg_mm_t       vmem;
} jpu_instance_pool_t;

typedef enum {
    JDI_LITTLE_ENDIAN = 0,
    JDI_BIG_ENDIAN,
    JDI_32BIT_LITTLE_ENDIAN,
    JDI_32BIT_BIG_ENDIAN,
} EndianMode;


typedef enum {
    JDI_LOG_CMD_PICRUN  = 0,
    JDI_LOG_CMD_INIT  = 1,
    JDI_LOG_CMD_RESET  = 2,
    JDI_LOG_CMD_PAUSE_INST_CTRL = 3,
    JDI_LOG_CMD_MAX
} jdi_log_cmd;


#if defined (__cplusplus)
extern "C" {
#endif
    int jdi_probe(void);
    /* @brief It returns the number of task using JDI.
     */
    int jdi_get_task_num(void);
    int jdi_init(void);
    int jdi_release(void);    //this function may be called only at system off.
    jpu_instance_pool_t *jdi_get_instance_pool(void);
    int jdi_allocate_dma_memory(jpu_buffer_t *vb);
    int jdi_invalidate_cache(jpu_buffer_t *vb);
    int jdi_flush_cache(jpu_buffer_t *vb);
    void jdi_free_dma_memory(jpu_buffer_t *vb);
    int jdi_wait_interrupt(unsigned int core_idx, int timeout, unsigned long instIdx);
    int jdi_hw_reset(void);
    int jdi_wait_inst_ctrl_busy(int core_idx, int timeout, unsigned int addr_flag_reg, unsigned int flag);
    int jdi_set_clock_gate(int enable);
    int jdi_get_clock_gate(void);
    int jdi_open_instance(unsigned long instIdx);
    int jdi_close_instance(unsigned long instIdx);
    int jdi_get_instance_num(void);
    int jdi_core_request_one_core(int timeout);
    void jdi_core_hash_table_clear_core(int coreidx);
    int jdi_core_hash_table_set_core(pid_t pid, int coreidx);
    int jdi_core_hash_table_get_core(pid_t pid);
    unsigned long jdi_get_extension_address(int coreidx);
    void jdi_set_extension_address(int coreidx, unsigned long addr);
    void jdi_sw_top_reset(int coreidx);

    void jdi_write_register(unsigned long addr, unsigned int data);
    unsigned long jdi_read_register(unsigned long addr);
    void jdi_write_register_ext(int core_idx, unsigned long addr, unsigned int data);
    unsigned long jdi_read_register_ext(int core_idx, unsigned long addr);

    size_t jdi_write_memory(unsigned long addr, unsigned char *data, size_t len, int endian);
    size_t jdi_read_memory(unsigned long addr, unsigned char *data, size_t len, int endian);

    int jdi_lock(void);
    void jdi_unlock(void);
    void jdi_log(int cmd, int step, int inst);
    void jdi_delay_us(unsigned int us);
#if defined(CNM_FPGA_PLATFORM)
    int jdi_set_clock_freg(int Device, int OutFreqMHz, int InFreqMHz);
#else
#define ACLK_MAX                    300
#define ACLK_MIN                    16
#define CCLK_MAX                    300
#define CCLK_MIN                    16
#endif

#if defined (__cplusplus)
}
#endif

#endif //#ifndef _JDI_HPI_H_
