/* ppg-welcome.c
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "ppg-log.h"
#include "ppg-path.h"
#include "ppg-util.h"
#include "ppg-welcome.h"

extern guint ppg_window_count (void);

typedef struct
{
	gsize       initialized;
	GtkBuilder *builder;
	GtkWidget  *window;
} PpgWelcome;

static PpgWelcome welcome = { 0 };

static gboolean
ppg_welcome_delete_event (GtkWidget *widget,    /* IN */
                          gpointer   user_data) /* IN */
{
	gtk_widget_hide(widget);
	if (!ppg_window_count()) {
		gtk_main_quit();
	}
	return TRUE;
}

static void
ppg_welcome_init (void)
{
	GtkWidget *window;
	GtkWidget *child;
	gchar *path;

	/*
	 * Create window.
	 */
	welcome.window = window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "");
	gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);

	/*
	 * Load widgets from gtk builder.
	 */
	path = ppg_path_build("ui", "ppg-welcome.ui", NULL);
	welcome.builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(welcome.builder, path, NULL)) {
		ERROR("Cannot load UI resources.");
	}
	g_free(path);

	/*
	 * Extract widgets from UI file.
	 */
	EXTRACT_WIDGET(welcome.builder, "welcome-child", child);

	/*
	 * Reparent child widget into window.
	 */
	gtk_widget_reparent(child, window);

	/*
	 * Connect signals.
	 */
	g_signal_connect(window,
	                 "delete-event",
	                 G_CALLBACK(ppg_welcome_delete_event),
	                 NULL);
}

/**
 * ppg_welcome_show:
 *
 * Shows the welcome dialog.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_welcome_show (void)
{
	if (g_once_init_enter(&welcome.initialized)) {
		ppg_welcome_init();
		g_once_init_leave(&welcome.initialized, TRUE);
	}
	gtk_widget_show(welcome.window);
}
