# Shared Volans Wi-Fi for V1 and Vee3.
ifneq ($(filter v1 vee3,$(TARGET_DEVICE)),)
ifeq ($(TARGET_BUILD_VARIANT),userdebug)
ifndef LGE_COMMON_WLAN_LOADER_INCLUDED
LGE_COMMON_WLAN_LOADER_INCLUDED := true
LOCAL_PATH := device/lge/msm7x27a-common/connectivity/wifi
include $(CLEAR_VARS)
LOCAL_MODULE := lge-wlan-loader
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := 32
LOCAL_CLANG := true
LOCAL_SRC_FILES := wlan_loader.c
LOCAL_SHARED_LIBRARIES := libcutils liblog
LOCAL_CFLAGS := -Wall -Wextra -Werror
include $(BUILD_EXECUTABLE)
endif
endif
endif
