
SUBDIRS = . test

noinst_LTLIBRARIES = libhelpers.la

libhelpers_la_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(MBIM_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/libqmi-glib \
	-I$(top_srcdir)/src/libqmi-glib/generated \
	-I$(top_builddir)/src/libqmi-glib \
	-I$(top_builddir)/src/libqmi-glib/generated \
	$(NULL)

if QMI_QRTR_SUPPORTED
libhelpers_la_CPPFLAGS += \
	-I$(top_srcdir)/src/libqrtr-glib \
	-I$(top_builddir)/src/libqrtr-glib \
	$(NULL)
endif

libhelpers_la_SOURCES = \
	qmicli-helpers.c \
	qmicli-helpers.h \
	$(NULL)

libhelpers_la_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(GLIB_LIBS) \
	$(NULL)

bin_PROGRAMS = qmicli

qmicli_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(MBIM_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/libqmi-glib \
	-I$(top_srcdir)/src/libqmi-glib/generated \
	-I$(top_builddir)/src/libqmi-glib \
	-I$(top_builddir)/src/libqmi-glib/generated \
	$(NULL)

if QMI_QRTR_SUPPORTED
qmicli_CPPFLAGS += \
	-I$(top_srcdir)/src/libqrtr-glib \
	-I$(top_builddir)/src/libqrtr-glib \
	$(NULL)
endif

qmicli_SOURCES = \
	qmicli.c \
	qmicli.h \
	qmicli-dms.c \
	qmicli-wds.c \
	qmicli-nas.c \
	qmicli-pbm.c \
	qmicli-pdc.c \
	qmicli-uim.c \
	qmicli-wms.c \
	qmicli-wda.c \
	qmicli-voice.c \
	qmicli-loc.c \
	qmicli-qos.c \
	qmicli-gas.c \
	qmicli-gms.c \
	qmicli-dsd.c \
	qmicli-sar.c

qmicli_LDADD = \
	libhelpers.la \
	$(top_builddir)/src/libqmi-glib/libqmi-glib.la \
	$(NULL)

if QMI_QRTR_SUPPORTED
qmicli_LDADD += \
	$(top_builddir)/src/libqrtr-glib/libqrtr-glib.la \
	$(NULL)
endif

qmicli_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(MBIM_LIBS) \
	$(GLIB_LIBS) \
	$(NULL)

completiondir = $(datadir)/bash-completion/completions

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(completiondir)
	$(INSTALL_DATA) $(srcdir)/qmicli-completion $(DESTDIR)$(completiondir)/qmicli

uninstall-hook:
	rm -f $(DESTDIR)$(completiondir)/qmicli

EXTRA_DIST = qmicli-completion
