#!/usr/bin/env bash
# Apply the device-carried Bluetooth and ART patches after repo sync.
# Only the target working files are changed; indexes and histories are preserved.
set -euo pipefail

check_only=false
case "${1:-}" in
    --check) check_only=true; shift ;;
    --help|-h)
        echo "Usage: bash device/lge/msm7x27a-common/apply-patches.sh [--check]"
        echo "Check both device patches before applying either working-tree change."
        exit 0
        ;;
esac
if (( $# != 0 )); then
    echo "ERROR: unexpected argument: $1" >&2
    exit 2
fi

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
top="$(cd "$script_dir/../../.." && pwd)"
projects=("system/bt" "art")
targets=("device/src/controller.c" "compiler/image_writer.cc")
patches=(
    "patches/system_bt/0001-bt-warn-if-controller-misses-read-encryption-key-size.patch"
    "patches/art/0001-fix-reflected-method-image-address.patch"
)
states=()

# Preflight every repository before changing any working file.
for i in "${!projects[@]}"; do
    repo="$top/${projects[$i]}"
    patch_file="$script_dir/${patches[$i]}"
    target="$repo/${targets[$i]}"
    if [[ ! -e "$repo/.git" || ! -f "$target" || ! -f "$patch_file" ]]; then
        echo "ERROR: missing checkout, source file, or device patch for ${projects[$i]}." >&2
        echo "Check repo sync and the contents of device/lge/msm7x27a-common/patches." >&2
        exit 1
    fi
    if [[ -n "$(git -C "$repo" ls-files --unmerged)" ]]; then
        echo "ERROR: resolve the unmerged index entries in ${projects[$i]} first." >&2
        exit 1
    fi
    if git -C "$repo" apply --check "$patch_file" >/dev/null 2>&1; then
        states[$i]=pending
        echo "msm7x27a-common: ${projects[$i]}: patch pending."
    elif git -C "$repo" apply --reverse --check "$patch_file" >/dev/null 2>&1; then
        states[$i]=applied
        echo "msm7x27a-common: ${projects[$i]}: patch already applied."
    else
        echo "ERROR: device patch conflicts with ${projects[$i]}/${targets[$i]}." >&2
        git -C "$repo" apply --check "$patch_file" >&2 || true
        echo "Preflight failed; no working files, indexes, or histories were changed." >&2
        exit 1
    fi
done

if "$check_only"; then
    echo "Preflight passed; no working files, indexes, or histories were changed."
    exit 0
fi

for i in "${!projects[@]}"; do
    [[ "${states[$i]}" == pending ]] || continue
    git -C "$top/${projects[$i]}" apply "$script_dir/${patches[$i]}"
    echo "msm7x27a-common: ${projects[$i]}: device patch applied."
done
echo "Device patches are ready. Target indexes and commit histories were preserved."
