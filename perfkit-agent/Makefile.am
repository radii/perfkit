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
	-DG_LOG_DOMAIN=\"Agent\"					\
	-D_GNU_SOURCE							\
	-I$(top_srcdir)/cut-n-paste					\
	$(WARNINGS)							\
	$(PERFKIT_AGENT_CFLAGS)						\
	$(NULL)

# AM_CPPFLAGS += -DDISABLE_TRACE

INST_H_FILES =								\
	pka-channel.h							\
	pka-config.h							\
	pka-context.h							\
	pka-encoder.h							\
	pka-listener.h							\
	pka-listener-lowlevel.h						\
	pka-log.h							\
	pka-manager.h							\
	pka-manifest.h							\
	pka-plugin.h							\
	pka-sample.h							\
	pka-source.h							\
	pka-source-simple.h						\
	pka-spawn-info.h						\
	pka-subscription.h						\
	pka-version.h							\
	perfkit-agent.h							\
	$(NULL)

NOINST_H_FILES =							\
	pka-listener-closures.h						\
	$(NULL)

BUILT_SOURCES =								\
	pka-marshal.h							\
	pka-marshal.c							\
	$(NULL)

libperfkit_agent_la_SOURCES =						\
	$(BUILT_SOURCES)						\
	$(INST_H_FILES)							\
	$(NOINST_H_FILES)						\
	pka-channel.c							\
	pka-config.c							\
	pka-context.c							\
	pka-encoder.c							\
	pka-listener.c							\
	pka-log.c							\
	pka-manager.c							\
	pka-manifest.c							\
	pka-plugin.c							\
	pka-sample.c							\
	pka-source.c							\
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

pka-marshal.c pka-marshal.h: pka-marshal.list
	$(GLIB_GENMARSHAL) --header --prefix=pka_marshal $(srcdir)/pka-marshal.list > $(builddir)/pka-marshal.h
	$(GLIB_GENMARSHAL) --body --prefix=pka_marshal $(srcdir)/pka-marshal.list > $(builddir)/pka-marshal.c

run: perfkit-agent
	@PERFKIT_PLUGINS_PATH=$(builddir)/sources/.libs:$(builddir)/listeners/.libs:$(builddir)/encoders/.libs \
	$(builddir)/perfkit-agent --stdout -c $(srcdir)/agent.conf

debug:
	@PERFKIT_PLUGINS_PATH=$(builddir)/sources/.libs:$(builddir)/listeners/.libs:$(builddir)/encoders/.libs \
	gdb --args $(builddir)/perfkit-agent --stdout -c $(srcdir)/agent.conf

valgrind:
	@PERFKIT_PLUGINS_PATH=$(builddir)/sources/.libs:$(builddir)/listeners/.libs:$(builddir)/encoders/.libs \
	PERFKIT_VALGRIND=1						\
	G_SLICE=always-malloc G_DEBUG=gc-friendly			\
	libtool --mode=execute valgrind --tool=memcheck			\
	--leak-check=full --leak-resolution=high			\
	--log-file=perfkit-agent.valgrind --show-reachable=yes		\
	--suppressions=$(top_builddir)/cut-n-paste/valgrind.suppression	\
	$(builddir)/perfkit-agent --stdout -c $(srcdir)/agent.conf

