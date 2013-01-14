
lib_LTLIBRARIES = libmbim-glib.la

libmbim_glib_la_CPPFLAGS = \
	$(LIBMBIM_GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-I$(top_srcdir)/libmbim-glib \
	-DLIBMBIM_GLIB_COMPILATION \
	-DG_LOG_DOMAIN=\"Mbim\"

libmbim_glib_la_SOURCES = \
	libmbim-glib.h

libmbim_glib_la_LIBADD = \
	$(LIBMBIM_GLIB_LIBS)

includedir = @includedir@/libmbim-glib
include_HEADERS = \
	libmbim-glib.h
