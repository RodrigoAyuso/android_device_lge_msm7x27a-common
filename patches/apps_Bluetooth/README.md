# Optional LG profile startup allowance

The supplied log starts BR/EDR profiles at 18:40:38.498, reaches the
four-second timeout at 18:40:42.594, reports A2DP ready at 18:40:44.036 and
kills com.android.bluetooth at 18:40:44.086. The failed attempt runs out of
time while profile services are still initializing. A subsequent attempt
at 18:41:14 reaches ON at 18:41:15.237. Times are copied from the phone log.

This app-only patch raises the BR/EDR profile startup budget from 4000 to
15000 ms on ro.hardware=v1/vee3. Readiness still completes immediately and
cancels the timer. Other hardware, profiles, controller firmware timeout,
BLE startup timeout and shutdown/cleanup behavior retain their defaults.
Permanently stuck startup still fails at the bounded timeout.

The patch is carried in the common device tree and is never applied from
envsetup or the Camera2/FM helper. Apply it explicitly:

```sh
python3 device/lge/msm7x27a-common/patches/apply-bluetooth-patch.py --root "$PWD" --check
python3 device/lge/msm7x27a-common/patches/apply-bluetooth-patch.py --root "$PWD"
```

The helper patches working source in packages/apps/Bluetooth without staging
or committing it. Its --check is read-only, conflicts stop before application
and repeated application is recognized. Reapply after a repo sync that resets
the app source. Rebuild and install the Bluetooth app as part of the ROM.

This addresses the startup timeout demonstrated in this collection. It does
not establish that every intermittent controller or UART failure is resolved.
Retest cold startup, quick settings, Settings, BLE discovery, classic pairing
and repeated disable/enable cycles on the handset.
