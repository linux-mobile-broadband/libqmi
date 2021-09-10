# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2019 - 2021 Iñigo Martinez <inigomartinez@gmail.com>

test_units = {
  'test-utils': {'dependencies': libutils_dep},
  'test-firehose': {'dependencies': libfirehose_dep},
}

test_env += {
  'G_TEST_BUILDDIR': meson.current_build_dir(),
  'G_TEST_SRCDIR': meson.current_source_dir(),
}

foreach test_name, test_args: test_units
  exe = executable(
    # FIXME: added `firm` prefix to avoid target name collision
    'firm-' + test_name,
    sources: test_name + '.c',
    include_directories: top_inc,
    c_args: '-DLIBQMI_GLIB_COMPILATION',
    kwargs: test_args,
  )

  test(
    test_name,
    exe,
    env: test_env,
  )
endforeach
