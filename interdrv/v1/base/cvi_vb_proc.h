/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_vb_proc.h
 * Description: vb proc driver header file
 */

#ifndef _CVI_VB_PROC_H_
#define _CVI_VB_PROC_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <generated/compile.h>
#include "base.h"
#include "mw/mw_base.h"

int vb_proc_init(struct proc_dir_entry *_proc_dir, void *shared_mem);
int vb_proc_remove(struct proc_dir_entry *_proc_dir);

#endif

