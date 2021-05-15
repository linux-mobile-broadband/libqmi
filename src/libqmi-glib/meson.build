# SPDX-License-Identifier: GPL-2
# Copyright (C) 2019 - 2021 Iñigo Martinez <inigomartinez@gmail.com>

libqmi_glib_inc = include_directories('.')

qmi_errors_header = files('qmi-errors.h')

qmi_enums_headers = files(
  'qmi-device.h',
  'qmi-enums.h',
  'qmi-enums-dms.h',
  'qmi-enums-dsd.h',
  'qmi-enums-gas.h',
  'qmi-enums-loc.h',
  'qmi-enums-nas.h',
  'qmi-enums-oma.h',
  'qmi-enums-pbm.h',
  'qmi-enums-pdc.h',
  'qmi-enums-pds.h',
  'qmi-enums-qos.h',
  'qmi-enums-sar.h',
  'qmi-enums-uim.h',
  'qmi-enums-voice.h',
  'qmi-enums-wda.h',
  'qmi-enums-wds.h',
  'qmi-enums-wms.h',
)

qmi_enums_private_header = files('qmi-enums-private.h')

qmi_flags64_headers = files(
  'qmi-flags64-dms.h',
  'qmi-flags64-dsd.h',
  'qmi-flags64-loc.h',
  'qmi-flags64-nas.h',
  'qmi-flags64-wds.h',
)

headers = qmi_errors_header + qmi_enums_headers + qmi_enums_private_header + qmi_flags64_headers + files(
  'libqmi-glib.h',
  'qmi-client.h',
  'qmi-message-context.h',
  'qmi-message.h',
  'qmi-proxy.h',
  'qmi-utils.h',
)

install_headers(
  headers + files('qmi-compat.h'),
  subdir: qmi_glib_include_subdir,
)

version_header = configure_file(
  input: 'qmi-version.h.in',
  output: '@BASENAME@',
  configuration: version_conf,
  install_dir: qmi_glib_pkgincludedir,
)

subdir('generated')

common_c_flags = [
  '-DLIBQMI_GLIB_COMPILATION',
  '-DG_LOG_DOMAIN="Qmi"',
]

libqmi_glib_compat = static_library(
  'qmi-glib-compat',
  sources: 'qmi-compat.c',
  include_directories: top_inc,
  dependencies: libqmi_glib_generated_dep,
  c_args: common_c_flags + no_deprecated_declarations_flags,
)

libname = 'qmi-glib'

sources = files(
  'qmi-client.c',
  'qmi-device.c',
  'qmi-endpoint.c',
  'qmi-endpoint-qmux.c',
  'qmi-enums-dms.c',
  'qmi-enums-nas.c',
  'qmi-enums-wds.c',
  'qmi-file.c',
  'qmi-helpers.c',
  'qmi-message.c',
  'qmi-message-context.c',
  'qmi-net-port-manager.c',
  'qmi-net-port-manager-qmiwwan.c',
  'qmi-proxy.c',
  'qmi-utils.c',
)

deps = [libqmi_glib_generated_dep]

private_deps = [gio_unix_dep]

if enable_mbim_qmux
  sources += files('qmi-endpoint-mbim.c')
  private_deps += mbim_glib_dep
endif

if enable_qrtr
  sources += files('qmi-endpoint-qrtr.c')
  deps += qrtr_glib_dep
endif

if have_rmnet
  sources += files('qmi-net-port-manager-rmnet.c')
endif

libqmi_glib = shared_library(
  libname,
  version: qmi_glib_version,
  sources: sources + [version_header],
  include_directories: top_inc,
  dependencies: deps + private_deps,
  c_args: common_c_flags + ['-DLIBEXEC_PATH="@0@"'.format(join_paths(qmi_prefix, qmi_libexecdir))],
  link_whole: libqmi_glib_compat,
  install: true,
)

libqmi_glib_dep = declare_dependency(
  sources: [gen_headers, version_header],
  include_directories: [libqmi_glib_inc, generated_inc],
  dependencies: deps,
  link_with: libqmi_glib,
)

pkg.generate(
  libraries: libqmi_glib,
  version: qmi_version,
  name: libname,
  description: 'Library to communicate with QMI-powered modems',
  subdirs: qmi_glib_include_subdir,
  # FIXME: produced by the inhability of meson to use internal dependencies
  requires: ['glib-2.0', 'gobject-2.0', 'gio-2.0'],
  variables: [
    'exec_prefix=${prefix}',
    'qmi_qrtr_supported=' + enable_qrtr.to_int().to_string(),
    'qmi_mbim_qmux_supported=' + enable_mbim_qmux.to_int().to_string(),
  ],
)

if enable_gir
  incs = [
    'Gio-2.0',
    'GLib-2.0',
    'GObject-2.0',
  ]

  if enable_qrtr
    incs += ['Qrtr-1.0']
  endif

  ns = 'Qmi'

  libqmi_glib_gir = gnome.generate_gir(
    libqmi_glib,
    sources: sources + headers + gen_sources + gen_headers,
    includes: incs,
    namespace: ns,
    nsversion: qmi_gir_version,
    identifier_prefix: ns,
    symbol_prefix: ns.to_lower() + '_',
    extra_args: common_c_flags + ['--identifier-prefix=' + ns.to_upper()],
    header: 'libqmi-glib.h',
    export_packages: 'libqmi-glib',
    install: true,
  )
endif

subdir('test')
