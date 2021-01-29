
libexec_PROGRAMS = qmi-proxy

qmi_proxy_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(QRTR_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/libqmi-glib \
	-I$(top_srcdir)/src/libqmi-glib/generated \
	-I$(top_builddir)/src/libqmi-glib \
	-I$(top_builddir)/src/libqmi-glib/generated \
	$(NULL)

qmi_proxy_SOURCES = qmi-proxy.c

qmi_proxy_LDADD = \
	$(GLIB_LIBS) \
	$(QRTR_LIBS) \
	$(top_builddir)/src/libqmi-glib/libqmi-glib.la

#Install udev rules only if configured with --enable-qmi-username
if QMI_USERNAME_ENABLED
udevrulesdir = $(UDEV_BASE_DIR)/rules.d
udevrules_DATA = 76-qmi-proxy-device-ownership.rules
endif

EXTRA_DIST = \
	76-qmi-proxy-device-ownership.rules.in
