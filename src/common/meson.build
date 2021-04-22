# SPDX-License-Identifier: GPL-2
# Copyright (C) 2021 Iñigo Martinez <inigomartinez@gmail.com>

libmbim_common = static_library(
  'mbim-common',
  sources: 'mbim-common.c',
  include_directories: top_inc,
  dependencies: glib_dep,
)

libmbim_common_dep = declare_dependency(
  include_directories: include_directories('.'),
  dependencies: glib_dep,
  link_with: libmbim_common,
)

subdir('test')
