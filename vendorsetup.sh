# Loaded by build/envsetup.sh on LineageOS 14.1.
# Patch application is explicit: never modify system/bt automatically during envsetup.
function msm7x27a_apply_patches() {
    local top
    top="$(gettop)"
    if [ -z "$top" ]; then
        echo "ERROR: run this command inside an Android source tree." >&2
        return 1
    fi
    bash "$top/device/lge/msm7x27a-common/apply-patches.sh"
}

# Optional helper: apply the compatibility patch before running brunch.
function msm7x27a_brunch() {
    msm7x27a_apply_patches || return 1
    brunch "$@"
}
