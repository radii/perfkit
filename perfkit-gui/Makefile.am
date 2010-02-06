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
	-DPERFKIT_COMPILATION						\
	$(PERFKIT_GUI_CFLAGS)						\
	$(NULL)

AM_LDFLAGS = $(PERFKIT_GUI_LIBS)

INST_H_FILES =								\
	perfkit-gui.h							\
	pkg-panels.h							\
	pkg-paths.h							\
	pkg-prefs.h							\
	pkg-runtime.h							\
	pkg-welcome.h							\
	pkg-window.h							\
	pkg-version.h							\
	$(NULL)

libperfkit_gui_la_SOURCES =						\
	$(INST_H_FILES)							\
	pkg-panels.c							\
	pkg-paths.c							\
	pkg-prefs.c							\
	pkg-runtime.c							\
	pkg-welcome.c							\
	pkg-window.c							\
	$(NULL)

headerdir = $(prefix)/include/perfkit-1.0/perfkit-gui
header_DATA = $(INST_H_FILES)

perfkit_gui_SOURCES = main.c
perfkit_gui_LDADD = libperfkit-gui.la
perfkit_gui_LDFLAGS =							\
	-export-dynamic							\
	-no-undefined							\
	-export-symbols-regex "^pkg_.*"					\
	$(NULL)

test:
	PERFKIT_GUI_DATA_PATH=$(top_srcdir)/data $(builddir)/perfkit-gui
