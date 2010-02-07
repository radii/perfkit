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

#include <gtk/gtk.h>

#include "pkg-panels.h"

static GtkWidget *sources = NULL;
static PkConnection *conn = NULL;

static void
pkg_panels_create_sources (void)
{
	GtkWidget *vbox,
	          *hbox,
	          *scroller,
	          *treeview,
	          *refresh,
	          *image;

	sources = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(sources), "");
	gtk_window_set_default_size(GTK_WINDOW(sources), 200, 400);
	gtk_window_set_type_hint(GTK_WINDOW(sources), GDK_WINDOW_TYPE_HINT_UTILITY);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(sources), vbox);
	gtk_widget_show(vbox);

	scroller = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
	                               GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_NEVER);
	gtk_box_pack_start(GTK_BOX(vbox), scroller, TRUE, TRUE, 0);
	gtk_widget_show(scroller);

	treeview = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(scroller), treeview);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
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

void
pkg_panels_show_sources (void)
{
	gtk_widget_show(sources);
}

void
pkg_panels_set_connection (PkConnection *connection)
{
	if (connection == conn) {
		return;
	} else if (conn) {
		g_object_unref(conn);
		conn = NULL;
	}

	g_debug("Set sources list");

	conn = g_object_ref(connection);
}
