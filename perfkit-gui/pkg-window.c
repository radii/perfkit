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

#include <gdatetime.h>
#include <glib/gi18n.h>
#include <perfkit/perfkit.h>

#include "pkg-log.h"
#include "pkg-window.h"

#define STR_OR_EMPTY(_s) ((_s) ? (_s) : "")
#define P_(_s, _p, _n) (g_dngettext(GETTEXT_PACKAGE, _s, _p, _n))

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

typedef struct
{
	PkgWindow    *window;
	PkConnection *connection;
	gint          channel;
} PkgWindowChannelCall;

enum
{
	TYPE_CHANNELS,
	TYPE_PLUGINS,
	TYPE_SUBSCRIPTIONS,
};

enum
{
	COLUMN_CONNECTION,
	COLUMN_TITLE,
	COLUMN_SUBTITLE,
	COLUMN_ID,
};

PkgWindowChannelCall*
pkg_window_channel_call_new (PkgWindow    *window,     /* IN */
                             PkConnection *connection, /* IN */
                             gint          channel)    /* IN */
{
	PkgWindowChannelCall *call;

	ENTRY;
	call = g_slice_new0(PkgWindowChannelCall);
	call->window = window;
	call->connection = connection;
	call->channel = channel;
	RETURN(call);
}

PkgWindowChannelCall*
pkg_window_channel_call_free (PkgWindowChannelCall *call)
{
	g_slice_free(PkgWindowChannelCall, call);
}

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
	ENTRY;
	RETURN(g_object_new(PKG_TYPE_WINDOW, NULL));
}

static gboolean
pkg_window_get_connection_iter (PkgWindow    *window,     /* IN */
                                PkConnection *connection, /* IN */
                                GtkTreeIter  *iter)       /* OUT */
{
	PkgWindowPrivate *priv;
	PkConnection *current;
	GtkTreeModel *model;

	g_return_val_if_fail(PKG_IS_WINDOW(window), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	ENTRY;
	priv = window->priv;
	model = GTK_TREE_MODEL(priv->model);
	if (!gtk_tree_model_get_iter_first(model, iter)) {
		RETURN(FALSE);
	}
	do {
		gtk_tree_model_get(model, iter, COLUMN_CONNECTION, &current, -1);
		if (current == connection) {
			RETURN(TRUE);
		}
	} while (gtk_tree_model_iter_next(model, iter));
	RETURN(FALSE);
}

static gboolean
pkg_window_get_connection_child_iter (PkgWindow    *window,     /* IN */
                                      PkConnection *connection, /* IN */
                                      GtkTreeIter  *iter,       /* OUT */
                                      gint          id_type)    /* IN */
{
	PkgWindowPrivate *priv;
	GtkTreeModel *model;
	GtkTreeIter parent;
	gint row_type = 0;

	g_return_val_if_fail(PKG_IS_WINDOW(window), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	ENTRY;
	priv = window->priv;
	model = GTK_TREE_MODEL(priv->model);
	if (!pkg_window_get_connection_iter(window, connection, &parent)) {
		RETURN(FALSE);
	}
	if (!gtk_tree_model_iter_children(model, iter, &parent)) {
		RETURN(FALSE);
	}
	do {
		gtk_tree_model_get(model, iter, COLUMN_ID, &row_type, -1);
		if (row_type == id_type) {
			RETURN(TRUE);
		}
	} while (gtk_tree_model_iter_next(model, iter));
	RETURN(FALSE);
}

static gboolean
pkg_window_get_channels_iter (PkgWindow    *window,     /* IN */
                              PkConnection *connection, /* IN */
                              GtkTreeIter  *iter)       /* OUT */
{
	return pkg_window_get_connection_child_iter (window, connection, iter,
	                                             TYPE_CHANNELS);
}

static gboolean
pkg_window_get_plugins_iter (PkgWindow    *window,     /* IN */
                             PkConnection *connection, /* IN */
                             GtkTreeIter  *iter)       /* OUT */
{
	return pkg_window_get_connection_child_iter (window, connection, iter,
	                                             TYPE_PLUGINS);
}

static gboolean
pkg_window_get_subscriptions_iter (PkgWindow    *window,     /* IN */
                                   PkConnection *connection, /* IN */
                                   GtkTreeIter  *iter)       /* OUT */
{
	return pkg_window_get_connection_child_iter (window, connection, iter,
	                                             TYPE_SUBSCRIPTIONS);
}

static gboolean
pkg_window_get_child_iter_with_id (PkgWindow    *window,     /* IN */
                                   PkConnection *connection, /* IN */
                                   GtkTreeIter  *iter,       /* IN */
                                   GtkTreeIter  *child,      /* OUT */
                                   gint          id)         /* IN */
{
	PkgWindowPrivate *priv;
	GtkTreeModel *model;
	gint row_id;

	g_return_val_if_fail(PKG_IS_WINDOW(window), FALSE);
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);
	g_return_val_if_fail(child != NULL, FALSE);

	ENTRY;
	priv = window->priv;
	model = GTK_TREE_MODEL(priv->model);
	if (!gtk_tree_model_iter_children(model, child, iter)) {
		RETURN(FALSE);
	}
	do {
		gtk_tree_model_get(model, child, COLUMN_ID, &row_id, -1);
		if (id == row_id) {
			RETURN(TRUE);
		}
	} while (gtk_tree_model_iter_next(model, child));
	RETURN(FALSE);
}

static void
pkg_window_expand_to_iter (PkgWindow   *window, /* IN */
                           GtkTreeIter *iter)   /* IN */
{
	PkgWindowPrivate *priv;
	GtkTreePath *path;

	g_return_if_fail(PKG_IS_WINDOW(window));
	g_return_if_fail(iter != NULL);

	priv = window->priv;
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(priv->model), iter);
	gtk_tree_view_expand_to_path(GTK_TREE_VIEW(priv->treeview), path);
	gtk_tree_path_free(path);
}

/**
 * pkg_window_connection_manager_get_hostname_cb:
 * @object: A #PkConnection.
 *
 * Callback which is fired upon receiving the hostname on which an agent runs
 * upon.  Updates the GtkTreeStore containing information about the agent.
 *
 * Returns: None.
 * Side effects: None.
 */
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
	if (pkg_window_get_connection_iter(user_data, connection, &iter)) {
		gtk_tree_store_set(priv->model, &iter,
		                   1, hostname,
		                   2, "Some sort of info here",
		                   -1);
	}
	g_free(hostname);
	EXIT;
}

static void
pkg_window_connection_channel_get_created_at_cb (GObject      *object,    /* IN */
                                                 GAsyncResult *result,    /* IN */
                                                 gpointer      user_data) /* IN */
{
	PkgWindowChannelCall *call = user_data;
	PkConnection *connection;
	PkgWindowPrivate *priv;
	gchar *subtitle = NULL;
	GError *error = NULL;
	GtkTreeIter iter;
	GtkTreeIter child;
	GDateTime *dt;
	GTimeVal tv;

	g_return_if_fail(user_data != NULL);

	ENTRY;
	priv = call->window->priv;
	connection = PK_CONNECTION(object);
	if (!pk_connection_channel_get_created_at_finish(connection, result,
	                                                 &tv, &error)) {
		WARNING(Connection, "Error retrieving channel created-at: %s",
		        error->message);
		g_error_free(error);
		EXIT;
	}
	if (!pkg_window_get_channels_iter(call->window, connection, &iter)) {
		GOTO(iter_not_found);
	}
	if (!pkg_window_get_child_iter_with_id(call->window, connection,
	                                       &iter, &child,
	                                       call->channel)) {
		GOTO(iter_not_found);
	}
	dt = g_date_time_new_from_timeval(&tv);
	subtitle = g_date_time_printf(dt, "Created on %x at %X");
	g_date_time_unref(dt);
	gtk_tree_store_set(priv->model, &child, COLUMN_SUBTITLE, subtitle, -1);
  iter_not_found:
	pkg_window_channel_call_free(call);
	g_free(subtitle);
	EXIT;
}

static void
pkg_window_connection_manager_get_channels_cb (GObject      *object,    /* IN */
                                               GAsyncResult *result,    /* IN */
                                               gpointer      user_data) /* IN */
{
	PkConnection *connection;
	PkgWindowPrivate *priv;
	gint *channels = NULL;
	gsize channels_len = 0;
	gchar *subtitle = NULL;
	GError *error = NULL;
	GtkTreeIter iter;
	GtkTreeIter child;
	gchar *str;
	gint i;

	g_return_if_fail(PKG_IS_WINDOW(user_data));

	ENTRY;
	priv = PKG_WINDOW(user_data)->priv;
	connection = PK_CONNECTION(object);
	if (!pk_connection_manager_get_channels_finish(connection, result,
	                                               &channels, &channels_len,
	                                               &error)) {
		WARNING(Connection, "Error retrieving channel list: %s",
		        error->message);
		g_error_free(error);
		EXIT;
	}
	if (!pkg_window_get_channels_iter(user_data, connection, &iter)) {
		GOTO(iter_not_found);
	}
	subtitle = g_strdup_printf(P_("%d channel", "%d channels", channels_len),
	                           (gint)channels_len);
	gtk_tree_store_set(priv->model, &iter,
	                   COLUMN_SUBTITLE, subtitle,
	                   -1);
	for (i = 0; i < channels_len; i++) {
		str = g_strdup_printf(_("Channel %d"), channels[i]);
		gtk_tree_store_append(priv->model, &child, &iter);
		gtk_tree_store_set(priv->model, &child,
		                   COLUMN_ID, channels[i],
		                   COLUMN_TITLE, str,
		                   COLUMN_SUBTITLE, _("Loading ..."),
		                   -1);
		pk_connection_channel_get_created_at_async(
				connection, channels[i], NULL,
				pkg_window_connection_channel_get_created_at_cb,
				pkg_window_channel_call_new(user_data,
				                            connection,
				                            channels[i]));
		pkg_window_expand_to_iter(user_data, &child);
		g_free(str);
	}
  iter_not_found:
	g_free(subtitle);
	g_free(channels);
	EXIT;
}

static void
pkg_window_connection_manager_get_plugins_cb (GObject      *object,    /* IN */
                                              GAsyncResult *result,    /* IN */
                                              gpointer      user_data) /* IN */
{
	PkConnection *connection;
	PkgWindowPrivate *priv;
	gchar **plugins = NULL;
	gchar *subtitle = NULL;
	GError *error = NULL;
	GtkTreeIter iter;
	GtkTreeIter child;
	gint len;
	gint i;

	g_return_if_fail(PKG_IS_WINDOW(user_data));

	ENTRY;
	priv = PKG_WINDOW(user_data)->priv;
	connection = PK_CONNECTION(object);
	if (!pk_connection_manager_get_plugins_finish(connection, result,
	                                              &plugins, &error)) {
		WARNING(Connection, "Error retrieving plugin list: %s",
		        error->message);
		g_error_free(error);
		EXIT;
	}
	if (!pkg_window_get_plugins_iter(user_data, connection, &iter)) {
		GOTO(iter_not_found);
	}
	len = g_strv_length(plugins);
	subtitle = g_strdup_printf(P_("%d plugin", "%d plugins", len), len);
	gtk_tree_store_set(priv->model, &iter, COLUMN_SUBTITLE, subtitle, -1);
	for (i = 0; plugins[i]; i++) {
		gchar *desc;
		gtk_tree_store_append(priv->model, &child, &iter);
		gtk_tree_store_set(priv->model, &child,
		                   COLUMN_ID, plugins[i],
		                   COLUMN_TITLE, plugins[i],
		                   COLUMN_SUBTITLE, "Show plugin description here",
		                   -1);
		pkg_window_expand_to_iter(user_data, &child);
		if (pk_connection_plugin_get_description(connection, plugins[i], &desc, NULL)) {
			gtk_tree_store_set(priv->model, &child, COLUMN_SUBTITLE, desc, -1);
			g_free(desc);
		}
	}
  iter_not_found:
  	g_free(subtitle);
	g_strfreev(plugins);
	EXIT;
}

static void
pkg_window_connection_manager_get_subscriptions_cb (GObject      *object,    /* IN */
                                                    GAsyncResult *result,    /* IN */
                                                    gpointer      user_data) /* IN */
{
	PkConnection *connection;
	PkgWindowPrivate *priv;
	gint *subscriptions;
	gsize subscriptions_len;
	gchar *title = NULL;
	gchar *subtitle = NULL;
	GError *error = NULL;
	GtkTreeIter iter;
	GtkTreeIter child;
	gint i;

	g_return_if_fail(PKG_IS_WINDOW(user_data));

	ENTRY;
	priv = PKG_WINDOW(user_data)->priv;
	connection = PK_CONNECTION(object);
	if (!pk_connection_manager_get_subscriptions_finish(connection, result,
	                                                    &subscriptions,
	                                                    &subscriptions_len,
	                                                    &error)) {
		WARNING(Connection, "Error retrieving subscriptions list: %s",
		        error->message);
		g_error_free(error);
		EXIT;
	}
	if (!pkg_window_get_subscriptions_iter(user_data, connection, &iter)) {
		GOTO(iter_not_found);
	}
	subtitle = g_strdup_printf(P_("%d subscription", "%d subscriptions",
	                              subscriptions_len),
	                           (gint)subscriptions_len);
	gtk_tree_store_set(priv->model, &iter, COLUMN_SUBTITLE, subtitle, -1);
  	g_free(subtitle);
	for (i = 0; i < subscriptions_len; i++) {
		title = g_strdup_printf(_("Subscription %d"), subscriptions[i]);
		gtk_tree_store_append(priv->model, &child, &iter);
		gtk_tree_store_set(priv->model, &child,
		                   COLUMN_ID, subscriptions[i],
		                   COLUMN_TITLE, title,
		                   COLUMN_SUBTITLE, _("Loading ..."),
		                   -1);
		pkg_window_expand_to_iter(user_data, &child);
		g_free(title);
	}
  iter_not_found:
  	g_free(subscriptions);
	EXIT;
}

static void
pkg_window_connection_connect_cb (GObject      *object,
                                  GAsyncResult *result,
                                  gpointer      user_data)
{
	PkgWindowPrivate *priv;
	PkConnection *connection;
	GError *error = NULL;
	GtkTreeIter iter;
	GtkTreeIter child;

	g_return_if_fail(PKG_IS_WINDOW(user_data));

	ENTRY;
	priv = PKG_WINDOW(user_data)->priv;
	connection = PK_CONNECTION(object);
	if (!pk_connection_connect_finish(connection, result, &error)) {
		WARNING(Connection, "Error connecting to agent: %s", error->message);
		g_error_free(error);
		EXIT;
	}
	INFO(Connection, "Established connection to agent at: %s.",
	     pk_connection_get_uri(connection));
	if (!pkg_window_get_connection_iter(user_data, connection, &iter)) {
		EXIT;
	}
	gtk_tree_store_append(priv->model, &child, &iter);
	gtk_tree_store_set(priv->model, &child,
	                   COLUMN_ID, TYPE_CHANNELS,
	                   COLUMN_TITLE, _("Channels"),
		               COLUMN_SUBTITLE, _("Loading ..."),
	                   -1);
	pkg_window_expand_to_iter(user_data, &child);
	gtk_tree_store_append(priv->model, &child, &iter);
	gtk_tree_store_set(priv->model, &child,
	                   COLUMN_ID, TYPE_PLUGINS,
	                   COLUMN_TITLE, _("Plugins"),
		               COLUMN_SUBTITLE, _("Loading ..."),
	                   -1);
	pkg_window_expand_to_iter(user_data, &child);
	gtk_tree_store_append(priv->model, &child, &iter);
	gtk_tree_store_set(priv->model, &child,
	                   COLUMN_ID, TYPE_SUBSCRIPTIONS,
	                   COLUMN_TITLE, _("Subscriptions"),
		               COLUMN_SUBTITLE, _("Loading ..."),
	                   -1);
	pkg_window_expand_to_iter(user_data, &child);
	pk_connection_manager_get_hostname_async(
			connection, NULL,
	        pkg_window_connection_manager_get_hostname_cb,
	        user_data);
	pk_connection_manager_get_channels_async(
			connection, NULL,
			pkg_window_connection_manager_get_channels_cb,
			user_data);
	pk_connection_manager_get_plugins_async(
			connection, NULL,
			pkg_window_connection_manager_get_plugins_cb,
			user_data);
	pk_connection_manager_get_subscriptions_async(
			connection, NULL,
			pkg_window_connection_manager_get_subscriptions_cb,
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
	gtk_tree_model_get(model, iter,
	                   COLUMN_TITLE, &title,
	                   COLUMN_SUBTITLE, &subtitle, -1);
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
	GtkTreePath *path;
	gint row_type;
	gint depth;

	g_return_if_fail(PKG_IS_WINDOW(window));

	/* quick path for top-level computer */
	if (!gtk_tree_model_iter_parent(model, &parent, iter)) {
		g_object_set(cell, "icon-name", "computer", NULL);
		return;
	}

	path = gtk_tree_model_get_path(model, iter);
	depth = gtk_tree_path_get_depth(path);
	gtk_tree_path_free(path);

	if (depth == 2) {
		gtk_tree_model_get(model, iter, COLUMN_ID, &row_type, -1);
		switch (row_type) {
		case TYPE_CHANNELS:
			g_object_set(cell, "icon-name", "stock_channel", NULL);
			break;
		case TYPE_SUBSCRIPTIONS:
			g_object_set(cell, "icon-name", "stock_autofilter", NULL);
			break;
		case TYPE_PLUGINS:
			g_object_set(cell, "icon-name", "stock_insert-plugin", NULL);
			break;
		default:
			GOTO(invalid_row_type);
		}
		return;
	}

  invalid_row_type:
	g_object_set(cell, "icon-name", NULL, NULL);
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
	

	priv->model = gtk_tree_store_new(4,
	                                 PK_TYPE_CONNECTION,
	                                 G_TYPE_STRING,
	                                 G_TYPE_STRING,
	                                 G_TYPE_INT);
	priv->treeview = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(scroller), priv->treeview);
	gtk_tree_view_set_model(GTK_TREE_VIEW(priv->treeview),
	                        GTK_TREE_MODEL(priv->model));
	gtk_tree_view_expand_all(GTK_TREE_VIEW(priv->treeview));
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
