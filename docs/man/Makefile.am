
dist_man_MANS = qmicli.1

# List of all source files which affect the output of --help-all
SOURCES_WITH_HELP = \
	$(top_srcdir)/cli/qmicli.c \
	$(top_srcdir)/cli/qmicli-dms.c \
	$(top_srcdir)/cli/qmicli-nas.c \
	$(top_srcdir)/cli/qmicli-wds.c \
	$(top_srcdir)/cli/qmicli-pbm.c \
	$(top_srcdir)/cli/qmicli-uim.c

# Depend only in the source files, not in the actual program, so that the
# manpage doesn't get rebuilt when building from a tarball
# Also, make sure that the qmicli.1 file is always generated, even when help2man
# is not available
qmicli.1: $(SOURCES_WITH_HELP)
	$(AM_V_GEN) \
		$(HELP2MAN) \
			--output=$@ \
			--name='Control QMI devices' \
			--help-option='--help-all' \
			--libtool \
			$(top_builddir)/cli/qmicli || \
		touch $@
