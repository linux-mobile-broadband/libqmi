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

#include <config.h>
#include <libqmi-glib.h>

#include "test-fixture.h"

static void
test_generated_core (TestFixture *fixture)
{
    /* Noop */
}

/*****************************************************************************/

int main (int argc, char **argv)
{
    g_type_init ();
    g_test_init (&argc, &argv, NULL);

    /* Test the setup/teardown test methods */
    TEST_ADD ("/libqmi-glib/generated/core", test_generated_core);

    return g_test_run ();
}
