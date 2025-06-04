SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=$(MW_PATH)/Makefile.param
	include $(PARAM_FILE)
endif

SDIR := $(CURDIR)
SRCS := $(wildcard $(SDIR)/*.c)
SRCS += $(SDIR)/common/ipcm_test_common_cust.c
SRCS += $(SDIR)/common/ipcm_test_common.c

# ipcm ut
SRCS := $(filter-out $(SDIR)/alios_cli.c, $(SRCS))
# INCS = -I$(MW_INC) -I$(IPCM_INC) -Icommon
INCS = -I$(MW_INC) -I../include -Icommon
$(warning CURDIR:$(CURDIR))
$(warning INCS:$(INCS))
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

TARGET = ipcm_ut
LIBS =

# alios cli
SRCS_C := $(filter-out $(SDIR)/test.c, $(SRCS))
SRCS_C += $(SDIR)/alios_cli.c
OBJS_C = $(SRCS_C:.c=.o)
DEPS_C = $(SRCS_C:.c=.d)

TARGET_C = alios_cli

EXTRA_CFLAGS += $(DEFS) $(INCS)
EXTRA_CFLAGS += -ffunction-sections -fdata-sections

EXTRA_LDFLAGS += $(LIBS)
EXTRA_LDFLAGS += $(MW_LIB)/libipcm.a -lpthread

ifneq ($(CONFIG_ENABLE_SDK_ASAN), y)
ELFFLAGS += -static
endif
.PHONY : all clean
all : $(TARGET) $(TARGET_C)

$(COMMON_DIR)/%.o: $(COMMON_DIR)/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(SDIR)/%.o: $(SDIR)/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET) : $(OBJS)
	@$(CXX) -o $@ -Wl,--start-group $(OBJS) -Wl,--end-group $(ELFFLAGS) $(EXTRA_LDFLAGS)
	@echo -e $(BLUE)[LINK]$(END)[$(notdir $(CC))] $(notdir $@)
	@cp $(TARGET) $(MW_PATH)/self_test/auto_test/

$(TARGET_C) : $(OBJS_C)
	@$(CXX) -o $@ -Wl,--start-group $(OBJS_C) -Wl,--end-group $(ELFFLAGS) $(EXTRA_LDFLAGS)
	@echo -e $(BLUE)[LINK]$(END)[$(notdir $(CC))] $(notdir $@)
	@$(STRIP) $(TARGET_C)
	@cp $(TARGET_C) $(MW_PATH)/self_test/auto_test/

clean :
	@rm -f $(OBJS) $(DEPS) $(TARGET) $(OBJS_C) $(DEPS_C) $(TARGET_C)

-include $(DEPS)
-include $(DEPS_C)
