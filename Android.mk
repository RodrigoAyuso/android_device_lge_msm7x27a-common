# Shared Android 8.1 modules for the legacy LG MSM7x27a family.
LOCAL_PATH := $(call my-dir)
ifneq ($(filter e610 p700 v1 vee3,$(TARGET_DEVICE)),)
include device/lge/msm7x27a-common/fallback/Android.mk
include device/lge/msm7x27a-common/graphics/Android.mk
include device/lge/msm7x27a-common/audio/Android.mk
ifeq ($(TARGET_DEVICE):$(TARGET_BUILD_VARIANT),vee3:userdebug)
include device/lge/vee3/connectivity/Android.mk
endif
endif

# Shared Bluetooth transport for V1 and Vee3.
ifneq ($(filter v1 vee3,$(TARGET_DEVICE)),)
ifeq ($(TARGET_BUILD_VARIANT),userdebug)
include device/lge/msm7x27a-common/libbt-vendor/Android.mk
endif
endif

# Shared Oreo camera selection for V1 and Vee3.
ifneq ($(filter v1 vee3,$(TARGET_DEVICE)),)
ifeq ($(TARGET_BUILD_VARIANT),userdebug)
include device/lge/msm7x27a-common/camera/Android.mk
endif
endif

# Shared Volans Wi-Fi for V1 and Vee3.
include device/lge/msm7x27a-common/connectivity/wifi/Android.mk

# Shared legacy RIL v7 and SEEK ABI for V1 and Vee3.
include device/lge/msm7x27a-common/radio-compat/Android.mk
