bin_PROGRAMS = perfkit-gui
noinst_LTLIBRARIES = libperfkit-gui.la

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
	$(NULL)

AM_LDFLAGS = $(PERFKIT_GUI_LIBS)

INST_H_FILES =								\
	perfkit-gui.h							\
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
	pkg-log.c							\
	pkg-window.c							\
	pkg-util.c							\
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
