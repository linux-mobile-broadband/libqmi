
noinst_PROGRAMS = \
	test-utils

test_utils_SOURCES = \
	test-utils.c

test_utils_CPPFLAGS = \
	$(LIBQMI_GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libqmi-glib \
	-I$(top_builddir)/libqmi-glib

test_utils_LDADD = \
	$(top_builddir)/libqmi-glib/libqmi-glib.la \
	$(LIBQMI_GLIB_LIBS)

if WITH_TESTS

check-local: test-utils
	$(abs_builddir)/test-utils

endif
