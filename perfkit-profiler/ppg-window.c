/* ppg-window.c
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
#include <perfkit/perfkit.h>

#include "ppg-log.h"
#include "ppg-log-window.h"
#include "ppg-panels.h"
#include "ppg-path.h"
#include "ppg-window.h"
#include "ppg-util.h"

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "Window"

/**
 * SECTION:ppg-window.h
 * @title: PpgWindow
 * @short_description: 
 *
 * Section overview.
 */

G_DEFINE_TYPE(PpgWindow, ppg_window, GTK_TYPE_WINDOW)

static guint window_count = 0;

struct _PpgWindowPrivate
{
	PkConnection *conn;

	GtkBuilder   *builder;
	GtkWidget    *toolbar;
	GtkWidget    *content;

	struct {
		GtkAction *record;
		GtkAction *debug;
	} actions;
};

/**
 * ppg_window_new:
 *
 * Creates a new instance of #PpgWindow.
 *
 * Returns: the newly created instance of #PpgWindow.
 * Side effects: None.
 */
GtkWidget*
ppg_window_new (void)
{
	return g_object_new(PPG_TYPE_WINDOW, NULL);
}

/**
 * ppg_window_count:
 *
 * Returns the count of active windows.
 *
 * Returns: Window count.
 * Side effects: None.
 */
guint
ppg_window_count (void)
{
	return window_count;
}

static void
ppg_window_connected (PkConnection *conn,   /* IN */
                      GAsyncResult *result, /* IN */
                      PpgWindow    *window) /* IN */
{
	PpgWindowPrivate *priv;
	GError *error = NULL;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;
	/*
	 * Check if the connection succeeded.
	 */
	if (!pk_connection_connect_finish(conn, result, &error)) {
		DISPLAY_ERROR(window,
		              _("Could not connect to Perfkit Agent"),
		              "%s", error->message);
		g_error_free(error);
		goto cleanup;
	}
	gtk_widget_set_sensitive(priv->content, TRUE);
  cleanup:
	g_object_unref(window);
}

/**
 * ppg_window_connect_to:
 * @window: A #PpgWindow.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_window_connect_to (PpgWindow   *window, /* IN */
                       const gchar *uri)    /* IN */
{
	PpgWindowPrivate *priv;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;
	if (priv->conn) {
		CRITICAL("%s() may only be called once for a window.", G_STRFUNC);
		return;
	}
	/*
	 * Create a connection for the uri.
	 */
	if (!(priv->conn = pk_connection_new_from_uri(uri))) {
		CRITICAL("Invalid URI: %s", uri);
		return;
	}
	/*
	 * Start connceting to the agent.
	 */
	gtk_widget_set_sensitive(priv->content, FALSE);
	pk_connection_connect_async(priv->conn, NULL,
	                            G_ASYNC(ppg_window_connected),
	                            g_object_ref(window));
}

/**
 * ppg_window_delete_event:
 * @window: A #PpgWindow.
 *
 * Handle delete-event for window.  If no windows are active, quit.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_delete_event (PpgWindow *window,    /* IN */
                         gpointer   user_data) /* IN */
{
	if (!ppg_window_count()) {
		gtk_main_quit();
	}
}

/**
 * ppg_window_show_sources:
 * @window: A #PpgWindow.
 *
 * Shows the sources panel and ensures it contains the list of sources
 * for this window.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_show_sources (PpgWindow *window) /* IN */
{
	ppg_panels_sources_show();
}

/**
 * ppg_window_debug_action:
 * @action: A #GtkAction.
 * @window: A #PpgWindow.
 *
 * Handles the #GtkAction for showing the debug log.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_debug_action (GtkAction *action, /* IN */
                         PpgWindow *window) /* IN */
{
	GtkWidget *log;

	log = ppg_log_window_new();
	gtk_window_present(GTK_WINDOW(log));
}

/**
 * ppg_window_finalize:
 * @object: A #PpgWindow.
 *
 * Finalizer for a #PpgWindow instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_finalize (GObject *object) /* IN */
{
	G_OBJECT_CLASS(ppg_window_parent_class)->finalize(object);
}

/**
 * ppg_window_class_init:
 * @klass: A #PpgWindowClass.
 *
 * Initializes the #PpgWindowClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_class_init (PpgWindowClass *klass) /* IN */
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_window_finalize;
	g_type_class_add_private(object_class, sizeof(PpgWindowPrivate));
}

/**
 * ppg_window_init:
 * @window: A #PpgWindow.
 *
 * Initializes the newly created #PpgWindow instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_init (PpgWindow *window) /* IN */
{
	PpgWindowPrivate *priv;
	GtkWidget *child;
	gchar *path;

	/*
	 * Allocate private data.
	 */
	window->priv = G_TYPE_INSTANCE_GET_PRIVATE(window,
	                                           PPG_TYPE_WINDOW,
	                                           PpgWindowPrivate);
	priv = window->priv;

	/*
	 * Set defaults.
	 */
	gtk_window_set_default_size(GTK_WINDOW(window), 770, 550);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

	/*
	 * Load GtkBuilder ui.
	 */
	path = ppg_path_build("ui", "ppg-window.ui", NULL);
	priv->builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(priv->builder, path, NULL)) {
		g_error("Error loading gtk builder.");
	}
	g_free(path);

	/*
	 * Extract widgets.
	 */
	EXTRACT_WIDGET(priv->builder, "window-child", child);
	EXTRACT_WIDGET(priv->builder, "toolbar", priv->toolbar);
	EXTRACT_WIDGET(priv->builder, "content", priv->content);
	EXTRACT_OBJECT(priv->builder, GtkAction*, "record-action",
	               priv->actions.record);
	EXTRACT_ACTION(priv->builder, "debug-action", priv->actions.debug,
	               ppg_window_debug_action, window);
	EXTRACT_MENU_ITEM(priv->builder, "show-sources",
	                  ppg_window_show_sources);

	/*
	 * Reparent gtk builder widgets.
	 */
	gtk_widget_reparent(child, GTK_WIDGET(window));

	/*
	 * Connect signals.
	 */
	gtk_builder_connect_signals(GTK_BUILDER(priv->builder), window);
	g_signal_connect(window,
	                 "delete-event",
	                 G_CALLBACK(ppg_window_delete_event),
	                 NULL);
}
