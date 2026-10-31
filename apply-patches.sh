#!/usr/bin/env bash
# Apply the device-carried Bluetooth, ART and Camera2 patches after repo sync.
# Only target working files are changed; indexes and histories are preserved.
set -euo pipefail

check_only=false
case "${1:-}" in
    --check) check_only=true; shift ;;
    --help|-h)
        echo "Usage: bash device/lge/msm7x27a-common/apply-patches.sh [--check]"
        echo "Preflight the Bluetooth, ART and available Camera2 patches before changing sources."
        exit 0 ;;
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

# Keep the existing ART/BT patch contents and preparation rules.
for i in "${!projects[@]}"; do
    repo="$top/${projects[$i]}"
    patch_file="$script_dir/${patches[$i]}"
    target="$repo/${targets[$i]}"
    if [[ ! -e "$repo/.git" || ! -f "$target" || ! -f "$patch_file" ]]; then
        echo "ERROR: missing checkout, source file, or device patch for ${projects[$i]}." >&2
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
        echo "Preflight failed; working files, indexes and histories were not changed." >&2
        exit 1
    fi
done

camera2=false
if [[ -e "$top/packages/apps/Camera2/.git" ]]; then
    camera2=true
    bash "$script_dir/patches/apply-camera2-patches.sh" --root "$top" --check
elif [[ -d "$top/packages/apps/Camera2" ]]; then
    echo "ERROR: packages/apps/Camera2 exists without its Git checkout; check repo sync." >&2
    exit 1
else
    echo "msm7x27a-common: Camera2 project absent; app patches skipped."
fi
if "$check_only"; then
    echo "Preflight passed; working files, indexes and histories were not changed."
    exit 0
fi

# Do not enter brunch until all selected source preparations succeed.
if "$camera2"; then bash "$script_dir/patches/apply-camera2-patches.sh" --root "$top"; fi
for i in "${!projects[@]}"; do
    [[ "${states[$i]}" == pending ]] || continue
    git -C "$top/${projects[$i]}" apply "$script_dir/${patches[$i]}"
    echo "msm7x27a-common: ${projects[$i]}: device patch applied."
done
echo "Device patches are ready. Target indexes and commit histories were preserved."
