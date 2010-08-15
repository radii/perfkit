/* ppg-log-window.c
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

#include "ppg-log.h"
#include "ppg-log-window.h"

/**
 * SECTION:ppg-log-window.h
 * @title: PpgLogWindow
 * @short_description: 
 *
 * Section overview.
 */

G_DEFINE_TYPE(PpgLogWindow, ppg_log_window, GTK_TYPE_WINDOW)

struct _PpgLogWindowPrivate
{
	GtkListStore *model;
	gboolean      enabled;
	guint         listener_id;
};

/**
 * ppg_log_window_new:
 *
 * Creates a new instance of #PpgLogWindow.
 *
 * Returns: the newly created instance of #PpgLogWindow.
 * Side effects: None.
 */
GtkWidget*
ppg_log_window_new (void)
{
	return g_object_new(PPG_TYPE_LOG_WINDOW, NULL);
}

static void
ppg_log_window_log_listener (const gchar *message,   /* IN */
                             gpointer     user_data) /* IN */
{
	PpgLogWindowPrivate *priv;
	GtkTreeIter iter;

	g_return_if_fail(PPG_IS_LOG_WINDOW(user_data));
	g_return_if_fail(message != NULL);

	priv = PPG_LOG_WINDOW(user_data)->priv;
	gtk_list_store_append(priv->model, &iter);
	gtk_list_store_set(priv->model, &iter, 0, message, -1);
}

static void
ppg_log_window_clear_clicked (GtkWidget    *button, /* IN */
                              PpgLogWindow *window) /* IN */
{
	PpgLogWindowPrivate *priv;

	g_return_if_fail(PPG_IS_LOG_WINDOW(window));

	priv = window->priv;
	gtk_list_store_clear(priv->model);
}

static void
ppg_log_window_enable_toggled (GtkWidget    *button, /* IN */
                               PpgLogWindow *window) /* IN */
{
	PpgLogWindowPrivate *priv;

	g_return_if_fail(PPG_IS_LOG_WINDOW(window));

	priv = window->priv;
	if (priv->enabled) {
		ppg_log_remove_listener(priv->listener_id);
		priv->listener_id = 0;
	} else {
		priv->listener_id = ppg_log_add_listener(ppg_log_window_log_listener,
		                                         window);
	}

	priv->enabled = !priv->enabled;
	DEBUG("Log observer enabled.");
}

/**
 * ppg_log_window_finalize:
 * @object: A #PpgLogWindow.
 *
 * Finalizer for a #PpgLogWindow instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_log_window_finalize (GObject *object) /* IN */
{
	PpgLogWindowPrivate *priv;

	priv = PPG_LOG_WINDOW(object)->priv;

	if (priv->listener_id) {
		ppg_log_remove_listener(priv->listener_id);
	}

	G_OBJECT_CLASS(ppg_log_window_parent_class)->finalize(object);
}

/**
 * ppg_log_window_class_init:
 * @klass: A #PpgLogWindowClass.
 *
 * Initializes the #PpgLogWindowClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_log_window_class_init (PpgLogWindowClass *klass) /* IN */
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_log_window_finalize;
	g_type_class_add_private(object_class, sizeof(PpgLogWindowPrivate));
}

/**
 * ppg_log_window_init:
 * @window: A #PpgLogWindow.
 *
 * Initializes the newly created #PpgLogWindow instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_log_window_init (PpgLogWindow *window) /* IN */
{
	PpgLogWindowPrivate *priv;
	GtkTreeViewColumn *column;
	GtkWidget *vbox;
	GtkWidget *bbox;
	GtkWidget *scroller;
	GtkWidget *treeview;
	GtkWidget *toggle;
	GtkWidget *clear;

	/*
	 * Allocate private data.
	 */
	window->priv = G_TYPE_INSTANCE_GET_PRIVATE(window,
	                                           PPG_TYPE_LOG_WINDOW,
	                                           PpgLogWindowPrivate);
	priv = window->priv;

	/*
	 * Create widgets.
	 */
	vbox = gtk_vbox_new(FALSE, 3);
	bbox = gtk_hbutton_box_new();
	scroller = gtk_scrolled_window_new(NULL, NULL);
	treeview = gtk_tree_view_new();
	toggle = gtk_toggle_button_new();
	gtk_button_set_label(GTK_BUTTON(toggle), _("Enable"));
	clear = gtk_button_new_from_stock(GTK_STOCK_CLEAR);

	/*
	 * Create widget layout.
	 */
	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(bbox), toggle, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(bbox), clear, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), scroller, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(scroller), treeview);

	/*
	 * Adjust styling.
	 */
	gtk_window_set_title(GTK_WINDOW(window), _("Debug Log"));
	gtk_window_set_default_size(GTK_WINDOW(window), 600, 200);
	gtk_button_set_relief(GTK_BUTTON(toggle), GTK_RELIEF_NONE);
	gtk_button_set_relief(GTK_BUTTON(clear), GTK_RELIEF_NONE);
	gtk_box_set_spacing(GTK_BOX(bbox), 3);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_START);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
	                               GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroller),
	                                    GTK_SHADOW_NONE);

	/*
	 * Setup tree model.
	 */
	priv->model = gtk_list_store_new(1, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(treeview),
	                        GTK_TREE_MODEL(priv->model));

	/*
	 * Setup treeview columns.
	 */
	column = gtk_tree_view_column_new_with_attributes("",
	                                                  gtk_cell_renderer_text_new(),
	                                                  "text", 0,
	                                                  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	/*
	 * Connect signals.
	 */
	g_signal_connect(clear,
	                 "clicked",
	                 G_CALLBACK(ppg_log_window_clear_clicked),
	                 window);
	g_signal_connect(toggle,
	                 "toggled",
	                 G_CALLBACK(ppg_log_window_enable_toggled),
	                 window);

	/*
	 * Enable logging to the observer.
	 */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), TRUE);

	/*
	 * Show widgets.
	 */
	gtk_widget_show(clear);
	gtk_widget_show(toggle);
	gtk_widget_show(bbox);
	gtk_widget_show(scroller);
	gtk_widget_show(treeview);
	gtk_widget_show(vbox);
}
