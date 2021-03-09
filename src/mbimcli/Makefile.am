
bin_PROGRAMS = mbimcli

mbimcli_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(MBIMCLI_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/libmbim-glib \
	-I$(top_builddir)/src/libmbim-glib \
	-I$(top_srcdir)/src/libmbim-glib/generated \
	-I$(top_builddir)/src/libmbim-glib/generated \
	$(NULL)

mbimcli_SOURCES = \
	mbimcli.h mbimcli.c \
	mbimcli-helpers.h mbimcli-helpers.c \
	mbimcli-basic-connect.c \
	mbimcli-phonebook.c \
	mbimcli-dss.c \
	mbimcli-ms-firmware-id.c \
	mbimcli-ms-host-shutdown.c \
	mbimcli-ms-sar.c \
	mbimcli-atds.c \
	mbimcli-intel-firmware-update.c \
	mbimcli-ms-basic-connect-extensions.c \
	mbimcli-link-management.c \
	$(NULL)

mbimcli_LDADD = \
	$(top_builddir)/src/common/libmbim-common.la \
	$(top_builddir)/src/libmbim-glib/libmbim-glib.la \
	$(NULL)

mbimcli_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(MBIMCLI_LIBS) \
	$(NULL)

completiondir = $(datadir)/bash-completion/completions

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(completiondir)
	$(INSTALL_DATA) $(srcdir)/mbimcli-completion $(DESTDIR)$(completiondir)/mbimcli

uninstall-hook:
	rm -f $(DESTDIR)$(completiondir)/mbimcli

EXTRA_DIST = mbimcli-completion
