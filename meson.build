# SPDX-License-Identifier: GPL-2
# Copyright (C) 2021 Iñigo Martinez <inigomartinez@gmail.com>

project(
  'libmbim', 'c',
  version: '1.25.5',
  license: 'GPL2',
  default_options: [
    'buildtype=debugoptimized',
    'c_std=gnu89',
    'warning_level=2',
  ],
  meson_version: '>= 0.45.1',
)

mbim_version = meson.project_version()
version_array = mbim_version.split('.')
mbim_major_version = version_array[0].to_int()
mbim_minor_version = version_array[1].to_int()
mbim_micro_version = version_array[2].to_int()

mbim_prefix = get_option('prefix')
mbim_bindir = get_option('bindir')
mbim_includedir = get_option('includedir')
mbim_libexecdir = get_option('libexecdir')
mbim_mandir = get_option('mandir')

mbim_glib_include_subdir = meson.project_name() + '-glib'
mbim_glib_pkgincludedir = join_paths(mbim_includedir, mbim_glib_include_subdir)

# libtool versioning for libmbim-glib (-version-info c:r:a)
# - If the interface is unchanged, but the implementation has changed or been fixed, then increment r
# - Otherwise, increment c and zero r.
#   - If the interface has grown (that is, the new library is compatible with old code), increment a.
#   - If the interface has changed in an incompatible way (that is, functions have changed or been removed), then zero a.
current = 9
revision = 0
age = 5
mbim_glib_version = '@0@.@1@.@2@'.format(current - age, age, revision)

mbim_gir_version = '1.0'

gnome = import('gnome')
i18n = import('i18n')
pkg = import('pkgconfig')
python = import('python3').find_python()

source_root = meson.current_source_dir()

data_dir = join_paths(source_root, 'data')
templates_dir = join_paths(source_root, 'build-aux/templates')

mbim_codegen = find_program(join_paths(source_root, 'build-aux/mbim-codegen/mbim-codegen'))
mbim_mkenums = find_program(join_paths(source_root, 'build-aux/mbim-mkenums'))

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

bash_completion_completionsdir = dependency('bash-completion').get_pkgconfig_variable('completionsdir', define_variable: ['prefix', mbim_prefix])

glib_prefix = glib_dep.get_pkgconfig_variable('prefix')

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
enable_gir = get_option('introspection')
if enable_gir
  dependency('gobject-introspection-1.0', version: '>= 0.9.6')
endif

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
  bash_completion_completionsdir,
)

output = '\n' + meson.project_name() + ' ' + meson.project_version() + '\n\n'
output += '  Build\n'
output += '    compiler:              ' + cc.get_id() + '\n'
output += '    cflags:                ' + ' '.join(cc_flags) + '\n'
output += '    Documentation:         ' + enable_gtk_doc.to_string() + '\n'
output += '    gobject introspection: ' + enable_gir.to_string() + '\n\n'
output += '  System paths\n'
output += '    prefix:                ' + mbim_prefix + '\n'
output += '    udev base directory:   ' + mbim_username + '\n\n'
output += '  Features\n'
output += '    MBIM username:         ' + mbim_username
message(output)
