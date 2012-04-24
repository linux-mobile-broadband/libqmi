/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * libqmi-glib -- GLib/GIO based library to control QMI devices
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 * Copyright (C) 2012 Aleksander Morgado <aleksander@lanedo.com>
 */

#ifndef _LIBQMI_GLIB_QMI_CLIENT_CTL_H_
#define _LIBQMI_GLIB_QMI_CLIENT_CTL_H_

#include <glib-object.h>

#include "qmi-client.h"
#include "qmi-message-ctl.h"

G_BEGIN_DECLS

#define QMI_TYPE_CLIENT_CTL            (qmi_client_ctl_get_type ())
#define QMI_CLIENT_CTL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QMI_TYPE_CLIENT_CTL, QmiClientCtl))
#define QMI_CLIENT_CTL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QMI_TYPE_CLIENT_CTL, QmiClientCtlClass))
#define QMI_IS_CLIENT_CTL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QMI_TYPE_CLIENT_CTL))
#define QMI_IS_CLIENT_CTL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QMI_TYPE_CLIENT_CTL))
#define QMI_CLIENT_CTL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QMI_TYPE_CLIENT_CTL, QmiClientCtlClass))

typedef struct _QmiClientCtl QmiClientCtl;
typedef struct _QmiClientCtlClass QmiClientCtlClass;

struct _QmiClientCtl {
    QmiClient parent;
    gpointer priv_unused;
};

struct _QmiClientCtlClass {
    QmiClientClass parent;
};

GType qmi_client_ctl_get_type (void);

/* Version info */
void    qmi_client_ctl_get_version_info        (QmiClientCtl *self,
                                                guint timeout,
                                                GCancellable *cancellable,
                                                GAsyncReadyCallback callback,
                                                gpointer user_data);
GArray *qmi_client_ctl_get_version_info_finish (QmiClientCtl *self,
                                                GAsyncResult *res,
                                                GError **error);

/* Allocate CID */
void   qmi_client_ctl_allocate_cid        (QmiClientCtl *self,
                                           QmiService service,
                                           guint timeout,
                                           GCancellable *cancellable,
                                           GAsyncReadyCallback callback,
                                           gpointer user_data);
guint8 qmi_client_ctl_allocate_cid_finish (QmiClientCtl *self,
                                           GAsyncResult *res,
                                           GError **error);

/* Release CID */
void   qmi_client_ctl_release_cid          (QmiClientCtl *self,
                                            QmiService service,
                                            guint8 cid,
                                            guint timeout,
                                            GCancellable *cancellable,
                                            GAsyncReadyCallback callback,
                                            gpointer user_data);
gboolean qmi_client_ctl_release_cid_finish (QmiClientCtl *self,
                                            GAsyncResult *res,
                                            GError **error);

/* Sync */
void     qmi_client_ctl_sync        (QmiClientCtl *self,
                                     guint timeout,
                                     GCancellable *cancellable,
                                     GAsyncReadyCallback callback,
                                     gpointer user_data);
gboolean qmi_client_ctl_sync_finish (QmiClientCtl *self,
                                     GAsyncResult *res,
                                     GError **error);

G_END_DECLS

#endif /* _LIBQMI_GLIB_QMI_CLIENT_CTL_H_ */
