########################################
# 2D module build flow
#
# Internal build (driver sources present):
# Step 1: Compile sources and link the module with Kbuild.
# Step 2: Save three basic objects to obj/<PROJECT_FULLNAME>.
#
# SDK build (driver sources absent):
# Step 1: Copy objects from obj/<PROJECT_FULLNAME> to the working directory.
# Step 2: Create temporary .cmd files and relink the module with Kbuild.
#
# Module clean: Remove build outputs, but preserve obj/.
########################################

tde_dir := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
include $(tde_dir)/../Makefile.interdrv.param

tde_objects := vg_lite_hal.o vg_lite_kernel.o vg_lite_platform_bm1688.o
tde_sources := $(tde_objects:.o=.c)
tde_present := $(notdir $(wildcard $(addprefix $(tde_dir)/,$(tde_sources))))
# Objects are tied to the board kernel .config (e.g. CONFIG_TRACING), so keep one set per board.
tde_object_dir := $(tde_dir)/obj/$(PROJECT_FULLNAME)

ifneq ($(KERNELRELEASE),)
$(CVIARCH_L)_2d_engine-y := $(tde_objects)
obj-m += $(CVIARCH_L)_2d_engine.o

ccflags-y += -I$(tde_dir)/chip/$(CVIARCH_L)/ -I$(tde_dir)/common/
ccflags-y += -I$(tde_dir)/../include/common/uapi/ -I$(tde_dir)/../include/chip/$(CVIARCH_L)/uapi/
ccflags-y += -I$(tde_dir)/../include/common/kapi/
ccflags-y += -I$(tde_dir)/../include/
ccflags-y += -I$(tde_dir)/../base/
ccflags-y += -I$(tde_dir)/../base/chip/$(CVIARCH_L)
ccflags-y += -I$(tde_dir)/../sys/common/ -I$(tde_dir)/../sys/common/uapi
ccflags-y += -I$(srctree)/drivers/tee
ccflags-y += -I$(srctree)/drivers/staging/android
ccflags-y += -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare
ifeq ($(CHIP), cv84x6)
ccflags-y += -DMEDIA_V3
endif

KBUILD_EXTRA_SYMBOLS += $(tde_dir)/../sys/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(tde_dir)/../base/Module.symvers
else

.PHONY: all clean
all:
ifeq ($(filter $(SIDE_TYPE),device edge),)
	@echo "ERROR: SIDE_TYPE must be device or edge" >&2; exit 1
else ifeq ($(strip $(PROJECT_FULLNAME)),)
	@echo "ERROR: PROJECT_FULLNAME is required for 2D objects" >&2; exit 1
else ifneq ($(words $(tde_present)),0)
ifneq ($(words $(tde_present)),3)
	@echo "ERROR: incomplete 2D sources: $(tde_present)" >&2; exit 1
else
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(tde_dir) V=$(V) modules
	@set -e; for object in $(tde_objects); do test -s "$(tde_dir)/$$object"; done
	@mkdir -p "$(tde_object_dir)"
	@cp -f $(addprefix $(tde_dir)/,$(tde_objects)) "$(tde_object_dir)/"
	@echo "2D objects: $(tde_object_dir)"
endif
else
	@set -e; for object in $(tde_objects); do \
		test -s "$(tde_object_dir)/$$object" || \
		{ echo "ERROR: missing 2D object: $(tde_object_dir)/$$object" >&2; exit 1; }; \
	done; \
	for object in $(tde_objects); do \
		cp -f "$(tde_object_dir)/$$object" "$(tde_dir)/$$object"; \
		printf '\n' > "$(tde_dir)/.$$object.cmd"; \
	done
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(tde_dir) V=$(V) modules
endif

clean:
	@find "$(tde_dir)" -path "$(tde_dir)/obj" -prune -o \
		-type f \( -name '*.o' -o -name '*.o.*' -o -name '*.ko' -o -name '*.mod' \
		-o -name '*.mod.c' -o -name '.*.cmd' -o -name 'Module.symvers' \
		-o -name 'modules.order' \) -exec rm -f {} +
	@rm -rf "$(tde_dir)/.tmp_versions"
endif
