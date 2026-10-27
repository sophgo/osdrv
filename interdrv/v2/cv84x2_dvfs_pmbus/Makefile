include $(PWD)/../Makefile.interdrv.param

$(CVIARCH_L)_cv84x2_dvfs_pmbus-objs = $(CVIARCH_L)/cv84x2_dvfs_pmbus.o

obj-m += $(CVIARCH_L)_cv84x2_dvfs_pmbus.o

subdir-ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c *.mod
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
