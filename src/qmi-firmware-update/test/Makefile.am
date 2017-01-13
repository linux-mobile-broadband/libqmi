include $(top_srcdir)/gtester.make

noinst_PROGRAMS = \
	test-utils \
	$(NULL)

TEST_PROGS += $(noinst_PROGRAMS)

test_utils_SOURCES = \
	test-utils.c \
	$(NULL)

test_utils_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/libqmi-glib \
	-I$(top_srcdir)/src/libqmi-glib/generated \
	-I$(top_builddir)/src/libqmi-glib \
	-I$(top_builddir)/src/libqmi-glib/generated \
	-I$(top_srcdir)/src/qmi-firmware-update \
	-I$(top_builddir)/src/qmi-firmware-update \
	-DLIBQMI_GLIB_COMPILATION \
	$(NULL)

test_utils_LDADD = \
	$(top_builddir)/src/qmi-firmware-update/libutils.la \
	$(GLIB_LIBS) \
	$(NULL)
