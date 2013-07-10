
SUBDIRS = . test

bin_PROGRAMS = qmicli

qmicli_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/libqmi-glib \
	-I$(top_srcdir)/src/libqmi-glib/generated \
	-I$(top_builddir)/src/libqmi-glib \
	-I$(top_builddir)/src/libqmi-glib/generated

qmicli_SOURCES = \
	qmicli.c \
	qmicli.h \
	qmicli-helpers.c \
	qmicli-helpers.h \
	qmicli-dms.c \
	qmicli-wds.c \
	qmicli-nas.c \
	qmicli-pbm.c \
	qmicli-uim.c

qmicli_LDADD = \
	$(GLIB_LIBS) \
	$(top_builddir)/src/libqmi-glib/libqmi-glib.la
