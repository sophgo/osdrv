ccflags-y += -I$(PWD)/platform/linux/
ccflags-y += -I$(PWD)/../vo/chip/$(CHIP_ARCH_L)
ccflags-y += -I$(PWD)/../vo/driver

$(CHIP_ARCH_L)_mipi_tx-objs += platform/linux/mipi_tx.o \
			       platform/linux/proc/mipi_tx_proc.o
