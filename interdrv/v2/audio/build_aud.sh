#!/bin/bash

export KERNEL_OUTPUT_FOLDER=build/cv1826_wevb_0005a_spinand
export KERNEL_PATH=/home/vincentyu/cv18xx/linux
export SYSTEM_OUT_DIR=/home/vincentyu/cv18xx/install/soc_cv1826_wevb_0005a_spinand/rootfs/mnt/system
export KERNEL_DIR="$KERNEL_PATH"/"$KERNEL_OUTPUT_FOLDER"
export INSTALL_DIR="$SYSTEM_OUT_DIR"/ko
export ARCH=arm
export arch_cvitek_chip=cv182x

make clean
make
