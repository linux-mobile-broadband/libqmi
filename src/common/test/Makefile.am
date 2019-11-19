include $(top_srcdir)/gtester.make

noinst_PROGRAMS = \
	test-common \
	$(NULL)

test_common_SOURCES = \
	test-common.c \
	$(NULL)

test_common_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(MBIM_COMMON_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-I$(top_srcdir)/src/common \
	$(NULL)

test_common_LDADD = \
	$(top_builddir)/src/common/libmbim-common.la \
	$(MBIM_COMMON_LIBS) \
	$(NULL)

test_common_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(MBIM_COMMON_LIBS) \
	$(NULL)

TEST_PROGS += $(noinst_PROGRAMS)
