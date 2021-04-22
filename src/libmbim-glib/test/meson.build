# SPDX-License-Identifier: GPL-2
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

random_number = mbim_minor_version + meson.version().split('.').get(1).to_int()

test_env = environment()
test_env.set('G_DEBUG', 'gc-friendly')
test_env.set('MALLOC_CHECK_', '2')
test_env.set('MALLOC_PERTURB_', (random_number % 256).to_string())
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())

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
    test_unit[0],
    exe,
    env: test_env,
  )
endforeach
