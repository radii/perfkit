/* pkg-panels.c
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

#include "pkg-panels.h"
#include "pkg-dialog.h"
#include "pkg-runtime.h"

typedef struct
{
	GtkWidget    *sources_window;
	GtkWidget    *sources_treeview;
	GtkListStore *sources_store;
	PkConnection *conn;
} PkgPanels;

static PkgPanels panels = {0};

static void
pkg_panels_sources_text_func (GtkTreeViewColumn   *tree_column,
                              GtkCellRenderer     *cell_renderer,
                              GtkTreeModel        *tree_model,
                              GtkTreeIter         *iter,
                              gpointer             data)
{
	PkSourceInfo *info = NULL;
	const gchar *desc;
	gchar *markup;

	gtk_tree_model_get(tree_model, iter, 0, &info, -1);
	desc = pk_source_info_get_description(info);

	if (desc && strlen(desc)) {
		markup = g_strdup_printf("<b>%s</b>\n<span size=\"smaller\"><i>%s</i></span>",
								 pk_source_info_get_name(info),
								 pk_source_info_get_description(info));
	} else {
		markup = g_strdup_printf("<b>%s</b>",
		                         pk_source_info_get_name(info));
	}
	g_object_set(cell_renderer, "markup", markup, NULL);
	g_free(markup);
}

static void
pkg_panels_sources_pixbuf_func (GtkTreeViewColumn   *tree_column,
                                GtkCellRenderer     *cell_renderer,
                                GtkTreeModel        *tree_model,
                                GtkTreeIter         *iter,
                                gpointer             data)
{
	g_object_set(cell_renderer,
	             "icon-name", "ethos-plugin",
	             NULL);
}

static void
pkg_panels_on_set_connection (PkConnection *connection)
{
	PkManager *manager;
	GList *list, *plugins;
	GtkTreeIter iter;

	/*
	 * Get the manager for the connection.
	 */
	if (!(manager = pk_connection_get_manager(panels.conn))) {
		g_warning("Error retrieving connection manager.");
		return;
	}

	/*
	 * Add the sources to the liststore.
	 */
	plugins = pk_manager_get_source_plugins(manager);
	for (list = plugins; list; list = list->next) {
		gtk_list_store_append(panels.sources_store, &iter);
		gtk_list_store_set(panels.sources_store, &iter, 0, list->data, -1);
	}
}

static void
pkg_panels_sources_refresh (GtkWidget *button,
                            gpointer   user_data)
{
	gtk_list_store_clear(panels.sources_store);

	if (panels.conn) {
		pkg_panels_on_set_connection(panels.conn);
	}
}

static gboolean
sources_window_delete_event (GtkWidget *widget,
                             GdkEvent  *event,
                             gpointer   user_data)
{
	gtk_widget_hide(widget);
	return TRUE;
}

static gboolean
sources_window_focus_out (GtkWidget *widget,
                          GdkEvent  *event,
                          gpointer   user_data)
{
	gtk_window_set_opacity(GTK_WINDOW(widget), .5);
	return FALSE;
}

static gboolean
sources_window_focus_in (GtkWidget *widget,
                         GdkEvent  *event,
                         gpointer   user_data)
{
	gtk_window_set_opacity(GTK_WINDOW(widget), 1.);
	return FALSE;
}

static void
pkg_panels_create_sources (void)
{
	GtkWidget *vbox,
	          *hbox,
	          *scroller,
	          *treeview,
	          *refresh,
	          *image;
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	gchar *icon_dir;
	GtkTargetEntry target_entry[] = {
		{ "text/x-perfkit-source-info", GTK_TARGET_SAME_APP, 0 },
	};

	icon_dir = g_build_filename(PACKAGE_DATA_DIR, "ethos", "icons", NULL);
	gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(),
	                                  icon_dir);
	g_free(icon_dir);

	panels.sources_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(panels.sources_window), "");
	gtk_window_set_default_size(GTK_WINDOW(panels.sources_window), 200, 400);
	gtk_window_set_type_hint(GTK_WINDOW(panels.sources_window),
	                         GDK_WINDOW_TYPE_HINT_UTILITY);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(panels.sources_window), TRUE);
	gtk_window_set_skip_pager_hint(GTK_WINDOW(panels.sources_window), TRUE);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(panels.sources_window), vbox);
	gtk_widget_show(vbox);

	scroller = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
	                               GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_NEVER);
	gtk_box_pack_start(GTK_BOX(vbox), scroller, TRUE, TRUE, 0);
	gtk_widget_show(scroller);

	treeview = gtk_tree_view_new();
	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(treeview), TRUE);
	gtk_container_add(GTK_CONTAINER(scroller), treeview);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
	gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(treeview),
	                                       GDK_BUTTON1_MASK,
	                                       target_entry,
	                                       G_N_ELEMENTS(target_entry),
	                                       GDK_ACTION_COPY);
	panels.sources_treeview = treeview;
	gtk_widget_show(treeview);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_widget_show(hbox);

	refresh = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU);
	gtk_button_set_relief(GTK_BUTTON(refresh), GTK_RELIEF_NONE);
	gtk_container_add(GTK_CONTAINER(refresh), image);
	gtk_box_pack_start(GTK_BOX(hbox), refresh, FALSE, FALSE, 0);
	gtk_widget_show(image);
	gtk_widget_show(refresh);

	panels.sources_store = gtk_list_store_new(1, PK_TYPE_SOURCE_INFO);
	gtk_tree_view_set_model(GTK_TREE_VIEW(treeview),
	                        GTK_TREE_MODEL(panels.sources_store));

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(GTK_TREE_VIEW_COLUMN(column), GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	cell = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, cell, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, cell,
			pkg_panels_sources_pixbuf_func, NULL, NULL);
	g_object_set(cell, "yalign", .5, NULL);

	cell = gtk_cell_renderer_text_new();
	gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(cell), 2);
	g_object_set(cell, "xpad", 6, NULL);
	gtk_tree_view_column_pack_start(column, cell, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, cell,
			pkg_panels_sources_text_func, NULL, NULL);

	g_signal_connect(refresh,
	                 "clicked",
	                 G_CALLBACK(pkg_panels_sources_refresh),
	                 NULL);

	g_signal_connect(panels.sources_window,
	                 "delete-event",
	                 G_CALLBACK(sources_window_delete_event),
	                 NULL);

	g_signal_connect(panels.sources_window,
	                 "focus-out-event",
	                 G_CALLBACK(sources_window_focus_out),
	                 NULL);

	g_signal_connect(panels.sources_window,
	                 "focus-in-event",
	                 G_CALLBACK(sources_window_focus_in),
	                 NULL);
}

/**
 * pkg_panels_init:
 *
 * Initializes the panels subsystem and panel widgets are created.
 */
void
pkg_panels_init (void)
{
	static gsize initialized = FALSE;

	if (g_once_init_enter(&initialized)) {
		pkg_panels_create_sources();
		g_once_init_leave(&initialized, TRUE);
	}
}

/**
 * pkg_panels_show_sources:
 *
 * Shows the Source Plugins window.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pkg_panels_show_sources (void)
{
	gtk_widget_show(panels.sources_window);
	gtk_window_present(GTK_WINDOW(panels.sources_window));
}

/**
 * pkg_panels_set_connection:
 * @connection: A #PkConnection.
 *
 * Sets the connection that is used by the active window.  This updates
 * the global panels that are relative to the active connection.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pkg_panels_set_connection (PkConnection *connection)
{
	GError *error = NULL;

	g_return_if_fail(PK_IS_CONNECTION(connection));

	/*
	 * Set the connection.
	 */
	if (panels.conn == connection)
		return;
	else if (panels.conn)
		g_object_unref(panels.conn);
	panels.conn = g_object_ref(connection);

	/*
	 * Clear the list of items.
	 */
	gtk_list_store_clear(panels.sources_store);

	/*
	 * If the connection is not connected, go ahead and connect it.
	 *
	 * TODO: Make this async.
	 */
	if (!pk_connection_connect(connection, &error)) {
		pkg_dialog_warning(GTK_WIDGET(pkg_runtime_get_active_window()), "",
		                   "There was an error connecting to the Perfkit system.",
		                   error ? error->message : _("An uknown occured"),
		                   TRUE);
		return;
	}

	/*
	 * Handle UI changes.
	 */
	pkg_panels_on_set_connection(connection);
}

/**
 * pkg_panels_get_selected_source_plugin:
 *
 * Retreives the currently selected #PkSourceInfo from the Source Plugins
 * window.
 *
 * Returns: A #PkSourceInfo or %NULL if successful; otherwise %NULL.
 * Side effects: None.
 */
PkSourceInfo*
pkg_panels_get_selected_source_plugin (void)
{
	PkSourceInfo *plugin = NULL;
	GtkTreeSelection *sel;
	GtkTreeModel *model = NULL;
	GtkTreeIter iter;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(panels.sources_treeview));
	if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
		gtk_tree_model_get(model, &iter, 0, &plugin, -1);
	}

	return plugin;
}
