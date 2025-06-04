include $(PWD)/../Makefile.interdrv.param

ifeq ($(KERNEL_DIR),)
KERNEL_DIR=/media/cvitek/allen.huang/code/dual_os_fastboot/linux_5.10/build/cv1801c_wevb_0009a_spinor
endif

obj-m := cvi_ipcm.o
obj-m += cvi_ipcm_test.o

cvi_ipcm-y := driver/src/ipcm_common.o \
	driver/src/ipcm_pool.o \
	driver/src/ipcm.o \
	driver/src/msg_queue.o \
	driver/src/mailbox.o \
	driver/src/cvi_spinlock.o \
	driver/src/ipcm_dbg_rec_info.o \
	driver/src/ipcm_port.o \
	driver/src/ipcm_message.o \
	driver/src/ipcm_custom.o

cvi_ipcm-y += platform/linux/src/ipcm_dev.o \
	platform/linux/src/ipcm_dev_port.o \
	platform/linux/src/ipcm_plat_adapter.o \
	platform/linux/src/ipcm_custom_linux.o \
	platform/linux/src/ipcm_system.o

cvi_ipcm-y += platform/linux/src/ipcmsg.o \
	platform/linux/src/ipcmsg_dev.o

# ipcm message test (kernel space)
 ifneq ($(findstring DRV_TEST, $(INTRERDRV_FLAGS)),)
cvi_ipcm-y += driver/test/common/ipcm_test_common.o \
	driver/test/common/ipcm_test_common_msg.o \
 	platform/linux/test/test_ker.o
 endif

# ipcm info record
 ifneq ($(findstring IPCM_INFO_REC, $(INTRERDRV_FLAGS)),)
 cvi_ipcm-y += driver/src/ring.o
 endif

ccflags-y += -I$(PWD)/driver/include -I$(PWD)/driver/include/linux -I$(PWD)/driver/src
ccflags-y += -I$(PWD)/platform/linux/include
ccflags-y += -I$(PWD)/driver/test/common
ccflags-y += -I$(PWD)/../include/common/uapi/
ccflags-y += -D__LINUX_DRV__

ccflags-y += $(INTRERDRV_FLAGS)

ccflags-y += -Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

cvi_ipcm_test-y := driver/src/ipcm_common.o \
	platform/linux/test/cust_kernel_test.o

# IPCM_OUT_DIR = $(PWD)/../build/ipcm
IPCM_OUT_DIR = $(PWD)/ipcm_out

all: $(IPCM_OUT_DIR)/Makefile
	@cp -f ${BUILD_PATH}/output/${PROJECT_FULLNAME}/cvi_board_memmap.h $(PWD)/driver/include/
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(IPCM_OUT_DIR) src=$(PWD) modules
	@cp $(IPCM_OUT_DIR)/*.ko $(PWD)/

$(IPCM_OUT_DIR):
	mkdir -p "$@"

$(IPCM_OUT_DIR)/Makefile: $(IPCM_OUT_DIR)
	touch "$@"

clean:
#	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(IPCM_OUT_DIR) src=$(PWD) clean
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c *.mod
	@rm -rf .tmp_versions
	@rm -rf $(IPCM_OUT_DIR)
	@find . -name \*.o* -type f -delete
