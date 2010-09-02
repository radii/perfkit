bin_PROGRAMS = perfkit-profiler
noinst_LTLIBRARIES = libperfkit-profiler.la

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

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)s/locale"\"	\
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\"				\
	-DPACKAGE_DATA_DIR=\""$(datadir)/perfkit"\"			\
	-DPACKAGE_LIB_DIR=\""$(libdir)"\"				\
	-DGETTEXT_PACKAGE=\""perfkit-profiler"\"			\
	-DHAVE_CONFIG_H							\
	-DG_LOG_DOMAIN=\"Profiler\"					\
	-DPERFKIT_COMPILATION						\
	-I$(top_srcdir)/cut-n-paste					\
	$(PERFKIT_PROFILER_CFLAGS)					\
	$(PERFKIT_DEBUG_CFLAGS)						\
	$(WARNINGS)							\
	$(NULL)

AM_LDFLAGS = $(PERFKIT_PROFILER_LIBS)

INST_H_FILES =								\
	perfkit-profiler.h						\
	ppg-config.h							\
	ppg-dialog.h							\
	ppg-log.h							\
	ppg-panels.h							\
	ppg-path.h							\
	ppg-session.h							\
	ppg-session-view.h						\
	ppg-welcome.h							\
	ppg-version.h							\
	$(NULL)

NOINST_H_FILES =							\
	ppg-log-window.h						\
	$(NULL)

libperfkit_profiler_la_SOURCES =					\
	$(INST_H_FILES)							\
	$(NOINST_H_FILES)						\
	$(top_srcdir)/cut-n-paste/gdatetime.c				\
	ppg-config.c							\
	ppg-dialog.c							\
	ppg-log-window.c						\
	ppg-log.c							\
	ppg-panels.c							\
	ppg-path.c							\
	ppg-session.c							\
	ppg-session-view.c						\
	ppg-welcome.c							\
	ppg-window.c							\
	$(NULL)

headerdir = $(prefix)/include/perfkit-1.0/perfkit-profiler
header_DATA = $(INST_H_FILES)

perfkit_profiler_SOURCES = main.c
perfkit_profiler_LDADD =						\
	libperfkit-profiler.la						\
	$(top_builddir)/perfkit/libperfkit-1.0.la			\
	$(NULL)
perfkit_profiler_LDFLAGS =						\
	-export-dynamic							\
	-no-undefined							\
	-export-symbols-regex "^ppg_.*"					\
	$(NULL)

run:
	@PERFKIT_PROFILER_DATA_PATH=$(top_srcdir)/data			\
	PERFKIT_CONNECTIONS_DIR=$(top_srcdir)/perfkit/connections/.libs	\
	$(builddir)/perfkit-profiler

debug:
	@$(builddir)/perfkit-profiler --help > /dev/null
	PERFKIT_PROFILER_DATA_PATH=$(top_srcdir)/data			\
	PERFKIT_CONNECTIONS_DIR=$(top_srcdir)/perfkit/connections/.libs	\
	gdb --args $(builddir)/.libs/lt-perfkit-profiler --sync

valgrind:
	PERFKIT_PROFILER_DATA_PATH=$(top_srcdir)/data			\
	G_SLICE=always-malloc						\
	PERFKIT_CONNECTIONS_DIR=$(top_srcdir)/perfkit/connections/.libs	\
	libtool --mode=execute valgrind --tool=memcheck			\
	--leak-check=full --leak-resolution=high			\
	--log-file=perfkit-agent.valgrind --show-reachable=yes		\
	--suppressions=$(top_builddir)/cut-n-paste/valgrind.suppression	\
	$(builddir)/perfkit-profiler
