#!/usr/bin/env python3
"""Explicitly apply the optional Camera2 patch carried by this device tree."""
import argparse
import os
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--root', type=Path,
                    default=Path(os.environ.get('ANDROID_BUILD_TOP', Path.home() / 'Android/LineageOS')))
parser.add_argument('--check', action='store_true')
args = parser.parse_args()
repo = args.root.expanduser().resolve() / 'packages/apps/Camera2'
patch = Path(__file__).resolve().parent / 'apps_Camera2/0001-fix-lg-virtual-front-camera.patch'

def run(*options):
    return subprocess.run(['git', '-C', str(repo), 'apply', '-p4', *options, str(patch)],
                          capture_output=True, text=True)

if not repo.is_dir():
    sys.exit('Missing Camera2 checkout: ' + str(repo))
forward = run('--check')
if forward.returncode:
    reverse = run('--reverse', '--check')
    if reverse.returncode == 0:
        print('Camera2 patch is already applied; no changes made.')
        sys.exit(0)
    sys.exit('Camera2 source conflict; no changes made.\n' + forward.stderr + reverse.stderr)
if args.check:
    print('Camera2 preflight passed; working files, index, and history were not changed.')
else:
    result = run()
    if result.returncode:
        sys.exit(result.stderr)
    print('Camera2 working source patched. The index and commit history were not changed.')
