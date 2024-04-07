arch_cvitek_chip := $(patsubst "%",%,$(CHIP_ARCH_L))

HAL_PATH = hal/$(arch_cvitek_chip)

ccflags-y += -I$(PWD) -I$(LDDINCDIR) -DCNM_FPGA_PLATFORM -DCNM_FPGA_PCI_INTERFACE -Wno-date-time

PWD	:= $(shell pwd)
obj-m += $(arch_cvitek_chip)_vcodec.o
$(arch_cvitek_chip)_vcodec-y += cvi_vcodec.o vcodec_common.o
$(arch_cvitek_chip)_vcodec-y += hal/$(arch_cvitek_chip)/cfg_$(arch_cvitek_chip).o

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -rf .tmp_versions Module.symvers
	rm -rf modules.order
	rm -rf $(HAL_PATH)/*.o

install:
	install -d $(INSTALL_DIR)
	install -d $(INSTALL_DIR)/drv
	install -m 755 load_vcodec.sh $(INSTALL_DIR)/drv
	install -m 755 unload_vcodec.sh $(INSTALL_DIR)/drv
	install -m 644 vcodec.ko $(INSTALL_DIR)/drv

uninstall:
	$(RM) $(INSTALL_DIR)/drv/vcodec.ko
	$(RM) $(INSTALL_DIR)/drv/load_vcodec.sh
	$(RM) $(INSTALL_DIR)/drv/unload_vcodec.sh

depend .depend dep:
	$(CC) $(CFLAGS)	-M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include	.depend
endif
