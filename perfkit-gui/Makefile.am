bin_PROGRAMS = perfkit-gui
noinst_LTLIBRARIES = libperfkit-gui.la

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
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"				\
	-DPACKAGE_LIB_DIR=\""$(libdir)"\"				\
	-DGETTEXT_PACKAGE=\""pkg"\"					\
	-DHAVE_CONFIG_H							\
	-DG_LOG_DOMAIN=\"PerfkitGui\"					\
	-DPERFKIT_GUI_COMPILATION					\
	-I$(top_srcdir)/cut-n-paste					\
	$(PERFKIT_GUI_CFLAGS)						\
	$(PERFKIT_DEBUG_CFLAGS)						\
	$(WARNINGS)							\
	$(NULL)

AM_LDFLAGS = $(PERFKIT_GUI_LIBS)

INST_H_FILES =								\
	perfkit-gui.h							\
	pkg-channel-page.h						\
	pkg-channels-page.h						\
	pkg-closures.h							\
	pkg-page.h							\
	pkg-path.h							\
	pkg-plugin-page.h						\
	pkg-source-page.h						\
	pkg-subscription-page.h						\
	pkg-version.h							\
	$(NULL)

NOINST_H_FILES =							\
	pkg-log.h							\
	pkg-util.h							\
	$(NULL)

libperfkit_gui_la_SOURCES =						\
	$(INST_H_FILES)							\
	$(NOINST_H_FILES)						\
	$(top_srcdir)/cut-n-paste/gdatetime.c				\
	pkg-channel-page.c						\
	pkg-channels-page.c						\
	pkg-closures.c							\
	pkg-log.c							\
	pkg-page.c							\
	pkg-path.c							\
	pkg-plugin-page.c						\
	pkg-source-page.c						\
	pkg-subscription-page.c						\
	pkg-util.c							\
	pkg-window.c							\
	$(NULL)

headerdir = $(prefix)/include/perfkit-1.0/perfkit-gui
header_DATA = $(INST_H_FILES)

perfkit_gui_SOURCES = main.c
perfkit_gui_LDADD =							\
	libperfkit-gui.la						\
	$(top_builddir)/perfkit/libperfkit-1.0.la			\
	$(NULL)
perfkit_gui_LDFLAGS =							\
	-export-dynamic							\
	-no-undefined							\
	-export-symbols-regex "^pkg_.*"					\
	$(NULL)

run:
	PERFKIT_GUI_DATA_PATH=$(top_srcdir)/data			\
	PERFKIT_CONNECTIONS_DIR=$(top_srcdir)/perfkit/connections/.libs	\
	$(builddir)/perfkit-gui

debug:
	$(builddir)/perfkit-gui --help > /dev/null
	PERFKIT_GUI_DATA_PATH=$(top_srcdir)/data			\
	PERFKIT_CONNECTIONS_DIR=$(top_srcdir)/perfkit/connections/.libs	\
	gdb --args $(builddir)/.libs/lt-perfkit-gui --sync
