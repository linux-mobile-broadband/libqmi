
lib_LTLIBRARIES = libqmi-glib.la

libqmi_glib_la_CPPFLAGS = \
	$(LIBQMI_GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src

# Error types
qmi-error-types.h: qmi-errors.h $(top_srcdir)/build-aux/qmi-error-types-template.h
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#ifndef __LIBQMI_GLIB_ERROR_TYPES_H__\n#define __LIBQMI_GLIB_ERROR_TYPES_H__\n#include \"qmi-errors.h\"\n" \
		--template $(top_srcdir)/build-aux/qmi-error-types-template.h \
		--ftail "#endif /* __LIBQMI_GLIB_ERROR_TYPES_H__ */\n" \
		qmi-errors.h > $@

qmi-error-types.c: qmi-errors.h qmi-error-types.h $(top_srcdir)/build-aux/qmi-error-types-template.c
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#include \"qmi-errors.h\"\n#include \"qmi-error-types.h\"\n" \
		--template $(top_srcdir)/build-aux/qmi-error-types-template.c \
		qmi-errors.h > $@

# Enum types
ENUMS = qmi-enums.h qmi-dms.h qmi-ctl.h
qmi-enum-types.h:  $(ENUMS) $(top_srcdir)/build-aux/qmi-enum-types-template.h
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#ifndef __LIBQMI_GLIB_ENUM_TYPES_H__\n#define __LIBQMI_GLIB_ENUM_TYPES_H__\n#include \"qmi-enums.h\"\n#include \"qmi-dms.h\"\n#include \"qmi-ctl.h\"\n" \
		--template $(top_srcdir)/build-aux/qmi-enum-types-template.h \
		--ftail "#endif /* __LIBQMI_GLIB_ENUM_TYPES_H__ */\n" \
		$(ENUMS) > $@

qmi-enum-types.c: $(ENUMS) qmi-enum-types.h $(top_srcdir)/build-aux/qmi-enum-types-template.c
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#include \"qmi-enum-types.h\"\n" \
		--template $(top_srcdir)/build-aux/qmi-enum-types-template.c \
		$(ENUMS) > $@

# Additional dependencies
qmi-device.c: qmi-error-types.h
qmi-client.c: qmi-error-types.h qmi-enum-types.h
qmi-client-ctl.c: qmi-error-types.h qmi-enum-types.h
qmi-message.c: qmi-error-types.h qmi-enum-types.h
qmi-message-ctl.c: qmi-error-types.h
qmi-message-dms.c: qmi-error-types.h

libqmi_glib_la_SOURCES = \
	libqmi-glib.h \
	qmi-errors.h qmi-error-types.h qmi-error-types.c \
	qmi-enums.h qmi-enum-types.h qmi-enum-types.c \
	qmi-utils.h qmi-utils.c \
	qmi-message.h qmi-message.c \
	qmi-message-ctl.h qmi-message-ctl.c \
	qmi-message-dms.h qmi-message-dms.c \
	qmi-device.h qmi-device.c \
	qmi-client.h qmi-client.c \
	qmi-client-ctl.h qmi-client-ctl.c \
	qmi-client-dms.h qmi-client-dms.c

libqmi_glib_la_LIBADD = \
	$(LIBQMI_GLIB_LIBS)

includedir = @includedir@/libqmi-glib
include_HEADERS = \
	libqmi-glib.h \
	qmi-errors.h qmi-error-types.h \
	qmi-enums.h qmi-enum-types.h \
	qmi-device.h \
	qmi-client.h \
	qmi-ctl.h qmi-client-ctl.h \
	qmi-dms.h qmi-client-dms.h
