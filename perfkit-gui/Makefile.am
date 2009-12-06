bin_PROGRAMS = perfkit-gui
noinst_LTLIBRARIES = libperfkit-gui.la

headerdir = $(prefix)/include/perfkit-gui-1.0/perfkit-gui
header_DATA = $(INST_H_FILES)

WARNINGS =								\
	-Wall -Werror -Wold-style-definition				\
	-Wdeclaration-after-statement					\
	-Wredundant-decls -Wmissing-noreturn				\
	-Wshadow -Wcast-align -Wwrite-strings				\
	-Winline -Wformat-nonliteral -Wformat-security			\
	-Wswitch-enum -Wswitch-default					\
	-Winit-self -Wmissing-include-dirs -Wundef			\
	-Waggregate-return -Wmissing-format-attribute			\
	-Wnested-externs

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)s/locale"\"	\
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\"				\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"				\
	-DPACKAGE_LIB_DIR=\""$(libdir)"\"				\
	-DGETTEXT_PACKAGE=\""perfkit-gui"\"				\
	-DHAVE_CONFIG_H							\
	-DG_LOG_DOMAIN=\"PerfkitGui\"					\
	$(WARNINGS)							\
	$(PERFKIT_GUI_CFLAGS)						\
	$(NULL)

INST_H_FILES =								\
	perfkit-gui.h							\
	pkg-paths.h							\
	pkg-runtime.h							\
	pkg-service.h							\
	pkg-version.h							\
	$(NULL)

NOINST_H_FILES =							\
	$(NULL)

libperfkit_gui_la_SOURCES =						\
	$(INST_H_FILES)							\
	$(NOINST_H_FILES)						\
	pkg-paths.c							\
	pkg-runtime.c							\
	pkg-service.c							\
	$(NULL)

libperfkit_gui_la_LIBADD = $(PERFKIT_GUI_LIBS)
libperfkit_gui_la_LDFLAGS =						\
	-export-dynamic							\
	-export-symbols-regex "^pkg_.*"					\
	$(NULL)

perfkit_gui_SOURCES = main.c
perfkit_gui_LDADD = libperfkit-gui.la $(PERFKIT_GUI_LIBS)
perfkit_gui_LDFLAGS =							\
	-export-dynamic							\
	-export-symbols-regex "^pkg_.*"					\
	$(NULL)
