#!/system/bin/sh
# Enter the daemon domain before loading the legacy ELF.
exec /system/libexec/lge/qmuxd "$@"
