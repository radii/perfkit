#
# DBus Perfkit Agent Listener - Accepts DBus Communication
#

dbus_LTLIBRARIES = dbus.la
dbusdir = $(libdir)/perfkit-agent/listeners

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
	-DGETTEXT_PACKAGE=\""perfkit-agent"\"				\
	-DHAVE_CONFIG_H							\
	-DG_LOG_DOMAIN=\"PerfkitAgent\"					\
	-I$(srcdir)/dbus						\
	$(WARNINGS)							\
	$(PERFKIT_AGENT_CFLAGS)						\
	$(DBUS_CFLAGS)							\
	$(NULL)

if HAVE_DBUS_0_82
AM_CPPFLAGS += -DHAVE_DBUS_0_82
endif

NOINST_H_FILES =							\
	pka-dbus.h							\
	dbus/pka-dbus-manager.h						\
	dbus/pka-dbus-subscription.h					\
	dbus/pka-channel-dbus.h						\
	dbus/pka-source-glue.h						\
	$(NULL)

dbus_la_SOURCES =							\
	$(NOINST_H_FILES)						\
	pka-dbus.c							\
	dbus/pka-dbus-manager.c						\
	dbus/pka-dbus-subscription.c					\
	$(NULL)

dbus_la_LIBADD =							\
	$(PERFKIT_AGENT_LIBS)						\
	$(DBUS_LIBS)							\
	$(top_builddir)/perfkit-agent/libperfkit-agent.la		\
	$(NULL)

dbus_la_LDFLAGS =							\
	-export-dynamic							\
	-export-symbols-regex "^pka_.*"					\
	-module								\
	$(NULL)

