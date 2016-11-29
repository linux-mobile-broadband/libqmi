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
 * Copyright (C) 2016 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "qfu-image-factory.h"
#include "qfu-image.h"
#include "qfu-image-cwe.h"

QfuImage *
qfu_image_factory_build (GFile        *file,
                         GCancellable *cancellable,
                         GError       **error)
{
	gchar        *basename;
    QfuImageType  image_type;
    QfuImage     *image = NULL;

    g_assert (G_IS_FILE (file));
    basename = g_file_get_basename (file);

    /* guessing image type based on the well known Gobi 1k and 2k
     * filenames, and assumes anything else is a CWE image
     *
     * This is based on the types in gobi-loader's snooped magic strings:
     *   0x05 => "amss.mbn"
     *   0x06 => "apps.mbn"
     *   0x0d => "uqcn.mbn" (Gobi 2000 only)
     */
    if (g_ascii_strcasecmp (basename, "amss.mbn") == 0)
        image_type = QFU_IMAGE_TYPE_AMSS_MODEM;
    else if (g_ascii_strcasecmp (basename, "apps.mbn") == 0)
        image_type = QFU_IMAGE_TYPE_AMSS_APPLICATION;
    else if (g_ascii_strcasecmp (basename, "uqcn.mbn") == 0)
        image_type = QFU_IMAGE_TYPE_AMSS_UQCN;
    else
        /* Note: should check validity of the image as this is the one used as
         * default fallback */
        image_type = QFU_IMAGE_TYPE_CWE;

    switch (image_type) {
    case QFU_IMAGE_TYPE_AMSS_MODEM:
    case QFU_IMAGE_TYPE_AMSS_APPLICATION:
    case QFU_IMAGE_TYPE_AMSS_UQCN:
        image = qfu_image_new (file, image_type, cancellable, error);
        break;
    case QFU_IMAGE_TYPE_CWE:
        image = qfu_image_cwe_new (file, cancellable, error);
        break;
    default:
        g_assert_not_reached ();
    }

    g_free (basename);
    return image;
}
