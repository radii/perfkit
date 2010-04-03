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
#include <perfkit/perfkit.h>
#include <perfkit/perfkit-lowlevel.h>

#include "pkg-channel-view.h"
#include "pkg-cmds.h"
#include "pkg-dialog.h"
#include "pkg-paths.h"
#include "pkg-panels.h"
#include "pkg-prefs.h"
#include "pkg-runtime.h"
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
	PkChannel      *channel;
	GtkWidget      *view;
	GtkAccelGroup  *accelgroup;

	/*
	 * Imported widgets from GtkBuiler.
	 */
	GtkWidget *toolbar;
	GtkWidget *view_align;
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
	PkChannel *channel;
	PkSpawnInfo info = {0};
	GtkWidget *window;
	GError *error = NULL;
	gint chan_id;

	window = pkg_window_new();
	conn = pk_connection_new_from_uri(uri);
	if (!conn) {
		return NULL;
	}

	/*
	 * TODO: Connect asynchronously.
	 */
	if (!pk_connection_is_connected(conn)) {
		if (!pk_connection_connect(conn, &error)) {
			pkg_dialog_warning(NULL,
			                   _("Error connecting to perfkit node"),
			                   _("There was an error while connecting to the "
			                     "desired perfkit node"),
			                   error ? error->message : _("Unknown error"),
			                   TRUE);

			/*
			 * TODO: Handle errors.
			 */
			g_warning("Error connecting to daemon.");
		}
	}

	/*
	 * TODO: Extract a channel from the URI.
	 */
	if (pk_connection_manager_create_channel(conn, &info, &chan_id, NULL)) {
		channel = g_object_new(PK_TYPE_CHANNEL,
		                       "connection", conn,
		                       "id", chan_id,
		                       NULL);
		pkg_window_attach(PKG_WINDOW(window), conn, channel);
		g_object_unref(channel);
	}

	g_object_unref(conn);

	return window;
}

/**
 * pkg_window_attach:
 * @window: A #PkgWindow.
 *
 * Attaches a connection and channel to the window for viewing.
 *
 * Returns: None.
 * Side effects: Everything.
 */
void
pkg_window_attach (PkgWindow    *window,     /* IN */
                   PkConnection *connection, /* IN */
                   PkChannel    *channel)    /* IN */
{
	PkgWindowPrivate *priv;

	g_return_if_fail(PKG_IS_WINDOW(window));
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(PK_IS_CHANNEL(channel));

	priv = window->priv;

	priv->conn = g_object_ref(connection);
	priv->channel = g_object_ref(channel);
	pkg_channel_view_set_channel(PKG_CHANNEL_VIEW(priv->view), channel);
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

/**
 * pkg_window_close:
 * @window: A #PkgWindow.
 *
 * Closes the #PkgWindow.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pkg_window_close (PkgWindow *window)
{
	gtk_widget_hide(GTK_WIDGET(window));

	/*
	 * If last window, exit.
	 */
	if (pkg_window_count_windows() == 1) {
		pkg_runtime_quit();
	}

	gtk_widget_destroy(GTK_WIDGET(window));
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
 * pkg_window_delete_event:
 * @window: A #PkgWindow.
 *
 * Handles the "delete-event" for the #PkgWindow.  If the window is the last
 * window, the application will exit.
 *
 * Returns: %FALSE.
 * Side effects: Everything.
 */
static gboolean
pkg_window_delete_event (PkgWindow *window)
{
	pkg_window_close(window);
	return FALSE;
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

/**
 * pkg_window_get_channel:
 * @window: A #PkgWindow.
 *
 * Retrieves the channel for the window.
 *
 * Returns: A #PkChannel or %NULL.
 * Side effects: None.
 */
PkChannel*
pkg_window_get_channel (PkgWindow *window)
{
	g_return_val_if_fail(PKG_IS_WINDOW(window), NULL);
	return window->priv->channel;
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
	BUILDER_WIDGET(accelgroup, GTK_TYPE_ACCEL_GROUP);
	BUILDER_WIDGET(toolbar,    GTK_TYPE_TOOLBAR);
	BUILDER_WIDGET(view_align, GTK_TYPE_ALIGNMENT);

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
	MENU_ITEM_COMMAND(mnuFileClose,      pkg_cmd_close);
	MENU_ITEM_COMMAND(mnuEditPrefs,      pkg_cmd_show_prefs);
	MENU_ITEM_COMMAND(mnuViewSources,    pkg_cmd_show_sources);
	MENU_ITEM_COMMAND(mnuHelpAbout,      pkg_cmd_show_about);

	/*
	 * Add the PkgChannelView to the container.
	 */
	priv->view = pkg_channel_view_new();
	gtk_container_add(GTK_CONTAINER(priv->view_align), priv->view);
	gtk_widget_show(priv->view);

	/*
	 * Wire up accelerators.
	 */
	gtk_window_add_accel_group(GTK_WINDOW(window), priv->accelgroup);

	/*
	 * Register signals.
	 */
	g_signal_connect(window, "focus-in-event",
	                 G_CALLBACK(pkg_window_focus_in), NULL);
	g_signal_connect(window, "delete-event",
	                 G_CALLBACK(pkg_window_delete_event), NULL);
}
