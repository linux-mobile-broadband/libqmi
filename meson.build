# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2019 - 2021 Iñigo Martinez <inigomartinez@gmail.com>

project(
  'libqmi', 'c',
  version: '1.37.995',
  license: 'GPL2',
  default_options: [
    'buildtype=debugoptimized',
    'c_std=gnu89',
    'warning_level=2',
  ],
  meson_version: '>= 0.56.0',
)

fs = import('fs')

qmi_name = meson.project_name()
qmi_version = meson.project_version()
version_array = qmi_version.split('.')
qmi_major_version = version_array[0].to_int()
qmi_minor_version = version_array[1].to_int()
qmi_micro_version = version_array[2].to_int()

qmi_prefix = get_option('prefix')
qmi_bindir = get_option('bindir')
qmi_datadir = get_option('datadir')
qmi_includedir = get_option('includedir')
qmi_libexecdir = get_option('libexecdir')
qmi_mandir = get_option('mandir')

qmi_glib_include_subdir = qmi_name + '-glib'
qmi_glib_pkgincludedir = qmi_includedir / qmi_glib_include_subdir

# libtool versioning for libqmi-glib (-version-info c:r:a)
# - If the interface is unchanged, but the implementation has changed or been fixed, then increment r
# - Otherwise, increment c and zero r.
#   - If the interface has grown (that is, the new library is compatible with old code), increment a.
#   - If the interface has changed in an incompatible way (that is, functions have changed or been removed), then zero a.
current = 16
revision = 0
age = 11
qmi_glib_version = '@0@.@1@.@2@'.format(current - age, age, revision)

qmi_gir_version = '1.0'

gnome = import('gnome')
i18n = import('i18n')
pkg = import('pkgconfig')
python = import('python').find_installation('python3')

source_root = meson.current_source_dir()

data_dir = source_root / 'data'
templates_dir = source_root / 'build-aux/templates'

qmi_codegen = find_program(source_root / 'build-aux/qmi-codegen/qmi-codegen')
qmi_mkenums = find_program(source_root / 'build-aux/qmi-mkenums')

top_inc = include_directories('.')

cc = meson.get_compiler('c')

config_h = configuration_data()
config_h.set_quoted('PACKAGE_VERSION', qmi_version)

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

no_deprecated_declarations_flags = cc.get_supported_arguments('-Wno-deprecated-declarations')

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

# message collection (existing file in data/qmi-collection-${COLLECTION}.json)
# dnl custom collections may be added as files in data/
qmi_collection_name = get_option('collection')

# qmi-firmware-update is optional, enabled by default
enable_firmware_update = get_option('firmware_update')
assert(not enable_firmware_update or qmi_collection_name != 'minimal', 'Cannot build qmi-firmware-update when \'minimal\' collection enabled, use at least the \'basic\' collection instead.')

# udev support is optional, enabled by default
enable_udev = get_option('udev')
if enable_udev
  assert(enable_firmware_update, 'Cannot build udev support without firmware_update support')
  gudev_dep = dependency('gudev-1.0', version: '>= 232')
endif
config_h.set('WITH_UDEV', enable_udev)

enable_bash_completion = get_option('bash_completion')
if enable_bash_completion
  bash_completion_dep = dependency('bash-completion')
  bash_completion_completionsdir = bash_completion_dep.get_variable(
    pkgconfig: 'completionsdir',
    # bash-completion 2.10 changed the substitutions
    pkgconfig_define: bash_completion_dep.version().version_compare('>= 2.10') ? ['datadir', qmi_datadir] : ['prefix', qmi_prefix],
  )
endif

# rmnet link management support
enable_rmnet = get_option('rmnet')
config_h.set('RMNET_SUPPORT_ENABLED', enable_rmnet)

# runtime MM check is optional, enabled by default
config_h.set('MM_RUNTIME_CHECK_ENABLED', get_option('mm_runtime_check'))

# QMI username
qmi_username = get_option('qmi_username')
enable_qmi_username = (qmi_username != '')
if enable_qmi_username
  config_h.set_quoted('QMI_USERNAME', qmi_username)
else
  qmi_username = 'no (root)'
endif
config_h.set('QMI_USERNAME_ENABLED', enable_qmi_username)

# qmi group
qmi_groupname = get_option('qmi_groupname')
enable_qmi_groupname = (qmi_groupname != '')
if enable_qmi_groupname
  config_h.set_quoted('QMI_GROUPNAME', qmi_groupname)
else
  qmi_groupname = 'none'
endif
config_h.set('QMI_GROUPNAME_ENABLED', enable_qmi_groupname)

# udev base directory - needed if either username or groupname is specified
if enable_qmi_username or enable_qmi_groupname
  # FIXME: udev base directory, prefix can't be overrided
  udev_udevdir = get_option('udevdir')
  if udev_udevdir == ''
    udev_udevdir = dependency('udev').get_variable(pkgconfig: 'udevdir')
  endif
endif

# MBIM QMUX service support
enable_mbim_qmux = get_option('mbim_qmux')
if enable_mbim_qmux
  mbim_glib_dep = dependency('mbim-glib', version: '>= 1.18.0')
endif
config_h.set('MBIM_QMUX_ENABLED', enable_mbim_qmux)

# QRTR support
enable_qrtr = get_option('qrtr')
if enable_qrtr
  qrtr_glib_dep = dependency('qrtr-glib', version: '>= 1.0.0')
endif
config_h.set10('QMI_QRTR_SUPPORTED', enable_qrtr)

version_conf = configuration_data()
version_conf.set('VERSION', qmi_version)
version_conf.set('QMI_MAJOR_VERSION', qmi_major_version)
version_conf.set('QMI_MINOR_VERSION', qmi_minor_version)
version_conf.set('QMI_MICRO_VERSION', qmi_micro_version)
version_conf.set10('QMI_MBIM_QMUX_SUPPORTED', enable_mbim_qmux)
version_conf.set10('QMI_QRTR_SUPPORTED', enable_qrtr)
version_conf.set10('QMI_RMNET_SUPPORTED', enable_rmnet)

# introspection support
enable_gir = get_option('introspection')
if enable_gir
  dependency('gobject-introspection-1.0', version: '>= 0.9.6')
endif

test_env = {
  'G_DEBUG': 'gc-friendly',
  'MALLOC_CHECK_': '2',
}

subdir('src')
subdir('utils')
subdir('examples/simple-tester-c')

enable_gtk_doc = get_option('gtk_doc')
if enable_gtk_doc
  assert(qmi_collection_name == 'full', 'Building documentation requires the \'full\' message collection.')
  subdir('docs/reference/libqmi-glib')
endif

enable_man = get_option('man')
if enable_man
  subdir('docs/man')
endif

enable_fuzzer = get_option('fuzzer')

configure_file(
  output: 'config.h',
  configuration: config_h,
)

summary({
  'compiler': cc.get_id(),
  'cflags': cc_flags,
  'Documentation': enable_gtk_doc,
  'bash completion': enable_bash_completion,
  'gobject introspection': enable_gir,
  'man pages': enable_man,
  'fuzzer': enable_fuzzer,
}, section: 'Build')

summary({
  'prefix': qmi_prefix,
  'udev base directory': (enable_qmi_username or enable_qmi_groupname ? udev_udevdir : 'disabled'),
}, section: 'System paths')

summary({
  'QMI over MBIM': enable_mbim_qmux,
  'QMI over QRTR': enable_qrtr,
  'QMI username': qmi_username,
  'QMI groupname': qmi_groupname,
  'rmnet support': enable_rmnet,
}, section: 'Features')
