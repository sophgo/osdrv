#ifndef _REG_H_
#define _REG_H_

#include "osal.h"

u32 _reg_read(uintptr_t addr);
void _reg_write(uintptr_t addr, u32 data);
void _reg_write_mask(uintptr_t addr, u32 mask, u32 data);

#endif //_REG_H_
