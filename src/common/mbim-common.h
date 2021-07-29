/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2013 - 2014 Aleksander Morgado <aleksander@aleksander.es>
 * Copyright (C) 2017 Google Inc.
 */

#ifndef _COMMON_MBIM_COMMON_H_
#define _COMMON_MBIM_COMMON_H_

#include <glib.h>

gchar *mbim_common_str_hex (gconstpointer mem,
                            gsize         size,
                            gchar         delimiter);

#endif /* _COMMON_MBIM_COMMON_H_ */
