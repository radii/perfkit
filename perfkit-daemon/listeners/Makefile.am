#
# DBus Perfkit Daemon Listener - Accepts DBus Communication
#

dbus_LTLIBRARIES = dbus.la
dbusdir = $(libdir)/perfkit-daemon/listeners

WARNINGS =								\
	-Wall								\
	-Werror								\
	-Wold-style-definition						\
	-Wdeclaration-after-statement					\
	-Wredundant-decls						\
	-Wmissing-noreturn						\
	-Wshadow							\
	-Wcast-align							\
	-Wwrite-strings							\
	-Winline							\
	-Wformat-nonliteral						\
	-Wformat-security						\
	-Wswitch-enum							\
	-Wswitch-default						\
	-Winit-self							\
	-Wmissing-include-dirs						\
	-Wundef								\
	-Waggregate-return						\
	-Wmissing-format-attribute					\
	-Wnested-externs

AM_CPPFLAGS =								\
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)s/locale"\"	\
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\"				\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"				\
	-DPACKAGE_LIB_DIR=\""$(libdir)"\"				\
	-DPACKAGE_SYSCONFDIR=\""$(sysconfdir)"\"			\
	-DGETTEXT_PACKAGE=\""perfkit-daemon"\"				\
	-DHAVE_CONFIG_H							\
	-DG_LOG_DOMAIN=\"PerfkitDaemon\"				\
	-I$(srcdir)/dbus						\
	$(WARNINGS)							\
	$(PERFKIT_DAEMON_CFLAGS)					\
	$(DBUS_CFLAGS)							\
	$(NULL)

if HAVE_DBUS_0_82
AM_CPPFLAGS += -DHAVE_DBUS_0_82
endif

NOINST_H_FILES =							\
	pkd-dbus.h							\
	dbus/pkd-dbus-manager.h						\
	dbus/pkd-dbus-subscription.h					\
	dbus/pkd-channel-dbus.h						\
	dbus/pkd-source-glue.h						\
	$(NULL)

dbus_la_SOURCES =							\
	$(NOINST_H_FILES)						\
	pkd-dbus.c							\
	dbus/pkd-dbus-manager.c						\
	dbus/pkd-dbus-subscription.c					\
	$(NULL)

dbus_la_LIBADD =							\
	$(PERFKIT_DAEMON_LIBS)						\
	$(DBUS_LIBS)							\
	$(top_builddir)/perfkit-daemon/libperfkit-daemon.la		\
	$(NULL)

dbus_la_LDFLAGS =							\
	-export-dynamic							\
	-export-symbols-regex "^pkd_.*"					\
	-module								\
	$(NULL)

