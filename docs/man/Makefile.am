
dist_man_MANS =

if BUILDOPT_MAN
dist_man_MANS += qmicli.1
endif

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
qmicli.1: $(SOURCES_WITH_HELP)
	- $(AM_V_GEN) $(HELP2MAN) \
		--output=$@ \
		--name='Control QMI devices' \
		--help-option='--help-all' \
		--libtool \
		$(top_builddir)/cli/qmicli
