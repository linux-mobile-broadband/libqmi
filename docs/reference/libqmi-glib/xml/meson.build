# SPDX-License-Identifier: GPL-2
# Copyright (C) 2019 - 2021 Iñigo Martinez <inigomartinez@gmail.com>

ent_conf = configuration_data()
ent_conf.set('PACKAGE', meson.project_name())
ent_conf.set('PACKAGE_BUGREPORT', meson.project_name() + '-devel@lists.freedesktop.org')
ent_conf.set('PACKAGE_NAME', meson.project_name())
ent_conf.set('PACKAGE_STRING', '@0@ @1@'.format(meson.project_name(), qmi_version))
ent_conf.set('PACKAGE_TARNAME', meson.project_name())
ent_conf.set('PACKAGE_URL', '')
ent_conf.set('PACKAGE_VERSION', qmi_version)

gtkdocentities_ent = configure_file(
  input: 'gtkdocentities.ent.in',
  output: '@BASENAME@',
  configuration: ent_conf,
)
