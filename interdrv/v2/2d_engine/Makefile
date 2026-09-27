include $(PWD)/../Makefile.interdrv.param

TDE_DEFCONFIG := $(notdir $(patsubst %/,%,$(KERNEL_DIR)))

ifneq ($(KERNELRELEASE),)
$(CVIARCH_L)_2d_engine-y += $(TDE_DEFCONFIG)/vg_lite_hal.o
$(CVIARCH_L)_2d_engine-y += $(TDE_DEFCONFIG)/vg_lite_kernel.o
$(CVIARCH_L)_2d_engine-y += $(TDE_DEFCONFIG)/vg_lite_platform_bm1688.o
$(TDE_DEFCONFIG)/%.o: $(TDE_DEFCONFIG)/%.c
	@echo "Skipping compilation of $< (using prebuilt .o)"
obj-m += $(CVIARCH_L)_2d_engine.o

KBUILD_EXTRA_SYMBOLS = $(PWD)/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../sys/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../base/Module.symvers
else
all:
	@touch $(PWD)/Module.symvers
	@touch $(PWD)/$(TDE_DEFCONFIG)/.vg_lite_hal.o.cmd $(PWD)/$(TDE_DEFCONFIG)/.vg_lite_kernel.o.cmd $(PWD)/$(TDE_DEFCONFIG)/.vg_lite_platform_bm1688.o.cmd
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) V=$(V) TDE_DEFCONFIG=$(TDE_DEFCONFIG) modules

clean:
	@rm -f *.ko *.mod* .*.cmd .*.o modules.order Module.symvers
	@rm -f soph_2d_engine.o
	@find . -mindepth 2 -name '.*.cmd' -type f -delete
	@rm -rf .tmp_versions
endif
