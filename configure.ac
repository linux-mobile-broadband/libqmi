dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.68])

AC_INIT([libqmi-glib], [0.0.1], [aleksander@gnu.org])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 no-define no-dist-gzip dist-xz tar-ustar -Wno-portability])
AM_MAINTAINER_MODE([enable])

dnl Support silent build rules. Disable
dnl by either passing --disable-silent-rules to configure or passing V=1
dnl to make
AM_SILENT_RULES([yes])

dnl Required programs
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL

dnl Initialize libtool
LT_PREREQ([2.2])
LT_INIT

dnl General dependencies
PKG_CHECK_MODULES(LIBQMI_GLIB,
                  glib-2.0 >= 2.22
                  gobject-2.0
                  gio-2.0)
AC_SUBST(LIBQMI_GLIB_CFLAGS)
AC_SUBST(LIBQMI_GLIB_LIBS)

dnl General cli dependencies
PKG_CHECK_MODULES(QMICLI,
                  glib-2.0 >= 2.22
                  gobject-2.0
                  gio-2.0)
AC_SUBST(QMICLI_CFLAGS)
AC_SUBST(QMICLI_LIBS)

GLIB_MKENUMS=`pkg-config --variable=glib_mkenums glib-2.0`
AC_SUBST(GLIB_MKENUMS)

AC_CONFIG_FILES([Makefile
                 build-aux/Makefile
                 src/Makefile
                 cli/Makefile])
AC_OUTPUT

echo "
    libqmi-glib $VERSION
    ==============================================

    compiler:                ${CC}
    cflags:                  ${CFLAGS}
    Maintainer mode:         ${USE_MAINTAINER_MODE}
"
