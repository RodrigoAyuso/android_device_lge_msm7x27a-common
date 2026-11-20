# The CAD backend and Oreo bridge are shared by the V1 and Vee3.
ifneq ($(filter lineage_v1 lineage_vee3,$(TARGET_PRODUCT)),)
PRODUCT_PACKAGES += audio.primary.lge_msm7x27a
PRODUCT_PROPERTY_OVERRIDES += ro.hardware.audio.primary=lge_msm7x27a
endif
