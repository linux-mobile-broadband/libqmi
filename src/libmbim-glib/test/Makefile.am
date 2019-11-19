include $(top_srcdir)/gtester.make

AM_CFLAGS = \
	$(WARN_CFLAGS) \
	$(LIBMBIM_GLIB_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/libmbim-glib \
	-I$(top_builddir)/src/libmbim-glib \
	-I$(top_builddir)/src/libmbim-glib/generated \
	-DLIBMBIM_GLIB_COMPILATION \
	$(NULL)

AM_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(LIBMBIM_GLIB_LIBS) \
	$(NULL)

noinst_PROGRAMS = \
	test-uuid \
	test-cid \
	test-message \
	test-fragment \
	test-message-parser \
	test-message-builder \
	test-proxy-helpers \
	$(NULL)

COMMON_LIBS_ADD =	\
	$(top_builddir)/src/common/libmbim-common.la \
	$(top_builddir)/src/libmbim-glib/libmbim-glib-core.la \
	$(top_builddir)/src/libmbim-glib/generated/libmbim-glib-generated.la \
	$(NULL)

test_uuid_SOURCES = test-uuid.c
test_uuid_LDADD = ${COMMON_LIBS_ADD}

test_cid_SOURCES = test-cid.c
test_cid_LDADD = ${COMMON_LIBS_ADD}

test_message_SOURCES = test-message.c
test_message_LDADD = ${COMMON_LIBS_ADD}

test_fragment_SOURCES = test-fragment.c
test_fragment_LDADD = $(COMMON_LIBS_ADD)

test_message_parser_SOURCES = test-message-parser.c
test_message_parser_LDADD = $(COMMON_LIBS_ADD)

test_message_builder_SOURCES = test-message-builder.c
test_message_builder_LDADD = $(COMMON_LIBS_ADD)

test_proxy_helpers_SOURCES = test-proxy-helpers.c
test_proxy_helpers_LDADD = $(COMMON_LIBS_ADD)

TEST_PROGS += $(noinst_PROGRAMS)
