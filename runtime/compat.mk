# Shared non-graphics runtime compatibility for the LG Android 8.1 port.
# Keep these boot/HAL settings separate from graphics backend policy.
ifeq ($(TARGET_BUILD_VARIANT),userdebug)
PRODUCT_PACKAGES += \
    power.default \
    audio.primary.default \
    android.hardware.power@1.0-impl \
    android.hardware.power@1.0-service \
    android.hardware.keymaster@3.0-impl \
    android.hardware.keymaster@3.0-service \
    android.hardware.audio@2.0-impl \
    android.hardware.audio.effect@2.0-impl \
    android.hardware.audio@2.0-service \
    android.hardware.light@2.0-impl \
    android.hardware.light@2.0-service

# Required by the existing Gralloc/EGL compatibility code and legacy ADB.
# This does not control the native init boot logger.
PRODUCT_PROPERTY_OVERRIDES += debug.lge.firstboot=1

# Retire the temporary first-boot filter on Vee3. Oreo's Zygote defaults to speed.
# Keep the existing bring-up policy for the other devices in this common tree.
ifneq ($(filter cm_vee3 lineage_vee3,$(TARGET_PRODUCT)),)
PRODUCT_PROPERTY_OVERRIDES += \
    dalvik.vm.systemservercompilerfilter=speed \
    ro.lge.performance.profile=vee3-oreo-stage1
else
PRODUCT_PROPERTY_OVERRIDES += dalvik.vm.systemservercompilerfilter=verify
endif

PRODUCT_COPY_FILES += \
    device/lge/msm7x27a-common/rootdir/system/etc/init/init.lge.legacy-usb.rc:system/etc/init/init.lge.firstboot-debug.rc
endif
