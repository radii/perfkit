-include $(INTROSPECTION_MAKEFILE)

SUBDIRS = . connections

lib_LTLIBRARIES = libperfkit-1.0.la

headerdir = $(prefix)/include/perfkit-1.0/perfkit
header_DATA = $(INST_H_FILES)

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

BUILD_SOURCES = pk-version.h

INST_H_FILES =								\
	$(BUILD_SOURCES)						\
	perfkit.h							\
	pk-connection.h							\
	pk-connection-lowlevel.h					\
	pk-manifest.h							\
	pk-sample.h							\
	$(NULL)

NOINST_H_FILES =							\
	pk-util.h							\
	$(NULL)

libperfkit_1_0_la_SOURCES =						\
	$(INST_H_FILES)							\
	$(NOINST_H_FILES)						\
	pk-connection.c							\
	pk-manifest.c							\
	pk-sample.c							\
	$(top_srcdir)/cut-n-paste/egg-buffer.c				\
	$(NULL)

libperfkit_1_0_la_CPPFLAGS =						\
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)s/locale"\"	\
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\"				\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"				\
	-DPACKAGE_LIB_DIR=\""$(libdir)"\"				\
	-DGETTEXT_PACKAGE=\""perfkit"\"					\
	-DHAVE_CONFIG_H							\
	-DG_LOG_DOMAIN=\"Perfkit\"					\
	-DPERFKIT_COMPILATION						\
	-I$(top_srcdir)/cut-n-paste					\
	$(WARNINGS)							\
	$(PERFKIT_CFLAGS)						\
	$(NULL)

INTROSPECTION_GIRS =
CLEANFILES =
DISTCLEANFILES = $(BUILD_SOURCES)

if HAVE_INTROSPECTION
introspection_files = $(INST_H_FILES) pk-*.c

Perfkit-1.0.gir: $(INTROSPECTION_SCANNER) libperfkit-1.0.la Makefile
Perfkit_1_0_gir_SCANNERFLAGS = --strip-prefix=Pk
Perfkit_1_0_gir_INCLUDES = Gio-2.0
Perfkit_1_0_gir_CFLAGS = $(INCLUDES)
Perfkit_1_0_gir_LIBS = libperfkit-1.0.la
Perfkit_1_0_gir_FILES = $(addprefix $(srcdir)/, $(introspection_files))

INTROSPECTION_GIRS += Perfkit-1.0.gir

girdir = $(datadir)/gir-1.0
dist_gir_DATA = $(INTROSPECTION_GIRS)

typelibsdir = $(libdir)/girepository-1.0
typelibs_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(dist_gir_DATA) $(typelibs_DATA)
endif
