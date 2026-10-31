#!/usr/bin/env python
"""Compatibility entry point for the Bash Camera2 preparation helper."""
from __future__ import print_function
import os
import subprocess
import sys


def main():
    helper = os.path.join(os.path.dirname(os.path.realpath(__file__)),
                          'apply-camera2-patches.sh')
    try:
        return subprocess.call(['bash', helper] + sys.argv[1:])
    except OSError as error:
        sys.stderr.write('Unable to start the Camera2 shell helper: ' + str(error) + '\n')
        return 1


if __name__ == '__main__':
    sys.exit(main())
