
SUBDIRS = . test

lib_LTLIBRARIES = libqmi-glib.la

libqmi_glib_la_CPPFLAGS = \
	$(LIBQMI_GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-I$(top_srcdir)/libqmi-glib \
	-I$(top_builddir)/libqmi-glib

# Error types
qmi-error-types.h: qmi-errors.h $(top_srcdir)/build-aux/templates/qmi-error-types-template.h
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#ifndef __LIBQMI_GLIB_ERROR_TYPES_H__\n#define __LIBQMI_GLIB_ERROR_TYPES_H__\n#include \"qmi-errors.h\"\n" \
		--template $(top_srcdir)/build-aux/templates/qmi-error-types-template.h \
		--ftail "#endif /* __LIBQMI_GLIB_ERROR_TYPES_H__ */\n" \
		qmi-errors.h > $@

qmi-error-types.c: qmi-errors.h qmi-error-types.h $(top_srcdir)/build-aux/templates/qmi-error-types-template.c
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#include \"qmi-errors.h\"\n#include \"qmi-error-types.h\"\n" \
		--template $(top_srcdir)/build-aux/templates/qmi-error-types-template.c \
		qmi-errors.h > $@

qmi-error-quarks.c: qmi-errors.h qmi-error-types.h $(top_srcdir)/build-aux/templates/qmi-error-quarks-template.c
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#include \"qmi-errors.h\"\n#include \"qmi-error-types.h\"\n" \
		--template $(top_srcdir)/build-aux/templates/qmi-error-quarks-template.c \
		qmi-errors.h > $@

# Enum/Flag types
ENUMS = qmi-enums.h qmi-enums-wds.h qmi-enums-dms.h qmi-enums-nas.h
qmi-enum-types.h:  $(ENUMS) $(top_srcdir)/build-aux/templates/qmi-enum-types-template.h
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#ifndef __LIBQMI_GLIB_ENUM_TYPES_H__\n#define __LIBQMI_GLIB_ENUM_TYPES_H__\n#include \"qmi-enums.h\"\n#include \"qmi-enums-wds.h\"\n#include \"qmi-enums-dms.h\"\n#include \"qmi-enums-nas.h\"\n" \
		--template $(top_srcdir)/build-aux/templates/qmi-enum-types-template.h \
		--ftail "#endif /* __LIBQMI_GLIB_ENUM_TYPES_H__ */\n" \
		$(ENUMS) > $@

qmi-enum-types.c: $(ENUMS) qmi-enum-types.h $(top_srcdir)/build-aux/templates/qmi-enum-types-template.c
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#include \"qmi-enum-types.h\"\n" \
		--template $(top_srcdir)/build-aux/templates/qmi-enum-types-template.c \
		$(ENUMS) > $@

# 64bit flag types
FLAGS64 = qmi-flags64-dms.h qmi-flags64-nas.h
qmi-flags64-types.h:  $(FLAGS64) $(top_srcdir)/build-aux/templates/qmi-flags64-types-template.h
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#ifndef __LIBQMI_GLIB_FLAGS64_TYPES_H__\n#define __LIBQMI_GLIB_FLAGS64_TYPES_H__\n#include \"qmi-flags64-dms.h\"\n#include \"qmi-flags64-nas.h\"\n" \
		--template $(top_srcdir)/build-aux/templates/qmi-flags64-types-template.h \
		--ftail "#endif /* __LIBQMI_GLIB_FLAGS64_TYPES_H__ */\n" \
		$(FLAGS64) > $@

qmi-flags64-types.c: $(FLAGS64) qmi-flags64-types.h $(top_srcdir)/build-aux/templates/qmi-flags64-types-template.c
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#include \"qmi-flags64-types.h\"\n" \
		--template $(top_srcdir)/build-aux/templates/qmi-flags64-types-template.c \
		$(FLAGS64) > $@

# CTL service
qmi-ctl.stamp: $(top_srcdir)/data/qmi-service-ctl.json $(top_srcdir)/build-aux/qmi-codegen/*.py $(top_srcdir)/build-aux/qmi-codegen/qmi-codegen
	$(AM_V_GEN)  \
		rm -f qmi-ctl.h && \
		rm -f qmi-ctl.c && \
		$(top_srcdir)/build-aux/qmi-codegen/qmi-codegen \
			--input $(top_srcdir)/data/qmi-service-ctl.json \
			--include $(top_srcdir)/data/qmi-common.json \
			--output qmi-ctl && \
		touch $@

# DMS service
qmi-dms.stamp: $(top_srcdir)/data/qmi-service-dms.json $(top_srcdir)/build-aux/qmi-codegen/*.py $(top_srcdir)/build-aux/qmi-codegen/qmi-codegen
	$(AM_V_GEN) \
		rm -f qmi-dms.h && \
		rm -f qmi-dms.c && \
		 $(top_srcdir)/build-aux/qmi-codegen/qmi-codegen \
			--input $(top_srcdir)/data/qmi-service-dms.json \
			--include $(top_srcdir)/data/qmi-common.json \
			--output qmi-dms && \
		touch $@

# WDS service
qmi-wds.stamp: $(top_srcdir)/data/qmi-service-wds.json $(top_srcdir)/build-aux/qmi-codegen/*.py $(top_srcdir)/build-aux/qmi-codegen/qmi-codegen
	$(AM_V_GEN) \
		rm -f qmi-wds.h && \
		rm -f qmi-wds.c && \
		$(top_srcdir)/build-aux/qmi-codegen/qmi-codegen \
			--input $(top_srcdir)/data/qmi-service-wds.json \
			--include $(top_srcdir)/data/qmi-common.json \
			--output qmi-wds && \
		touch $@

# NAS service
qmi-nas.stamp: $(top_srcdir)/data/qmi-service-nas.json $(top_srcdir)/build-aux/qmi-codegen/*.py $(top_srcdir)/build-aux/qmi-codegen/qmi-codegen
	$(AM_V_GEN) \
		rm -f qmi-nas.h && \
		rm -f qmi-nas.c && \
		$(top_srcdir)/build-aux/qmi-codegen/qmi-codegen \
			--input $(top_srcdir)/data/qmi-service-nas.json \
			--include $(top_srcdir)/data/qmi-common.json \
			--output qmi-nas && \
		touch $@

# Additional dependencies
qmi-device.c: qmi-error-types.h qmi-enum-types.h qmi-ctl.h qmi-dms.h qmi-wds.h qmi-nas.h
qmi-client.c: qmi-error-types.h qmi-enum-types.h
qmi-message.c: qmi-error-types.h qmi-enum-types.h qmi-ctl.h qmi-dms.h qmi-wds.h qmi-nas.h
qmi-ctl.h: qmi-ctl.stamp
qmi-ctl.c: qmi-error-types.h qmi-enum-types.h qmi-ctl.h
qmi-dms.h: qmi-dms.stamp qmi-enums-dms.h
qmi-dms.c: qmi-error-types.h qmi-enum-types.h qmi-flags64-types.h qmi-dms.h
qmi-wds.h: qmi-wds.stamp qmi-enums-wds.h
qmi-wds.c: qmi-error-types.h qmi-enum-types.h qmi-wds.h
qmi-nas.h: qmi-nas.stamp qmi-enums-nas.h
qmi-nas.c: qmi-error-types.h qmi-enum-types.h qmi-flags64-types.h qmi-nas.h

libqmi_glib_la_SOURCES = \
	libqmi-glib.h \
	qmi-errors.h qmi-error-types.h qmi-error-types.c qmi-error-quarks.c \
	qmi-enums-wds.h \
	qmi-enums-dms.h \
	qmi-enums-nas.h \
	qmi-enums.h qmi-enum-types.h qmi-enum-types.c qmi-flags64-types.h qmi-flags64-types.c \
	qmi-utils.h qmi-utils.c \
	qmi-message.h qmi-message.c \
	qmi-device.h qmi-device.c \
	qmi-client.h qmi-client.c

nodist_libqmi_glib_la_SOURCES = \
	qmi-ctl.c qmi-ctl.h \
	qmi-dms.c qmi-dms.h \
	qmi-wds.c qmi-wds.h \
	qmi-nas.c qmi-nas.h

libqmi_glib_la_LIBADD = \
	$(LIBQMI_GLIB_LIBS)

includedir = @includedir@/libqmi-glib
include_HEADERS = \
	libqmi-glib.h \
	qmi-errors.h qmi-error-types.h \
	qmi-enums.h qmi-enum-types.h qmi-flags64-types.h \
	qmi-message.h \
	qmi-device.h \
	qmi-client.h \
	qmi-ctl.h \
	qmi-enums-dms.h qmi-flags64-dms.h qmi-dms.h \
	qmi-enums-wds.h qmi-wds.h \
	qmi-enums-nas.h qmi-flags64-nas.h qmi-nas.h

CLEANFILES = \
	qmi-ctl.h qmi-ctl.c qmi-ctl.stamp \
	qmi-dms.h qmi-dms.c qmi-dms.stamp \
	qmi-wds.h qmi-wds.c qmi-wds.stamp \
	qmi-nas.h qmi-nas.c qmi-nas.stamp
