# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2019 - 2021 Iñigo Martinez <inigomartinez@gmail.com>

generated_inc = include_directories('.')

gen_sources = []
gen_headers = []
private_gen_sources = []
private_gen_headers = []
gen_sections = []
gen_sections_deps = []

# Error types
enum_types = 'qmi-error-types'

gen_headers += custom_target(
  enum_types + '.h',
  input: qmi_errors_header,
  output: enum_types + '.h',
  command: [
    python,
    qmi_mkenums,
    '--fhead', '#ifndef __LIBQMI_GLIB_ERROR_TYPES_H__\n#define __LIBQMI_GLIB_ERROR_TYPES_H__\n#include "qmi-errors.h"\n',
    '--template', files(templates_dir / enum_types + '.h.template'),
    '--ftail', '#endif /* __LIBQMI_GLIB_ERROR_TYPES_H__ */\n',
    '@INPUT@'],
  capture: true,
  install: true,
  install_dir: qmi_glib_pkgincludedir,
)

gen_sources += custom_target(
  enum_types + '.c',
  input: qmi_errors_header,
  output: enum_types + '.c',
  command: [
    python,
    qmi_mkenums,
    '--fhead', '#include "qmi-errors.h"\n#include "qmi-error-types.h"\n',
    '--template', files(templates_dir / enum_types + '.c.template'),
    '@INPUT@'],
  capture: true,
)

enum_types = 'qmi-error-quarks'

gen_sources += custom_target(
  enum_types + '.c',
  input: qmi_errors_header,
  output: enum_types + '.c',
  command: [
    python,
    qmi_mkenums,
    '--fhead', '#include "qmi-errors.h"\n#include "qmi-error-types.h"\n',
    '--template', files(templates_dir / enum_types + '.c.template'),
    '@INPUT@'],
  capture: true,
)

# Enum types
enum_types = 'qmi-enum-types'

gen_headers += custom_target(
  enum_types + '.h',
  input: qmi_enums_headers,
  output: enum_types + '.h',
  command: [
    python,
    qmi_mkenums,
    '--enums-only',
    '--fhead', '#ifndef __LIBQMI_GLIB_ENUM_TYPES_H__\n#define __LIBQMI_GLIB_ENUM_TYPES_H__\n#include "qmi-enums.h"\n#include "qmi-enums-wds.h"\n#include "qmi-enums-dms.h"\n#include "qmi-enums-nas.h"\n#include "qmi-enums-wms.h"\n#include "qmi-enums-pds.h"\n#include "qmi-enums-pdc.h"\n#include "qmi-enums-pbm.h"\n#include "qmi-enums-uim.h"\n#include "qmi-enums-sar.h"\n#include "qmi-enums-oma.h"\n#include "qmi-enums-wda.h"\n#include "qmi-enums-voice.h"\n#include "qmi-enums-loc.h"\n#include "qmi-enums-qos.h"\n#include "qmi-enums-gas.h"\n#include "qmi-enums-dsd.h"\n#include "qmi-enums-fox.h"\n#include "qmi-enums-imsp.h"\n#include "qmi-device.h"\n',
    '--template', files(templates_dir / enum_types + '.h.template'),
    '--ftail', '#endif /* __LIBQMI_GLIB_ENUM_TYPES_H__ */\n',
    '@INPUT@'],
  capture: true,
  install: true,
  install_dir: qmi_glib_pkgincludedir,
)

gen_sources += custom_target(
  enum_types + '.c',
  input: qmi_enums_headers,
  output: enum_types + '.c',
  command: [
    python,
    qmi_mkenums,
    '--enums-only',
    '--fhead', '#include "qmi-enum-types.h"\n',
    '--template', files(templates_dir / enum_types + '.c.template'),
    '@INPUT@'],
  capture: true,
)

# Flag types
enum_types = 'qmi-flag-types'

gen_headers += custom_target(
  enum_types + '.h',
  input: qmi_enums_headers,
  output: enum_types + '.h',
  command: [
    python,
    qmi_mkenums,
    '--flags-only',
    '--fhead', '#ifndef __LIBQMI_GLIB_FLAG_TYPES_H__\n#define __LIBQMI_GLIB_FLAG_TYPES_H__\n#include "qmi-enums.h"\n#include "qmi-enums-wds.h"\n#include "qmi-enums-dms.h"\n#include "qmi-enums-nas.h"\n#include "qmi-enums-wms.h"\n#include "qmi-enums-pds.h"\n#include "qmi-enums-pdc.h"\n#include "qmi-enums-pbm.h"\n#include "qmi-enums-uim.h"\n#include "qmi-enums-sar.h"\n#include "qmi-enums-oma.h"\n#include "qmi-enums-wda.h"\n#include "qmi-enums-voice.h"\n#include "qmi-enums-loc.h"\n#include "qmi-enums-qos.h"\n#include "qmi-enums-gas.h"\n#include "qmi-enums-dsd.h"\n#include "qmi-enums-fox.h"\n#include "qmi-device.h"\n',
    '--template', files(templates_dir / enum_types + '.h.template'),
    '--ftail', '#endif /* __LIBQMI_GLIB_FLAG_TYPES_H__ */\n',
    '@INPUT@'],
  capture: true,
  install: true,
  install_dir: qmi_glib_pkgincludedir,
)

gen_sources += custom_target(
  enum_types + '.c',
  input: qmi_enums_headers,
  output: enum_types + '.c',
  command: [
    python,
    qmi_mkenums,
    '--flags-only',
    '--fhead', '#include "qmi-flag-types.h"\n',
    '--template', files(templates_dir / enum_types + '.c.template'),
    '@INPUT@'],
  capture: true,
)

# Private Enum types
enum_types = 'qmi-enum-types-private'

private_gen_headers += custom_target(
  enum_types + '.h',
  input: qmi_enums_private_header,
  output: enum_types + '.h',
  command: [
    python,
    qmi_mkenums,
    '--enums-only',
    '--fhead', '#ifndef __LIBQMI_GLIB_ENUM_TYPES_PRIVATE_H__\n#define __LIBQMI_GLIB_ENUM_TYPES_PRIVATE_H__\n#include "qmi-enums-private.h"\n',
    '--template', files(templates_dir / enum_types + '.h.template'),
    '--ftail', '#endif /* __LIBQMI_GLIB_ENUM_TYPES_PRIVATE_H__ */\n',
    '@INPUT@'],
  capture: true,
)

private_gen_sources += custom_target(
  enum_types + '.c',
  input: qmi_enums_private_header,
  output: enum_types + '.c',
  command: [
    python,
    qmi_mkenums,
    '--enums-only',
    '--fhead', '#include "qmi-enum-types-private.h"\n',
    '--template', files(templates_dir / enum_types + '.c.template'),
    '@INPUT@'],
  capture: true,
)

# Private Flag types
enum_types = 'qmi-flag-types-private'

private_gen_headers += custom_target(
  enum_types + '.h',
  input: qmi_enums_private_header,
  output: enum_types + '.h',
  command: [
    python,
    qmi_mkenums,
    '--flags-only',
    '--fhead', '#ifndef __LIBQMI_GLIB_FLAG_TYPES_PRIVATE_H__\n#define __LIBQMI_GLIB_FLAG_TYPES_PRIVATE_H__\n#include "qmi-enums-private.h"\n',
    '--template', files(templates_dir / enum_types + '.h.template'),
    '--ftail', '#endif /* __LIBQMI_GLIB_FLAG_TYPES_PRIVATE_H__ */\n',
    '@INPUT@'],
  capture: true,
)

private_gen_sources += custom_target(
  enum_types + '.c',
  input: qmi_enums_private_header,
  output: enum_types + '.c',
  command: [
    python,
    qmi_mkenums,
    '--flags-only',
    '--fhead', '#include "qmi-flag-types-private.h"\n',
    '--template', files(templates_dir / enum_types + '.c.template'),
    '@INPUT@'],
  capture: true,
)

# 64bit flag types
enum_types = 'qmi-flags64-types'

gen_headers += custom_target(
  enum_types + '.h',
  input: qmi_flags64_headers,
  output: enum_types + '.h',
  command: [
    python,
    qmi_mkenums,
    '--fhead', '#ifndef __LIBQMI_GLIB_FLAGS64_TYPES_H__\n#define __LIBQMI_GLIB_FLAGS64_TYPES_H__\n#include "qmi-flags64-dms.h"\n#include "qmi-flags64-nas.h"\n#include "qmi-flags64-loc.h"\n#include "qmi-flags64-dsd.h"\n#include "qmi-flags64-wds.h"\n',
    '--template', files(templates_dir / enum_types + '.h.template'),
    '--ftail', '#endif /* __LIBQMI_GLIB_FLAGS64_TYPES_H__ */\n',
    '@INPUT@'],
  capture: true,
  install: true,
  install_dir: qmi_glib_pkgincludedir,
)

gen_sources += custom_target(
  enum_types + '.c',
  input: qmi_flags64_headers,
  output: enum_types + '.c',
  command: [
    python,
    qmi_mkenums,
    '--fhead', '#include "qmi-flags64-types.h"\n',
    '--template', files(templates_dir / enum_types + '.c.template'),
    '@INPUT@'],
  capture: true,
)

qmi_common = data_dir / 'qmi-common.json'

service = 'ctl'
name = 'qmi-' + service

generated = custom_target(
  name,
  input: data_dir / 'qmi-service-@0@.json'.format(service),
  output: [name + '.c', name + '.h', name + '.sections'],
  command: [qmi_codegen, '--input', '@INPUT@', '--include', qmi_common, '--output', '@OUTDIR@' / name],
)

private_gen_sources += [generated[0], generated[1]]

services = [
  'atr',
  'dms',
  'dpm',
  'dsd',
  'imsp',
  'fox',
  'gas',
  'gms',
  'loc',
  'nas',
  'oma',
  'pbm',
  'pdc',
  'pds',
  'qos',
  'sar',
  'uim',
  'voice',
  'wda',
  'wds',
  'wms',
]

command = [
  qmi_codegen,
  '--input', '@INPUT@',
  '--include', qmi_common,
]

if qmi_collection_name != 'full'
  command += ['--collection', data_dir / 'qmi-collection-@0@.json'.format(qmi_collection_name)]
endif

foreach service: services
  name = 'qmi-' + service

  generated = custom_target(
    name,
    input: data_dir / 'qmi-service-@0@.json'.format(service),
    output: [name + '.c', name + '.h', name + '.sections'],
    command: command + ['--output', '@OUTDIR@' / name],
    install: true,
    install_dir: [false, qmi_glib_pkgincludedir, false],
  )

  gen_sources += generated[0]
  gen_headers += generated[1]
  # FIXME: the third target generated by custom target can't by used because is not a known
  #        source file, to the path has to be used. the first workaround is to use the
  #        build paths to point to the files, and the second workaround is to use
  #        custom target objects to force its building.
  gen_sections += [meson.current_build_dir() / name + '.sections']
  gen_sections_deps += [generated]
endforeach

incs = [
  top_inc,
  libqmi_glib_inc,
]

c_flags = [
  '-DLIBQMI_GLIB_COMPILATION',
  '-DG_LOG_DOMAIN="Qmi"',
  '-Wno-unused-function',
]

deps = [glib_deps]

if enable_qrtr
  deps += qrtr_glib_dep
endif

libqmi_glib_generated = static_library(
  'qmi-glib-generated',
  sources: private_gen_sources + private_gen_headers + gen_sources + gen_headers + [version_header],
  include_directories: incs,
  dependencies: deps,
  c_args: c_flags,
)

# FIXME: in older versions of meson, if libqmi-glib-generated is linked as shared
#        it will also try to link it in documentation generation
generated_dep = declare_dependency(
  sources: private_gen_headers + gen_headers + [version_header],
  include_directories: [libqmi_glib_inc, generated_inc],
  dependencies: deps,
)
