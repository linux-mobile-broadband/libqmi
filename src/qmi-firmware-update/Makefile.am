
bin_PROGRAMS = qmi-firmware-update

qmi_firmware_update_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(GUDEV_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/libqmi-glib \
	-I$(top_srcdir)/src/libqmi-glib/generated \
	-I$(top_builddir)/src/libqmi-glib \
	-I$(top_builddir)/src/libqmi-glib/generated \
	$(NULL)

qmi_firmware_update_SOURCES = \
	qfu-main.c \
	qfu-updater.h qfu-updater.c \
	qfu-udev-helpers.h qfu-udev-helpers.c \
	$(NULL)

qmi_firmware_update_LDADD = \
	$(GUDEV_LIBS) \
	$(GLIB_LIBS) \
	$(top_builddir)/src/libqmi-glib/libqmi-glib.la \
	$(NULL)
