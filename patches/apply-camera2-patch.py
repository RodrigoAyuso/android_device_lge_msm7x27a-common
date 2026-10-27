#!/usr/bin/env python3
"""Apply only the Camera2 patches; the shared helper preserves index and history."""
from pathlib import Path
import subprocess
import sys
helper = Path(__file__).resolve().with_name('apply-userspace-patches.py')
sys.exit(subprocess.call([sys.executable, str(helper), *sys.argv[1:], '--component', 'camera2']))
