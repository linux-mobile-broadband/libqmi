# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2021 Iñigo Martinez <inigomartinez@gmail.com>

libmbim_common = static_library(
  'mbim-common',
  sources: 'mbim-common.c',
  include_directories: top_inc,
  dependencies: glib_dep,
)

libmbim_common_dep = declare_dependency(
  include_directories: '.',
  dependencies: glib_dep,
  link_with: libmbim_common,
)

subdir('test')
