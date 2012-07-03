bin_PROGRAMS = qmicli

qmicli_CPPFLAGS = \
	$(QMICLI_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libqmi-glib \
	-I$(top_builddir)/libqmi-glib

qmicli_SOURCES = \
	qmicli.c \
	qmicli.h \
	qmicli-dms.c \
	qmicli-wds.c

qmicli_LDADD = \
	$(QMICLI_LIBS) \
	$(top_builddir)/libqmi-glib/libqmi-glib.la
