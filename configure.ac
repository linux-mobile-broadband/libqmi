dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.68])

dnl The QMI version number
m4_define([qmi_major_version], [1])
m4_define([qmi_minor_version], [29])
m4_define([qmi_micro_version], [7])
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
m4_define([qmi_glib_lt_current],  [12])
m4_define([qmi_glib_lt_revision], [0])
m4_define([qmi_glib_lt_age],      [7])

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

dnl-----------------------------------------------------------------------------
dnl Compiler warnings
dnl

dnl Make sure autoconf-archive is available
m4_pattern_forbid([^AX_(COMPILER_FLAGS|COMPILER_FLAGS_(CFLAGS|GIR|LDFLAGS))\b],
  [Unexpanded AX_ macro found. Please install GNU autoconf-archive])

dnl Detect git or release builds
AX_IS_RELEASE([git-directory])

dnl Function type cast disabled: used throughout the code especially to
dnl cast GAsyncReadyCallbacks with the real object type instead of GObject
DISABLED_WARNINGS="${DISABLED_WARNINGS} -Wno-cast-function-type"

dnl All message protocol structs are packed, never complain about it
DISABLED_WARNINGS="${DISABLED_WARNINGS} -Wno-packed"

dnl Setup compiler checks
AX_COMPILER_FLAGS()
AX_COMPILER_FLAGS_CFLAGS(,,,[${DISABLED_WARNINGS}])

dnl Specify gnu89 mode
if test "$GCC" = "yes"; then
   CFLAGS="$CFLAGS -std=gnu89"
fi

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
GLIB_VERSION=2.56
GUDEV_VERSION=232

GLIB_BUILD_SYMBOLS="-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_56 -DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_56 -DGLIB_DISABLE_DEPRECATION_WARNINGS"

dnl GLib, GIO...
PKG_CHECK_MODULES(GLIB,
                  glib-2.0 >= $GLIB_VERSION
                  gobject-2.0
                  gio-2.0
                  gio-unix-2.0)
GLIB_CFLAGS="$GLIB_CFLAGS $GLIB_BUILD_SYMBOLS"
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

AM_PATH_PYTHON([], [], [PYTHON=python])

dnl GObject Introspection
GOBJECT_INTROSPECTION_CHECK([0.9.6])

dnl rmnet support in the kernel
AC_EGREP_HEADER([ifla_rmnet_flags],
                [linux/if_link.h],
                [enable_rmnet_support=yes],
                [enable_rmnet_support=no])
if test "x$enable_rmnet_support" = "xyes"; then
    AC_DEFINE(RMNET_SUPPORT_ENABLED, 1, [Define if rmnet support is built])
fi
AM_CONDITIONAL([RMNET_SUPPORT_ENABLED], test "x$enable_rmnet_support" = "xyes")

dnl message collection (existing file in data/qmi-collection-${COLLECTION}.json)
dnl custom collections may be added as files in data/
AC_ARG_ENABLE(collection,
              AS_HELP_STRING([--enable-collection=[minimal|basic|full]],
                             [message collection to build [default=full]]),
              [enable_collection=$enableval],
              [enable_collection=full])
QMI_COLLECTION_NAME="$enable_collection"
AC_SUBST(QMI_COLLECTION_NAME)
AM_CONDITIONAL([QMI_COLLECTION_USED], test "$enable_collection" != "full")

dnl qmi-firmware-update is optional, enabled by default
AC_ARG_ENABLE([firmware-update],
              AS_HELP_STRING([--enable-firmware-update],
                             [enable compilation of `qmi-firmware-update' [default=yes]]),
              [build_firmware_update=$enableval],
              [build_firmware_update=yes])

if test "x$build_firmware_update" = "xyes"; then
    if test "$QMI_COLLECTION_NAME" = "minimal"; then
        AC_MSG_WARN([Cannot build qmi-firmware-update when 'minimal' collection enabled, use at least the 'basic' collection instead.])
        build_firmware_update=no
    fi
fi
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
if test "x$enable_gtk_doc" = "xyes"; then
    if test "x$QMI_COLLECTION_NAME" != "xfull"; then
        AC_MSG_WARN([Cannot build gtk-doc when using an explicit message collection.])
        enable_gtk_doc=no
    fi
fi
AM_CONDITIONAL([ENABLE_GTK_DOC], [test "x$enable_gtk_doc" = "xyes"])

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
AM_CONDITIONAL([QMI_MBIM_QMUX_SUPPORTED], [test "x$QMI_MBIM_QMUX_SUPPORTED" = "x1"])

# QRTR support
QRTR_GLIB_VERSION=1.0.0
PKG_CHECK_MODULES([QRTR], [qrtr-glib >= ${QRTR_GLIB_VERSION}], [have_qrtr=yes], [have_qrtr=no])
AC_ARG_ENABLE(qrtr,
              AS_HELP_STRING([--enable-qrtr], [Enable support for QMI over QRTR [default=auto]]),
              [enable_qrtr=$enableval],
              [enable_qrtr=auto])

if test "x$enable_qrtr" = "xauto"; then
    enable_qrtr=$have_qrtr
fi

if test "x$enable_qrtr" = "xyes"; then
    if test "x$have_qrtr" = "xno"; then
        AC_MSG_ERROR([Couldn't find `libqrtr-glib` >= ${QRTR_GLIB_VERSION}. Install it, or otherwise configure using --disable-qrtr to disable the QMI over QRTR support.])
    fi
    AC_DEFINE(QRTR_ENABLED, 1, [Define if QRTR QMUX support enabled])
    AC_SUBST(QRTR_CFLAGS)
    AC_SUBST(QRTR_LIBS)
    QMI_QRTR_SUPPORTED=1
    QRTR_PKGCONFIG_NAME="qrtr-glib"
else
    QMI_QRTR_SUPPORTED=0
    QRTR_PKGCONFIG_NAME=""
fi
AC_SUBST(QMI_QRTR_SUPPORTED)
AC_SUBST(QRTR_PKGCONFIG_NAME)
AM_CONDITIONAL([QMI_QRTR_SUPPORTED], [test "x$QMI_QRTR_SUPPORTED" = "x1"])

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
                 examples/Makefile
                 examples/simple-tester-python/Makefile
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
    libqmi $VERSION
    ==============================================

    Build:
      compiler:                 ${CC}
      cflags:                   ${CFLAGS}
      ldflags:                  ${LDFLAGS}
      warn cflags:              ${WARN_CFLAGS}
      warn ldflags:             ${WARN_LDFLAGS}
      Maintainer mode:          ${USE_MAINTAINER_MODE}
      Documentation:            ${enable_gtk_doc}
      gobject introspection:    ${found_introspection}

    System paths:
      prefix:                   ${prefix}
      udev base directory:      ${UDEV_BASE_DIR}

    Features:
      QMI over MBIM:            ${enable_mbim_qmux}
      QMI over QRTR:            ${enable_qrtr}
      QMI username:             ${QMI_USERNAME_ENABLED} (${QMI_USERNAME})
      rmnet support:            ${enable_rmnet_support}

    Built items:
      libqmi-glib:              yes (${QMI_COLLECTION_NAME})
      qmicli:                   yes
      qmi-firmware-update:      ${build_firmware_update}
          with udev:                    ${with_udev}
          with MM runtime check:        ${enable_mm_runtime_check}
"
