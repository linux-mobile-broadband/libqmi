include $(top_srcdir)/gtester.make

noinst_PROGRAMS = \
	test-common

TEST_PROGS += $(noinst_PROGRAMS)

test_common_SOURCES = \
	test-common.c
test_common_CPPFLAGS = \
	$(MBIM_COMMON_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-I$(top_srcdir)/src/common
test_common_LDADD = \
	$(top_builddir)/src/common/libmbim-common.la \
	$(MBIM_COMMON_LIBS)
