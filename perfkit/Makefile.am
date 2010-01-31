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
	perfkit-lowlevel.h						\
	pk-channel.h							\
	pk-connection.h							\
	pk-encoder.h							\
	pk-encoder-info.h						\
	pk-manager.h							\
	pk-manifest.h							\
	pk-sample.h							\
	pk-source.h							\
	pk-source-info.h						\
	pk-subscription.h						\
	$(NULL)

NOINST_H_FILES =							\
	$(NULL)

libperfkit_1_0_la_SOURCES =						\
	$(INST_H_FILES)							\
	$(NOINST_H_FILES)						\
	pk-channel.c							\
	pk-connection.c							\
	pk-encoder.c							\
	pk-encoder-info.c						\
	pk-manager.c							\
	pk-manifest.c							\
	pk-sample.c							\
	pk-source.c							\
	pk-source-info.c						\
	pk-subscription.c						\
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

DISTCLEANFILES = $(BUILD_SOURCES)
