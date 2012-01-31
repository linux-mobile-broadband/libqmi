/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <glib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

typedef u_int16_t u16;
typedef u_int8_t u8;
typedef u_int32_t u32;
typedef u_int64_t u64;

typedef unsigned int qbool;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/***********************************************************/

static void
print_buf (const char *detail, const char *buf, size_t len)
{
    int i = 0, z;
    qbool newline = FALSE, indent = FALSE;
    char *f;
    guint flen;

    f = g_strdup_printf ("%s (%zu)  ", detail, len);
    flen = strlen (f);
    g_print ("%s", f);
    for (i = 0; i < len; i++) {
        if (indent) {
            z = flen;
            while (z--)
                g_print (" ");
            indent = FALSE;
        }
        g_print ("%02x ", buf[i] & 0xFF);
        if (((i + 1) % 16) == 0) {
            g_print ("\n");
            newline = TRUE;
            indent = TRUE;
        } else
            newline = FALSE;
    }

    if (!newline)
        g_print ("\n");
}

/**************************************************************/

#define QMI_SVC_CTL 0
#define QMI_SVC_WDS 1
#define QMI_SVC_DMS 2
#define QMI_SVC_NAS 3

struct qmux {
	u8 tf;	/* always 1 */
	u16 len;
	u8 ctrl;
	u8 service;
	u8 qmicid;
} __attribute__((__packed__));

struct getcid_req {
	struct qmux header;
	u8 req;
	u8 tid;
	u16 msgid;
	u16 tlvsize;
	u8 service;
	u16 size;
	u8 qmisvc;
} __attribute__((__packed__));

struct releasecid_req {
	struct qmux header;
	u8 req;
	u8 tid;
	u16 msgid;
	u16 tlvsize;
	u8 rlscid;
	u16 size;
	u16 cid;
} __attribute__((__packed__));

struct ready_req {
	struct qmux header;
	u8 req;
	u8 tid;
	u16 msgid;
	u16 tlvsize;
} __attribute__((__packed__));

struct seteventreport_req {
	struct qmux header;
	u8 req;
	u16 tid;
	u16 msgid;
	u16 tlvsize;
	u8 reportchanrate;
	u16 size;
	u8 period;
	u32 mask;
} __attribute__((__packed__));

struct getpkgsrvcstatus_req {
	struct qmux header;
	u8 req;
	u16 tid;
	u16 msgid;
	u16 tlvsize;
} __attribute__((__packed__));

struct getmeid_req {
	struct qmux header;
	u8 req;
	u16 tid;
	u16 msgid;
	u16 tlvsize;
} __attribute__((__packed__));

struct qmiwds_stats {
	u32 txok;
	u32 rxok;
	u32 txerr;
	u32 rxerr;
	u32 txofl;
	u32 rxofl;
	u64 txbytesok;
	u64 rxbytesok;
	qbool linkstate;
	qbool reconfigure;
};

const size_t qmux_size = sizeof(struct qmux);

static void
qmux_fill(struct qmux *qmux, u8 service, u16 cid, u16 size)
{
	qmux->tf = 1;
	qmux->len = size - 1;
	qmux->ctrl = 0;
	qmux->service = cid & 0xff;
	qmux->qmicid = cid >> 8;
}

static void *
qmictl_new_getcid(u8 tid, u8 svctype, size_t *size)
{
	struct getcid_req *req;

    req = g_malloc0 (sizeof (*req));
	req->req = 0x00;
	req->tid = tid;
	req->msgid = 0x0022;
	req->tlvsize = 0x0004;
	req->service = 0x01;
	req->size = 0x0001;
	req->qmisvc = svctype;
	*size = sizeof(*req);
    qmux_fill (&req->header, QMI_SVC_CTL, 0, *size);
	return req;
}

static void *
qmictl_new_releasecid(u8 tid, u16 cid, size_t *size)
{
	struct releasecid_req *req;

    req = g_malloc0 (sizeof (*req));
	req->req = 0x00;
	req->tid = tid;
	req->msgid = 0x0023;
	req->tlvsize = 0x05;
	req->rlscid = 0x01;
	req->size = 0x0002;
	req->cid = cid;
	*size = sizeof(*req);
    qmux_fill (&req->header, QMI_SVC_CTL, 0, *size);
	return req;
}

static void *
qmictl_new_ready(u8 tid, size_t *size)
{
	struct ready_req *req;

    req = g_malloc0 (sizeof (*req));
	req->req = 0x00;
	req->tid = tid;
	req->msgid = 0x21;
	req->tlvsize = 0;
	*size = sizeof(*req);
    qmux_fill (&req->header, QMI_SVC_CTL, 0, *size);
	return req;
}

static void *
qmiwds_new_seteventreport(u8 tid, size_t *size)
{
	struct seteventreport_req *req;

    req = g_malloc0 (sizeof (*req));
	req->req = 0x00;
	req->tid = tid;
	req->msgid = 0x0001;
	req->tlvsize = 0x0008;
	req->reportchanrate = 0x11;
	req->size = 0x0005;
	req->period = 0x01;
	req->mask = 0x000000ff;
	*size = sizeof(*req);
	return req;
}

static void *
qmiwds_new_getpkgsrvcstatus(u8 tid, size_t *size)
{
	struct getpkgsrvcstatus_req *req;

    req = g_malloc0 (sizeof (*req));
	req->req = 0x00;
	req->tid = tid;
	req->msgid = 0x22;
	req->tlvsize = 0x0000;
	*size = sizeof(*req);
	return req;
}

static void *
qmidms_new_getmeid(u16 cid, u8 tid, size_t *size)
{
	struct getmeid_req *req;

    req = g_malloc0 (sizeof (*req));
	req->req = 0x00;
	req->tid = tid;
	req->msgid = 0x25;
	req->tlvsize = 0x0000;
	*size = sizeof(*req);
    qmux_fill (&req->header, QMI_SVC_WDS, cid, *size);
	return req;
}

static int
qmux_parse(u16 *cid, void *buf, size_t size)
{
	struct qmux *qmux = buf;

	if (!buf || size < 12)
		return -ENOMEM;

	if (qmux->tf != 1 || qmux->len != size - 1 || qmux->ctrl != 0x80)
		return -EINVAL;

	*cid = (qmux->qmicid << 8) + qmux->service;
	return sizeof(*qmux);
}

static u16
tlv_get(void *msg, u16 msgsize, u8 type, void *buf, u16 bufsize)
{
	u16 pos;
	u16 msize = 0;

	if (!msg || !buf)
		return -ENOMEM;

	for (pos = 4;  pos + 3 < msgsize; pos += msize + 3) {
		msize = *(u16 *)(msg + pos + 1);
		if (*(u8 *)(msg + pos) == type) {
			if (bufsize < msize)
				return -ENOMEM;

			memcpy(buf, msg + pos + 3, msize);
			return msize;
		}
	}

	return -ENOMSG;
}

static int
qmi_msgisvalid(void *msg, u16 size)
{
	char tlv[4];

	if (tlv_get(msg, size, 2, &tlv[0], 4) == 4) {
		if (*(u16 *)&tlv[0] != 0)
			return *(u16 *)&tlv[2];
		else
			return 0;
	}
	return -ENOMSG;
}

static int
qmi_msgid(void *msg, u16 size)
{
	return size < 2 ? -ENODATA : *(u16 *)msg;
}

static int
qmictl_getcid_resp(void *buf, u16 size, u16 *cid)
{
	int result;
	u8 offset = sizeof(struct qmux) + 2;

	if (!buf || size < offset)
		return -ENOMEM;

	buf = buf + offset;
	size -= offset;

	result = qmi_msgid(buf, size);
	if (result != 0x22)
		return -EFAULT;

	result = qmi_msgisvalid(buf, size);
	if (result != 0)
		return -EFAULT;

	result = tlv_get(buf, size, 0x01, cid, 2);
	if (result != 2)
		return -EFAULT;

	return 0;
}

static int
qmictl_releasecid_resp(void *buf, u16 size)
{
	int result;
	u8 offset = sizeof(struct qmux) + 2;

	if (!buf || size < offset)
		return -ENOMEM;

	buf = buf + offset;
	size -= offset;

	result = qmi_msgid(buf, size);
	if (result != 0x23)
		return -EFAULT;

	result = qmi_msgisvalid(buf, size);
	if (result != 0)
		return -EFAULT;

	return 0;
}

static int
qmiwds_event_resp(void *buf, u16 size, struct qmiwds_stats *stats)
{
	int result;
	u8 status[2];

	u8 offset = sizeof(struct qmux) + 3;

	if (!buf || size < offset || !stats)
		return -ENOMEM;

	buf = buf + offset;
	size -= offset;

	result = qmi_msgid(buf, size);
	if (result == 0x01) {
		tlv_get(buf, size, 0x10, &stats->txok, 4);
		tlv_get(buf, size, 0x11, &stats->rxok, 4);
		tlv_get(buf, size, 0x12, &stats->txerr, 4);
		tlv_get(buf, size, 0x13, &stats->rxerr, 4);
		tlv_get(buf, size, 0x14, &stats->txofl, 4);
		tlv_get(buf, size, 0x15, &stats->rxofl, 4);
		tlv_get(buf, size, 0x19, &stats->txbytesok, 8);
		tlv_get(buf, size, 0x1A, &stats->rxbytesok, 8);
	} else if (result == 0x22) {
		result = tlv_get(buf, size, 0x01, &status[0], 2);
		if (result >= 1)
			stats->linkstate = status[0] == 0x02;
		if (result == 2)
			stats->reconfigure = status[1] == 0x01;

		if (result < 0)
			return result;
	} else {
		return -EFAULT;
	}

	return 0;
}

static int
qmidms_meid_resp(void *buf, u16 size, char *meid, int meidsize)
{
	int result;

	u8 offset = sizeof(struct qmux) + 3;

	if (!buf || size < offset || meidsize < 14)
		return -ENOMEM;

	buf = buf + offset;
	size -= offset;

	result = qmi_msgid(buf, size);
	if (result != 0x25)
		return -EFAULT;

	result = qmi_msgisvalid(buf, size);
	if (result)
		return -EFAULT;

	result = tlv_get(buf, size, 0x12, meid, 14);
	if (result != 14)
		return -EFAULT;

	return 0;
}

/*****************************************************/

static size_t
send_and_wait_reply (int fd, void *b, size_t blen, char *reply, size_t rlen)
{
    ssize_t num;
    fd_set in;
    int result;
    struct timeval timeout = { 1, 0 };

    print_buf (">>>", b, blen);
    num = write (fd, b, blen);
    if (num != blen) {
        g_warning ("Failed to write: wrote %zd err %d", num, errno);
        return 0;
    }

    FD_ZERO (&in);
    FD_SET (fd, &in);
    result = select (fd + 1, &in, NULL, NULL, &timeout);
    if (result != 1 || !FD_ISSET (fd, &in)) {
        g_warning ("No data pending");
        return 0;
    }

    errno = 0;
    num = read (fd, reply, rlen);
    if (num < 0) {
        g_warning ("read error %d", errno);
        return 0;
    }
    print_buf ("<<<", reply, num);
    return num;
}

int main(int argc, char *argv[])
{
    int fd;
    void *b;
    size_t blen;
    u8 ctl_tid = 1;
    u8 dms_tid = 1;
    u16 dms_cid = 0;
    char reply[2048];
    size_t rlen;
    char meid[16];
    int err;

    if (argc != 2) {
        g_warning ("usage: %s <port>", argv[0]);
        return 1;
    }

    errno = 0;
    fd = open (argv[1], O_RDWR | O_EXCL | O_NONBLOCK | O_NOCTTY);
    if (fd < 0) {
        g_warning ("%s: open failed: %d", argv[1], errno);
        return 1;
    }

    /* Send the ready request */
    b = qmictl_new_ready (ctl_tid++, &blen);
    g_assert (b);

    rlen = send_and_wait_reply (fd, b, blen, reply, sizeof (reply));
    g_free (b);
    if (rlen <= 0)
        goto out;

    /* Allocate a DMS client ID */
    b = qmictl_new_getcid (ctl_tid++, QMI_SVC_DMS, &blen);
    g_assert (b);

    rlen = send_and_wait_reply (fd, b, blen, reply, sizeof (reply));
    g_free (b);
    if (rlen <= 0)
        goto out;

    err = qmictl_getcid_resp (reply, rlen, &dms_cid);
    if (err < 0) {
        g_warning ("Failed to get DMS client ID: %d", err);
        goto out;
    }

    g_message ("DMS CID %d 0x%X", dms_cid, dms_cid);

    /* Get the MEID */
    b = qmidms_new_getmeid(dms_cid, dms_tid++, &blen);
    g_assert (b);

    rlen = send_and_wait_reply (fd, b, blen, reply, sizeof (reply));
    g_free (b);
    if (rlen <= 0)
        goto out;

    memset (meid, 0, sizeof (meid));
    err = qmidms_meid_resp (reply, rlen, meid, sizeof (meid) - 1);
    if (err < 0)
        g_warning ("Failed to get MEID: %d", err);
    else
        g_message ("MEID: %s", meid);

    /* Relese the DMS client ID */
    b = qmictl_new_releasecid (ctl_tid++, dms_cid, &blen);
    g_assert (b);

    rlen = send_and_wait_reply (fd, b, blen, reply, sizeof (reply));
    g_free (b);
    if (rlen <= 0)
        goto out;

    err = qmictl_releasecid_resp (reply, rlen);
    if (err < 0) {
        g_warning ("Failed to release DMS client ID: %d", err);
        goto out;
    }

out:
    close (fd);
    return 0;
}

