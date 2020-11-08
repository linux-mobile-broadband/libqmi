# SPDX-License-Identifier: GPL-2
# Copyright (C) 2019 - 2021 Iñigo Martinez <inigomartinez@gmail.com>

libutils = static_library(
  'utils',
  sources: 'qfu-utils.c',
  include_directories: top_inc,
  dependencies: libqmi_glib_dep,
)

libutils_dep = declare_dependency(
  include_directories: include_directories('.'),
  dependencies: libqmi_glib_dep,
  link_with: libutils,
)

libfirehose = static_library(
  'firehose',
  sources: 'qfu-firehose-message.c',
  dependencies: glib_deps,
)

libfirehose_dep = declare_dependency(
  include_directories: include_directories('.'),
  dependencies: glib_deps,
  link_with: libfirehose,
)

enum_headers = files(
  'qfu-dload-message.h',
  'qfu-image.h',
  'qfu-qdl-message.h',
  'qfu-sahara-message.h',
)

sources = files(
  'qfu-at-device.c',
  'qfu-device-selection.c',
  'qfu-dload-message.c',
  'qfu-image.c',
  'qfu-image-cwe.c',
  'qfu-image-factory.c',
  'qfu-log.c',
  'qfu-main.c',
  'qfu-operation-reset.c',
  'qfu-operation-update.c',
  'qfu-operation-verify.c',
  'qfu-qdl-device.c',
  'qfu-qdl-message.c',
  'qfu-reseter.c',
  'qfu-sahara-device.c',
  'qfu-sahara-message.c',
  'qfu-udev-helpers.c',
  'qfu-updater.c',
)

enum_types = 'qfu-enum-types'

sources += custom_target(
  enum_types + '.h',
  input: enum_headers,
  output: enum_types + '.h',
  command: [
    python,
    qmi_mkenums,
    '--fhead', '#ifndef QFU_ENUM_TYPES_H\n#define QFU_ENUM_TYPES_H\n#include "qfu-image.h"\n#include "qfu-qdl-message.h"\n#include "qfu-dload-message.h"\n#include "qfu-sahara-message.h"\n',
    '--template', files(join_paths(templates_dir, enum_types + '.h.template')),
    '--ftail', '#endif /* __QFUENUM_TYPES_H__ */\n',
    '@INPUT@'],
  capture: true,
)

sources += custom_target(
  enum_types + '.c',
  input: enum_headers,
  output: enum_types + '.c',
  command: [
    python,
    qmi_mkenums,
    '--fhead', '#include "qfu-enum-types.h"\n',
    '--template', files(join_paths(templates_dir, enum_types + '.c.template')),
    '@INPUT@'],
  capture: true,
)

deps = [
  gudev_dep,
  libfirehose_dep,
  libutils_dep,
]

if enable_mbim_qmux
  deps += mbim_glib_dep
endif

qmi_firmware_update = executable(
  'qmi-firmware-update',
  sources: sources,
  include_directories: top_inc,
  dependencies: deps,
  install: true,
)

subdir('test')
