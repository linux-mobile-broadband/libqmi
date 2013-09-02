/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * qmicli -- Command line interface to control QMI devices
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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
 * Copyright (C) 2012 Aleksander Morgado <aleksander@gnu.org>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#include <libqmi-glib.h>

#include "qmicli.h"
#include "qmicli-helpers.h"

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientUim *client;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gboolean read_efspn_flag;
static gboolean read_efimsi_flag;
static gboolean read_eficcid_flag;
static gboolean reset_flag;
static gboolean noop_flag;

static GOptionEntry entries[] = {
    { "uim-read-efspn", 0, 0, G_OPTION_ARG_NONE, &read_efspn_flag,
      "Read the EFspn file",
      NULL
    },
    { "uim-read-efimsi", 0, 0, G_OPTION_ARG_NONE, &read_efimsi_flag,
      "Read the EFimsi file",
      NULL
    },
    { "uim-read-eficcid", 0, 0, G_OPTION_ARG_NONE, &read_eficcid_flag,
      "Read the EFiccid file",
      NULL
    },
    { "uim-reset", 0, 0, G_OPTION_ARG_NONE, &reset_flag,
      "Reset the service state",
      NULL
    },
    { "uim-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a UIM client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL }
};

GOptionGroup *
qmicli_uim_get_option_group (void)
{
	GOptionGroup *group;

	group = g_option_group_new ("uim",
	                            "UIM options",
	                            "Show User Identity Module options",
	                            NULL,
	                            NULL);
	g_option_group_add_entries (group, entries);

	return group;
}

gboolean
qmicli_uim_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (read_efspn_flag +
                 read_efimsi_flag +
                 read_eficcid_flag +
                 reset_flag +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many UIM actions requested\n");
        exit (EXIT_FAILURE);
    }

    checked = TRUE;
    return !!n_actions;
}

static void
context_free (Context *context)
{
    if (!context)
        return;

    if (context->client)
        g_object_unref (context->client);
    g_object_unref (context->cancellable);
    g_object_unref (context->device);
    g_slice_free (Context, context);
}

static void
shutdown (gboolean operation_status)
{
    /* Cleanup context and finish async operation */
    context_free (ctx);
    qmicli_async_operation_done (operation_status);
}

static void
reset_ready (QmiClientUim *client,
             GAsyncResult *res)
{
    QmiMessageUimResetOutput *output;
    GError *error = NULL;

    output = qmi_client_uim_reset_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_reset_output_get_result (output, &error)) {
        g_printerr ("error: couldn't reset the UIM service: %s\n", error->message);
        g_error_free (error);
        qmi_message_uim_reset_output_unref (output);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully performed UIM service reset\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_uim_reset_output_unref (output);
    shutdown (TRUE);
}

static gboolean
noop_cb (gpointer unused)
{
    shutdown (TRUE);
    return FALSE;
}

typedef struct {
    gchar *name;
    guint16 path[3];
} SimFile;

static const SimFile sim_files[] = {
    { "EFspn",    { 0x3F00, 0x7F20, 0x6F46 } },
    { "EFimsi",   { 0x3F00, 0x7F20, 0x6F07 } },
    { "EFiccid",  { 0x3F00, 0x2FE2, 0x0000 } },
};

static void
read_transparent_ready (QmiClientUim *client,
                        GAsyncResult *res)
{
    QmiMessageUimReadTransparentOutput *output;
    GError *error = NULL;
    guint8 sw1 = 0;
    guint8 sw2 = 0;
    GArray *read_result = NULL;

    output = qmi_client_uim_read_transparent_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_read_transparent_output_get_result (output, &error)) {
        g_printerr ("error: couldn't read transparent file from the UIM: %s\n", error->message);
        g_error_free (error);

        /* Card result */
        if (qmi_message_uim_read_transparent_output_get_card_result (
                output,
                &sw1,
                &sw2,
                NULL)) {
            g_print ("Card result:\n"
                     "\tSW1: '0x%02x'\n"
                     "\tSW2: '0x%02x'\n",
                     sw1, sw2);
        }

        qmi_message_uim_read_transparent_output_unref (output);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully read information from the UIM:\n",
             qmi_device_get_path_display (ctx->device));

    /* Card result */
    if (qmi_message_uim_read_transparent_output_get_card_result (
            output,
            &sw1,
            &sw2,
            NULL)) {
        g_print ("Card result:\n"
                 "\tSW1: '0x%02x'\n"
                 "\tSW2: '0x%02x'\n",
                 sw1, sw2);
    }

    /* Read result */
    if (qmi_message_uim_read_transparent_output_get_read_result (
            output,
            &read_result,
            NULL)) {
        gchar *str;

        str = qmicli_get_raw_data_printable (read_result, 80, "\t");
        g_print ("Read result:\n"
                 "%s\n",
                 str);
        g_free (str);
    }

    qmi_message_uim_read_transparent_output_unref (output);
    shutdown (TRUE);
}

static QmiMessageUimReadTransparentInput *
read_transparent_build_input (const gchar *file_name)
{
    QmiMessageUimReadTransparentInput *input;
    guint16 file_id = 0;
    GArray *file_path = NULL;
    guint i;

    for (i = 0; i < G_N_ELEMENTS (sim_files); i++) {
        if (g_str_equal (sim_files[i].name, file_name))
            break;
    }

    g_assert (i != G_N_ELEMENTS (sim_files));

    file_path = g_array_sized_new (FALSE, FALSE, sizeof (guint16), 3);
    g_array_append_val (file_path, sim_files[i].path[0]);
    if (sim_files[i].path[2] != 0) {
        g_array_append_val (file_path, sim_files[i].path[1]);
        g_array_append_val (file_path, sim_files[i].path[2]);
        file_id = sim_files[i].path[2];
    } else {
        g_array_append_val (file_path, sim_files[i].path[1]);
        file_id = sim_files[i].path[1];
    }

    input = qmi_message_uim_read_transparent_input_new ();
    qmi_message_uim_read_transparent_input_set_session_information (
        input,
        QMI_UIM_SESSION_TYPE_PRIMARY_GW_PROVISIONING,
        "",
        NULL);
    qmi_message_uim_read_transparent_input_set_file (
        input,
        file_id,
        file_path,
        NULL);
    qmi_message_uim_read_transparent_input_set_read_information (input, 0, 0, NULL);
    g_array_unref (file_path);
    return input;
}

static void
get_file_attributes_ready (QmiClientUim *client,
                           GAsyncResult *res,
                           gchar *file_name)
{
    QmiMessageUimGetFileAttributesOutput *output;
    GError *error = NULL;
    guint8 sw1 = 0;
    guint8 sw2 = 0;
    guint16 file_size;
    guint16 file_id;
    QmiUimFileType file_type;
    guint16 record_size;
    guint16 record_count;
    QmiUimSecurityAttributeLogic read_security_attributes_logic;
    QmiUimSecurityAttribute read_security_attributes;
    QmiUimSecurityAttributeLogic write_security_attributes_logic;
    QmiUimSecurityAttribute write_security_attributes;
    QmiUimSecurityAttributeLogic increase_security_attributes_logic;
    QmiUimSecurityAttribute increase_security_attributes;
    QmiUimSecurityAttributeLogic deactivate_security_attributes_logic;
    QmiUimSecurityAttribute deactivate_security_attributes;
    QmiUimSecurityAttributeLogic activate_security_attributes_logic;
    QmiUimSecurityAttribute activate_security_attributes;
    GArray *raw = NULL;
    QmiMessageUimReadTransparentInput *input;

    output = qmi_client_uim_get_file_attributes_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        g_free (file_name);
        return;
    }

    if (!qmi_message_uim_get_file_attributes_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get '%s' file attributes from the UIM: %s\n",
                    file_name,
                    error->message);
        g_error_free (error);

        /* Card result */
        if (qmi_message_uim_get_file_attributes_output_get_card_result (
                output,
                &sw1,
                &sw2,
                NULL)) {
            g_print ("Card result:\n"
                     "\tSW1: '0x%02x'\n"
                     "\tSW2: '0x%02x'\n",
                     sw1, sw2);
        }

        qmi_message_uim_get_file_attributes_output_unref (output);
        shutdown (FALSE);
        g_free (file_name);
        return;
    }

    g_print ("[%s] Successfully got file '%s' attributes from the UIM:\n",
             file_name,
             qmi_device_get_path_display (ctx->device));

    /* Card result */
    if (qmi_message_uim_get_file_attributes_output_get_card_result (
            output,
            &sw1,
            &sw2,
            NULL)) {
        g_print ("Card result:\n"
                 "\tSW1: '0x%02x'\n"
                 "\tSW2: '0x%02x'\n",
                 sw1, sw2);
    }

    /* File attributes */
    if (qmi_message_uim_get_file_attributes_output_get_file_attributes (
            output,
            &file_size,
            &file_id,
            &file_type,
            &record_size,
            &record_count,
            &read_security_attributes_logic,
            &read_security_attributes,
            &write_security_attributes_logic,
            &write_security_attributes,
            &increase_security_attributes_logic,
            &increase_security_attributes,
            &deactivate_security_attributes_logic,
            &deactivate_security_attributes,
            &activate_security_attributes_logic,
            &activate_security_attributes,
            &raw,
            NULL)) {
        gchar *str;

        g_print ("File attributes:\n");
        g_print ("\tFile size: %u\n", (guint)file_size);
        g_print ("\tFile ID: %u\n", (guint)file_id);
        g_print ("\tFile type: %s\n", qmi_uim_file_type_get_string (file_type));
        g_print ("\tRecord size: %u\n", (guint)record_size);
        g_print ("\tRecord count: %u\n", (guint)record_count);

        str = qmi_uim_security_attribute_build_string_from_mask (read_security_attributes);
        g_print ("\tRead security attributes: (%s) %s\n",
                 qmi_uim_security_attribute_logic_get_string (read_security_attributes_logic),
                 str);
        g_free (str);

        str = qmi_uim_security_attribute_build_string_from_mask (write_security_attributes);
        g_print ("\tWrite security attributes: (%s) %s\n",
                 qmi_uim_security_attribute_logic_get_string (write_security_attributes_logic),
                 str);
        g_free (str);

        str = qmi_uim_security_attribute_build_string_from_mask (increase_security_attributes);
        g_print ("\tIncrease security attributes: (%s) %s\n",
                 qmi_uim_security_attribute_logic_get_string (increase_security_attributes_logic),
                 str);
        g_free (str);

        str = qmi_uim_security_attribute_build_string_from_mask (deactivate_security_attributes);
        g_print ("\tDeactivate security attributes: (%s) %s\n",
                 qmi_uim_security_attribute_logic_get_string (deactivate_security_attributes_logic),
                 str);
        g_free (str);

        str = qmi_uim_security_attribute_build_string_from_mask (activate_security_attributes);
        g_print ("\tActivate security attributes: (%s) %s\n",
                 qmi_uim_security_attribute_logic_get_string (activate_security_attributes_logic),
                 str);
        g_free (str);

        str = qmicli_get_raw_data_printable (raw, 80, "\t");
        g_print ("\tRaw: %s\n", str);
        g_free (str);
    }

    qmi_message_uim_get_file_attributes_output_unref (output);

    /* Now really read the record */
    input = read_transparent_build_input (file_name);
    g_debug ("Asynchronously reading %s...", file_name);
    qmi_client_uim_read_transparent (ctx->client,
                                     input,
                                     10,
                                     ctx->cancellable,
                                     (GAsyncReadyCallback)read_transparent_ready,
                                     NULL);
    qmi_message_uim_read_transparent_input_unref (input);
}

static QmiMessageUimGetFileAttributesInput *
get_file_attributes_build_input (const gchar *file_name)
{
    QmiMessageUimGetFileAttributesInput *input;
    guint16 file_id = 0;
    GArray *file_path = NULL;
    guint i;

    for (i = 0; i < G_N_ELEMENTS (sim_files); i++) {
        if (g_str_equal (sim_files[i].name, file_name))
            break;
    }

    g_assert (i != G_N_ELEMENTS (sim_files));

    file_path = g_array_sized_new (FALSE, FALSE, sizeof (guint16), 3);
    g_array_append_val (file_path, sim_files[i].path[0]);
    if (sim_files[i].path[2] != 0) {
        g_array_append_val (file_path, sim_files[i].path[1]);
        g_array_append_val (file_path, sim_files[i].path[2]);
        file_id = sim_files[i].path[2];
    } else {
        g_array_append_val (file_path, sim_files[i].path[1]);
        file_id = sim_files[i].path[1];
    }

    input = qmi_message_uim_get_file_attributes_input_new ();
    qmi_message_uim_get_file_attributes_input_set_session_information (
        input,
        QMI_UIM_SESSION_TYPE_PRIMARY_GW_PROVISIONING,
        "",
        NULL);
    qmi_message_uim_get_file_attributes_input_set_file (
        input,
        file_id,
        file_path,
        NULL);
    g_array_unref (file_path);
    return input;
}

static void
read_file (const gchar *file_name)
{
    QmiMessageUimGetFileAttributesInput *input;

    input = get_file_attributes_build_input (file_name);
    g_debug ("Asynchronously reading %s file attributes...", file_name);
    qmi_client_uim_get_file_attributes (ctx->client,
                                        input,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback)get_file_attributes_ready,
                                        g_strdup (file_name));
    qmi_message_uim_get_file_attributes_input_unref (input);
}

void
qmicli_uim_run (QmiDevice *device,
                QmiClientUim *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    ctx->cancellable = g_object_ref (cancellable);

    /* Request to read EFspn? */
    if (read_efspn_flag) {
        read_file ("EFspn");
        return;
    }

    /* Request to read EFimsi? */
    if (read_efimsi_flag) {
        read_file ("EFimsi");
        return;
    }

    /* Request to read EFiccid? */
    if (read_eficcid_flag) {
        read_file ("EFiccid");
        return;
    }

    /* Request to reset UIM service? */
    if (reset_flag) {
        g_debug ("Asynchronously resetting UIM service...");
        qmi_client_uim_reset (ctx->client,
                              NULL,
                              10,
                              ctx->cancellable,
                              (GAsyncReadyCallback)reset_ready,
                              NULL);
        return;
    }

    /* Just client allocate/release? */
    if (noop_flag) {
        g_idle_add (noop_cb, NULL);
        return;
    }

    g_warn_if_reached ();
}
