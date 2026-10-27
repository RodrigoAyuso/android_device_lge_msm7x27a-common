# LG virtual-front behavior in Camera2

These optional app patches are carried in the common device tree. Apply them
explicitly after syncing packages/apps/Camera2:

```sh
python3 device/lge/msm7x27a-common/patches/apply-userspace-patches.py --root "$PWD" --component camera2 --check
python3 device/lge/msm7x27a-common/patches/apply-userspace-patches.py --root "$PWD" --component camera2
```

The helper modifies working files only. It does not stage files, create app
commits or run from envsetup. The old apply-camera2-patch.py command forwards
to the same implementation. An existing first app patch is recognized and
only the missing second patch is applied.

The alias gate requires ro.hardware=v1/vee3, two cameras, rear ID 0, front
ID 1 and equal sensor mount angles. Other devices, the rear camera, the HAL
and OpenCamera retain their previous behavior.

Patch 1 handles the legacy PhotoModule and VideoModule previews and mirrors
PhotoModule JPEGs. Patch 2 covers the CaptureModule path used by the supplied
runtime logs: rotate its preview by 180 degrees, map tap focus through that
rotation, and wrap only a virtual-front photo's CaptureSession. Both the
legacy OneCamera implementation and the image-backend implementation save
through that session's saveAndFinish method.

The session wrapper reflects JPEG pixels in display coordinates, keeping
dimensions, orientation, EXIF/GPS and embedded thumbnails. It also reflects
the UI thumbnails and delegates the other session operations and the original
completion future. Processing uses the existing JPEG worker, one mutable
full-size bitmap and row buffers. It adds decode/re-encode work and another
JPEG compression pass, using the backend's high-quality camera profile.
On allocation/decode/encode failure the original photograph is retained and
an error is logged. Video recording behavior is unchanged.

Expected log tags: LgeFrontCapture for the final JPEG; CaptureModule for the
preview correction; VirtualFrontCamera for a processing failure. Verify
portrait, reverse portrait and both landscape orientations on the handset,
including text in the saved image and tap focus. Full APK compilation and
physical camera validation are still required in the Android build tree.
