#
# Data source plugins.
#

gtkmodule_LTLIBRARIES = libgdkevent-module.la
source_LTLIBRARIES =		\
	memory.la		\
	sched.la		\
	netdev.la		\
	cpu.la			\
	gdkevent.la		\
	$(NULL)

gtkmoduledir = $(libdir)/gtk-2.0/modules
sourcedir = $(libdir)/perfkit-agent/plugins

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
	-DG_LOG_DOMAIN=\"Source\"					\
	-I$(top_srcdir)/cut-n-paste					\
	$(WARNINGS)							\
	$(PERFKIT_AGENT_CFLAGS)						\
	$(PERFKIT_DEBUG_CFLAGS)						\
	$(NULL)

AM_LDFLAGS =								\
	-export-dynamic							\
	-module								\
	$(NULL)

memory_la_SOURCES = memory.c
sched_la_SOURCES = sched.c
netdev_la_SOURCES = netdev.c src-utils.c src-utils.h
cpu_la_SOURCES = cpu.c src-utils.c src-utils.h
gdkevent_la_SOURCES = gdkevent.c

libgdkevent_module_la_SOURCES = gdkevent-module.c
libgdkevent_module_la_CPPFLAGS = $(GTK_CFLAGS)
libgdkevent_module_la_LIBADD = $(GTK_LIBS)
