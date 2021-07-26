#!/usr/bin/env python3

import os
import subprocess
import sys

prefix = os.environ['MESON_INSTALL_DESTDIR_PREFIX']

bindir = os.path.join(prefix, sys.argv[1])
subprocess.check_call(['chmod', '755', os.path.join(bindir, 'qmi-network')])

bash_completion_completionsdir = sys.argv[2]
if bash_completion_completionsdir:
    if not os.path.isabs(bash_completion_completionsdir):
        bash_completion_completionsdir = os.path.join(prefix, bash_completion_completionsdir)
    os.rename(os.path.join(bash_completion_completionsdir, 'qmicli-completion'),
              os.path.join(bash_completion_completionsdir, 'qmicli'))
