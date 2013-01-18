noinst_LTLIBRARIES = libmbim-glib-generated.la

GENERATED_H = \
	mbim-error-types.h

GENERATED_C = \
	mbim-error-types.c \
	mbim-error-quarks.c

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
	mbim-error-types.h

CLEANFILES = $(GENERATED_H) $(GENERATED_C)
