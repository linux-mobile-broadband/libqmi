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
 * Copyright (C) 2016-2022 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBQMI_GLIB_QMI_COMPAT_H_
#define _LIBQMI_GLIB_QMI_COMPAT_H_

#ifndef QMI_DISABLE_DEPRECATED

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
# error "Only <libqmi-glib.h> can be included directly."
#endif

#include "qmi-device.h"
#include "qmi-dms.h"
#include "qmi-nas.h"
#include "qmi-uim.h"
#include "qmi-wda.h"
#include "qmi-wds.h"
#include "qmi-pdc.h"
#include "qmi-loc.h"
#include "qmi-enums-nas.h"
#include "qmi-enums-wms.h"
#include "qmi-enums-pdc.h"

/**
 * SECTION:qmi-compat
 * @short_description: Deprecated types and methods.
 *
 * These types and methods are flagged as deprecated and therefore
 * shouldn't be used in newly written code. They are provided to avoid
 * innecessary API/ABI breaks, for compatibility purposes only.
 */

/**
 * qmi_utils_read_guint8_from_buffer:
 * @buffer: a buffer with raw binary data.
 * @buffer_size: size of @buffer.
 * @out: return location for the read variable.
 *
 * Reads an unsigned byte from the buffer.
 *
 * The user needs to make sure that at least 1 byte is available
 * in the buffer.
 *
 * Also note that both @buffer and @buffer_size get updated after the 1 byte
 * read.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_read_guint8() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_read_guint8)
void qmi_utils_read_guint8_from_buffer  (const guint8 **buffer,
                                         guint16       *buffer_size,
                                         guint8        *out);

/**
 * qmi_utils_read_gint8_from_buffer:
 * @buffer: a buffer with raw binary data.
 * @buffer_size: size of @buffer.
 * @out: return location for the read variable.
 *
 * Reads a signed byte from the buffer.
 *
 * The user needs to make sure that at least 1 byte is available
 * in the buffer.
 *
 * Also note that both @buffer and @buffer_size get updated after the 1 byte
 * read.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_read_gint8() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_read_gint8)
void qmi_utils_read_gint8_from_buffer   (const guint8 **buffer,
                                         guint16       *buffer_size,
                                         gint8         *out);

/**
 * qmi_utils_read_guint16_from_buffer:
 * @buffer: a buffer with raw binary data.
 * @buffer_size: size of @buffer.
 * @endian: endianness of firmware value; swapped to host byte order if necessary
 * @out: return location for the read variable.
 *
 * Reads an unsigned 16-bit integer from the buffer. The number in the buffer is
 * expected to be given in the byte order specificed by @endian, and this method
 * takes care of converting the read value to the proper host endianness.
 *
 * The user needs to make sure that at least 2 bytes are available
 * in the buffer.
 *
 * Also note that both @buffer and @buffer_size get updated after the 2 bytes
 * read.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_read_guint16() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_read_guint16)
void qmi_utils_read_guint16_from_buffer (const guint8 **buffer,
                                         guint16       *buffer_size,
                                         QmiEndian      endian,
                                         guint16       *out);

/**
 * qmi_utils_read_gint16_from_buffer:
 * @buffer: a buffer with raw binary data.
 * @buffer_size: size of @buffer.
 * @endian: endianness of firmware value; swapped to host byte order if necessary
 * @out: return location for the read variable.
 *
 * Reads a signed 16-bit integer from the buffer. The number in the buffer is
 * expected to be given in the byte order specified by @endian, and this method
 * takes care of converting the read value to the proper host endianness.
 *
 * The user needs to make sure that at least 2 bytes are available
 * in the buffer.
 *
 * Also note that both @buffer and @buffer_size get updated after the 2 bytes
 * read.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_read_gint16() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_read_gint16)
void qmi_utils_read_gint16_from_buffer  (const guint8 **buffer,
                                         guint16       *buffer_size,
                                         QmiEndian      endian,
                                         gint16        *out);

/**
 * qmi_utils_read_guint32_from_buffer:
 * @buffer: a buffer with raw binary data.
 * @buffer_size: size of @buffer.
 * @endian: endianness of firmware value; swapped to host byte order if necessary
 * @out: return location for the read variable.
 *
 * Reads an unsigned 32-bit integer from the buffer. The number in the buffer is
 * expected to be given in the byte order specified by @endian, and this method
 * takes care of converting the read value to the proper host endianness.
 *
 * The user needs to make sure that at least 4 bytes are available
 * in the buffer.
 *
 * Also note that both @buffer and @buffer_size get updated after the 4 bytes
 * read.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_read_guint32() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_read_guint32)
void qmi_utils_read_guint32_from_buffer (const guint8 **buffer,
                                         guint16       *buffer_size,
                                         QmiEndian      endian,
                                         guint32       *out);

/**
 * qmi_utils_read_gint32_from_buffer:
 * @buffer: a buffer with raw binary data.
 * @buffer_size: size of @buffer.
 * @endian: endianness of firmware value; swapped to host byte order if necessary
 * @out: return location for the read variable.
 *
 * Reads a signed 32-bit integer from the buffer. The number in the buffer is
 * expected to be given in the byte order specified by @endian, and this method
 * takes care of converting the read value to the proper host endianness.
 *
 * The user needs to make sure that at least 4 bytes are available
 * in the buffer.
 *
 * Also note that both @buffer and @buffer_size get updated after the 4 bytes
 * read.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_read_gint32() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_read_gint32)
void qmi_utils_read_gint32_from_buffer  (const guint8 **buffer,
                                         guint16       *buffer_size,
                                         QmiEndian      endian,
                                         gint32        *out);

/**
 * qmi_utils_read_guint64_from_buffer:
 * @buffer: a buffer with raw binary data.
 * @buffer_size: size of @buffer.
 * @endian: endianness of firmware value; swapped to host byte order if necessary
 * @out: return location for the read variable.
 *
 * Reads an unsigned 64-bit integer from the buffer. The number in the buffer is
 * expected to be given in the byte order specified by @endian, and this method
 * takes care of converting the read value to the proper host endianness.
 *
 * The user needs to make sure that at least 8 bytes are available
 * in the buffer.
 *
 * Also note that both @buffer and @buffer_size get updated after the 8 bytes
 * read.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_read_guint64() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_read_guint64)
void qmi_utils_read_guint64_from_buffer (const guint8 **buffer,
                                         guint16       *buffer_size,
                                         QmiEndian      endian,
                                         guint64       *out);

/**
 * qmi_utils_read_gint64_from_buffer:
 * @buffer: a buffer with raw binary data.
 * @buffer_size: size of @buffer.
 * @endian: endianness of firmware value; swapped to host byte order if necessary
 * @out: return location for the read variable.
 *
 * Reads a signed 64-bit integer from the buffer. The number in the buffer is
 * expected to be given in the byte order specified by @endian, and this method
 * takes care of converting the read value to the proper host endianness.
 *
 * The user needs to make sure that at least 8 bytes are available
 * in the buffer.
 *
 * Also note that both @buffer and @buffer_size get updated after the 8 bytes
 * read.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_read_gint64() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_read_gint64)
void qmi_utils_read_gint64_from_buffer  (const guint8 **buffer,
                                         guint16       *buffer_size,
                                         QmiEndian      endian,
                                         gint64        *out);

/**
 * qmi_utils_read_sized_guint_from_buffer:
 * @buffer: a buffer with raw binary data.
 * @buffer_size: size of @buffer.
 * @n_bytes: number of bytes to read.
 * @endian: endianness of firmware value; swapped to host byte order if necessary
 * @out: return location for the read variable.
 *
 * Reads a @n_bytes-sized unsigned integer from the buffer. The number in the
 * buffer is expected to be given in the byte order specified by @endian, and
 * this method takes care of converting the read value to the proper host
 * endianness.
 *
 * The user needs to make sure that at least @n_bytes bytes are available
 * in the buffer.
 *
 * Also note that both @buffer and @buffer_size get updated after the @n_bytes
 * bytes read.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_read_sized_guint() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_read_sized_guint)
void qmi_utils_read_sized_guint_from_buffer (const guint8 **buffer,
                                             guint16       *buffer_size,
                                             guint          n_bytes,
                                             QmiEndian      endian,
                                             guint64       *out);

/**
 * qmi_utils_read_gfloat_from_buffer:
 * @buffer: a buffer with raw binary data.
 * @buffer_size: size of @buffer.
 * @out: return location for the read variable.
 *
 * Reads a 32-bit floating-point number from the buffer.
 *
 * The user needs to make sure that at least 4 bytes are available
 * in the buffer.
 *
 * Also note that both @buffer and @buffer_size get updated after the 4 bytes
 * read.
 *
 * Since: 1.10
 * Deprecated: 1.12: Use qmi_message_tlv_read_gfloat() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_read_gfloat)
void qmi_utils_read_gfloat_from_buffer  (const guint8 **buffer,
                                         guint16       *buffer_size,
                                         gfloat        *out);

/**
 * qmi_utils_write_guint8_to_buffer:
 * @buffer: a buffer.
 * @buffer_size: size of @buffer.
 * @in: location of the variable to be written.
 *
 * Writes an unsigned byte into the buffer.
 *
 * The user needs to make sure that the buffer is at least 1 byte long.
 *
 * Also note that both @buffer and @buffer_size get updated after the 1 byte
 * write.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_write_guint8() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_write_guint8)
void qmi_utils_write_guint8_to_buffer (guint8  **buffer,
                                       guint16  *buffer_size,
                                       guint8   *in);

/**
 * qmi_utils_write_gint8_to_buffer:
 * @buffer: a buffer.
 * @buffer_size: size of @buffer.
 * @in: location of the variable to be written.
 *
 * Writes a signed byte into the buffer.
 *
 * The user needs to make sure that the buffer is at least 1 byte long.
 *
 * Also note that both @buffer and @buffer_size get updated after the 1 byte
 * write.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_write_gint8() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_write_gint8)
void qmi_utils_write_gint8_to_buffer  (guint8  **buffer,
                                       guint16  *buffer_size,
                                       gint8    *in);

/**
 * qmi_utils_write_guint16_to_buffer:
 * @buffer: a buffer.
 * @buffer_size: size of @buffer.
 * @endian: endianness of firmware value; swapped from host byte order if necessary
 * @in: location of the variable to be written.
 *
 * Writes an unsigned 16-bit integer into the buffer. The number to be written
 * is expected to be given in host endianness, and this method takes care of
 * converting the value written to the byte order specified by @endian.
 *
 * The user needs to make sure that the buffer is at least 2 bytes long.
 *
 * Also note that both @buffer and @buffer_size get updated after the 2 bytes
 * write.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_write_guint16() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_write_guint16)
void qmi_utils_write_guint16_to_buffer (guint8  **buffer,
                                        guint16  *buffer_size,
                                        QmiEndian endian,
                                        guint16  *in);

/**
 * qmi_utils_write_gint16_to_buffer:
 * @buffer: a buffer.
 * @buffer_size: size of @buffer.
 * @endian: endianness of firmware value; swapped from host byte order if necessary
 * @in: location of the variable to be written.
 *
 * Writes a signed 16-bit integer into the buffer. The number to be written
 * is expected to be given in host endianness, and this method takes care of
 * converting the value written to the byte order specified by @endian.
 *
 * The user needs to make sure that the buffer is at least 2 bytes long.
 *
 * Also note that both @buffer and @buffer_size get updated after the 2 bytes
 * write.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_write_gint16() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_write_gint16)
void qmi_utils_write_gint16_to_buffer (guint8  **buffer,
                                       guint16  *buffer_size,
                                       QmiEndian endian,
                                       gint16   *in);

/**
 * qmi_utils_write_guint32_to_buffer:
 * @buffer: a buffer.
 * @buffer_size: size of @buffer.
 * @endian: endianness of firmware value; swapped from host byte order if necessary
 * @in: location of the variable to be written.
 *
 * Writes an unsigned 32-bit integer into the buffer. The number to be written
 * is expected to be given in host endianness, and this method takes care of
 * converting the value written to the byte order specified by @endian.
 *
 * The user needs to make sure that the buffer is at least 4 bytes long.
 *
 * Also note that both @buffer and @buffer_size get updated after the 4 bytes
 * write.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_write_guint32() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_write_guint32)
void qmi_utils_write_guint32_to_buffer (guint8  **buffer,
                                        guint16  *buffer_size,
                                        QmiEndian endian,
                                        guint32  *in);

/**
 * qmi_utils_write_gint32_to_buffer:
 * @buffer: a buffer.
 * @buffer_size: size of @buffer.
 * @endian: endianness of firmware value; swapped from host byte order if necessary
 * @in: location of the variable to be written.
 *
 * Writes a signed 32-bit integer into the buffer. The number to be written
 * is expected to be given in host endianness, and this method takes care of
 * converting the value written to the byte order specified by @endian.
 *
 * The user needs to make sure that the buffer is at least 4 bytes long.
 *
 * Also note that both @buffer and @buffer_size get updated after the 4 bytes
 * write.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_write_gint32() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_write_gint32)
void qmi_utils_write_gint32_to_buffer  (guint8  **buffer,
                                        guint16  *buffer_size,
                                        QmiEndian endian,
                                        gint32   *in);

/**
 * qmi_utils_write_guint64_to_buffer:
 * @buffer: a buffer.
 * @buffer_size: size of @buffer.
 * @endian: endianness of firmware value; swapped from host byte order if necessary
 * @in: location of the variable to be written.
 *
 * Writes an unsigned 64-bit integer into the buffer. The number to be written
 * is expected to be given in host endianness, and this method takes care of
 * converting the value written to the byte order specified by @endian.
 *
 * The user needs to make sure that the buffer is at least 8 bytes long.
 *
 * Also note that both @buffer and @buffer_size get updated after the 8 bytes
 * write.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_write_guint64() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_write_guint64)
void qmi_utils_write_guint64_to_buffer (guint8  **buffer,
                                        guint16  *buffer_size,
                                        QmiEndian endian,
                                        guint64  *in);

/**
 * qmi_utils_write_gint64_to_buffer:
 * @buffer: a buffer.
 * @buffer_size: size of @buffer.
 * @endian: endianness of firmware value; swapped from host byte order if necessary
 * @in: location of the variable to be written.
 *
 * Writes a signed 64-bit integer into the buffer. The number to be written
 * is expected to be given in host endianness, and this method takes care of
 * converting the value written to the byte order specified by @endian.
 *
 * The user needs to make sure that the buffer is at least 8 bytes long.
 *
 * Also note that both @buffer and @buffer_size get updated after the 8 bytes
 * write.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_write_gint64() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_write_gint64)
void qmi_utils_write_gint64_to_buffer  (guint8  **buffer,
                                        guint16  *buffer_size,
                                        QmiEndian endian,
                                        gint64   *in);

/**
 * qmi_utils_write_sized_guint_to_buffer:
 * @buffer: a buffer.
 * @buffer_size: size of @buffer.
 * @n_bytes: number of bytes to write.
 * @endian: endianness of firmware value; swapped from host byte order if necessary
 * @in: location of the variable to be written.
 *
 * Writes a @n_bytes-sized unsigned integer into the buffer. The number to be
 * written is expected to be given in host endianness, and this method takes
 * care of converting the value written to the byte order specified by @endian.
 *
 * The user needs to make sure that the buffer is at least @n_bytes bytes long.
 *
 * Also note that both @buffer and @buffer_size get updated after the @n_bytes
 * bytes write.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_write_sized_guint() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_write_sized_guint)
void qmi_utils_write_sized_guint_to_buffer (guint8  **buffer,
                                            guint16  *buffer_size,
                                            guint     n_bytes,
                                            QmiEndian endian,
                                            guint64  *in);

/**
 * qmi_utils_read_string_from_buffer:
 * @buffer: a buffer with raw binary data.
 * @buffer_size: size of @buffer.
 * @length_prefix_size: size of the length prefix integer in bits.
 * @max_size: maximum number of bytes to read, or 0 to read all available bytes.
 * @out: return location for the read string. The returned value should be freed with g_free().
 *
 * Reads a string from the buffer.
 *
 * If @length_prefix_size is greater than 0, only the amount of bytes given
 * there will be read. Otherwise, up to @buffer_size bytes will be read.
 *
 * Also note that both @buffer and @buffer_size get updated after the write.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_read_string() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_read_string)
void qmi_utils_read_string_from_buffer (const guint8 **buffer,
                                        guint16       *buffer_size,
                                        guint8         length_prefix_size,
                                        guint16        max_size,
                                        gchar        **out);

/**
 * qmi_utils_write_string_to_buffer:
 * @buffer: a buffer.
 * @buffer_size: size of @buffer.
 * @length_prefix_size: size of the length prefix integer in bits.
 * @in: string to write.
 *
 * Writes a string to the buffer.
 *
 * If @length_prefix_size is greater than 0, a length prefix integer will be
 * included in the write operation.
 *
 * The user needs to make sure that the buffer has enough space for both the
 * whole string and the length prefix.
 *
 * Also note that both @buffer and @buffer_size get updated after the write.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_write_string() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_write_string)
void qmi_utils_write_string_to_buffer  (guint8      **buffer,
                                        guint16      *buffer_size,
                                        guint8        length_prefix_size,
                                        const gchar  *in);

/**
 * qmi_utils_read_fixed_size_string_from_buffer:
 * @buffer: a buffer with raw binary data.
 * @buffer_size: size of @buffer.
 * @fixed_size: number of bytes to read.
 * @out: buffer preallocated by the client, with at least @fixed_size bytes.
 *
 * Reads a @fixed_size-sized string from the buffer into the @out buffer.
 *
 * Also note that both @buffer and @buffer_size get updated after the
 * @fixed_size bytes read.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_read_fixed_size_string() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_read_fixed_size_string)
void qmi_utils_read_fixed_size_string_from_buffer (const guint8 **buffer,
                                                   guint16       *buffer_size,
                                                   guint16        fixed_size,
                                                   gchar         *out);

/**
 * qmi_utils_write_fixed_size_string_to_buffer:
 * @buffer: a buffer.
 * @buffer_size: size of @buffer.
 * @fixed_size: number of bytes to write.
 * @in: string to write.
 *
 * Writes a @fixed_size-sized string to the buffer, without any length prefix.
 *
 * The user needs to make sure that the buffer is at least @fixed_size bytes
 * long.
 *
 * Also note that both @buffer and @buffer_size get updated after the
 * @fixed_size bytes write.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use qmi_message_tlv_write_string() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_write_string)
void qmi_utils_write_fixed_size_string_to_buffer  (guint8      **buffer,
                                                   guint16      *buffer_size,
                                                   guint16       fixed_size,
                                                   const gchar  *in);

/**
 * QMI_NAS_SIM_REJECT_STATE_SIM_VAILABLE:
 *
 * SIM available.
 *
 * Since: 1.0
 * Deprecated: 1.14.0: Use the correct #QMI_NAS_SIM_REJECT_STATE_SIM_AVAILABLE name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedNasSimRejectState;
#define QMI_NAS_SIM_REJECT_STATE_SIM_VAILABLE (QmiDeprecatedNasSimRejectState) QMI_NAS_SIM_REJECT_STATE_SIM_AVAILABLE

/**
 * qmi_message_get_printable:
 * @self: a #QmiMessage.
 * @line_prefix: prefix string to use in each new generated line.
 *
 * Gets a printable string with the contents of the whole QMI message.
 *
 * If known, the printable string will contain translated TLV values as well as the raw
 * data buffer contents.
 *
 * Returns: (transfer full): a newly allocated string, which should be freed with g_free().
 *
 * Since: 1.0
 * Deprecated: 1.18: Use qmi_message_get_printable_full() instead.
 */
G_DEPRECATED_FOR (qmi_message_get_printable_full)
gchar *qmi_message_get_printable (QmiMessage  *self,
                                  const gchar *line_prefix);

/**
 * qmi_message_get_version_introduced:
 * @self: a #QmiMessage.
 * @major: (out): return location for the major version.
 * @minor: (out): return location for the minor version.
 *
 * Gets, if known, the service version in which the given message was first
 * introduced.
 *
 * Returns: %TRUE if @major and @minor are set, %FALSE otherwise.
 *
 * Since: 1.0
 * Deprecated: 1.18: Use qmi_message_get_version_introduced_full() instead.
 */
G_DEPRECATED_FOR (qmi_message_get_version_introduced_full)
gboolean qmi_message_get_version_introduced (QmiMessage *self,
                                             guint      *major,
                                             guint      *minor);

/**
 * qmi_message_get_version_introduced_full:
 * @self: a #QmiMessage.
 * @context: a #QmiMessageContext.
 * @major: (out): return location for the major version.
 * @minor: (out): return location for the minor version.
 *
 * Gets, if known, the service version in which the given message was first
 * introduced.
 *
 * The lookup of the version may be specific to the @context provided, e.g. for
 * vendor-specific messages.
 *
 * If no @context given, the behavior is the same as qmi_message_get_version_introduced().
 *
 * Since 1.26, this method will return %FALSE as the library no longer provides
 * version information for each message, given that this information was never
 * reliable.
 *
 * Returns: %TRUE if @major and @minor are set, %FALSE otherwise.
 *
 * Since: 1.18
 * Deprecated: 1.26: The version information of each message is no longer available.
 */
G_DEPRECATED
gboolean qmi_message_get_version_introduced_full (QmiMessage        *self,
                                                  QmiMessageContext *context,
                                                  guint             *major,
                                                  guint             *minor);

/**
 * qmi_device_close:
 * @self: a #QmiDevice
 * @error: Return location for error or %NULL.
 *
 * Synchronously closes a #QmiDevice, preventing any further I/O.
 *
 * If this device was opened with @QMI_DEVICE_OPEN_FLAGS_MBIM, this
 * operation will not wait for the response of the underlying MBIM
 * close sequence.
 *
 * Closing a #QmiDevice multiple times will not return an error.
 *
 * Returns: %TRUE if successful, %FALSE if @error is set.
 *
 * Since: 1.0
 * Deprecated: 1.18: Use qmi_device_close_async() instead.
 */
G_DEPRECATED_FOR (qmi_device_close_async)
gboolean qmi_device_close (QmiDevice  *self,
                           GError    **error);

/**
 * qmi_device_command:
 * @self: a #QmiDevice.
 * @message: the message to send.
 * @timeout: maximum time, in seconds, to wait for the response.
 * @cancellable: a #GCancellable, or %NULL.
 * @callback: a #GAsyncReadyCallback to call when the operation is finished.
 * @user_data: the data to pass to callback function.
 *
 * Asynchronously sends a generic #QmiMessage to the device with no context.
 *
 * If the operation is cancelled via @cancellable, a %QMI_PROTOCOL_ERROR_ABORTED
 * error will be returned always. If the QMI method may be aborted, there is
 * no guarantee that the operation is truly aborted before the error is returned
 * so it may really happen that the operation really succeeded and the method
 * would still return %QMI_PROTOCOL_ERROR_ABORTED. In order to use abortable
 * methods and make sure the response is the correct one, use
 * qmi_device_command_abortable().
 *
 * When the operation is finished @callback will be called. You can then call
 * qmi_device_command_finish() to get the result of the operation.
 *
 * Since: 1.0
 * Deprecated: 1.18: Use qmi_device_command_full() instead.
 */
G_DEPRECATED_FOR (qmi_device_command_full)
void qmi_device_command (QmiDevice           *self,
                         QmiMessage          *message,
                         guint                timeout,
                         GCancellable        *cancellable,
                         GAsyncReadyCallback  callback,
                         gpointer             user_data);

/**
 * qmi_device_command_finish:
 * @self: a #QmiDevice.
 * @res: a #GAsyncResult.
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with qmi_device_command().
 *
 * Returns: (transfer full): a #QmiMessage response, or %NULL if @error is set. The returned value should be freed with qmi_message_unref().
 *
 * Since: 1.0
 * Deprecated: 1.18. Use qmi_device_command_full_finish() instead.
 */
G_DEPRECATED_FOR (qmi_device_command_full_finish)
QmiMessage  *qmi_device_command_finish (QmiDevice     *self,
                                        GAsyncResult  *res,
                                        GError       **error);

/**
 * QMI_WDS_CDMA_CAUSE_CODE_NETWORK_ADDRESS_VACANT:
 *
 * Address is valid but not yet allocated.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_NETWORK_ADDRESS_VACANT name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_NETWORK_ADDRESS_VACANT (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_NETWORK_ADDRESS_VACANT

/**
 * QMI_WDS_CDMA_CAUSE_CODE_NETWORK_ADDRESS_TRANSLATION_FAILURE:
 *
 * Address is invalid.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_NETWORK_ADDRESS_TRANSLATION_FAILURE name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_NETWORK_ADDRESS_TRANSLATION_FAILURE (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_NETWORK_ADDRESS_TRANSLATION_FAILURE

/**
 * QMI_WDS_CDMA_CAUSE_CODE_NETWORK_RESOURCE_SHORTAGE:
 *
 * Network resource shortage.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_NETWORK_RESOURCE_SHORTAGE name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_NETWORK_RESOURCE_SHORTAGE (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_NETWORK_RESOURCE_SHORTAGE

/**
 * QMI_WDS_CDMA_CAUSE_CODE_NETWORK_FAILURE:
 *
 * Network failed.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_NETWORK_FAILURE name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_NETWORK_FAILURE (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_NETWORK_FAILURE

/**
 * QMI_WDS_CDMA_CAUSE_CODE_NETWORK_INVALID_TELESERVICE_ID:
 *
 * SMS teleservice ID is invalid.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_NETWORK_INVALID_TELESERVICE_ID name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_NETWORK_INVALID_TELESERVICE_ID (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_NETWORK_INVALID_TELESERVICE_ID

/**
 * QMI_WDS_CDMA_CAUSE_CODE_NETWORK_OTHER:
 *
 * Other network error.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_NETWORK_OTHER name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_NETWORK_OTHER (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_NETWORK_OTHER

/**
 * QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_NO_PAGE_RESPONSE:
 *
 * No page response from destination.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_NO_PAGE_RESPONSE name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_NO_PAGE_RESPONSE (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_NO_PAGE_RESPONSE

/**
 * QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_BUSY:
 *
 * Destination is busy.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_BUSY name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_BUSY (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_BUSY

/**
 * QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_NO_ACK:
 *
 * No acknowledge from destination.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_NO_ACK name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_NO_ACK (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_NO_ACK

/**
 * QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_RESOURCE_SHORTAGE:
 *
 * Destination resource shortage.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_RESOURCE_SHORTAGE name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_RESOURCE_SHORTAGE (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_RESOURCE_SHORTAGE

/**
 * QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_SMS_DELIVERY_POSTPONED:
 *
 * SMS delivery postponed.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_SMS_DELIVERY_POSTPONED name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_SMS_DELIVERY_POSTPONED (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_SMS_DELIVERY_POSTPONED

/**
 * QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_OUT_OF_SERVICE:
 *
 * Destination out of service.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_OUT_OF_SERVICE name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_OUT_OF_SERVICE (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_OUT_OF_SERVICE

/**
 * QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_NOT_AT_ADDRESS:
 *
 * Destination not at address.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_NOT_AT_ADDRESS name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_NOT_AT_ADDRESS (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_NOT_AT_ADDRESS

/**
 * QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_OTHER:
 *
 * Other destination error.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_OTHER name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_OTHER (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_OTHER

/**
 * QMI_WDS_CDMA_CAUSE_CODE_RADIO_INTERFACE_RESOURCE_SHORTAGE:
 *
 * Radio interface resource shortage.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_RADIO_INTERFACE_RESOURCE_SHORTAGE name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_RADIO_INTERFACE_RESOURCE_SHORTAGE (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_RADIO_INTERFACE_RESOURCE_SHORTAGE

/**
 * QMI_WDS_CDMA_CAUSE_CODE_RADIO_INTERFACE_INCOMPATIBILITY:
 *
 * Radio interface incompatibility.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_RADIO_INTERFACE_INCOMPATIBILITY name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_RADIO_INTERFACE_INCOMPATIBILITY (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_RADIO_INTERFACE_INCOMPATIBILITY

/**
 * QMI_WDS_CDMA_CAUSE_CODE_RADIO_INTERFACE_OTHER:
 *
 * Other radio interface error
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_RADIO_INTERFACE_OTHER name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_RADIO_INTERFACE_OTHER (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_RADIO_INTERFACE_OTHER

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_ENCODING:
 *
 * Encoding error.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_ENCODING name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_ENCODING (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_ENCODING

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_SMS_ORIGIN_DENIED:
 *
 * SMS origin denied.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_SMS_ORIGIN_DENIED name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_SMS_ORIGIN_DENIED (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_SMS_ORIGIN_DENIED

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_SMS_DESTINATION_DENIED:
 *
 * SMS destination denied.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_SMS_DESTINATION_DENIED name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_SMS_DESTINATION_DENIED (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_SMS_DESTINATION_DENIED

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_SUPPLEMENTARY_SERVICE_NOT_SUPPORTED:
 *
 * Supplementary service not supported.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_SUPPLEMENTARY_SERVICE_NOT_SUPPORTED name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_SUPPLEMENTARY_SERVICE_NOT_SUPPORTED (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_SUPPLEMENTARY_SERVICE_NOT_SUPPORTED

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_SMS_NOT_SUPPORTED:
 *
 * SMS not supported.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_SMS_NOT_SUPPORTED name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_SMS_NOT_SUPPORTED (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_SMS_NOT_SUPPORTED

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_MISSING_EXPECTED_PARAMETER:
 *
 * Missing optional expected parameter.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_MISSING_EXPECTED_PARAMETER name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_MISSING_EXPECTED_PARAMETER (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_MISSING_EXPECTED_PARAMETER

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_MISSING_MANDATORY_PARAMETER:
 *
 * Missing mandatory parameter.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_MISSING_MANDATORY_PARAMETER name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_MISSING_MANDATORY_PARAMETER (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_MISSING_MANDATORY_PARAMETER

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_UNRECOGNIZED_PARAMETER_VALUE:
 *
 * Unrecognized parameter value.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_UNRECOGNIZED_PARAMETER_VALUE name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_UNRECOGNIZED_PARAMETER_VALUE (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_UNRECOGNIZED_PARAMETER_VALUE

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_UNEXPECTED_PARAMETER_VALUE:
 *
 * Unexpected parameter value.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_UNEXPECTED_PARAMETER_VALUE name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_UNEXPECTED_PARAMETER_VALUE (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_UNEXPECTED_PARAMETER_VALUE

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_USER_DATA_SIZE_ERROR:
 *
 * User data size error.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_USER_DATA_SIZE_ERROR name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_USER_DATA_SIZE_ERROR (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_USER_DATA_SIZE_ERROR

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_OTHER:
 *
 * Other general error.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_OTHER name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsCdmaCauseCode;
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_OTHER (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_OTHER

/**
 * QMI_PROTOCOL_ERROR_QOS_UNAVAILABLE:
 *
 * QoS unavailable.
 *
 * Since: 1.0
 * Deprecated: 1.22.0: Use the #QMI_PROTOCOL_ERROR_REQUESTED_NUMBER_UNSUPPORTED instead.
 */
G_DEPRECATED typedef int QmiDeprecatedProtocolError;
#define QMI_PROTOCOL_ERROR_QOS_UNAVAILABLE (QmiDeprecatedProtocolError) QMI_PROTOCOL_ERROR_REQUESTED_NUMBER_UNSUPPORTED

/**
 * QMI_WDS_VERBOSE_CALL_END_REASON_CM_CONFIGURATION_FAILED:
 *
 * Conference (not configuration) failed.
 *
 * Since: 1.0
 * Deprecated: 1.24.10: Use the correct #QMI_WDS_VERBOSE_CALL_END_REASON_CM_CONFERENCE_FAILED name instead.
 */
G_DEPRECATED typedef int QmiDeprecatedWdsVerboseCallEndReasonCm;
#define QMI_WDS_VERBOSE_CALL_END_REASON_CM_CONFIGURATION_FAILED (QmiDeprecatedWdsVerboseCallEndReasonCm) QMI_WDS_VERBOSE_CALL_END_REASON_CM_CONFERENCE_FAILED

/**
 * qmi_message_tlv_read_gfloat:
 * @self: a #QmiMessage.
 * @tlv_offset: offset that was returned by qmi_message_tlv_read_init().
 * @offset: address of a the offset within the TLV value.
 * @out: return location for the read #gfloat.
 * @error: return location for error or %NULL.
 *
 * Reads a 32-bit floating-point number from the TLV.
 *
 * @offset needs to point to a valid @gsize specifying the index to start
 * reading from within the TLV value (0 for the first item). If the variable
 * is successfully read, @offset will be updated to point past the read item.
 *
 * The implementation assumes the float is encoded with the same endianness as
 * the host, which may not be true. The use of this method is discouraged, and
 * new code should use qmi_message_tlv_read_gfloat_endian() instead.
 *
 * Returns: %TRUE if the variable is successfully read, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 * Deprecated: 1.22: Use qmi_message_tlv_read_gfloat_endian() instead.
 */
G_DEPRECATED_FOR (qmi_message_tlv_read_gfloat_endian)
gboolean qmi_message_tlv_read_gfloat (QmiMessage  *self,
                                      gsize        tlv_offset,
                                      gsize       *offset,
                                      gfloat      *out,
                                      GError     **error);

/**
 * QmiDmsDellFirmwareVersionType:
 * @QMI_DMS_DELL_FIRMWARE_VERSION_TYPE_FIRMWARE_MCFG: E.g. T77W968.F0.0.0.2.3.GC.004.
 * @QMI_DMS_DELL_FIRMWARE_VERSION_TYPE_FIRMWARE_MCFG_APPS: E.g. T77W968.F0.0.0.2.3.GC.004.011.
 * @QMI_DMS_DELL_FIRMWARE_VERSION_TYPE_APPS: E.g. 011.
 *
 * Dell specific firmware version types.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use #QmiDmsFoxconnFirmwareVersionType instead.
 */

/* The following type exists just so that we don't get deprecation warnings on
 * our own methods */
typedef enum {
    QMI_DMS_DELL_FIRMWARE_VERSION_TYPE_FIRMWARE_MCFG      = QMI_DMS_FOXCONN_FIRMWARE_VERSION_TYPE_FIRMWARE_MCFG,
    QMI_DMS_DELL_FIRMWARE_VERSION_TYPE_FIRMWARE_MCFG_APPS = QMI_DMS_FOXCONN_FIRMWARE_VERSION_TYPE_FIRMWARE_MCFG_APPS,
    QMI_DMS_DELL_FIRMWARE_VERSION_TYPE_APPS               = QMI_DMS_FOXCONN_FIRMWARE_VERSION_TYPE_APPS,
} QmiDeprecatedDmsDellFirmwareVersionType;

G_DEPRECATED
typedef QmiDeprecatedDmsDellFirmwareVersionType QmiDmsDellFirmwareVersionType;

G_DEPRECATED_FOR (qmi_dms_foxconn_firmware_version_type_get_type)
GType qmi_dms_dell_firmware_version_type_get_type (void) G_GNUC_CONST;
#define QMI_TYPE_DMS_DELL_FIRMWARE_VERSION_TYPE QMI_TYPE_DMS_FOXCONN_FIRMWARE_VERSION_TYPE

/**
 * qmi_dms_dell_firmware_version_type_get_string:
 * @val: a #QmiDmsFoxconnFirmwareVersionType.
 *
 * Gets the nickname string for the #QmiDmsFoxconnFirmwareVersionType specified at @val.
 *
 * Returns: (transfer none): a string with the nickname, or %NULL if not found. Do not free the returned value.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_dms_foxconn_firmware_version_type_get_string() instead.
 */
G_DEPRECATED_FOR (qmi_dms_foxconn_firmware_version_type_get_string)
const gchar *qmi_dms_dell_firmware_version_type_get_string (QmiDeprecatedDmsDellFirmwareVersionType val);

/**
 * QmiDmsDellDeviceMode:
 * @QMI_DMS_DELL_DEVICE_MODE_UNKNOWN: Unknown mode.
 * @QMI_DMS_DELL_DEVICE_MODE_FASTBOOT_ONLINE: Fastboot download mode for full partition files.
 * @QMI_DMS_DELL_DEVICE_MODE_FASTBOOT_OTA: Fastboot download mode for OTA files.
 *
 * Dell specific device modes.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use #QmiDmsFoxconnDeviceMode instead.
 */

/* The following type exists just so that we don't get deprecation warnings on
 * our own methods */
typedef enum {
    QMI_DMS_DELL_DEVICE_MODE_UNKNOWN         = QMI_DMS_FOXCONN_DEVICE_MODE_UNKNOWN,
    QMI_DMS_DELL_DEVICE_MODE_FASTBOOT_ONLINE = QMI_DMS_FOXCONN_DEVICE_MODE_FASTBOOT_ONLINE,
    QMI_DMS_DELL_DEVICE_MODE_FASTBOOT_OTA    = QMI_DMS_FOXCONN_DEVICE_MODE_FASTBOOT_OTA,
} QmiDeprecatedDmsDellDeviceMode;

G_DEPRECATED
typedef QmiDeprecatedDmsDellDeviceMode QmiDmsDellDeviceMode;

G_DEPRECATED_FOR (qmi_dms_foxconn_device_mode_get_type)
GType qmi_dms_dell_device_mode_get_type (void) G_GNUC_CONST;
#define QMI_TYPE_DMS_DELL_DEVICE_MODE QMI_TYPE_DMS_FOXCONN_DEVICE_MODE

/**
 * qmi_dms_dell_device_mode_get_string:
 * @val: a QmiDmsDellDeviceMode.
 *
 * Gets the nickname string for the #QmiDmsDellDeviceMode specified at @val.
 *
 * Returns: (transfer none): a string with the nickname, or %NULL if not found. Do not free the returned value.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_dms_foxconn_device_mode_get_string() instead.
 */
G_DEPRECATED_FOR (qmi_dms_foxconn_device_mode_get_string)
const gchar *qmi_dms_dell_device_mode_get_string (QmiDeprecatedDmsDellDeviceMode val);


/**
 * QmiWdsGetCurrentSettingsRequestedSettings:
 * @QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_NONE: no settings requested
 * @QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_PROFILE_ID: request profile ID
 * @QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_PROFILE_NAME: request profile name
 * @QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_PDP_TYPE: request PDP context type
 * @QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_APN_NAME: request APN name
 * @QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_DNS_ADDRESS: request DNS server addresses
 * @QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_GRANTED_QOS: request granted QoS
 * @QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_USERNAME: request username
 * @QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_AUTH_PROTOCOL: request authentication protocol, ie PAP/CHAP/none.
 * @QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_IP_ADDRESS: request IP address
 * @QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_GATEWAY_INFO: request gateway address
 * @QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_PCSCF_ADDRESS: request PCSCF address
 * @QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_PCSCF_SERVER_ADDRESS_LIST: request PCSCF server address list
 * @QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_PCSCF_DOMAIN_NAME_LIST: request PCSCF domain name list
 * @QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_MTU: request MTU
 * @QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_DOMAIN_NAME_LIST: request domain name list
 * @QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_IP_FAMILY: request IP family, ie IPv4 or IPv6.
 * @QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_IMCN_FLAG: request IMCN flag
 * @QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_EXTENDED_TECHNOLOGY: request extended technology info
 * @QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_OPERATOR_RESERVED_PCO: operator reserved protocol configuration option. Since 1.32.
 *
 * Flags specifying WDS bearer settings.
 *
 * Since: 1.0
 * Deprecated: 1.32: Use #QmiWdsRequestedSettings instead.
 */

/* The following type exists just so that we don't get deprecation warnings on
 * our own methods */
typedef enum {
    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_NONE                      = QMI_WDS_REQUESTED_SETTINGS_NONE,
    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_PROFILE_ID                = QMI_WDS_REQUESTED_SETTINGS_PROFILE_ID,
    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_PROFILE_NAME              = QMI_WDS_REQUESTED_SETTINGS_PROFILE_NAME,
    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_PDP_TYPE                  = QMI_WDS_REQUESTED_SETTINGS_PDP_TYPE,
    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_APN_NAME                  = QMI_WDS_REQUESTED_SETTINGS_APN_NAME,
    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_DNS_ADDRESS               = QMI_WDS_REQUESTED_SETTINGS_DNS_ADDRESS,
    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_GRANTED_QOS               = QMI_WDS_REQUESTED_SETTINGS_GRANTED_QOS,
    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_USERNAME                  = QMI_WDS_REQUESTED_SETTINGS_USERNAME,
    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_AUTH_PROTOCOL             = QMI_WDS_REQUESTED_SETTINGS_AUTH_PROTOCOL,
    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_IP_ADDRESS                = QMI_WDS_REQUESTED_SETTINGS_IP_ADDRESS,
    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_GATEWAY_INFO              = QMI_WDS_REQUESTED_SETTINGS_GATEWAY_INFO,
    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_PCSCF_ADDRESS             = QMI_WDS_REQUESTED_SETTINGS_PCSCF_ADDRESS,
    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_PCSCF_SERVER_ADDRESS_LIST = QMI_WDS_REQUESTED_SETTINGS_PCSCF_SERVER_ADDRESS_LIST,
    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_PCSCF_DOMAIN_NAME_LIST    = QMI_WDS_REQUESTED_SETTINGS_PCSCF_DOMAIN_NAME_LIST,
    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_MTU                       = QMI_WDS_REQUESTED_SETTINGS_MTU,
    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_DOMAIN_NAME_LIST          = QMI_WDS_REQUESTED_SETTINGS_DOMAIN_NAME_LIST,
    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_IP_FAMILY                 = QMI_WDS_REQUESTED_SETTINGS_IP_FAMILY,
    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_IMCN_FLAG                 = QMI_WDS_REQUESTED_SETTINGS_IMCN_FLAG,
    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_EXTENDED_TECHNOLOGY       = QMI_WDS_REQUESTED_SETTINGS_EXTENDED_TECHNOLOGY,
    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_OPERATOR_RESERVED_PCO     = QMI_WDS_REQUESTED_SETTINGS_OPERATOR_RESERVED_PCO,
} QmiDeprecatedWdsGetCurrentSettingsRequestedSettings;

G_DEPRECATED
typedef QmiDeprecatedWdsGetCurrentSettingsRequestedSettings QmiWdsGetCurrentSettingsRequestedSettings;

G_DEPRECATED_FOR (qmi_wds_get_current_settings_requested_settings_get_type)
GType qmi_wds_get_current_settings_requested_settings_get_type (void) G_GNUC_CONST;
#define QMI_TYPE_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS QMI_TYPE_WDS_REQUESTED_SETTINGS

/**
 * qmi_wds_get_current_settings_requested_settings_build_string_from_mask:
 * @mask: bitmask of QmiWdsGetCurrentSettingsRequestedSettings values.
 *
 * Builds a string containing a comma-separated list of nicknames for
 * each #QmiWdsGetCurrentSettingsRequestedSettings in @mask.
 *
 * Returns: (transfer full): a string with the list of nicknames, or %NULL if none given. The returned value should be freed with g_free().
 * Since: 1.0
 * Deprecated: 1.32: Use qmi_wds_requested_settings_build_string_from_mask() instead.
 */
G_DEPRECATED_FOR (qmi_wds_requested_settings_build_string_from_mask)
gchar *qmi_wds_get_current_settings_requested_settings_build_string_from_mask (QmiDeprecatedWdsGetCurrentSettingsRequestedSettings mask);

/******************************************************************************/
/* Conditional method definitions */

#if defined HAVE_QMI_MESSAGE_DMS_SET_SERVICE_PROGRAMMING_CODE

/**
 * qmi_message_dms_set_service_programming_code_input_get_new:
 * @self: a #QmiMessageDmsSetServiceProgrammingCodeInput.
 * @arg_new: a placeholder for the output constant string, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'New Code' field from @self.
 *
 * Returns: %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.0
 * Deprecated: 1.14.0: Use qmi_message_dms_set_service_programming_code_input_get_new_code() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_set_service_programming_code_input_get_new_code)
gboolean qmi_message_dms_set_service_programming_code_input_get_new (
    QmiMessageDmsSetServiceProgrammingCodeInput *self,
    const gchar **arg_new,
    GError **error);

/**
 * qmi_message_dms_set_service_programming_code_input_set_new:
 * @self: a #QmiMessageDmsSetServiceProgrammingCodeInput.
 * @arg_new: a constant string of exactly 6 characters.
 * @error: Return location for error or %NULL.
 *
 * Set the 'New Code' field in the message.
 *
 * Returns: %TRUE if @value was successfully set, %FALSE otherwise.
 *
 * Since: 1.0
 * Deprecated: 1.14.0: Use qmi_message_dms_set_service_programming_code_input_set_new_code() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_set_service_programming_code_input_set_new_code)
gboolean qmi_message_dms_set_service_programming_code_input_set_new (
    QmiMessageDmsSetServiceProgrammingCodeInput *self,
    const gchar *arg_new,
    GError **error);

/**
 * qmi_message_dms_set_service_programming_code_input_get_current:
 * @self: a #QmiMessageDmsSetServiceProgrammingCodeInput.
 * @arg_current: a placeholder for the output constant string, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Current Code' field from @self.
 *
 * Returns: %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.0
 * Deprecated: 1.14.0: Use qmi_message_dms_set_service_programming_code_input_get_current_code() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_set_service_programming_code_input_get_current_code)
gboolean qmi_message_dms_set_service_programming_code_input_get_current (
    QmiMessageDmsSetServiceProgrammingCodeInput *self,
    const gchar **arg_current,
    GError **error);

/**
 * qmi_message_dms_set_service_programming_code_input_set_current:
 * @self: a #QmiMessageDmsSetServiceProgrammingCodeInput.
 * @arg_current: a placeholder for the output constant string, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Current Code' field from @self.
 *
 * Returns: %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.0
 * Deprecated: 1.14.0: Use qmi_message_dms_set_service_programming_code_input_set_current_code() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_set_service_programming_code_input_set_current_code)
gboolean qmi_message_dms_set_service_programming_code_input_set_current (
    QmiMessageDmsSetServiceProgrammingCodeInput *self,
    const gchar *arg_current,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_DMS_SET_SERVICE_PROGRAMMING_CODE */

#if defined HAVE_QMI_MESSAGE_UIM_READ_TRANSPARENT

/**
 * qmi_message_uim_read_transparent_input_get_session_information:
 * @self: a #QmiMessageUimReadTransparentInput.
 * @value_session_information_session_type: a placeholder for the output #QmiUimSessionType, or %NULL if not required.
 * @value_session_information_application_identifier: a placeholder for the output constant string, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Session Information' field from @self.
 *
 * Returns: %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.6
 * Deprecated: 1.22: Use qmi_message_uim_read_transparent_input_get_session() instead.
 */
G_DEPRECATED_FOR (qmi_message_uim_read_transparent_input_get_session)
gboolean qmi_message_uim_read_transparent_input_get_session_information (
    QmiMessageUimReadTransparentInput *self,
    QmiUimSessionType *value_session_information_session_type,
    const gchar **value_session_information_application_identifier,
    GError **error);

/**
 * qmi_message_uim_read_transparent_input_set_session_information:
 * @self: a #QmiMessageUimReadTransparentInput.
 * @value_session_information_session_type: a #QmiUimSessionType.
 * @value_session_information_application_identifier: a constant string.
 * @error: Return location for error or %NULL.
 *
 * Set the 'Session Information' field in the message.
 *
 * Returns: %TRUE if @value was successfully set, %FALSE otherwise.
 *
 * Since: 1.6
 * Deprecated: 1.22: Use qmi_message_uim_read_transparent_input_set_session() instead.
 */
G_DEPRECATED_FOR (qmi_message_uim_read_transparent_input_set_session)
gboolean qmi_message_uim_read_transparent_input_set_session_information (
    QmiMessageUimReadTransparentInput *self,
    QmiUimSessionType value_session_information_session_type,
    const gchar *value_session_information_application_identifier,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_UIM_READ_TRANSPARENT */

#if defined HAVE_QMI_MESSAGE_UIM_READ_RECORD

/**
 * qmi_message_uim_read_record_input_get_session_information:
 * @self: a #QmiMessageUimReadRecordInput.
 * @value_session_information_session_type: a placeholder for the output #QmiUimSessionType, or %NULL if not required.
 * @value_session_information_application_identifier: a placeholder for the output constant string, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Session Information' field from @self.
 *
 * Returns: %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.6
 * Deprecated: 1.22: Use qmi_message_uim_read_record_input_get_session_information() instead.
 */
G_DEPRECATED_FOR (qmi_message_uim_read_record_input_get_session_information)
gboolean qmi_message_uim_read_record_input_get_session_information (
    QmiMessageUimReadRecordInput *self,
    QmiUimSessionType *value_session_information_session_type,
    const gchar **value_session_information_application_identifier,
    GError **error);

/**
 * qmi_message_uim_read_record_input_set_session_information:
 * @self: a #QmiMessageUimReadRecordInput.
 * @value_session_information_session_type: a #QmiUimSessionType.
 * @value_session_information_application_identifier: a constant string.
 * @error: Return location for error or %NULL.
 *
 * Set the 'Session Information' field in the message.
 *
 * Returns: %TRUE if @value was successfully set, %FALSE otherwise.
 *
 * Since: 1.6
 * Deprecated: 1.22: Use qmi_message_uim_read_record_input_set_session() instead.
 */
G_DEPRECATED_FOR (qmi_message_uim_read_record_input_set_session)
gboolean qmi_message_uim_read_record_input_set_session_information (
    QmiMessageUimReadRecordInput *self,
    QmiUimSessionType value_session_information_session_type,
    const gchar *value_session_information_application_identifier,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_UIM_READ_RECORD */

#if defined HAVE_QMI_MESSAGE_UIM_GET_FILE_ATTRIBUTES

/**
 * qmi_message_uim_get_file_attributes_input_get_session_information:
 * @self: a #QmiMessageUimGetFileAttributesInput.
 * @value_session_information_session_type: a placeholder for the output #QmiUimSessionType, or %NULL if not required.
 * @value_session_information_application_identifier: a placeholder for the output constant string, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Session Information' field from @self.
 *
 * Returns: %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.6
 * Deprecated: 1.22: Use qmi_message_uim_get_file_attributes_input_get_session() instead.
 */
G_DEPRECATED_FOR (qmi_message_uim_get_file_attributes_input_get_session)
gboolean qmi_message_uim_get_file_attributes_input_get_session_information (
    QmiMessageUimGetFileAttributesInput *self,
    QmiUimSessionType *value_session_information_session_type,
    const gchar **value_session_information_application_identifier,
    GError **error);

/**
 * qmi_message_uim_get_file_attributes_input_set_session_information:
 * @self: a #QmiMessageUimGetFileAttributesInput.
 * @value_session_information_session_type: a #QmiUimSessionType.
 * @value_session_information_application_identifier: a constant string.
 * @error: Return location for error or %NULL.
 *
 * Set the 'Session Information' field in the message.
 *
 * Returns: %TRUE if @value was successfully set, %FALSE otherwise.
 *
 * Since: 1.6
 * Deprecated: 1.22: Use qmi_message_uim_get_file_attributes_input_set_session() instead.
 */
G_DEPRECATED_FOR (qmi_message_uim_get_file_attributes_input_set_session)
gboolean qmi_message_uim_get_file_attributes_input_set_session_information (
    QmiMessageUimGetFileAttributesInput *self,
    QmiUimSessionType value_session_information_session_type,
    const gchar *value_session_information_application_identifier,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_UIM_GET_FILE_ATTRIBUTES */

#if defined HAVE_QMI_MESSAGE_UIM_SET_PIN_PROTECTION

/**
 * qmi_message_uim_set_pin_protection_input_get_session_information:
 * @self: a #QmiMessageUimSetPinProtectionInput.
 * @value_session_information_session_type: a placeholder for the output #QmiUimSessionType, or %NULL if not required.
 * @value_session_information_application_identifier: a placeholder for the output constant string, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Session Information' field from @self.
 *
 * Returns: %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.14
 * Deprecated: 1.22: Use qmi_message_uim_set_pin_protection_input_get_session() instead.
 */
G_DEPRECATED_FOR (qmi_message_uim_set_pin_protection_input_get_session)
gboolean qmi_message_uim_set_pin_protection_input_get_session_information (
    QmiMessageUimSetPinProtectionInput *self,
    QmiUimSessionType *value_session_information_session_type,
    const gchar **value_session_information_application_identifier,
    GError **error);

/**
 * qmi_message_uim_set_pin_protection_input_set_session_information:
 * @self: a #QmiMessageUimSetPinProtectionInput.
 * @value_session_information_session_type: a #QmiUimSessionType.
 * @value_session_information_application_identifier: a constant string.
 * @error: Return location for error or %NULL.
 *
 * Set the 'Session Information' field in the message.
 *
 * Returns: %TRUE if @value was successfully set, %FALSE otherwise.
 *
 * Since: 1.14
 * Deprecated: 1.22: Use qmi_message_uim_set_pin_protection_input_set_session() instead.
 */
G_DEPRECATED_FOR (qmi_message_uim_set_pin_protection_input_set_session)
gboolean qmi_message_uim_set_pin_protection_input_set_session_information (
    QmiMessageUimSetPinProtectionInput *self,
    QmiUimSessionType value_session_information_session_type,
    const gchar *value_session_information_application_identifier,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_UIM_SET_PIN_PROTECTION */

#if defined HAVE_QMI_MESSAGE_UIM_VERIFY_PIN

/**
 * qmi_message_uim_verify_pin_input_get_session_information:
 * @self: a #QmiMessageUimVerifyPinInput.
 * @value_session_information_session_type: a placeholder for the output #QmiUimSessionType, or %NULL if not required.
 * @value_session_information_application_identifier: a placeholder for the output constant string, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Session Information' field from @self.
 *
 * Returns: %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.14
 * Deprecated: 1.22: Use qmi_message_uim_verify_pin_input_get_session() instead.
 */
G_DEPRECATED_FOR (qmi_message_uim_verify_pin_input_get_session)
gboolean qmi_message_uim_verify_pin_input_get_session_information (
    QmiMessageUimVerifyPinInput *self,
    QmiUimSessionType *value_session_information_session_type,
    const gchar **value_session_information_application_identifier,
    GError **error);

/**
 * qmi_message_uim_verify_pin_input_set_session_information:
 * @self: a #QmiMessageUimVerifyPinInput.
 * @value_session_information_session_type: a #QmiUimSessionType.
 * @value_session_information_application_identifier: a constant string.
 * @error: Return location for error or %NULL.
 *
 * Set the 'Session Information' field in the message.
 *
 * Returns: %TRUE if @value was successfully set, %FALSE otherwise.
 *
 * Since: 1.14
 * Deprecated: 1.22: Use qmi_message_uim_verify_pin_input_set_session() instead.
 */
G_DEPRECATED_FOR (qmi_message_uim_verify_pin_input_set_session)
gboolean qmi_message_uim_verify_pin_input_set_session_information (
    QmiMessageUimVerifyPinInput *self,
    QmiUimSessionType value_session_information_session_type,
    const gchar *value_session_information_application_identifier,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_UIM_VERIFY_PIN */

#if defined HAVE_QMI_MESSAGE_UIM_UNBLOCK_PIN

/**
 * qmi_message_uim_unblock_pin_input_get_session_information:
 * @self: a #QmiMessageUimUnblockPinInput.
 * @value_session_information_session_type: a placeholder for the output #QmiUimSessionType, or %NULL if not required.
 * @value_session_information_application_identifier: a placeholder for the output constant string, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Session Information' field from @self.
 *
 * Returns: %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.14
 * Deprecated: 1.22: Use qmi_message_uim_unblock_pin_input_get_session() instead.
 */
G_DEPRECATED_FOR (qmi_message_uim_unblock_pin_input_get_session)
gboolean qmi_message_uim_unblock_pin_input_get_session_information (
    QmiMessageUimUnblockPinInput *self,
    QmiUimSessionType *value_session_information_session_type,
    const gchar **value_session_information_application_identifier,
    GError **error);

/**
 * qmi_message_uim_unblock_pin_input_set_session_information:
 * @self: a #QmiMessageUimUnblockPinInput.
 * @value_session_information_session_type: a #QmiUimSessionType.
 * @value_session_information_application_identifier: a constant string.
 * @error: Return location for error or %NULL.
 *
 * Set the 'Session Information' field in the message.
 *
 * Returns: %TRUE if @value was successfully set, %FALSE otherwise.
 *
 * Since: 1.14
 * Deprecated: 1.22: Use qmi_message_uim_unblock_pin_input_set_session() instead.
 */
G_DEPRECATED_FOR (qmi_message_uim_unblock_pin_input_set_session)
gboolean qmi_message_uim_unblock_pin_input_set_session_information (
    QmiMessageUimUnblockPinInput *self,
    QmiUimSessionType value_session_information_session_type,
    const gchar *value_session_information_application_identifier,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_UIM_UNBLOCK_PIN */

#if defined HAVE_QMI_MESSAGE_UIM_CHANGE_PIN

/**
 * qmi_message_uim_change_pin_input_get_session_information:
 * @self: a #QmiMessageUimChangePinInput.
 * @value_session_information_session_type: a placeholder for the output #QmiUimSessionType, or %NULL if not required.
 * @value_session_information_application_identifier: a placeholder for the output constant string, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Session Information' field from @self.
 *
 * Returns: %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.14
 * Deprecated: 1.22: Use qmi_message_uim_change_pin_input_get_session() instead.
 */
G_DEPRECATED_FOR (qmi_message_uim_change_pin_input_get_session)
gboolean qmi_message_uim_change_pin_input_get_session_information (
    QmiMessageUimChangePinInput *self,
    QmiUimSessionType *value_session_information_session_type,
    const gchar **value_session_information_application_identifier,
    GError **error);

/**
 * qmi_message_uim_change_pin_input_set_session_information:
 * @self: a #QmiMessageUimChangePinInput.
 * @value_session_information_session_type: a #QmiUimSessionType.
 * @value_session_information_application_identifier: a constant string.
 * @error: Return location for error or %NULL.
 *
 * Set the 'Session Information' field in the message.
 *
 * Returns: %TRUE if @value was successfully set, %FALSE otherwise.
 *
 * Since: 1.14
 * Deprecated: 1.22: Use qmi_message_uim_change_pin_input_set_session() instead.
 */
G_DEPRECATED_FOR (qmi_message_uim_change_pin_input_set_session)
gboolean qmi_message_uim_change_pin_input_set_session_information (
    QmiMessageUimChangePinInput *self,
    QmiUimSessionType value_session_information_session_type,
    const gchar *value_session_information_application_identifier,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_UIM_CHANGE_PIN */

#if defined HAVE_QMI_MESSAGE_DMS_FOXCONN_GET_FIRMWARE_VERSION

/**
 * QmiMessageDmsDellGetFirmwareVersionInput:
 *
 * The #QmiMessageDmsDellGetFirmwareVersionInput structure contains private data and should only be accessed
 * using the provided API.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use #QmiMessageDmsFoxconnGetFirmwareVersionInput instead.
 */
typedef QmiMessageDmsFoxconnGetFirmwareVersionInput QmiDeprecatedMessageDmsDellGetFirmwareVersionInput;
G_DEPRECATED_FOR (QmiMessageDmsFoxconnGetFirmwareVersionInput)
typedef QmiDeprecatedMessageDmsDellGetFirmwareVersionInput QmiMessageDmsDellGetFirmwareVersionInput;

#define QMI_TYPE_MESSAGE_DMS_DELL_GET_FIRMWARE_VERSION_INPUT QMI_TYPE_MESSAGE_DMS_FOXCONN_GET_FIRMWARE_VERSION_INPUT

G_DEPRECATED_FOR (qmi_message_dms_foxconn_get_firmware_version_input_get_type)
GType qmi_message_dms_dell_get_firmware_version_input_get_type (void) G_GNUC_CONST;

/**
 * qmi_message_dms_dell_get_firmware_version_input_get_version_type:
 * @self: a #QmiMessageDmsDellGetFirmwareVersionInput.
 * @value_version_type: a placeholder for the output #QmiDmsDellFirmwareVersionType, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Version Type' field from @self.
 *
 * Returns: %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_message_dms_foxconn_get_firmware_version_input_get_version_type() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_foxconn_get_firmware_version_input_get_version_type)
gboolean qmi_message_dms_dell_get_firmware_version_input_get_version_type (
    QmiDeprecatedMessageDmsDellGetFirmwareVersionInput *self,
    QmiDeprecatedDmsDellFirmwareVersionType *value_version_type,
    GError **error);

/**
 * qmi_message_dms_dell_get_firmware_version_input_set_version_type:
 * @self: a #QmiMessageDmsDellGetFirmwareVersionInput.
 * @value_version_type: a #QmiDmsDellFirmwareVersionType.
 * @error: Return location for error or %NULL.
 *
 * Set the 'Version Type' field in the message.
 *
 * Returns: %TRUE if @value was successfully set, %FALSE otherwise.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_message_dms_foxconn_get_firmware_version_input_set_version_type() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_foxconn_get_firmware_version_input_set_version_type)
gboolean qmi_message_dms_dell_get_firmware_version_input_set_version_type (
    QmiDeprecatedMessageDmsDellGetFirmwareVersionInput *self,
    QmiDeprecatedDmsDellFirmwareVersionType value_version_type,
    GError **error);

/**
 * qmi_message_dms_dell_get_firmware_version_input_ref:
 * @self: a #QmiMessageDmsDellGetFirmwareVersionInput.
 *
 * Atomically increments the reference count of @self by one.
 *
 * Returns: the new reference to @self.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_message_dms_foxconn_get_firmware_version_input_ref() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_foxconn_get_firmware_version_input_ref)
QmiDeprecatedMessageDmsDellGetFirmwareVersionInput *qmi_message_dms_dell_get_firmware_version_input_ref (QmiDeprecatedMessageDmsDellGetFirmwareVersionInput *self);

/**
 * qmi_message_dms_dell_get_firmware_version_input_unref:
 * @self: a #QmiMessageDmsDellGetFirmwareVersionInput.
 *
 * Atomically decrements the reference count of @self by one.
 * If the reference count drops to 0, @self is completely disposed.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_message_dms_foxconn_get_firmware_version_input_unref() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_foxconn_get_firmware_version_input_unref)
void qmi_message_dms_dell_get_firmware_version_input_unref (QmiDeprecatedMessageDmsDellGetFirmwareVersionInput *self);

/**
 * qmi_message_dms_dell_get_firmware_version_input_new:
 *
 * Allocates a new #QmiMessageDmsDellGetFirmwareVersionInput.
 *
 * Returns: the newly created #QmiMessageDmsDellGetFirmwareVersionInput. The returned value should be freed with qmi_message_dms_dell_get_firmware_version_input_unref().
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_message_dms_foxconn_get_firmware_version_input_new() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_foxconn_get_firmware_version_input_new)
QmiDeprecatedMessageDmsDellGetFirmwareVersionInput *qmi_message_dms_dell_get_firmware_version_input_new (void);

/**
 * QmiMessageDmsDellGetFirmwareVersionOutput:
 *
 * The #QmiMessageDmsDellGetFirmwareVersionOutput structure contains private data and should only be accessed
 * using the provided API.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use #QmiMessageDmsFoxconnGetFirmwareVersionOutput instead.
 */
typedef QmiMessageDmsFoxconnGetFirmwareVersionOutput QmiDeprecatedMessageDmsDellGetFirmwareVersionOutput;
G_DEPRECATED_FOR (QmiMessageDmsFoxconnGetFirmwareVersionOutput)
typedef QmiDeprecatedMessageDmsDellGetFirmwareVersionOutput QmiMessageDmsDellGetFirmwareVersionOutput;

#define QMI_TYPE_MESSAGE_DMS_DELL_GET_FIRMWARE_VERSION_OUTPUT QMI_TYPE_MESSAGE_DMS_FOXCONN_GET_FIRMWARE_VERSION_OUTPUT

G_DEPRECATED_FOR (qmi_message_dms_foxconn_get_firmware_version_output_get_type)
GType qmi_message_dms_dell_get_firmware_version_output_get_type (void) G_GNUC_CONST;

/**
 * qmi_message_dms_dell_get_firmware_version_output_get_result:
 * @self: a QmiMessageDmsDellGetFirmwareVersionOutput.
 * @error: Return location for error or %NULL.
 *
 * Get the result of the QMI operation.
 *
 * Returns: %TRUE if the QMI operation succeeded, %FALSE if @error is set.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_message_dms_foxconn_get_firmware_version_output_get_result() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_foxconn_get_firmware_version_output_get_result)
gboolean qmi_message_dms_dell_get_firmware_version_output_get_result (
    QmiDeprecatedMessageDmsDellGetFirmwareVersionOutput *self,
    GError **error);

/**
 * qmi_message_dms_dell_get_firmware_version_output_get_version:
 * @self: a #QmiMessageDmsDellGetFirmwareVersionOutput.
 * @value_version: a placeholder for the output constant string, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Version' field from @self.
 *
 * Returns: %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_message_dms_foxconn_get_firmware_version_output_get_version() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_foxconn_get_firmware_version_output_get_version)
gboolean qmi_message_dms_dell_get_firmware_version_output_get_version (
    QmiDeprecatedMessageDmsDellGetFirmwareVersionOutput *self,
    const gchar **value_version,
    GError **error);

/**
 * qmi_message_dms_dell_get_firmware_version_output_ref:
 * @self: a #QmiMessageDmsDellGetFirmwareVersionOutput.
 *
 * Atomically increments the reference count of @self by one.
 *
 * Returns: the new reference to @self.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_message_dms_foxconn_get_firmware_version_output_ref() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_foxconn_get_firmware_version_output_ref)
QmiDeprecatedMessageDmsDellGetFirmwareVersionOutput *qmi_message_dms_dell_get_firmware_version_output_ref (QmiDeprecatedMessageDmsDellGetFirmwareVersionOutput *self);

/**
 * qmi_message_dms_dell_get_firmware_version_output_unref:
 * @self: a #QmiMessageDmsDellGetFirmwareVersionOutput.
 *
 * Atomically decrements the reference count of @self by one.
 * If the reference count drops to 0, @self is completely disposed.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_message_dms_foxconn_get_firmware_version_output_unref() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_foxconn_get_firmware_version_output_unref)
void qmi_message_dms_dell_get_firmware_version_output_unref (QmiDeprecatedMessageDmsDellGetFirmwareVersionOutput *self);

/**
 * qmi_client_dms_dell_get_firmware_version:
 * @self: a #QmiClientDms.
 * @input: a #QmiMessageDmsDellGetFirmwareVersionInput.
 * @timeout: maximum time to wait for the method to complete, in seconds.
 * @cancellable: a #GCancellable or %NULL.
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: user data to pass to @callback.
 *
 * Asynchronously sends a Dell Get Firmware Version request to the device.
 *
 * When the operation is finished, @callback will be invoked in the thread-default main loop of the thread you are calling this method from.
 *
 * You can then call qmi_client_dms_dell_get_firmware_version_finish() to get the result of the operation.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_client_dms_foxconn_get_firmware_version() instead.
 */
G_DEPRECATED_FOR (qmi_client_dms_foxconn_get_firmware_version)
void qmi_client_dms_dell_get_firmware_version (
    QmiClientDms *self,
    QmiDeprecatedMessageDmsDellGetFirmwareVersionInput *input,
    guint timeout,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data);

/**
 * qmi_client_dms_dell_get_firmware_version_finish:
 * @self: a #QmiClientDms.
 * @res: the #GAsyncResult obtained from the #GAsyncReadyCallback passed to qmi_client_dms_dell_get_firmware_version().
 * @error: Return location for error or %NULL.
 *
 * Finishes an async operation started with qmi_client_dms_dell_get_firmware_version().
 *
 * Returns: a #QmiMessageDmsDellGetFirmwareVersionOutput, or %NULL if @error is set. The returned value should be freed with qmi_message_dms_dell_get_firmware_version_output_unref().
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_client_dms_foxconn_get_firmware_version_finish() instead.
 */
G_DEPRECATED_FOR (qmi_client_dms_foxconn_get_firmware_version_finish)
QmiDeprecatedMessageDmsDellGetFirmwareVersionOutput *qmi_client_dms_dell_get_firmware_version_finish (
    QmiClientDms *self,
    GAsyncResult *res,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_DMS_FOXCONN_GET_FIRMWARE_VERSION */

#if defined HAVE_QMI_MESSAGE_DMS_FOXCONN_CHANGE_DEVICE_MODE

/**
 * QmiMessageDmsDellChangeDeviceModeInput:
 *
 * The #QmiMessageDmsDellChangeDeviceModeInput structure contains private data and should only be accessed
 * using the provided API.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use #QmiMessageDmsFoxconnChangeDeviceModeInput instead.
 */
typedef QmiMessageDmsFoxconnChangeDeviceModeInput QmiDeprecatedMessageDmsDellChangeDeviceModeInput;
G_DEPRECATED_FOR (QmiMessageDmsFoxconnChangeDeviceModeInput)
typedef QmiDeprecatedMessageDmsDellChangeDeviceModeInput QmiMessageDmsDellChangeDeviceModeInput;

#define QMI_TYPE_MESSAGE_DMS_DELL_CHANGE_DEVICE_MODE_INPUT QMI_TYPE_MESSAGE_DMS_FOXCONN_CHANGE_DEVICE_MODE_INPUT

G_DEPRECATED_FOR (qmi_message_dms_foxconn_change_device_mode_input_get_type)
GType qmi_message_dms_dell_change_device_mode_input_get_type (void) G_GNUC_CONST;

/**
 * qmi_message_dms_dell_change_device_mode_input_get_mode:
 * @self: a #QmiMessageDmsDellChangeDeviceModeInput.
 * @value_mode: a placeholder for the output #QmiDmsDellDeviceMode, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Mode' field from @self.
 *
 * Returns: %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_message_dms_foxconn_change_device_mode_input_get_mode() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_foxconn_change_device_mode_input_get_mode)
gboolean qmi_message_dms_dell_change_device_mode_input_get_mode (
    QmiDeprecatedMessageDmsDellChangeDeviceModeInput *self,
    QmiDeprecatedDmsDellDeviceMode *value_mode,
    GError **error);

/**
 * qmi_message_dms_dell_change_device_mode_input_set_mode:
 * @self: a #QmiMessageDmsDellChangeDeviceModeInput.
 * @value_mode: a #QmiDmsDellDeviceMode.
 * @error: Return location for error or %NULL.
 *
 * Set the 'Mode' field in the message.
 *
 * Returns: %TRUE if @value was successfully set, %FALSE otherwise.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_message_dms_foxconn_change_device_mode_input_set_mode() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_foxconn_change_device_mode_input_set_mode)
gboolean qmi_message_dms_dell_change_device_mode_input_set_mode (
    QmiDeprecatedMessageDmsDellChangeDeviceModeInput *self,
    QmiDeprecatedDmsDellDeviceMode value_mode,
    GError **error);

/**
 * qmi_message_dms_dell_change_device_mode_input_ref:
 * @self: a #QmiMessageDmsDellChangeDeviceModeInput.
 *
 * Atomically increments the reference count of @self by one.
 *
 * Returns: the new reference to @self.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_message_dms_foxconn_change_device_mode_input_ref() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_foxconn_change_device_mode_input_ref)
QmiDeprecatedMessageDmsDellChangeDeviceModeInput *qmi_message_dms_dell_change_device_mode_input_ref (QmiDeprecatedMessageDmsDellChangeDeviceModeInput *self);

/**
 * qmi_message_dms_dell_change_device_mode_input_unref:
 * @self: a #QmiMessageDmsDellChangeDeviceModeInput.
 *
 * Atomically decrements the reference count of @self by one.
 * If the reference count drops to 0, @self is completely disposed.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_message_dms_foxconn_change_device_mode_input_unref() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_foxconn_change_device_mode_input_unref)
void qmi_message_dms_dell_change_device_mode_input_unref (QmiDeprecatedMessageDmsDellChangeDeviceModeInput *self);

/**
 * qmi_message_dms_dell_change_device_mode_input_new:
 *
 * Allocates a new #QmiMessageDmsDellChangeDeviceModeInput.
 *
 * Returns: the newly created #QmiMessageDmsDellChangeDeviceModeInput. The returned value should be freed with qmi_message_dms_dell_change_device_mode_input_unref().
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_message_dms_foxconn_change_device_mode_input_new() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_foxconn_change_device_mode_input_new)
QmiDeprecatedMessageDmsDellChangeDeviceModeInput *qmi_message_dms_dell_change_device_mode_input_new (void);

/**
 * QmiMessageDmsDellChangeDeviceModeOutput:
 *
 * The #QmiMessageDmsDellChangeDeviceModeOutput structure contains private data and should only be accessed
 * using the provided API.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use #QmiMessageDmsFoxconnChangeDeviceModeOutput instead.
 */
typedef QmiMessageDmsFoxconnChangeDeviceModeOutput QmiDeprecatedMessageDmsDellChangeDeviceModeOutput;
G_DEPRECATED_FOR (QmiMessageDmsFoxconnChangeDeviceModeOutput)
typedef QmiDeprecatedMessageDmsDellChangeDeviceModeOutput QmiMessageDmsDellChangeDeviceModeOutput;

#define QMI_TYPE_MESSAGE_DMS_DELL_CHANGE_DEVICE_MODE_OUTPUT QMI_TYPE_MESSAGE_DMS_FOXCONN_CHANGE_DEVICE_MODE_OUTPUT

G_DEPRECATED_FOR (qmi_message_dms_foxconn_change_device_mode_output_get_type)
GType qmi_message_dms_dell_change_device_mode_output_get_type (void) G_GNUC_CONST;

/**
 * qmi_message_dms_dell_change_device_mode_output_get_result:
 * @self: a QmiMessageDmsDellChangeDeviceModeOutput.
 * @error: Return location for error or %NULL.
 *
 * Get the result of the QMI operation.
 *
 * Returns: %TRUE if the QMI operation succeeded, %FALSE if @error is set.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_message_dms_foxconn_change_device_mode_output_get_result() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_foxconn_change_device_mode_output_get_result)
gboolean qmi_message_dms_dell_change_device_mode_output_get_result (
    QmiDeprecatedMessageDmsDellChangeDeviceModeOutput *self,
    GError **error);

/**
 * qmi_message_dms_dell_change_device_mode_output_ref:
 * @self: a #QmiMessageDmsDellChangeDeviceModeOutput.
 *
 * Atomically increments the reference count of @self by one.
 *
 * Returns: the new reference to @self.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_message_dms_foxconn_change_device_mode_output_ref() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_foxconn_change_device_mode_output_ref)
QmiDeprecatedMessageDmsDellChangeDeviceModeOutput *qmi_message_dms_dell_change_device_mode_output_ref (QmiDeprecatedMessageDmsDellChangeDeviceModeOutput *self);

/**
 * qmi_message_dms_dell_change_device_mode_output_unref:
 * @self: a #QmiMessageDmsDellChangeDeviceModeOutput.
 *
 * Atomically decrements the reference count of @self by one.
 * If the reference count drops to 0, @self is completely disposed.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_message_dms_foxconn_change_device_mode_output_unref() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_foxconn_change_device_mode_output_unref)
void qmi_message_dms_dell_change_device_mode_output_unref (QmiDeprecatedMessageDmsDellChangeDeviceModeOutput *self);

/**
 * qmi_client_dms_dell_change_device_mode:
 * @self: a #QmiClientDms.
 * @input: a #QmiMessageDmsDellChangeDeviceModeInput.
 * @timeout: maximum time to wait for the method to complete, in seconds.
 * @cancellable: a #GCancellable or %NULL.
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: user data to pass to @callback.
 *
 * Asynchronously sends a Dell Change Device Mode request to the device.
 *
 * When the operation is finished, @callback will be invoked in the thread-default main loop of the thread you are calling this method from.
 *
 * You can then call qmi_client_dms_dell_change_device_mode_finish() to get the result of the operation.
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_client_dms_foxconn_change_device_mode() instead.
 */
G_DEPRECATED_FOR (qmi_client_dms_foxconn_change_device_mode)
void qmi_client_dms_dell_change_device_mode (
    QmiClientDms *self,
    QmiDeprecatedMessageDmsDellChangeDeviceModeInput *input,
    guint timeout,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data);

/**
 * qmi_client_dms_dell_change_device_mode_finish:
 * @self: a #QmiClientDms.
 * @res: the #GAsyncResult obtained from the #GAsyncReadyCallback passed to qmi_client_dms_dell_change_device_mode().
 * @error: Return location for error or %NULL.
 *
 * Finishes an async operation started with qmi_client_dms_dell_change_device_mode().
 *
 * Returns: a #QmiMessageDmsDellChangeDeviceModeOutput, or %NULL if @error is set. The returned value should be freed with qmi_message_dms_dell_change_device_mode_output_unref().
 *
 * Since: 1.22.4
 * Deprecated: 1.26: Use qmi_client_dms_foxconn_change_device_mode_finish() instead.
 */
G_DEPRECATED_FOR (qmi_client_dms_foxconn_change_device_mode_finish)
QmiDeprecatedMessageDmsDellChangeDeviceModeOutput *qmi_client_dms_dell_change_device_mode_finish (
    QmiClientDms *self,
    GAsyncResult *res,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_DMS_FOXCONN_CHANGE_DEVICE_MODE */

#if defined HAVE_QMI_MESSAGE_NAS_GET_OPERATOR_NAME

/**
 * qmi_message_nas_get_operator_name_output_get_operator_nitz_information:
 * @self: a #QmiMessageNasGetOperatorNameOutput.
 * @value_operator_nitz_information_name_encoding: a placeholder for the output #QmiNasPlmnEncodingScheme, or %NULL if not required.
 * @value_operator_nitz_information_short_country_initials: a placeholder for the output #QmiNasPlmnNameCountryInitials, or %NULL if not required.
 * @value_operator_nitz_information_long_name_spare_bits: a placeholder for the output #QmiNasPlmnNameSpareBits, or %NULL if not required.
 * @value_operator_nitz_information_short_name_spare_bits: a placeholder for the output #QmiNasPlmnNameSpareBits, or %NULL if not required.
 * @value_operator_nitz_information_long_name: a placeholder for the output constant string, or %NULL if not required.
 * @value_operator_nitz_information_short_name: a placeholder for the output constant string, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Operator NITZ Information' field from @self.
 *
 * Returns: %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.18
 * Deprecated: 1.24.6: Use qmi_message_nas_get_operator_name_output_get_nitz_information() instead.
 */
G_DEPRECATED_FOR (qmi_message_nas_get_operator_name_output_get_nitz_information)
gboolean qmi_message_nas_get_operator_name_output_get_operator_nitz_information (
    QmiMessageNasGetOperatorNameOutput *self,
    QmiNasPlmnEncodingScheme *value_operator_nitz_information_name_encoding,
    QmiNasPlmnNameCountryInitials *value_operator_nitz_information_short_country_initials,
    QmiNasPlmnNameSpareBits *value_operator_nitz_information_long_name_spare_bits,
    QmiNasPlmnNameSpareBits *value_operator_nitz_information_short_name_spare_bits,
    const gchar **value_operator_nitz_information_long_name,
    const gchar **value_operator_nitz_information_short_name,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_NAS_GET_OPERATOR_NAME */

#if defined HAVE_QMI_INDICATION_NAS_OPERATOR_NAME

/**
 * qmi_indication_nas_operator_name_output_get_operator_nitz_information:
 * @self: a #QmiIndicationNasOperatorNameOutput.
 * @value_operator_nitz_information_name_encoding: a placeholder for the output #QmiNasPlmnEncodingScheme, or %NULL if not required.
 * @value_operator_nitz_information_short_country_initials: a placeholder for the output #QmiNasPlmnNameCountryInitials, or %NULL if not required.
 * @value_operator_nitz_information_long_name_spare_bits: a placeholder for the output #QmiNasPlmnNameSpareBits, or %NULL if not required.
 * @value_operator_nitz_information_short_name_spare_bits: a placeholder for the output #QmiNasPlmnNameSpareBits, or %NULL if not required.
 * @value_operator_nitz_information_long_name: a placeholder for the output constant string, or %NULL if not required.
 * @value_operator_nitz_information_short_name: a placeholder for the output constant string, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Operator NITZ Information' field from @self.
 *
 * Returns: %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.18
 * Deprecated: 1.24.6: Use qmi_indication_nas_operator_name_output_get_nitz_information() instead.
 */
G_DEPRECATED_FOR (qmi_indication_nas_operator_name_output_get_nitz_information)
gboolean qmi_indication_nas_operator_name_output_get_operator_nitz_information (
    QmiIndicationNasOperatorNameOutput *self,
    QmiNasPlmnEncodingScheme *value_operator_nitz_information_name_encoding,
    QmiNasPlmnNameCountryInitials *value_operator_nitz_information_short_country_initials,
    QmiNasPlmnNameSpareBits *value_operator_nitz_information_long_name_spare_bits,
    QmiNasPlmnNameSpareBits *value_operator_nitz_information_short_name_spare_bits,
    const gchar **value_operator_nitz_information_long_name,
    const gchar **value_operator_nitz_information_short_name,
    GError **error);

#endif /* HAVE_QMI_INDICATION_NAS_OPERATOR_NAME */

#if defined HAVE_QMI_MESSAGE_NAS_GET_HOME_NETWORK

/**
 * qmi_message_nas_get_home_network_output_get_home_network_3gpp2:
 * @self: a #QmiMessageNasGetHomeNetworkOutput.
 * @value_home_network_3gpp2_mcc: a placeholder for the output #guint16, or %NULL if not required.
 * @value_home_network_3gpp2_mnc: a placeholder for the output #guint16, or %NULL if not required.
 * @value_home_network_3gpp2_display_description: a placeholder for the output #QmiNasNetworkDescriptionDisplay, or %NULL if not required.
 * @value_home_network_3gpp2_description_encoding: a placeholder for the output #QmiNasNetworkDescriptionEncoding, or %NULL if not required.
 * @value_home_network_3gpp2_description: a placeholder for the output constant string, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Home Network 3GPP2' field from @self.
 *
 * Returns: %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.0
 * Deprecated: 1.24.6: Use qmi_message_nas_get_home_network_output_get_home_network_3gpp2_ext() instead.
 */
G_DEPRECATED_FOR (qmi_message_nas_get_home_network_output_get_home_network_3gpp2_ext)
gboolean qmi_message_nas_get_home_network_output_get_home_network_3gpp2 (
    QmiMessageNasGetHomeNetworkOutput *self,
    guint16 *value_home_network_3gpp2_mcc,
    guint16 *value_home_network_3gpp2_mnc,
    QmiNasNetworkDescriptionDisplay *value_home_network_3gpp2_display_description,
    QmiNasNetworkDescriptionEncoding *value_home_network_3gpp2_description_encoding,
    const gchar **value_home_network_3gpp2_description,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_NAS_GET_HOME_NETWORK */

#if defined HAVE_QMI_MESSAGE_NAS_GET_CELL_LOCATION_INFO

/**
 * QmiMessageNasGetCellLocationInfoOutputGeranInfoCellElement:
 * @cell_id: a #guint32.
 * @plmn: a string of exactly 3 characters.
 * @lac: a #guint16.
 * @geran_absolute_rf_channel_number: a #guint16.
 * @base_station_identity_code: a #guint8.
 * @rx_level: a #guint16.
 *
 * GERAN info cell.
 *
 * This type is deprecated and returns always an empty @plmn string.
 *
 * Since: 1.10
 * Deprecated: 1.26.6.
 */
G_DEPRECATED
typedef struct _QmiMessageNasGetCellLocationInfoOutputGeranInfoCellElement {
    guint32 cell_id;
    gchar *plmn;
    guint16 lac;
    guint16 geran_absolute_rf_channel_number;
    guint8 base_station_identity_code;
    guint16 rx_level;
} QmiMessageNasGetCellLocationInfoOutputGeranInfoCellElement;

/**
 * QmiMessageNasGetCellLocationInfoOutputIntrafrequencyLteInfoCellElement:
 *
 * Intra-frequency LTE info cell.
 *
 * The format of this deprecated type is really the same as for the
 * new #QmiMessageNasGetCellLocationInfoOutputIntrafrequencyLteInfoV2CellElement type,
 * it is only a name change.
 *
 * Since: 1.10
 * Deprecated: 1.26.6. Use #QmiMessageNasGetCellLocationInfoOutputIntrafrequencyLteInfoV2CellElement instead.
 */
G_DEPRECATED_FOR (QmiMessageNasGetCellLocationInfoOutputIntrafrequencyLteInfoV2CellElement)
typedef QmiMessageNasGetCellLocationInfoOutputIntrafrequencyLteInfoV2CellElement QmiMessageNasGetCellLocationInfoOutputIntrafrequencyLteInfoCellElement;

/**
 * qmi_message_nas_get_cell_location_info_output_get_intrafrequency_lte_info:
 * @self: a #QmiMessageNasGetCellLocationInfoOutput.
 * @value_intrafrequency_lte_info_ue_in_idle: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_intrafrequency_lte_info_plmn: (out): a placeholder for the output constant string, or %NULL if not required.
 * @value_intrafrequency_lte_info_tracking_area_code: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @value_intrafrequency_lte_info_global_cell_id: (out): a placeholder for the output #guint32, or %NULL if not required.
 * @value_intrafrequency_lte_info_eutra_absolute_rf_channel_number: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @value_intrafrequency_lte_info_serving_cell_id: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @value_intrafrequency_lte_info_cell_reselection_priority: (out): a placeholder for the output #guint8, or %NULL if not required.
 * @value_intrafrequency_lte_info_s_non_intra_search_threshold: (out): a placeholder for the output #guint8, or %NULL if not required.
 * @value_intrafrequency_lte_info_serving_cell_low_threshold: (out): a placeholder for the output #guint8, or %NULL if not required.
 * @value_intrafrequency_lte_info_s_intra_search_threshold: (out): a placeholder for the output #guint8, or %NULL if not required.
 * @value_intrafrequency_lte_info_cell: (out)(element-type QmiMessageNasGetCellLocationInfoOutputIntrafrequencyLteInfoCellElement)(transfer none): a placeholder for the output #GArray of #QmiMessageNasGetCellLocationInfoOutputIntrafrequencyLteInfoCellElement elements, or %NULL if not required. Do not free it, it is owned by @self.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Intrafrequency LTE Info' field from @self.
 *
 * This method is deprecated and returns an empty
 * @value_intrafrequency_lte_info_plmn string.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.10
 * Deprecated: 1.26.6. Use qmi_message_nas_get_cell_location_info_output_get_intrafrequency_lte_info_v2() instead.
 */
G_DEPRECATED_FOR (qmi_message_nas_get_cell_location_info_output_get_intrafrequency_lte_info_v2)
gboolean qmi_message_nas_get_cell_location_info_output_get_intrafrequency_lte_info (
    QmiMessageNasGetCellLocationInfoOutput *self,
    gboolean *value_intrafrequency_lte_info_ue_in_idle,
    const gchar **value_intrafrequency_lte_info_plmn,
    guint16 *value_intrafrequency_lte_info_tracking_area_code,
    guint32 *value_intrafrequency_lte_info_global_cell_id,
    guint16 *value_intrafrequency_lte_info_eutra_absolute_rf_channel_number,
    guint16 *value_intrafrequency_lte_info_serving_cell_id,
    guint8 *value_intrafrequency_lte_info_cell_reselection_priority,
    guint8 *value_intrafrequency_lte_info_s_non_intra_search_threshold,
    guint8 *value_intrafrequency_lte_info_serving_cell_low_threshold,
    guint8 *value_intrafrequency_lte_info_s_intra_search_threshold,
    GArray **value_intrafrequency_lte_info_cell,
    GError **error);

/**
 * QmiMessageNasGetCellLocationInfoOutputUmtsInfoCellElement:
 *
 * UMTS info cell.
 *
 * The format of this deprecated type is really the same as for the
 * new #QmiMessageNasGetCellLocationInfoOutputUmtsInfoV2CellElement type,
 * it is only a name change.
 *
 * Since: 1.10
 * Deprecated: 1.26.6. Use #QmiMessageNasGetCellLocationInfoOutputUmtsInfoV2CellElement instead.
 */
G_DEPRECATED_FOR (QmiMessageNasGetCellLocationInfoOutputUmtsInfoV2CellElement)
typedef QmiMessageNasGetCellLocationInfoOutputUmtsInfoV2CellElement QmiMessageNasGetCellLocationInfoOutputUmtsInfoCellElement;

/**
 * QmiMessageNasGetCellLocationInfoOutputUmtsInfoNeighboringGeranElement:
 *
 * UMTS info neighboring GERAN cell.
 *
 * The format of this deprecated type is really the same as for the
 * new #QmiMessageNasGetCellLocationInfoOutputUmtsInfoV2NeighboringGeranElement type,
 * it is only a name change.
 *
 * Since: 1.10
 * Deprecated: 1.26.6. Use #QmiMessageNasGetCellLocationInfoOutputUmtsInfoV2NeighboringGeranElement instead.
 */
G_DEPRECATED_FOR (QmiMessageNasGetCellLocationInfoOutputUmtsInfoV2NeighboringGeranElement)
typedef QmiMessageNasGetCellLocationInfoOutputUmtsInfoV2NeighboringGeranElement QmiMessageNasGetCellLocationInfoOutputUmtsInfoNeighboringGeranElement;

/**
 * qmi_message_nas_get_cell_location_info_output_get_umts_info:
 * @self: a #QmiMessageNasGetCellLocationInfoOutput.
 * @value_umts_info_cell_id: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @value_umts_info_plmn: (out): a placeholder for the output constant string, or %NULL if not required.
 * @value_umts_info_lac: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @value_umts_info_utra_absolute_rf_channel_number: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @value_umts_info_primary_scrambling_code: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @value_umts_info_rscp: (out): a placeholder for the output #gint16, or %NULL if not required.
 * @value_umts_info_ecio: (out): a placeholder for the output #gint16, or %NULL if not required.
 * @value_umts_info_cell: (out)(element-type QmiMessageNasGetCellLocationInfoOutputUmtsInfoCellElement)(transfer none): a placeholder for the output #GArray of #QmiMessageNasGetCellLocationInfoOutputUmtsInfoCellElement elements, or %NULL if not required. Do not free it, it is owned by @self.
 * @value_umts_info_neighboring_geran: (out)(element-type QmiMessageNasGetCellLocationInfoOutputUmtsInfoNeighboringGeranElement)(transfer none): a placeholder for the output #GArray of #QmiMessageNasGetCellLocationInfoOutputUmtsInfoNeighboringGeranElement elements, or %NULL if not required. Do not free it, it is owned by @self.
 * @error: Return location for error or %NULL.
 *
 * Get the 'UMTS Info' field from @self.
 *
 * This method is deprecated and returns an empty @value_umts_info_plmn
 * string.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.10
 * Deprecated: 1.26.6. Use qmi_message_nas_get_cell_location_info_output_get_umts_info_v2() instead.
 */
G_DEPRECATED_FOR (qmi_message_nas_get_cell_location_info_output_get_umts_info_v2)
gboolean qmi_message_nas_get_cell_location_info_output_get_umts_info (
    QmiMessageNasGetCellLocationInfoOutput *self,
    guint16 *value_umts_info_cell_id,
    const gchar **value_umts_info_plmn,
    guint16 *value_umts_info_lac,
    guint16 *value_umts_info_utra_absolute_rf_channel_number,
    guint16 *value_umts_info_primary_scrambling_code,
    gint16 *value_umts_info_rscp,
    gint16 *value_umts_info_ecio,
    GArray **value_umts_info_cell,
    GArray **value_umts_info_neighboring_geran,
    GError **error);

/**
 * qmi_message_nas_get_cell_location_info_output_get_geran_info:
 * @self: a #QmiMessageNasGetCellLocationInfoOutput.
 * @value_geran_info_cell_id: (out): a placeholder for the output #guint32, or %NULL if not required.
 * @value_geran_info_plmn: (out): a placeholder for the output constant string, or %NULL if not required.
 * @value_geran_info_lac: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @value_geran_info_geran_absolute_rf_channel_number: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @value_geran_info_base_station_identity_code: (out): a placeholder for the output #guint8, or %NULL if not required.
 * @value_geran_info_timing_advance: (out): a placeholder for the output #guint32, or %NULL if not required.
 * @value_geran_info_rx_level: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @value_geran_info_cell: (out)(element-type QmiMessageNasGetCellLocationInfoOutputGeranInfoCellElement)(transfer none): a placeholder for the output #GArray of #QmiMessageNasGetCellLocationInfoOutputGeranInfoCellElement elements, or %NULL if not required. Do not free it, it is owned by @self.
 * @error: Return location for error or %NULL.
 *
 * Get the 'GERAN Info' field from @self.
 *
 * This method is deprecated and returns an empty @value_geran_info_plmn
 * string.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.10
 * Deprecated: 1.26.6. Use qmi_message_nas_get_cell_location_info_output_get_geran_info_v2() instead.
 */
G_DEPRECATED_FOR (qmi_message_nas_get_cell_location_info_output_get_geran_info_v2)
gboolean qmi_message_nas_get_cell_location_info_output_get_geran_info (
    QmiMessageNasGetCellLocationInfoOutput *self,
    guint32 *value_geran_info_cell_id,
    const gchar **value_geran_info_plmn,
    guint16 *value_geran_info_lac,
    guint16 *value_geran_info_geran_absolute_rf_channel_number,
    guint8 *value_geran_info_base_station_identity_code,
    guint32 *value_geran_info_timing_advance,
    guint16 *value_geran_info_rx_level,
    GArray **value_geran_info_cell,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_NAS_GET_CELL_LOCATION_INFO */

#if defined HAVE_QMI_MESSAGE_WDS_GET_DEFAULT_PROFILE_NUMBER

/**
 * QmiMessageWdsGetDefaultProfileNumInput:
 *
 * The #QmiMessageWdsGetDefaultProfileNumInput structure contains private data and should only be accessed
 * using the provided API.
 *
 * Since: 1.22
 * Deprecated: 1.28: Use #QmiMessageWdsGetDefaultProfileNumberInput instead.
 */
typedef QmiMessageWdsGetDefaultProfileNumberInput QmiDeprecatedMessageWdsGetDefaultProfileNumInput;
G_DEPRECATED_FOR (QmiMessageWdsGetDefaultProfileNumberInput)
typedef QmiDeprecatedMessageWdsGetDefaultProfileNumInput QmiMessageWdsGetDefaultProfileNumInput;

#define QMI_TYPE_MESSAGE_WDS_GET_DEFAULT_PROFILE_NUM_INPUT QMI_TYPE_MESSAGE_WDS_GET_DEFAULT_PROFILE_NUMBER_INPUT

G_DEPRECATED_FOR (qmi_message_wds_get_default_profile_number_input_get_type)
GType qmi_message_wds_get_default_profile_num_input_get_type (void) G_GNUC_CONST;

/**
 * qmi_message_wds_get_default_profile_num_input_get_profile_type:
 * @self: a #QmiMessageWdsGetDefaultProfileNumInput.
 * @value_profile_type_profile_type: (out): a placeholder for the output #QmiWdsProfileType, or %NULL if not required.
 * @value_profile_type_profile_family: (out): a placeholder for the output #QmiWdsProfileFamily, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Profile Type' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_message_wds_get_default_profile_number_input_get_profile_type() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_get_default_profile_number_input_get_profile_type)
gboolean qmi_message_wds_get_default_profile_num_input_get_profile_type (
    QmiDeprecatedMessageWdsGetDefaultProfileNumInput *self,
    QmiWdsProfileType *value_profile_type_profile_type,
    QmiWdsProfileFamily *value_profile_type_profile_family,
    GError **error);

/**
 * qmi_message_wds_get_default_profile_num_input_set_profile_type:
 * @self: a #QmiMessageWdsGetDefaultProfileNumInput.
 * @value_profile_type_profile_type: a #QmiWdsProfileType.
 * @value_profile_type_profile_family: a #QmiWdsProfileFamily.
 * @error: Return location for error or %NULL.
 *
 * Set the 'Profile Type' field in the message.
 *
 * Returns: (skip): %TRUE if @value was successfully set, %FALSE otherwise.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_message_wds_get_default_profile_number_input_set_profile_type() instead.
 */
G_DEPRECATED_FOR (Use qmi_message_wds_get_default_profile_number_input_set_profile_type)
gboolean qmi_message_wds_get_default_profile_num_input_set_profile_type (
    QmiDeprecatedMessageWdsGetDefaultProfileNumInput *self,
    QmiWdsProfileType value_profile_type_profile_type,
    QmiWdsProfileFamily value_profile_type_profile_family,
    GError **error);

/**
 * qmi_message_wds_get_default_profile_num_input_ref:
 * @self: a #QmiMessageWdsGetDefaultProfileNumInput.
 *
 * Atomically increments the reference count of @self by one.
 *
 * Returns: the new reference to @self.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_message_wds_get_default_profile_number_input_ref() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_get_default_profile_number_input_ref)
QmiDeprecatedMessageWdsGetDefaultProfileNumInput *qmi_message_wds_get_default_profile_num_input_ref (QmiDeprecatedMessageWdsGetDefaultProfileNumInput *self);

/**
 * qmi_message_wds_get_default_profile_num_input_unref:
 * @self: a #QmiMessageWdsGetDefaultProfileNumInput.
 *
 * Atomically decrements the reference count of @self by one.
 * If the reference count drops to 0, @self is completely disposed.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_message_wds_get_default_profile_number_input_unref() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_get_default_profile_number_input_unref)
void qmi_message_wds_get_default_profile_num_input_unref (QmiDeprecatedMessageWdsGetDefaultProfileNumInput *self);

/**
 * qmi_message_wds_get_default_profile_num_input_new:
 *
 * Allocates a new #QmiMessageWdsGetDefaultProfileNumInput.
 *
 * Returns: the newly created #QmiMessageWdsGetDefaultProfileNumInput. The returned value should be freed with qmi_message_wds_get_default_profile_num_input_unref().
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_message_wds_get_default_profile_number_input_new() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_get_default_profile_number_input_new)
QmiDeprecatedMessageWdsGetDefaultProfileNumInput *qmi_message_wds_get_default_profile_num_input_new (void);

/**
 * QmiMessageWdsGetDefaultProfileNumOutput:
 *
 * The #QmiMessageWdsGetDefaultProfileNumOutput structure contains private data and should only be accessed
 * using the provided API.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use #QmiMessageWdsGetDefaultProfileNumberOutput instead.
 */
typedef QmiMessageWdsGetDefaultProfileNumberOutput QmiDeprecatedMessageWdsGetDefaultProfileNumOutput;
G_DEPRECATED_FOR (QmiMessageWdsGetDefaultProfileNumberOutput)
typedef QmiDeprecatedMessageWdsGetDefaultProfileNumOutput QmiMessageWdsGetDefaultProfileNumOutput;

#define QMI_TYPE_MESSAGE_WDS_GET_DEFAULT_PROFILE_NUM_OUTPUT QMI_TYPE_MESSAGE_WDS_GET_DEFAULT_PROFILE_NUMBER_OUTPUT

G_DEPRECATED_FOR (qmi_message_wds_get_default_profile_number_output_get_type)
GType qmi_message_wds_get_default_profile_num_output_get_type (void) G_GNUC_CONST;

/**
 * qmi_message_wds_get_default_profile_num_output_get_result:
 * @self: a QmiMessageWdsGetDefaultProfileNumOutput.
 * @error: Return location for error or %NULL.
 *
 * Get the result of the QMI operation.
 *
 * Returns: (skip): %TRUE if the QMI operation succeeded, %FALSE if @error is set.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_message_wds_get_default_profile_number_output_get_result() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_get_default_profile_number_output_get_result)
gboolean qmi_message_wds_get_default_profile_num_output_get_result (
    QmiDeprecatedMessageWdsGetDefaultProfileNumOutput *self,
    GError **error);

/**
 * qmi_message_wds_get_default_profile_num_output_get_default_profile_number:
 * @self: a #QmiMessageWdsGetDefaultProfileNumOutput.
 * @value_default_profile_number: (out): a placeholder for the output #guint8, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Default Profile Number' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_message_wds_get_default_profile_number_output_get_index() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_get_default_profile_number_output_get_index)
gboolean qmi_message_wds_get_default_profile_num_output_get_default_profile_number (
    QmiDeprecatedMessageWdsGetDefaultProfileNumOutput *self,
    guint8 *value_default_profile_number,
    GError **error);

/**
 * qmi_message_wds_get_default_profile_num_output_get_extended_error_code:
 * @self: a #QmiMessageWdsGetDefaultProfileNumOutput.
 * @value_extended_error_code: (out): a placeholder for the output #QmiWdsDsProfileError, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Extended Error Code' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_message_wds_get_default_profile_number_output_get_extended_error_code() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_get_default_profile_number_output_get_extended_error_code)
gboolean qmi_message_wds_get_default_profile_num_output_get_extended_error_code (
    QmiDeprecatedMessageWdsGetDefaultProfileNumOutput *self,
    QmiWdsDsProfileError *value_extended_error_code,
    GError **error);

/**
 * qmi_message_wds_get_default_profile_num_output_ref:
 * @self: a #QmiMessageWdsGetDefaultProfileNumOutput.
 *
 * Atomically increments the reference count of @self by one.
 *
 * Returns: the new reference to @self.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_message_wds_get_default_profile_number_output_ref() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_get_default_profile_number_output_ref)
QmiDeprecatedMessageWdsGetDefaultProfileNumOutput *qmi_message_wds_get_default_profile_num_output_ref (QmiDeprecatedMessageWdsGetDefaultProfileNumOutput *self);

/**
 * qmi_message_wds_get_default_profile_num_output_unref:
 * @self: a #QmiMessageWdsGetDefaultProfileNumOutput.
 *
 * Atomically decrements the reference count of @self by one.
 * If the reference count drops to 0, @self is completely disposed.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_message_wds_get_default_profile_number_output_unref() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_get_default_profile_number_output_unref)
void qmi_message_wds_get_default_profile_num_output_unref (QmiDeprecatedMessageWdsGetDefaultProfileNumOutput *self);

/**
 * qmi_client_wds_get_default_profile_num:
 * @self: a #QmiClientWds.
 * @input: a #QmiMessageWdsGetDefaultProfileNumberInput.
 * @timeout: maximum time to wait for the method to complete, in seconds.
 * @cancellable: a #GCancellable or %NULL.
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: user data to pass to @callback.
 *
 * Asynchronously sends a Get Default Profile Number request to the device.
 *
 * When the operation is finished, @callback will be invoked in the thread-default main loop of the thread you are calling this method from.
 *
 * You can then call qmi_client_wds_get_default_profile_number_finish() to get the result of the operation.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_client_wds_get_default_profile_number() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_get_default_profile_number)
void qmi_client_wds_get_default_profile_num (
    QmiClientWds *self,
    QmiDeprecatedMessageWdsGetDefaultProfileNumInput *input,
    guint timeout,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data);

/**
 * qmi_client_wds_get_default_profile_num_finish:
 * @self: a #QmiClientWds.
 * @res: the #GAsyncResult obtained from the #GAsyncReadyCallback passed to qmi_client_wds_get_default_profile_number().
 * @error: Return location for error or %NULL.
 *
 * Finishes an async operation started with qmi_client_wds_get_default_profile_number().
 *
 * Returns: a #QmiMessageWdsGetDefaultProfileNumberOutput, or %NULL if @error is set. The returned value should be freed with qmi_message_wds_get_default_profile_number_output_unref().
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_client_wds_get_default_profile_number_finish() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_get_default_profile_number_finish)
QmiDeprecatedMessageWdsGetDefaultProfileNumOutput *qmi_client_wds_get_default_profile_num_finish (
    QmiClientWds *self,
    GAsyncResult *res,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_WDS_GET_DEFAULT_PROFILE_NUMBER */

#if defined HAVE_QMI_MESSAGE_WDS_SET_DEFAULT_PROFILE_NUMBER

/**
 * QmiMessageWdsSetDefaultProfileNumInput:
 *
 * The #QmiMessageWdsSetDefaultProfileNumInput structure contains private data and should only be accessed
 * using the provided API.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use #QmiMessageWdsSetDefaultProfileNumberInput instead.
 */
typedef QmiMessageWdsSetDefaultProfileNumberInput QmiDeprecatedMessageWdsSetDefaultProfileNumInput;
G_DEPRECATED_FOR (QmiMessageWdsSetDefaultProfileNumberInput)
typedef QmiDeprecatedMessageWdsSetDefaultProfileNumInput QmiMessageWdsSetDefaultProfileNumInput;

#define QMI_TYPE_MESSAGE_WDS_SET_DEFAULT_PROFILE_NUM_INPUT QMI_TYPE_MESSAGE_WDS_SET_DEFAULT_PROFILE_NUMBER_INPUT

G_DEPRECATED_FOR (qmi_message_wds_set_default_profile_number_input_get_type)
GType qmi_message_wds_set_default_profile_num_input_get_type (void) G_GNUC_CONST;

/**
 * qmi_message_wds_set_default_profile_num_input_get_profile_identifier:
 * @self: a #QmiMessageWdsSetDefaultProfileNumInput.
 * @value_profile_identifier_profile_type: (out): a placeholder for the output #QmiWdsProfileType, or %NULL if not required.
 * @value_profile_identifier_profile_family: (out): a placeholder for the output #QmiWdsProfileFamily, or %NULL if not required.
 * @value_profile_identifier_profile_index: (out): a placeholder for the output #guint8, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Profile Identifier' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_message_wds_set_default_profile_number_input_get_profile_identifier() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_set_default_profile_number_input_get_profile_identifier)
gboolean qmi_message_wds_set_default_profile_num_input_get_profile_identifier (
    QmiDeprecatedMessageWdsSetDefaultProfileNumInput *self,
    QmiWdsProfileType *value_profile_identifier_profile_type,
    QmiWdsProfileFamily *value_profile_identifier_profile_family,
    guint8 *value_profile_identifier_profile_index,
    GError **error);

/**
 * qmi_message_wds_set_default_profile_num_input_set_profile_identifier:
 * @self: a #QmiMessageWdsSetDefaultProfileNumInput.
 * @value_profile_identifier_profile_type: a #QmiWdsProfileType.
 * @value_profile_identifier_profile_family: a #QmiWdsProfileFamily.
 * @value_profile_identifier_profile_index: a #guint8.
 * @error: Return location for error or %NULL.
 *
 * Set the 'Profile Identifier' field in the message.
 *
 * Returns: (skip): %TRUE if @value was successfully set, %FALSE otherwise.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_message_wds_set_default_profile_number_input_set_profile_identifier() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_set_default_profile_number_input_set_profile_identifier)
gboolean qmi_message_wds_set_default_profile_num_input_set_profile_identifier (
    QmiDeprecatedMessageWdsSetDefaultProfileNumInput *self,
    QmiWdsProfileType value_profile_identifier_profile_type,
    QmiWdsProfileFamily value_profile_identifier_profile_family,
    guint8 value_profile_identifier_profile_index,
    GError **error);

/**
 * qmi_message_wds_set_default_profile_num_input_ref:
 * @self: a #QmiMessageWdsSetDefaultProfileNumInput.
 *
 * Atomically increments the reference count of @self by one.
 *
 * Returns: the new reference to @self.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_message_wds_set_default_profile_number_input_ref() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_set_default_profile_number_input_ref)
QmiDeprecatedMessageWdsSetDefaultProfileNumInput *qmi_message_wds_set_default_profile_num_input_ref (QmiDeprecatedMessageWdsSetDefaultProfileNumInput *self);

/**
 * qmi_message_wds_set_default_profile_num_input_unref:
 * @self: a #QmiMessageWdsSetDefaultProfileNumInput.
 *
 * Atomically decrements the reference count of @self by one.
 * If the reference count drops to 0, @self is completely disposed.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_message_wds_set_default_profile_number_input_unref() instead.
 */
G_DEPRECATED_FOR (Use qmi_message_wds_set_default_profile_number_input_unref)
void qmi_message_wds_set_default_profile_num_input_unref (QmiDeprecatedMessageWdsSetDefaultProfileNumInput *self);

/**
 * qmi_message_wds_set_default_profile_num_input_new:
 *
 * Allocates a new #QmiMessageWdsSetDefaultProfileNumInput.
 *
 * Returns: the newly created #QmiMessageWdsSetDefaultProfileNumInput. The returned value should be freed with qmi_message_wds_set_default_profile_num_input_unref().
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_message_wds_set_default_profile_number_input_new() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_set_default_profile_number_input_new)
QmiDeprecatedMessageWdsSetDefaultProfileNumInput *qmi_message_wds_set_default_profile_num_input_new (void);

/**
 * QmiMessageWdsSetDefaultProfileNumOutput:
 *
 * The #QmiMessageWdsSetDefaultProfileNumOutput structure contains private data and should only be accessed
 * using the provided API.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use #QmiMessageWdsSetDefaultProfileNumberOutput instead.
 */
typedef QmiMessageWdsSetDefaultProfileNumberOutput QmiDeprecatedMessageWdsSetDefaultProfileNumOutput;
G_DEPRECATED_FOR (QmiMessageWdsSetDefaultProfileNumberOutput)
typedef QmiDeprecatedMessageWdsSetDefaultProfileNumOutput QmiMessageWdsSetDefaultProfileNumOutput;

#define QMI_TYPE_MESSAGE_WDS_SET_DEFAULT_PROFILE_NUM_OUTPUT QMI_TYPE_MESSAGE_WDS_SET_DEFAULT_PROFILE_NUMBER_OUTPUT

G_DEPRECATED_FOR (qmi_message_wds_set_default_profile_number_output_get_type)
GType qmi_message_wds_set_default_profile_num_output_get_type (void) G_GNUC_CONST;

/**
 * qmi_message_wds_set_default_profile_num_output_get_result:
 * @self: a QmiMessageWdsSetDefaultProfileNumOutput.
 * @error: Return location for error or %NULL.
 *
 * Get the result of the QMI operation.
 *
 * Returns: (skip): %TRUE if the QMI operation succeeded, %FALSE if @error is set.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_message_wds_set_default_profile_number_output_get_result() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_set_default_profile_number_output_get_result)
gboolean qmi_message_wds_set_default_profile_num_output_get_result (
    QmiDeprecatedMessageWdsSetDefaultProfileNumOutput *self,
    GError **error);

/**
 * qmi_message_wds_set_default_profile_num_output_get_extended_error_code:
 * @self: a #QmiMessageWdsSetDefaultProfileNumOutput.
 * @value_extended_error_code: (out): a placeholder for the output #QmiWdsDsProfileError, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Extended Error Code' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_message_wds_set_default_profile_number_output_get_extended_error_code() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_set_default_profile_number_output_get_extended_error_code)
gboolean qmi_message_wds_set_default_profile_num_output_get_extended_error_code (
    QmiDeprecatedMessageWdsSetDefaultProfileNumOutput *self,
    QmiWdsDsProfileError *value_extended_error_code,
    GError **error);

/**
 * qmi_message_wds_set_default_profile_num_output_ref:
 * @self: a #QmiMessageWdsSetDefaultProfileNumOutput.
 *
 * Atomically increments the reference count of @self by one.
 *
 * Returns: the new reference to @self.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_message_wds_set_default_profile_number_output_ref() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_set_default_profile_number_output_ref)
QmiDeprecatedMessageWdsSetDefaultProfileNumOutput *qmi_message_wds_set_default_profile_num_output_ref (QmiDeprecatedMessageWdsSetDefaultProfileNumOutput *self);

/**
 * qmi_message_wds_set_default_profile_num_output_unref:
 * @self: a #QmiMessageWdsSetDefaultProfileNumOutput.
 *
 * Atomically decrements the reference count of @self by one.
 * If the reference count drops to 0, @self is completely disposed.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_message_wds_set_default_profile_number_output_unref() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_set_default_profile_number_output_unref)
void qmi_message_wds_set_default_profile_num_output_unref (QmiDeprecatedMessageWdsSetDefaultProfileNumOutput *self);

/**
 * qmi_client_wds_set_default_profile_num:
 * @self: a #QmiClientWds.
 * @input: a #QmiMessageWdsSetDefaultProfileNumberInput.
 * @timeout: maximum time to wait for the method to complete, in seconds.
 * @cancellable: a #GCancellable or %NULL.
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: user data to pass to @callback.
 *
 * Asynchronously sends a Set Default Profile Number request to the device.
 *
 * When the operation is finished, @callback will be invoked in the thread-default main loop of the thread you are calling this method from.
 *
 * You can then call qmi_client_wds_set_default_profile_number_finish() to get the result of the operation.
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_client_wds_set_default_profile_number() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_set_default_profile_number)
void qmi_client_wds_set_default_profile_num (
    QmiClientWds *self,
    QmiDeprecatedMessageWdsSetDefaultProfileNumInput *input,
    guint timeout,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data);

/**
 * qmi_client_wds_set_default_profile_num_finish:
 * @self: a #QmiClientWds.
 * @res: the #GAsyncResult obtained from the #GAsyncReadyCallback passed to qmi_client_wds_set_default_profile_number().
 * @error: Return location for error or %NULL.
 *
 * Finishes an async operation started with qmi_client_wds_set_default_profile_number().
 *
 * Returns: a #QmiMessageWdsSetDefaultProfileNumberOutput, or %NULL if @error is set. The returned value should be freed with qmi_message_wds_set_default_profile_number_output_unref().
 *
 * Since: 1.22
 * Deprecated: 1.28. Use qmi_client_wds_set_default_profile_number_finish() instead.
 */
G_DEPRECATED_FOR (qmi_message_wds_set_default_profile_number_finish)
QmiDeprecatedMessageWdsSetDefaultProfileNumOutput *qmi_client_wds_set_default_profile_num_finish (
    QmiClientWds *self,
    GAsyncResult *res,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_WDS_SET_DEFAULT_PROFILE_NUMBER */

#if defined HAVE_QMI_MESSAGE_NAS_GET_SYSTEM_INFO

/**
 * qmi_message_nas_get_system_info_output_get_gsm_system_info:
 * @self: a #QmiMessageNasGetSystemInfoOutput.
 * @value_gsm_system_info_domain_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_domain: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_gsm_system_info_service_capability_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_service_capability: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_gsm_system_info_roaming_status_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_roaming_status: (out): a placeholder for the output #QmiNasRoamingStatus, or %NULL if not required.
 * @value_gsm_system_info_forbidden_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_forbidden: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_lac_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_lac: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @value_gsm_system_info_cid_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_cid: (out): a placeholder for the output #guint32, or %NULL if not required.
 * @value_gsm_system_info_registration_reject_info_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_registration_reject_domain: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_gsm_system_info_registration_reject_cause: (out): a placeholder for the output #guint8, or %NULL if not required.
 * @value_gsm_system_info_network_id_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_mcc: (out): a placeholder for the output constant string, or %NULL if not required.
 * @value_gsm_system_info_mnc: (out): a placeholder for the output constant string, or %NULL if not required.
 * @value_gsm_system_info_egprs_support_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_egprs_support: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_dtm_support_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_dtm_support: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'GSM System Info' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.0
 * Deprecated: 1.30. Use qmi_message_nas_get_system_info_output_get_gsm_system_info_v2() instead.
 */
G_DEPRECATED_FOR (qmi_message_nas_get_system_info_output_get_gsm_system_info_v2)
gboolean qmi_message_nas_get_system_info_output_get_gsm_system_info (
    QmiMessageNasGetSystemInfoOutput *self,
    gboolean *value_gsm_system_info_domain_valid,
    QmiNasNetworkServiceDomain *value_gsm_system_info_domain,
    gboolean *value_gsm_system_info_service_capability_valid,
    QmiNasNetworkServiceDomain *value_gsm_system_info_service_capability,
    gboolean *value_gsm_system_info_roaming_status_valid,
    QmiNasRoamingStatus *value_gsm_system_info_roaming_status,
    gboolean *value_gsm_system_info_forbidden_valid,
    gboolean *value_gsm_system_info_forbidden,
    gboolean *value_gsm_system_info_lac_valid,
    guint16 *value_gsm_system_info_lac,
    gboolean *value_gsm_system_info_cid_valid,
    guint32 *value_gsm_system_info_cid,
    gboolean *value_gsm_system_info_registration_reject_info_valid,
    QmiNasNetworkServiceDomain *value_gsm_system_info_registration_reject_domain,
    guint8 *value_gsm_system_info_registration_reject_cause,
    gboolean *value_gsm_system_info_network_id_valid,
    const gchar **value_gsm_system_info_mcc,
    const gchar **value_gsm_system_info_mnc,
    gboolean *value_gsm_system_info_egprs_support_valid,
    gboolean *value_gsm_system_info_egprs_support,
    gboolean *value_gsm_system_info_dtm_support_valid,
    gboolean *value_gsm_system_info_dtm_support,
    GError **error);

/**
 * qmi_message_nas_get_system_info_output_get_wcdma_system_info:
 * @self: a #QmiMessageNasGetSystemInfoOutput.
 * @value_wcdma_system_info_domain_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_domain: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_wcdma_system_info_service_capability_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_service_capability: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_wcdma_system_info_roaming_status_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_roaming_status: (out): a placeholder for the output #QmiNasRoamingStatus, or %NULL if not required.
 * @value_wcdma_system_info_forbidden_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_forbidden: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_lac_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_lac: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @value_wcdma_system_info_cid_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_cid: (out): a placeholder for the output #guint32, or %NULL if not required.
 * @value_wcdma_system_info_registration_reject_info_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_registration_reject_domain: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_wcdma_system_info_registration_reject_cause: (out): a placeholder for the output #guint8, or %NULL if not required.
 * @value_wcdma_system_info_network_id_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_mcc: (out): a placeholder for the output constant string, or %NULL if not required.
 * @value_wcdma_system_info_mnc: (out): a placeholder for the output constant string, or %NULL if not required.
 * @value_wcdma_system_info_hs_call_status_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_hs_call_status: (out): a placeholder for the output #QmiNasWcdmaHsService, or %NULL if not required.
 * @value_wcdma_system_info_hs_service_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_hs_service: (out): a placeholder for the output #QmiNasWcdmaHsService, or %NULL if not required.
 * @value_wcdma_system_info_primary_scrambling_code_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_primary_scrambling_code: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'WCDMA System Info' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.0
 * Deprecated: 1.30. Use qmi_message_nas_get_system_info_output_get_wcdma_system_info_v2() instead.
 */
G_DEPRECATED_FOR (qmi_message_nas_get_system_info_output_get_wcdma_system_info_v2)
gboolean qmi_message_nas_get_system_info_output_get_wcdma_system_info (
    QmiMessageNasGetSystemInfoOutput *self,
    gboolean *value_wcdma_system_info_domain_valid,
    QmiNasNetworkServiceDomain *value_wcdma_system_info_domain,
    gboolean *value_wcdma_system_info_service_capability_valid,
    QmiNasNetworkServiceDomain *value_wcdma_system_info_service_capability,
    gboolean *value_wcdma_system_info_roaming_status_valid,
    QmiNasRoamingStatus *value_wcdma_system_info_roaming_status,
    gboolean *value_wcdma_system_info_forbidden_valid,
    gboolean *value_wcdma_system_info_forbidden,
    gboolean *value_wcdma_system_info_lac_valid,
    guint16 *value_wcdma_system_info_lac,
    gboolean *value_wcdma_system_info_cid_valid,
    guint32 *value_wcdma_system_info_cid,
    gboolean *value_wcdma_system_info_registration_reject_info_valid,
    QmiNasNetworkServiceDomain *value_wcdma_system_info_registration_reject_domain,
    guint8 *value_wcdma_system_info_registration_reject_cause,
    gboolean *value_wcdma_system_info_network_id_valid,
    const gchar **value_wcdma_system_info_mcc,
    const gchar **value_wcdma_system_info_mnc,
    gboolean *value_wcdma_system_info_hs_call_status_valid,
    QmiNasWcdmaHsService *value_wcdma_system_info_hs_call_status,
    gboolean *value_wcdma_system_info_hs_service_valid,
    QmiNasWcdmaHsService *value_wcdma_system_info_hs_service,
    gboolean *value_wcdma_system_info_primary_scrambling_code_valid,
    guint16 *value_wcdma_system_info_primary_scrambling_code,
    GError **error);

/**
 * qmi_message_nas_get_system_info_output_get_lte_system_info:
 * @self: a #QmiMessageNasGetSystemInfoOutput.
 * @value_lte_system_info_domain_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_lte_system_info_domain: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_lte_system_info_service_capability_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_lte_system_info_service_capability: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_lte_system_info_roaming_status_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_lte_system_info_roaming_status: (out): a placeholder for the output #QmiNasRoamingStatus, or %NULL if not required.
 * @value_lte_system_info_forbidden_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_lte_system_info_forbidden: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_lte_system_info_lac_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_lte_system_info_lac: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @value_lte_system_info_cid_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_lte_system_info_cid: (out): a placeholder for the output #guint32, or %NULL if not required.
 * @value_lte_system_info_registration_reject_info_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_lte_system_info_registration_reject_domain: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_lte_system_info_registration_reject_cause: (out): a placeholder for the output #guint8, or %NULL if not required.
 * @value_lte_system_info_network_id_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_lte_system_info_mcc: (out): a placeholder for the output constant string, or %NULL if not required.
 * @value_lte_system_info_mnc: (out): a placeholder for the output constant string, or %NULL if not required.
 * @value_lte_system_info_tac_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_lte_system_info_tac: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'LTE System Info' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.0
 * Deprecated: 1.30. Use qmi_message_nas_get_system_info_output_get_lte_system_info_v2() instead.
 */
G_DEPRECATED_FOR (qmi_message_nas_get_system_info_output_get_lte_system_info_v2)
gboolean qmi_message_nas_get_system_info_output_get_lte_system_info (
    QmiMessageNasGetSystemInfoOutput *self,
    gboolean *value_lte_system_info_domain_valid,
    QmiNasNetworkServiceDomain *value_lte_system_info_domain,
    gboolean *value_lte_system_info_service_capability_valid,
    QmiNasNetworkServiceDomain *value_lte_system_info_service_capability,
    gboolean *value_lte_system_info_roaming_status_valid,
    QmiNasRoamingStatus *value_lte_system_info_roaming_status,
    gboolean *value_lte_system_info_forbidden_valid,
    gboolean *value_lte_system_info_forbidden,
    gboolean *value_lte_system_info_lac_valid,
    guint16 *value_lte_system_info_lac,
    gboolean *value_lte_system_info_cid_valid,
    guint32 *value_lte_system_info_cid,
    gboolean *value_lte_system_info_registration_reject_info_valid,
    QmiNasNetworkServiceDomain *value_lte_system_info_registration_reject_domain,
    guint8 *value_lte_system_info_registration_reject_cause,
    gboolean *value_lte_system_info_network_id_valid,
    const gchar **value_lte_system_info_mcc,
    const gchar **value_lte_system_info_mnc,
    gboolean *value_lte_system_info_tac_valid,
    guint16 *value_lte_system_info_tac,
    GError **error);

/**
 * qmi_message_nas_get_system_info_output_get_td_scdma_system_info:
 * @self: a #QmiMessageNasGetSystemInfoOutput.
 * @value_td_scdma_system_info_domain_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scdma_system_info_domain: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_td_scdma_system_info_service_capability_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scdma_system_info_service_capability: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_td_scdma_system_info_roaming_status_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scdma_system_info_roaming_status: (out): a placeholder for the output #QmiNasRoamingStatus, or %NULL if not required.
 * @value_td_scdma_system_info_forbidden_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scdma_system_info_forbidden: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scdma_system_info_lac_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scdma_system_info_lac: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @value_td_scdma_system_info_cid_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scdma_system_info_cid: (out): a placeholder for the output #guint32, or %NULL if not required.
 * @value_td_scdma_system_info_registration_reject_info_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scdma_system_info_registration_reject_domain: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_td_scdma_system_info_registration_reject_cause: (out): a placeholder for the output #guint8, or %NULL if not required.
 * @value_td_scdma_system_info_network_id_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scdma_system_info_mcc: (out): a placeholder for the output constant string, or %NULL if not required.
 * @value_td_scdma_system_info_mnc: (out): a placeholder for the output constant string, or %NULL if not required.
 * @value_td_scdma_system_info_hs_call_status_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scdma_system_info_hs_call_status: (out): a placeholder for the output #QmiNasWcdmaHsService, or %NULL if not required.
 * @value_td_scdma_system_info_hs_service_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scdma_system_info_hs_service: (out): a placeholder for the output #QmiNasWcdmaHsService, or %NULL if not required.
 * @value_td_scdma_system_info_cell_parameter_id_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scdma_system_info_cell_parameter_id: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @value_td_scdma_system_info_cell_broadcast_support_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scdma_system_info_cell_broadcast_support: (out): a placeholder for the output #QmiNasCellBroadcastCapability, or %NULL if not required.
 * @value_td_scdma_system_info_cs_call_barring_status_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scdma_system_info_cs_call_barring_status: (out): a placeholder for the output #QmiNasCallBarringStatus, or %NULL if not required.
 * @value_td_scdma_system_info_ps_call_barring_status_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scdma_system_info_ps_call_barring_status: (out): a placeholder for the output #QmiNasCallBarringStatus, or %NULL if not required.
 * @value_td_scdma_system_info_cipher_domain_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scdma_system_info_cipher_domain: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'TD SCDMA System Info' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.0
 * Deprecated: 1.30. Use qmi_message_nas_get_system_info_output_get_td_scdma_system_info_v2() instead.
 */
G_DEPRECATED_FOR (qmi_message_nas_get_system_info_output_get_td_scdma_system_info_v2)
gboolean qmi_message_nas_get_system_info_output_get_td_scdma_system_info (
    QmiMessageNasGetSystemInfoOutput *self,
    gboolean *value_td_scdma_system_info_domain_valid,
    QmiNasNetworkServiceDomain *value_td_scdma_system_info_domain,
    gboolean *value_td_scdma_system_info_service_capability_valid,
    QmiNasNetworkServiceDomain *value_td_scdma_system_info_service_capability,
    gboolean *value_td_scdma_system_info_roaming_status_valid,
    QmiNasRoamingStatus *value_td_scdma_system_info_roaming_status,
    gboolean *value_td_scdma_system_info_forbidden_valid,
    gboolean *value_td_scdma_system_info_forbidden,
    gboolean *value_td_scdma_system_info_lac_valid,
    guint16 *value_td_scdma_system_info_lac,
    gboolean *value_td_scdma_system_info_cid_valid,
    guint32 *value_td_scdma_system_info_cid,
    gboolean *value_td_scdma_system_info_registration_reject_info_valid,
    QmiNasNetworkServiceDomain *value_td_scdma_system_info_registration_reject_domain,
    guint8 *value_td_scdma_system_info_registration_reject_cause,
    gboolean *value_td_scdma_system_info_network_id_valid,
    const gchar **value_td_scdma_system_info_mcc,
    const gchar **value_td_scdma_system_info_mnc,
    gboolean *value_td_scdma_system_info_hs_call_status_valid,
    QmiNasWcdmaHsService *value_td_scdma_system_info_hs_call_status,
    gboolean *value_td_scdma_system_info_hs_service_valid,
    QmiNasWcdmaHsService *value_td_scdma_system_info_hs_service,
    gboolean *value_td_scdma_system_info_cell_parameter_id_valid,
    guint16 *value_td_scdma_system_info_cell_parameter_id,
    gboolean *value_td_scdma_system_info_cell_broadcast_support_valid,
    QmiNasCellBroadcastCapability *value_td_scdma_system_info_cell_broadcast_support,
    gboolean *value_td_scdma_system_info_cs_call_barring_status_valid,
    QmiNasCallBarringStatus *value_td_scdma_system_info_cs_call_barring_status,
    gboolean *value_td_scdma_system_info_ps_call_barring_status_valid,
    QmiNasCallBarringStatus *value_td_scdma_system_info_ps_call_barring_status,
    gboolean *value_td_scdma_system_info_cipher_domain_valid,
    QmiNasNetworkServiceDomain *value_td_scdma_system_info_cipher_domain,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_NAS_GET_SYSTEM_INFO */

#if defined HAVE_QMI_INDICATION_NAS_SYSTEM_INFO

/**
 * qmi_indication_nas_system_info_output_get_gsm_system_info:
 * @self: a #QmiIndicationNasSystemInfoOutput.
 * @value_gsm_system_info_domain_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_domain: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_gsm_system_info_service_capability_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_service_capability: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_gsm_system_info_roaming_status_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_roaming_status: (out): a placeholder for the output #QmiNasRoamingStatus, or %NULL if not required.
 * @value_gsm_system_info_forbidden_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_forbidden: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_lac_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_lac: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @value_gsm_system_info_cid_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_cid: (out): a placeholder for the output #guint32, or %NULL if not required.
 * @value_gsm_system_info_registration_reject_info_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_registration_reject_domain: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_gsm_system_info_registration_reject_cause: (out): a placeholder for the output #guint8, or %NULL if not required.
 * @value_gsm_system_info_network_id_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_mcc: (out): a placeholder for the output constant string, or %NULL if not required.
 * @value_gsm_system_info_mnc: (out): a placeholder for the output constant string, or %NULL if not required.
 * @value_gsm_system_info_egprs_support_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_egprs_support: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_dtm_support_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_gsm_system_info_dtm_support: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'GSM System Info' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.0
 * Deprecated: 1.30. Use qmi_indication_nas_system_info_output_get_gsm_system_info_v2() instead.
 */
G_DEPRECATED_FOR (qmi_indication_nas_system_info_output_get_gsm_system_info_v2)
gboolean qmi_indication_nas_system_info_output_get_gsm_system_info (
    QmiIndicationNasSystemInfoOutput *self,
    gboolean *value_gsm_system_info_domain_valid,
    QmiNasNetworkServiceDomain *value_gsm_system_info_domain,
    gboolean *value_gsm_system_info_service_capability_valid,
    QmiNasNetworkServiceDomain *value_gsm_system_info_service_capability,
    gboolean *value_gsm_system_info_roaming_status_valid,
    QmiNasRoamingStatus *value_gsm_system_info_roaming_status,
    gboolean *value_gsm_system_info_forbidden_valid,
    gboolean *value_gsm_system_info_forbidden,
    gboolean *value_gsm_system_info_lac_valid,
    guint16 *value_gsm_system_info_lac,
    gboolean *value_gsm_system_info_cid_valid,
    guint32 *value_gsm_system_info_cid,
    gboolean *value_gsm_system_info_registration_reject_info_valid,
    QmiNasNetworkServiceDomain *value_gsm_system_info_registration_reject_domain,
    guint8 *value_gsm_system_info_registration_reject_cause,
    gboolean *value_gsm_system_info_network_id_valid,
    const gchar **value_gsm_system_info_mcc,
    const gchar **value_gsm_system_info_mnc,
    gboolean *value_gsm_system_info_egprs_support_valid,
    gboolean *value_gsm_system_info_egprs_support,
    gboolean *value_gsm_system_info_dtm_support_valid,
    gboolean *value_gsm_system_info_dtm_support,
    GError **error);

/**
 * qmi_indication_nas_system_info_output_get_wcdma_system_info:
 * @self: a #QmiIndicationNasSystemInfoOutput.
 * @value_wcdma_system_info_domain_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_domain: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_wcdma_system_info_service_capability_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_service_capability: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_wcdma_system_info_roaming_status_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_roaming_status: (out): a placeholder for the output #QmiNasRoamingStatus, or %NULL if not required.
 * @value_wcdma_system_info_forbidden_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_forbidden: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_lac_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_lac: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @value_wcdma_system_info_cid_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_cid: (out): a placeholder for the output #guint32, or %NULL if not required.
 * @value_wcdma_system_info_registration_reject_info_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_registration_reject_domain: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_wcdma_system_info_registration_reject_cause: (out): a placeholder for the output #guint8, or %NULL if not required.
 * @value_wcdma_system_info_network_id_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_mcc: (out): a placeholder for the output constant string, or %NULL if not required.
 * @value_wcdma_system_info_mnc: (out): a placeholder for the output constant string, or %NULL if not required.
 * @value_wcdma_system_info_hs_call_status_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_hs_call_status: (out): a placeholder for the output #QmiNasWcdmaHsService, or %NULL if not required.
 * @value_wcdma_system_info_hs_service_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_hs_service: (out): a placeholder for the output #QmiNasWcdmaHsService, or %NULL if not required.
 * @value_wcdma_system_info_primary_scrambling_code_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_wcdma_system_info_primary_scrambling_code: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'WCDMA System Info' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.0
 * Deprecated: 1.30. Use qmi_indication_nas_system_info_output_get_wcdma_system_info_v2() instead.
 */
G_DEPRECATED_FOR (qmi_indication_nas_system_info_output_get_wcdma_system_info_v2)
gboolean qmi_indication_nas_system_info_output_get_wcdma_system_info (
    QmiIndicationNasSystemInfoOutput *self,
    gboolean *value_wcdma_system_info_domain_valid,
    QmiNasNetworkServiceDomain *value_wcdma_system_info_domain,
    gboolean *value_wcdma_system_info_service_capability_valid,
    QmiNasNetworkServiceDomain *value_wcdma_system_info_service_capability,
    gboolean *value_wcdma_system_info_roaming_status_valid,
    QmiNasRoamingStatus *value_wcdma_system_info_roaming_status,
    gboolean *value_wcdma_system_info_forbidden_valid,
    gboolean *value_wcdma_system_info_forbidden,
    gboolean *value_wcdma_system_info_lac_valid,
    guint16 *value_wcdma_system_info_lac,
    gboolean *value_wcdma_system_info_cid_valid,
    guint32 *value_wcdma_system_info_cid,
    gboolean *value_wcdma_system_info_registration_reject_info_valid,
    QmiNasNetworkServiceDomain *value_wcdma_system_info_registration_reject_domain,
    guint8 *value_wcdma_system_info_registration_reject_cause,
    gboolean *value_wcdma_system_info_network_id_valid,
    const gchar **value_wcdma_system_info_mcc,
    const gchar **value_wcdma_system_info_mnc,
    gboolean *value_wcdma_system_info_hs_call_status_valid,
    QmiNasWcdmaHsService *value_wcdma_system_info_hs_call_status,
    gboolean *value_wcdma_system_info_hs_service_valid,
    QmiNasWcdmaHsService *value_wcdma_system_info_hs_service,
    gboolean *value_wcdma_system_info_primary_scrambling_code_valid,
    guint16 *value_wcdma_system_info_primary_scrambling_code,
    GError **error);

/**
 * qmi_indication_nas_system_info_output_get_lte_system_info:
 * @self: a #QmiIndicationNasSystemInfoOutput.
 * @value_lte_system_info_domain_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_lte_system_info_domain: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_lte_system_info_service_capability_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_lte_system_info_service_capability: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_lte_system_info_roaming_status_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_lte_system_info_roaming_status: (out): a placeholder for the output #QmiNasRoamingStatus, or %NULL if not required.
 * @value_lte_system_info_forbidden_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_lte_system_info_forbidden: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_lte_system_info_lac_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_lte_system_info_lac: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @value_lte_system_info_cid_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_lte_system_info_cid: (out): a placeholder for the output #guint32, or %NULL if not required.
 * @value_lte_system_info_registration_reject_info_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_lte_system_info_registration_reject_domain: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_lte_system_info_registration_reject_cause: (out): a placeholder for the output #guint8, or %NULL if not required.
 * @value_lte_system_info_network_id_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_lte_system_info_mcc: (out): a placeholder for the output constant string, or %NULL if not required.
 * @value_lte_system_info_mnc: (out): a placeholder for the output constant string, or %NULL if not required.
 * @value_lte_system_info_tac_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_lte_system_info_tac: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'LTE System Info' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.0
 * Deprected: 1.30. Use qmi_indication_nas_system_info_output_get_lte_system_info_v2() instead.
 */
G_DEPRECATED_FOR (qmi_indication_nas_system_info_output_get_lte_system_info_v2)
gboolean qmi_indication_nas_system_info_output_get_lte_system_info (
    QmiIndicationNasSystemInfoOutput *self,
    gboolean *value_lte_system_info_domain_valid,
    QmiNasNetworkServiceDomain *value_lte_system_info_domain,
    gboolean *value_lte_system_info_service_capability_valid,
    QmiNasNetworkServiceDomain *value_lte_system_info_service_capability,
    gboolean *value_lte_system_info_roaming_status_valid,
    QmiNasRoamingStatus *value_lte_system_info_roaming_status,
    gboolean *value_lte_system_info_forbidden_valid,
    gboolean *value_lte_system_info_forbidden,
    gboolean *value_lte_system_info_lac_valid,
    guint16 *value_lte_system_info_lac,
    gboolean *value_lte_system_info_cid_valid,
    guint32 *value_lte_system_info_cid,
    gboolean *value_lte_system_info_registration_reject_info_valid,
    QmiNasNetworkServiceDomain *value_lte_system_info_registration_reject_domain,
    guint8 *value_lte_system_info_registration_reject_cause,
    gboolean *value_lte_system_info_network_id_valid,
    const gchar **value_lte_system_info_mcc,
    const gchar **value_lte_system_info_mnc,
    gboolean *value_lte_system_info_tac_valid,
    guint16 *value_lte_system_info_tac,
    GError **error);

/**
 * qmi_indication_nas_system_info_output_get_td_scma_system_info:
 * @self: a #QmiIndicationNasSystemInfoOutput.
 * @value_td_scma_system_info_domain_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scma_system_info_domain: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_td_scma_system_info_service_capability_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scma_system_info_service_capability: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_td_scma_system_info_roaming_status_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scma_system_info_roaming_status: (out): a placeholder for the output #QmiNasRoamingStatus, or %NULL if not required.
 * @value_td_scma_system_info_forbidden_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scma_system_info_forbidden: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scma_system_info_lac_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scma_system_info_lac: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @value_td_scma_system_info_cid_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scma_system_info_cid: (out): a placeholder for the output #guint32, or %NULL if not required.
 * @value_td_scma_system_info_registration_reject_info_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scma_system_info_registration_reject_domain: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @value_td_scma_system_info_registration_reject_cause: (out): a placeholder for the output #guint8, or %NULL if not required.
 * @value_td_scma_system_info_network_id_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scma_system_info_mcc: (out): a placeholder for the output constant string, or %NULL if not required.
 * @value_td_scma_system_info_mnc: (out): a placeholder for the output constant string, or %NULL if not required.
 * @value_td_scma_system_info_hs_call_status_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scma_system_info_hs_call_status: (out): a placeholder for the output #QmiNasWcdmaHsService, or %NULL if not required.
 * @value_td_scma_system_info_hs_service_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scma_system_info_hs_service: (out): a placeholder for the output #QmiNasWcdmaHsService, or %NULL if not required.
 * @value_td_scma_system_info_cell_parameter_id_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scma_system_info_cell_parameter_id: (out): a placeholder for the output #guint16, or %NULL if not required.
 * @value_td_scma_system_info_cell_broadcast_support_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scma_system_info_cell_broadcast_support: (out): a placeholder for the output #QmiNasCellBroadcastCapability, or %NULL if not required.
 * @value_td_scma_system_info_cs_call_barring_status_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scma_system_info_cs_call_barring_status: (out): a placeholder for the output #QmiNasCallBarringStatus, or %NULL if not required.
 * @value_td_scma_system_info_ps_call_barring_status_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scma_system_info_ps_call_barring_status: (out): a placeholder for the output #QmiNasCallBarringStatus, or %NULL if not required.
 * @value_td_scma_system_info_cipher_domain_valid: (out): a placeholder for the output #gboolean, or %NULL if not required.
 * @value_td_scma_system_info_cipher_domain: (out): a placeholder for the output #QmiNasNetworkServiceDomain, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'TD SCMA System Info' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.0
 * Deprecated: 1.30. Use qmi_indication_nas_system_info_output_get_td_scma_system_info_v2() instead.
 */
G_DEPRECATED_FOR (qmi_indication_nas_system_info_output_get_td_scma_system_info_v2)
gboolean qmi_indication_nas_system_info_output_get_td_scma_system_info (
    QmiIndicationNasSystemInfoOutput *self,
    gboolean *value_td_scma_system_info_domain_valid,
    QmiNasNetworkServiceDomain *value_td_scma_system_info_domain,
    gboolean *value_td_scma_system_info_service_capability_valid,
    QmiNasNetworkServiceDomain *value_td_scma_system_info_service_capability,
    gboolean *value_td_scma_system_info_roaming_status_valid,
    QmiNasRoamingStatus *value_td_scma_system_info_roaming_status,
    gboolean *value_td_scma_system_info_forbidden_valid,
    gboolean *value_td_scma_system_info_forbidden,
    gboolean *value_td_scma_system_info_lac_valid,
    guint16 *value_td_scma_system_info_lac,
    gboolean *value_td_scma_system_info_cid_valid,
    guint32 *value_td_scma_system_info_cid,
    gboolean *value_td_scma_system_info_registration_reject_info_valid,
    QmiNasNetworkServiceDomain *value_td_scma_system_info_registration_reject_domain,
    guint8 *value_td_scma_system_info_registration_reject_cause,
    gboolean *value_td_scma_system_info_network_id_valid,
    const gchar **value_td_scma_system_info_mcc,
    const gchar **value_td_scma_system_info_mnc,
    gboolean *value_td_scma_system_info_hs_call_status_valid,
    QmiNasWcdmaHsService *value_td_scma_system_info_hs_call_status,
    gboolean *value_td_scma_system_info_hs_service_valid,
    QmiNasWcdmaHsService *value_td_scma_system_info_hs_service,
    gboolean *value_td_scma_system_info_cell_parameter_id_valid,
    guint16 *value_td_scma_system_info_cell_parameter_id,
    gboolean *value_td_scma_system_info_cell_broadcast_support_valid,
    QmiNasCellBroadcastCapability *value_td_scma_system_info_cell_broadcast_support,
    gboolean *value_td_scma_system_info_cs_call_barring_status_valid,
    QmiNasCallBarringStatus *value_td_scma_system_info_cs_call_barring_status,
    gboolean *value_td_scma_system_info_ps_call_barring_status_valid,
    QmiNasCallBarringStatus *value_td_scma_system_info_ps_call_barring_status,
    gboolean *value_td_scma_system_info_cipher_domain_valid,
    QmiNasNetworkServiceDomain *value_td_scma_system_info_cipher_domain,
    GError **error);

#endif /* HAVE_QMI_INDICATION_NAS_SYSTEM_INFO */

#if defined HAVE_QMI_MESSAGE_NAS_SWI_GET_STATUS

/**
 * qmi_message_nas_swi_get_status_output_get_common_info:
 * @self: a #QmiMessageNasSwiGetStatusOutput.
 * @value_common_info_temperature: (out)(optional): a placeholder for the output #guint8, or %NULL if not required.
 * @value_common_info_modem_mode: (out)(optional): a placeholder for the output #QmiNasSwiModemMode, or %NULL if not required.
 * @value_common_info_system_mode: (out)(optional): a placeholder for the output #QmiNasSwiSystemMode, or %NULL if not required.
 * @value_common_info_ims_registration_state: (out)(optional): a placeholder for the output #QmiNasSwiImsRegState, or %NULL if not required.
 * @value_common_info_packet_service_state: (out)(optional): a placeholder for the output #QmiNasSwiPsState, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Common Info' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.24
 * Deprecated: 1.30: Use qmi_message_nas_swi_get_status_output_get_common_info_v2() instead.
 */
G_DEPRECATED_FOR (qmi_message_nas_swi_get_status_output_get_common_info_v2)
gboolean qmi_message_nas_swi_get_status_output_get_common_info (
    QmiMessageNasSwiGetStatusOutput *self,
    guint8 *value_common_info_temperature,
    QmiNasSwiModemMode *value_common_info_modem_mode,
    QmiNasSwiSystemMode *value_common_info_system_mode,
    QmiNasSwiImsRegState *value_common_info_ims_registration_state,
    QmiNasSwiPsState *value_common_info_packet_service_state,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_NAS_SWI_GET_STATUS */

#if defined HAVE_QMI_MESSAGE_NAS_SET_SYSTEM_SELECTION_PREFERENCE

/**
 * qmi_message_nas_set_system_selection_preference_input_get_mnc_pds_digit_include_status:
 * @self: a #QmiMessageNasSetSystemSelectionPreferenceInput.
 * @value_mnc_pds_digit_include_status: (out)(optional): a placeholder for the output #gboolean, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'MNC PDS Digit Include Status' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.0
 * Deprecated: 1.30: Use qmi_message_nas_set_system_selection_preference_input_get_mnc_pcs_digit_include_status() instead.
 */
G_DEPRECATED_FOR (qmi_message_nas_set_system_selection_preference_input_get_mnc_pcs_digit_include_status)
gboolean qmi_message_nas_set_system_selection_preference_input_get_mnc_pds_digit_include_status (
    QmiMessageNasSetSystemSelectionPreferenceInput *self,
    gboolean *value_mnc_pds_digit_include_status,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_NAS_SET_SYSTEM_SELECTION_PREFERENCE */

#if defined HAVE_QMI_MESSAGE_PDC_CONFIG_CHANGE ||          \
    defined HAVE_QMI_MESSAGE_PDC_SET_SELECTED_CONFIG ||    \
    defined HAVE_QMI_MESSAGE_PDC_GET_CONFIG_INFO

/**
 * QmiConfigTypeAndId:
 * @config_type: a #QmiPdcConfigurationType.
 * @id: a #GArray of #guint8 elements.
 *
 * A QmiConfigTypeAndId struct.
 *
 * This type is deprecated and there is no replacement as the
 * new methods don't require any intermediate type any more.
 *
 * Since: 1.18
 * Deprecated: 1.32
 */
typedef struct _QmiDeprecatedConfigTypeAndId {
    QmiPdcConfigurationType  config_type;
    GArray                  *id;
} QmiDeprecatedConfigTypeAndId;
G_DEPRECATED
typedef QmiDeprecatedConfigTypeAndId QmiConfigTypeAndId;

#endif /* HAVE_QMI_MESSAGE_PDC_CONFIG_CHANGE
        * HAVE_QMI_MESSAGE_PDC_SET_SELECTED_CONFIG
        * HAVE_QMI_MESSAGE_PDC_GET_CONFIG_INFO */

#if defined HAVE_QMI_MESSAGE_PDC_CONFIG_CHANGE

/**
 * qmi_message_pdc_config_change_input_get_type_with_id:
 * @self: a #QmiMessagePdcConfigChangeInput.
 * @value_type_with_id: (out)(optional)(transfer none): a placeholder for the output constant #QmiConfigTypeAndId, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Type With Id' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.18
 * Deprecated: 1.32: Use qmi_message_pdc_config_change_input_get_type_with_id_v2() instead.
 */
G_DEPRECATED_FOR (qmi_message_pdc_config_change_input_get_type_with_id_v2)
gboolean qmi_message_pdc_config_change_input_get_type_with_id (
    QmiMessagePdcConfigChangeInput *self,
    QmiDeprecatedConfigTypeAndId *value_type_with_id,
    GError **error);

/**
 * qmi_message_pdc_config_change_input_set_type_with_id:
 * @self: a #QmiMessagePdcConfigChangeInput.
 * @value_type_with_id: the address of the #QmiConfigTypeAndId to set.
 * @error: Return location for error or %NULL.
 *
 * Set the 'Type With Id' field in the message.
 *
 * Returns: (skip): %TRUE if @value was successfully set, %FALSE otherwise.
 *
 * Since: 1.18
 * Deprecated: 1.32: Use qmi_message_pdc_config_change_input_set_type_with_id_v2() instead.
 */
G_DEPRECATED_FOR (qmi_message_pdc_config_change_input_set_type_with_id_v2)
gboolean qmi_message_pdc_config_change_input_set_type_with_id (
    QmiMessagePdcConfigChangeInput *self,
    const QmiDeprecatedConfigTypeAndId *value_type_with_id,
    GError **error);

/**
 * qmi_message_pdc_config_change_output_get_type_with_id:
 * @self: a #QmiMessagePdcConfigChangeOutput.
 * @value_type_with_id: (out)(optional)(transfer none): a placeholder for the output constant #QmiConfigTypeAndId, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Type With Id' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.18
 * Deprecated: 1.32: Use qmi_message_pdc_config_change_output_get_type_with_id_v2() instead.
 */
G_DEPRECATED_FOR (qmi_message_pdc_config_change_output_get_type_with_id_v2)
gboolean qmi_message_pdc_config_change_output_get_type_with_id (
    QmiMessagePdcConfigChangeOutput *self,
    QmiDeprecatedConfigTypeAndId *value_type_with_id,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_PDC_CONFIG_CHANGE */

#if defined HAVE_QMI_MESSAGE_PDC_SET_SELECTED_CONFIG

/**
 * qmi_message_pdc_set_selected_config_input_get_type_with_id:
 * @self: a #QmiMessagePdcSetSelectedConfigInput.
 * @value_type_with_id: (out)(optional)(transfer none): a placeholder for the output constant #QmiConfigTypeAndId, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Type With Id' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.18
 * Deprecated: 1.32: Use qmi_message_pdc_set_selected_config_input_get_type_with_id_v2() instead.
 */
G_DEPRECATED_FOR (qmi_message_pdc_set_selected_config_input_get_type_with_id_v2)
gboolean qmi_message_pdc_set_selected_config_input_get_type_with_id (
    QmiMessagePdcSetSelectedConfigInput *self,
    QmiDeprecatedConfigTypeAndId *value_type_with_id,
    GError **error);


/**
 * qmi_message_pdc_set_selected_config_input_set_type_with_id:
 * @self: a #QmiMessagePdcSetSelectedConfigInput.
 * @value_type_with_id: the address of the #QmiConfigTypeAndId to set.
 * @error: Return location for error or %NULL.
 *
 * Set the 'Type With Id' field in the message.
 *
 * Returns: (skip): %TRUE if @value was successfully set, %FALSE otherwise.
 *
 * Since: 1.18
 * Deprecated: 1.32: Use qmi_message_pdc_set_selected_config_input_set_type_with_id() instead.
 */
G_DEPRECATED_FOR (qmi_message_pdc_set_selected_config_input_set_type_with_id_v2)
gboolean qmi_message_pdc_set_selected_config_input_set_type_with_id (
    QmiMessagePdcSetSelectedConfigInput *self,
    const QmiDeprecatedConfigTypeAndId *value_type_with_id,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_PDC_SET_SELECTED_CONFIG */

#if defined HAVE_QMI_MESSAGE_PDC_GET_CONFIG_INFO

/**
 * qmi_message_pdc_get_config_info_input_get_type_with_id:
 * @self: a #QmiMessagePdcGetConfigInfoInput.
 * @value_type_with_id: (out)(optional)(transfer none): a placeholder for the output constant #QmiConfigTypeAndId, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Type With Id' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.18
 * Deprecated: 1.32: Use qmi_message_pdc_get_config_info_input_get_type_with_id_v2() instead.
 */
G_DEPRECATED_FOR (qmi_message_pdc_get_config_info_input_get_type_with_id_v2)
gboolean qmi_message_pdc_get_config_info_input_get_type_with_id (
    QmiMessagePdcGetConfigInfoInput *self,
    QmiDeprecatedConfigTypeAndId *value_type_with_id,
    GError **error);

/**
 * qmi_message_pdc_get_config_info_input_set_type_with_id:
 * @self: a #QmiMessagePdcGetConfigInfoInput.
 * @value_type_with_id: the address of the #QmiConfigTypeAndId to set.
 * @error: Return location for error or %NULL.
 *
 * Set the 'Type With Id' field in the message.
 *
 * Returns: (skip): %TRUE if @value was successfully set, %FALSE otherwise.
 *
 * Since: 1.18
 * Deprecated: 1.32: Use qmi_message_pdc_get_config_info_input_set_type_with_id_v2() instead.
 */
G_DEPRECATED_FOR (qmi_message_pdc_get_config_info_input_set_type_with_id_v2)
gboolean qmi_message_pdc_get_config_info_input_set_type_with_id (
    QmiMessagePdcGetConfigInfoInput *self,
    const QmiDeprecatedConfigTypeAndId *value_type_with_id,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_PDC_GET_CONFIG_INFO */

#if defined HAVE_QMI_MESSAGE_DMS_DELETE_STORED_IMAGE

/**
 * QmiMessageDmsDeleteStoredImageInputImage:
 * @type: a #QmiDmsFirmwareImageType.
 * @unique_id: a #GArray of #guint8 elements.
 * @build_id: a string.
 *
 * A QmiMessageDmsDeleteStoredImageInputImage struct.
 *
 * This type is deprecated and there is no replacement as the
 * new methods don't require any intermediate type any more.
 *
 * Since: 1.0
 * Deprecated: 1.32
 */
typedef struct _QmiDeprecatedMessageDmsDeleteStoredImageInputImage {
    QmiDmsFirmwareImageType type;
    GArray *unique_id;
    gchar *build_id;
} QmiDeprecatedMessageDmsDeleteStoredImageInputImage;
G_DEPRECATED
typedef QmiDeprecatedMessageDmsDeleteStoredImageInputImage QmiMessageDmsDeleteStoredImageInputImage;

/**
 * qmi_message_dms_delete_stored_image_input_get_image:
 * @self: a #QmiMessageDmsDeleteStoredImageInput.
 * @value_image: (out)(optional)(transfer none): a placeholder for the output constant #QmiMessageDmsDeleteStoredImageInputImage, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Image' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.0
 * Deprecated: 1.32: Use qmi_message_dms_delete_stored_image_input_set_image_details() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_delete_stored_image_input_get_image_details)
gboolean qmi_message_dms_delete_stored_image_input_get_image (
    QmiMessageDmsDeleteStoredImageInput *self,
    QmiDeprecatedMessageDmsDeleteStoredImageInputImage *value_image,
    GError **error);


/**
 * qmi_message_dms_delete_stored_image_input_set_image:
 * @self: a #QmiMessageDmsDeleteStoredImageInput.
 * @value_image: the address of the #QmiMessageDmsDeleteStoredImageInputImage to set.
 * @error: Return location for error or %NULL.
 *
 * Set the 'Image' field in the message.
 *
 * Returns: (skip): %TRUE if @value was successfully set, %FALSE otherwise.
 *
 * Since: 1.0
 * Deprecated: 1.32: Use qmi_message_dms_delete_stored_image_input_set_image_details() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_delete_stored_image_input_set_image_details)
gboolean qmi_message_dms_delete_stored_image_input_set_image (
    QmiMessageDmsDeleteStoredImageInput *self,
    const QmiDeprecatedMessageDmsDeleteStoredImageInputImage *value_image,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_DMS_DELETE_STORED_IMAGE */

#if defined HAVE_QMI_MESSAGE_DMS_GET_STORED_IMAGE_INFO

/**
 * QmiMessageDmsGetStoredImageInfoInputImage:
 * @type: a #QmiDmsFirmwareImageType.
 * @unique_id: a #GArray of #guint8 elements.
 * @build_id: a string.
 *
 * A QmiMessageDmsGetStoredImageInfoInputImage struct.
 *
 * This type is deprecated and there is no replacement as the
 * new methods don't require any intermediate type any more.
 *
 * Since: 1.0
 * Deprecated: 1.32
 */
typedef struct _QmiDeprecatedMessageDmsGetStoredImageInfoInputImage {
    QmiDmsFirmwareImageType type;
    GArray *unique_id;
    gchar *build_id;
} QmiDeprecatedMessageDmsGetStoredImageInfoInputImage;
G_DEPRECATED
typedef QmiDeprecatedMessageDmsGetStoredImageInfoInputImage QmiMessageDmsGetStoredImageInfoInputImage;

/**
 * qmi_message_dms_get_stored_image_info_input_get_image:
 * @self: a #QmiMessageDmsGetStoredImageInfoInput.
 * @value_image: (out)(optional)(transfer none): a placeholder for the output constant #QmiMessageDmsGetStoredImageInfoInputImage, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Image' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.0
 * Deprecated: 1.32: Use qmi_message_dms_get_stored_image_info_input_get_image_details() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_get_stored_image_info_input_get_image_details)
gboolean qmi_message_dms_get_stored_image_info_input_get_image (
    QmiMessageDmsGetStoredImageInfoInput *self,
    QmiDeprecatedMessageDmsGetStoredImageInfoInputImage *value_image,
    GError **error);

/**
 * qmi_message_dms_get_stored_image_info_input_set_image:
 * @self: a #QmiMessageDmsGetStoredImageInfoInput.
 * @value_image: the address of the #QmiMessageDmsGetStoredImageInfoInputImage to set.
 * @error: Return location for error or %NULL.
 *
 * Set the 'Image' field in the message.
 *
 * Returns: (skip): %TRUE if @value was successfully set, %FALSE otherwise.
 *
 * Since: 1.0
 * Deprecated: 1.32: Use qmi_message_dms_get_stored_image_info_input_set_image_details() instead.
 */
G_DEPRECATED_FOR (qmi_message_dms_get_stored_image_info_input_set_image_details)
gboolean qmi_message_dms_get_stored_image_info_input_set_image (
    QmiMessageDmsGetStoredImageInfoInput *self,
    const QmiDeprecatedMessageDmsGetStoredImageInfoInputImage *value_image,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_DMS_GET_STORED_IMAGE_INFO */

#if defined HAVE_QMI_INDICATION_LOC_POSITION_REPORT

/**
 * QmiIndicationLocPositionReportOutputDilutionOfPrecision:
 * @position_dilution_of_precision: a #gfloat.
 * @horizontal_dilution_of_precision: a #gfloat.
 * @vertical_dilution_of_precision: a #gfloat.
 *
 * A QmiIndicationLocPositionReportOutputDilutionOfPrecision struct.
 *
 * This type is deprecated and there is no replacement as the
 * new methods don't require any intermediate type any more.
 *
 * Since: 1.22
 * Deprecated: 1.32
 */
typedef struct _QmiDeprecatedIndicationLocPositionReportOutputDilutionOfPrecision {
    gfloat position_dilution_of_precision;
    gfloat horizontal_dilution_of_precision;
    gfloat vertical_dilution_of_precision;
} QmiDeprecatedIndicationLocPositionReportOutputDilutionOfPrecision;
G_DEPRECATED
typedef QmiDeprecatedIndicationLocPositionReportOutputDilutionOfPrecision QmiIndicationLocPositionReportOutputDilutionOfPrecision;

/**
 * qmi_indication_loc_position_report_output_get_dilution_of_precision:
 * @self: a #QmiIndicationLocPositionReportOutput.
 * @value_dilution_of_precision: (out)(optional)(transfer none): a placeholder for the output constant #QmiIndicationLocPositionReportOutputDilutionOfPrecision, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Dilution of Precision' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.22
 * Deprecated: 1.32: Use qmi_indication_loc_position_report_output_get_dop() instead.
 */
G_DEPRECATED_FOR (qmi_indication_loc_position_report_output_get_dop)
gboolean qmi_indication_loc_position_report_output_get_dilution_of_precision (
    QmiIndicationLocPositionReportOutput *self,
    QmiDeprecatedIndicationLocPositionReportOutputDilutionOfPrecision *value_dilution_of_precision,
    GError **error);

/**
 * QmiIndicationLocPositionReportOutputGpsTime:
 * @gps_weeks: a #guint16.
 * @gps_time_of_week_milliseconds: a #guint32.
 *
 * A QmiIndicationLocPositionReportOutputGpsTime struct.
 *
 * This type is deprecated and there is no replacement as the
 * new methods don't require any intermediate type any more.
 *
 * Since: 1.22
 * Deprecated: 1.32
 */
typedef struct _QmiDeprecatedIndicationLocPositionReportOutputGpsTime {
    guint16 gps_weeks;
    guint32 gps_time_of_week_milliseconds;
} QmiDeprecatedIndicationLocPositionReportOutputGpsTime;
G_DEPRECATED
typedef QmiDeprecatedIndicationLocPositionReportOutputGpsTime QmiIndicationLocPositionReportOutputGpsTime;

/**
 * qmi_indication_loc_position_report_output_get_gps_time:
 * @self: a #QmiIndicationLocPositionReportOutput.
 * @value_gps_time: (out)(optional)(transfer none): a placeholder for the output constant #QmiIndicationLocPositionReportOutputGpsTime, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'GPS Time' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.22
 * Deprecated: 1.32: Use qmi_indication_loc_position_report_output_get_gps_date_time() instead.
 */
G_DEPRECATED_FOR (qmi_indication_loc_position_report_output_get_gps_date_time)
gboolean qmi_indication_loc_position_report_output_get_gps_time (
    QmiIndicationLocPositionReportOutput *self,
    QmiDeprecatedIndicationLocPositionReportOutputGpsTime *value_gps_time,
    GError **error);

#endif /* HAVE_QMI_INDICATION_LOC_POSITION_REPORT */

#if defined HAVE_QMI_MESSAGE_UIM_GET_SLOT_STATUS

/**
 * qmi_message_uim_get_slot_status_output_get_slot_eid_information:
 * @self: a #QmiMessageUimGetSlotStatusOutput.
 * @value_slot_eid_information: (out)(optional)(element-type GArray)(transfer none): a placeholder for the output #GArray of #GArray * elements, or %NULL if not required. Do not free it, it is owned by @self.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Slot EID Information' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.26
 * Deprecated: 1.32: Use qmi_message_uim_get_slot_status_output_get_slot_eid() instead.
 */
G_DEPRECATED_FOR (qmi_message_uim_get_slot_status_output_get_slot_eid)
gboolean qmi_message_uim_get_slot_status_output_get_slot_eid_information (
    QmiMessageUimGetSlotStatusOutput *self,
    GArray **value_slot_eid_information,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_UIM_GET_SLOT_STATUS */

#if defined HAVE_QMI_INDICATION_UIM_SLOT_STATUS

/**
 * qmi_indication_uim_slot_status_output_get_slot_eid_information:
 * @self: a #QmiIndicationUimSlotStatusOutput.
 * @value_slot_eid_information: (out)(optional)(element-type GArray)(transfer none): a placeholder for the output #GArray of #GArray * elements, or %NULL if not required. Do not free it, it is owned by @self.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Slot EID Information' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.26
 * Deprecated: 1.32: Use qmi_indication_uim_slot_status_output_get_slot_eid() instead.
 */
G_DEPRECATED_FOR (qmi_indication_uim_slot_status_output_get_slot_eid)
gboolean qmi_indication_uim_slot_status_output_get_slot_eid_information (
    QmiIndicationUimSlotStatusOutput *self,
    GArray **value_slot_eid_information,
    GError **error);

#endif /* HAVE_QMI_INDICATION_UIM_SLOT_STATUS */

#if defined HAVE_QMI_MESSAGE_UIM_GET_CONFIGURATION

/**
 * QmiMessageUimGetConfigurationOutputPersonalizationStatusOtherSlotsSlotsElement:
 * @feature: a #QmiUimCardApplicationPersonalizationFeature.
 * @verify_left: a #guint8.
 * @unblock_left: a #guint8.
 *
 * A QmiMessageUimGetConfigurationOutputPersonalizationStatusOtherSlotsSlotsElement struct.
 *
 * Since: 1.30
 * Deprecated: 1.32: Use #QmiMessageUimGetConfigurationOutputPersonalizationStatusOtherElementSlotElement instead.
 */
G_DEPRECATED_FOR (QmiMessageUimGetConfigurationOutputPersonalizationStatusOtherElementSlotElement)
typedef struct _QmiMessageUimGetConfigurationOutputPersonalizationStatusOtherSlotsSlotsElement {
    QmiUimCardApplicationPersonalizationFeature feature;
    guint8 verify_left;
    guint8 unblock_left;
} QmiMessageUimGetConfigurationOutputPersonalizationStatusOtherSlotsSlotsElement;

/**
 * qmi_message_uim_get_configuration_output_get_personalization_status_other_slots:
 * @self: a #QmiMessageUimGetConfigurationOutput.
 * @value_personalization_status_other_slots: (out)(optional)(element-type GArray)(transfer none): a placeholder for the output #GArray of #GArray * elements, or %NULL if not required. Do not free it, it is owned by @self.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Personalization Status Other Slots' field from @self.
 *
 * Returns: (skip): %TRUE if the field is found, %FALSE otherwise.
 *
 * Since: 1.30
 * Deprecated: 1.32: Use qmi_message_uim_get_configuration_output_get_personalization_status_other() instead.
 */
G_DEPRECATED_FOR (qmi_message_uim_get_configuration_output_get_personalization_status_other)
gboolean qmi_message_uim_get_configuration_output_get_personalization_status_other_slots (
    QmiMessageUimGetConfigurationOutput *self,
    GArray **value_personalization_status_other_slots,
    GError **error);

#endif /* HAVE_QMI_MESSAGE_UIM_GET_CONFIGURATION */

#endif /* QMI_DISABLE_DEPRECATED */

#endif /* _LIBQMI_GLIB_QMI_COMPAT_H_ */
