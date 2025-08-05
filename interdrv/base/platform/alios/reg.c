#include "mmio.h"


u32 _reg_read(uintptr_t addr)
{
	return mmio_read_32(addr);
}

void _reg_write(uintptr_t addr, u32 data)
{
	mmio_write_32(addr, data);
}

void _reg_write_mask(uintptr_t addr, u32 mask, u32 data)
{
	mmio_clrsetbits_32(addr, mask, data);
}

