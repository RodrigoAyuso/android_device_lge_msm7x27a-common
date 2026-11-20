# Upstream Oreo Snap replaces Camera2 through LOCAL_OVERRIDES_PACKAGES.
PRODUCT_PACKAGES += Snap camera.msm7x27a

# The HAL1 sources and provider bridge are enabled on the second generation.
ifneq ($(filter lineage_v1 lineage_vee3,$(TARGET_PRODUCT)),)
ifeq ($(TARGET_BUILD_VARIANT),userdebug)
PRODUCT_PACKAGES += \
    libmmcamera_interface2 \
    liblge_camera_hdr_effect \
    android.hardware.camera.provider@2.4-impl \
    android.hardware.camera.provider@2.4-service
endif
endif
