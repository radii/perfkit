/* pkg-welcome.c
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

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "pkg-paths.h"
#include "pkg-runtime.h"
#include "pkg-welcome.h"
#include "pkg-window.h"

G_DEFINE_TYPE(PkgWelcome, pkg_welcome, GTK_TYPE_WINDOW)

/**
 * SECTION:pkg-welcome
 * @title: PkgWelcome
 * @short_description: 
 *
 * 
 */

struct _PkgWelcomePrivate
{
	GtkBuilder *builder;
};

/**
 * pkg_welcome_new:
 *
 * Creates a new instance of #PkgWelcome.
 *
 * Return value: the newly created #PkgWelcome instance.
 */
GtkWidget*
pkg_welcome_new (void)
{
	return g_object_new(PKG_TYPE_WELCOME, NULL);
}

static gboolean
pkg_welcome_delete_event (GtkWidget *window,
                          GdkEvent  *event,
                          gpointer   data)
{
	if (!pkg_window_count_windows()) {
		pkg_runtime_quit();
	}

	return FALSE;
}

static gboolean
pkg_welcome_key_press (GtkWidget   *window,
                       GdkEventKey *event,
                       gpointer     data)
{
	/* Close the window and possibly quit if Escape is pressed. */
	if (event->keyval == GDK_Escape) {
		gtk_widget_hide(window);
		pkg_welcome_delete_event(window, NULL, NULL);
	}
}

static void
pkg_welcome_finalize (GObject *object)
{
	PkgWelcomePrivate *priv;

	priv = PKG_WELCOME(object)->priv;

	g_object_unref(priv->builder);

	G_OBJECT_CLASS(pkg_welcome_parent_class)->finalize(object);
}

static void
pkg_welcome_class_init (PkgWelcomeClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkg_welcome_finalize;
	g_type_class_add_private(object_class, sizeof(PkgWelcomePrivate));
}

static void
pkg_welcome_init (PkgWelcome *welcome)
{
	PkgWelcomePrivate *priv;
	GtkWidget *child;
	GError *error = NULL;
	gchar *path;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(welcome,
	                                   PKG_TYPE_WELCOME,
	                                   PkgWelcomePrivate);
	welcome->priv = priv;

	/* set defaults */
	gtk_window_set_title(GTK_WINDOW(welcome), "");
	gtk_window_set_default_size(GTK_WINDOW(welcome), 620, 420);
	gtk_window_set_position(GTK_WINDOW(welcome), GTK_WIN_POS_CENTER);

	/* load user interface */
	path = pkg_paths_build_data_path("ui", "welcome.ui", NULL);
	priv->builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(priv->builder, path, &error)) {
		g_error("Error loading UI resources. Check your installation.\n%s\n",
		        error->message);
	}
	g_free(path);

	/* reparent child widget into our window */
	child = GTK_WIDGET(gtk_builder_get_object(priv->builder, "welcome-child"));
	gtk_widget_reparent(child, GTK_WIDGET(welcome));

	/* connect signals */
	g_signal_connect(welcome, "delete-event",
	                 G_CALLBACK(pkg_welcome_delete_event),
	                 NULL);
	g_signal_connect(welcome, "key-press-event",
	                 G_CALLBACK(pkg_welcome_key_press),
	                 NULL);
}
