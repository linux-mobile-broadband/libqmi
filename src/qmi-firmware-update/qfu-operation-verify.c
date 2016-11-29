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
 * Copyright (C) 2016 Zodiac Inflight Innovations
 * Copyright (C) 2016 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <glib-object.h>
#include <gio/gio.h>

#include "qfu-operation.h"
#include "qfu-image.h"
#include "qfu-image-cwe.h"
#include "qfu-image-factory.h"
#include "qfu-enum-types.h"

gboolean
qfu_operation_verify_run (const gchar *image_path)
{
    QfuImage *image;
    GFile    *file;
    GError   *error = NULL;
    gboolean  result = FALSE;

    file = g_file_new_for_commandline_arg (image_path);
    image = qfu_image_factory_build (file, NULL, &error);
    if (!image) {
        g_printerr ("error: couldn't detect image type: %s",
                    error->message);
        g_error_free (error);
        goto out;
    }

    g_print ("Firmware image:\n");
    g_print ("  filename:      %s\n", qfu_image_get_display_name (image));
    g_print ("  detected type: %s\n", qfu_image_type_get_string (qfu_image_get_image_type (image)));
    g_print ("  size:          %" G_GOFFSET_FORMAT " bytes\n", qfu_image_get_size (image));
    g_print ("    header:      %" G_GOFFSET_FORMAT " bytes\n", qfu_image_get_header_size (image));
    g_print ("    data:        %" G_GOFFSET_FORMAT " bytes\n", qfu_image_get_data_size (image));
    g_print ("  data chunks:   %" G_GUINT16_FORMAT " (%lu bytes/chunk)\n", qfu_image_get_n_data_chunks (image), (gulong) QFU_IMAGE_CHUNK_SIZE);

    if (QFU_IS_IMAGE_CWE (image)) {
        g_print ("  [cwe] type:    %s\n", qfu_image_cwe_header_get_type    (QFU_IMAGE_CWE (image)));
        g_print ("  [cwe] product: %s\n", qfu_image_cwe_header_get_product (QFU_IMAGE_CWE (image)));
        g_print ("  [cwe] version: %s\n", qfu_image_cwe_header_get_version (QFU_IMAGE_CWE (image)));
        g_print ("  [cwe] date:    %s\n", qfu_image_cwe_header_get_date    (QFU_IMAGE_CWE (image)));
    }

    result = TRUE;

out:
    if (image)
        g_object_unref (image);
    return result;
}
