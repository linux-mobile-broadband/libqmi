noinst_PROGRAMS = \
	test-uuid \
	test-message \
	test-fragment

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

test_message_SOURCES = \
	test-message.c
test_message_CPPFLAGS = \
	$(LIBMBIM_GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libmbim-glib \
	-DLIBMBIM_GLIB_COMPILATION
test_message_LDADD = \
	$(top_builddir)/libmbim-glib/libmbim-glib.la \
	$(LIBMBIM_GLIB_LIBS)

test_fragment_SOURCES = \
	test-fragment.c
test_fragment_CPPFLAGS = \
	$(LIBMBIM_GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libmbim-glib \
	-DLIBMBIM_GLIB_COMPILATION
test_fragment_LDADD = \
	$(top_builddir)/libmbim-glib/libmbim-glib.la \
	$(LIBMBIM_GLIB_LIBS)

if WITH_TESTS

check-local: test-uuid test-message test-fragment
	$(abs_builddir)/test-uuid
	$(abs_builddir)/test-message
	$(abs_builddir)/test-fragment

endif
