# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2021 Iñigo Martinez <inigomartinez@gmail.com>

ent_conf = configuration_data()
ent_conf.set('PACKAGE', mbim_name)
ent_conf.set('PACKAGE_BUGREPORT', mbim_name + '-devel@lists.freedesktop.org')
ent_conf.set('PACKAGE_NAME', mbim_name)
ent_conf.set('PACKAGE_STRING', '@0@ @1@'.format(mbim_name, mbim_version))
ent_conf.set('PACKAGE_TARNAME', mbim_name)
ent_conf.set('PACKAGE_URL', '')
ent_conf.set('PACKAGE_VERSION', mbim_version)

gtkdocentities_ent = configure_file(
  input: 'gtkdocentities.ent.in',
  output: '@BASENAME@',
  configuration: ent_conf,
)
