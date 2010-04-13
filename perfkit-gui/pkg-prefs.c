/* pkg-prefs.c
 *
 * Copyright (C) 2010 Christian Hergert
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <clutter/clutter.h>
#include <glib/gi18n.h>
#include <ethos/ethos-ui.h>
#include <gtk/gtk.h>

#include "pkg-prefs.h"
#include "pkg-util.h"

typedef struct
{
	GOptionContext *context;
	GtkWidget      *window;
	GtkBuilder     *builder;
} PkgPrefs;

static GOptionEntry entries[] = {
	{ NULL }
};

static PkgPrefs prefs;

static gboolean
pkg_prefs_window_delete_event (GtkWidget *widget)
{
	gtk_widget_hide(widget);
	return TRUE;
}

static void
pkg_prefs_create_window (void)
{
	GtkWidget *window;
	GtkWidget *plugins;
	GObject *alignment;
	GObject *close_;

	/*
	 * Create the window.
	 */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), _("Perfkit Preferences"));
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_UTILITY);
	gtk_widget_set_size_request(window, 400, 480);
	gtk_container_set_border_width(GTK_CONTAINER(window), 12);

	/*
	 * Reparent the children widgets inside our window.
	 */
	prefs.window = window;
	prefs.builder = pkg_util_get_builder("prefs.ui");
	pkg_util_reparent(prefs.builder, "child", window);

	/*
	 * Add the plugins widget.
	 */
	plugins = ethos_ui_manager_widget_new();
	alignment = gtk_builder_get_object(prefs.builder, "plugins_align");
	gtk_container_add(GTK_CONTAINER(alignment), plugins);
	gtk_widget_show(plugins);

	/*
	 * Hide the window when close is clicked.
	 */
	close_ = gtk_builder_get_object(prefs.builder, "close");
	g_signal_connect_swapped(close_,
	                         "clicked",
	                         G_CALLBACK(gtk_widget_hide),
	                         window);

	g_signal_connect(window,
	                 "delete-event",
	                 G_CALLBACK(pkg_prefs_window_delete_event),
	                 NULL);
}

/**
 * pkg_prefs_init:
 * @argc: argument count
 * @argv: vector containing arguments
 * @error: location for a #GError or %NULL.
 *
 * Initializes the preferences subsystem using the arguments from the
 * command line.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 */
gboolean
pkg_prefs_init (gint     *argc,
                gchar  ***argv,
                GError  **error)
{
	/*
	 * Initialize preferences state.
	 */
	memset(&prefs, 0, sizeof(prefs));

	/*
	 * Parse runtime arguments.
	 */
	prefs.context = g_option_context_new(_("- A Perfkit Gui"));
	g_option_context_add_main_entries(prefs.context, entries, GETTEXT_PACKAGE);
	g_option_context_add_group(prefs.context, gtk_get_option_group(TRUE));
	g_option_context_add_group(prefs.context, clutter_get_option_group());
	if (!g_option_context_parse(prefs.context, argc, argv, error)) {
		return FALSE;
	}

	/*
	 * Create preferences window.
	 */
	pkg_prefs_create_window();

	return TRUE;
}

/**
 * pkg_prefs_shutdown:
 *
 * Shuts down the preferences subsystem.
 *
 * Returns: None.
 * Side effects: Everything.
 */
void
pkg_prefs_shutdown (void)
{
	g_option_context_free(prefs.context);
}

/**
 * pkg_prefs_show:
 *
 * Shows the preferences window and brings it to the front.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pkg_prefs_show (void)
{
	gtk_window_set_position(GTK_WINDOW(prefs.window), GTK_WIN_POS_CENTER);
	gtk_widget_show(prefs.window);
	gtk_window_present(GTK_WINDOW(prefs.window));
}

/**
 * pkg_prefs_get_window_size:
 * @width: A location for the width.
 * @height: A location for the height.
 *
 * Determines the preferred window size for a new #PkgWindow.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pkg_prefs_get_window_size (gint *width,
                           gint *height)
{
	if (width != NULL) {
		*width = 800;
	}

	if (height != NULL) {
		*height = 550;
	}
}
