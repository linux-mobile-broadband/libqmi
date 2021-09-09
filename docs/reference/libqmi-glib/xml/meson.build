# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2019 - 2021 Iñigo Martinez <inigomartinez@gmail.com>

ent_conf = {
  'PACKAGE': meson.project_name(),
  'PACKAGE_BUGREPORT': meson.project_name() + '-devel@lists.freedesktop.org',
  'PACKAGE_NAME': meson.project_name(),
  'PACKAGE_STRING': '@0@ @1@'.format(meson.project_name(), qmi_version),
  'PACKAGE_TARNAME': meson.project_name(),
  'PACKAGE_URL': '',
  'PACKAGE_VERSION': qmi_version,
}

gtkdocentities_ent = configure_file(
  input: 'gtkdocentities.ent.in',
  output: '@BASENAME@',
  configuration: ent_conf,
)
