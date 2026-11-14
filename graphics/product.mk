# Shared LG MSM7x27a graphics policy. All clients use the same Adreno backend.
# Inherit this file from the shared runtime, rather than from each device.
ifeq ($(TARGET_BUILD_VARIANT),userdebug)
PRODUCT_PACKAGES += \
    libstlport \
    gralloc.lge_msm7x27a \
    android.hardware.graphics.mapper@2.0-impl \
    android.hardware.graphics.allocator@2.0-impl

# Full GPU redraws avoid the buffer-age/preserved-swap path on old EGL drivers.
# These options keep the OpenGL HWUI pipeline; they do not select CPU Canvas.
PRODUCT_PROPERTY_OVERRIDES += \
    ro.lge.graphics.profile=adreno \
    debug.lge.adreno=1 \
    debug.gralloc.map_fb_memory=1 \
    debug.hwui.renderer=opengl \
    debug.hwui.use_partial_updates=false \
    debug.hwui.use_buffer_age=false
endif
