
SUBDIRS = generated . test

noinst_LTLIBRARIES = libqmi-glib-compat.la

libqmi_glib_compat_la_SOURCES = \
	qmi-compat.h qmi-compat.c \
	$(NULL)

libqmi_glib_compat_la_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(QRTR_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/libqmi-glib \
	-I$(top_srcdir)/src/libqmi-glib/generated \
	-I$(top_builddir)/src/libqmi-glib \
	-I$(top_builddir)/src/libqmi-glib/generated \
	-DLIBQMI_GLIB_COMPILATION \
	-DG_LOG_DOMAIN=\"Qmi\" \
	-Wno-deprecated-declarations \
	$(NULL)

libqmi_glib_compat_la_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(GLIB_LIBS) \
	$(QRTR_LIBS) \
	$(NULL)

lib_LTLIBRARIES = libqmi-glib.la

libqmi_glib_la_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(MBIM_CFLAGS) \
	$(QRTR_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-I$(top_srcdir)/src/libqmi-glib \
	-I$(top_srcdir)/src/libqmi-glib/generated \
	-I$(top_builddir)/src/libqmi-glib \
	-I$(top_builddir)/src/libqmi-glib/generated \
	-DLIBQMI_GLIB_COMPILATION \
	-DLIBEXEC_PATH=\""$(libexecdir)"\" \
	-DG_LOG_DOMAIN=\"Qmi\" \
	$(NULL)

libqmi_glib_la_SOURCES = \
	libqmi-glib.h \
	qmi-errors.h \
	qmi-enums-wds.h qmi-flags64-wds.h qmi-enums-wds.c \
	qmi-enums-dms.h qmi-flags64-dms.h qmi-enums-dms.c \
	qmi-enums-nas.h qmi-flags64-nas.h qmi-enums-nas.c \
	qmi-enums-wms.h \
	qmi-enums-pds.h \
	qmi-enums-pdc.h \
	qmi-enums-pbm.h \
	qmi-enums-uim.h \
	qmi-enums-sar.h \
	qmi-enums-oma.h \
	qmi-enums-wda.h \
	qmi-enums-voice.h \
	qmi-enums-loc.h qmi-flags64-loc.h \
	qmi-enums-qos.h \
	qmi-enums-gas.h \
	qmi-enums-dsd.h qmi-flags64-dsd.h\
	qmi-enums.h qmi-enums-private.h \
	qmi-utils.h qmi-utils.c \
	qmi-helpers.h qmi-helpers.c \
	qmi-message.h qmi-message.c \
	qmi-message-context.h qmi-message-context.c \
	qmi-device.h qmi-device.c \
	qmi-client.h qmi-client.c \
	qmi-proxy.h qmi-proxy.c \
	qmi-file.h qmi-file.c \
	qmi-endpoint.h qmi-endpoint.c \
	qmi-endpoint-qmux.h qmi-endpoint-qmux.c \
	qmi-net-port-manager.h qmi-net-port-manager.c \
	qmi-net-port-manager-qmiwwan.h qmi-net-port-manager-qmiwwan.c \
	$(NULL)

nodist_libqmi_glib_la_SOURCES = \
	qmi-version.h \
	$(NULL)

libqmi_glib_la_LIBADD = \
	${top_builddir}/src/libqmi-glib/generated/libqmi-glib-generated.la \
	libqmi-glib-compat.la \
	$(NULL)

if QMI_MBIM_QMUX_SUPPORTED
libqmi_glib_la_SOURCES += \
	qmi-endpoint-mbim.h qmi-endpoint-mbim.c
endif

if QMI_QRTR_SUPPORTED
libqmi_glib_la_SOURCES += \
	qmi-endpoint-qrtr.h qmi-endpoint-qrtr.c \
	$(NULL)
endif

if RMNET_SUPPORT_ENABLED
libqmi_glib_la_SOURCES += \
	qmi-net-port-manager-rmnet.h qmi-net-port-manager-rmnet.c \
	$(NULL)
endif

libqmi_glib_la_LDFLAGS = \
	-version-info $(QMI_GLIB_LT_CURRENT):$(QMI_GLIB_LT_REVISION):$(QMI_GLIB_LT_AGE) \
	$(WARN_CFLAGS) \
	$(GLIB_LIBS) \
	$(MBIM_LIBS) \
	$(QRTR_LIBS) \
	$(NULL)

includedir = @includedir@/libqmi-glib
include_HEADERS = \
	libqmi-glib.h \
	qmi-errors.h \
	qmi-enums.h \
	qmi-enums-private.h \
	qmi-compat.h \
	qmi-enums-dms.h qmi-flags64-dms.h \
	qmi-enums-wds.h qmi-flags64-wds.h \
	qmi-enums-nas.h qmi-flags64-nas.h \
	qmi-enums-wms.h \
	qmi-enums-pds.h \
	qmi-enums-pdc.h \
	qmi-enums-pbm.h \
	qmi-enums-uim.h \
	qmi-enums-oma.h \
	qmi-enums-wda.h \
	qmi-enums-voice.h \
	qmi-enums-loc.h qmi-flags64-loc.h \
	qmi-enums-qos.h \
	qmi-enums-gas.h \
	qmi-enums-dsd.h qmi-flags64-dsd.h \
	qmi-enums-sar.h \
	qmi-utils.h \
	qmi-message.h \
	qmi-message-context.h \
	qmi-device.h \
	qmi-client.h \
	qmi-proxy.h

nodist_include_HEADERS = \
	qmi-version.h \
	$(NULL)

EXTRA_DIST = \
	qmi-version.h.in \
	$(NULL)

DISTCLEANFILES = \
	qmi-version.h \
	$(NULL)

CLEANFILES =

# Introspection

if HAVE_INTROSPECTION

INTROSPECTION_GIRS = Qmi-1.0.gir
INTROSPECTION_SCANNER_ARGS = --warn-all
INTROSPECTION_COMPILER_ARGS =

Qmi-1.0.gir: libqmi-glib.la
Qmi_1_0_gir_INCLUDES = GLib-2.0 GObject-2.0 Gio-2.0
if QMI_QRTR_SUPPORTED
Qmi_1_0_gir_INCLUDES += Qrtr-1.0
endif
Qmi_1_0_gir_CFLAGS = $(libqmi_glib_la_CPPFLAGS)
Qmi_1_0_gir_LIBS = libqmi-glib.la
Qmi_1_0_gir_EXPORT_PACKAGES = qmi-glib
Qmi_1_0_gir_SCANNERFLAGS = \
	$(WARN_SCANNERFLAGS) \
	--c-include "libqmi-glib.h" \
	--identifier-prefix=QMI \
	--identifier-prefix=Qmi \
	--symbol-prefix=qmi_ \
	$(NULL)
Qmi_1_0_gir_FILES = \
	$(filter-out qmi-compat.h,$(include_HEADERS)) \
	$(filter-out %.h,$(libqmi_glib_la_SOURCES)) \
	$(filter %.c,$(libqmi_glib_la_SOURCES)) \
	$(wildcard generated/*.h) \
	$(wildcard generated/*.c) \
	$(NULL)

girdir = $(datadir)/gir-1.0
nodist_gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
nodist_typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(nodist_gir_DATA) $(nodist_typelib_DATA)

endif # HAVE_INTROSPECTION

-include $(INTROSPECTION_MAKEFILE)
