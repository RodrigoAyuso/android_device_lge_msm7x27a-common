#!/system/bin/sh
# Enter the daemon domain before loading the legacy ELF.
exec /system/bin/lge-legacy/mm-qcamera-daemon "$@"
