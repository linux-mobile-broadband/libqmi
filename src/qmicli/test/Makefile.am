include $(top_srcdir)/gtester.make

noinst_PROGRAMS = \
	test-helpers

TEST_PROGS += $(noinst_PROGRAMS)

test_helpers_SOURCES = \
	test-helpers.c

test_helpers_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(QRTR_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/qmicli \
	-I$(top_srcdir)/src/libqmi-glib \
	-I$(top_srcdir)/src/libqmi-glib/generated \
	-I$(top_builddir)/src/libqmi-glib \
	-I$(top_builddir)/src/libqmi-glib/generated \
	$(NULL)

test_helpers_LDADD = \
	$(GLIB_LIBS) \
	$(top_builddir)/src/qmicli/libhelpers.la \
	$(top_builddir)/src/libqmi-glib/libqmi-glib.la
