# SPDX-License-Identifier: GPL-2
# Copyright (C) 2021 Iñigo Martinez <inigomartinez@gmail.com>

mans = [
  ['mbimcli', [help2man, '--output=@OUTPUT@', '--name=Control MBIM devices', '--help-option="--help-all"', mbimcli], [mbimcli_sources]],
  ['mbim-network', [help2man, '--output=@OUTPUT@', '--name=Simple network management of MBIM devices', mbim_network], []],
]

foreach man: mans
  custom_target(
    man[0],
    output: man[0] + '.1',
    command: man[1],
    depend_files: man[2],
    install: true,
    install_dir: join_paths(mbim_mandir, 'man1'),
  )
endforeach
