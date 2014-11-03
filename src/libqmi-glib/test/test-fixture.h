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

#ifndef TEST_FIXTURE_H
#define TEST_FIXTURE_H

#include <gio/gio.h>
#include <libqmi-glib.h>

#include "test-port-context.h"

/*****************************************************************************/
/* Test fixture setup */

typedef struct {
    QmiClient *client;
    guint16    transaction_id;
} TestServiceInfo;

typedef struct {
    GMainLoop       *loop;
    gchar           *path;
    TestPortContext *ctx;
    QmiDevice       *device;
    TestServiceInfo  service_info[255];
} TestFixture;

void test_fixture_setup     (TestFixture *fixture);
void test_fixture_teardown  (TestFixture *fixture);
void test_fixture_loop_run  (TestFixture *fixture);
void test_fixture_loop_stop (TestFixture *fixture);

typedef void (*TCFunc) (TestFixture *, gconstpointer);
#define TEST_ADD(path,method)                        \
    g_test_add (path,                                \
                TestFixture,                         \
                NULL,                                \
                (TCFunc)test_fixture_setup,          \
                (TCFunc)method,                      \
                (TCFunc)test_fixture_teardown)

#endif /* TEST_FIXTURE_H */
