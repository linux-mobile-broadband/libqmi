
# ship under libqmi-glib for now
includedir = @includedir@/libqmi-glib

if QMI_QRTR_SUPPORTED

noinst_LTLIBRARIES = libqrtr-glib.la

libqrtr_glib_la_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-I$(top_srcdir)/src/libqrtr-glib \
	-I$(top_builddir)/src/libqrtr-glib \
	-DLIBQRTR_GLIB_COMPILATION \
	-DLIBEXEC_PATH=\""$(libexecdir)"\" \
	-DG_LOG_DOMAIN=\"Qrtr\" \
	$(NULL)

libqrtr_glib_la_SOURCES = \
	libqrtr-glib.h \
	qrtr-bus.h qrtr-bus.c \
	qrtr-node.h qrtr-node.c \
	qrtr-utils.h qrtr-utils.c \
	$(NULL)

libqrtr_glib_la_LDFLAGS = \
	$(WARN_CFLAGS) \
	$(GLIB_LIBS) \
	$(NULL)

include_HEADERS = \
	libqrtr-glib.h \
	qrtr-bus.h \
	qrtr-node.h \
	qrtr-utils.h \
	$(NULL)

# Introspection

if HAVE_INTROSPECTION

INTROSPECTION_GIRS = Qrtr-1.0.gir
INTROSPECTION_SCANNER_ARGS = --warn-all
INTROSPECTION_COMPILER_ARGS =

Qrtr-1.0.gir: libqrtr-glib.la
Qrtr_1_0_gir_INCLUDES = GLib-2.0 GObject-2.0 Gio-2.0
Qrtr_1_0_gir_CFLAGS = $(libqrtr_glib_la_CPPFLAGS)
Qrtr_1_0_gir_LIBS = libqrtr-glib.la
Qrtr_1_0_gir_EXPORT_PACKAGES = libqrtr-glib
Qrtr_1_0_gir_SCANNERFLAGS = \
	$(WARN_SCANNERFLAGS) \
	--c-include "libqrtr-glib.h" \
	--identifier-prefix=QRTR \
	--identifier-prefix=Qrtr \
	--symbol-prefix=qrtr_ \
	$(NULL)
Qrtr_1_0_gir_FILES = \
	$(include_HEADERS) \
	$(filter-out %.h,$(libqrtr_glib_la_SOURCES)) \
	$(filter %.c,$(libqrtr_glib_la_SOURCES)) \
	$(NULL)

girdir = $(datadir)/gir-1.0
nodist_gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
nodist_typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES = $(nodist_gir_DATA) $(nodist_typelib_DATA)

endif # HAVE_INTROSPECTION

-include $(INTROSPECTION_MAKEFILE)

endif
