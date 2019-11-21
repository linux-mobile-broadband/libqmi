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
 * Copyright (C) 2016-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <glib-object.h>
#include <gio/gio.h>

#include "qfu-operation.h"
#include "qfu-image.h"
#include "qfu-image-cwe.h"
#include "qfu-image-factory.h"
#include "qfu-enum-types.h"

#define VALIDATE_STR_NA(str) (str && str[0] ? str : "n/a")

static void
print_image_cwe (QfuImageCwe *image,
                 const gchar *indent_prefix,
                 const gchar *id_str,
                 guint        idx)
{
    guint i;
    guint j;

    g_print ("%s-------------------------------------\n", indent_prefix);
    g_print ("%s[cwe %s] type:    %s\n",
             indent_prefix, id_str, VALIDATE_STR_NA (qfu_image_cwe_embedded_header_get_type (image, idx)));
    g_print ("%s[cwe %s] product: %s\n",
             indent_prefix, id_str, VALIDATE_STR_NA (qfu_image_cwe_embedded_header_get_product (image, idx)));
    g_print ("%s[cwe %s] version: %s\n",
             indent_prefix, id_str, VALIDATE_STR_NA (qfu_image_cwe_embedded_header_get_version (image, idx)));
    g_print ("%s[cwe %s] date:    %s\n",
             indent_prefix, id_str, VALIDATE_STR_NA (qfu_image_cwe_embedded_header_get_date (image, idx)));
    g_print ("%s[cwe %s] size:    %" G_GUINT32_FORMAT "\n",
             indent_prefix, id_str, qfu_image_cwe_embedded_header_get_image_size (image, idx));

    for (i = idx + 1, j = 0; i < qfu_image_cwe_get_n_embedded_headers (image); i++) {
        gchar *sub_id_str;
        gchar *sub_indent_prefix;

        /* As soon as we find one which doesn't have the expected parent index, break */
        if (qfu_image_cwe_embedded_header_get_parent_index (image, i) != (gint) idx)
            continue;

        sub_id_str = g_strdup_printf ("%s.%u", id_str, j++);
        sub_indent_prefix = g_strdup_printf ("%s    ", indent_prefix);

        print_image_cwe (image, sub_indent_prefix, sub_id_str, i);

        g_free (sub_id_str);
        g_free (sub_indent_prefix);
    }
}

static gboolean
operation_verify_run_single (const gchar *image_path)
{
    QfuImage *image;
    GFile    *file;
    GError   *error = NULL;
    gboolean  result = FALSE;

    file = g_file_new_for_commandline_arg (image_path);
    image = qfu_image_factory_build (file, NULL, &error);
    if (!image) {
        g_printerr ("error: couldn't detect image type: %s\n", error->message);
        g_error_free (error);
        goto out;
    }

    g_print ("\n");
    g_print ("Firmware image:\n");
    g_print ("  filename:      %s\n", qfu_image_get_display_name (image));
    g_print ("  detected type: %s\n", qfu_image_type_get_string (qfu_image_get_image_type (image)));
    g_print ("  size:          %" G_GOFFSET_FORMAT " bytes\n", qfu_image_get_size (image));
    g_print ("    header:      %" G_GOFFSET_FORMAT " bytes\n", qfu_image_get_header_size (image));
    g_print ("    data:        %" G_GOFFSET_FORMAT " bytes\n", qfu_image_get_data_size (image));
    g_print ("  data chunks:   %" G_GUINT16_FORMAT " (%lu bytes/chunk)\n", qfu_image_get_n_data_chunks (image), (gulong) QFU_IMAGE_CHUNK_SIZE);

    if (QFU_IS_IMAGE_CWE (image)) {
        QfuImageCwe *image_cwe = QFU_IMAGE_CWE (image);

        g_print ("  [cwe] detected firmware version: %s\n",
                 VALIDATE_STR_NA (qfu_image_cwe_get_parsed_firmware_version (image_cwe)));
        g_print ("  [cwe] detected config version:   %s\n",
                 VALIDATE_STR_NA (qfu_image_cwe_get_parsed_config_version (image_cwe)));
        g_print ("  [cwe] detected carrier:          %s\n",
                 VALIDATE_STR_NA (qfu_image_cwe_get_parsed_carrier (image_cwe)));

        print_image_cwe (image_cwe, "  ", "0", 0);
    }

    result = TRUE;

out:
    if (image)
        g_object_unref (image);
    return result;
}

gboolean
qfu_operation_verify_run (const gchar **images)
{
    guint invalid_images = 0;
    guint i;

    for (i = 0; images[i] ; i++)
        invalid_images += !operation_verify_run_single (images[i]);

    return !invalid_images;
}
