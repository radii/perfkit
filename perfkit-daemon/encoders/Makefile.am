#
# ZLib Perfkit Daemon Encoder - Encode Samples/Manifests with ZLib
#

zlib_LTLIBRARIES = zlib.la
zlibdir = $(libdir)/perfkit-daemon/encoders

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
	pkd-zlib.h							\
	$(NULL)

zlib_la_SOURCES =							\
	$(NOINST_H_FILES)						\
	pkd-zlib.c							\
	$(NULL)

la_LIBADD =								\
	$(PERFKIT_DAEMON_LIBS)						\
	$(top_builddir)/perfkit-daemon/libperfkit-daemon.la		\
	-lz								\
	$(NULL)

zlib_la_LDFLAGS =							\
	-export-dynamic							\
	-export-symbols-regex "^pkd_.*"					\
	-module								\
	$(NULL)

