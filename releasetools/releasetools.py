# SPDX-License-Identifier: Apache-2.0
# Python 2.7/3 compatible: this hook runs in the Android build environment.
"""LG OTA assertions compatible with legacy TWRP property readers."""
import re
RECOVERY_KEYS = ("ro.build.product", "ro.product.device", "ro.cm.device")
MACHINE_NAMES = {"v1": ("MSM7225A", "v1"), "vee3": ("MSM7225A", "v3")}


def _quote(value):
    return '"' + value.replace('\\', '\\\\').replace('"', '\\"') + '"'


def _boot_hardware_check(device):
    # TWRP for V1 was sometimes built from the Vee3 recovery product. Use
    # an exact kernel argument, not that other product's identity. Conflicting
    # hardware arguments are rejected. Only the recovery shell is executed.
    command = ('set -f; read -r line < /proc/cmdline || exit 1; found=0; '
               'for arg in $line; do case "$arg" in androidboot.hardware=*) '
               '[ "$arg" = "androidboot.hardware=%s" ] || exit 1; found=1;; '
               'esac; done; [ "$found" = 1 ]') % device
    return 'run_program("/sbin/sh", "-c", %s) == 0' % _quote(command)


def _legacy_recovery_assertions(info):
    product = info.info_dict.get("build.prop", {}).get("ro.product.device")
    if product not in MACHINE_NAMES:
        raise ValueError("LG releasetools: unknown product %r" % product)
    device = product
    needle = 'abort("E3004:'
    matches = [(i, value) for i, value in enumerate(info.script.script)
               if value.lstrip().startswith("assert(") and needle in value]
    if len(matches) != 1:
        raise ValueError("LG releasetools: expected exactly one E3004 device assertion")
    index, original = matches[0]
    if 'getprop("ro.product.device") == %s' % _quote(device) not in original:
        raise ValueError("LG releasetools: original assertion omits " + device)
    # Keep retail aliases owned by each BoardConfig's TARGET_OTA_ASSERT_DEVICE.
    # The generated assertion is the authoritative list; never add other models.
    aliases = sorted(set(re.findall(r'getprop\("ro.product.device"\) == "([A-Za-z0-9_.-]+)"', original)))
    checks = ['file_getprop("/default.prop", %s) == %s' % (_quote(key), _quote(alias))
              for key in RECOVERY_KEYS for alias in aliases]
    checks += ['getprop("ro.boot.hardware") == %s' % _quote(device),
               _boot_hardware_check(device), _kernel_machine_check(device)]
    if all(check in original for check in checks):
        return
    if any(check in original for check in checks):
        raise ValueError("LG releasetools: partial recovery fallback needs review")
    offset = original.index(needle)
    info.script.script[index] = original[:offset] + " || ".join(checks) + " || " + original[offset:]


def _kernel_machine_check(device):
    # The V1 recovery may carry Vee3 product properties and command line.
    # The board-specific kernel machine name remains distinct in cpuinfo.
    chip, board = MACHINE_NAMES[device]
    command = ('set -f; while IFS=: read -r key value; do set -- $key; '
               'if [ "$#" = 1 ] && [ "$1" = Hardware ]; then set -- $value; '
               '[ "$#" = 2 ] && [ "$1" = "%s" ] && [ "$2" = "%s" ] && exit 0; '
               'fi; done < /proc/cpuinfo; exit 1') % (chip, board)
    return 'run_program("/sbin/sh", "-c", %s) == 0' % _quote(command)


def FullOTA_Assertions(info):
    _legacy_recovery_assertions(info)


def IncrementalOTA_Assertions(info):
    _legacy_recovery_assertions(info)


def VerifyOTA_Assertions(info):
    _legacy_recovery_assertions(info)
