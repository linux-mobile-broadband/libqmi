# The name of the module.
DOC_MODULE = libqmi-glib

# The top-level SGML file.
DOC_MAIN_SGML_FILE = $(DOC_MODULE)-docs.xml

# Extra options to supply to gtkdoc-scan
SCAN_OPTIONS =

# The directory containing the source code.
DOC_SOURCE_DIR = $(top_srcdir)/libqmi-glib

HFILE_GLOB =
CFILE_GLOB =

# Headers to ignore
IGNORE_HFILES = \
	$(NULL)

# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.
INCLUDES = \
	-I$(srcdir) \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(LIBQMI_GLIB_CFLAGS) \
	$(NULL)

GTKDOC_LIBS = \
	$(LIBQMI_GLIB_LIBS) \
	$(top_builddir)/libqmi-glib/libqmi-glib.la \
	$(NULL)

# Extra options to supply to gtkdoc-mkdb
MKDB_OPTIONS = --output-format=xml --sgml-mode --name-space=qmi

# Images to copy into HTML directory
HTML_IMAGES = \
	$(NULL)

#  Note that PNG files are also added in content_files so that
#  the documentation is not built before the PNGs.
content_files = \
	$(HTML_IMAGES) \
	$(NULL)

expand_content_files = \
	$(NULL)

extra_files = \
	$(NULL)

if ENABLE_GTK_DOC
include $(top_srcdir)/gtk-doc.make
else
EXTRA_DIST =
CLEANFILES =
endif

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
	$(DOC_MODULE)-sections.txt \
	$(DOC_MODULE).args \
	$(DOC_MODULE).hierarchy \
	$(DOC_MODULE).interfaces \
	$(DOC_MODULE).prerequisites \
	$(DOC_MODULE).signals \
	$(DOC_MODULE).types \
	*.stamp \
	-rf xml html tmpl \
	$(NULL)
