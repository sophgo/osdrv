arch_cvitek_chip := $(patsubst "%",%,$(CHIP_ARCH_L))

HAL_PATH = hal/$(arch_cvitek_chip)

CHIP_TYPE = CHIP_BM1880
SUB_TYPE = ASIC_EVB

# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
  DEBFLAGS = -O	-g # "-O" is needed to expand inlines
else
  DEBFLAGS = -O2
endif

ccflags-y += $(DEBFLAGS) -I$(PWD) -I$(LDDINCDIR) -D$(CHIP_TYPE) -D$(SUB_TYPE) -Wno-date-time

PWD	:= $(shell pwd)
obj-m += $(arch_cvitek_chip)_jpeg.o
$(arch_cvitek_chip)_jpeg-y += cvi_jpeg.o jpeg_common.o jmm.o hal/$(arch_cvitek_chip)/$(arch_cvitek_chip)_jpeg_cfg.o

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -rf .tmp_versions Module.symvers
	rm -rf modules.order
	rm -rf $(HAL_PATH)/*.o

depend .depend dep:
	$(CC) $(CFLAGS)	-M *.c > .depend


ifeq (.depend,$(wildcard .depend))
include	.depend
endif
