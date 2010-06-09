dnl ************************************************************************
dnl Versioning Macros
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
AC_PREREQ(2.62)
AC_INIT(perfkit, perfkit_version)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([build/m4])
AM_MAINTAINER_MODE
AC_ISC_POSIX
AC_PROG_CC
# AC_PROG_CC_STDC
# AC_HEADER_STDC
AM_PATH_GLIB_2_0
AM_PROG_CC_C_O
GOBJECT_INTROSPECTION_CHECK([0.6.7])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])
AS_AM_REALLY_SILENT


dnl ************************************************************************
dnl Versioning Definitions
dnl ************************************************************************
PKA_MAJOR_VERSION=perfkit_major_version
PKA_MINOR_VERSION=perfkit_minor_version
PKA_MICRO_VERSION=perfkit_micro_version
PKA_API_VERSION=1.0
PKA_VERSION=perfkit_version
PK_MAJOR_VERSION=perfkit_major_version
PK_MINOR_VERSION=perfkit_minor_version
PK_MICRO_VERSION=perfkit_micro_version
PK_API_VERSION=1.0
PK_VERSION=perfkit_version
PKG_MAJOR_VERSION=perfkit_major_version
PKG_MINOR_VERSION=perfkit_minor_version
PKG_MICRO_VERSION=perfkit_micro_version
PKG_API_VERSION=1.0
PK_VERSION=perfkit_version
AC_SUBST(PKA_MAJOR_VERSION)
AC_SUBST(PKA_MINOR_VERSION)
AC_SUBST(PKA_MICRO_VERSION)
AC_SUBST(PKA_API_VERSION)
AC_SUBST(PKA_VERSION)
AC_SUBST(PK_MAJOR_VERSION)
AC_SUBST(PK_MINOR_VERSION)
AC_SUBST(PK_MICRO_VERSION)
AC_SUBST(PK_API_VERSION)
AC_SUBST(PK_VERSION)
AC_SUBST(PKG_MAJOR_VERSION)
AC_SUBST(PKG_MINOR_VERSION)
AC_SUBST(PKG_MICRO_VERSION)
AC_SUBST(PKG_API_VERSION)
AC_SUBST(PKG_VERSION)


dnl ************************************************************************
dnl Check for Required Modules
dnl ************************************************************************
PKG_CHECK_MODULES(PERFKIT_AGENT,  [gobject-2.0      >= 2.20
                                   gthread-2.0      >= 2.20
                                   gmodule-2.0      >= 2.20
                                   gio-2.0          >= 2.20])
PKG_CHECK_MODULES(PERFKIT_TOOLS,  [gobject-2.0      >= 2.20
                                   gthread-2.0      >= 2.20
                                   gio-2.0          >= 2.20])
PKG_CHECK_MODULES(PERFKIT,        [gobject-2.0      >= 2.20
                                   gthread-2.0      >= 2.20
                                   gio-2.0          >= 2.20])
PKG_CHECK_MODULES(PERFKIT_GUI,    [gobject-2.0      >= 2.20
                                   gthread-2.0      >= 2.20
                                   gtk+-2.0         >= 2.18
                                   ethos-1.0        >= 0.2.2
                                   ethos-ui-1.0     >= 0.2.2
                                   clutter-gtk-0.10 >= 0.10.2])
PKG_CHECK_MODULES(DBUS,           [dbus-glib-1      >= 0.80])


dnl ************************************************************************
dnl Enable additional DBUS features
dnl ************************************************************************
PKG_CHECK_MODULES(DBUS_0_82, [dbus-glib-1 >= 0.82],
                             [have_dbus_0_82=yes],
                             [have_dbus_0_82=no])
AM_CONDITIONAL(HAVE_DBUS_0_82, test "x$have_dbus_0_82" = "xyes")


dnl ************************************************************************
dnl Enable extra debugging options
dnl ************************************************************************
m4_define([debug_default],
	  [m4_if(m4_eval(perfkit_minor_version % 2), [1],
	  [yes], [minimum])])
AC_ARG_ENABLE([debug],
	      [AC_HELP_STRING([--enable-debug=@<:@no/minimum/yes@:>@],
	      		      [turn on debugging @<:@default=debug_default@:>@])],
	      		      [],
	      		      [enable_debug=debug_default])
AS_CASE([$enable_debug],
	[yes], [
		test "$cflags_set" = set || CFLAGS="$CFLAGS -g"
		PERFKIT_DEBUG_CFLAGS="-DPERFKIT_TRACE"
	],
	[minimum], [
		PERFKIT_DEBUG_CFLAGS="-DG_DISABLE_CAST_CHECKS"
	],
	[no], [
		PERFKIT_DEBUG_CFLAGS="-DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS"
	],
	[AC_MSG_ERROR([Unknown argument to --enable-debug])]
)
AC_SUBST(PERFKIT_DEBUG_CFLAGS)


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
SHAMROCK_EXPAND_LIBDIR
SHAMROCK_EXPAND_BINDIR
SHAMROCK_EXPAND_DATADIR


dnl ************************************************************************
dnl Output
dnl ************************************************************************
AC_OUTPUT([
	Makefile

	data/Makefile
	data/ui/Makefile

	doc/Makefile
	doc/reference/Makefile
	doc/reference/perfkit/Makefile
	doc/reference/perfkit/version.xml
	doc/reference/perfkit-agent/Makefile
	doc/reference/perfkit-agent/version.xml
	doc/reference/perfkit-gui/Makefile
	doc/reference/perfkit-gui/version.xml

	tools/Makefile

	perfkit/Makefile
	perfkit/connections/Makefile
	perfkit/pk-version.h

	perfkit-agent/Makefile
	perfkit-agent/sources/Makefile
	perfkit-agent/encoders/Makefile
	perfkit-agent/listeners/Makefile
	perfkit-agent/pka-version.h

	perfkit-gui/Makefile
	perfkit-gui/pkg-version.h

	bindings/Makefile
	bindings/vala/Makefile

	tests/Makefile
	tests/perfkit/Makefile
	tests/perfkit-agent/Makefile
	tests/test-suite/Makefile
])

echo ""
echo "Perfkit $VERSION"
echo ""
echo "  Prefix.....................: ${prefix}"
echo "  Debug Level................: ${enable_debug}"
echo "  Compiler Flags.............: ${CFLAGS}"
echo "  Enable API Reference.......: ${enable_gtk_doc}"
echo "  Enable Test Suite..........: ${enable_glibtest}"
echo ""
