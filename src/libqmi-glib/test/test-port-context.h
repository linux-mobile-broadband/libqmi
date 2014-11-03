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
 *
 * Higly based on the test-port-context setup in ModemManager.
 */

#ifndef TEST_PORT_CONTEXT_H
#define TEST_PORT_CONTEXT_H

#include <glib.h>
#include <glib-object.h>

typedef struct _TestPortContext TestPortContext;

TestPortContext *test_port_context_new           (const gchar     *name);
void             test_port_context_start         (TestPortContext *ctx);
void             test_port_context_stop          (TestPortContext *ctx);
void             test_port_context_free          (TestPortContext *ctx);
void             test_port_context_set_command   (TestPortContext *ctx,
                                                  const guint8    *command,
                                                  gsize            command_size,
                                                  const guint8    *response,
                                                  gsize            response_size,
                                                  guint16          transaction_id);

#endif /* TEST_PORT_CONTEXT_H */
