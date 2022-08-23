# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2021 Iñigo Martinez <inigomartinez@gmail.com>

mbimcli_sources = files(
  'mbimcli-atds.c',
  'mbimcli-basic-connect.c',
  'mbimcli.c',
  'mbimcli-dss.c',
  'mbimcli-intel-firmware-update.c',
  'mbimcli-intel-thermal-rf.c',
  'mbimcli-ms-basic-connect-extensions.c',
  'mbimcli-ms-firmware-id.c',
  'mbimcli-ms-host-shutdown.c',
  'mbimcli-ms-sar.c',
  'mbimcli-ms-uicc-low-level-access.c',
  'mbimcli-ms-voice-extensions.c',
  'mbimcli-phonebook.c',
  'mbimcli-quectel.c',
)

sources = mbimcli_sources + files(
  'mbimcli-helpers.c',
  'mbimcli-link-management.c',
)

deps = [
  libmbim_common_dep,
  libmbim_glib_dep,
]

mbimcli = executable(
  'mbimcli',
  sources: sources,
  include_directories: top_inc,
  dependencies: deps,
  install: true,
)

if enable_bash_completion
  install_data(
    'mbimcli',
    install_dir: bash_completion_completionsdir,
  )
endif
