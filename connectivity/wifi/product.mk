# Shared Volans Wi-Fi for V1 and Vee3.
ifneq ($(filter lineage_v1 lineage_vee3,$(TARGET_PRODUCT)),)
ifeq ($(TARGET_BUILD_VARIANT),userdebug)
PRODUCT_PACKAGES += wpa_supplicant wificond lge-wlan-loader
PRODUCT_PROPERTY_OVERRIDES += wifi.interface=wlan0
PRODUCT_COPY_FILES += \
    device/lge/msm7x27a-common/connectivity/wifi/init.lge.wifi.rc:system/etc/init/init.lge.wifi.rc \
    device/lge/msm7x27a-common/connectivity/wifi/wpa_supplicant.conf:vendor/etc/wifi/wpa_supplicant.conf
endif
endif
