include $(top_srcdir)/gtester.make

AM_CFLAGS = \
	$(WARN_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(QRTR_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/libqmi-glib \
	-I$(top_srcdir)/src/libqmi-glib/generated \
	-I$(top_builddir)/src/libqmi-glib \
	-I$(top_builddir)/src/libqmi-glib/generated \
	-DLIBQMI_GLIB_COMPILATION \
	$(NULL)

AM_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(QRTR_LIBS) \
	$(GLIB_LIBS) \
	$(NULL)

noinst_PROGRAMS = \
	test-utils \
	test-compat-utils \
	test-message \
	test-generated \
	$(NULL)

TEST_PROGS += $(noinst_PROGRAMS)

test_utils_SOURCES = test-utils.c
test_utils_LDADD = $(top_builddir)/src/libqmi-glib/libqmi-glib.la

test_compat_utils_SOURCES = test-compat-utils.c
test_compat_utils_CPPFLAGS = -Wno-deprecated-declarations
test_compat_utils_LDADD = $(top_builddir)/src/libqmi-glib/libqmi-glib.la

test_message_SOURCES = test-message.c
test_message_LDADD = $(top_builddir)/src/libqmi-glib/libqmi-glib.la

test_generated_SOURCES = \
	test-fixture.h test-fixture.c \
	test-port-context.h test-port-context.c \
	test-generated.c \
	$(NULL)
test_generated_LDADD = $(top_builddir)/src/libqmi-glib/libqmi-glib.la
