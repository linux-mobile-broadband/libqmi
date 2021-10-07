/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
 *
 * Copyright (C) 2021 Aleksander Morgado <aleksander@aleksander.es>
 * Copyright (C) 2021 Intel Corporation
 *
 * This is a private non-installed header
 */

#ifndef _LIBMBIM_GLIB_MBIM_TLV_PRIVATE_H_
#define _LIBMBIM_GLIB_MBIM_TLV_PRIVATE_H_

#if !defined (LIBMBIM_GLIB_COMPILATION)
#error "This is a private header!!"
#endif

#include <glib.h>

#include "mbim-tlv.h"

G_BEGIN_DECLS

/*****************************************************************************/
/* The MbimTlv */

/* Defined in the same way as GByteArray */
struct _MbimTlv {
  guint8 *data;
  guint   len;
};

struct tlv {
  guint16 type;
  guint8  reserved;
  guint8  padding_length;
  guint32 data_length;
  guint8  data[];
} __attribute__((packed));

#define MBIM_TLV_HEADER(self) ((struct tlv *)(((MbimTlv *)self)->data))

#define MBIM_TLV_FIELD_TYPE(self)           MBIM_TLV_HEADER (self)->type
#define MBIM_TLV_FIELD_RESERVED(self)       MBIM_TLV_HEADER (self)->reserved
#define MBIM_TLV_FIELD_PADDING_LENGTH(self) MBIM_TLV_HEADER (self)->padding_length
#define MBIM_TLV_FIELD_DATA_LENGTH(self)    MBIM_TLV_HEADER (self)->data_length
#define MBIM_TLV_FIELD_DATA(self)           MBIM_TLV_HEADER (self)->data

#define MBIM_TLV_GET_TLV_TYPE(self)    (MbimTlvType) GUINT16_FROM_LE (MBIM_TLV_FIELD_TYPE (self))
#define MBIM_TLV_GET_DATA_LENGTH(self) GUINT32_FROM_LE (MBIM_TLV_FIELD_DATA_LENGTH (self))

/*****************************************************************************/
/* Print support */

gchar *_mbim_tlv_print (const MbimTlv *tlv,
                        const gchar   *line_prefix);

/*****************************************************************************/
/* Parsing support */

MbimTlv *_mbim_tlv_new_from_raw (const guint8  *raw,
                                 guint32        raw_length,
                                 guint32       *bytes_read,
                                 GError       **error);

G_END_DECLS

#endif /* _LIBMBIM_GLIB_MBIM_TLV_PRIVATE_H_ */
