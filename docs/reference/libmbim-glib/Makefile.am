
# The name of the module.
DOC_MODULE = libmbim-glib

# The top-level SGML file.
DOC_MAIN_SGML_FILE = $(DOC_MODULE)-docs.xml

BUILT_SOURCES =

# Extra options to supply to gtkdoc-scan
SCAN_OPTIONS = --rebuild-types

# The directory containing the source code.
DOC_SOURCE_DIR = \
	$(top_srcdir)/libmbim-glib \
	$(top_builddir)/libmbim-glib/generated

# Used for dependencies
HFILE_GLOB = \
	$(top_srcdir)/libmbim-glib/*.h \
	$(top_builddir)/libmbim-glib/generated/*.h
CFILE_GLOB = \
	$(top_srcdir)/libmbim-glib/*.c \
	$(top_builddir)/libmbim-glib/generated/*.c

# Headers to ignore
IGNORE_HFILES = \
	mbim-message-private.h

# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.
AM_CPPFLAGS = \
	-I$(srcdir) \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(LIBMBIM_GLIB_CFLAGS)

GTKDOC_LIBS = \
	$(LIBMBIM_GLIB_LIBS) \
	$(top_builddir)/libmbim-glib/libmbim-glib.la

# Extra options to supply to gtkdoc-mkdb
MKDB_OPTIONS = --output-format=xml --sgml-mode --name-space=mbim

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE)
content_files = version.xml

expand_content_files =

extra_files =

include $(top_srcdir)/gtk-doc.make


EXTRA_DIST += \
	version.xml.in \
	$(DIAGRAMS) \
	$(NULL)

CLEANFILES += \
	$(DOC_MODULE)-decl-list.txt \
	$(DOC_MODULE)-decl.txt \
	$(DOC_MODULE)-overrides.txt \
	$(DOC_MODULE)-undeclared.txt \
	$(DOC_MODULE)-undocumented.txt \
	$(DOC_MODULE)-overrides.txt \
	$(DOC_MODULE)-unused.txt \
	$(DOC_MODULE).args \
	$(DOC_MODULE).hierarchy \
	$(DOC_MODULE).interfaces \
	$(DOC_MODULE).prerequisites \
	$(DOC_MODULE).signals \
	$(DOC_MODULE).types \
	*.stamp \
	-rf xml html tmpl \
	$(NULL)
