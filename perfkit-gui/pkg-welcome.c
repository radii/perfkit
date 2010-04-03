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

#include "pkg-panels.h"
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
	GtkBuilder   *builder;
	GtkListStore *items;
};

static GtkWidget *singleton = NULL;

/**
 * pkg_welcome_new:
 *
 * Creates a new instance of #PkgWelcome.
 *
 * Return value: the newly created #PkgWelcome instance.
 */
static GtkWidget*
pkg_welcome_new (void)
{
	return g_object_new(PKG_TYPE_WELCOME, NULL);
}

GtkWidget*
pkg_welcome_get_instance (void)
{
	if (g_once_init_enter((gsize *)&singleton)) {
		g_once_init_leave((gsize *)&singleton, (gsize)pkg_welcome_new());
	}

	return singleton;
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

static gboolean
pkg_welcome_local_clicked (GtkWidget *button,
                           gpointer   user_data)
{
	GtkWidget *window;

	/* create new create new window for session */
	window = pkg_window_new_for_uri("dbus://");
	gtk_window_present(GTK_WINDOW(window));
	gtk_widget_show(window);

	/* show the sources toolboox */
	pkg_panels_show_sources();

	/* hide the welcome window */
	gtk_widget_hide(user_data);
}

static gboolean
pkg_welcome_remote_clicked (GtkWidget *widget,
                            gpointer  user_data)
{
	g_debug("Remote activated");
}

static gboolean
pkg_welcome_row_sep_func (GtkTreeModel *model,
                          GtkTreeIter  *iter,
                          gpointer      data)
{
	gboolean sep = FALSE;
	gtk_tree_model_get(model, iter, 2, &sep, -1);
	return sep;
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
	GtkWidget *child,
	          *remote_button,
	          *local_button,
	          *treeview;
	GtkCellRenderer *cpix,
	                *ctext;
	GtkTreeViewColumn *col;
	GtkTreeIter iter;
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

	child = GTK_WIDGET(gtk_builder_get_object(priv->builder, "welcome-child"));
	local_button = GTK_WIDGET(gtk_builder_get_object(priv->builder, "local-button"));
	remote_button = GTK_WIDGET(gtk_builder_get_object(priv->builder, "remote-button"));
	treeview = GTK_WIDGET(gtk_builder_get_object(priv->builder, "treeview"));

	gtk_tree_view_set_row_separator_func(GTK_TREE_VIEW(treeview),
	                                     pkg_welcome_row_sep_func,
	                                     NULL, NULL);

	/* setup list store and treeview */
	col = gtk_tree_view_column_new();
	cpix = gtk_cell_renderer_pixbuf_new();
	g_object_set(cpix,
	             "height", 40,
	             "stock-size", GTK_ICON_SIZE_DND,
	             "ypad", 3,
	             "xpad", 3,
	             NULL);
	gtk_tree_view_column_pack_start(col, cpix, FALSE);
	gtk_tree_view_column_add_attribute(col, cpix, "icon-name", 0);
	ctext = gtk_cell_renderer_text_new();
	g_object_set(ctext,
	             "xpad", 6,
	             NULL);
	gtk_tree_view_column_pack_start(col, ctext, TRUE);
	gtk_tree_view_column_add_attribute(col, ctext, "text", 1);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col);

	/* setup list store for treeview items */
	priv->items = gtk_list_store_new(3,
	                                 G_TYPE_STRING,
	                                 G_TYPE_STRING,
	                                 G_TYPE_BOOLEAN);
	gtk_tree_view_set_model(GTK_TREE_VIEW(treeview),
	                        GTK_TREE_MODEL(priv->items));
	gtk_list_store_append(priv->items, &iter);
	gtk_list_store_set(priv->items, &iter,
	                   0, GTK_STOCK_HOME,
	                   1, _("Home"),
	                   2, FALSE, -1);
	gtk_tree_selection_select_iter(
			gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)),
			&iter);
	gtk_list_store_append(priv->items, &iter);
	gtk_list_store_set(priv->items, &iter, 2, TRUE, -1);

	/* reparent */
	gtk_widget_unparent(child);
	gtk_container_add(GTK_CONTAINER(welcome), child);

	/* set focus */
	gtk_widget_set_can_default(local_button, TRUE);
	gtk_window_set_default(GTK_WINDOW(welcome), local_button);
	gtk_window_set_focus(GTK_WINDOW(welcome), local_button);

	/* connect signals */
	g_signal_connect(welcome, "delete-event",
	                 G_CALLBACK(pkg_welcome_delete_event),
	                 NULL);
	g_signal_connect(welcome, "key-press-event",
	                 G_CALLBACK(pkg_welcome_key_press),
	                 NULL);
	g_signal_connect(local_button, "clicked",
	                 G_CALLBACK(pkg_welcome_local_clicked),
	                 welcome);
	g_signal_connect(remote_button, "clicked",
	                 G_CALLBACK(pkg_welcome_remote_clicked),
	                 NULL);
}
