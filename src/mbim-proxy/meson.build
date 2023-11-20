# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2021 Iñigo Martinez <inigomartinez@gmail.com>

name = 'mbim-proxy'

executable(
  name,
  sources: name + '.c',
  include_directories: top_inc,
  dependencies: libmbim_glib_dep,
  install: true,
  install_dir: mbim_libexecdir,
)
