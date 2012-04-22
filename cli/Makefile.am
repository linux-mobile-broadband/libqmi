bin_PROGRAMS = qmicli

qmicli_CPPFLAGS = \
	$(QMICLI_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src

qmicli_SOURCES = \
	qmicli.c \
	qmicli.h \
	qmicli-ctl.c

qmicli_LDADD = \
	$(QMICLI_LIBS) \
	$(top_builddir)/src/libqmi-glib.la
