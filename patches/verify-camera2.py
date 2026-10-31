#!/usr/bin/env python
"""Verify Camera2 source, built code and optional phone hashes on Python 2.7/3."""
from __future__ import print_function
import argparse
import hashlib
import os
import re
import subprocess
import sys
import threading
import zipfile

BadZipError = getattr(zipfile, 'BadZipFile', None) or zipfile.BadZipfile
SERIES = ('0001-fix-lg-virtual-front-camera.patch',
          '0002-fix-capture-module-virtual-front.patch',
          '0003-fix-virtual-front-video-preview.patch')
MARKERS = {
    'virtual-front JPEG reflection': b'Mirroring failed; retaining the original capture',
    'CaptureModule preview correction': b'LG virtual front: rotate preview by 180 degrees',
    'CaptureSession JPEG reflection': b'Mirrored virtual-front JPEG ',
    'video TextureView correction': b'LG virtual front: rotate video preview by 180 degrees',
    'physical-sensor recording orientation': b'LG virtual front: use rear sensor video orientation',
}


def read_bytes(path):
    with open(path, 'rb') as source:
        return source.read()


def call(argv):
    process = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    expired = threading.Event()

    def stop():
        if process.poll() is None:
            expired.set()
            process.kill()

    timer = threading.Timer(45, stop)
    timer.daemon = True
    timer.start()
    try:
        out, err = process.communicate()
    finally:
        timer.cancel()
    if expired.is_set():
        raise RuntimeError('Command timed out: ' + argv[0])
    return process.returncode, out, err


def run(argv):
    code, out, err = call(argv)
    if code:
        raise RuntimeError('Command failed: ' + ' '.join(argv[:3]) + '\n' +
                           err.decode('utf-8', 'replace').strip())
    return out


def verify_source(root, bundle):
    repo = os.path.join(root, 'packages/apps/Camera2')
    if not os.path.exists(os.path.join(repo, '.git')):
        raise RuntimeError('Missing Camera2 Git checkout: ' + repo)
    for name in SERIES:
        code, out, err = call(['git', '-C', repo, 'apply', '-p4', '--reverse', '--check',
                              os.path.join(bundle, 'apps_Camera2', name)])
        if code:
            raise RuntimeError('Camera2 source is missing or conflicts with ' + name +
                               '\nRun apply-patches.sh --check before rebuilding.\n' +
                               err.decode('utf-8', 'replace'))
    print('PASS: all three Camera2 source patches are present, including video.')


def built_files(root, device):
    system = os.path.join(root, 'out/target/product', device, 'system')
    apks = [os.path.join(system, kind, 'Camera2/Camera2.apk') for kind in ('app', 'priv-app')]
    apks = [p for p in apks if os.path.isfile(p)]
    if len(apks) != 1:
        raise RuntimeError('Expected one built Camera2 APK; found ' + str(len(apks)) +
                           '. Rebuild the selected product and check obsolete outputs.')
    apk = apks[0]
    companions = []
    for directory, dirs, files in os.walk(os.path.dirname(apk)):
        for name in files:
            if os.path.splitext(name)[1] in ('.odex', '.oat', '.art'):
                companions.append(os.path.join(directory, name))
    return system, apk, sorted(companions)


def inspect_code(apk, companions):
    groups = {'APK DEX': [], 'OAT/ODEX': []}
    try:
        with zipfile.ZipFile(apk) as archive:
            for info in archive.infolist():
                if re.match(r'classes\d*\.dex\Z', info.filename):
                    if info.file_size > 256 * 2**20:
                        raise RuntimeError('Unexpectedly large Camera2 DEX entry.')
                    data = archive.read(info)
                    if not data.startswith(b'dex\n'):
                        raise RuntimeError('Camera2 APK contains an invalid DEX header.')
                    groups['APK DEX'].append(data)
    except BadZipError:
        raise RuntimeError('Invalid built Camera2 APK: ' + apk)
    for path in companions:
        if os.path.splitext(path)[1] not in ('.odex', '.oat'):
            continue
        if os.path.getsize(path) > 256 * 2**20:
            raise RuntimeError('Unexpectedly large Camera2 OAT file: ' + path)
        data = read_bytes(path)
        if data.startswith((b'\x7fELF', b'oat\n')):
            groups['OAT/ODEX'].append(data)
    if not any(groups.values()):
        raise RuntimeError('No readable DEX/OAT code found in the built Camera2 files.')
    for group, streams in groups.items():
        if not streams:
            continue
        missing = [label for label, marker in MARKERS.items()
                   if not any(marker in data for data in streams)]
        if missing:
            raise RuntimeError('Built Camera2 ' + group + ' is missing: ' + ', '.join(missing) +
                               '. Rebuild Camera2 and the ROM before installing.')
    print('PASS: built DEX/OAT contains the photo, video-preview and recording-orientation markers.')


def verify_phone(adb, system, apk, companions):
    completed = run(adb + ['shell', 'getprop sys.boot_completed']).decode('utf-8', 'replace').strip()
    if completed != '1':
        raise RuntimeError('Boot Android normally before comparing installed Camera2 files.')
    output = run(adb + ['shell', 'pm path com.android.camera2']).decode('utf-8', 'replace')
    paths = [line[len('package:'):].strip() for line in output.splitlines() if line.startswith('package:')]
    expected = '/system/' + os.path.relpath(apk, system)
    if paths != [expected]:
        raise RuntimeError('Android is loading Camera2 from ' + repr(paths) +
                           '; expected ' + expected + '. A package update may override the system app.')
    for path in [apk] + companions:
        remote = '/system/' + os.path.relpath(path, system)
        if not re.match(r'/system/[A-Za-z0-9_./-]+\Z', remote):
            raise RuntimeError('Unsupported installed file path.')
        actual = run(adb + ['exec-out', 'cat', remote])
        if hashlib.sha256(actual).digest() != hashlib.sha256(read_bytes(path)).digest():
            raise RuntimeError('Installed Camera2 differs from this build: ' + remote +
                               '. Install the newly built system image or ROM package.')
    print('PASS: installed Camera2 APK/OAT files match this build.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', default=os.path.expanduser('~/Android/LineageOS'))
    parser.add_argument('--device', choices=('v1', 'vee3'), default='vee3')
    parser.add_argument('--source-only', action='store_true')
    parser.add_argument('--phone', action='store_true')
    parser.add_argument('--adb', default='adb')
    parser.add_argument('--serial')
    args = parser.parse_args()
    if args.source_only and args.phone:
        parser.error('--source-only cannot be combined with --phone')
    root = os.path.realpath(os.path.expanduser(args.root))
    verify_source(root, os.path.dirname(os.path.realpath(__file__)))
    if args.source_only:
        return
    system, apk, companions = built_files(root, args.device)
    inspect_code(apk, companions)
    if args.phone:
        adb = [args.adb] + (['-s', args.serial] if args.serial else [])
        verify_phone(adb, system, apk, companions)
    print('File checks passed. Confirm preview, photos, video orientation and focus on the phone.')


if __name__ == '__main__':
    try:
        main()
    except (IOError, OSError, RuntimeError) as error:
        sys.stderr.write(str(error) + '\n')
        sys.exit(1)
