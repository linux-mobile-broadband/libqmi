
dist_man_MANS = mbimcli.1

# List of all source files which affect the output of --help-all
SOURCES_WITH_HELP = \
	$(top_srcdir)/cli/mbimcli-basic-connect.c \
	$(top_srcdir)/cli/mbimcli.c

# Depend only in the source files, not in the actual program, so that the
# manpage doesn't get rebuilt when building from a tarball
# Also, make sure that the mbimcli.1 file is always generated, even when help2man
# is not available
mbimcli.1: $(SOURCES_WITH_HELP)
	$(AM_V_GEN) \
		$(HELP2MAN) \
			--output=$@ \
			--name='Control MBIM devices' \
			--help-option='--help-all' \
			--libtool \
			$(top_builddir)/cli/mbimcli || \
		touch $@
