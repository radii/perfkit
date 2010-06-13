/* main.c
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

#include <stdlib.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "perfkit-gui.h"

static GOptionEntry options[] = {
	{ NULL }
};

void pkg_log_init     (gboolean _stdout, const gchar *filename);
void pkg_log_shutdown (void);

gint
main (gint   argc,
      gchar *argv[])
{
	GOptionContext *context;
	GError *error = NULL;
	GtkWidget *window;

	/* initialize i18n */
	textdomain(GETTEXT_PACKAGE);
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	g_set_application_name(_("perfkit-gui"));

	/* initialize threading early */
	g_thread_init(NULL);

	/* parse command line arguments */
	context = g_option_context_new(_("- Perfkit Administration Gui"));
	g_option_context_add_main_entries(context, options, GETTEXT_PACKAGE);
	g_option_context_add_group(context, gtk_get_option_group(TRUE));
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_printerr("%s\n", error->message);
		g_error_free(error);
		return EXIT_FAILURE;
	}

	pkg_log_init(TRUE, NULL);
	gtk_window_set_default_icon_name("clock");
	window = pkg_window_new();
	pkg_window_connect_to(PKG_WINDOW(window), "dbus://");
	g_signal_connect(window, "delete-event", gtk_main_quit, NULL);
	gtk_widget_show(window);
	gtk_main();

	return EXIT_SUCCESS;
}
