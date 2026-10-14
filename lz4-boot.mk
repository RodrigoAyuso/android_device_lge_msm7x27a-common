# LZ4 legacy boot ramdisks for LG devices sharing msm7x27a-common.
# Included after the CM 14.1 ramdisk, boot and recovery rules are defined.

MSM7X27A_LZ4 ?= $(if $(strip $(LZ4)),$(LZ4),lz4c)
msm7x27a_lz4_tool := $(shell command -v $(MSM7X27A_LZ4) 2>/dev/null)
ifeq ($(strip $(msm7x27a_lz4_tool)),)
$(error Install host lz4c 1.9.4 or set MSM7X27A_LZ4 to its executable path)
endif

BOOT_RAMDISK_COMPRESSOR := $(msm7x27a_lz4_tool) -l -9 -c

$(INSTALLED_RAMDISK_TARGET): $(msm7x27a_lz4_tool) $(BOARD_CUSTOM_BOOTIMG_MK)

ifneq ($(strip $(INSTALLED_BOOTIMAGE_TARGET)),)
$(INSTALLED_BOOTIMAGE_TARGET): $(MKBOOTIMG) $(INTERNAL_BOOTIMAGE_FILES) $(BOOTIMAGE_EXTRA_DEPS)
	$(call pretty,"Target boot image: $@")
	$(hide) $(MKBOOTIMG) $(INTERNAL_BOOTIMAGE_ARGS) $(INTERNAL_MKBOOTIMG_VERSION_ARGS) $(BOARD_MKBOOTIMG_ARGS) --output $@
	$(hide) $(call assert-max-image-size,$@,$(BOARD_BOOTIMAGE_PARTITION_SIZE))

.PHONY: bootimage-nodeps
bootimage-nodeps: $(MKBOOTIMG)
	@echo "make $@: ignoring dependencies"
	$(hide) $(MKBOOTIMG) $(INTERNAL_BOOTIMAGE_ARGS) $(INTERNAL_MKBOOTIMG_VERSION_ARGS) $(BOARD_MKBOOTIMG_ARGS) --output $(INSTALLED_BOOTIMAGE_TARGET)
	$(hide) $(call assert-max-image-size,$(INSTALLED_BOOTIMAGE_TARGET),$(BOARD_BOOTIMAGE_PARTITION_SIZE))
endif

# BOARD_CUSTOM_BOOTIMG_MK suppresses the default recovery image target.
# Restore it when present, retaining the stock recovery compressor and recipe.
ifneq ($(strip $(INSTALLED_RECOVERYIMAGE_TARGET)),)
$(INSTALLED_RECOVERYIMAGE_TARGET): $(MKBOOTIMG) $(recovery_ramdisk) $(recovery_kernel) $(RECOVERYIMAGE_EXTRA_DEPS)
	@echo "----- Making recovery image ------"
	$(call build-recoveryimage-target, $@)
endif
