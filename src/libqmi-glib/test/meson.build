# SPDX-License-Identifier: GPL-2
# Copyright (C) 2019 - 2021 Iñigo Martinez <inigomartinez@gmail.com>

test_units = [
  ['test-compat-utils', files('test-compat-utils.c'), [libqmi_glib_dep]],
  ['test-message', files('test-message.c'), [libqmi_glib_dep]],
  ['test-utils', files('test-utils.c'), [libqmi_glib_dep]],
]

sources = files(
  'test-fixture.c',
  'test-generated.c',
  'test-port-context.c',
)

deps = [
  gio_unix_dep,
  libqmi_glib_dep,
]

test_units += [['test-generated', sources, deps]]

test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())

foreach test_unit: test_units
  exe = executable(
    test_unit[0],
    sources: test_unit[1],
    include_directories: top_inc,
    dependencies: test_unit[2],
    c_args: ['-DLIBQMI_GLIB_COMPILATION'] + no_deprecated_declarations_flags,
  )

  test(
    test_unit[0],
    exe,
    env: test_env,
  )
endforeach
