include $(top_srcdir)/gtester.make

noinst_PROGRAMS = \
	test-utils \
	test-message \
	test-generated

TEST_PROGS += $(noinst_PROGRAMS)

test_utils_SOURCES = \
	test-utils.c
test_utils_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/libqmi-glib \
	-I$(top_srcdir)/src/libqmi-glib/generated \
	-I$(top_builddir)/src/libqmi-glib \
	-I$(top_builddir)/src/libqmi-glib/generated \
	-DLIBQMI_GLIB_COMPILATION
test_utils_LDADD = \
	$(top_builddir)/src/libqmi-glib/libqmi-glib.la \
	$(GLIB_LIBS)

test_message_SOURCES = \
	test-message.c
test_message_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/libqmi-glib \
	-I$(top_srcdir)/src/libqmi-glib/generated \
	-I$(top_builddir)/src/libqmi-glib \
	-I$(top_builddir)/src/libqmi-glib/generated \
	-DLIBQMI_GLIB_COMPILATION
test_message_LDADD = \
	$(top_builddir)/src/libqmi-glib/libqmi-glib.la \
	$(GLIB_LIBS)

test_generated_SOURCES = \
	test-fixture.h test-fixture.c \
	test-port-context.h test-port-context.c \
	test-generated.c
test_generated_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/libqmi-glib \
	-I$(top_srcdir)/src/libqmi-glib/generated \
	-I$(top_builddir)/src/libqmi-glib \
	-I$(top_builddir)/src/libqmi-glib/generated \
	-DLIBQMI_GLIB_COMPILATION
test_generated_LDADD = \
	$(top_builddir)/src/libqmi-glib/libqmi-glib.la \
	$(GLIB_LIBS)
