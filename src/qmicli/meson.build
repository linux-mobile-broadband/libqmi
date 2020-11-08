# SPDX-License-Identifier: GPL-2
# Copyright (C) 2019 - 2021 Iñigo Martinez <inigomartinez@gmail.com>

libhelpers = static_library(
  'helpers',
  sources: 'qmicli-helpers.c',
  dependencies: libqmi_glib_dep,
)

libhelpers_dep = declare_dependency(
  include_directories: include_directories('.'),
  dependencies: libqmi_glib_dep,
  link_with: libhelpers,
)

sources = files(
  'qmicli.c',
  'qmicli-dms.c',
  'qmicli-dpm.c',
  'qmicli-dsd.c',
  'qmicli-gas.c',
  'qmicli-gms.c',
  'qmicli-link-management.c',
  'qmicli-loc.c',
  'qmicli-nas.c',
  'qmicli-pbm.c',
  'qmicli-pdc.c',
  'qmicli-qmiwwan.c',
  'qmicli-qos.c',
  'qmicli-sar.c',
  'qmicli-uim.c',
  'qmicli-voice.c',
  'qmicli-wda.c',
  'qmicli-wds.c',
  'qmicli-wms.c',
)

deps = [libhelpers_dep]

if enable_mbim_qmux
  deps += mbim_glib_dep
endif

qmicli = executable(
  'qmicli',
  sources: sources,
  include_directories: top_inc,
  dependencies: deps,
  install: true,
)

install_data(
  'qmicli-completion',
  install_dir: bash_completion_completionsdir,
)

subdir('test')
