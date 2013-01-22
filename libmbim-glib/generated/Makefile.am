noinst_LTLIBRARIES = libmbim-glib-generated.la

GENERATED_H = \
	mbim-error-types.h \
	mbim-enum-types.h

GENERATED_C = \
	mbim-error-types.c \
	mbim-error-quarks.c \
	mbim-enum-types.c

# Error types
mbim-error-types.h: $(top_srcdir)/libmbim-glib/mbim-errors.h $(top_srcdir)/build-aux/templates/mbim-error-types-template.h
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#ifndef __LIBMBIM_GLIB_ERROR_TYPES_H__\n#define __LIBMBIM_GLIB_ERROR_TYPES_H__\n#include \"mbim-errors.h\"\n" \
		--template $(top_srcdir)/build-aux/templates/mbim-error-types-template.h \
		--ftail "#endif /* __LIBMBIM_GLIB_ERROR_TYPES_H__ */\n" \
		$(top_srcdir)/libmbim-glib/mbim-errors.h > $@

mbim-error-types.c: $(top_srcdir)/libmbim-glib/mbim-errors.h mbim-error-types.h $(top_srcdir)/build-aux/templates/mbim-error-types-template.c
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#include \"mbim-errors.h\"\n#include \"mbim-error-types.h\"\n" \
		--template $(top_srcdir)/build-aux/templates/mbim-error-types-template.c \
		$(top_srcdir)/libmbim-glib/mbim-errors.h > $@

mbim-error-quarks.c: $(top_srcdir)/libmbim-glib/mbim-errors.h mbim-error-types.h $(top_srcdir)/build-aux/templates/mbim-error-quarks-template.c
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#include \"mbim-errors.h\"\n#include \"mbim-error-types.h\"\n" \
		--template $(top_srcdir)/build-aux/templates/mbim-error-quarks-template.c \
		$(top_srcdir)/libmbim-glib/mbim-errors.h > $@

# Enum/Flag types
ENUMS = \
	$(top_srcdir)/libmbim-glib/mbim-uuid.h \
	$(top_srcdir)/libmbim-glib/mbim-cid.h \
	$(top_srcdir)/libmbim-glib/mbim-message.h \
	$(top_srcdir)/libmbim-glib/mbim-message-basic-connect.h
mbim-enum-types.h:  $(ENUMS) $(top_srcdir)/build-aux/templates/mbim-enum-types-template.h
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#ifndef __LIBMBIM_GLIB_ENUM_TYPES_H__\n#define __LIBMBIM_GLIB_ENUM_TYPES_H__\n#include \"mbim-uuid.h\"\n#include \"mbim-cid.h\"\n#include \"mbim-message.h\"\n#include \"mbim-message-basic-connect.h\"\n" \
		--template $(top_srcdir)/build-aux/templates/mbim-enum-types-template.h \
		--ftail "#endif /* __LIBMBIM_GLIB_ENUM_TYPES_H__ */\n" \
		$(ENUMS) > $@

mbim-enum-types.c: $(ENUMS) mbim-enum-types.h $(top_srcdir)/build-aux/templates/mbim-enum-types-template.c
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#include \"mbim-enum-types.h\"\n" \
		--template $(top_srcdir)/build-aux/templates/mbim-enum-types-template.c \
		$(ENUMS) > $@

BUILT_SOURCES = $(GENERATED_H) $(GENERATED_C)

nodist_libmbim_glib_generated_la_SOURCES = \
	$(GENERATED_H) \
	$(GENERATED_C)

libmbim_glib_generated_la_CPPFLAGS = \
	$(LIBMBIM_GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libmbim-glib \
	-DLIBMBIM_GLIB_COMPILATION \
	-DG_LOG_DOMAIN=\"Mbim\" \
	-Wno-unused-function

libmbim_glib_generated_la_LIBADD = \
	$(LIBMBIM_GLIB_LIBS)

includedir = @includedir@/libmbim-glib
nodist_include_HEADERS = \
	mbim-error-types.h \
	mbim-enum-types.h

CLEANFILES = $(GENERATED_H) $(GENERATED_C)
