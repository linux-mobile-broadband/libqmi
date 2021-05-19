# SPDX-License-Identifier: GPL-2
# Copyright (C) 2019 - 2021 Iñigo Martinez <inigomartinez@gmail.com>

project(
  'libqmi', 'c',
  version: '1.29.7',
  license: 'GPL2',
  default_options: [
    'buildtype=debugoptimized',
    'c_std=gnu89',
    'warning_level=2',
  ],
  meson_version: '>= 0.45.1',
)

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

qmi_glib_include_subdir = meson.project_name() + '-glib'
qmi_glib_pkgincludedir = join_paths(qmi_includedir, qmi_glib_include_subdir)

# libtool versioning for libqmi-glib (-version-info c:r:a)
# - If the interface is unchanged, but the implementation has changed or been fixed, then increment r
# - Otherwise, increment c and zero r.
#   - If the interface has grown (that is, the new library is compatible with old code), increment a.
#   - If the interface has changed in an incompatible way (that is, functions have changed or been removed), then zero a.
current = 12
revision = 0
age = 7
qmi_glib_version = '@0@.@1@.@2@'.format(current - age, age, revision)

qmi_gir_version = '1.0'

gnome = import('gnome')
i18n = import('i18n')
pkg = import('pkgconfig')
python = import('python3').find_python()

source_root = meson.current_source_dir()

data_dir = join_paths(source_root, 'data')
templates_dir = join_paths(source_root, 'build-aux/templates')

qmi_codegen = find_program(join_paths(source_root, 'build-aux/qmi-codegen/qmi-codegen'))
qmi_mkenums = find_program(join_paths(source_root, 'build-aux/qmi-mkenums'))

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

gudev_dep = dependency('gudev-1.0', version: '>= 232', required: enable_udev and enable_firmware_update)
config_h.set('WITH_UDEV', gudev_dep.found())

bash_completion_completionsdir = dependency('bash-completion').get_pkgconfig_variable('completionsdir', define_variable: ['datadir', qmi_datadir])

have_rmnet = cc.has_type('struct ifla_rmnet_flags', prefix: '#include<linux/if_link.h>')
config_h.set('RMNET_SUPPORT_ENABLED', have_rmnet)

# runtime MM check is optional, enabled by default
enable_mm_runtime_check = get_option('mm_runtime_check')
config_h.set('MM_RUNTIME_CHECK_ENABLED', enable_mm_runtime_check)

# QMI username
qmi_username = get_option('qmi_username')
enable_qmi_username = (qmi_username != '')
if enable_qmi_username
  config_h.set_quoted('QMI_USERNAME', qmi_username)

  # FIXME: udev base directory, prefix can't be overrided
  udev_udevdir = get_option('udevdir')
  if udev_udevdir == ''
    udev_udevdir = dependency('udev').get_pkgconfig_variable('udevdir')
  endif
else
  qmi_username = 'no (root)'
endif
config_h.set('QMI_USERNAME_ENABLED', enable_qmi_username)

# MBIM QMUX service support
enable_mbim_qmux = false
if get_option('mbim_qmux')
  mbim_glib_dep = dependency('mbim-glib', version: '>= 1.18.0', required: false)
  enable_mbim_qmux = mbim_glib_dep.found()
endif
config_h.set('MBIM_QMUX_ENABLED', enable_mbim_qmux)

# QRTR support
enable_qrtr = false
if get_option('qrtr')
  qrtr_glib_dep = dependency('qrtr-glib', version: '>= 1.0.0', required: false)
  enable_qrtr = qrtr_glib_dep.found()
endif
config_h.set10('QMI_QRTR_SUPPORTED', enable_qrtr)

version_conf = configuration_data()
version_conf.set('VERSION', qmi_version)
version_conf.set('QMI_MAJOR_VERSION', qmi_major_version)
version_conf.set('QMI_MINOR_VERSION', qmi_minor_version)
version_conf.set('QMI_MICRO_VERSION', qmi_micro_version)
version_conf.set10('QMI_MBIM_QMUX_SUPPORTED', enable_mbim_qmux)
version_conf.set10('QMI_QRTR_SUPPORTED', enable_qrtr)

# introspection support
enable_gir = dependency('gobject-introspection-1.0', version: '>= 0.9.6', required: get_option('introspection')).found()

random_number = qmi_minor_version + meson.version().split('.').get(1).to_int()

test_env = environment()
test_env.set('G_DEBUG', 'gc-friendly')
test_env.set('MALLOC_CHECK_', '2')
test_env.set('MALLOC_PERTURB_', (random_number % 256).to_string())

subdir('src')
subdir('utils')

enable_gtk_doc = get_option('gtk_doc')
if enable_gtk_doc
  subdir('docs/reference/libqmi-glib')
endif

help2man = find_program('help2man', required: false)
if help2man.found()
  subdir('docs/man')
endif

configure_file(
  output: 'config.h',
  configuration: config_h,
)

output = '\n' + meson.project_name() + ' ' + meson.project_version() + '\n\n'
output += '  Build\n'
output += '    compiler:              ' + cc.get_id() + '\n'
output += '    cflags:                ' + ' '.join(cc_flags) + '\n'
output += '    Documentation:         ' + enable_gtk_doc.to_string() + '\n'
output += '    gobject introspection: ' + enable_gir.to_string() + '\n\n'
output += '  System paths\n'
output += '    prefix:                ' + qmi_prefix + '\n'
output += '    udev base directory:   ' + qmi_username + '\n\n'
output += '  Features\n'
output += '    QMI over MBIM:         ' + enable_mbim_qmux.to_string() + '\n'
output += '    QMI over QRTR:         ' + enable_qrtr.to_string() + '\n'
output += '    QMI username:          ' + qmi_username + '\n'
output += '    rmnet support:         ' + have_rmnet.to_string()
message(output)
