/* ppg-panels.c
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

#include "ppg-panels.h"

typedef struct
{
	GtkWidget *sources;
	GtkWidget *sources_tv;
	GtkWidget *sources_refresh;

} PpgPanels;

static PpgPanels panels = { 0 };

void
ppg_panels_sources_show (void)
{
	gtk_window_present(GTK_WINDOW(panels.sources));
}

void
ppg_panels_sources_hide (void)
{
	gtk_widget_hide(panels.sources);
}

void
ppg_panels_init (void)
{
	GtkWidget *scroller;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *image;

	/*
	 * Sources panels.
	 */
	panels.sources = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(panels.sources), "");
	gtk_window_set_default_size(GTK_WINDOW(panels.sources), 200, 375);
	gtk_window_set_type_hint(GTK_WINDOW(panels.sources),
	                         GDK_WINDOW_TYPE_HINT_UTILITY);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(panels.sources), vbox);
	gtk_widget_show(vbox);
	scroller = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
	                               GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroller),
	                                    GTK_SHADOW_NONE);
	gtk_box_pack_start(GTK_BOX(vbox), scroller, TRUE, TRUE, 0);
	gtk_widget_show(scroller);
	panels.sources_tv = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(scroller), panels.sources_tv);
	gtk_widget_show(panels.sources_tv);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_widget_show(hbox);
	panels.sources_refresh = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(panels.sources_refresh), GTK_RELIEF_NONE);
	gtk_box_pack_start(GTK_BOX(hbox), panels.sources_refresh, FALSE, TRUE, 0);
	gtk_widget_show(panels.sources_refresh);
	image = gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU);
	gtk_container_add(GTK_CONTAINER(panels.sources_refresh), image);
	gtk_widget_show(image);
	g_signal_connect(panels.sources,
	                 "delete-event",
	                 G_CALLBACK(gtk_widget_hide_on_delete),
	                 NULL);
}
