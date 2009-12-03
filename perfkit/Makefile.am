lib_LTLIBRARIES = libperfkit-1.0.la

headerdir = $(prefix)/include/perfkit-1.0/perfkit
header_DATA = $(INST_H_FILES)

WARNINGS = \
	-Wall -Werror -Wold-style-definition -Wdeclaration-after-statement \
	-Wredundant-decls -Wmissing-noreturn -Wshadow -Wcast-align  \
	-Wwrite-strings -Winline -Wformat-nonliteral -Wformat-security \
	-Wswitch-enum -Wswitch-default -Winit-self -Wmissing-include-dirs \
	-Wundef -Waggregate-return -Wmissing-format-attribute \
	-Wnested-externs

BUILD_SOURCES = pk-version.h

INST_H_FILES = \
	$(BUILD_SOURCES) \
	perfkit.h \
	pk-channel.h \
	pk-channels.h \
	pk-connection.h \
	$(NULL)

NOINST_H_FILES = \
	pk-connection-dbus.h \
	pk-channel-priv.h \
	pk-channels-priv.h \
	$(NULL)

libperfkit_1_0_la_SOURCES = \
	$(INST_H_FILES) \
	$(NOINST_H_FILES) \
	pk-channel.c \
	pk-channels.c \
	pk-connection.c \
	pk-connection-dbus.c \
	$(NULL)

libperfkit_1_0_la_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)s/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LIB_DIR=\""$(libdir)"\" \
	-DGETTEXT_PACKAGE=\""perfkit"\" \
	-DHAVE_CONFIG_H \
	-DG_LOG_DOMAIN=\"Perfkit\" \
	$(WARNINGS) \
	$(PERFKIT_CFLAGS) \
	$(NULL)

DISTCLEANFILES = $(BUILD_SOURCES)

update-dbus:
	dbus-binding-tool --mode=glib-client --prefix=pk_channels \
		$(top_srcdir)/data/dbus/com.dronelabs.Perfkit.Channels.xml \
		--output=pk-channels-dbus.h
	dbus-binding-tool --mode=glib-client --prefix=pk_channel \
		$(top_srcdir)/data/dbus/com.dronelabs.Perfkit.Channel.xml \
		--output=pk-channel-dbus.h
