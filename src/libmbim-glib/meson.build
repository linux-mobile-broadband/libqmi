# SPDX-License-Identifier: GPL-2
# Copyright (C) 2021 Iñigo Martinez <inigomartinez@gmail.com>

libmbim_glib_inc = include_directories('.')

mbim_errors_header = files('mbim-errors.h')

mbim_enums_headers = files(
  'mbim-cid.h',
  'mbim-enums.h',
  'mbim-message.h',
  'mbim-uuid.h',
)

subdir('generated')

headers = mbim_errors_header + mbim_enums_headers + files(
  'libmbim-glib.h',
  'mbim-compat.h',
  'mbim-device.h',
  'mbim-proxy.h',
  'mbim-utils.h',
)

install_headers(
  headers,
  subdir: mbim_glib_include_subdir,
)

common_c_flags = [
  '-DLIBMBIM_GLIB_COMPILATION',
  '-DG_LOG_DOMAIN="Mbim"',
  '-DLIBEXEC_PATH="@0@"'.format(join_paths(mbim_prefix, mbim_libexecdir)),
]

sources = files(
  'mbim-cid.c',
  'mbim-compat.c',
  'mbim-device.c',
  'mbim-helpers.c',
  'mbim-message.c',
  'mbim-net-port-manager.c',
  'mbim-proxy.c',
  'mbim-proxy-helpers.c',
  'mbim-utils.c',
  'mbim-uuid.c',
)

deps = [
  libmbim_common_dep,
  libmbim_glib_generated_dep,
]

libmbim_glib_core = static_library(
  'mbim-glib-core',
  sources: sources,
  include_directories: top_inc,
  dependencies: deps + [gio_unix_dep],
  c_args: common_c_flags,
)

libmbim_glib_core_dep = declare_dependency(
  include_directories: libmbim_glib_inc,
  dependencies: deps,
  link_with: libmbim_glib_core,
)

libname = 'mbim-glib'

version_header = configure_file(
  input: 'mbim-version.h.in',
  output: '@BASENAME@',
  configuration: version_conf,
  install_dir: mbim_glib_pkgincludedir,
)

symbol_map = join_paths(meson.current_source_dir(), 'symbol.map')

libmbim_glib = shared_library(
  libname,
  version: mbim_glib_version,
  sources: version_header,
  include_directories: top_inc,
  link_args: '-Wl,--version-script,' + symbol_map,
  link_depends: symbol_map,
  link_whole: libmbim_glib_core,
  install: true,
)

libmbim_glib_dep = declare_dependency(
  sources: [gen_headers, version_header],
  include_directories: [libmbim_glib_inc, generated_inc],
  dependencies: glib_deps,
  link_with: libmbim_glib,
)

pkg.generate(
  libraries: libmbim_glib,
  version: mbim_version,
  name: libname,
  description: 'Library to communicate with MBIM-powered modems',
  subdirs: mbim_glib_include_subdir,
  # FIXME: produced by the inhability of meson to use internal dependencies
  requires: ['glib-2.0', 'gobject-2.0', 'gio-2.0'],
  variables: 'exec_prefix=${prefix}',
)

# Introspection
if enable_gir
  incs = [
    'Gio-2.0',
    'GLib-2.0',
    'GObject-2.0',
  ]

  ns = 'Mbim'

  libmbim_glib_gir = gnome.generate_gir(
    libmbim_glib,
    sources: headers + [version_header] + gen_sources + gen_headers,
    includes: incs,
    dependencies: libmbim_glib_dep,
    namespace: ns,
    nsversion: mbim_gir_version,
    identifier_prefix: ns,
    symbol_prefix: ns.to_lower() + '_',
    extra_args: common_c_flags + ['--identifier-prefix=' + ns.to_upper()],
    header: 'libmbim-glib.h',
    export_packages: 'libmbim-glib',
    install: true,
  )
endif

subdir('test')
