
bin_SCRIPTS = qmi-network
noinst_PROGRAMS = swi-update

qmi-network: qmi-network.in
	$(AM_V_GEN) sed -e s,@VERSION\@,$(VERSION), $< > $@.tmp && mv $@.tmp $@
	@chmod a+x $@

EXTRA_DIST = qmi-network.in

CLEANFILES = qmi-network
