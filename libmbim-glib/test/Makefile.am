noinst_PROGRAMS = \
	test-uuid

test_uuid_SOURCES = \
	test-uuid.c
test_uuid_CPPFLAGS = \
	$(LIBMBIM_GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libmbim-glib \
	-DLIBMBIM_GLIB_COMPILATION
test_uuid_LDADD = \
	$(top_builddir)/libmbim-glib/libmbim-glib.la \
	$(LIBMBIM_GLIB_LIBS)

if WITH_TESTS

check-local: test-uuid
	$(abs_builddir)/test-uuid

endif
