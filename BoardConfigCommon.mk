# Copyright (C) 2013-2016 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# LG MSM7x27a shared Android 8.1 board configuration.
# Device-specific partitions and peripherals belong to each device BoardConfig.
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_VARIANT := generic
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_BOARD_PLATFORM := msm7x27a
TARGET_BOOTLOADER_BOARD_NAME := 7x27
TARGET_NO_BOOTLOADER := true
TARGET_KERNEL_SOURCE := kernel/lge/msm7x27a-common
TARGET_KERNEL_CROSS_COMPILE_PREFIX := arm-linux-androideabi-
KERNEL_TOOLCHAIN := $(ANDROID_BUILD_TOP)/prebuilts/gcc/$(HOST_OS)-x86/arm/arm-linux-androideabi-4.9/bin
BOARD_KERNEL_BASE := 0x00200000
BOARD_KERNEL_PAGESIZE := 4096
BOARD_MKBOOTIMG_ARGS := --ramdisk_offset 0x1200000
BOARD_FLASH_BLOCK_SIZE := 131072
TARGET_USERIMAGES_USE_EXT4 := true
# Default Oreo gzip ramdisk; do not inherit the CM custom LZ4 recipes.
KERNEL_CONFIG_OVERRIDE := CONFIG_RD_GZIP=y
BOARD_KERNEL_IMAGE_NAME := zImage
TARGET_USES_64_BIT_BINDER := true
TARGET_USES_HWC2 := false
TARGET_USES_GRALLOC1 := false
TARGET_USES_QCOM_BSP := false
TARGET_USE_PAN_DISPLAY := true
TARGET_PROVIDES_LIBLIGHT := true
USE_OPENGL_RENDERER := true
MALLOC_SVELTE := true
BOARD_USES_LEGACY_MMAP := true
TARGET_NEEDS_PLATFORM_TEXT_RELOCATIONS := true
WITH_DEXPREOPT := false
DEVICE_MANIFEST_FILE := device/lge/msm7x27a-common/manifest.xml
# Keep upstream neverallow checks; SELinux permissive is userdebug-only.
BOARD_SEPOLICY_DIRS += device/lge/msm7x27a-common/sepolicy
ifneq ($(TARGET_BUILD_VARIANT),userdebug)
$(error The current Oreo port supports userdebug only)
endif

