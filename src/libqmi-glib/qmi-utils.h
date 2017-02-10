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
 * Copyright (C) 2012-2015 Dan Williams <dcbw@redhat.com>
 * Copyright (C) 2012-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBQMI_GLIB_QMI_UTILS_H_
#define _LIBQMI_GLIB_QMI_UTILS_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

#include <glib.h>

G_BEGIN_DECLS

/**
 * QmiEndian:
 * @QMI_ENDIAN_LITTLE: Little endian.
 * @QMI_ENDIAN_BIG: Big endian.
 *
 * Type of endianness
 *
 * Since: 1.0
 */
typedef enum {
    QMI_ENDIAN_LITTLE = 0,
    QMI_ENDIAN_BIG    = 1
} QmiEndian;

/* Reading/Writing integer variables */

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

/* Reading/Writing string variables */

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

/* Enabling/Disabling traces */
/**
 * qmi_utils_get_traces_enabled:
 *
 * Checks whether QMI message traces are currently enabled.
 *
 * Returns: %TRUE if traces are enabled, %FALSE otherwise.
 *
 * Since: 1.0
 */
gboolean qmi_utils_get_traces_enabled (void);

/**
 * qmi_utils_set_traces_enabled:
 * @enabled: %TRUE to enable traces, %FALSE to disable them.
 *
 * Sets whether QMI message traces are enabled or disabled.
 *
 * Since: 1.0
 */
void qmi_utils_set_traces_enabled (gboolean enabled);

/* Other private methods */

#if defined (LIBQMI_GLIB_COMPILATION)
G_GNUC_INTERNAL
gchar *__qmi_utils_str_hex (gconstpointer mem,
                            gsize size,
                            gchar delimiter);
G_GNUC_INTERNAL
gboolean __qmi_user_allowed (uid_t uid,
                             GError **error);
G_GNUC_INTERNAL
gchar *__qmi_utils_get_driver (const gchar *cdc_wdm_path);
#endif

G_END_DECLS

#endif /* _LIBQMI_GLIB_QMI_UTILS_H_ */
