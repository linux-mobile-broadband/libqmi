/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * mbimcli -- Command line interface to control MBIM devices
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
 * Copyright (C) 2021 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gio/gio.h>

#include <libmbim-glib.h>

#include "mbimcli.h"
#include "mbimcli-helpers.h"

/* Options */
static gchar *link_list_str;
static gchar *link_add_str;
static gchar *link_delete_str;
static gchar *link_delete_all_str;

static GOptionEntry entries[] = {
    { "link-list", 0, 0, G_OPTION_ARG_STRING, &link_list_str,
      "List links created from a given interface",
      "[IFACE]"
    },
    { "link-add", 0, 0, G_OPTION_ARG_STRING, &link_add_str,
      "Create new network interface link",
      "[iface=IFACE,prefix=PREFIX[,session-id=N]]"
    },
    { "link-delete", 0, 0, G_OPTION_ARG_STRING, &link_delete_str,
      "Delete a given network interface link",
      "IFACE"
    },
    { "link-delete-all", 0, 0, G_OPTION_ARG_STRING, &link_delete_all_str,
      "Delete all network interface links from the given interface",
      "[IFACE]"
    },
    { NULL }
};

GOptionGroup *
mbimcli_link_management_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("link-management",
                                "Link management options:",
                                "Show link management specific options",
                                NULL, NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
mbimcli_link_management_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (!!link_list_str +
                 !!link_add_str +
                 !!link_delete_str +
                 !!link_delete_all_str);

    if (n_actions > 1) {
        g_printerr ("error: too many link management actions requested\n");
        exit (EXIT_FAILURE);
    }

    checked = TRUE;
    return !!n_actions;
}

/******************************************************************************/

static void
link_delete_all_ready (MbimDevice   *dev,
                       GAsyncResult *res)
{
    g_autoptr(GError) error = NULL;

    if (!mbim_device_delete_all_links_finish (dev, res, &error))
        g_printerr ("error: couldn't delete all links: %s\n", error->message);
    else
        g_print ("[%s] all links successfully deleted\n",
                 mbim_device_get_path_display (dev));

    mbimcli_async_operation_done (!error);
}

static void
device_link_delete_all (MbimDevice   *dev,
                        GCancellable *cancellable,
                        const gchar  *iface)
{
    mbim_device_delete_all_links (dev,
                                  iface,
                                  cancellable,
                                  (GAsyncReadyCallback)link_delete_all_ready,
                                  NULL);
}

/******************************************************************************/

static void
link_delete_ready (MbimDevice   *dev,
                   GAsyncResult *res)
{
    g_autoptr(GError) error = NULL;

    if (!mbim_device_delete_link_finish (dev, res, &error))
        g_printerr ("error: couldn't delete link: %s\n",
                    error->message);
    else
        g_print ("[%s] link successfully deleted\n",
                 mbim_device_get_path_display (dev));

    mbimcli_async_operation_done (!error);
}

static void
device_link_delete (MbimDevice   *dev,
                    GCancellable *cancellable,
                    const gchar  *link_iface)
{
    mbim_device_delete_link (dev,
                             link_iface,
                             cancellable,
                             (GAsyncReadyCallback)link_delete_ready,
                             NULL);
}

typedef struct {
    guint  session_id;
    gchar *iface;
    gchar *prefix;
} AddLinkProperties;

static void
link_add_ready (MbimDevice   *dev,
                GAsyncResult *res)
{
    g_autoptr(GError)  error = NULL;
    g_autofree gchar  *link_iface = NULL;
    guint              session_id;

    link_iface = mbim_device_add_link_finish (dev, res, &session_id, &error);
    if (!link_iface)
        g_printerr ("error: couldn't add link: %s\n",
                    error->message);
    else
        g_print ("[%s] link successfully added:\n"
                 "  iface name: %s\n"
                 "  session id: %u\n",
                 mbim_device_get_path_display (dev),
                 link_iface,
                 session_id);

    mbimcli_async_operation_done (!error);
}

static gboolean
add_link_properties_handle (const gchar        *key,
                            const gchar        *value,
                            GError            **error,
                            AddLinkProperties  *props)
{
    if (g_ascii_strcasecmp (key, "session-id") == 0 && props->session_id == MBIM_DEVICE_SESSION_ID_AUTOMATIC) {
        if (!mbimcli_read_uint_from_string (value, &props->session_id)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                         "invalid session-id given: '%s'", value);
            return FALSE;
        }
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "iface") == 0 && !props->iface) {
        props->iface = g_strdup (value);
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "prefix") == 0 && !props->prefix) {
        props->prefix = g_strdup (value);
        return TRUE;
    }

    g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                 "unrecognized or duplicate option '%s'", key);
    return FALSE;
}

static void
device_link_add (MbimDevice   *dev,
                 GCancellable *cancellable,
                 const gchar  *add_settings)
{
    g_autoptr(GError) error = NULL;
    AddLinkProperties props = {
        .session_id = MBIM_DEVICE_SESSION_ID_AUTOMATIC,
        .iface = NULL,
        .prefix = NULL,
    };

    if (!mbimcli_parse_key_value_string (add_settings,
                                         &error,
                                         (MbimParseKeyValueForeachFn)add_link_properties_handle,
                                         &props)) {
        g_printerr ("error: couldn't parse input add link settings: %s\n",
                    error->message);
        mbimcli_async_operation_done (FALSE);
        return;
    }

    if (!props.iface) {
        g_printerr ("error: missing mandatory 'iface' setting\n");
        mbimcli_async_operation_done (FALSE);
        return;
    }

    if (!props.prefix)
        props.prefix = g_strdup_printf ("%s.", props.iface);

    if ((props.session_id != MBIM_DEVICE_SESSION_ID_AUTOMATIC) &&
        (props.session_id > MBIM_DEVICE_SESSION_ID_MAX)) {
        g_printerr ("error: session id %u out of range [%u,%u]\n",
                    props.session_id, MBIM_DEVICE_SESSION_ID_MIN, MBIM_DEVICE_SESSION_ID_MAX);
        mbimcli_async_operation_done (FALSE);
        return;
    }

    mbim_device_add_link (dev,
                          props.session_id,
                          props.iface,
                          props.prefix,
                          cancellable,
                          (GAsyncReadyCallback)link_add_ready,
                          NULL);

    g_free (props.iface);
    g_free (props.prefix);
}

static void
device_link_list (MbimDevice   *dev,
                  GCancellable *cancellable,
                  const gchar  *iface)
{
    g_autoptr(GError) error = NULL;
    g_autoptr(GPtrArray) links = NULL;

    if (!mbim_device_list_links (dev, iface, &links, &error))
        g_printerr ("error: couldn't list links: %s\n", error->message);
    else {
        guint i;
        guint n_links;

        n_links = (links ? links->len : 0);

        g_print ("[%s] found %u links%s\n",
                 mbim_device_get_path_display (dev),
                 n_links,
                 n_links > 0 ? ":" : "");
        for (i = 0; i < n_links; i++)
            g_print ("  [%u] %s\n", i, (const gchar *) g_ptr_array_index (links, i));
    }

    mbimcli_async_operation_done (!error);
}

/******************************************************************************/
/* Common */

void
mbimcli_link_management_run (MbimDevice   *dev,
                             GCancellable *cancellable)
{
    if (link_list_str)
        device_link_list (dev, cancellable, link_list_str);
    else if (link_add_str)
        device_link_add (dev, cancellable, link_add_str);
    else if (link_delete_str)
        device_link_delete (dev, cancellable, link_delete_str);
    else if (link_delete_all_str)
        device_link_delete_all (dev, cancellable, link_delete_all_str);
    else
      g_warn_if_reached ();
}
