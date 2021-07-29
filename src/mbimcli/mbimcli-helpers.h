/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mbimcli -- Command line interface to control QMI devices
 *
 * Copyright (C) 2014 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <glib.h>

#include <libmbim-glib.h>

#ifndef __MBIMCLI_HELPERS_H__
#define __MBIMCLI_HELPERS_H__

gboolean mbimcli_read_uint_from_string (const gchar *str,
                                        guint *out);

gboolean mbimcli_print_ip_config (MbimDevice *device,
                                  MbimMessage *response,
                                  GError **error);

typedef gboolean (*MbimParseKeyValueForeachFn) (const gchar *key,
                                                const gchar *value,
                                                GError **error,
                                                gpointer user_data);

gboolean mbimcli_parse_key_value_string (const gchar *str,
                                         GError **error,
                                         MbimParseKeyValueForeachFn callback,
                                         gpointer user_data);

MbimPinType mbimcli_read_pintype_from_string (const gchar *str);

#endif /* __MBIMCLI_H__ */
