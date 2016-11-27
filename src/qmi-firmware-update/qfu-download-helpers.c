/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * qmi-firmware-update -- Command line tool to update firmware in QMI devices
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2016 Bjørn Mork <bjorn@mork.no>
 * Copyright (C) 2016 Aleksander Morgado <aleksander@aleksander.es>
 *
 * Additional copyrights:
 *
 *    crc16 and hdlc parts:
 *      Copyright (C) 2010 Red Hat, Inc.
 *
 *    parts of this is based on gobi-loader, which is
 *     "Copyright 2009 Red Hat <mjg@redhat.com> - heavily based on work done by
 *      Alexander Shumakovitch <shurik@gwu.edu>
 *
 *    gobi 2000 support provided by Anssi Hannula <anssi.hannula@iki.fi>
 */

#include <config.h>

#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include "qfu-download-helpers.h"

/******************************************************************************/

static gchar *
utils_str_hex (gconstpointer mem,
               gsize         size,
               gchar         delimiter)
{
    const guint8 *data = mem;
    gsize         i;
    gsize         j;
    gsize         new_str_length;
    gchar        *new_str;

    /* Get new string length. If input string has N bytes, we need:
     * - 1 byte for last NUL char
     * - 2N bytes for hexadecimal char representation of each byte...
     * - N-1 bytes for the separator ':'
     * So... a total of (1+2N+N-1) = 3N bytes are needed... */
    new_str_length =  3 * size;

    /* Allocate memory for new array and initialize contents to NUL */
    new_str = g_malloc0 (new_str_length);

    /* Print hexadecimal representation of each byte... */
    for (i = 0, j = 0; i < size; i++, j += 3) {
        /* Print character in output string... */
        snprintf (&new_str[j], 3, "%02X", data[i]);
        /* And if needed, add separator */
        if (i != (size - 1) )
            new_str[j + 2] = delimiter;
    }

    /* Set output string */
    return new_str;
}

/******************************************************************************/
/* DLOAD protocol */

/*
 * Most of this is from Josuah Hill's DLOAD tool for iPhone
 * Some spec is also available in document 80-39912-1 Rev. E  DMSS Download Protocol Interface Specification and Operational Description
 * https://github.com/posixninja/DLOADTool/blob/master/dloadtool/dload.h
 *
 * The 0x70 switching command was found by snooping on firmware updates
 */

typedef enum {
    DLOAD_CMD_ACK = 0x02, /* Acknowledge receiving a packet */
    DLOAD_CMD_NOP = 0x06, /* No operation, useful for debugging */
    DLOAD_CMD_SDP = 0x70, /* Switch to Streaming DLOAD */
} DloadCmd;

/* 0x02 - guint8 cmd only */
/* 0x06 - guint8 cmd only */

typedef struct _DloadSdp DloadSdp;

struct _DloadSdp {
    guint8  cmd;       /* 0x70 */
    guint16 reserved;  /* 0x0000 */
} __attribute__ ((packed));

static DloadSdp dload_sdp = {
    .cmd      = 0x70,
    .reserved = 0x0000,
};

/******************************************************************************/
/* HDLC */

/*
 * crc16 and HDLC escape code borrowed from modemmanager/libqcdm
 * Copyright (C) 2010 Red Hat, Inc.
 */

/* Table of CRCs for each possible byte, with a generator polynomial of 0x8408 */
static const guint16 crc_table[256] = {
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

/* Calculate the CRC for a buffer using a seed of 0xffff */
static guint16
crc16 (const gchar *buffer,
       gsize        len)
{
    guint16 crc = 0xffff;

    while (len--)
            crc = crc_table[(crc ^ *buffer++) & 0xff] ^ (crc >> 8);
    return ~crc;
}

#define CONTROL 0x7e
#define ESCAPE  0x7d
#define MASK    0x20

static gsize
escape (const gchar *in, gsize inlen, gchar *out, gsize outlen)
{
    gsize i, j;

    for (i = 0, j = 0; i < inlen; i++) {
        /* Caller should give a big enough buffer */
        g_assert ((j + 1) < outlen);
        if (in[i] == CONTROL || in[i] == ESCAPE) {
            out[j++] = ESCAPE;
            out[j++] = in[i] ^ MASK;
        } else
            out[j++] = in[i];
    }
    return j;
}

static gsize
unescape (const gchar *in,
          gsize        inlen,
          gchar       *out,
          gsize        outlen)
{
    gsize    i, j = 0;
    gboolean escaping = FALSE;

    for (i = 0; i < inlen; i++) {
        /* Caller should give a big enough buffer */
        g_assert (j < outlen);
        if (escaping) {
            out[j++] = in[i] ^ MASK;
            escaping = FALSE;
        } else if (in[i] == ESCAPE) {
            escaping = TRUE;
        } else {
            out[j++] = in[i];
        }
    }

    return j;
}

/* copy a possibly escaped single byte to out */
static gsize
escape_byte (guint8  byte,
             gchar  *out,
             gsize   outlen)
{
    gsize j = 0;

    if (byte == CONTROL || byte == ESCAPE) {
        out[j++] = ESCAPE;
        byte ^= MASK;
    }
    out[j++] = byte;
    return j;
}

static gsize
hdlc_frame (const gchar *in,
            gsize        inlen,
            gchar       *out,
            gsize        outlen)
{
    guint16 crc;
    gsize j = 0;

    out[j++] = CONTROL;
    j += escape (in, inlen, &out[j], outlen - j);
    crc = crc16 (in, inlen);
    j += escape_byte (crc & 0xff, &out[j], outlen - j);
    j += escape_byte (crc >> 8 & 0xff, &out[j], outlen - j);
    out[j++] = CONTROL;

    return j;
}

static gsize
hdlc_unframe (const gchar  *in,
              gsize         inlen,
              gchar        *out,
              gsize         outlen,
              GError      **error)
{
    guint16 crc;
    gsize j, i = inlen;

    /* the first control char is optional */
    if (*in == CONTROL) {
        in++;
        i--;
    }
    if (in[i - 1] == CONTROL)
        i--;

    j = unescape (in, i, out, outlen);
    if (j < 2) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "unescaping failed: too few bytes as output: %" G_GSIZE_FORMAT, j);
        return 0;
    }
    j -= 2; /* remove the crc */

    /* verify the crc */
    crc = crc16 (out, j);
    if (crc != ((guint8) out[j] | (guint8) out[j + 1] << 8)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "crc check failed: 0x%04x != 0x%04x\n", crc, out[j] | out[j + 1] << 8);
        return 0;
    }

    return j;
}

static gboolean
write_hdlc (gint          fd,
            const gchar  *in,
            gsize         inlen,
            GError      **error)
{
    gchar  wbuf[512];
    gsize  wlen;
    gssize written;

    /* Pack into an HDLC frame */
    wlen = hdlc_frame (in, inlen, wbuf, sizeof (wbuf));
    g_assert_cmpuint (wlen, >, 0);

    /* Send HDLC frame to device */
    written = write (fd, wbuf, wlen);
    if (written < 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "couldn't write HDLC frame: %s",
                     g_strerror (errno));
        return FALSE;
    }

    /* Treat this as an error, we may want to improve this by writing out the
     * remaning amount of bytes */
    if (written != wlen) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "error writing full HDLC frame: only %" G_GSSIZE_FORMAT "/" G_GSIZE_FORMAT " bytes written",
                     written, wlen);
        return FALSE;
    }

    /* Debug output */
    if (qmi_utils_get_traces_enabled ()) {
        gchar *printable;

        printable = utils_str_hex (wbuf, wlen);
        g_debug ("[qfu-download,hdlc] >> %s", printable);
        g_free (printable);
    }

    return TRUE;
}

/******************************************************************************/
/* QDL */

/* from GobiAPI_1.0.40/Core/QDLEnum.h and
 * GobiAPI_1.0.40/Core/QDLBuffers.h with additional details from USB
 * snooping
 */
typedef enum {
    QDL_CMD_HELLO_REQ          = 0x01,
    QDL_CMD_HELLO_RSP          = 0x02,
    QDL_CMD_ERROR              = 0x0d,
    QDL_CMD_OPEN_UNFRAMED_REQ  = 0x25,
    QDL_CMD_OPEN_UNFRAMED_RSP  = 0x26,
    QDL_CMD_WRITE_UNFRAMED_REQ = 0x27,
    QDL_CMD_WRITE_UNFRAMED_RSP = 0x28,
    QDL_CMD_SESSION_DONE_REQ   = 0x29,
    QDL_CMD_SESSION_DONE_RSP   = 0x2a,
    QDL_CMD_DOWNLOAD_REQ       = 0x2b,
    QDL_CMD_SESSION_CLOSE_REQ  = 0x2d,
    QDL_CMD_GET_IMAGE_PREF_REQ = 0x2e,
    QDL_CMD_GET_IMAGE_PREF_RSP = 0x2f,
} QdlCmd;

static const gchar *
qdl_cmd_to_string (QdlCmd cmd)
{
    switch (cmd) {
    case QDL_CMD_HELLO_REQ:
        return "Hello request";
    case QDL_CMD_HELLO_RSP:
        return "Hello response";
    case QDL_CMD_ERROR:
        return "Error";
    case QDL_CMD_OPEN_UNFRAMED_REQ:
        return "Open unframed image write request";
    case QDL_CMD_OPEN_UNFRAMED_RSP:
        return "Open unframed image write response";
    case QDL_CMD_WRITE_UNFRAMED_REQ:
        return "Unframed image write request";
    case QDL_CMD_WRITE_UNFRAMED_RSP:
        return "Unframed image write response";
    case QDL_CMD_SESSION_DONE_REQ:
        return "Unframed session done request";
    case QDL_CMD_SESSION_DONE_RSP:
        return "Unframed session done response";
    case QDL_CMD_DOWNLOAD_REQ:
        return "Switch to download protocol request";
    case QDL_CMD_SESSION_CLOSE_REQ:
        return "Close unframed session request";
    case QDL_CMD_GET_IMAGE_PREF_REQ:
        return "Get image preference request";
    case QDL_CMD_GET_IMAGE_PREF_RSP:
        return "Get image preference response";
    default:
        return NULL;
    }
}

typedef enum {
    QDL_ERROR_NONE              = 0x00,
    QDL_ERROR_01_RESERVED       = 0x01,
    QDL_ERROR_BAD_ADDR          = 0x02,
    QDL_ERROR_BAD_LEN           = 0x03,
    QDL_ERROR_BAD_PACKET        = 0x04,
    QDL_ERROR_BAD_CMD           = 0x05,
    QDL_ERROR_06                = 0x06,
    QDL_ERROR_OP_FAILED         = 0x07,
    QDL_ERROR_BAD_FLASH_ID      = 0x08,
    QDL_ERROR_BAD_VOLTAGE       = 0x09,
    QDL_ERROR_WRITE_FAILED      = 0x0a,
    QDL_ERROR_11_RESERVED       = 0x0b,
    QDL_ERROR_BAD_SPC           = 0x0c,
    QDL_ERROR_POWERDOWN         = 0x0d,
    QDL_ERROR_UNSUPPORTED       = 0x0e,
    QDL_ERROR_CMD_SEQ           = 0x0f,
    QDL_ERROR_CLOSE             = 0x10,
    QDL_ERROR_BAD_FEATURES      = 0x11,
    QDL_ERROR_SPACE             = 0x12,
    QDL_ERROR_BAD_SECURITY      = 0x13,
    QDL_ERROR_MULTI_UNSUPPORTED = 0x14,
    QDL_ERROR_POWEROFF          = 0x15,
    QDL_ERROR_CMD_UNSUPPORTED   = 0x16,
    QDL_ERROR_BAD_CRC           = 0x17,
    QDL_ERROR_STATE             = 0x18,
    QDL_ERROR_TIMEOUT           = 0x19,
    QDL_ERROR_IMAGE_AUTH        = 0x1a,
    QDL_ERROR_LAST
} QdlError;

static const gchar *qdl_error_str[] = {
    [QDL_ERROR_NONE              ] = "None"
    [QDL_ERROR_01_RESERVED       ] = "Reserved",
    [QDL_ERROR_BAD_ADDR          ] = "Invalid destination address",
    [QDL_ERROR_BAD_LEN           ] = "Invalid length",
    [QDL_ERROR_BAD_PACKET        ] = "Unexpected end of packet",
    [QDL_ERROR_BAD_CMD           ] = "Invalid command",
    [QDL_ERROR_06                ] = "Reserved",
    [QDL_ERROR_OP_FAILED         ] = "Operation failed",
    [QDL_ERROR_BAD_FLASH_ID      ] = "Invalid flash intelligent ID",
    [QDL_ERROR_BAD_VOLTAGE       ] = "Invalid programming voltage",
    [QDL_ERROR_WRITE_FAILED      ] = "Write verify failed",
    [QDL_ERROR_11_RESERVED       ] = "Reserved",
    [QDL_ERROR_BAD_SPC           ] = "Invalid security code",
    [QDL_ERROR_POWERDOWN         ] = "Power-down failed",
    [QDL_ERROR_UNSUPPORTED       ] = "NAND flash programming not supported",
    [QDL_ERROR_CMD_SEQ           ] = "Command out of sequence",
    [QDL_ERROR_CLOSE             ] = "Close failed",
    [QDL_ERROR_BAD_FEATURES      ] = "Invalid feature bits",
    [QDL_ERROR_SPACE             ] = "Out of space",
    [QDL_ERROR_BAD_SECURITY      ] = "Invalid security mode",
    [QDL_ERROR_MULTI_UNSUPPORTED ] = "Multi-image NAND not supported",
    [QDL_ERROR_POWEROFF          ] = "Power-off command not supported",
    [QDL_ERROR_CMD_UNSUPPORTED   ] = "Command not supported",
    [QDL_ERROR_BAD_CRC           ] = "Invalid CRC",
    [QDL_ERROR_STATE             ] = "Command received in invalid state",
    [QDL_ERROR_TIMEOUT           ] = "Receive timeout",
    [QDL_ERROR_IMAGE_AUTH        ] = "Image authentication error",
};

G_STATIC_ASSERT (G_N_ELEMENTS (qdl_error_str) == QDL_ERROR_LAST);

static const gchar *
qdl_error_to_string (QdlError err)
{
    return (err < QDL_ERROR_LAST ? qdl_error_str[err] : "Unknown");
}

/* most of these origin from GobiAPI_1.0.40/Core/QDLEnum.h
 *
 * The gobi-loader's snooped magic strings use types
 *   0x05 => "amss.mbn"
 *   0x06 => "apps.mbn"
 *   0x0d => "uqcn.mbn" (Gobi 2000 only)
 *  with no file header data
 *
 * The 0x80 type is snooped from the Sierra Wireless firmware
 * uploaders, using 400 bytes file header data
 */

typedef enum {
    QDL_IMAGE_TYPE_AMSS_MODEM       = 0x05, /* 05 AMSS modem image         */
    QDL_IMAGE_TYPE_AMSS_APPLICATION = 0x06, /* 06 AMSS application image   */
    QDL_IMAGE_TYPE_AMSS_UQCN        = 0x0d, /* 13 Provisioning information */
    QDL_IMAGE_TYPE_DBL              = 0x0f, /* 15 DBL image                */
    QDL_IMAGE_TYPE_OSBL             = 0x10, /* 16 OSBL image               */
    QDL_IMAGE_TYPE_CWE              = 0x80, /* 128 CWE image               */
} QdlImageType;

static const gchar *
qdl_type_to_string (QdlImageType type)
{
    switch (type) {
    case QDL_IMAGE_AMSS_MODEM:
        return "AMSS modem image";
    case QDL_IMAGE_AMSS_APPLICATION:
        return "AMSS application image";
    case QDL_IMAGE_AMSS_UQCN:
        return "AMSS Provisioning information";
    case QDL_IMAGE_DBL:
        return "DBL image";
    case QDL_IMAGE_OSBL:
        return "OSBL image";
    case QDL_IMAGE_CWE:
        return "CWE image";
    default:
        return "UNKNOWN";
    }
}

/* feature bits */
#define QDL_FEATURE_GENERIC_UNFRAMED 0x10
#define QDL_FEATURE_QDL_UNFRAMED     0x20
#define QDL_FEATURE_BAR_MODE         0x40

typedef struct _QdlHelloReq QdlHelloReq;
struct _QdlHelloReq {
    guint8 cmd; /* 0x01 */
    gchar  magic[32];
    guint8 maxver;
    guint8 minver;
    guint8 features;
}  __attribute__ ((packed));

static const QdlHelloReq qdl_hello_req = {
    .cmd = QDL_CMD_HELLO_REQ,
    .magic = { 'Q', 'C', 'O', 'M', ' ', 'h', 'i', 'g', 'h', ' ', 's', 'p', 'e', 'e', 'd', ' ', 'p', 'r', 'o', 't', 'o', 'c', 'o', 'l', ' ', 'h', 's', 't', },
    .maxver = 0,
    .minver = 0,
    .features = QDL_FEATURE_QDL_UNFRAMED | QDL_FEATURE_GENERIC_UNFRAMED,
};

typedef struct _QdlHelloRsp QdlHelloRsp;
struct _QdlHelloRsp {
    guint8  cmd; /* 0x02 */
    gchar   magic[32];
    guint8  maxver;
    guint8  minver;
    guint32 reserved1;
    guint32 reserved2;
    guint8  reserved3;
    guint16 reserved4;
    guint16 reserved5;
    guint8  features;
}  __attribute__ ((packed));

typedef struct _QdlErrRsp QdlErrRsp;
struct _QdlErrRsp {
    guint8  cmd; /* 0x0d */
    guint32 error;
    guint8  errortxt;
}  __attribute__ ((packed));

typedef struct _QdlUfopenReq QdlUfopenReq;
struct _QdlUfopenReq {
    guint8  cmd; /* 0x25 */
    guint8  type;
    guint32 length;
    guint8  windowsize;
    guint32 chunksize;
    guint16 reserved;
    /* The first 400 bytes of the image is appended to the "open
     * unframed" request on Sierra Wireless modems.  This chunk is
     * not included here as it is not a part of the request.
     *
     * The file header inclusion here seems to depend on the file
     * type
     */
}  __attribute__ ((packed));

typedef struct _QdlUfopenRsp QdlUfopenRsp;
struct _QdlUfopenRsp {
    guint8  cmd; /* 0x26 */
    guint16 status;
    guint8  windowsize;
    guint32 chunksize;
}  __attribute__ ((packed));

/* This request is not HDLC framed, so this "header" includes the crc */
typedef struct _QdlUfwriteReq QdlUfwriteReq;
struct QdlUfwriteReq {
    guint8  cmd; /* 0x27 */
    guint16 sequence;
    guint32 reserved;
    guint32 chunksize;
    guint16 crc;
}  __attribute__ ((packed));

/* the buffer most hold a file chunk + this header */
#define CHUNK   (1024 * 1024)
#define BUFSIZE (CHUNK + sizeof (QdlUfwriteReq))

/* The response is HDLC framed, so the crc is part of the framing */
typedef struct _QdlUfwriteRsp QdlUfwriteRsp;
struct _QdlUfwriteRsp {
    guint8  cmd; /* 0x28 */
    guint16 sequence;
    guint32 reserved;
    guint16 status;
}  __attribute__ ((packed));

/* 0x29 - cmd only */

typedef struct _QdlUfcloseRsp QdlUfcloseRsp;
struct QdlUfcloseRsp {
    guint8  cmd; /* 0x2a */
    guint16 status;
    guint8  type;
    guint8  errortxt;
}  __attribute__ ((packed));

/* 0x2d - cmd only */
/* 0x2e - cmd only */

typedef struct _QdlImageprefRsp QdlImageprefRsp;
struct _QdlImageprefRsp {
    guint8 cmd; /* 0x2f */
    guint8 entries;
    struct {
        guint8 type;
        gchar  id[16];
    } image[];
}  __attribute__ ((packed));

/* should the unframed open request include a file header? */
static inline size_t hdrlen(__u8 type)
{
	switch (type) {
	case QDL_IMAGE_CWE:
		return 400;
	default:
		return 0;
	}
}

/* some image types contain trailing garbage - from gobi-loader */
static inline size_t imglen(__u8 type, size_t len)
{
	switch (type) {
	case QDL_IMAGE_AMSS_MODEM:
		return len - 8;
	default:
		return len;
	}
}

static size_t create_ufopen_req(char *in, size_t filelen, __u8 type)
{
	struct qdl_ufopen_req *req = (void *)in;

	req->cmd = QDL_CMD_OPEN_UNFRAMED_REQ;
	req->type = type;
	req ->windowsize = 1; /* snooped */
	req->length = imglen(type, filelen);
	req->chunksize = imglen(type, filelen) - hdrlen(type);
	return sizeof(struct qdl_ufopen_req);
}

static size_t create_ufwrite_req(char *out, size_t chunksize, int sequence)
{
	struct qdl_ufwrite_req *req = (void *)out;

	req->cmd = QDL_CMD_WRITE_UNFRAMED_REQ;
	req->sequence = sequence;
	req->reserved = 0;
	req->chunksize = chunksize;
	req->crc = crc16(out, sizeof(*req) - 2);
	return sizeof(*req);
}

static int parse_sdp_hello(const char *in, size_t inlen)
{
	struct qdl_hello_rsp *r;
	char buf[sizeof(*r) + sizeof(__u16)];
	char magicbuf[33] = {0};
	size_t ret;

	r = (struct qdl_hello_rsp *)buf;
	ret = hdlc_unframe(in, inlen, buf, sizeof(buf));
	if (ret == sizeof(*r) && r->cmd == QDL_CMD_HELLO_RSP) {
		memcpy(magicbuf, r->magic, r->maxver <= 5 ? 24 : 32);
		debug("magic: '%s'\nmaxver: %u\nminver: %u\nfeatures: 0x%02x\n", magicbuf, r->maxver, r->minver, r->features);
		return 0;
	}
	return -1; /* unexpected error */
}

static int parse_sdp_err(const char *in, size_t inlen, bool silent)
{
	struct qdl_err_rsp *r;
	char buf[sizeof(*r) + sizeof(__u16)];
	size_t ret;

	r = (struct qdl_err_rsp *)buf;
	ret = hdlc_unframe(in, inlen, buf, sizeof(buf));
	if (ret == sizeof(*r) && r->cmd == QDL_CMD_ERROR) {
		if (!silent)
			fprintf(stderr, "SDP error %d (%d): %s\n", r->error, r->errortxt, sdperr2str(r->error));
		return -r->error;
	}
	return -1; /* not an proper error frame */
}

static int parse_ufopen(const char *in, size_t inlen)
{
	struct qdl_ufopen_rsp *r;
	char buf[sizeof(*r) + sizeof(__u16)];
	size_t ret;

	r = (struct qdl_ufopen_rsp *)buf;
	ret = hdlc_unframe(in, inlen, buf, sizeof(buf));
	if (ret != sizeof(*r) || r->cmd != QDL_CMD_OPEN_UNFRAMED_RSP)
		return -1;
	debug("status=%d, windowsize=%d, chunksize=%d\n", r->status, r->windowsize, r->chunksize);
	return -r->status;
}

static int parse_ufwrite(const char *in, size_t inlen)
{
	struct qdl_ufwrite_rsp *r;
	char buf[sizeof(*r) + sizeof(__u16)];
	size_t ret;

	r = (struct qdl_ufwrite_rsp *)buf;
	ret = hdlc_unframe(in, inlen, buf, sizeof(buf));
	if (ret != sizeof(*r) || r->cmd != QDL_CMD_WRITE_UNFRAMED_RSP)
		return -1;
	if (r->status) {
		fprintf(stderr, "seq 0x%04x status=%d\n", r->sequence, r->status);
		return -r->status;
	}
	debug("ack: %d\n", r->sequence);
	return r->sequence;
}

static int parse_ufdone(const char *in, size_t inlen)
{
	struct qdl_ufclose_rsp *r;
	char buf[sizeof(*r) + sizeof(__u16)];
	size_t ret;

	r = (struct qdl_ufclose_rsp *)buf;
	ret = hdlc_unframe(in, inlen, buf, sizeof(buf));
	if (ret != sizeof(*r) || r->cmd != QDL_CMD_SESSION_DONE_RSP)
		return -1;
	debug("UF close: status=%d, type=%d, errortxt=%d\n", r->status, r->type, r->errortxt);
	return -r->status;
}

/* read and parse QDL if available.  Will return unless data is
 * available withing 5 ms
 */
static int read_and_parse(int fd, bool silent)
{
	char *end, *p, rbuf[512];
	int rlen, framelen, ret = 0;
	fd_set rd;
	struct timeval tv = { .tv_sec = 1, .tv_usec = 0, };

	FD_ZERO(&rd);
	FD_SET(fd, &rd);
	if (select(fd + 1, &rd, NULL, NULL, &tv) <= 0) {
		debug("timeout: no data read\n");
		return 0;
	}

	rlen = read(fd, rbuf, sizeof(rbuf));
	if (rlen <= 0)
		return rlen;
	print_packet("read", rbuf, rlen);

	p = rbuf;
	while (rlen > 0 && (end = memchr(p + 1, CONTROL, rlen - 1)) != NULL) {
		end++;
		framelen = end - p;
		switch (p[1]) {
		case QDL_CMD_ERROR:
			ret = parse_sdp_err(p, framelen, silent);
			break;
		case QDL_CMD_HELLO_RSP: /* == DLOAD_ACK */
			if (framelen != 5)
				ret = parse_sdp_hello(p, framelen);
			else
				debug("Got DLOAD_ACK\n");
			break;
		case QDL_CMD_OPEN_UNFRAMED_RSP:
			ret = parse_ufopen(p, framelen);
			break;
		case QDL_CMD_WRITE_UNFRAMED_RSP:
			ret = parse_ufwrite(p, framelen);
			break;
		case QDL_CMD_SESSION_DONE_RSP:
			ret = parse_ufdone(p, framelen);
			break;
		default:
			fprintf(stderr, "Unsupported response code: 0x%02x\n", p[1]);
		}
		if (ret < 0)
			return ret;
		p = end;
		rlen -= framelen;
	}
	return ret;
}

/******************************************************************************/
/* Serial port */

static gint
serial_open (GFile   *tty,
             GError **error)
{
    struct termios  terminal_data;
    gint            fd;
    gchar          *path;

    path = g_file_get_path (tty);
    g_debug ("[qfu-download] opening TTY: %s", path);

    fd = open (path, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "error opening serial device: %s",
                     g_strerror (errno));
    } else {
        g_debug ("[qfu-download] setting terminal in raw mode...");
        tcgetattr (fd, &terminal_data);
        cfmakeraw (&terminal_data);
        tcsetattr (fd, TCSANOW, &terminal_data);
    }

    g_free (path);

    return fd;
}

/******************************************************************************/

typedef struct {
    GFile     *tty;
    GFile     *image;
    GFileInfo *image_info;
    gint       fd;
} RunContext;

static void
run_context_free (RunContext *ctx)
{
    g_object_unref (ctx->image_info);
    g_object_unref (ctx->image);
    g_object_unref (ctx->tty);
    g_slice_free (RunContext, ctx);
}

gboolean
qfu_download_helper_run_finish (GAsyncResult  *res,
                                GError       **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

static void
download_helper_run (GTask *task)
{
    RunContext *ctx;
    GError     *error = NULL;

    ctx = (RunContext *) g_task_get_task_data (task);

    /* Open serial */
    ctx->fd = serial_open (ctx->tty, &error);
    if (ctx->fd < 0) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    /* switch to SDP - this is required for some modems like MC7710 */
    if (write_hdlc (ctx->fd, (const gchar *) &dload_sdp, sizeof (dload_sdp)) < 0)
    ret = read_and_parse (serfd, true);
}

void
qfu_download_helper_run (GFile               *tty,
                         GFile               *image,
                         GFileInfo           *image_info,
                         GCancellable        *cancellable,
                         GAsyncReadyCallback  callback,
                         gpointer             user_data)
{
    GTask      *task;
    RunContext *ctx;

    g_assert (G_IS_FILE (tty));
    g_assert (G_IS_FILE (image));

    ctx = g_slice_new0 (RunContext);
    ctx->tty = g_object_ref (tty);
    ctx->image = g_object_ref (image);
    ctx->image_info = g_object_ref (image_info);

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_task_data (task, ctx, (GDestroyNotify) run_context_free);

    /* Run */
    download_helper_run (task);
}
