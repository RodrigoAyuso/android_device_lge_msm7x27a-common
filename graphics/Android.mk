# Dedicated gralloc0 for the ARM32 LG first-boot profile.
# Compile only the allocator/mapper/fbdev sources; no old HWC/framework extensions.
ifndef LGE_ADRENO_MODULES_INCLUDED
LGE_ADRENO_MODULES_INCLUDED := true
lge_adreno_path := $(call my-dir)
lge_adreno_display := hardware/qcom/display-caf/msm7x27a

ifeq ($(TARGET_BUILD_VARIANT),userdebug)
ifeq ($(TARGET_BOARD_PLATFORM),msm7x27a)
LOCAL_PATH := $(lge_adreno_display)
include $(CLEAR_VARS)
LOCAL_MODULE := gralloc.lge_msm7x27a
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := libgralloc/gpu.cpp libgralloc/gralloc.cpp \
    libgralloc/framebuffer.cpp libgralloc/mapper.cpp \
    libgralloc/alloc_controller.cpp libgralloc/ionalloc.cpp \
    libqdutils/mdp_version.cpp libqdutils/comptype.cpp libqdutils/profiler.cpp
LOCAL_C_INCLUDES := $(lge_adreno_display)/libgralloc \
    $(lge_adreno_display)/libqdutils \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_SHARED_LIBRARIES := liblog libutils libcutils libhardware libdl libGLESv1_CM
LOCAL_REQUIRED_MODULES := libstlport
LOCAL_CFLAGS := -DNO_IOMMU -DSECURE_MM_HEAP -DUSE_MDP3 -DLOG_TAG=\"lge_adreno_gralloc\"
LOCAL_CFLAGS += -DLGE_OREO_FB_IMPORT
LOCAL_LDFLAGS := -Wl,-Bsymbolic
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
include $(BUILD_SHARED_LIBRARY)

endif
endif
LOCAL_PATH := $(lge_adreno_path)
endif # LGE_ADRENO_MODULES_INCLUDED
