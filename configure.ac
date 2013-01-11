dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.68])

AC_INIT([libmbim], [0.0.1], [aleksander@gnu.org])
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

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo "
    libmbim $VERSION
    ==============================================

    compiler:                ${CC}
    cflags:                  ${CFLAGS}
    Maintainer mode:         ${USE_MAINTAINER_MODE}
"
