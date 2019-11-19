SUBDIRS = . test

# common library, built as a noinst
noinst_LTLIBRARIES = libmbim-common.la

libmbim_common_la_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(MBIM_COMMON_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-I$(top_srcdir)/src/common \
	$(NULL)

libmbim_common_la_SOURCES = \
	mbim-common.h \
	mbim-common.c \
	$(NULL)

libmbim_common_la_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(MBIM_COMMON_LIBS) \
	$(NULL)
