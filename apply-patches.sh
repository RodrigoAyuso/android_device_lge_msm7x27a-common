#!/usr/bin/env bash
# Apply the LineageOS 14.1 Bluetooth compatibility patch after repo sync.
# This modifies only the system/bt working tree, not its upstream Git history.
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
top="$(cd "$script_dir/../../.." && pwd)"
bt_dir="$top/system/bt"
patch_file="$script_dir/patches/system_bt/0001-bt-warn-if-controller-misses-read-encryption-key-size.patch"

if [[ ! -f "$bt_dir/device/src/controller.c" ]]; then
    echo "ERROR: system/bt/device/src/controller.c was not found." >&2
    echo "Run repo sync to fetch system/bt (cm-14.1 branch)." >&2
    exit 1
fi

if git -C "$bt_dir" apply --check "$patch_file" >/dev/null 2>&1; then
    git -C "$bt_dir" apply "$patch_file"
    echo "msm7x27a-common: Bluetooth compatibility patch applied."
elif git -C "$bt_dir" apply --reverse --check "$patch_file" >/dev/null 2>&1; then
    echo "msm7x27a-common: Bluetooth compatibility patch is already applied; no changes needed."
else
    echo "ERROR: Bluetooth compatibility patch does not match the current system/bt source." >&2
    echo "Check the branch, local modifications, and device/src/controller.c." >&2
    echo "No files were modified during this run." >&2
    exit 1
fi
