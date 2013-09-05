dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.68])

dnl The QMI version number
m4_define([qmi_major_version], [1])
m4_define([qmi_minor_version], [6])
m4_define([qmi_micro_version], [0])
m4_define([qmi_version],
          [qmi_major_version.qmi_minor_version.qmi_micro_version])

dnl libtool versioning for libqmi-glib (-version-info c:r:a)
dnl    If the interface is unchanged, but the implementation has changed or
dnl        been fixed, then increment r.
dnl    Otherwise, increment c and zero r.
dnl        If the interface has grown (that is, the new library is compatible
dnl            with old code), increment a.
dnl        If the interface has changed in an incompatible way (that is,
dnl            functions have changed or been removed), then zero a.
m4_define([qmi_glib_lt_current],  [1])
m4_define([qmi_glib_lt_revision], [0])
m4_define([qmi_glib_lt_age],      [0])


AC_INIT([libqmi], [qmi_version], [libqmi-devel@lists.freedesktop.org])
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
LIBQMI_COMPILER_WARNINGS

dnl Version stuff
QMI_MAJOR_VERSION=qmi_major_version
QMI_MINOR_VERSION=qmi_minor_version
QMI_MICRO_VERSION=qmi_micro_version
QMI_VERSION=qmi_version
AC_SUBST(QMI_MAJOR_VERSION)
AC_SUBST(QMI_MINOR_VERSION)
AC_SUBST(QMI_MICRO_VERSION)
AC_SUBST(QMI_VERSION)

dnl libtool version stuff
QMI_GLIB_LT_CURRENT=qmi_glib_lt_current
QMI_GLIB_LT_REVISION=qmi_glib_lt_revision
QMI_GLIB_LT_AGE=qmi_glib_lt_age
AC_SUBST(QMI_GLIB_LT_CURRENT)
AC_SUBST(QMI_GLIB_LT_REVISION)
AC_SUBST(QMI_GLIB_LT_AGE)

dnl General dependencies for libqmi-glib
PKG_CHECK_MODULES(LIBQMI_GLIB,
                  glib-2.0 >= 2.32
                  gobject-2.0
                  gio-2.0)
AC_SUBST(LIBQMI_GLIB_CFLAGS)
AC_SUBST(LIBQMI_GLIB_LIBS)

dnl General dependencies for qmicli
PKG_CHECK_MODULES(QMICLI,
                  glib-2.0 >= 2.28
                  gobject-2.0
                  gio-2.0)
AC_SUBST(QMICLI_CFLAGS)
AC_SUBST(QMICLI_LIBS)

GLIB_MKENUMS=`$PKG_CONFIG --variable=glib_mkenums glib-2.0`
AC_SUBST(GLIB_MKENUMS)

dnl Documentation
GTK_DOC_CHECK(1.0)

dnl Man page
AC_PATH_PROG(HELP2MAN, help2man, false)
AM_CONDITIONAL(BUILDOPT_MAN, test x$HELP2MAN != xfalse)

AC_CONFIG_FILES([Makefile
                 data/Makefile
                 data/pkg-config/Makefile
                 data/pkg-config/qmi-glib.pc
                 build-aux/Makefile
                 build-aux/templates/Makefile
                 build-aux/qmi-codegen/Makefile
                 libqmi-glib/Makefile
                 libqmi-glib/qmi-version.h
                 libqmi-glib/generated/Makefile
                 libqmi-glib/test/Makefile
                 cli/Makefile
                 cli/test/Makefile
                 utils/Makefile
                 docs/Makefile
                 docs/reference/Makefile
                 docs/reference/libqmi-glib/Makefile
                 docs/reference/libqmi-glib/version.xml
                 docs/man/Makefile])
AC_OUTPUT

echo "
    libqmi (libqmi-glib, qmicli) $VERSION
    ==============================================

    compiler:                ${CC}
    cflags:                  ${CFLAGS}
    Maintainer mode:         ${USE_MAINTAINER_MODE}
    Documentation:           ${enable_gtk_doc}
"
