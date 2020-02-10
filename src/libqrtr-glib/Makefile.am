
# ship under libqmi-glib for now
includedir = @includedir@/libqmi-glib

if QMI_QRTR_SUPPORTED

noinst_LTLIBRARIES = libqrtr-glib.la

libqrtr_glib_la_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-I$(top_srcdir)/src/libqrtr-glib \
	-I$(top_builddir)/src/libqrtr-glib \
	-DLIBQRTR_GLIB_COMPILATION \
	-DLIBEXEC_PATH=\""$(libexecdir)"\" \
	-DG_LOG_DOMAIN=\"Qrtr\" \
	$(NULL)

libqrtr_glib_la_SOURCES = \
	libqrtr-glib.h \
	qrtr-control-socket.h qrtr-control-socket.c \
	qrtr-node.h qrtr-node.c \
	qrtr-utils.h qrtr-utils.c \
	$(NULL)

libqrtr_glib_la_LDFLAGS = \
	$(WARN_CFLAGS) \
	$(GLIB_LIBS) \
	$(NULL)

include_HEADERS = \
	libqrtr-glib.h \
	qrtr-control-socket.h \
	qrtr-node.h \
	qrtr-utils.h \
	$(NULL)

endif
