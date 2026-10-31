#!/usr/bin/env bash
# Prepare the DT-carried Camera2 patches with the same Bash/Git tools as ART/BT.
set -euo pipefail

check_only=false
build_marker=false
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
common_dir="$(cd "$script_dir/.." && pwd)"
top="$(cd "$common_dir/../../.." && pwd)"
while (( $# )); do
    case "$1" in
        --check) check_only=true; shift ;;
        --build) build_marker=true; shift ;;
        --root)
            (( $# >= 2 )) || { echo "ERROR: --root requires a source directory." >&2; exit 2; }
            top="$(cd -- "$2" && pwd)"; shift 2 ;;
        --component)
            (( $# >= 2 )) || { echo "ERROR: --component requires a value." >&2; exit 2; }
            [[ "$2" == camera2 || "$2" == all ]] || { echo "ERROR: unsupported component: $2" >&2; exit 2; }
            shift 2 ;;
        --help|-h)
            echo "Usage: bash patches/apply-camera2-patches.sh [--root SOURCE_ROOT] [--check]"
            exit 0 ;;
        *) echo "ERROR: unexpected argument: $1" >&2; exit 2 ;;
    esac
done
repo="$top/packages/apps/Camera2"
if [[ ! -e "$repo/.git" ]]; then
    echo "ERROR: missing Camera2 Git checkout: $repo" >&2
    exit 1
fi
git_dir="$(git -C "$repo" rev-parse --git-dir)"
if [[ "$git_dir" != /* ]]; then git_dir="$repo/$git_dir"; fi
# Lock an existing directory rather than creating a tracked/untracked lock file.
exec {camera_lock}< "$git_dir"
flock -x "$camera_lock"
if [[ -n "$(git -C "$repo" ls-files --unmerged)" ]]; then
    echo "ERROR: finish the Camera2 merge before applying the device patches." >&2
    exit 1
fi

temporary="$(mktemp -d "${TMPDIR:-/tmp}/lge-camera2-plan.XXXXXX")"
trap 'rm -rf -- "$temporary"' EXIT
work="$temporary/worktree"
mkdir -p "$work"
pending="$temporary/pending.patch"
: > "$pending"
patches=(
    "$script_dir/apps_Camera2/0001-fix-lg-virtual-front-camera.patch"
    "$script_dir/apps_Camera2/0002-fix-capture-module-virtual-front.patch"
    "$script_dir/apps_Camera2/0003-fix-virtual-front-video-preview.patch"
)
sections=()
declare -A originals

check_path() {
    local relative="$1" current
    if [[ ! "$relative" =~ ^[A-Za-z0-9_./-]+$ || "$relative" == /* || "$relative" == *".."* ]]; then
        echo "ERROR: unsafe Camera2 patch path: $relative" >&2
        exit 1
    fi
    current="$repo/$relative"
    while [[ "$current" != "$repo" ]]; do
        if [[ -L "$current" ]]; then
            echo "ERROR: refusing a symlink in Camera2 patch path: $current" >&2
            exit 1
        fi
        current="$(dirname -- "$current")"
    done
    if [[ -e "$repo/$relative" && ! -f "$repo/$relative" ]]; then
        echo "ERROR: Camera2 patch target is not a regular file: $relative" >&2
        exit 1
    fi
}

fingerprint() {
    local relative="$1"
    check_path "$relative"
    if [[ ! -e "$repo/$relative" ]]; then
        echo missing
    else
        printf '%s:%s\n' "$(stat -c '%a' -- "$repo/$relative")" \
            "$(git -C "$repo" hash-object --no-filters -- "$repo/$relative")"
    fi
}

# Split by file so a reset with matching helper classes left behind is repairable.
number=0
shopt -s nullglob
for patch in "${patches[@]}"; do
    [[ -f "$patch" ]] || { echo "ERROR: missing carried patch: $patch" >&2; exit 1; }
    directory="$temporary/sections-$number"
    mkdir "$directory"
    awk -v directory="$directory" '
        /^diff --git / { if (output != "") close(output); n++; output=sprintf("%s/%04d.patch",directory,n) }
        output != "" { print > output }
        END { if (output != "") close(output) }
    ' "$patch"
    files=("$directory"/*.patch)
    if (( ${#files[@]} == 0 )); then
        echo "ERROR: no supported file patches in $patch" >&2
        exit 1
    fi
    sections+=("${files[@]}")
    number=$((number + 1))
done

for section in "${sections[@]}"; do
    read -r first second before after extra < "$section"
    if [[ "$first" != diff || "$second" != --git || -n "$extra" \
          || "$before" != a/packages/apps/Camera2/* \
          || "$after" != b/packages/apps/Camera2/* \
          || "${before#a/}" != "${after#b/}" ]]; then
        echo "ERROR: unexpected Camera2 diff header in $section" >&2
        exit 1
    fi
    relative="${after#b/packages/apps/Camera2/}"
    check_path "$relative"
    if [[ -z "${originals[$relative]+present}" ]]; then
        originals["$relative"]="$(fingerprint "$relative")"
        if [[ -f "$repo/$relative" ]]; then
            mkdir -p "$(dirname -- "$work/$relative")"
            cp -p -- "$repo/$relative" "$work/$relative"
        fi
    fi
done

for section in "${sections[@]}"; do
    if git -C "$work" apply -p4 --check "$section" >/dev/null 2>&1; then
        git -C "$work" apply -p4 "$section"
        cat "$section" >> "$pending"
    elif git -C "$work" apply -p4 --reverse --check "$section" >/dev/null 2>&1; then
        :
    else
        echo "ERROR: Camera2 conflicts with the carried patch series." >&2
        git -C "$work" apply -p4 --check "$section" >&2 || true
        echo "Preflight failed; Camera2 working files, index and history were not changed." >&2
        exit 1
    fi
done
if "$check_only"; then
    echo "Camera2 preflight passed; working files, index and history were not changed."
    exit 0
fi
for relative in "${!originals[@]}"; do
    if [[ "$(fingerprint "$relative")" != "${originals[$relative]}" ]]; then
        echo "ERROR: Camera2 file changed during preflight: $relative" >&2
        exit 1
    fi
done
if [[ -s "$pending" ]]; then
    git -C "$repo" apply -p4 --check "$pending"
    git -C "$repo" apply -p4 "$pending"
    echo "Camera2: device patches applied; index and commit history preserved."
else
    echo "Camera2: all three device patches are already applied."
fi
if "$build_marker"; then echo LGE_CAMERA2_READY; fi
