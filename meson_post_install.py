#!/usr/bin/env python3

# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2019 - 2021 Iñigo Martinez <inigomartinez@gmail.com>

import os
import subprocess
import sys

prefix = os.environ['MESON_INSTALL_DESTDIR_PREFIX']

bash_completion_completionsdir = sys.argv[2]
if bash_completion_completionsdir:
    if not os.path.isabs(bash_completion_completionsdir):
        bash_completion_completionsdir = os.path.join(prefix, bash_completion_completionsdir)
    os.rename(os.path.join(bash_completion_completionsdir, 'qmicli-completion'),
              os.path.join(bash_completion_completionsdir, 'qmicli'))
