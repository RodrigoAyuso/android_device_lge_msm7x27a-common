# Camera2 patches prepared with ART and Bluetooth

The common DT carries all four Camera2 application patches. They are prepared
through the same explicit Bash/Git workflow as the ART and Bluetooth patches:

~~~sh
source build/envsetup.sh
msm7x27a_apply_patches --check
msm7x27a_brunch vee3
~~~

Use v1 for V1. The brunch wrapper runs apply-patches.sh before calling the normal
brunch function. A failed source preflight stops before brunch is entered.
The coordinator preflights ART, Bluetooth and the available Camera2 project
before changing their working sources.

There is no Camera2 execution from BoardConfigCommon.mk or product discovery.
The obsolete camera2.mk hook is removed. Preparation uses Bash, Git, awk, standard
Linux file tools and flock; it does not invoke Python. The LOS 14.1 Python 2
environment is unchanged.

The old apply-camera2-patch.py and apply-userspace-patches.py entry points remain
as Python 2.7/3-compatible wrappers around the shell helper. They do not import
pathlib or use subprocess.run/capture_output/text. Existing --root, --check and
--component camera2/all calls are accepted.

For Camera2-only preparation from the source root:

~~~sh
bash device/lge/msm7x27a-common/patches/apply-camera2-patches.sh --check
bash device/lge/msm7x27a-common/patches/apply-camera2-patches.sh
~~~

The helper preflights the complete app series in a temporary working directory,
preserves the app index/history and unrelated edits, and handles matching helper
classes left behind after a reset or removed by a source clean. A conflicting
custom file is not overwritten. Repeated preparation recognizes applied patches.

Camera2 remains optional if its project is absent. If selected for the ROM,
keep the standard packages/apps/Camera2 cm-14.1 project in the manifest. The
coordinator does not download missing source during a build.

The first three application payloads are unchanged. The virtual-front video
preview retains its TextureView rotation correction and same-profile refresh.
The recorder retains the physical rear sensor's rotation convention. A fourth
patch requests pixel reflection in a separate HAL encoder pool. Photo JPEG
reflection and its EXIF handling remain as before.

After a full build:

~~~sh
python device/lge/msm7x27a-common/patches/verify-camera2.py --root "$PWD" --device vee3
~~~

The verifier supports Python 2.7 and 3 without extra modules. Add --phone after
installing the new ROM/system image to compare APK/OAT hashes. File checks do
not replace testing video preview, recording orientation, camera switching,
tap focus and the already working photo behavior on the handset.


## Recorded virtual-front video reflection

The fourth patch requests pixel reflection through the device-specific camera
HAL before Camera2 unlocks the camera for MediaRecorder. This applies only to
the known V1/Vee3 rear-sensor alias. A separate encoder pool protects the working
preview from changes. The reflection axis accounts for recording orientation;
the review thumbnail is not mirrored a second time.

Rebuild and install the camera HAL and Camera2 together. Replacing only the APK
does not implement video reflection on the old HAL. The helper logs an explicit
message if the matching HAL capability is absent.
