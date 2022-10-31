# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2019 - 2021 Iñigo Martinez <inigomartinez@gmail.com>

test_units = {
  'test-compat-utils': {'sources': files('test-compat-utils.c'), 'dependencies': libqmi_glib_dep},
  'test-message': {'sources': files('test-message.c'), 'dependencies': libqmi_glib_dep},
  'test-utils': {'sources': files('test-utils.c'), 'dependencies': libqmi_glib_dep},
}

sources = files(
  'test-fixture.c',
  'test-generated.c',
  'test-port-context.c',
)

deps = [
  gio_unix_dep,
  libqmi_glib_dep,
]

test_units += {'test-generated': {'sources': sources, 'dependencies': deps}}

test_env += {
  'G_TEST_BUILDDIR': meson.current_build_dir(),
  'G_TEST_SRCDIR': meson.current_source_dir(),
}

foreach test_name, test_args: test_units
  exe = executable(
    test_name,
    include_directories: top_inc,
    c_args: ['-DLIBQMI_GLIB_COMPILATION'] + no_deprecated_declarations_flags,
    kwargs: test_args,
  )

  test(
    test_name,
    exe,
    env: test_env,
  )
endforeach

if get_option('fuzzer')
  fuzzer_name = 'test-message-fuzzer'
  exe = executable(
    fuzzer_name,
    sources: fuzzer_name + '.c',
    include_directories: top_inc,
    dependencies: libqmi_glib_dep,
    c_args: '-DLIBQMI_GLIB_COMPILATION',
    link_args : '-fsanitize=fuzzer',
  )
endif
