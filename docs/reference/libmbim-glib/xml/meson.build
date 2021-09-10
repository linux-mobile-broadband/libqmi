# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2021 Iñigo Martinez <inigomartinez@gmail.com>

ent_conf = {
  'PACKAGE': mbim_name,
  'PACKAGE_BUGREPORT': mbim_name + '-devel@lists.freedesktop.org',
  'PACKAGE_NAME': mbim_name,
  'PACKAGE_STRING': '@0@ @1@'.format(mbim_name, mbim_version),
  'PACKAGE_TARNAME': mbim_name,
  'PACKAGE_URL': '',
  'PACKAGE_VERSION': mbim_version,
}

gtkdocentities_ent = configure_file(
  input: 'gtkdocentities.ent.in',
  output: '@BASENAME@',
  configuration: ent_conf,
)
