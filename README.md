# LG MSM7x27a — LineageOS 15.1

Shared Android 8.1 sources for e610, p700, v1 and vee3. Vee3 is the developed
device; the other products retain their minimal, unvalidated bring-up profiles.
Only userdebug is supported. Permissive SELinux remains a development limitation.

BoardConfigCommon.mk is the sole shared board configuration. Each device keeps
its own partitions, kernel defconfig and peripheral settings in BoardConfig.mk.
runtime/compat.mk carries the common HIDL/legacy transport requirements;
graphics/product.mk selects Adreno/OpenGL HWUI for the Vee3 product path.
fallback/ and bringup/ keep the incomplete profiles of the other devices explicit.
camera/ and audio/ contain the active Clang HAL build recipes.

debug.lge.firstboot and debug.lge.adreno are compatibility gates consumed by the
existing modified frameworks/native code. Removing them here would break the
working graphics/USB path. They are not logging switches.

Diagnostic probes, old preparation scripts and bring-up notes live outside the
source repositories in the organizer output. Python used by releasetools or
by a real build rule is retained; it is not a diagnostic collector.

The current ROM also depends on existing changes in system/core,
frameworks/native, display/media/RIL HALs and external/stlport. This DT does not
patch those repositories during envsetup or brunch. Preserve the separately
recorded dependency versions when reproducing the build after repo sync.

The automatic /cache/lineage15-bootlogs writer is compiled into the current
system/core/init and has no opt-out. It cannot be removed by this DT-only cleanup.
The external report records the exact limitation and current dependency state.

For Vee3: source build/envsetup.sh; lunch lineage_vee3-userdebug; m bacon.
Compile inside the established Ubuntu 16.04 Distrobox. Use the organizer with
Python 3 on the host. Rebuild and install the complete ROM when changing ART
configuration or CPU variants; do not mix libart/OAT files from different builds.
