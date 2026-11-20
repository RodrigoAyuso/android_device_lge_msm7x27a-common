# Shared boot policy for the Cortex-A5 V1 and Vee3 products.
ifneq ($(filter lineage_v1 lineage_vee3,$(TARGET_PRODUCT)),)
PRODUCT_COPY_FILES += \
    device/lge/msm7x27a-common/boot/init.lge.bootanimation-early.rc:system/etc/init/init.lge.bootanimation-early.rc

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    ro.lge.usb.functionfs=1 \
    ro.lge.boot.graphics_early=1 \
    ro.lge.boot.fsck_flags=-y

PRODUCT_DEXPREOPT_SPEED_APPS += SystemUI Settings Trebuchet
PRODUCT_ART_USE_READ_BARRIER := false
PRODUCT_PROPERTY_OVERRIDES += \
    ro.lge.art.profile=msm7x27a-gen2-cms-v1 \
    ro.lge.boot.profile=msm7x27a-gen2-host-preopt
endif
