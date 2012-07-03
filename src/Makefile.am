
lib_LTLIBRARIES = libqmi-glib.la

libqmi_glib_la_CPPFLAGS = \
	$(LIBQMI_GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src

libqmi_glib_la_SOURCES = \
	libqmi-glib.h

libqmi_glib_la_LIBADD = \
	$(LIBQMI_GLIB_LIBS)

includedir = @includedir@/libqmi-glib
include_HEADERS = \
	libqmi-glib.h
