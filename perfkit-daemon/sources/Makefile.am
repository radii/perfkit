#
# Data source plugins.
#

source_LTLIBRARIES = memory.la
sourcedir = $(libdir)/perfkit-daemon/sources

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
	$(WARNINGS)							\
	$(PERFKIT_DAEMON_CFLAGS)					\
	$(NULL)

NOINST_H_FILES =							\
	pkd-memory.h							\
	$(NULL)

memory_la_SOURCES =							\
	$(NOINST_H_FILES)						\
	pkd-memory.c							\
	$(NULL)

memory_la_LIBADD =							\
	$(PERFKIT_DAEMON_LIBS)						\
	$(top_builddir)/perfkit-daemon/libperfkit-daemon.la		\
	-lz								\
	$(NULL)

memory_la_LDFLAGS =							\
	-export-dynamic							\
	-export-symbols-regex "^pkd_.*"					\
	-module								\
	$(NULL)

