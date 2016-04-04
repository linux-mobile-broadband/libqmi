dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.68])

dnl The QMI version number
m4_define([qmi_major_version], [1])
m4_define([qmi_minor_version], [15])
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
m4_define([qmi_glib_lt_current],  [5])
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

dnl Dependencies
PKG_CHECK_MODULES(GLIB,
                  glib-2.0 >= 2.32
                  gobject-2.0
                  gio-2.0
                  gio-unix-2.0)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

GLIB_MKENUMS=`$PKG_CONFIG --variable=glib_mkenums glib-2.0`
AC_SUBST(GLIB_MKENUMS)

dnl Documentation
GTK_DOC_CHECK(1.0)

# QMI username
QMI_USERNAME="root"
AC_ARG_ENABLE(qmi-username,
              AS_HELP_STRING([--enable-qmi-username=<username>], [user allowed to access QMI devices]))
if test -n "$enable_qmi_username" ; then
    QMI_USERNAME_ENABLED=yes
    AC_DEFINE(QMI_USERNAME_ENABLED, 1, [Define if we enable QMI username])
    QMI_USERNAME="$enable_qmi_username"
    AC_SUBST(QMI_USERNAME)
    AC_DEFINE_UNQUOTED(QMI_USERNAME, "$QMI_USERNAME", [Define the QMI username])
else
    QMI_USERNAME_ENABLED=no
fi

AM_CONDITIONAL([QMI_USERNAME_ENABLED], [test "x$QMI_USERNAME_ENABLED" = "xyes"])

# MBIM QMUX service support
AC_ARG_ENABLE(mbim-qmux,
              AS_HELP_STRING([--enable-mbim-qmux], [support QMI over MBIM QMUX service]))
if test -n "$enable_mbim_qmux"; then
    PKG_CHECK_MODULES(MBIM, mbim-glib >= 1.13)
    MBIM_CFLAGS="$MBIM_CFLAGS -DMBIM_QMUX"
    AC_SUBST(MBIM_CFLAGS)
    AC_SUBST(MBIM_LIBS)
fi

AM_CONDITIONAL(MBIM_QMUX, [test -n "$MBIM_CFLAGS"])

# udev base directory
AC_ARG_WITH(udev-base-dir, AS_HELP_STRING([--with-udev-base-dir=DIR], [where udev base directory is]))
if test -n "$with_udev_base_dir" ; then
    UDEV_BASE_DIR="$with_udev_base_dir"
else
    UDEV_BASE_DIR="/lib/udev"
fi
AC_SUBST(UDEV_BASE_DIR)

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
                 src/Makefile
                 src/libqmi-glib/Makefile
                 src/libqmi-glib/qmi-version.h
                 src/libqmi-glib/generated/Makefile
                 src/libqmi-glib/test/Makefile
                 src/qmicli/Makefile
                 src/qmicli/test/Makefile
                 src/qmi-proxy/Makefile
                 utils/Makefile
                 docs/Makefile
                 docs/reference/Makefile
                 docs/reference/libqmi-glib/Makefile
                 docs/reference/libqmi-glib/version.xml
                 docs/man/Makefile])

if test "x$QMI_USERNAME_ENABLED" = "xyes"; then
    AC_CONFIG_FILES([src/qmi-proxy/76-qmi-proxy-device-ownership.rules])
fi

AC_OUTPUT

echo "
    libqmi (libqmi-glib, qmicli) $VERSION
    ==============================================

    compiler:             ${CC}
    cflags:               ${CFLAGS}
    Maintainer mode:      ${USE_MAINTAINER_MODE}
    udev base directory:  ${UDEV_BASE_DIR}
    Documentation:        ${enable_gtk_doc}
    QMI username:         ${QMI_USERNAME_ENABLED} (${QMI_USERNAME})
    QMUX over MBIM:       ${enable_mbim_qmux}
"
