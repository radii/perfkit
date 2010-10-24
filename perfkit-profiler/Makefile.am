bin_PROGRAMS = perfkit-profiler

headerdir = $(prefix)/include/perfkit-1.0/perfkit-profiler
header_DATA = $(INST_H_FILES)

WARNINGS =								\
	-Wall								\
	-Werror								\
	-Wold-style-definition						\
	-Wdeclaration-after-statement					\
	-Wredundant-decls						\
	-Wmissing-noreturn						\
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
	-Wnested-externs						\
	-Wshadow

AM_CFLAGS =								\
	$(PERFKIT_PROFILER_CFLAGS)					\
	-DPRODUCT_NAME="\"Perfkit Profiler\""				\
	-DGETTEXT_PACKAGE="\"Perfkit\""					\
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)s/locale"\"	\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"				\
	-DG_LOG_DOMAIN=\""Profiler"\"					\
	-I$(top_srcdir)/perfkit-profiler/				\
	-I$(top_srcdir)/uber/						\
	$(WARNINGS)							\
	$(NULL)

AM_LDFLAGS =								\
	$(PERFKIT_PROFILER_LIBS)					\
	$(NULL)

BUILD_SOURCES = ppg-version.h

INST_H_FILES =								\
	$(BUILD_SOURCES)						\
	perfkit-profiler.h						\
	ppg-about-dialog.h						\
	ppg-actions.h							\
	ppg-add-instrument-dialog.h					\
	ppg-color.h							\
	ppg-configure-instrument-dialog.h				\
	ppg-edit-channel-task.h						\
	ppg-header.h							\
	ppg-instrument.h						\
	ppg-instruments.h						\
	ppg-line-visualizer.h						\
	ppg-log.h							\
	ppg-memory-instrument.h						\
	ppg-menu-tool-item.h						\
	ppg-model.h							\
	ppg-monitor.h							\
	ppg-paths.h							\
	ppg-prefs.h							\
	ppg-prefs-dialog.h						\
	ppg-restart-task.h						\
	ppg-row.h							\
	ppg-ruler.h							\
	ppg-runtime.h							\
	ppg-session.h							\
	ppg-session-task.h						\
	ppg-sessions-store.h						\
	ppg-settings-dialog.h						\
	ppg-spawn-process-dialog.h					\
	ppg-status-actor.h						\
	ppg-task.h							\
	ppg-timer-tool-item.h						\
	ppg-util.h							\
	ppg-visualizer.h						\
	ppg-visualizer-menu.h						\
	ppg-welcome-dialog.h						\
	ppg-window.h							\
	$(NULL)

perfkit_profiler_SOURCES =						\
	$(NOINST_H_FILES)						\
	$(INST_H_FILES)							\
	main.c								\
	ppg-about-dialog.c						\
	ppg-actions.c							\
	ppg-color.c							\
	ppg-configure-instrument-dialog.c				\
	ppg-add-instrument-dialog.c					\
	ppg-edit-channel-task.c						\
	ppg-header.c							\
	ppg-instrument.c						\
	ppg-instruments.c						\
	ppg-line-visualizer.c						\
	ppg-log.c							\
	ppg-memory-instrument.c						\
	ppg-menu-tool-item.c						\
	ppg-model.c							\
	ppg-monitor.c							\
	ppg-paths.c							\
	ppg-prefs.c							\
	ppg-prefs-dialog.c						\
	ppg-restart-task.c						\
	ppg-row.c							\
	ppg-ruler.c							\
	ppg-runtime.c							\
	ppg-session.c							\
	ppg-session-task.c						\
	ppg-sessions-store.c						\
	ppg-settings-dialog.c						\
	ppg-spawn-process-dialog.c					\
	ppg-status-actor.c						\
	ppg-task.c							\
	ppg-timer-tool-item.c						\
	ppg-util.c							\
	ppg-visualizer.c						\
	ppg-visualizer-menu.c						\
	ppg-welcome-dialog.c						\
	ppg-window.c							\
	ppg-window-ui.h							\
	$(NULL)

perfkit_profiler_LDADD =						\
	$(top_builddir)/perfkit/libperfkit-1.0.la			\
	$(top_builddir)/uber/libuber.la					\
	$(NULL);

run: perfkit-profiler
	@PPG_DATA_DIR=$(top_builddir)/data \
	PERFKIT_CONNECTIONS_DIR=$(top_builddir)/perfkit/connections/.libs \
	GSETTINGS_SCHEMA_DIR=$(top_builddir)/data/settings \
	libtool --mode=execute $(builddir)/perfkit-profiler

debug: perfkit-profiler
	@PPG_DATA_DIR=$(top_builddir)/data \
	PERFKIT_CONNECTIONS_DIR=$(top_builddir)/perfkit/connections/.libs \
	GSETTINGS_SCHEMA_DIR=$(top_builddir)/data/settings \
	libtool --mode=execute gdb $(builddir)/perfkit-profiler
