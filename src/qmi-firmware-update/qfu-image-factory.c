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
 * Copyright (C) 2016 Zodiac Inflight Innovations
 * Copyright (C) 2016-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "qfu-image-factory.h"
#include "qfu-image.h"
#include "qfu-image-cwe.h"

QfuImage *
qfu_image_factory_build (GFile        *file,
                         GCancellable *cancellable,
                         GError       **error)
{
    gchar    *basename;
    QfuImage *image = NULL;
    GError   *inner_error = NULL;

    g_assert (G_IS_FILE (file));
    basename = g_file_get_basename (file);

    /* guessing image type based on the well known Gobi 1k and 2k
     * filenames, and assumes anything else could be a CWE image
     *
     * This is based on the types in gobi-loader's snooped magic strings:
     *   0x05 => "amss.mbn"
     *   0x06 => "apps.mbn"
     *   0x0d => "uqcn.mbn" (Gobi 2000 only)
     */
    if (g_ascii_strcasecmp (basename, "amss.mbn") == 0)
        image = qfu_image_new (file, QFU_IMAGE_TYPE_AMSS_MODEM, cancellable, error);
    else if (g_ascii_strcasecmp (basename, "apps.mbn") == 0)
        image = qfu_image_new (file, QFU_IMAGE_TYPE_AMSS_APPLICATION, cancellable, error);
    else if (g_ascii_strcasecmp (basename, "uqcn.mbn") == 0)
        image = qfu_image_new (file, QFU_IMAGE_TYPE_AMSS_UQCN, cancellable, error);
    else {
        /* Try to autodetect format */

        /* Maybe a CWE image? */
        image = qfu_image_cwe_new (file, cancellable, &inner_error);
        if (!image) {
            g_debug ("[qfu-image-factory] not a CWE file: %s", inner_error->message);
            g_error_free (inner_error);

            g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                         "unknown firmware image file");
        }
    }

    g_free (basename);
    return image;
}
