#
# Perfkit Agent
#

SUBDIRS = . sources encoders listeners

noinst_LTLIBRARIES = libperfkit-agent.la
bin_PROGRAMS = perfkit-agent

headerdir = $(prefix)/include/perfkit-agent-1.0/perfkit-agent
header_DATA = $(INST_H_FILES)

etcdir = $(sysconfdir)/perfkit
etc_DATA = agent.conf

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
	-DPERFKIT_COMPILATION						\
	-DHAVE_CONFIG_H							\
	-DG_LOG_DOMAIN=\"PerfkitAgent\"					\
	-I$(top_srcdir)/cut-n-paste					\
	$(WARNINGS)							\
	$(PERFKIT_AGENT_CFLAGS)						\
	$(NULL)

INST_H_FILES =								\
	pka-channel.h							\
	pka-config.h							\
	pka-encoder.h							\
	pka-encoder-info.h						\
	pka-listener.h							\
	pka-manifest.h							\
	pka-pipeline.h							\
	pka-sample.h							\
	pka-source.h							\
	pka-source-info.h						\
	pka-source-simple.h						\
	pka-spawn-info.h						\
	pka-subscription.h						\
	pka-version.h							\
	perfkit-agent.h							\
	$(NULL)

NOINST_H_FILES =							\
	pka-log.h							\
	pka-plugins.h							\
	$(NULL)

libperfkit_agent_la_SOURCES =						\
	$(INST_H_FILES)							\
	$(NOINST_H_FILES)						\
	pka-channel.c							\
	pka-config.c							\
	pka-encoder.c							\
	pka-encoder-info.c						\
	pka-listener.c							\
	pka-log.c							\
	pka-manifest.c							\
	pka-pipeline.c							\
	pka-plugins.c							\
	pka-sample.c							\
	pka-source.c							\
	pka-source-info.c						\
	pka-source-simple.c						\
	pka-subscription.c						\
	$(top_srcdir)/cut-n-paste/egg-buffer.c				\
	$(NULL)

libperfkit_agent_la_LIBADD = $(PERFKIT_AGENT_LIBS)
libperfkit_agent_la_LDFLAGS =						\
	-export-dynamic							\
	-export-symbols-regex "^pka_.*"					\
	$(NULL)

perfkit_agent_SOURCES = main.c
perfkit_agent_LDADD = libperfkit-agent.la $(PERFKIT_AGENT_LIBS)
perfkit_agent_LDFLAGS =							\
	-export-dynamic							\
	-export-symbols-regex "^pka_.*"					\
	$(NULL)

test:
	@PERFKIT_SOURCES_DIR=$(builddir)/sources/.libs			\
	PERFKIT_LISTENERS_DIR=$(builddir)/listeners/.libs		\
	PERFKIT_ENCODERS_DIR=$(builddir)/encoders/.libs			\
	$(builddir)/perfkit-agent --stdout -c $(srcdir)/agent.conf

debug:
	@PERFKIT_SOURCES_DIR=$(builddir)/sources/.libs			\
	PERFKIT_LISTENERS_DIR=$(builddir)/listeners/.libs		\
	PERFKIT_ENCODERS_DIR=$(builddir)/encoders/.libs			\
	gdb --args $(builddir)/perfkit-agent --stdout			\
	-c $(srcdir)/agent.conf

valgrind:
	@PERFKIT_SOURCES_DIR=$(builddir)/sources/.libs			\
	PERFKIT_LISTENERS_DIR=$(builddir)/listeners/.libs		\
	PERFKIT_ENCODERS_DIR=$(builddir)/encoders/.libs			\
	PERFKIT_VALGRIND=1						\
	G_SLICE=always-malloc G_DEBUG=gc-friendly			\
	libtool --mode=execute valgrind --tool=memcheck			\
	--leak-check=full --leak-resolution=high			\
	--log-file=perfkit-agent.valgrind --show-reachable=yes		\
	--suppressions=$(top_builddir)/cut-n-paste/valgrind.suppression	\
	$(builddir)/perfkit-agent --stdout -c $(srcdir)/agent.conf

