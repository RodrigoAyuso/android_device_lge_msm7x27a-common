# Shared Oreo camera selection for V1 and Vee3.
ifndef LGE_OREO_CAMERA_MODULES_INCLUDED
LGE_OREO_CAMERA_MODULES_INCLUDED := true
ifneq ($(filter v1 vee3,$(TARGET_DEVICE)),)
ifeq ($(TARGET_BUILD_VARIANT),userdebug)
LOCAL_PATH := device/lge/msm7x27a-common/camera

include $(CLEAR_VARS)
LOCAL_MODULE := libmmcamera_interface2
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := 32
LOCAL_CLANG := true
LOCAL_SRC_FILES := \
    mm-camera-interface/mm_camera_interface2.c \
    mm-camera-interface/mm_camera_stream.c \
    mm-camera-interface/mm_camera_channel.c \
    mm-camera-interface/mm_camera.c \
    mm-camera-interface/mm_camera_poll_thread.c \
    mm-camera-interface/mm_camera_notify.c \
    mm-camera-interface/mm_camera_sock.c \
    mm-camera-interface/mm_camera_helper.c \
    mm-camera-interface/mm_omx_jpeg_encoder.c
LOCAL_CFLAGS := -D_ANDROID_ -DLGE_CAMERA_WDN_HDR_START_BEFORE_ERROR
LOCAL_CFLAGS += -include sys/socket.h -include netinet/in.h -include sys/un.h
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/mm-camera-interface \
    hardware/qcom/media-caf/msm7x27a/mm-core/inc \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
LOCAL_SHARED_LIBRARIES := libdl libcutils liblog
include $(BUILD_SHARED_LIBRARY)

# Small CPU tone mapper for still captures; no extra ION pool or JPEG pass.
include $(CLEAR_VARS)
LOCAL_MODULE := liblge_camera_hdr_effect
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := 32
LOCAL_CLANG := true
LOCAL_SRC_FILES := hdr-effect/HdrEffect.cpp
LOCAL_CFLAGS := -Wall -Wextra -Werror
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := camera.msm7x27a
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := 32
LOCAL_CLANG := true
LOCAL_SRC_FILES := \
    QCameraHAL.cpp QCameraHWI_Parm.cpp QCameraHWI.cpp \
    QCameraHWI_Preview.cpp QCameraHWI_Record_7x27A.cpp \
    QCameraHWI_Still.cpp QCameraHWI_Mem.cpp QCameraHWI_Display.cpp \
    QCameraStream.cpp QualcommCamera2.cpp QCameraHWI_Rdi.cpp QCameraParameters.cpp
LOCAL_CFLAGS := \
    -D_ANDROID_ -DUSE_ION -DHW_ENCODE -DDLOPEN_LIBMMCAMERA=1 \
    -DNUM_PREVIEW_BUFFERS=4 -DLGE_CAMERA_WDN_HDR_START_BEFORE_ERROR \
    -DLGE_CAMERA_DISABLE_ZSL \
    -DLGE_CAMERA_OREO_FRAME_COMPAT \
    -DLGE_CAMERA_NATIVE_HDR_EFFECT \
    -DCAMERA_ION_HEAP_ID=ION_CAMERA_HEAP_ID \
    -DCAMERA_ZSL_ION_HEAP_ID=ION_CAMERA_HEAP_ID \
    -DCAMERA_GRALLOC_HEAP_ID=GRALLOC_USAGE_PRIVATE_CAMERA_HEAP \
    -DCAMERA_GRALLOC_FALLBACK_HEAP_ID=GRALLOC_USAGE_PRIVATE_SYSTEM_HEAP \
    -DCAMERA_GRALLOC_CACHING_ID=GRALLOC_USAGE_PRIVATE_UNCACHED \
    -DCAMERA_ION_FALLBACK_HEAP_ID=ION_CAMERA_HEAP_ID \
    -DCAMERA_ZSL_ION_FALLBACK_HEAP_ID=ION_CAMERA_HEAP_ID
LOCAL_CFLAGS += -include sys/socket.h
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/mm-camera-interface \
    hardware/qcom/display-caf/msm7x27a/libgralloc \
    hardware/qcom/media-caf/msm7x27a/libstagefrighthw \
    hardware/qcom/media-caf/msm7x27a/mm-core/inc \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_HEADER_LIBRARIES := libhardware_headers media_plugin_headers
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
LOCAL_SHARED_LIBRARIES := \
    libcamera_client libcutils liblog libui libutils libhardware \
    libbinder libdl libnativewindow libmmcamera_interface2 liblge_camera_hdr_effect
include $(BUILD_SHARED_LIBRARY)
endif
endif
endif
