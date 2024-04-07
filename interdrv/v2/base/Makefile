include $(PWD)/../Makefile.interdrv.param

$(CVIARCH_L)_base-objs = base.o \
				base_common.o \
				bind.o \
				callback.o \
				ion.o \
				vb.o \
				vbq.o \
				proc/cvi_log_proc.o \
				proc/cvi_sys_proc.o \
				proc/cvi_vb_proc.o



obj-m += $(CVIARCH_L)_base.o

ccflags-y += -I$(srctree)/drivers/tee \
             -I$(PWD)/../include/common/uapi/ \
             -I$(PWD)/../include/common/kapi/ \
             -I$(PWD)/../base/ \
             -I$(PWD)/../include/chip/$(CVIARCH_L)/uapi/ \
             -I$(PWD)/../sys/common/ \
             -I$(PWD)/../sys/common/uapi \
             -I$(srctree)/drivers/staging/android \
             -I$(PWD)/proc \


ifeq ($(SUBTYPE), fpga)
ccflags-y += -DDRV_FPGA_PORTING
endif

ccflags-y += $(INTRERDRV_FLAGS)

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare


all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
