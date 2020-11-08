# SPDX-License-Identifier: GPL-2
# Copyright (C) 2019 - 2021 Iñigo Martinez <inigomartinez@gmail.com>

cflags = cc.get_supported_arguments([
  '-Wno-empty-body',
  '-Wno-sign-compare',
  '-Wno-switch-default',
])

executable(
  'swi-update',
  sources: 'swi-update.c',
  c_args: cflags,
)

qmi_network = configure_file(
  input: 'qmi-network.in',
  output: '@BASENAME@',
  configuration: version_conf,
  install_dir: qmi_bindir,
)
