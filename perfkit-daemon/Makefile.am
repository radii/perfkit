#
# Perfkit Daemon
#

SUBDIRS = . sources encoders listeners

noinst_LTLIBRARIES = libperfkit-daemon.la
bin_PROGRAMS = perfkit-daemon

headerdir = $(prefix)/include/perfkit-daemon-1.0/perfkit-daemon
header_DATA = $(INST_H_FILES)

etcdir = $(sysconfdir)/perfkit
etc_DATA = daemon.conf

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
	-DGETTEXT_PACKAGE=\""perfkit-daemon"\"				\
	-DPERFKIT_COMPILATION						\
	-DHAVE_CONFIG_H							\
	-DG_LOG_DOMAIN=\"PerfkitDaemon\"				\
	-I$(top_srcdir)/cut-n-paste					\
	$(WARNINGS)							\
	$(PERFKIT_DAEMON_CFLAGS)					\
	$(NULL)

INST_H_FILES =								\
	pkd-channel.h							\
	pkd-config.h							\
	pkd-encoder.h							\
	pkd-encoder-info.h						\
	pkd-listener.h							\
	pkd-manifest.h							\
	pkd-pipeline.h							\
	pkd-sample.h							\
	pkd-source.h							\
	pkd-source-info.h						\
	pkd-spawn-info.h						\
	pkd-subscription.h						\
	pkd-version.h							\
	perfkit-daemon.h						\
	$(NULL)

NOINST_H_FILES =							\
	pkd-log.h							\
	pkd-plugins.h							\
	$(NULL)

libperfkit_daemon_la_SOURCES =						\
	$(INST_H_FILES)							\
	$(NOINST_H_FILES)						\
	pkd-channel.c							\
	pkd-config.c							\
	pkd-encoder.c							\
	pkd-encoder-info.c						\
	pkd-listener.c							\
	pkd-log.c							\
	pkd-manifest.c							\
	pkd-pipeline.c							\
	pkd-plugins.c							\
	pkd-sample.c							\
	pkd-source.c							\
	pkd-source-info.c						\
	pkd-subscription.c						\
	$(top_srcdir)/cut-n-paste/egg-buffer.c				\
	$(top_srcdir)/cut-n-paste/gdatetime.c				\
	$(NULL)

libperfkit_daemon_la_LIBADD = $(PERFKIT_DAEMON_LIBS)
libperfkit_daemon_la_LDFLAGS =						\
	-export-dynamic							\
	-export-symbols-regex "^pkd_.*"					\
	$(NULL)

perfkit_daemon_SOURCES = main.c
perfkit_daemon_LDADD = libperfkit-daemon.la $(PERFKIT_DAEMON_LIBS)
perfkit_daemon_LDFLAGS =						\
	-export-dynamic							\
	-export-symbols-regex "^pkd_.*"					\
	$(NULL)

test:
	PERFKIT_SOURCES_DIR=$(builddir)/sources/.libs			\
	PERFKIT_LISTENERS_DIR=$(builddir)/listeners/.libs		\
	PERFKIT_ENCODERS_DIR=$(builddir)/encoders/.libs			\
	$(builddir)/perfkit-daemon --stdout -c $(srcdir)/daemon.conf
