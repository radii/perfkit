dbus_LTLIBRARIES = libdbus.la
dbusdir = $(libdir)/perfkit/connections

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
	-DGETTEXT_PACKAGE=\""perfkit"\"					\
	-DHAVE_CONFIG_H							\
	-DG_LOG_DOMAIN=\"Perfkit\"					\
	-I$(srcdir)/dbus						\
	$(WARNINGS)							\
	$(PERFKIT_CFLAGS)						\
	$(DBUS_CFLAGS)							\
	$(NULL)

NOINST_H_FILES =							\
	pk-dbus.h							\
	$(NULL)

libdbus_la_SOURCES =							\
	$(NOINST_H_FILES)						\
	pk-dbus.c							\
	$(NULL)

libdbus_la_LIBADD =							\
	$(PERFKIT_LIBS)							\
	$(DBUS_LIBS)							\
	$(top_builddir)/perfkit/libperfkit-1.0.la			\
	$(NULL)

libdbus_la_LDFLAGS =							\
	-export-dynamic							\
	-export-symbols-regex "^pk_.*"					\
	-module								\
	$(NULL)

