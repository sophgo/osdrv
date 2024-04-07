TOOL_PATH := $(shell pwd)/../../../../../..
CROSS_COMPILE_32 := $(TOOL_PATH)/host-tools/gcc/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
CROSS_COMPILE_64 := $(TOOL_PATH)/host-tools/gcc/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-

SDK_VER := 64bit

ifeq ($(SDK_VER), 32bit)
  CROSS_COMPILE := $(CROSS_COMPILE_32)
else
  CROSS_COMPILE := $(CROSS_COMPILE_64)
endif
CC := $(CROSS_COMPILE)gcc

KERNEL_INC := $(shell pwd)/../../../../../build/$(PROJECT_FULLNAME)/usr/include/
INC := -I$(shell pwd)/common/

CFLAGS += -DDEBUG_LOG
ifeq ($(SDK_VER), 32bit)
  CFLAGS += -D_FILE_OFFSET_BITS=64
endif
CFLAGS += -pthread -static -I$(KERNEL_INC) $(INC)
CFLAGS += -g -O2 -Wall -Werror -Wno-unused

SRC := $(shell find . -name "*.c")
objects := $(patsubst %.c,%.o,$(SRC))

TARGET = DrvUT

$(TARGET): $(objects)
	$(CC) -o $@ $(objects) $(CFLAGS)

.PHONY : clean
clean:
	-rm $(TARGET) $(objects)
