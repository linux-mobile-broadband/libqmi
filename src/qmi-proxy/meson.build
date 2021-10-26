# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2019 - 2021 Iñigo Martinez <inigomartinez@gmail.com>

name = 'qmi-proxy'

executable(
  name,
  sources: name + '.c',
  include_directories: top_inc,
  dependencies: libqmi_glib_dep,
  install: true,
  install_dir: qmi_libexecdir,
)

if enable_qmi_username
  configure_file(
    input: '76-qmi-proxy-device-ownership.rules.in',
    output: '@BASENAME@',
    configuration: {'QMI_USERNAME': qmi_username},
    install_dir: udev_udevdir / 'rules.d',
  )
endif
