
SUBDIRS = . test

noinst_LTLIBRARIES = libutils.la

libutils_la_SOURCES = \
	qfu-utils.h qfu-utils.c \
	$(NULL)

libutils_la_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	-I$(top_srcdir)/src/libqmi-glib \
	-I$(top_srcdir)/src/libqmi-glib/generated \
	-I$(top_builddir)/src/libqmi-glib \
	-I$(top_builddir)/src/libqmi-glib/generated \
	$(NULL)

libutils_la_LIBADD = \
	$(top_builddir)/src/libqmi-glib/libqmi-glib.la \
	$(GLIB_LDFLAGS) \
	$(NULL)

bin_PROGRAMS = qmi-firmware-update

qmi_firmware_update_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(GUDEV_CFLAGS) \
	$(MBIM_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/libqmi-glib \
	-I$(top_srcdir)/src/libqmi-glib/generated \
	-I$(top_builddir)/src/libqmi-glib \
	-I$(top_builddir)/src/libqmi-glib/generated \
	$(NULL)

ENUMS = \
	$(top_srcdir)/src/qmi-firmware-update/qfu-image.h \
	$(top_srcdir)/src/qmi-firmware-update/qfu-qdl-message.h \
	$(top_srcdir)/src/qmi-firmware-update/qfu-dload-message.h \
	$(NULL)

ENUMS_GENERATED = \
	qfu-enum-types.h qfu-enum-types.c \
	$(NULL)

qfu-enum-types.h: Makefile.am $(ENUMS) $(top_srcdir)/build-aux/templates/qmi-enum-types-template.h
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#ifndef QFU_ENUM_TYPES_H\n#define QFU_ENUM_TYPES_H\n#include \"qfu-image.h\"\n#include \"qfu-qdl-message.h\"\n#include \"qfu-dload-message.h\"\n" \
		--template $(top_srcdir)/build-aux/templates/qmi-enum-types-template.h \
		--ftail "#endif /* __QFUENUM_TYPES_H__ */\n" \
		$(ENUMS) > $@

qfu-enum-types.c: $(ENUMS) qfu-enum-types.h $(top_srcdir)/build-aux/templates/qmi-enum-types-template.c
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#include \"qfu-enum-types.h\"\n" \
		--template $(top_srcdir)/build-aux/templates/qmi-enum-types-template.c \
		$(ENUMS) > $@

nodist_qmi_firmware_update_SOURCES = \
	$(ENUMS_GENERATED) \
	$(NULL)

qmi_firmware_update_SOURCES = \
	qfu-main.c \
	qfu-device-selection.h qfu-device-selection.c \
	qfu-operation.h \
	qfu-operation-update.c \
	qfu-operation-verify.c \
	qfu-operation-reset.c \
	qfu-log.h qfu-log.c \
	qfu-updater.h qfu-updater.c \
	qfu-udev-helpers.h qfu-udev-helpers.c \
	qfu-image.h qfu-image.c \
	qfu-image-cwe.h qfu-image-cwe.c \
	qfu-image-factory.h qfu-image-factory.c \
	qfu-dload-message.h qfu-dload-message.c \
	qfu-qdl-message.h qfu-qdl-message.c \
	qfu-qdl-device.h qfu-qdl-device.c \
	qfu-reseter.h qfu-reseter.c \
	qfu-at-device.h qfu-at-device.c \
	$(NULL)

qmi_firmware_update_LDADD = \
	$(MBIM_LIBS) \
	$(GUDEV_LIBS) \
	$(GLIB_LIBS) \
	$(builddir)/libutils.la \
	$(NULL)

# Request to build enum types before anything else
BUILT_SOURCES = $(ENUMS_GENERATED)
CLEANFILES    = $(ENUMS_GENERATED)
