# SPDX-License-Identifier: GPL-2
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

if enable_mbim_username
  configure_file(
    input: '76-mbim-proxy-device-ownership.rules.in',
    output: '@BASENAME@',
    configuration: config_h,
    install_dir: join_paths(udev_udevdir, 'rules.d'),
  )
endif
