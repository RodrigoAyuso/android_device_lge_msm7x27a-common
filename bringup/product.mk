# Minimal userdebug profile for V1, E610 and P700; hardware bring-up is incomplete.
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)
$(call inherit-product, vendor/lineage/config/common_mini.mk)
$(call inherit-product, frameworks/native/build/phone-hdpi-512-dalvik-heap.mk)
PRODUCT_FULL_TREBLE := false
PRODUCT_PACKAGES += \
    gralloc.default \
    libEGL_swiftshader \
    libGLESv1_CM_swiftshader \
    libGLESv2_swiftshader \
    android.hardware.graphics.allocator@2.0-impl \
    android.hardware.graphics.allocator@2.0-service \
    android.hardware.graphics.mapper@2.0-impl \
    android.hardware.configstore@1.0-service \
    android.hidl.allocator@1.0-service \
    lights.msm7x27a \
    android.hardware.light@2.0-impl \
    android.hardware.light@2.0-service \
    power.msm7x27a \
    android.hardware.power@1.0-impl \
    android.hardware.power@1.0-service \
    audio.primary.default \
    audio.a2dp.default \
    audio.r_submix.default \
    audio.usb.default \
    android.hardware.audio@2.0-impl \
    android.hardware.audio@2.0-service \
    android.hardware.audio.effect@2.0-impl \
    android.hardware.keymaster@3.0-impl \
    android.hardware.keymaster@3.0-service \
    gatekeeperd
PRODUCT_PROPERTY_OVERRIDES += \
    ro.hardware.gralloc=default \
    ro.hardware.egl=swiftshader \
    ro.opengles.version=131072 \
    ro.config.low_ram=true \
    debug.sf.nobootanimation=0 \
    dalvik.vm.dex2oat-threads=1 \
    dalvik.vm.boot-dex2oat-threads=1 \
    persist.sys.usb.config=adb
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += ro.adb.secure=1
PRODUCT_COPY_FILES += \
    device/lge/msm7x27a-common/bringup/init.lge.usb.rc:root/init.lge.oreo.usb.rc
