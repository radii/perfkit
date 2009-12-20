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
	-DPACKAGE_SYSCONFDIR=\""$(sysconfdir)"\" \
	-DHAVE_CONFIG_H \
	-DG_LOG_DOMAIN=\"PerfkitDaemon\" \
	-I$(srcdir)/dbus \
	$(WARNINGS) \
	$(PERFKIT_DAEMON_CFLAGS) \
	$(NULL)

INST_H_FILES = \
	pkd-channel.h \
	pkd-channels.h \
	pkd-config.h \
	pkd-listener.h \
	pkd-paths.h \
	pkd-runtime.h \
	pkd-sample.h \
	pkd-service.h \
	pkd-source.h \
	pkd-source-info.h \
	pkd-source-simple.h \
	pkd-sources.h \
	pkd-version.h \
	perfkit-daemon.h \
	$(NULL)

NOINST_H_FILES = \
	pkd-log.h \
	pkd-channel-priv.h \
	pkd-listener-dbus.h \
	$(NULL)

libperfkit_daemon_la_SOURCES = \
	$(INST_H_FILES) \
	$(NOINST_H_FILES) \
	pkd-channel.c \
	pkd-channels.c \
	pkd-config.c \
	pkd-listener.c \
	pkd-listener-dbus.c \
	pkd-log.c \
	pkd-paths.c \
	pkd-runtime.c \
	pkd-sample.c \
	pkd-service.c \
	pkd-source.c \
	pkd-source-info.c \
	pkd-source-simple.c \
	pkd-sources.c \
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

$(srcdir)/dbus/pkd-channels-dbus.h: $(top_srcdir)/data/dbus/com.dronelabs.Perfkit.Channels.xml
	dbus-binding-tool --mode=glib-server --prefix=pkd_channels --output=$@ $<
$(srcdir)/dbus/pkd-channel-dbus.h: $(top_srcdir)/data/dbus/com.dronelabs.Perfkit.Channel.xml
	dbus-binding-tool --mode=glib-server --prefix=pkd_channel --output=$@ $<
$(srcdir)/dbus/pkd-source-dbus.h: $(top_srcdir)/data/dbus/com.dronelabs.Perfkit.Source.xml
	dbus-binding-tool --mode=glib-server --prefix=pkd_source --output=$@ $<
$(srcdir)/dbus/pkd-sources-dbus.h: $(top_srcdir)/data/dbus/com.dronelabs.Perfkit.Sources.xml
	dbus-binding-tool --mode=glib-server --prefix=pkd_sources --output=$@ $<
$(srcdir)/dbus/pkd-source-info-dbus.h: $(top_srcdir)/data/dbus/com.dronelabs.Perfkit.SourceInfo.xml
	dbus-binding-tool --mode=glib-server --prefix=pkd_source_info --output=$@ $<
 
test:
	PERFKIT_PLUGIN_DIR=$(top_builddir)/plugins/.libs $(builddir)/perfkit-daemon --stdout
