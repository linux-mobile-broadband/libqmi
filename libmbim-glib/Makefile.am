
SUBDIRS = generated . test

lib_LTLIBRARIES = libmbim-glib.la

libmbim_glib_la_CPPFLAGS = \
	$(LIBMBIM_GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-I$(top_srcdir)/libmbim-glib \
	-I$(top_srcdir)/libmbim-glib/generated \
	-I$(top_builddir)/libmbim-glib/generated \
	-DLIBMBIM_GLIB_COMPILATION \
	-DG_LOG_DOMAIN=\"Mbim\"

libmbim_glib_la_SOURCES = \
	libmbim-glib.h \
	mbim-errors.h \
	mbim-utils.h mbim-utils.c \
	mbim-uuid.h mbim-uuid.c \
	mbim-message-private.h mbim-message.h mbim-message.c

libmbim_glib_la_LIBADD = \
	${top_builddir}/libmbim-glib/generated/libmbim-glib-generated.la \
	$(LIBMBIM_GLIB_LIBS)

includedir = @includedir@/libmbim-glib
include_HEADERS = \
	libmbim-glib.h \
	mbim-errors.h \
	mbim-utils.h \
	mbim-uuid.h \
	mbim-message.h
