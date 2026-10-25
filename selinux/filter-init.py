#!/usr/bin/env python3
"""Generate the selected board's ramdisk without losing local init tuning."""
import argparse
from pathlib import Path
import re

def render(source, device):
    if device not in ('v1', 'vee3'):
        return source
    services = {'qcamerasvr': 'mm-qcamera-daemon', 'netmgrd': 'netmgrd',
                'qmuxd': 'qmuxd', 'rmt_storage': 'rmt_storage', 'sensord': 'sensord'}
    for service, executable in services.items():
        pattern = r'(?m)^(service ' + re.escape(service) + r' )/system/bin/' + re.escape(executable) + r'(?=\s|$)'
        source, count = re.subn(pattern, r'\1/system/bin/lge-start-' + executable, source)
        if count != 1:
            raise RuntimeError('Expected exactly one original service: ' + service)
    source = source.replace('service p2p_supplicant /system/bin/logwrapper /system/bin/wpa_supplicant',
                            'service p2p_supplicant /system/bin/wpa_supplicant')
    pattern = r'(?m)^(\s*socket wpa_wlan0 dgram 660 wifi wifi)(?: u:r:wpa:s0)?$'
    source, count = re.subn(pattern, r'\1 u:r:wpa:s0', source)
    if count != 2:
        raise RuntimeError('Expected both Wi-Fi control socket declarations')
    anchor = '    mkdir /data/misc/sensor 2755 system compass'
    if source.count(anchor) != 1:
        raise RuntimeError('Expected the sensor data directory action')
    return source.replace(anchor, anchor + '\n    restorecon_recursive /data/misc/sensor'
                          + '\n    restorecon /data/fifo_cmd /data/sensord.err /mpt/prox_calibration.dat /data/cam_socket0'
                          + '\n    restorecon_recursive /cache')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--device', required=True)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    result = render(args.source.read_text(), args.device)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(result)
