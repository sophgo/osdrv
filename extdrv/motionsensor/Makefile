
obj-m += motionsensor.o

motionsensor-objs := src/main/motionsensor.o
# motionsensor-objs += src/dev/mpu6500/mpu6500.o
motionsensor-objs += src/dev/icm42605/icm42605.o
motionsensor-objs += src/transfer/i2c/i2c_dev.o

ccflags-y += -I$(PWD)/inc -I$(PWD)/src/dev -I$(PWD)/src/main
ccflags-y += -I$(PWD)/src/transfer/i2c

all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod *.mod.c
	rm -rf .tmp_versions

