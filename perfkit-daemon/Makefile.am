bin_PROGRAMS = perfkit-daemon
noinst_LTLIBRARIES = libperfkit-daemon.la

headerdir = $(prefix)/include/perfkit-daemon-1.0/perfkit-daemon
header_DATA = $(INST_H_FILES)

WARNINGS = -Wall -Werror -Wold-style-definition -Wdeclaration-after-statement \
	-Wredundant-decls -Wmissing-noreturn -Wshadow -Wcast-align -Wwrite-strings \
	-Winline -Wformat-nonliteral -Wformat-security -Wswitch-enum -Wswitch-default \
	-Winit-self -Wmissing-include-dirs -Wundef -Waggregate-return \
	-Wmissing-format-attribute -Wnested-externs

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)s/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LIB_DIR=\""$(libdir)"\" \
	-DGETTEXT_PACKAGE=\""perfkit-daemon"\" \
	-DHAVE_CONFIG_H \
	-DG_LOG_DOMAIN=\"PerfkitDaemon\" \
	$(WARNINGS) \
	$(PERFKIT_DAEMON_CFLAGS) \
	$(NULL)

INST_H_FILES = \
	pkd-version.h \
	pkd-sample.h \
	pkd-channel.h \
	pkd-channels.h \
	pkd-service.h \
	pkd-runtime.h \
	pkd-paths.h \
	pkd-source.h \
	pkd-source-simple.h \
	pkd-sources.h \
	pkd-source-info.h \
	perfkit-daemon.h \
	$(NULL)

NOINST_H_FILES = \
	pkd-log.h \
	pkd-channel-priv.h \
	$(NULL)

libperfkit_daemon_la_SOURCES = \
	$(INST_H_FILES) \
	$(NOINST_H_FILES) \
	pkd-sample.c \
	pkd-channel.c \
	pkd-channels.c \
	pkd-log.c \
	pkd-service.c \
	pkd-runtime.c \
	pkd-paths.c \
	pkd-source.c \
	pkd-source-simple.c \
	pkd-sources.c \
	pkd-source-info.c \
	$(NULL)

libperfkit_daemon_la_LIBADD = $(PERFKIT_DAEMON_LIBS)
libperfkit_daemon_la_LDFLAGS = \
	-export-dynamic \
	-export-symbols-regex "^pkd_.*" \
	$(NULL)

perfkit_daemon_SOURCES = main.c
perfkit_daemon_LDADD = libperfkit-daemon.la $(PERFKIT_DAEMON_LIBS)
perfkit_daemon_LDFLAGS = \
	-export-dynamic \
	-export-symbols-regex "^pkd_.*" \
	$(NULL)

update-dbus:
	dbus-binding-tool --mode=glib-server --prefix=pkd_channels --output=pkd-channels-dbus.h \
		$(top_srcdir)/data/dbus/com.dronelabs.Perfkit.Channels.xml
	dbus-binding-tool --mode=glib-server --prefix=pkd_channel --output=pkd-channel-dbus.h \
		$(top_srcdir)/data/dbus/com.dronelabs.Perfkit.Channel.xml
	dbus-binding-tool --mode=glib-server --prefix=pkd_sources --output=pkd-sources-dbus.h \
		$(top_srcdir)/data/dbus/com.dronelabs.Perfkit.Sources.xml
	dbus-binding-tool --mode=glib-server --prefix=pkd_source --output=pkd-source-dbus.h \
		$(top_srcdir)/data/dbus/com.dronelabs.Perfkit.Source.xml

test:
	PERFKIT_PLUGIN_DIR=$(top_builddir)/plugins/.libs $(builddir)/perfkit-daemon --stdout
