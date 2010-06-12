/* pkg-window.c
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <perfkit/perfkit.h>

#include "pkg-log.h"
#include "pkg-window.h"

#define STR_OR_EMPTY(_s) ((_s) ? (_s) : "")

G_DEFINE_TYPE(PkgWindow, pkg_window, GTK_TYPE_WINDOW)

/**
 * SECTION:pkg-window
 * @title: PkgWindow
 * @short_description: 
 *
 * Section overview.
 */

struct _PkgWindowPrivate
{
	GStaticRWLock  rw_lock;   /* Synchronization */
	GPtrArray     *ops;       /* Active asynchronous operations */
	GtkWidget     *treeview;  /* Sidebar treeview */
	GtkTreeStore  *model;     /* Sidebar treeview data */
	GtkWidget     *container; /* Page container */
};

/**
 * pkg_window_new:
 *
 * Creates a new instance of #PkgWindow.
 *
 * Returns: the newly created instance of #PkgWindow.
 * Side effects: None.
 */
GtkWidget*
pkg_window_new (void)
{
	return g_object_new(PKG_TYPE_WINDOW, NULL);
}

static void
pkg_window_connection_manager_get_hostname_cb (GObject      *object,    /* IN */
                                               GAsyncResult *result,    /* IN */
                                               gpointer      user_data) /* IN */
{
	PkConnection *connection = PK_CONNECTION(object);
	PkgWindowPrivate *priv;
	PkConnection *iter_conn;
	gchar *hostname = NULL;
	GError *error = NULL;
	GtkTreeIter iter;

	g_return_if_fail(PKG_IS_WINDOW(user_data));

	ENTRY;
	priv = PKG_WINDOW(user_data)->priv;
	if (!pk_connection_manager_get_hostname_finish(connection, result,
	                                               &hostname, &error)) {
	    WARNING(Connection, "Error retrieving hostname: %s", error->message);
	    g_error_free(error);
	    EXIT;
	}
	if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->model), &iter)) {
		/* item removed? */
		g_free(hostname);
		EXIT;
	}
	do {
		gtk_tree_model_get(GTK_TREE_MODEL(priv->model), &iter, 0, &iter_conn, -1);
		if (iter_conn == connection) {
			gtk_tree_store_set(priv->model, &iter, 1, hostname, 2, "Some sort of info here", -1);
			BREAK;
		}
	} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->model), &iter));
	EXIT;
}

static void
pkg_window_connection_connect_cb (GObject      *object,
                                  GAsyncResult *result,
                                  gpointer      user_data)
{
	PkConnection *connection = PK_CONNECTION(object);
	GError *error = NULL;

	g_return_if_fail(PKG_IS_WINDOW(user_data));

	ENTRY;
	DEBUG(Connection, "Established connection to agent.");
	if (!pk_connection_connect_finish(connection, result, &error)) {
		WARNING(Connection, "Error connecting to agent: %s", error->message);
		g_error_free(error);
		EXIT;
	}
	pk_connection_manager_get_hostname_async(
			connection, NULL,
	        pkg_window_connection_manager_get_hostname_cb,
	        user_data);
	EXIT;
}

gboolean
pkg_window_connect_to (PkgWindow   *window,
                       const gchar *uri)
{
	PkgWindowPrivate *priv;
	PkConnection *connection;
	GtkTreeIter iter;

	g_return_if_fail(PKG_IS_WINDOW(window));

	ENTRY;
	priv = window->priv;
	if (!(connection = pk_connection_new_from_uri(uri))) {
		RETURN(FALSE);
	}
	gtk_tree_store_append(priv->model, &iter, NULL);
	gtk_tree_store_set(priv->model, &iter, 0, connection, -1);
	DEBUG(Connection, "Connecting to agent located at: %s",
	      pk_connection_get_uri(connection));
	pk_connection_connect_async(connection,
	                            NULL,
	                            pkg_window_connection_connect_cb,
	                            window);
	EXIT;
}

static void
pkg_window_label_data_func (GtkTreeViewColumn *column,
                            GtkCellRenderer   *cell,
                            GtkTreeModel      *model,
                            GtkTreeIter       *iter,
                            gpointer           user_data)
{
	PkgWindowPrivate *priv;
	PkgWindow *window = user_data;
	GtkTreeSelection *selection;
	gchar *title;
	gchar *subtitle;
	gchar *markup;
	gchar color[12] = { 0 };
	GtkStateType state = GTK_STATE_NORMAL;

	g_return_if_fail(PKG_IS_WINDOW(window));

	priv = window->priv;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));
	if (gtk_tree_selection_iter_is_selected(selection, iter)) {
		state = GTK_STATE_SELECTED;
	}
	gtk_tree_model_get(model, iter, 1, &title, 2, &subtitle, -1);
	if (!title) {
		g_object_set(cell,
		             "markup",
		             _("<span size=\"smaller\"><i>Loading ...</i></span>"),
		             NULL);
		return;
	}
	pkg_util_get_mix_color(GTK_WIDGET(window), state, color, sizeof(color));
	markup = g_markup_printf_escaped("<span size=\"smaller\">"
	                                 "<b>%s</b>\n"
	                                 "<span color=\"%s\">%s</span>"
	                                 "</span>",
	                                 title, color, STR_OR_EMPTY(subtitle));
	g_object_set(cell, "markup", markup, NULL);
	g_free(markup);
}

static void
pkg_window_pixbuf_data_func (GtkTreeViewColumn *column,
                             GtkCellRenderer   *cell,
                             GtkTreeModel      *model,
                             GtkTreeIter       *iter,
                             gpointer           user_data)
{
	PkgWindow *window = user_data;
	PkConnection *connection;
	GtkTreeIter parent;

	g_return_if_fail(PKG_IS_WINDOW(window));

	if (!gtk_tree_model_iter_parent(model, &parent, iter)) {
		g_object_set(cell, "icon-name", "computer", NULL);
	} else {
		g_object_set(cell, "pixbuf", NULL, NULL);
	}
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

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pkg_window_finalize;
	g_type_class_add_private(object_class, sizeof(PkgWindowPrivate));
}

static void
pkg_window_init (PkgWindow *window)
{
	PkgWindowPrivate *priv;
	GtkAccelGroup *accel_group;
	GtkWidget *vbox;
	GtkWidget *menu_bar;
	GtkWidget *perfkit_menu;
	GtkWidget *help_menu;
	GtkWidget *hpaned;
	GtkWidget *scroller;
	GtkTreeViewColumn *column;
	GtkCellRenderer *text;
	GtkCellRenderer *cpix;

	ENTRY;
	priv = G_TYPE_INSTANCE_GET_PRIVATE(window, PKG_TYPE_WINDOW,
	                                   PkgWindowPrivate);
	window->priv = priv;
	g_static_rw_lock_init(&priv->rw_lock);

	gtk_window_set_title(GTK_WINDOW(window), _("Perfkit"));
	gtk_window_set_default_size(GTK_WINDOW(window), 780, 550);
	accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_widget_show(vbox);

	menu_bar = gtk_menu_bar_new();
	gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, TRUE, 0);
	gtk_widget_show(menu_bar);

	#define ADD_MENU(_n, _s)                                       \
	    G_STMT_START {                                             \
	    	GtkWidget *_w;                                         \
	        _w = gtk_menu_item_new_with_mnemonic((_s));            \
	        gtk_widget_show((_w));                                 \
	        gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), (_w)); \
	        _n = gtk_menu_new();                                   \
	        gtk_menu_item_set_submenu(GTK_MENU_ITEM(_w), _n);      \
	    } G_STMT_END

	#define ADD_MENU_ITEM(_p, _s)                                  \
	    G_STMT_START {                                             \
	        GtkWidget *_w;                                         \
	        _w = gtk_menu_item_new_with_mnemonic(_s);              \
	        gtk_widget_show((_w));                                 \
	        gtk_menu_shell_append(GTK_MENU_SHELL(_p), (_w));       \
	    } G_STMT_END

	#define ADD_MENU_ITEM_STOCK(_p, _s)                            \
	    G_STMT_START {                                             \
	        GtkWidget *_w = gtk_image_menu_item_new_from_stock(    \
	                (_s), accel_group);                            \
	        gtk_widget_show((_w));                                 \
	        gtk_menu_shell_append(GTK_MENU_SHELL(_p), (_w));       \
	    } G_STMT_END

	#define ADD_SEPARATOR(_p)                                      \
	    G_STMT_START {                                             \
	        GtkWidget *_w = gtk_separator_menu_item_new();         \
	        gtk_widget_show((_w));                                 \
	        gtk_menu_shell_append(GTK_MENU_SHELL(_p), (_w));       \
	    } G_STMT_END

	ADD_MENU(perfkit_menu, "_Perfkit");
	ADD_MENU_ITEM(perfkit_menu, "Connect to _Server");
	ADD_SEPARATOR(perfkit_menu);
	ADD_MENU_ITEM_STOCK(perfkit_menu, GTK_STOCK_QUIT);
	ADD_MENU(help_menu, "_Help");
	ADD_MENU_ITEM_STOCK(help_menu, GTK_STOCK_ABOUT);

	hpaned = gtk_hpaned_new();
	gtk_paned_set_position(GTK_PANED(hpaned), 275);
	gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 0);
	gtk_widget_show(hpaned);

	scroller = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
	                               GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroller),
	                                    GTK_SHADOW_IN);
	gtk_paned_add1(GTK_PANED(hpaned), scroller);
	gtk_widget_show(scroller);
	

	priv->model = gtk_tree_store_new(3,
	                                 PK_TYPE_CONNECTION,
	                                 G_TYPE_STRING,
	                                 G_TYPE_STRING);
	priv->treeview = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(scroller), priv->treeview);
	gtk_tree_view_set_model(GTK_TREE_VIEW(priv->treeview),
	                        GTK_TREE_MODEL(priv->model));
	gtk_widget_show(priv->treeview);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Agents"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->treeview), column);

	cpix = gtk_cell_renderer_pixbuf_new();
	g_object_set(cpix,
	             "stock-size", GTK_ICON_SIZE_LARGE_TOOLBAR,
	             NULL);
	gtk_tree_view_column_pack_start(column, cpix, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, cpix,
	                                        pkg_window_pixbuf_data_func,
	                                        window, NULL);

	text = gtk_cell_renderer_text_new();
	g_object_set(text,
	             "ellipsize", PANGO_ELLIPSIZE_END,
	             NULL);
	gtk_tree_view_column_pack_start(column, text, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, text,
	                                        pkg_window_label_data_func,
	                                        window, NULL);

	priv->container = gtk_alignment_new(0.5f, 0.5f, 1.0f, 1.0f);
	gtk_paned_add2(GTK_PANED(hpaned), priv->container);
	gtk_widget_show(priv->container);
	

	EXIT;
}
