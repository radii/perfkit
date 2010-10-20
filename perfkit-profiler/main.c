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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <stdlib.h>

#include "ppg-runtime.h"
#include "ppg-welcome-dialog.h"

gint
main (gint   argc,
      gchar *argv[])
{
	GtkWidget *welcome;
	gint ret;

	g_thread_init(NULL);

	/*
	 * Initialize i18n before parsing command line arguments.
	 */
	textdomain(GETTEXT_PACKAGE);
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	g_set_application_name(_(PRODUCT_NAME));

	/*
	 * Initialize the runtime system including preferences,
	 * configuration, and toolkits. Exit immediately if we fail to
	 * cleanly initialize.
	 */
	if (!ppg_runtime_init(&argc, &argv)) {
		return EXIT_FAILURE;
	}

	welcome = g_object_new(PPG_TYPE_WELCOME_DIALOG,
	                       "visible", TRUE,
	                       "window-position", GTK_WIN_POS_CENTER,
	                       NULL);
	gtk_window_present(GTK_WINDOW(welcome));

	/*
	 * Run the application. If non-zero is returned, then the system
	 * wants to fast fail and exit so don't bother cleaning up.
	 */
	if (!(ret = ppg_runtime_run())) {
		ppg_runtime_shutdown();
	}

	return ret;
}
