/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details:
 *
 * Copyright (C) 2014 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "test-fixture.h"

#define VIRTUAL_SOCKET_PATH "virtual-socket-path"

static const QmiService services [] = {
#if defined HAVE_QMI_SERVICE_DMS
    QMI_SERVICE_DMS,
#endif
#if defined HAVE_QMI_SERVICE_NAS
    QMI_SERVICE_NAS,
#endif
#if defined HAVE_QMI_SERVICE_WDS
    QMI_SERVICE_WDS,
#endif
#if defined HAVE_QMI_SERVICE_PDS
    QMI_SERVICE_PDS
#endif
};

static void
device_allocate_client_ready (QmiDevice    *device,
                              GAsyncResult *res,
                              TestFixture  *fixture)
{
    GError    *error = NULL;
    QmiClient *client;
    QmiService service;

    client = qmi_device_allocate_client_finish (device, res, &error);
    g_assert_no_error (error);
    g_assert (QMI_IS_CLIENT (client));

    service = qmi_client_get_service (client);
    g_assert (service > QMI_SERVICE_CTL);
    fixture->service_info[service].client = client;
    fixture->service_info[service].transaction_id = 0x0001;
    test_fixture_loop_stop (fixture);
}

static void
device_open_ready (QmiDevice    *device,
                   GAsyncResult *res,
                   TestFixture  *fixture)
{
    GError   *error = NULL;
    gboolean  ret;

    ret = qmi_device_open_finish (device, res, &error);
    g_assert_no_error (error);
    g_assert (ret);
    test_fixture_loop_stop (fixture);
}

static void
device_virtual_new_ready (GObject      *source,
                          GAsyncResult *res,
                          TestFixture  *fixture)
{
    GError *error = NULL;

    fixture->device = qmi_device_new_finish (res, &error);
    g_assert_no_error (error);
    g_assert (QMI_IS_DEVICE (fixture->device));
    test_fixture_loop_stop (fixture);
}

void
test_fixture_setup (TestFixture *fixture)
{
    GFile *file;
    guint  i;
    static guint32 num = 0;

    g_debug ("[%lu,%p] fixture setup", (gulong) pthread_self (), g_main_context_get_thread_default ());

    qmi_utils_set_traces_enabled (TRUE);

    /* Create port name, and add process ID so that multiple runs of this test
     * in the same system don't clash with each other */
    fixture->path = g_strdup_printf ("/dev/qmi%08lu%04u", (gulong) getpid (), num++);
    fixture->service_info[QMI_SERVICE_CTL].transaction_id = 0x0001;
    fixture->ctx = test_port_context_new (fixture->path);
    test_port_context_start (fixture->ctx);

    /* Create device */
    file = g_file_new_for_path (fixture->path);
    g_async_initable_new_async (QMI_TYPE_DEVICE,
                                G_PRIORITY_DEFAULT,
                                NULL,
                                (GAsyncReadyCallback) device_virtual_new_ready,
                                fixture,
                                QMI_DEVICE_FILE,          file,
                                QMI_DEVICE_NO_FILE_CHECK, TRUE,
                                QMI_DEVICE_PROXY_PATH,    fixture->path,
                                NULL);
    g_object_unref (file);
    test_fixture_loop_run (fixture);

    /* Open device */
    {
        guint8 expected[] = {
            0x01, /* marker */
            /* QMUX */
            0x22, 0x00, /* length */
            0x00,       /* flags */
            0x00,       /* service CTL */
            0x00,       /* client */
            /* QMI header */
            0x00,       /* flags */
            0xFF,       /* transaction */
            0x00, 0xFF, /* message: Internal proxy open */
            0x17, 0x00, /* tlv length */
            /* TLV */
            0x01,       /* type */
            0x14, 0x00, /* length */
            0x2F, 0x64, 0x65, 0x76, 0x2F, 0x76, 0x69, 0x72, 0x74, 0x75, 0x61, 0x6C, 0x2F, 0x71, 0x6D, 0x69, 0x00, 0x00, 0x00, 0x00
        };
        guint8 response[] = {
            0x01, /* marker */
            /* QMUX */
            0x12, 0x00, /* length */
            0x00,       /* flags */
            0x00,       /* service CTL */
            0x00,       /* client */
            /* QMI header */
            0x01,       /* flags */
            0xFF,       /* transaction */
            0x00, 0xFF, /* message: Internal proxy open */
            0x07, 0x00, /* tlv length */
            /* TLV */
            0x02,       /* type: Result */
            0x04, 0x00, /* length */
            0x00, 0x00, /* error status */
            0x00, 0x00, /* error code */
        };

        g_assert_cmpuint (strlen (fixture->path), ==, 20);
        memcpy (&expected[15], fixture->path, strlen (fixture->path));

        test_port_context_set_command (fixture->ctx,
                                       expected, G_N_ELEMENTS (expected),
                                       response, G_N_ELEMENTS (response),
                                       fixture->service_info[QMI_SERVICE_CTL].transaction_id++);
    }
    qmi_device_open (fixture->device, QMI_DEVICE_OPEN_FLAGS_PROXY, 1, NULL,
                     (GAsyncReadyCallback) device_open_ready,
                     fixture);
    test_fixture_loop_run (fixture);

    /* Allocate clients */
    for (i = 0; i < G_N_ELEMENTS (services); i++) {
        guint8 expected[] = {
            0x01,       /* marker */
            /* QMUX */
            0x0F, 0x00, /* length */
            0x00,       /* flags */
            0x00,       /* service CTL */
            0x00,       /* client */
            /* QMI header */
            0x00,       /* flags */
            0xFF,       /* transaction */
            0x22, 0x00, /* message: Allocate CID */
            0x04, 0x00, /* tlv length */
            /* TLV */
            0x01,       /* type */
            0x01, 0x00, /* length */
            0xFF        /* UPDATE: service */
        };
        guint8 response[] = {
            0x01,       /* marker */
            /* QMUX */
            0x17, 0x00, /* length */
            0x00,       /* flags */
            0x00,       /* service */
            0x00,       /* client */
            /* QMI header */
            0x01,       /* flags: Response */
            0xFF,       /* transaction */
            0x22, 0x00, /* message */
            0x0C, 0x00, /* tlv length */
            /* TLV */
            0x02,       /* type: Result */
            0x04, 0x00, /* length */
            0x00, 0x00, /* error status */
            0x00, 0x00, /* error code */
            /* TLV */
            0x01,       /* type: Allocation info */
            0x02, 0x00, /* length */
            0xFF,       /* UPDATE: service */
            0x01,       /* cid: 1 */
        };

        expected[15] = services[i];
        response[22] = services[i];

        test_port_context_set_command (fixture->ctx,
                                       expected, G_N_ELEMENTS (expected),
                                       response, G_N_ELEMENTS (response),
                                       fixture->service_info[QMI_SERVICE_CTL].transaction_id++);
        qmi_device_allocate_client (fixture->device, services[i], QMI_CID_NONE, 10, NULL,
                                    (GAsyncReadyCallback) device_allocate_client_ready,
                                    fixture);
        test_fixture_loop_run (fixture);
    }
}

static void
device_release_client_ready (QmiDevice    *device,
                             GAsyncResult *res,
                             TestFixture  *fixture)
{
    GError *error = NULL;
    gboolean st;

    st = qmi_device_release_client_finish (device, res, &error);
    g_assert_no_error (error);
    g_assert (st);
    test_fixture_loop_stop (fixture);
}

static void
device_close_ready (QmiDevice    *device,
                    GAsyncResult *res,
                    TestFixture  *fixture)
{
    GError *error = NULL;
    gboolean st;

    st = qmi_device_close_finish (device, res, &error);
    g_assert_no_error (error);
    g_assert (st);
    test_fixture_loop_stop (fixture);
}

void
test_fixture_teardown (TestFixture *fixture)
{
    guint i;

    for (i = 0; i < G_N_ELEMENTS (services); i++) {
        guint8 expected[] = {
            0x01,       /* marker */
            /* QMUX */
            0x10, 0x00, /* length */
            0x00,       /* flags */
            0x00,       /* service CTL */
            0x00,       /* client */
            /* QMI header */
            0x00,       /* flags */
            0xFF,       /* transaction */
            0x23, 0x00, /* message: Release CID */
            0x05, 0x00, /* tlv length: 5 bytes */
            /* TLV */
            0x01,       /* type */
            0x02, 0x00, /* length */
            0xFF,       /* UPDATE: service */
            0x01
        };
        guint8 response[] = {
            0x01,       /* marker */
            /* QMUX */
            0x17, 0x00, /* length */
            0x00,       /* flags */
            0x00,       /* service */
            0x00,       /* client */
            /* QMI header */
            0x01,       /* flags: Response */
            0xFF,       /* transaction */
            0x23, 0x00, /* message */
            0x0C, 0x00, /* tlv length */
            /* TLV */
            0x02,       /* type: Result*/
            0x04, 0x00, /* length */
            0x00, 0x00, /* error status */
            0x00, 0x00, /* error code */
            /* TLV */
            0x01,       /* type: Allocation Info */
            0x02, 0x00, /* length */
            0xFF,       /* UPDATE: service */
            0x01,       /* cid: 1 */
        };

        expected[15] = services[i];
        response[22] = services[i];
        test_port_context_set_command (fixture->ctx,
                                       expected, G_N_ELEMENTS (expected),
                                       response, G_N_ELEMENTS (response),
                                       fixture->service_info[QMI_SERVICE_CTL].transaction_id++);

        qmi_device_release_client (fixture->device, fixture->service_info[services[i]].client, QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID, 10, NULL,
                                   (GAsyncReadyCallback) device_release_client_ready,
                                   fixture);

        test_fixture_loop_run (fixture);

        g_clear_object (&fixture->service_info[services[i]].client);
        fixture->service_info[services[i]].transaction_id = 0x0000;
    }

    if (fixture->device) {
        qmi_device_close_async (fixture->device, 10, NULL,
                                (GAsyncReadyCallback) device_close_ready,
                                fixture);

        test_fixture_loop_run (fixture);

        g_object_unref (fixture->device);
    }

    g_free (fixture->path);

    /* Stop port context */
    test_port_context_stop (fixture->ctx);
    test_port_context_free (fixture->ctx);
}

void
test_fixture_loop_stop (TestFixture *fixture)
{
    g_assert (fixture->loop);
    g_main_loop_quit (fixture->loop);
}

void
test_fixture_loop_run (TestFixture *fixture)
{
    g_assert (!fixture->loop);
    fixture->loop = g_main_loop_new (g_main_context_get_thread_default (), FALSE);
    g_main_loop_run (fixture->loop);
    g_main_loop_unref (fixture->loop);
    fixture->loop = NULL;
}
