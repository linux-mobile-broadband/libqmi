# SPDX-License-Identifier: LGPL-2.1-or-later
# Copyright (C) 2023 Fabio Porcedda <fabio.porcedda@gmail.com>

libqmi_common = static_library(
  'qmi-common',
  sources: 'qmi-common.c',
  include_directories: top_inc,
  dependencies: glib_dep,
)

libqmi_common_dep = declare_dependency(
  include_directories: '.',
  dependencies: glib_dep,
  link_with: libqmi_common,
)
