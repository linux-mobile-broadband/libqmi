# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2021 Iñigo Martinez <inigomartinez@gmail.com>

project(
  'libmbim', 'c',
  version: '1.27.0',
  license: ['GPL-2.0-or-later', 'LGPL-2.1-or-later'],
  default_options: [
    'buildtype=debugoptimized',
    'c_std=gnu89',
    'warning_level=2',
  ],
  meson_version: '>= 0.53.0',
)

mbim_name = meson.project_name()
mbim_version = meson.project_version()
version_array = mbim_version.split('.')
mbim_major_version = version_array[0].to_int()
mbim_minor_version = version_array[1].to_int()
mbim_micro_version = version_array[2].to_int()

mbim_prefix = get_option('prefix')
mbim_bindir = get_option('bindir')
mbim_datadir = get_option('datadir')
mbim_includedir = get_option('includedir')
mbim_libexecdir = get_option('libexecdir')
mbim_mandir = get_option('mandir')

mbim_glib_include_subdir = mbim_name + '-glib'
mbim_glib_pkgincludedir = mbim_includedir / mbim_glib_include_subdir

# libtool versioning for libmbim-glib (-version-info c:r:a)
# - If the interface is unchanged, but the implementation has changed or been fixed, then increment r
# - Otherwise, increment c and zero r.
#   - If the interface has grown (that is, the new library is compatible with old code), increment a.
#   - If the interface has changed in an incompatible way (that is, functions have changed or been removed), then zero a.
current = 10
revision = 0
age = 6
mbim_glib_version = '@0@.@1@.@2@'.format(current - age, age, revision)

mbim_gir_version = '1.0'

gnome = import('gnome')
i18n = import('i18n')
pkg = import('pkgconfig')
python = import('python').find_installation('python3')

source_root = meson.current_source_dir()

data_dir = source_root / 'data'
templates_dir = source_root / 'build-aux/templates'

mbim_codegen = find_program(source_root / 'build-aux/mbim-codegen/mbim-codegen')
mbim_mkenums = find_program(source_root / 'build-aux/mbim-mkenums')

top_inc = include_directories('.')

cc = meson.get_compiler('c')

config_h = configuration_data()
config_h.set_quoted('PACKAGE_VERSION', mbim_version)

# compiler flags
common_flags = ['-DHAVE_CONFIG_H']

# compiler flags that are always enabled, even in release builds
cc_flags = cc.get_supported_arguments([
  # warning on unused parameters is overkill, never do that
  '-Wno-unused-parameter',
  # function type cast disabled: used throughout the code especially to
  # cast GAsyncReadyCallbacks with the real object type instead of GObject
  '-Wno-cast-function-type',
  # all message protocol structs are packed, never complain about it
  '-Wno-packed',
])

# strict flags to use in debug builds
if get_option('buildtype').contains('debug')
  cc_flags += cc.get_supported_arguments([
    '-fno-strict-aliasing',
    '-Waggregate-return',
    '-Wcast-align',
    '-Wdeclaration-after-statement',
    '-Wdouble-promotion',
    '-Wduplicated-branches',
    '-Wduplicated-cond',
    '-Wfloat-equal',
    '-Wformat=2',
    '-Wformat-nonliteral',
    '-Wformat-security',
    '-Winit-self',
    '-Winline',
    '-Wjump-misses-init',
    '-Wlogical-op',
    '-Wnested-externs',
    '-Wmissing-declarations',
    '-Wmissing-format-attribute',
    '-Wmissing-include-dirs',
    '-Wmissing-noreturn',
    '-Wmissing-prototypes',
    '-Wnull-dereference',
    '-Wpointer-arith',
    '-Wredundant-decls',
    '-Wrestrict',
    '-Wreturn-type',
    '-Wshadow',
    '-Wstrict-prototypes',
    '-Wsuggest-attribute=format',
    '-Wswitch-default',
    '-Wswitch-enum',
    '-Wundef',
    '-Wunused-but-set-variable',
    '-Wwrite-strings',
  ])
endif

add_project_arguments(common_flags + cc_flags, language: 'c')

glib_version = '2.56'

glib_dep = dependency('glib-2.0', version: '>= ' + glib_version)
gio_unix_dep = dependency('gio-unix-2.0')

deps = [
  glib_dep,
  dependency('gio-2.0'),
  dependency('gobject-2.0'),
]

c_flags = [
  '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_' + glib_version.underscorify(),
  '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_' + glib_version.underscorify(),
  '-DGLIB_DISABLE_DEPRECATION_WARNINGS',
]

glib_deps = declare_dependency(
  dependencies: deps,
  compile_args: c_flags,
)

enable_bash_completion = get_option('bash_completion')
if enable_bash_completion
  bash_completion_dep = dependency('bash-completion')
  bash_completion_completionsdir = bash_completion_dep.get_pkgconfig_variable(
    'completionsdir',
    # bash-completion 2.10 changed the substitutions
    define_variable: bash_completion_dep.version().version_compare('>= 2.10') ? ['datadir', mbim_datadir] : ['prefix', mbim_prefix],
  )
endif

# MBIM username
mbim_username = get_option('mbim_username')
enable_mbim_username = (mbim_username != '')
if enable_mbim_username
  config_h.set_quoted('MBIM_USERNAME', mbim_username)

  # FIXME: udev base directory, prefix can't be overrided
  udev_udevdir = get_option('udevdir')
  if udev_udevdir == ''
    udev_udevdir = dependency('udev').get_pkgconfig_variable('udevdir')
  endif
else
  mbim_username = 'no (root)'
endif
config_h.set('MBIM_USERNAME_ENABLED', enable_mbim_username)

# introspection support
enable_gir = dependency('gobject-introspection-1.0', version: '>= 0.9.6', required: get_option('introspection')).found()

version_conf = configuration_data()
version_conf.set('MBIM_MAJOR_VERSION', mbim_major_version)
version_conf.set('MBIM_MINOR_VERSION', mbim_minor_version)
version_conf.set('MBIM_MICRO_VERSION', mbim_micro_version)
version_conf.set('VERSION', mbim_version)

subdir('src')
subdir('utils')

enable_gtk_doc = get_option('gtk_doc')
if enable_gtk_doc
  subdir('docs/reference/libmbim-glib')
endif

help2man = find_program('help2man', required: false)
if help2man.found()
  subdir('docs/man')
endif

configure_file(
  output: 'config.h',
  configuration: config_h,
)

meson.add_install_script(
  'meson_post_install.py',
  get_option('bindir'),
)

summary({
  'compiler': cc.get_id(),
  'cflags': cc_flags,
  'Documentation': enable_gtk_doc,
  'bash completion': enable_bash_completion,
  'gobject introspection': enable_gir,
}, section: 'Build')

summary({
  'prefix': mbim_prefix,
  'udev base directory': (enable_mbim_username ? udev_udevdir : 'disabled'),
}, section: 'System paths')

summary({
  'MBIM username': mbim_username,
}, section: 'Features')
