/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: ive_interface.h
 * Description: ive driver interface header file
 */

#ifndef __IVE_CORE_RES_H__
#define __IVE_CORE_RES_H__


int ive_core_request_resource(int timeout);
int ive_core_release_resource(int id);
int ive_core_init_resources(unsigned int core_num);
void ive_core_cleanup_resources(void);

#endif /* __IVE_CORE_RES_H__ */