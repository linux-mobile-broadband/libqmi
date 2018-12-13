
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
 * Copyright (C) 2016-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBQMI_GLIB_QMI_COMPAT_H_
#define _LIBQMI_GLIB_QMI_COMPAT_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

#include "qmi-device.h"
#include "qmi-dms.h"
#include "qmi-uim.h"
#include "qmi-enums-nas.h"
#include "qmi-enums-wms.h"

/**
 * SECTION:qmi-compat
 * @title: Deprecated interface
 *
 * These types and methods are flagged as deprecated and therefore
 * shouldn't be used in newly written code. They are provided to avoid
 * innecessary API/ABI breaks, for compatibility purposes only.
 */

#ifndef QMI_DISABLE_DEPRECATED

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

/* The following type exists just so that we can get deprecation warnings */
G_DEPRECATED
typedef int QmiDeprecatedNasSimRejectState;

/**
 * QMI_NAS_SIM_REJECT_STATE_SIM_VAILABLE:
 *
 * SIM available.
 *
 * Since: 1.0
 * Deprecated: 1.14.0: Use the correct #QMI_NAS_SIM_REJECT_STATE_SIM_AVAILABLE name instead.
 */
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
 * @major: (out) return location for the major version.
 * @minor: (out) return location for the minor version.
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

/* The following type exists just so that we can get deprecation warnings */
G_DEPRECATED
typedef int QmiDeprecatedWdsCdmaCauseCode;

/**
 * QMI_WDS_CDMA_CAUSE_CODE_NETWORK_ADDRESS_VACANT:
 *
 * Address is valid but not yet allocated.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_NETWORK_ADDRESS_VACANT name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_NETWORK_ADDRESS_VACANT (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_NETWORK_ADDRESS_VACANT

/**
 * QMI_WDS_CDMA_CAUSE_CODE_NETWORK_ADDRESS_TRANSLATION_FAILURE:
 *
 * Address is invalid.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_NETWORK_ADDRESS_TRANSLATION_FAILURE name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_NETWORK_ADDRESS_TRANSLATION_FAILURE (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_NETWORK_ADDRESS_TRANSLATION_FAILURE

/**
 * QMI_WDS_CDMA_CAUSE_CODE_NETWORK_RESOURCE_SHORTAGE:
 *
 * Network resource shortage.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_NETWORK_RESOURCE_SHORTAGE name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_NETWORK_RESOURCE_SHORTAGE (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_NETWORK_RESOURCE_SHORTAGE

/**
 * QMI_WDS_CDMA_CAUSE_CODE_NETWORK_FAILURE:
 *
 * Network failed.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_NETWORK_FAILURE name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_NETWORK_FAILURE (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_NETWORK_FAILURE

/**
 * QMI_WDS_CDMA_CAUSE_CODE_NETWORK_INVALID_TELESERVICE_ID:
 *
 * SMS teleservice ID is invalid.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_NETWORK_INVALID_TELESERVICE_ID name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_NETWORK_INVALID_TELESERVICE_ID (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_NETWORK_INVALID_TELESERVICE_ID

/**
 * QMI_WDS_CDMA_CAUSE_CODE_NETWORK_OTHER:
 *
 * Other network error.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_NETWORK_OTHER name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_NETWORK_OTHER (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_NETWORK_OTHER

/**
 * QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_NO_PAGE_RESPONSE:
 *
 * No page response from destination.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_NO_PAGE_RESPONSE name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_NO_PAGE_RESPONSE (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_NO_PAGE_RESPONSE

/**
 * QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_BUSY:
 *
 * Destination is busy.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_BUSY name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_BUSY (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_BUSY

/**
 * QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_NO_ACK:
 *
 * No acknowledge from destination.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_NO_ACK name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_NO_ACK (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_NO_ACK


/**
 * QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_RESOURCE_SHORTAGE:
 *
 * Destination resource shortage.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_RESOURCE_SHORTAGE name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_RESOURCE_SHORTAGE (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_RESOURCE_SHORTAGE

/**
 * QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_SMS_DELIVERY_POSTPONED:
 *
 * SMS delivery postponed.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_SMS_DELIVERY_POSTPONED name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_SMS_DELIVERY_POSTPONED (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_SMS_DELIVERY_POSTPONED

/**
 * QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_OUT_OF_SERVICE:
 *
 * Destination out of service.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_OUT_OF_SERVICE name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_OUT_OF_SERVICE (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_OUT_OF_SERVICE

/**
 * QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_NOT_AT_ADDRESS:
 *
 * Destination not at address.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_NOT_AT_ADDRESS name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_NOT_AT_ADDRESS (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_NOT_AT_ADDRESS

/**
 * QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_OTHER:
 *
 * Other destination error.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_OTHER name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_DESTINATION_OTHER (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_DESTINATION_OTHER

/**
 * QMI_WDS_CDMA_CAUSE_CODE_RADIO_INTERFACE_RESOURCE_SHORTAGE:
 *
 * Radio interface resource shortage.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_RADIO_INTERFACE_RESOURCE_SHORTAGE name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_RADIO_INTERFACE_RESOURCE_SHORTAGE (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_RADIO_INTERFACE_RESOURCE_SHORTAGE

/**
 * QMI_WDS_CDMA_CAUSE_CODE_RADIO_INTERFACE_INCOMPATIBILITY:
 *
 * Radio interface incompatibility.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_RADIO_INTERFACE_INCOMPATIBILITY name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_RADIO_INTERFACE_INCOMPATIBILITY (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_RADIO_INTERFACE_INCOMPATIBILITY

/**
 * QMI_WDS_CDMA_CAUSE_CODE_RADIO_INTERFACE_OTHER:
 *
 * Other radio interface error
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_RADIO_INTERFACE_OTHER name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_RADIO_INTERFACE_OTHER (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_RADIO_INTERFACE_OTHER

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_ENCODING:
 *
 * Encoding error.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_ENCODING name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_ENCODING (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_ENCODING

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_SMS_ORIGIN_DENIED:
 *
 * SMS origin denied.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_SMS_ORIGIN_DENIED name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_SMS_ORIGIN_DENIED (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_SMS_ORIGIN_DENIED

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_SMS_DESTINATION_DENIED:
 *
 * SMS destination denied.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_SMS_DESTINATION_DENIED name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_SMS_DESTINATION_DENIED (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_SMS_DESTINATION_DENIED

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_SUPPLEMENTARY_SERVICE_NOT_SUPPORTED:
 *
 * Supplementary service not supported.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_SUPPLEMENTARY_SERVICE_NOT_SUPPORTED name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_SUPPLEMENTARY_SERVICE_NOT_SUPPORTED (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_SUPPLEMENTARY_SERVICE_NOT_SUPPORTED

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_SMS_NOT_SUPPORTED:
 *
 * SMS not supported.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_SMS_NOT_SUPPORTED name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_SMS_NOT_SUPPORTED (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_SMS_NOT_SUPPORTED

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_MISSING_EXPECTED_PARAMETER:
 *
 * Missing optional expected parameter.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_MISSING_EXPECTED_PARAMETER name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_MISSING_EXPECTED_PARAMETER (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_MISSING_EXPECTED_PARAMETER

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_MISSING_MANDATORY_PARAMETER:
 *
 * Missing mandatory parameter.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_MISSING_MANDATORY_PARAMETER name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_MISSING_MANDATORY_PARAMETER (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_MISSING_MANDATORY_PARAMETER

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_UNRECOGNIZED_PARAMETER_VALUE:
 *
 * Unrecognized parameter value.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_UNRECOGNIZED_PARAMETER_VALUE name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_UNRECOGNIZED_PARAMETER_VALUE (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_UNRECOGNIZED_PARAMETER_VALUE

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_UNEXPECTED_PARAMETER_VALUE:
 *
 * Unexpected parameter value.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_UNEXPECTED_PARAMETER_VALUE name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_UNEXPECTED_PARAMETER_VALUE (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_UNEXPECTED_PARAMETER_VALUE

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_USER_DATA_SIZE_ERROR:
 *
 * User data size error.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_USER_DATA_SIZE_ERROR name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_USER_DATA_SIZE_ERROR (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_USER_DATA_SIZE_ERROR

/**
 * QMI_WDS_CDMA_CAUSE_CODE_GENERAL_OTHER:
 *
 * Other general error.
 *
 * Since: 1.0
 * Deprecated: 1.18.0: Use the correct #QMI_WMS_CDMA_CAUSE_CODE_GENERAL_OTHER name instead.
 */
#define QMI_WDS_CDMA_CAUSE_CODE_GENERAL_OTHER (QmiDeprecatedWdsCdmaCauseCode) QMI_WMS_CDMA_CAUSE_CODE_GENERAL_OTHER

/**
 * QMI_PROTOCOL_ERROR_QOS_UNAVAILABLE:
 *
 * QoS unavailable.
 *
 * Since: 1.0
 * Deprecated: 1.22.0: Use the #QMI_PROTOCOL_ERROR_REQUESTED_NUMBER_UNSUPPORTED instead.
 */
#define QMI_PROTOCOL_ERROR_QOS_UNAVAILABLE (QmiProtocolError) QMI_PROTOCOL_ERROR_REQUESTED_NUMBER_UNSUPPORTED

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

#endif /* QMI_DISABLE_DEPRECATED */

#endif /* _LIBQMI_GLIB_QMI_COMPAT_H_ */
