# Shared Android 8.1 modules for the legacy LG MSM7x27a family.
LOCAL_PATH := $(call my-dir)
ifneq ($(filter e610 p700 v1 vee3,$(TARGET_DEVICE)),)
include device/lge/msm7x27a-common/fallback/Android.mk
include device/lge/msm7x27a-common/graphics/Android.mk
include device/lge/msm7x27a-common/audio/Android.mk
ifeq ($(TARGET_DEVICE):$(TARGET_BUILD_VARIANT),vee3:userdebug)
include device/lge/msm7x27a-common/camera/Android.mk
include device/lge/msm7x27a-common/libbt-vendor/Android.mk
include device/lge/vee3/connectivity/Android.mk
endif
endif
