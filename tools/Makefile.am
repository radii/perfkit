bin_PROGRAMS = perfkit-shell

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)s/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LIB_DIR=\""$(libdir)"\" \
	-DG_LOG_DOMAIN=\"PerfkitTools\" \
	-DGETTEXT_PACKAGE=\"PerfkitTools\" \
	-DLOCALEDIR=\""$(localedir)"\" \
	-I$(top_srcdir)/cut-n-paste \
	-I$(top_srcdir) \
	$(PERFKIT_TOOLS_CFLAGS) \
	$(NULL)

AM_LDFLAGS = \
	$(PERFKIT_TOOLS_LIBS) \
	-lreadline \
	$(NULL)

perfkit_shell_SOURCES = \
	perfkit-shell.c \
	$(top_srcdir)/cut-n-paste/egg-line.c \
	$(NULL)

update-dbus:
	dbus-binding-tool --mode=glib-client --prefix=pk_channels \
		$(top_srcdir)/data/dbus/com.dronelabs.Perfkit.Channels.xml \
		--output=pk-channels-dbus.h
	dbus-binding-tool --mode=glib-client --prefix=pk_channel \
		$(top_srcdir)/data/dbus/com.dronelabs.Perfkit.Channel.xml \
		--output=pk-channel-dbus.h
