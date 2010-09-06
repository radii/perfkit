bin_PROGRAMS = perfkit-profiler

AM_CFLAGS = $(PERFKIT_PROFILER_CFLAGS) \
	-DGETTEXT_PACKAGE="\"Perfkit\""

perfkit_profiler_SOURCES = \
	main.vala \
	window.vala \
	session.vala \
	welcome.vala \
	actions.vala \
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
	$(NULL)

perfkit_profiler_LDADD = $(PERFKIT_PROFILER_LIBS)
perfkit_profiler_VALAFLAGS = --pkg=gtk+-2.0 --pkg=clutter-gtk-0.10 --pkg=gdkkeysyms --vapidir=$(srcdir)/vapi

run: perfkit-profiler
	@libtool --mode=execute $(builddir)/perfkit-profiler
