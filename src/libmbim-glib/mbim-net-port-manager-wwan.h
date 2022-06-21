/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
 *
 * Copyright (C) 2022 Daniele Palmas <dnlplm@gmail.com>
 *
 * Based on previous work:
 *   Copyright (C) 2020-2021 Eric Caruso <ejcaruso@chromium.org>
 *   Copyright (C) 2020-2021 Andrew Lassalle <andrewlassalle@chromium.org>
 *   Copyright (C) 2021 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBMBIM_GLIB_MBIM_NET_PORT_MANAGER_WWAN_H_
#define _LIBMBIM_GLIB_MBIM_NET_PORT_MANAGER_WWAN_H_

#include <gio/gio.h>
#include <glib-object.h>

#define MBIM_TYPE_NET_PORT_MANAGER_WWAN            (mbim_net_port_manager_wwan_get_type ())
#define MBIM_NET_PORT_MANAGER_WWAN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MBIM_TYPE_NET_PORT_MANAGER_WWAN, MbimNetPortManagerWwan))
#define MBIM_NET_PORT_MANAGER_WWAN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  MBIM_TYPE_NET_PORT_MANAGER_WWAN, MbimNetPortManagerWwanClass))
#define MBIM_IS_NET_PORT_MANAGER_WWAN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MBIM_TYPE_NET_PORT_MANAGER_WWAN))
#define MBIM_IS_NET_PORT_MANAGER_WWAN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  MBIM_TYPE_NET_PORT_MANAGER_WWAN))
#define MBIM_NET_PORT_MANAGER_WWAN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  MBIM_TYPE_NET_PORT_MANAGER_WWAN, MbimNetPortManagerWwanClass))

typedef struct _MbimNetPortManagerWwan      MbimNetPortManagerWwan;
typedef struct _MbimNetPortManagerWwanClass MbimNetPortManagerWwanClass;

struct _MbimNetPortManagerWwan {
    MbimNetPortManager parent;
};

struct _MbimNetPortManagerWwanClass {
    MbimNetPortManagerClass parent;
};

GType mbim_net_port_manager_wwan_get_type (void);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (MbimNetPortManagerWwan, g_object_unref)

MbimNetPortManagerWwan *mbim_net_port_manager_wwan_new (GError **error);

#endif /* _LIBMBIM_GLIB_MBIM_NET_PORT_MANAGER_WWAN_H_ */
