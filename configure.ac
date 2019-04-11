dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.68])

dnl The QMI version number
m4_define([qmi_major_version], [1])
m4_define([qmi_minor_version], [22])
m4_define([qmi_micro_version], [4])
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
m4_define([qmi_glib_lt_current],  [10])
m4_define([qmi_glib_lt_revision], [0])
m4_define([qmi_glib_lt_age],      [5])


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

dnl Required dependency versions
GLIB_VERSION=2.36
GUDEV_VERSION=147

dnl GLib, GIO...
PKG_CHECK_MODULES(GLIB,
                  glib-2.0 >= $GLIB_VERSION
                  gobject-2.0
                  gio-2.0
                  gio-unix-2.0)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

GLIB_MKENUMS=`$PKG_CONFIG --variable=glib_mkenums glib-2.0`
AC_SUBST(GLIB_MKENUMS)

AM_PATH_PYTHON([], [], [PYTHON=python])

dnl qmi-firmware-update is optional, enabled by default
AC_ARG_ENABLE([firmware-update],
              AS_HELP_STRING([--enable-firmware-update],
                             [enable compilation of `qmi-firmware-update' [default=yes]]),
              [build_firmware_update=$enableval],
              [build_firmware_update=yes])
AM_CONDITIONAL([BUILD_FIRMWARE_UPDATE], [test "x$build_firmware_update" = "xyes"])

dnl udev support is optional, enabled by default
AC_ARG_WITH(udev, AS_HELP_STRING([--without-udev], [Build without udev support]), [], [with_udev=yes])
case $with_udev in
    yes)
        if test "x$build_firmware_update" = "xyes"; then
            PKG_CHECK_MODULES(GUDEV, [gudev-1.0 >= $GUDEV_VERSION], [have_gudev=yes],[have_gudev=no])
            if test "x$have_gudev" = "xno"; then
                AC_MSG_ERROR([Couldn't find gudev >= $GUDEV_VERSION. Install it, or otherwise configure using --without-udev to disable udev support.])
            else
                AC_DEFINE(WITH_UDEV, 1, [Define if you want udev support])
                AC_SUBST(GUDEV_CFLAGS)
                AC_SUBST(GUDEV_LIBS)
            fi
        else
            with_udev="n/a"
        fi
        ;;
    *)
        with_udev=no
        ;;
esac

dnl runtime MM check is optional, enabled by default
AC_ARG_ENABLE(mm-runtime-check, AS_HELP_STRING([--disable-mm-runtime-check], [Build without ModemManager runtime check]), [], [enable_mm_runtime_check=yes])
case $enable_mm_runtime_check in
    yes)
        if test "x$build_firmware_update" = "xyes"; then
            AC_DEFINE(MM_RUNTIME_CHECK_ENABLED, 1, [Define if you want ModemManager runtime check])
        else
            enable_mm_runtime_check="n/a"
        fi
        ;;
    *)
        enable_mm_runtime_check=no
        ;;
esac

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
MBIM_GLIB_VERSION=1.18.0
PKG_CHECK_MODULES([MBIM], [mbim-glib >= ${MBIM_GLIB_VERSION}], [have_mbim=yes], [have_mbim=no])
AC_ARG_ENABLE(mbim-qmux,
              AS_HELP_STRING([--enable-mbim-qmux], [Enable support for QMI over MBIM QMUX service [default=auto]]),
              [enable_mbim_qmux=$enableval],
              [enable_mbim_qmux=auto])

if test "x$enable_mbim_qmux" = "xauto"; then
    enable_mbim_qmux=$have_mbim
fi

if test "x$enable_mbim_qmux" = "xyes"; then
    if test "x$have_mbim" = "xno"; then
        AC_MSG_ERROR([Couldn't find `libmbim-glib` >= ${MBIM_GLIB_VERSION}. Install it, or otherwise configure using --disable-mbim-qmux to disable the QMI over MBIM QMUX service.])
    fi
    AC_DEFINE(MBIM_QMUX_ENABLED, 1, [Define if MBIM QMUX support enabled])
    AC_SUBST(MBIM_CFLAGS)
    AC_SUBST(MBIM_LIBS)
    QMI_MBIM_QMUX_SUPPORTED=1
else
    QMI_MBIM_QMUX_SUPPORTED=0
fi
AC_SUBST(QMI_MBIM_QMUX_SUPPORTED)

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
                 src/qmi-firmware-update/Makefile
                 src/qmi-firmware-update/test/Makefile
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

    compiler:              ${CC}
    cflags:                ${CFLAGS}
    Maintainer mode:       ${USE_MAINTAINER_MODE}
    udev base directory:   ${UDEV_BASE_DIR}
    Documentation:         ${enable_gtk_doc}
    QMI username:          ${QMI_USERNAME_ENABLED} (${QMI_USERNAME})
    QMUX over MBIM:        ${enable_mbim_qmux}

    Built items:
      libqmi-glib:         yes
      qmicli:              yes
      qmi-firmware-update: ${build_firmware_update}
          with udev:             ${with_udev}
          with MM runtime check: ${enable_mm_runtime_check}
"
