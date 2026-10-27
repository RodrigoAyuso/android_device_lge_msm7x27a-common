#!/usr/bin/env python3
"""Explicitly apply the Camera2 source patches carried by this device tree."""
import argparse
import os
from pathlib import Path
import re
import stat
import subprocess
import sys
import tempfile


def command(repo, data, *options):
    return subprocess.run(['git', '-C', str(repo), 'apply', '-p4', *options],
                          input=data, text=True, capture_output=True)


def fingerprint(repo, name):
    path = repo / name
    for parent in [path, *path.parents]:
        if parent == repo:
            break
        if parent.is_symlink():
            raise RuntimeError('Refusing a symlink in patched path: ' + str(path))
    if not path.exists():
        return None
    if not path.is_file():
        raise RuntimeError('Expected a regular file: ' + str(path))
    return path.read_bytes(), stat.S_IMODE(path.stat().st_mode)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path,
                        default=Path(os.environ.get('ANDROID_BUILD_TOP', Path.home() / 'Android/LineageOS')))
    parser.add_argument('--component', choices=['all', 'camera2'], default='all')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    root = args.root.expanduser().resolve()
    patches = Path(__file__).resolve().parent
    groups = [
        ('camera2', 'packages/apps/Camera2', [
            patches / 'apps_Camera2/0001-fix-lg-virtual-front-camera.patch',
            patches / 'apps_Camera2/0002-fix-capture-module-virtual-front.patch']),
    ]
    plans = []
    with tempfile.TemporaryDirectory(prefix='lge-userspace-plan-') as temporary:
        for component, relative_repo, series in groups:
            if args.component not in ('all', component):
                continue
            repo = root / relative_repo
            if not (repo / '.git').exists():
                raise RuntimeError('Missing Git checkout: ' + str(repo))
            texts = [patch.read_text() for patch in series]
            paths = set()
            for data in texts:
                names = re.findall(r'^diff --git a/(\S+) b/(\S+)$', data, re.M)
                if not names:
                    raise RuntimeError('A carried patch has no supported diff headers')
                for before, after in names:
                    if before != after or not after.startswith(relative_repo + '/'):
                        raise RuntimeError('Unexpected patch path: ' + after)
                    name = after[len(relative_repo) + 1:]
                    if '..' in Path(name).parts or Path(name).is_absolute():
                        raise RuntimeError('Unsafe patch path: ' + name)
                    paths.add(name)
            originals = {name: fingerprint(repo, name) for name in paths}
            sandbox = Path(temporary) / component
            sandbox.mkdir()
            for name, original in originals.items():
                if original is not None:
                    destination = sandbox / name
                    destination.parent.mkdir(parents=True, exist_ok=True)
                    destination.write_bytes(original[0])
                    destination.chmod(original[1])
            pending = []
            for patch, data in zip(series, texts):
                check = command(sandbox, data, '--check')
                if check.returncode:
                    reverse = command(sandbox, data, '--reverse', '--check')
                    if reverse.returncode:
                        raise RuntimeError(str(repo) + ': ' + patch.name + '\n'
                                           + check.stderr + reverse.stderr)
                    print(relative_repo + ': already applied: ' + patch.name)
                else:
                    result = command(sandbox, data)
                    if result.returncode:
                        raise RuntimeError(result.stderr)
                    pending.append(data)
                    print(relative_repo + ': pending: ' + patch.name)
            plans.append((repo, originals, ''.join(pending)))
        if args.check:
            print('Preflight passed. Working files, indexes and commit histories were not changed.')
            return
        # Check every selected checkout before applying to any real checkout.
        for repo, originals, data in plans:
            for name, original in originals.items():
                if fingerprint(repo, name) != original:
                    raise RuntimeError('File changed during preflight: ' + str(repo / name))
            if data:
                result = command(repo, data, '--check')
                if result.returncode:
                    raise RuntimeError(result.stderr)
        for repo, originals, data in plans:
            if data:
                result = command(repo, data)
                if result.returncode:
                    raise RuntimeError(result.stderr)
                print(str(repo) + ': working source patched; index and history preserved.')


if __name__ == '__main__':
    try:
        main()
    except (OSError, RuntimeError) as error:
        sys.exit(str(error))
