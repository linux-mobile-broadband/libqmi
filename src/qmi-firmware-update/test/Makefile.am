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
	-I$(top_srcdir)/src/qmi-firmware-update \
	-DLIBQMI_GLIB_COMPILATION \
	$(NULL)

AM_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(GLIB_LIBS) \
	$(NULL)

noinst_PROGRAMS = \
	test-utils \
	test-firehose \
	$(NULL)

TEST_PROGS += $(noinst_PROGRAMS)

test_utils_SOURCES = test-utils.c
test_utils_LDADD = $(top_builddir)/src/qmi-firmware-update/libutils.la

test_firehose_SOURCES = test-firehose.c
test_firehose_LDADD = $(top_builddir)/src/qmi-firmware-update/libfirehose.la
