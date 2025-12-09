# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2019 - 2021 Iñigo Martinez <inigomartinez@gmail.com>

md_files = []
foreach md_src : gen_sections
  md_files += fs.name(md_src)
endforeach

toml_data = configuration_data()
toml_data.set('VERSION', meson.project_version())
toml_data.set('CONTENT_FILES', '"' + '","'.join(md_files) + '"')

qmi_toml = configure_file(
  input: 'libqmi-glib.toml.in',
  output: 'libqmi-glib.toml',
  configuration: toml_data,
)

dependency(
  'gi-docgen',
  version: '>= 2021.1',
  native: true,
  required: get_option('gtk_doc'),
)

gidocgen = find_program('gi-docgen')

docs_dir = qmi_datadir / 'doc'

custom_target(
  'libqmi-glib',
  input: [qmi_toml, libqmi_glib_gir[0]],
  output: 'libqmi-glib-@0@'.format(qmi_gir_version),
  command: [
    gidocgen,
    'generate',
    '--quiet',
    '--config=@INPUT0@',
    '--output-dir=@OUTPUT@',
    '--no-namespace-dir',
    '--content-dir=@0@'.format(meson.current_source_dir()),
    '--content-dir=@0@'.format(meson.project_build_root() / 'src' / 'libqmi-glib' / 'generated'),
    '--fatal-warnings',
    '@INPUT1@',
  ],
  build_by_default: true,
  install: true,
  install_dir: docs_dir,
  depends: gen_sections_deps,
)
