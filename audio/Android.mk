# Oreo bridge for the existing LG CAD HAL. Do not import the CM14 policy HAL.
ifndef LGE_CAD_AUDIO_OREO_INCLUDED
LGE_CAD_AUDIO_OREO_INCLUDED := true
ifneq ($(filter v1 vee3,$(TARGET_DEVICE)),)
ifeq ($(TARGET_BUILD_VARIANT),userdebug)
LOCAL_PATH := device/lge/msm7x27a-common/audio
include $(CLEAR_VARS)
LOCAL_MODULE := audio.primary.lge_msm7x27a
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := 32
LOCAL_SRC_FILES := audio_hw_hal.cpp AudioHardware_cad.cpp HardwarePinSwitching.c
LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libmedia libdl
LOCAL_STATIC_LIBRARIES := libmedia_helper libaudiohw_legacy
LOCAL_HEADER_LIBRARIES := libhardware_headers
LOCAL_C_INCLUDES := hardware/libhardware_legacy/include \
    frameworks/base/include system/core/include \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
LOCAL_CFLAGS := -D_POSIX_SOURCE -fno-short-enums
ifeq ($(strip $(NEW_INPUT_AUDIO_HAL)),true)
LOCAL_CFLAGS += -DNEW_INPUT_AUDIO_HAL
endif
ifeq ($(strip $(AUDIO_FEATURE_ENABLED_FM)),true)
LOCAL_CFLAGS += -DQCOM_FM_ENABLED
endif
ifneq ($(strip $(QCOM_PROXY_DEVICE_ENABLED)),false)
LOCAL_CFLAGS += -DQCOM_PROXY_DEVICE_ENABLED
endif
ifeq ($(strip $(BOARD_USES_SRS_TRUEMEDIA)),true)
LOCAL_CFLAGS += -DSRS_PROCESSING
endif
# Oreo rejects GCC for this module; use the platform Clang toolchain.
LOCAL_CLANG := true
include $(BUILD_SHARED_LIBRARY)

endif
endif
endif
