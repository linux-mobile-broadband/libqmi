
bin_PROGRAMS = qmi-firmware-update

qmi_firmware_update_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/libqmi-glib \
	-I$(top_srcdir)/src/libqmi-glib/generated \
	-I$(top_builddir)/src/libqmi-glib \
	-I$(top_builddir)/src/libqmi-glib/generated \
	$(NULL)

qmi_firmware_update_SOURCES = \
	qfu-main.c \
	$(NULL)

qmi_firmware_update_LDADD = \
	$(GLIB_LIBS) \
	$(top_builddir)/src/libqmi-glib/libqmi-glib.la \
	$(NULL)
