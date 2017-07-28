/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
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
