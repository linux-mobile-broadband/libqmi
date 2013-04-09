noinst_PROGRAMS = \
	test-uuid \
	test-cid \
	test-message \
	test-fragment \
	test-message-contents \
	test-message-command-builder

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

test_cid_SOURCES = \
	test-cid.c
test_cid_CPPFLAGS = \
	$(LIBMBIM_GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libmbim-glib \
	-DLIBMBIM_GLIB_COMPILATION
test_cid_LDADD = \
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

test_message_contents_SOURCES = \
	test-message-contents.c
test_message_contents_CPPFLAGS = \
	$(LIBMBIM_GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libmbim-glib \
	-I$(top_builddir)/libmbim-glib/generated \
	-DLIBMBIM_GLIB_COMPILATION
test_message_contents_LDADD = \
	$(top_builddir)/libmbim-glib/libmbim-glib.la \
	$(LIBMBIM_GLIB_LIBS)

test_message_command_builder_SOURCES = \
	test-message-command-builder.c
test_message_command_builder_CPPFLAGS = \
	$(LIBMBIM_GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libmbim-glib \
	-I$(top_builddir)/libmbim-glib/generated \
	-DLIBMBIM_GLIB_COMPILATION
test_message_command_builder_LDADD = \
	$(top_builddir)/libmbim-glib/libmbim-glib.la \
	$(LIBMBIM_GLIB_LIBS)


if WITH_TESTS

check-local: test-uuid test-cid test-message test-fragment test-message-contents test-message-command-builder
	$(abs_builddir)/test-uuid
	$(abs_builddir)/test-cid
	$(abs_builddir)/test-message
	$(abs_builddir)/test-fragment
	$(abs_builddir)/test-message-contents
	$(abs_builddir)/test-message-command-builder

endif
