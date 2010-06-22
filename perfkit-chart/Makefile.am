-include $(INTROSPECTION_MAKEFILE)

lib_LTLIBRARIES = libperfkit-chart-1.0.la
noinst_PROGRAMS = perfkit-chart

headerdir = $(prefix)/include/perfkit-1.0/perfkit-chart
header_DATA = $(INST_H_FILES)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = perfkit-chart-1.0.pc

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

BUILD_SOURCES = pkc-version.h

INST_H_FILES =								\
	$(BUILD_SOURCES)						\
	perfkit-chart.h							\
	pkc-scatter.h							\
	$(NULL)

NOINST_H_FILES =							\
	pkc-log.h							\
	$(NULL)

INCLUDES =								\
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)s/locale"\"	\
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\"				\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"				\
	-DPACKAGE_LIB_DIR=\""$(libdir)"\"				\
	-DGETTEXT_PACKAGE=\""perfkit-chart"\"				\
	-DG_LOG_DOMAIN=\"PerfkitChart\"					\
	-DPERFKIT_CHART_COMPILATION					\
	-I$(top_srcdir)/cut-n-paste					\
	$(WARNINGS)							\
	$(PERFKIT_CHART_CFLAGS)						\
	$(PERFKIT_DEBUG_CFLAGS)						\
	$(NULL)

libperfkit_chart_1_0_la_SOURCES =					\
	$(INST_H_FILES)							\
	$(NOINST_H_FILES)						\
	pkc-scatter.c							\
	$(NULL)
libperfkit_chart_1_0_la_CPPFLAGS = $(INCLUDES)
libperfkit_chart_1_0_la_LIBADD =					\
	$(PERFKIT_CHART_LIBS)						\
	$(NULL)

perfkit_chart_SOURCES =							\
	main.c								\
	$(NULL)
perfkit_chart_CPPFLAGS = $(INCLUDES)
perfkit_chart_LDADD =							\
	libperfkit-chart-1.0.la						\
	$(NULL)

INTROSPECTION_GIRS =
CLEANFILES =
DISTCLEANFILES = $(BUILD_SOURCES)

if HAVE_INTROSPECTION
introspection_files = $(INST_H_FILES) pkc-*.c

Perfkit-Chart-1.0.gir: $(INTROSPECTION_SCANNER) libperfkit-chart-1.0.la Makefile
Perfkit_Chart_1_0_gir_SCANNERFLAGS = --strip-prefix=Pkc
Perfkit_Chart_1_0_gir_INCLUDES = Gio-2.0 Gtk-2.0
Perfkit_Chart_1_0_gir_CFLAGS = $(PERFKIT_CHART_CFLAGS) -DPERFKIT_CHART_COMPILATION
Perfkit_Chart_1_0_gir_LIBS = libperfkit-chart-1.0.la
Perfkit_Chart_1_0_gir_FILES = $(addprefix $(srcdir)/, $(introspection_files))

INTROSPECTION_GIRS += Perfkit-Chart-1.0.gir

girdir = $(datadir)/gir-1.0
dist_gir_DATA = $(INTROSPECTION_GIRS)

typelibsdir = $(libdir)/girepository-1.0
typelibs_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(dist_gir_DATA) $(typelibs_DATA)
endif

run: perfkit-chart
	./perfkit-chart
