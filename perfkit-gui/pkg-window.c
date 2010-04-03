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

#include "pkg-cmds.h"
#include "pkg-paths.h"
#include "pkg-panels.h"
#include "pkg-prefs.h"
#include "pkg-runtime.h"
#include "pkg-session.h"
#include "pkg-session-view.h"
#include "pkg-util.h"
#include "pkg-window.h"

G_DEFINE_TYPE(PkgWindow, pkg_window, GTK_TYPE_WINDOW)

/**
 * SECTION:pkg-window
 * @title: PkgWindow
 * @short_description: 
 *
 * 
 */

typedef struct
{
	PkgWindow      *window;
	GtkWidget      *widget;
	PkgCommandFunc  func;
} PkgWindowClosure;

struct _PkgWindowPrivate
{
	GtkBuilder     *builder;
	PkConnection   *conn;
	PkgSessionView *selected;

	/*
	 * Imported widgets from GtkBuiler.
	 */
	GtkWidget *notebook;
	GtkWidget *toolbar;
};

static GList *windows = NULL;

/**
 * pkg_window_closure_new:
 * @window: A #PkgWindow.
 * @widget: A #GtkWidget.
 * @func: A PkgCommandFunc.
 *
 * Creates a new closure to be used for dispatching commands from the
 * window. @widget should be the calling widget.
 *
 * Returns: the newly created #PkgWindowClosure.
 * Side effects: None.
 */
static PkgWindowClosure*
pkg_window_closure_new (PkgWindow      *window,
                        GtkWidget      *widget,
                        PkgCommandFunc  func)
{
	PkgWindowClosure *closure;

	closure = g_slice_new(PkgWindowClosure);
	closure->window = window;
	closure->func = func;
	g_object_add_weak_pointer(G_OBJECT(window),
	                          (gpointer *)&closure->window);

	return closure;
}

/**
 * pkg_window_closure_free:
 * @closure: A #PkgWindowClosure.
 *
 * Frees #PkgWindowClosure resources.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pkg_window_closure_free (PkgWindowClosure *closure)
{
	g_object_remove_weak_pointer(G_OBJECT(closure->window),
	                             (gpointer *)&closure->window);
	g_slice_free(PkgWindowClosure, closure);
}

/**
 * pkg_window_closure_dispatch:
 * @closure: A #PkgWIndowClosure.
 *
 * Dispatches a closure to the proper command handler.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pkg_window_closure_dispatch (PkgWindowClosure *closure)
{
	PkgCommand cmd = { NULL };

	cmd.window = closure->window;
	cmd.widget = closure->widget;
	cmd.connection = pkg_window_get_connection(cmd.window);
	cmd.channel = NULL; /* TODO */
	cmd.sources = NULL; /* TODO */
	closure->func(&cmd);
}

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

	/*
	 * Create connection.
	 */
	conn = pk_connection_new_from_uri(uri);
	if (!conn) {
		return NULL;
	}

	/*
	 * Create window and attach connection.
	 */
	window = pkg_window_new();
	pkg_window_set_connection(PKG_WINDOW(window), conn);

	/*
	 * Done with our connection reference.
	 */
	g_object_unref(conn);

	return window;
}

/**
 * pkg_window_count_windows:
 *
 * Counts the number of active #PkgWindow instances.
 *
 * Returns: the number of windows.
 * Side effects: None.
 */
gint
pkg_window_count_windows (void)
{
	return g_list_length(windows);
}

/* DELETE */
static gboolean
pkg_window_close_clicked (GtkButton *button,
                          PkgWindow *window)
{
	g_debug("close session matching close button");
	return FALSE;
}

/* DELETE */
void
pkg_window_add_session (PkgWindow  *window,
                        PkgSession *session)
{
	PkgWindowPrivate *priv;
	GtkWidget *notebook,
	          *view,
	          *label,
	          *close,
	          *close_img;

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

	close_img = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	gtk_widget_show(close_img);

	close = gtk_button_new();
	gtk_widget_set_size_request(close, 22, 22);
	gtk_container_add(GTK_CONTAINER(close), close_img);
	gtk_button_set_relief(GTK_BUTTON(close), GTK_RELIEF_NONE);
	g_signal_connect(close, "clicked",
	                 G_CALLBACK(pkg_window_close_clicked),
	                 window);
	gtk_widget_show(close);

	view = pkg_session_view_new();
	pkg_session_view_set_session(PKG_SESSION_VIEW(view), session);
	gtk_widget_show(view);

	label = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(label),
	                   pkg_session_view_get_label(PKG_SESSION_VIEW(view)),
	                   TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(label), close, FALSE, TRUE, 0);
	gtk_widget_show(label);

	gtk_notebook_append_page(GTK_NOTEBOOK(priv->notebook), view, label);
}

/**
 * pkg_window_focus_in:
 * @window: A #PkgWindow.
 *
 * Handle the "focus-in" event for the window.  Notifies the runtime
 * that @window is the newly selected window.  Also sets the active
 * connection so the panels can adjust accordingly.
 */
static gboolean
pkg_window_focus_in (PkgWindow *window,
                     GdkEvent  *event,
                     gpointer   user_data)
{
	pkg_runtime_set_active_window(window);
	pkg_panels_set_connection(window->priv->conn);
	return FALSE;
}

/**
 * pkg_window_set_connection:
 * @window: A #PkgWindow.
 * @connection: A #PkConnection.
 *
 * Sets the #PkConnection for the window.  This method may only be called
 * once.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pkg_window_set_connection (PkgWindow    *window,
                           PkConnection *connection)
{
	g_return_if_fail(PKG_IS_WINDOW(window));
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(!window->priv->conn);

	window->priv->conn = g_object_ref(connection);
}

/* DELETE */
PkgSession*
pkg_window_get_session (PkgWindow *window)
{
	g_return_val_if_fail(PKG_IS_WINDOW(window), NULL);
	return pkg_session_view_get_session(window->priv->selected);
}

/**
 * pkg_window_get_connection:
 * @window: A #PkgWindow.
 *
 * Retrieves the connection which the window represents.
 *
 * Returns: A #PkConnection or %NULL.
 * Side effects: None.
 */
PkConnection*
pkg_window_get_connection (PkgWindow *window)
{
	g_return_val_if_fail(PKG_IS_WINDOW(window), NULL);
	return window->priv->conn;
}

static void
pkg_window_dispose (GObject *object)
{
	PkgWindowPrivate *priv = PKG_WINDOW(object)->priv;

	/*
	 * Unregister the window.
	 */
	windows = g_list_remove(windows, object);

	/*
	 * Lose references.
	 */
	g_object_unref(priv->builder);
	g_object_unref(priv->conn);

	G_OBJECT_CLASS(pkg_window_parent_class)->dispose(object);
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
	object_class->dispose = pkg_window_dispose;
	g_type_class_add_private(object_class, sizeof(PkgWindowPrivate));
}

/* DELETE */
static void
pkg_window_session_set (GtkWidget       *widget,
                        GtkNotebookPage *page,
                        guint            page_num,
                        PkgWindow       *window)
{
	GtkWidget *child;

	child = gtk_notebook_get_nth_page(GTK_NOTEBOOK(widget), page_num);
	if (!PKG_IS_SESSION_VIEW(child)) {
		g_warning("Child is not a PkgSessionView");
		return;
	}

	window->priv->selected = PKG_SESSION_VIEW(child);
}

static void
pkg_window_init (PkgWindow *window)
{
	PkgWindowPrivate *priv;
	gint width, height;

	window->priv = priv = G_TYPE_INSTANCE_GET_PRIVATE(window,
	                                                  PKG_TYPE_WINDOW,
	                                                  PkgWindowPrivate);

	/*
	 * Register the window.
	 */
	windows = g_list_prepend(windows, window);

	/*
	 * Set window defaults.
	 */
	gtk_window_set_title(GTK_WINDOW(window), _("Perfkit"));
	pkg_prefs_get_window_size(&width, &height);
	gtk_window_set_default_size(GTK_WINDOW(window), width, height);

	/*
	 * Load UI components from GtkBuilder.
	 */
	priv->builder = pkg_util_get_builder("perfkit.ui");
	pkg_util_reparent(priv->builder, "perfkit-child", GTK_WIDGET(window));

#define BUILDER_WIDGET(_name, _type) G_STMT_START {                            \
    GObject *_obj;                                                             \
                                                                               \
    _obj = gtk_builder_get_object(priv->builder, #_name);                      \
    if (!G_TYPE_CHECK_INSTANCE_TYPE(_obj, (_type))) {                          \
    	g_critical("%s is of type %s, expected %s.",                           \
                   #_name, g_type_name(G_TYPE_FROM_INSTANCE(_obj)),            \
                   g_type_name((_type)));                                      \
        break;                                                                 \
	}                                                                          \
	priv->_name = (gpointer)_obj;                                              \
	g_object_add_weak_pointer(G_OBJECT(priv->_name),                           \
                              (gpointer *)&priv->_name);                       \
} G_STMT_END

	/*
	 * Import widgets from GtkBuilder.
	 */
	BUILDER_WIDGET(notebook, GTK_TYPE_NOTEBOOK);
	BUILDER_WIDGET(toolbar,  GTK_TYPE_TOOLBAR);

#define MENU_ITEM_COMMAND(name, handler) G_STMT_START {                        \
    GObject *_widget;                                                          \
    PkgWindowClosure *_closure;                                                \
                                                                               \
    _widget = gtk_builder_get_object(priv->builder, #name);                    \
    _closure = pkg_window_closure_new(window, GTK_WIDGET(_widget), (handler)); \
    g_signal_connect_swapped(_widget,                                          \
                             "activate",                                       \
                             G_CALLBACK(pkg_window_closure_dispatch),          \
                             _closure);                                        \
} G_STMT_END

	MENU_ITEM_COMMAND(mnuFileQuit,       pkg_cmd_quit);
	MENU_ITEM_COMMAND(mnuEditPrefs,      pkg_cmd_show_prefs);
	MENU_ITEM_COMMAND(mnuViewSources,    pkg_cmd_show_sources);
	MENU_ITEM_COMMAND(mnuHelpAbout,      pkg_cmd_show_about);

#undef MENU_ITEM_COMMAND

	/*
	 * Register signals.
	 */
	g_signal_connect(window, "focus-in-event",
	                 G_CALLBACK(pkg_window_focus_in), NULL);
}
