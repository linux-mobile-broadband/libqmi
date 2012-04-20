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
 * Copyright (C) 2012 Aleksander Morgado <aleksander@lanedo.com>
 */

#ifndef _LIBQMI_GLIB_QMI_ERRORS_H_
#define _LIBQMI_GLIB_QMI_ERRORS_H_

/**
 * QmiCoreError:
 * @QMI_CORE_ERROR_FAILED: Operation failed.
 * @QMI_CORE_ERROR_WRONG_STATE: Operation cannot be executed in the current state.
 * @QMI_CORE_ERROR_TIMEOUT: Operation timed out.
 * @QMI_CORE_ERROR_INVALID_ARGS: Invalid arguments given.
 * @QMI_CORE_ERROR_INVALID_MESSAGE: QMI message is invalid.
 * @QMI_CORE_ERROR_TLV_NOT_FOUND: TLV not found.
 * @QMI_CORE_ERROR_TLV_TOO_LONG: TLV is too long.
 *
 * Common errors that may be reported by libqmi-glib.
 */
typedef enum {
    QMI_CORE_ERROR_FAILED,
    QMI_CORE_ERROR_WRONG_STATE,
    QMI_CORE_ERROR_TIMEOUT,
    QMI_CORE_ERROR_INVALID_ARGS,
    QMI_CORE_ERROR_INVALID_MESSAGE,
    QMI_CORE_ERROR_TLV_NOT_FOUND,
    QMI_CORE_ERROR_TLV_TOO_LONG,
} QmiCoreError;

#endif /* _LIBQMI_GLIB_QMI_ERRORS_H_ */
