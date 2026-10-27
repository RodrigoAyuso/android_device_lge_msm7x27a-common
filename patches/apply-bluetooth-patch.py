#!/usr/bin/env python3
"""Explicitly apply the carried LG Bluetooth startup patch to working source."""
import argparse
import os
from pathlib import Path
import subprocess
import sys


def apply(repo, data, *options):
    return subprocess.run(['git', '-C', str(repo), 'apply', '-p4', *options],
                          input=data, text=True, capture_output=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path,
                        default=Path(os.environ.get('ANDROID_BUILD_TOP', Path.home() / 'Android/LineageOS')))
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    repo = args.root.expanduser().resolve() / 'packages/apps/Bluetooth'
    if not (repo / '.git').exists():
        raise RuntimeError('Missing Git checkout: ' + str(repo))
    target = repo / 'src/com/android/bluetooth/btservice/AdapterState.java'
    for path in [target, *target.parents]:
        if path == repo:
            break
        if path.is_symlink():
            raise RuntimeError('Refusing a symlink in the patched path: ' + str(path))
    patch = Path(__file__).resolve().parent / 'apps_Bluetooth/0001-lg-profile-startup-timeout.patch'
    data = patch.read_text()
    result = apply(repo, data, '--check')
    if result.returncode:
        reverse = apply(repo, data, '--reverse', '--check')
        if reverse.returncode:
            raise RuntimeError('Bluetooth source conflicts with the carried patch:\n'
                               + result.stderr + reverse.stderr)
        print('Bluetooth: already applied; source, index and history were not changed.')
        return
    if args.check:
        print('Preflight passed. Working source, index and history were not changed.')
        return
    result = apply(repo, data)
    if result.returncode:
        raise RuntimeError(result.stderr)
    print('Bluetooth: working source patched; index and history preserved.')


if __name__ == '__main__':
    try:
        main()
    except (OSError, RuntimeError) as error:
        sys.exit(str(error))
