
if QMI_QRTR_SUPPORTED

# The name of the module.
DOC_MODULE = libqrtr-glib

# The top-level SGML file.
DOC_MAIN_SGML_FILE = $(DOC_MODULE)-docs.xml

# Extra options to supply to gtkdoc-scan
SCAN_OPTIONS = --rebuild-types --deprecated-guards="QRTR_DISABLE_DEPRECATED"

# The directory containing the source code.
DOC_SOURCE_DIR = \
	$(top_srcdir)/src/libqrtr-glib \
	$(top_builddir)/src/libqrtr-glib

# Used for dependencies
HFILE_GLOB = \
	$(top_srcdir)/src/libqrtr-glib/*.h \
	$(top_builddir)/src/libqrtr-glib/*.h
CFILE_GLOB = \
	$(top_srcdir)/src/libqrtr-glib/*.c

# Headers to ignore
IGNORE_HFILES =

# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.
AM_CPPFLAGS = \
	-I$(srcdir) \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(GLIB_CFLAGS)

GTKDOC_LIBS = \
	$(GLIB_LIBS) \
	$(top_builddir)/src/libqrtr-glib/libqrtr-glib.la

# Extra options to supply to gtkdoc-mkdb
MKDB_OPTIONS = --output-format=xml --sgml-mode --name-space=qrtr

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE)
content_files = version.xml

expand_content_files =

extra_files =

include $(top_srcdir)/gtk-doc.make

EXTRA_DIST += \
	$(DOC_MODULE)-sections.txt \
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
	$(DOC_MODULE).actions \
	$(DOC_MODULE).types \
	$(DOC_MODULE)-sections.txt \
	*.stamp \
	-rf xml html tmpl \
	$(NULL)

endif
