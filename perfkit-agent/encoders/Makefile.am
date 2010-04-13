#
# ZLib Perfkit Agent Encoder - Encode Samples/Manifests with ZLib
#

zlib_LTLIBRARIES = zlib.la
zlibdir = $(libdir)/perfkit-agent/encoders

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
	$(WARNINGS)							\
	$(PERFKIT_AGENT_CFLAGS)						\
	$(NULL)

NOINST_H_FILES =							\
	pka-zlib.h							\
	$(NULL)

zlib_la_SOURCES =							\
	$(NOINST_H_FILES)						\
	pka-zlib.c							\
	$(NULL)

zlib_la_LIBADD =							\
	$(PERFKIT_AGENT_LIBS)						\
	$(top_builddir)/perfkit-agent/libperfkit-agent.la		\
	-lz								\
	$(NULL)

zlib_la_LDFLAGS =							\
	-export-dynamic							\
	-export-symbols-regex "^pka_.*"					\
	-module								\
	$(NULL)

