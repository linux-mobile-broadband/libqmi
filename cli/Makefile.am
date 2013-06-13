
bin_PROGRAMS = mbimcli

mbimcli_CPPFLAGS = \
	$(MBIMCLI_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libmbim-glib \
	-I$(top_builddir)/libmbim-glib \
	-I$(top_srcdir)/libmbim-glib/generated \
	-I$(top_builddir)/libmbim-glib/generated

mbimcli_SOURCES = \
	mbimcli.h \
	mbimcli.c \
	mbimcli-basic-connect.c

mbimcli_LDADD = \
	$(MBIMCLI_LIBS) \
	$(top_builddir)/libmbim-glib/libmbim-glib.la
