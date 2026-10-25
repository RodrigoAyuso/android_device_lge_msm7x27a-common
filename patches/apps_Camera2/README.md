# Optional LG virtual-front behavior in Camera2

This patch belongs to the Camera2 app, not to the camera HAL. It is carried
inside the LG common tree and only changes app source when explicitly applied:

```sh
python3 device/lge/msm7x27a-common/patches/apply-camera2-patch.py --root "$PWD" --check
python3 device/lge/msm7x27a-common/patches/apply-camera2-patch.py --root "$PWD"
```

The helper changes working files in packages/apps/Camera2. It does not create
an app commit, stage changes, or run automatically from envsetup. The original
restriction on LOS-source changes remains respected unless this explicit
application is chosen. The common-tree commit only stores the patch.

The patch recognizes the rear-sensor alias on ro.hardware=v1/vee3, logical ID 1,
only when both exposed camera records share the same mount angle and report
rear/front facing respectively. The rear camera and OpenCamera are unchanged.

The legacy photo and video modules correct the alias's preview rotation while
retaining front-facing preview reflection. Saved JPEG photos are reflected in
display coordinates, including their embedded JPEG thumbnail. EXIF/GPS and
the selected JPEG compression quality are retained. This requires JPEG
decoding and recompression; it is performed in the background using one full
mutable bitmap and row buffers, rather than two full-size bitmaps.

On decoding, allocation, or encoding failure, the original capture is retained
and a VirtualFrontCamera error is logged. Video files retain their previous
recording behavior; this patch changes their preview orientation only.

After syncing the app repository, re-run the explicit helper before rebuilding.
No complete Android app build or on-device camera test was performed here.
