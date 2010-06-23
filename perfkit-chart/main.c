/* main.c
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <stdlib.h>

#include "perfkit-chart.h"

static GOptionEntry options[] = {
	{ NULL }
};

static void
create_scatter_window (void)
{
	GtkWidget *window;
	GtkWidget *scatter;
	GtkAdjustment *x_adj;
	GtkAdjustment *y_adj;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "PkcScatter");
	g_signal_connect(window, "delete-event", gtk_main_quit, NULL);
	gtk_widget_show(window);

	scatter = pkc_scatter_new();
	x_adj = pkc_scatter_get_xadjustment(PKC_SCATTER(scatter));
	y_adj = pkc_scatter_get_yadjustment(PKC_SCATTER(scatter));

	gtk_adjustment_set_lower(x_adj, 0);
	gtk_adjustment_set_upper(x_adj, 100);
	gtk_adjustment_set_page_size(x_adj, 100);
	gtk_adjustment_set_page_increment(x_adj, 10);
	gtk_adjustment_set_value(x_adj, 50);

	gtk_adjustment_set_upper(y_adj, 200);
	gtk_adjustment_set_page_size(y_adj, 200);
	gtk_adjustment_set_page_increment(y_adj, 10);
	gtk_container_set_border_width(GTK_CONTAINER(scatter), 12);
	gtk_container_add(GTK_CONTAINER(window), scatter);
	gtk_widget_show(scatter);
}

gint
main (gint   argc,
      gchar *argv[])
{
	GOptionContext *context;
	GError *error = NULL;

	/* initialize i18n */
	textdomain(GETTEXT_PACKAGE);
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	g_set_application_name(_("PerfkitChart"));

	/* initialize threading early */
	g_thread_init(NULL);

	/* parse command line arguments */
	context = g_option_context_new(_("- PerfkitChart"));
	g_option_context_add_main_entries(context, options, GETTEXT_PACKAGE);
	g_option_context_add_group(context, gtk_get_option_group(FALSE));
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_printerr("%s\n", error->message);
		g_error_free(error);
		return EXIT_FAILURE;
	}

	gtk_clutter_init(&argc, &argv);

	/* make a new window with a scatter plot */
	create_scatter_window();

	gtk_main();

	return EXIT_SUCCESS;
}
