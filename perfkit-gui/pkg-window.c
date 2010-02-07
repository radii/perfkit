/* pkg-window.c
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

#include "pkg-paths.h"
#include "pkg-window.h"
#include "pkg-runtime.h"
#include "pkg-session.h"
#include "pkg-session-view.h"

G_DEFINE_TYPE(PkgWindow, pkg_window, GTK_TYPE_WINDOW)

/**
 * SECTION:pkg-window
 * @title: PkgWindow
 * @short_description: 
 *
 * 
 */

struct _PkgWindowPrivate
{
	GtkBuilder   *builder;
	PkConnection *conn;
};

static GList *windows = NULL;

/**
 * pkg_window_new:
 *
 * Creates a new instance of #PkgWindow.
 *
 * Return value: the newly created #PkgWindow instance.
 */
GtkWidget*
pkg_window_new (void)
{
	return g_object_new(PKG_TYPE_WINDOW, NULL);
}

/**
 * pkg_window_new_for_uri:
 * @uri: A uri to a daemon
 *
 * Creates a new instance of #PkgWindow setting the connection using @uri.
 *
 * Return value: the newly created #PkgWindow instance.
 */
GtkWidget*
pkg_window_new_for_uri (const gchar *uri)
{
	PkConnection *conn;
	GtkWidget *window;

	conn = pk_connection_new_from_uri(uri);
	if (!conn) {
		return NULL;
	}

	window = pkg_window_new();
	pkg_window_set_connection(PKG_WINDOW(window), conn);
	g_object_unref(conn);

	return window;
}

gint
pkg_window_count_windows (void)
{
	return g_list_length(windows);
}

void
pkg_window_add_session (PkgWindow  *window,
                        PkgSession *session)
{
	PkgWindowPrivate *priv;
	GtkWidget *notebook,
	          *view;

	g_return_if_fail(PKG_IS_WINDOW(window));
	g_return_if_fail(!session || PKG_IS_SESSION(session));

	priv = window->priv;

	/* create new session if needed */
	if (!session) {
		session = g_object_new(PKG_TYPE_SESSION,
		                       "connection", priv->conn,
		                       NULL);
	}

	g_assert(pkg_session_get_connection(session) == priv->conn);
	notebook = (GtkWidget *)gtk_builder_get_object(priv->builder, "notebook1");
	g_assert(notebook);

	view = pkg_session_view_new();
	pkg_session_view_set_session(PKG_SESSION_VIEW(view), session);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), view,
	                         pkg_session_view_get_label(PKG_SESSION_VIEW(view)));
	gtk_widget_show(view);
}

void
pkg_window_set_connection (PkgWindow    *window,
                           PkConnection *connection)
{
	PkgWindowPrivate *priv;

	g_return_if_fail(PKG_IS_WINDOW(window));
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(!window->priv->conn);

	priv = window->priv;

	priv->conn = g_object_ref(connection);
}

static void
pkg_window_finalize (GObject *object)
{
	G_OBJECT_CLASS(pkg_window_parent_class)->finalize(object);
}

static void
pkg_window_class_init (PkgWindowClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkg_window_finalize;
	g_type_class_add_private(object_class, sizeof(PkgWindowPrivate));
}

static void
pkg_window_init (PkgWindow *window)
{
	PkgWindowPrivate *priv;
	GError *error = NULL;
	GtkWidget *child;
	gchar *path;

	/* create private data */
	priv = G_TYPE_INSTANCE_GET_PRIVATE(window,
	                                   PKG_TYPE_WINDOW,
	                                   PkgWindowPrivate);
	window->priv = priv;

	/* set defaults */
	gtk_window_set_title(GTK_WINDOW(window), _("Perfkit"));
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 550);

	/* load ui from gtkbuilder */
	priv->builder = gtk_builder_new();
	path = pkg_paths_build_data_path("ui", "perfkit.ui", NULL);
	if (!gtk_builder_add_from_file(priv->builder, path, &error)) {
		g_error("Error loading UI resources: %s", error->message);
	}
	g_free(path);

	/* reparent child widget */
	child = GTK_WIDGET(gtk_builder_get_object(priv->builder, "perfkit-child"));
	gtk_widget_reparent(child, GTK_WIDGET(window));

	windows = g_list_prepend(windows, g_object_ref(window));
}
