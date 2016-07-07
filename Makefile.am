
SUBDIRS = . data build-aux src utils docs

ACLOCAL_AMFLAGS = -I m4

DISTCHECK_CONFIGURE_FLAGS = \
	--with-udev-base-dir="$$dc_install_base" \
	--enable-gtk-doc \
	--enable-mbim-qmux \
	$(NULL)

EXTRA_DIST = \
	gtester.make \
	COPYING.LIB
