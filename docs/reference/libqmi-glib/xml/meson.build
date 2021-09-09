# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2019 - 2021 Iñigo Martinez <inigomartinez@gmail.com>

ent_conf = {
  'PACKAGE': qmi_name,
  'PACKAGE_BUGREPORT': qmi_name + '-devel@lists.freedesktop.org',
  'PACKAGE_NAME': qmi_name,
  'PACKAGE_STRING': '@0@ @1@'.format(qmi_name, qmi_version),
  'PACKAGE_TARNAME': qmi_name,
  'PACKAGE_URL': '',
  'PACKAGE_VERSION': qmi_version,
}

gtkdocentities_ent = configure_file(
  input: 'gtkdocentities.ent.in',
  output: '@BASENAME@',
  configuration: ent_conf,
)
