include $(PWD)/../Makefile.interdrv.param

TDE_USE_LINUX_COMMON ?= 0

ifneq ($(KERNELRELEASE),)
ifeq ($(TDE_USE_LINUX_COMMON),1)
$(CVIARCH_L)_2d_engine-y += linux-common/vg_lite_hal.o
$(CVIARCH_L)_2d_engine-y += linux-common/vg_lite_kernel.o
$(CVIARCH_L)_2d_engine-y += linux-common/vg_lite_platform_bm1688.o
linux-common/%.o: linux-common/%.c
	@echo "Skipping compilation of $< (using prebuilt .o in linux-common)"
else
$(CVIARCH_L)_2d_engine-y += vg_lite_hal.o
$(CVIARCH_L)_2d_engine-y += vg_lite_kernel.o
$(CVIARCH_L)_2d_engine-y += vg_lite_platform_bm1688.o
%.o: %.c
	@echo "Skipping compilation of $< (using prebuilt .o files)"
endif
obj-m += $(CVIARCH_L)_2d_engine.o

KBUILD_EXTRA_SYMBOLS = $(PWD)/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../sys/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../base/Module.symvers
else
TDE_USE_LINUX_COMMON := 0
ifneq ($(findstring linux-common,$(KERNEL_DIR)),)
TDE_USE_LINUX_COMMON := 1
endif

all:
	@if [ "$(TDE_USE_LINUX_COMMON)" = "1" ]; then \
		touch $(PWD)/Module.symvers; \
		touch $(PWD)/linux-common/.vg_lite_hal.o.cmd $(PWD)/linux-common/.vg_lite_kernel.o.cmd $(PWD)/linux-common/.vg_lite_platform_bm1688.o.cmd; \
	fi
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) V=$(V) TDE_USE_LINUX_COMMON=$(TDE_USE_LINUX_COMMON) modules

clean:
	@rm -f *.ko *.mod* .*.cmd modules.order Module.symvers
	@rm -f soph_2d_engine.o
	@rm -f linux-common/.*.cmd
	@rm -rf .tmp_versions
endif
