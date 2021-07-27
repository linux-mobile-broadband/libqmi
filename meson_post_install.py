#!/usr/bin/env python3

import os
import subprocess
import sys

prefix = os.environ['MESON_INSTALL_DESTDIR_PREFIX']

bindir = os.path.join(prefix, sys.argv[1])
subprocess.check_call(['chmod', '755', os.path.join(bindir, 'mbim-network')])
