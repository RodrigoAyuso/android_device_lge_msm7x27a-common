# Shared legacy RIL v7 and SEEK ABI for V1 and Vee3.
ifneq ($(filter lineage_v1 lineage_vee3,$(TARGET_PRODUCT)),)
ifeq ($(TARGET_BUILD_VARIANT),userdebug)
PRODUCT_PACKAGES += libril_lge_msm7x27a_compat rild
PRODUCT_PROPERTY_OVERRIDES += rild.libpath=/system/lib/libril_lge_msm7x27a_compat.so
PRODUCT_COPY_FILES += \
    device/lge/msm7x27a-common/radio-compat/init.lge.ril-compat.rc:system/etc/init/init.lge.ril-compat.rc
endif
endif

# Root ramdisk service access for V1 and Vee3.
ifneq ($(filter lineage_v1 lineage_vee3,$(TARGET_PRODUCT)),)
PRODUCT_COPY_FILES += \
    device/lge/msm7x27a-common/rootdir/root/init.lge.qualcomm-access.rc:root/init.lge.qualcomm-access.$(patsubst lineage_%,%,$(TARGET_PRODUCT)).rc
endif
