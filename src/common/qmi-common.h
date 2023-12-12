/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2023 Fabio Porcedda <fabio.porcedda@gmail.com>
 */

#ifndef _COMMON_QMI_COMMON_H_
#define _COMMON_QMI_COMMON_H_

#include <glib.h>

gchar *qmi_common_str_hex (gconstpointer mem,
                           gsize         size,
                           gchar         delimiter);

#endif /* _COMMON_QMI_COMMON_H_ */
