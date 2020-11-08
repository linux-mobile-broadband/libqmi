# SPDX-License-Identifier: GPL-2
# Copyright (C) 2019 - 2021 Iñigo Martinez <inigomartinez@gmail.com>

test_units = [
  ['test-utils', libutils_dep],
  ['test-firehose', libfirehose_dep],
]

test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())

foreach test_unit: test_units
  exe = executable(
    # FIXME: added `firm` prefix to avoid target name collision
    'firm-' + test_unit[0],
    sources: test_unit[0] + '.c',
    include_directories: top_inc,
    dependencies: test_unit[1],
    c_args: '-DLIBQMI_GLIB_COMPILATION',
  )

  test(
    test_unit[0],
    exe,
    env: test_env,
  )
endforeach
