LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := init.lge.shared.rc
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)
ifneq ($(filter v1 vee3,$(TARGET_DEVICE)),)
LOCAL_REQUIRED_MODULES := lge-start-mm-qcamera-daemon lge-start-netmgrd lge-start-qmuxd lge-start-rmt_storage lge-start-sensord lge-mm-qcamera-daemon lge-netmgrd lge-qmuxd lge-rmt_storage lge-sensord
endif
include $(BUILD_SYSTEM)/base_rules.mk
$(LOCAL_BUILT_MODULE): PRIVATE_LGE_DEVICE := $(TARGET_DEVICE)
$(LOCAL_BUILT_MODULE): PRIVATE_LGE_INIT_SOURCE := $(LOCAL_PATH)/../rootdir/root/init.lge.shared.rc
$(LOCAL_BUILT_MODULE): PRIVATE_LGE_INIT_FILTER := $(LOCAL_PATH)/filter-init.py
$(LOCAL_BUILT_MODULE): $(LOCAL_PATH)/../rootdir/root/init.lge.shared.rc $(LOCAL_PATH)/filter-init.py
	@mkdir -p $(dir $@)
	$(hide) python3 $(PRIVATE_LGE_INIT_FILTER) --device $(PRIVATE_LGE_DEVICE) $(PRIVATE_LGE_INIT_SOURCE) $@

ifneq ($(filter v1 vee3,$(TARGET_DEVICE)),)
define lge-legacy-entrypoint
include $$(CLEAR_VARS)
LOCAL_MODULE := lge-start-$(1)
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
LOCAL_STRIP_MODULE := false
LOCAL_SRC_FILES := entrypoints/$(1).sh
include $$(BUILD_PREBUILT)

include $$(CLEAR_VARS)
LOCAL_MODULE := lge-$(1)
LOCAL_MODULE_STEM := $(1)
LOCAL_MODULE_PATH := $$(TARGET_OUT_EXECUTABLES)/lge-legacy
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
LOCAL_STRIP_MODULE := false
LOCAL_PREBUILT_MODULE_FILE := vendor/lge/$(2)/proprietary/bin/$(1)
include $$(BUILD_PREBUILT)
endef
$(foreach daemon,mm-qcamera-daemon netmgrd qmuxd,$(eval $(call lge-legacy-entrypoint,$(daemon),msm7x27a-common)))
$(foreach daemon,rmt_storage sensord,$(eval $(call lge-legacy-entrypoint,$(daemon),$(TARGET_DEVICE))))
endif
