dnl ************************************************************************
dnl Versioning
dnl ************************************************************************
m4_define([perfkit_major_version], [0])
m4_define([perfkit_minor_version], [1])
m4_define([perfkit_micro_version], [1])
m4_define([perfkit_version],
          [perfkit_major_version.perfkit_minor_version.perfkit_micro_version])
m4_define([perfkit_binary_age], [m4_eval(100 * perfkit_minor_version +
           perfkit_micro_version)])
m4_define([lt_current], [m4_eval(100 * perfkit_minor_version +
           perfkit_micro_version - perfkit_interface_age)])
m4_define([lt_revision], [perfkit_interface_age])
m4_define([lt_age], [m4_eval(perfkit_binary_age - perfkit_interface_age)])


dnl ************************************************************************
dnl Initialize Automake
dnl ************************************************************************
AC_INIT(perfkit, perfkit_version)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([build/m4])
AM_MAINTAINER_MODE
AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CC_STDC
AC_HEADER_STDC
AM_PATH_GLIB_2_0
PKD_MAJOR_VERSION=perfkit_major_version
PKD_MINOR_VERSION=perfkit_minor_version
PKD_MICRO_VERSION=perfkit_micro_version
PKD_API_VERSION=1.0
PKD_VERSION=perfkit_version
AC_SUBST(PKD_MAJOR_VERSION)
AC_SUBST(PKD_MINOR_VERSION)
AC_SUBST(PKD_MICRO_VERSION)
AC_SUBST(PKD_API_VERSION)
AC_SUBST(PKD_VERSION)


dnl ************************************************************************
dnl Check for Required Modules
dnl ************************************************************************
PKG_CHECK_MODULES(PERFKIT_DAEMON, [gobject-2.0 >= 2.16
                                   gthread-2.0 >= 2.16
                                   dbus-glib-1 >= 0.80])
PKG_CHECK_MODULES(PERFKIT_TOOLS,  [gobject-2.0 >= 2.16
				   dbus-glib-1 >= 0.80])


dnl ************************************************************************
dnl Additional Headers
dnl ************************************************************************
AC_CHECK_HEADERS([readline/readline.h readline/history.h],
		 [],
		 [AC_MSG_ERROR([Readline headers were not found.])])


dnl ************************************************************************
dnl Unit Tests
dnl ************************************************************************
AC_PATH_PROG([GTESTER], [gtester])
AC_PATH_PROG([GTESTER_REPORT], [gtester-report])
AM_CONDITIONAL(ENABLE_GLIB_TEST, test "x$enable_glibtest" = "xyes")


dnl ************************************************************************
dnl Miscellaneous
dnl ************************************************************************
AM_PROG_LIBTOOL
GTK_DOC_CHECK([1.11])
SHAVE_INIT([build], [enable])
SHAMROCK_EXPAND_LIBDIR
SHAMROCK_EXPAND_BINDIR
SHAMROCK_EXPAND_DATADIR


dnl ************************************************************************
dnl Output
dnl ************************************************************************
AC_OUTPUT([
	Makefile
	build/shave
	build/shave-libtool
	data/Makefile
	data/dbus/Makefile
	doc/Makefile
	doc/reference/Makefile
	tests/Makefile
	plugins/Makefile
	tools/Makefile

	doc/reference/perfkit-daemon/Makefile
	doc/reference/perfkit-daemon/version.xml
	perfkit-daemon/Makefile
	perfkit-daemon/pkd-version.h
])

echo ""
echo "Perfkit $VERSION"
echo ""
echo "  Prefix.....................: ${prefix}"
echo "  Enable API Reference.......: ${enable_gtk_doc}"
echo "  Enable Test Suite..........: ${enable_glibtest}"
echo ""
