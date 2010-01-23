bin_PROGRAMS = perfkit-gui

uidir = $(datadir)/perfkit-gui/ui/
ui_DATA = views/connection-view.ui

AM_CPPFLAGS = \
	$(PERFKIT_GUI_CFLAGS) \
	-DGETTEXT_PACKAGE="\"perfkit-gui\"" \
	-DLOCALE_DIR=\""$(datadir)/locale"\" \
	-DPACKAGE_DATADIR=\""$(datadir)/perfkit-gui"\" \
	$(NULL)

AM_LDFLAGS = $(PERFKIT_GUI_LIBS)

perfkit_gui_SOURCES = \
	main.vala \
	controller.vala \
	view.vala \
	runtime.vala \
	connections.vala \
	controllers/connection-controller.vala \
	views/connection-view.vala \
	views/source-types-view.vala

perfkit_gui_LDADD = \
	$(top_builddir)/perfkit/libperfkit-1.0.la

perfkit_gui_VALAFLAGS = \
	--pkg=gtk+-2.0 \
	--pkg=perfkit-1.0 \
	--pkg=config \
	--pkg=gee-1.0 \
	--thread \
	--vapidir=$(top_srcdir)/bindings/vala

test:
	PK_CONNECTIONS_DIR=$(top_builddir)/perfkit/connections/.libs \
	PERFKIT_GUI_UIDIR=$(srcdir)/views \
	$(builddir)/perfkit-gui
