# Shared legacy RIL v7 and SEEK ABI for V1 and Vee3.
ifneq ($(filter v1 vee3,$(TARGET_DEVICE)),)
ifeq ($(TARGET_BUILD_VARIANT),userdebug)
ifndef LGE_COMMON_RIL_COMPAT_INCLUDED
LGE_COMMON_RIL_COMPAT_INCLUDED := true
LOCAL_PATH := device/lge/msm7x27a-common/radio-compat
include $(CLEAR_VARS)
LOCAL_MODULE := libril_lge_msm7x27a_compat
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := 32
LOCAL_SRC_FILES := ril_compat.cpp
LOCAL_C_INCLUDES := hardware/ril/include
LOCAL_CFLAGS := -DRIL_SHLIB
LOCAL_CLANG := true
LOCAL_SHARED_LIBRARIES := libdl liblog libcutils
include $(BUILD_SHARED_LIBRARY)
endif
endif
endif
