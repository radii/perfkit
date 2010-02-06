/* main.c
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

#include <stdlib.h>
#include <glib/gthread.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "pkg-paths.h"
#include "pkg-runtime.h"
#include "pkg-welcome.h"

gint
main (gint   argc,
      gchar *argv[])
{
	const gchar *data_dir = NULL;
	GError *error = NULL;
	GtkWidget *welcome;
	gint i;

	/* initialize i18n */
	data_dir = PACKAGE_LOCALE_DIR;
	bindtextdomain(GETTEXT_PACKAGE, data_dir);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	/* initialize threading */
	g_thread_init(NULL);

	/* initialize runtime */
	if (!pkg_runtime_initialize(&argc, &argv, &error)) {
		g_printerr("%s\n", error->message);
		return EXIT_FAILURE;
	}

	/* show the welcome window */
	welcome = pkg_welcome_new();
	gtk_widget_show(welcome);

	/* block on mainloop */
	pkg_runtime_run();

	/* cleanup after runtime */
	pkg_runtime_shutdown();

	return EXIT_SUCCESS;
}
