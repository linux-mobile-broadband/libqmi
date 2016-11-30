
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

ENUMS = \
	qfu-image.h \
	qfu-qdl-message.h \
	qfu-dload-message.h \
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
	qfu-operation.h \
	qfu-operation-download.c \
	qfu-operation-verify.c \
	qfu-updater.h qfu-updater.c \
	qfu-udev-helpers.h qfu-udev-helpers.c \
	qfu-image.h qfu-image.c \
	qfu-image-cwe.h qfu-image-cwe.c \
	qfu-image-factory.h qfu-image-factory.c \
	qfu-dload-message.h qfu-dload-message.c \
	qfu-qdl-message.h qfu-qdl-message.c \
	qfu-qdl-device.h qfu-qdl-device.c \
	qfu-utils.h qfu-utils.c \
	$(NULL)

qmi_firmware_update_LDADD = \
	$(GUDEV_LIBS) \
	$(GLIB_LIBS) \
	$(top_builddir)/src/libqmi-glib/libqmi-glib.la \
	$(NULL)

# Request to build enum types before anything else
BUILT_SOURCES = $(ENUMS_GENERATED)
CLEANFILES    = $(ENUMS_GENERATED)
