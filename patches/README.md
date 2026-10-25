# Device-carried LineageOS 14.1 patches

These patches are maintained in the common device tree and applied explicitly
to the target working trees after repo sync. They are not runtime init scripts.
Sourcing build/envsetup.sh does not apply patches or create commits.

## Patches

- system_bt/0001-bt-warn-if-controller-misses-read-encryption-key-size.patch:
  existing Bluetooth controller compatibility patch, unchanged.
- art/0001-fix-reflected-method-image-address.patch: correct reflected Method
  and Constructor native addresses in split ART boot images. It reuses
  ImageWriter::GetImageMethodAddress() so the relocation uses the owning image
  instead of the first image's base.

ART provenance: upstream commit
[96e640c1909d21081a2c342d160ed65ba536dae0](https://github.com/LineageOS/android_art/commit/96e640c1909d21081a2c342d160ed65ba536dae0),
"Fix the target art method address within a method object", bug 31311504,
original change 1717159b07abefcf59d099bc81158644f0a0bffb.
Only the diff paths were adapted to be relative to the art checkout.

## Apply before compilation

From the Android source root:

```sh
bash device/lge/msm7x27a-common/apply-patches.sh --check
bash device/lge/msm7x27a-common/apply-patches.sh
```

The script checks both patches before applying either. A patch already present
in the working tree or HEAD is accepted. Conflicts and missing checkouts stop
preflight. It preserves staged work and creates no commits in system/bt or art.
Do not edit the target sources concurrently with patch application.

After sourcing build/envsetup.sh, the existing helpers are also available:

```sh
msm7x27a_apply_patches --check
msm7x27a_brunch v1
```

Replace v1 with vee3 for the other device. The brunch wrapper applies both
patches before it calls brunch. Normal mka or brunch calls do not apply them;
run apply-patches.sh first when using those commands. Repeat the check after
repo sync. The script does not stash, reset, or discard target changes.

## DEXPREOPT validation

Rebuild the host dex2oat and dex2oatd tools and the complete ROM. Flashing only
boot.img cannot update the precompiled files under /system. For the first test,
clear only Dalvik/ART cache in recovery to avoid reusing the old relocated
image; preserve application data and internal storage.

This fixes a verified image-writer defect. Whether it resolves the specific
Wi-Fi-triggered native crash still requires testing on the phone. Keep the
normal TLS and Internet validation checks enabled.
