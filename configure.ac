dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.68])

dnl The libmbim version number
m4_define([mbim_major_version], [1])
m4_define([mbim_minor_version], [1])
m4_define([mbim_micro_version], [0])
m4_define([mbim_version],
          [mbim_major_version.mbim_minor_version.mbim_micro_version])

AC_INIT([libmbim], [mbim_version], [aleksander@gnu.org])
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

dnl Specific warnings to always use
LIBMBIM_COMPILER_WARNINGS

dnl Version stuff
MBIM_MAJOR_VERSION=mbim_major_version
MBIM_MINOR_VERSION=mbim_minor_version
MBIM_MICRO_VERSION=mbim_micro_version
MBIM_VERSION=mbim_version
AC_SUBST(MBIM_MAJOR_VERSION)
AC_SUBST(MBIM_MINOR_VERSION)
AC_SUBST(MBIM_MICRO_VERSION)
AC_SUBST(MBIM_VERSION)

dnl General dependencies for libmbim-glib
PKG_CHECK_MODULES(LIBMBIM_GLIB,
                  glib-2.0 >= 2.32
                  gobject-2.0
                  gio-2.0
                  gudev-1.0 >= 147)
AC_SUBST(LIBMBIM_GLIB_CFLAGS)
AC_SUBST(LIBMBIM_GLIB_LIBS)

dnl General dependencies for mbimcli
PKG_CHECK_MODULES(MBIMCLI,
                  glib-2.0 >= 2.32
                  gobject-2.0
                  gio-2.0)
AC_SUBST(MBIMCLI_CFLAGS)
AC_SUBST(MBIMCLI_LIBS)

GLIB_MKENUMS=`$PKG_CONFIG --variable=glib_mkenums glib-2.0`
AC_SUBST(GLIB_MKENUMS)

dnl Documentation
GTK_DOC_CHECK(1.0)

dnl Tests
AC_ARG_WITH(tests, AS_HELP_STRING([--with-tests], [Build libmbim tests]))
AM_CONDITIONAL(WITH_TESTS, test "x$with_tests" = "xyes")
case $with_tests in
    yes)
        with_tests=yes
        ;;
    *)
        with_tests=no
        ;;
esac

AC_CONFIG_FILES([Makefile
                 build-aux/Makefile
                 build-aux/templates/Makefile
                 build-aux/mbim-codegen/Makefile
                 data/Makefile
                 data/pkg-config/Makefile
                 data/pkg-config/mbim-glib.pc
                 libmbim-glib/Makefile
                 libmbim-glib/mbim-version.h
                 libmbim-glib/generated/Makefile
                 libmbim-glib/test/Makefile
                 cli/Makefile
                 utils/Makefile
                 docs/Makefile
                 docs/reference/Makefile
                 docs/reference/libmbim-glib/Makefile
                 docs/reference/libmbim-glib/version.xml])
AC_OUTPUT

echo "
    libmbim $VERSION
    ==============================================

    compiler:                ${CC}
    cflags:                  ${CFLAGS}
    Maintainer mode:         ${USE_MAINTAINER_MODE}
    Tests:                   ${with_tests}
    Documentation:           ${enable_gtk_doc}
"
