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

#include "ppg-path.h"
#include "ppg-window.h"
#include "ppg-util.h"

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
	GtkBuilder *builder;
	GtkAction  *record_action;
	GtkWidget  *toolbar;
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
	EXTRACT_OBJECT(priv->builder, GtkAction*, "record-action",
	               priv->record_action);

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
