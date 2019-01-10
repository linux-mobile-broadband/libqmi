
SUBDIRS = . test

noinst_LTLIBRARIES = libutils.la libfirehose.la

libutils_la_SOURCES = \
	qfu-utils.h qfu-utils.c \
	$(NULL)

libutils_la_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(QRTR_CFLAGS) \
	-I$(top_srcdir)/src/libqmi-glib \
	-I$(top_srcdir)/src/libqmi-glib/generated \
	-I$(top_builddir)/src/libqmi-glib \
	-I$(top_builddir)/src/libqmi-glib/generated \
	$(NULL)

libutils_la_LIBADD = \
	$(top_builddir)/src/libqmi-glib/libqmi-glib.la \
	$(NULL)

libutils_la_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(GLIB_LIBS) \
	$(NULL)

libfirehose_la_SOURCES = \
	qfu-firehose-message.h qfu-firehose-message.c \
	$(NULL)

libfirehose_la_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(NULL)

libfirehose_la_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(GLIB_LIBS) \
	$(NULL)

bin_PROGRAMS = qmi-firmware-update

qmi_firmware_update_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GUDEV_CFLAGS) \
	$(MBIM_CFLAGS) \
	$(QRTR_CFLAGS) \
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
	$(top_srcdir)/src/qmi-firmware-update/qfu-sahara-message.h \
	$(NULL)

ENUMS_GENERATED = \
	qfu-enum-types.h qfu-enum-types.c \
	$(NULL)

qfu-enum-types.h: Makefile.am $(ENUMS) $(top_srcdir)/build-aux/templates/qmi-enum-types.h.template
	$(AM_V_GEN) $(PYTHON) $(top_srcdir)/build-aux/qmi-mkenums \
		--fhead "#ifndef QFU_ENUM_TYPES_H\n#define QFU_ENUM_TYPES_H\n#include \"qfu-image.h\"\n#include \"qfu-qdl-message.h\"\n#include \"qfu-dload-message.h\"\n#include \"qfu-sahara-message.h\"\n" \
		--template $(top_srcdir)/build-aux/templates/qmi-enum-types.h.template \
		--ftail "#endif /* __QFUENUM_TYPES_H__ */\n" \
		$(ENUMS) > $@

qfu-enum-types.c: $(ENUMS) qfu-enum-types.h $(top_srcdir)/build-aux/templates/qmi-enum-types.c.template
	$(AM_V_GEN) $(PYTHON) $(top_srcdir)/build-aux/qmi-mkenums \
		--fhead "#include \"qfu-enum-types.h\"\n" \
		--template $(top_srcdir)/build-aux/templates/qmi-enum-types.c.template \
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
	qfu-sahara-message.h qfu-sahara-message.c \
	qfu-sahara-device.h qfu-sahara-device.c \
	qfu-reseter.h qfu-reseter.c \
	qfu-at-device.h qfu-at-device.c \
	$(NULL)

qmi_firmware_update_LDADD = \
	$(builddir)/libutils.la \
	$(builddir)/libfirehose.la \
	$(NULL)

qmi_firmware_update_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(MBIM_LIBS) \
	$(GUDEV_LIBS) \
	$(GLIB_LIBS) \
	$(NULL)

# Request to build enum types before anything else
BUILT_SOURCES = $(ENUMS_GENERATED)
CLEANFILES    = $(ENUMS_GENERATED)
