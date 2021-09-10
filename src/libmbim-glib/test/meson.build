# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2021 Iñigo Martinez <inigomartinez@gmail.com>

test_units = [
  'uuid',
  'cid',
  'message',
  'fragment',
  'message-parser',
  'message-builder',
  'proxy-helpers',
]

test_env = {
  'G_DEBUG': 'gc-friendly',
  'MALLOC_CHECK_': '2',
  'G_TEST_SRCDIR': meson.current_source_dir(),
  'G_TEST_BUILDDIR': meson.current_build_dir(),
}

foreach test_unit: test_units
  test_name = 'test-' + test_unit

  exe = executable(
    test_name,
    sources: test_name + '.c',
    include_directories: top_inc,
    dependencies: libmbim_glib_core_dep,
    c_args: '-DLIBMBIM_GLIB_COMPILATION',
  )

  test(
    test_unit,
    exe,
    env: test_env,
  )
endforeach
