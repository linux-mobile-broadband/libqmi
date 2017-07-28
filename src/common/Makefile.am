SUBDIRS = . test

# common library, built as a noinst
noinst_LTLIBRARIES = libmbim-common.la

libmbim_common_la_CPPFLAGS = \
	$(MBIM_COMMON_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-I$(top_srcdir)/src/common
libmbim_common_la_SOURCES = \
	mbim-common.h \
	mbim-common.c
libmbim_common_la_LIBADD = \
	$(MBIM_COMMON_LIBS)
