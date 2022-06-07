/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
 *
 * Copyright (C) 2013 - 2021 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <config.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <pwd.h>

#include "mbim-helpers.h"
#include "mbim-error-types.h"

/*****************************************************************************/

gboolean
mbim_helpers_check_user_allowed (uid_t    uid,
                                 GError **error)
{
#ifndef MBIM_USERNAME_ENABLED
    if (uid == 0)
        return TRUE;
#else
# ifndef MBIM_USERNAME
#  error MBIM username not defined
# endif

    struct passwd *expected_usr = NULL;

    /* Root user is always allowed, regardless of the specified MBIM_USERNAME */
    if (uid == 0)
        return TRUE;

    expected_usr = getpwnam (MBIM_USERNAME);
    if (!expected_usr) {
        g_set_error (error,
                     MBIM_CORE_ERROR,
                     MBIM_CORE_ERROR_FAILED,
                     "Not enough privileges (unknown username %s)", MBIM_USERNAME);
        return FALSE;
    }

    if (uid == expected_usr->pw_uid)
        return TRUE;
#endif

    g_set_error (error,
                 MBIM_CORE_ERROR,
                 MBIM_CORE_ERROR_FAILED,
                 "Not enough privileges");
    return FALSE;
}

/*****************************************************************************/

gchar *
mbim_helpers_get_devpath (const gchar  *cdc_wdm_path,
                          GError      **error)
{
    gchar *aux;

    if (!g_file_test (cdc_wdm_path, G_FILE_TEST_IS_SYMLINK))
        return g_strdup (cdc_wdm_path);

    aux = realpath (cdc_wdm_path, NULL);
    if (!aux) {
        int saved_errno = errno;

        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "Couldn't get realpath: %s", g_strerror (saved_errno));
        return NULL;
    }

    return aux;
}

/*****************************************************************************/

gchar *
mbim_helpers_get_devname (const gchar  *cdc_wdm_path,
                          GError      **error)
{
    gchar *aux;
    gchar *devname = NULL;

    aux = mbim_helpers_get_devpath (cdc_wdm_path, error);
    if (aux) {
        devname = g_path_get_basename (aux);
        g_free (aux);
    }

    return devname;
}

/*****************************************************************************/

gboolean
mbim_helpers_list_links_wdm (GFile         *sysfs_file,
                             GCancellable  *cancellable,
                             GPtrArray     *previous_links,
                             GPtrArray    **out_links,
                             GError       **error)
{
    g_autofree gchar           *sysfs_path = NULL;
    g_autoptr(GFileEnumerator)  direnum = NULL;
    g_autoptr(GPtrArray)        links = NULL;

    direnum = g_file_enumerate_children (sysfs_file,
                                         G_FILE_ATTRIBUTE_STANDARD_NAME,
                                         G_FILE_QUERY_INFO_NONE,
                                         cancellable,
                                         error);
    if (!direnum)
        return FALSE;

    sysfs_path = g_file_get_path (sysfs_file);
    links = g_ptr_array_new_with_free_func (g_free);

    while (TRUE) {
        GFileInfo        *info;
        g_autofree gchar *filename = NULL;
        g_autofree gchar *link_path = NULL;
        g_autofree gchar *real_path = NULL;
        g_autofree gchar *basename = NULL;

        if (!g_file_enumerator_iterate (direnum, &info, NULL, cancellable, error))
            return FALSE;
        if (!info)
            break;

        filename = g_file_info_get_attribute_as_string (info, G_FILE_ATTRIBUTE_STANDARD_NAME);
        if (!filename || !g_str_has_prefix (filename, "upper_"))
            continue;

        link_path = g_strdup_printf ("%s/%s", sysfs_path, filename);
        real_path = realpath (link_path, NULL);
        if (!real_path)
            continue;

        basename = g_path_get_basename (real_path);

        /* skip interface if it was already known */
        if (previous_links && g_ptr_array_find_with_equal_func (previous_links, basename, g_str_equal, NULL))
            continue;

        g_ptr_array_add (links, g_steal_pointer (&basename));
    }

    if (!links || !links->len) {
        *out_links = NULL;
        return TRUE;
    }

    g_ptr_array_sort (links, (GCompareFunc) g_ascii_strcasecmp);
    *out_links = g_steal_pointer (&links);
    return TRUE;
}

#if !GLIB_CHECK_VERSION(2,54,0)

gboolean
mbim_ptr_array_find_with_equal_func (GPtrArray     *haystack,
                                     gconstpointer  needle,
                                     GEqualFunc     equal_func,
                                     guint         *index_)
{
  guint i;

  g_return_val_if_fail (haystack != NULL, FALSE);

  if (equal_func == NULL)
      equal_func = g_direct_equal;

  for (i = 0; i < haystack->len; i++) {
      if (equal_func (g_ptr_array_index (haystack, i), needle)) {
          if (index_ != NULL)
              *index_ = i;
          return TRUE;
      }
  }

  return FALSE;
}

#endif
