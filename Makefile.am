
SUBDIRS = . build-aux data src utils docs

ACLOCAL_AMFLAGS = -I m4

DISTCHECK_CONFIGURE_FLAGS = \
	--with-udev-base-dir="$$dc_install_base" \
	--enable-gtk-doc

EXTRA_DIST = \
	gtester.make \
	COPYING.LIB
