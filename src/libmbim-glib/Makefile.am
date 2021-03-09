
SUBDIRS = generated . test

# Core library, built as a noinst
noinst_LTLIBRARIES = libmbim-glib-core.la
libmbim_glib_core_la_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(LIBMBIM_GLIB_CFLAGS) \
	$(GUDEV_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/libmbim-glib \
	-I$(top_srcdir)/src/libmbim-glib/generated \
	-I$(top_builddir)/src/libmbim-glib \
	-I$(top_builddir)/src/libmbim-glib/generated \
	-DLIBMBIM_GLIB_COMPILATION \
	-DLIBEXEC_PATH=\""$(libexecdir)"\" \
	-DG_LOG_DOMAIN=\"Mbim\" \
	$(NULL)

libmbim_glib_core_la_SOURCES = \
	mbim-version.h \
	mbim-errors.h \
	mbim-enums.h \
	mbim-helpers.h mbim-helpers.c \
	mbim-utils.h mbim-utils.c \
	mbim-uuid.h mbim-uuid.c \
	mbim-cid.h mbim-cid.c \
	mbim-message-private.h mbim-message.h mbim-message.c \
	mbim-device.h mbim-device.c \
	mbim-compat.h mbim-compat.c \
	mbim-proxy.h mbim-proxy.c \
	mbim-proxy-helpers.h mbim-proxy-helpers.c \
	mbim-net-port-manager.h mbim-net-port-manager.c \
	$(NULL)

# Final installable library
lib_LTLIBRARIES = libmbim-glib.la

libmbim_glib_la_SOURCES = libmbim-glib.h

libmbim_glib_la_LIBADD = \
	libmbim-glib-core.la \
	${top_builddir}/src/common/libmbim-common.la \
	${top_builddir}/src/libmbim-glib/generated/libmbim-glib-generated.la \
	$(NULL)

libmbim_glib_la_LDFLAGS = \
	$(WARN_LDFLAGS) \
	-version-info $(MBIM_GLIB_LT_CURRENT):$(MBIM_GLIB_LT_REVISION):$(MBIM_GLIB_LT_AGE) \
	-export-symbols-regex '^mbim_.*' \
	$(LIBMBIM_GLIB_LIBS) \
	$(GUDEV_LIBS) \
	$(NULL)

includedir = @includedir@/libmbim-glib
include_HEADERS = \
	libmbim-glib.h \
	mbim-version.h \
	mbim-errors.h \
	mbim-enums.h \
	mbim-utils.h \
	mbim-uuid.h \
	mbim-cid.h \
	mbim-message.h \
	mbim-device.h \
	mbim-compat.h \
	mbim-proxy.h \
	$(NULL)

EXTRA_DIST = mbim-version.h.in

# Introspection

if HAVE_INTROSPECTION

INTROSPECTION_GIRS = Mbim-1.0.gir
INTROSPECTION_SCANNER_ARGS = --warn-all
INTROSPECTION_COMPILER_ARGS =

Mbim-1.0.gir: libmbim-glib.la
Mbim_1_0_gir_INCLUDES = GLib-2.0 GObject-2.0 Gio-2.0
Mbim_1_0_gir_CFLAGS = $(libmbim_glib_core_la_CPPFLAGS)
Mbim_1_0_gir_LIBS = libmbim-glib.la
Mbim_1_0_gir_EXPORT_PACKAGES = libmbim-glib
Mbim_1_0_gir_SCANNERFLAGS = \
	$(WARN_SCANNERFLAGS) \
	--c-include "libmbim-glib.h" \
	--identifier-prefix=MBIM \
	--identifier-prefix=Mbim \
	--symbol-prefix=mbim_ \
	$(NULL)
Mbim_1_0_gir_FILES = \
	$(include_HEADERS) \
	$(filter-out %.h,$(libmbim_glib_la_SOURCES)) \
	$(filter %.c,$(libmbim_glib_la_SOURCES)) \
	$(wildcard generated/*.h) \
	$(wildcard generated/*.c) \
	$(NULL)

girdir = $(datadir)/gir-1.0
nodist_gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
nodist_typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES = $(nodist_gir_DATA) $(nodist_typelib_DATA)

endif # HAVE_INTROSPECTION

-include $(INTROSPECTION_MAKEFILE)
