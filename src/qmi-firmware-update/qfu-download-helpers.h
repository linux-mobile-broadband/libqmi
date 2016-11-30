/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * qmi-firmware-update -- Command line tool to update firmware in QMI devices
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2016 Bjørn Mork <bjorn@mork.no>
 * Copyright (C) 2016 Aleksander Morgado <aleksander@aleksander.es>
 *
 * Additional copyrights:
 *
 *    crc16 and hdlc parts:
 *      Copyright (C) 2010 Red Hat, Inc.
 *
 *    parts of this is based on gobi-loader, which is
 *     "Copyright 2009 Red Hat <mjg@redhat.com> - heavily based on work done by
 *      Alexander Shumakovitch <shurik@gwu.edu>
 *
 *    gobi 2000 support provided by Anssi Hannula <anssi.hannula@iki.fi>
 */

#ifndef QFU_DOWNLOAD_HELPERS_H
#define QFU_DOWNLOAD_HELPERS_H

#include <gio/gio.h>
#include "qfu-image.h"

G_BEGIN_DECLS

void     qfu_download_helper_run        (GFile                *tty,
                                         QfuImage             *image,
                                         GCancellable         *cancellable,
                                         GAsyncReadyCallback   callback,
                                         gpointer              user_data);
gboolean qfu_download_helper_run_finish (GAsyncResult         *res,
                                         GError              **error);

G_END_DECLS

#endif /* QFU_DOWNLOAD_HELPERS_H */
