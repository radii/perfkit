/* ppg-welcome.c
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
#include <gtk/gtk.h>

#include "ppg-log.h"
#include "ppg-path.h"
#include "ppg-util.h"
#include "ppg-welcome.h"
#include "ppg-window.h"


typedef struct
{
	gsize         initialized;
	GtkBuilder   *builder;
	GtkWidget    *window;
	GtkWidget    *local;
	GtkWidget    *treeview;
	GtkListStore *model;
} PpgWelcome;


static PpgWelcome welcome = { 0 };


/**
 * ppg_welcome_delete_event:
 * @widget: A #PpgWelcome.
 *
 * Handle "delete-event" for welcome window. Hide instead.
 *
 * Returns: TRUE always.
 * Side effects: Window hidden.
 */
static gboolean
ppg_welcome_delete_event (GtkWidget *widget,
                          gpointer   user_data)
{
	g_return_val_if_fail(GTK_IS_WIDGET(widget), FALSE);

	gtk_widget_hide(widget);
	if (!ppg_window_count()) {
		gtk_main_quit();
	}
	return TRUE;
}

/**
 * ppg_welcome_separator_func:
 * @welcome: A #PpgWelcome.
 *
 * Determines if the current GtkTreeIter is for a separator row.
 *
 * Returns: TRUE if the row is a separator; otherwise FALSE.
 * Side effects: None.
 */
static gboolean
ppg_welcome_separator_func (GtkTreeModel *model,
                            GtkTreeIter  *iter,
                            gpointer      data)
{
	gboolean sep;

	g_return_val_if_fail(GTK_IS_TREE_MODEL(model), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	gtk_tree_model_get(model, iter, 2, &sep, -1);
	return sep;
}

/**
 * ppg_welcome_local_clicked:
 * @welcome: A #PpgWelcome.
 *
 * Handle "clicked" signal for new local profiling session button.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_welcome_local_clicked (GtkWidget *button,
                           gpointer   user_data)
{
	GtkWidget *window;

	window = g_object_new(PPG_TYPE_WINDOW,
	                      "visible", TRUE,
	                      NULL);
	gtk_widget_hide(welcome.window);
	ppg_window_connect_to(PPG_WINDOW(window), "dbus://");
}

/**
 * ppg_welcome_init:
 *
 * Initialize the welcome screen.
 *
 * Returns: None.
 * Side effects: Welcome screen is initialized.
 */
static void
ppg_welcome_init (void)
{
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;
	GtkCellRenderer *cell;
	GtkTreeIter iter;
	GtkWidget *window;
	GtkWidget *child;
	gchar *path;

	/*
	 * Create window.
	 */
	welcome.window = window = g_object_new(GTK_TYPE_WINDOW,
	                                       "height-request", 420,
	                                       "resizable", FALSE,
	                                       "title", _("Perfkit Profiler"),
	                                       "type", GTK_WINDOW_TOPLEVEL,
	                                       "width-request", 620,
	                                       NULL);

	/*
	 * Load widgets from gtk builder.
	 */
	path = ppg_path_build("ui", "ppg-welcome.ui", NULL);
	welcome.builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(welcome.builder, path, NULL)) {
		ERROR("Cannot load UI resources.");
	}
	g_free(path);

	/*
	 * Extract widgets from UI file.
	 */
	EXTRACT_WIDGET(welcome.builder, "welcome-child", child);
	EXTRACT_WIDGET(welcome.builder, "treeview", welcome.treeview);
	EXTRACT_WIDGET(welcome.builder, "local-button", welcome.local);

	/*
	 * Build treeview columns.
	 */
	column = gtk_tree_view_column_new();
	cell = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, cell, FALSE);
	gtk_tree_view_column_add_attribute(column, cell, "icon-name", 0);
	g_object_set(cell, "stock-size", GTK_ICON_SIZE_DND, NULL);
	cell = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, cell, TRUE);
	gtk_tree_view_column_add_attribute(column, cell, "text", 1);
	gtk_tree_view_append_column(GTK_TREE_VIEW(welcome.treeview),
	                            column);
	gtk_tree_view_set_row_separator_func(GTK_TREE_VIEW(welcome.treeview),
	                                     ppg_welcome_separator_func,
	                                     NULL, NULL);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(welcome.treeview));

	/*
	 * Attach model.
	 */
	welcome.model = gtk_list_store_new(3,
	                                   G_TYPE_STRING,
	                                   G_TYPE_STRING,
	                                   G_TYPE_BOOLEAN);
	gtk_tree_view_set_model(GTK_TREE_VIEW(welcome.treeview),
	                        GTK_TREE_MODEL(welcome.model));

	/*
	 * Add Home button.
	 */
	gtk_list_store_append(welcome.model, &iter);
	gtk_list_store_set(welcome.model, &iter,
	                   0, GTK_STOCK_HOME,
	                   1, _("Home"),
	                   -1);
	gtk_tree_selection_select_iter(selection, &iter);
	gtk_list_store_append(welcome.model, &iter);
	gtk_list_store_set(welcome.model, &iter, 2, TRUE, -1);

	/*
	 * Reparent child widget into window.
	 */
	gtk_widget_reparent(child, window);
	gtk_widget_set_can_default(welcome.local, TRUE);
	gtk_window_set_default(GTK_WINDOW(window), welcome.local);
	gtk_widget_grab_focus(welcome.local);

	/*
	 * Connect signals.
	 */
	g_signal_connect(window,
	                 "delete-event",
	                 G_CALLBACK(ppg_welcome_delete_event),
	                 NULL);
	g_signal_connect(welcome.local,
	                 "clicked",
	                 G_CALLBACK(ppg_welcome_local_clicked),
	                 NULL);
}

/**
 * ppg_welcome_show:
 *
 * Shows the welcome dialog.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_welcome_show (void)
{
	/*
	 * Initialize welcome window if needed.
	 */
	if (g_once_init_enter(&welcome.initialized)) {
		ppg_welcome_init();
		g_once_init_leave(&welcome.initialized, TRUE);
	}

	/*
	 * Center window on screen and display it.
	 */
	gtk_window_set_position(GTK_WINDOW(welcome.window), GTK_WIN_POS_CENTER);
	gtk_window_present(GTK_WINDOW(welcome.window));
}
