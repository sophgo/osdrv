/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: ldc_proc.h
 * Description:
 */

#ifndef _LDC_PROC_H_
#define _LDC_PROC_H_

#ifdef __cplusplus
	extern "C" {
#endif
int get_gdc_proc(char *outbuf);
int ldc_proc_init(void *shm);
int ldc_proc_remove(void);

#ifdef __cplusplus
}
#endif

#endif /* _LDC_PROC_H_ */

