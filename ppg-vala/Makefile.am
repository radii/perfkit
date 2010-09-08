bin_PROGRAMS = perfkit-profiler

AM_CFLAGS = \
	$(PERFKIT_PROFILER_CFLAGS) \
	-DGETTEXT_PACKAGE="\"Perfkit\"" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(NULL)

perfkit_profiler_SOURCES = \
	main.vala \
	window.vala \
	session.vala \
	welcome.vala \
	actions.vala \
	paths.vala \
	actions/edit-action.vala \
	actions/preferences-action.vala \
	actions/add-source-action.vala \
	actions/remove-source-action.vala \
	actions/quit-action.vala \
	actions/perfkit-action.vala \
	actions/profile-action.vala \
	actions/stop-action.vala \
	actions/pause-action.vala \
	actions/run-action.vala \
	actions/restart-action.vala \
	dialogs/add-source-dialog.vala \
	dialogs/preferences-dialog.vala \
	widgets/cairo-util.vala \
	widgets/header.vala \
	widgets/ruler.vala \
	$(NULL)

perfkit_profiler_LDADD = \
	$(PERFKIT_PROFILER_LIBS) \
	$(top_builddir)/perfkit/libperfkit-1.0.la \
	$(NULL)

perfkit_profiler_VALAFLAGS = \
	--girdir=$(top_builddir)/perfkit \
	--pkg=gtk+-2.0 \
	--pkg=clutter-gtk-0.10 \
	--pkg=gdkkeysyms \
	--pkg=Perfkit-1.0 \
	--pkg=config \
	--vapidir=$(srcdir)/vapi \
	$(NULL)

run: perfkit-profiler
	@PPG_DATA_DIR=$(top_builddir)/data \
	PERFKIT_CONNECTIONS_DIR=$(top_builddir)/perfkit/connections/.libs \
	libtool --mode=execute $(builddir)/perfkit-profiler

debug:
	@PPG_DATA_DIR=$(top_builddir)/data \
	PERFKIT_CONNECTIONS_DIR=$(top_builddir)/perfkit/connections/.libs \
	libtool --mode=execute gdb $(builddir)/perfkit-profiler
