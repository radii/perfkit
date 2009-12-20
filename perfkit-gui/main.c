/* main.c
 * 
 * Copyright (C) 2009 Christian Hergert
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

#include <glib.h>
#include <glib-object.h>
#include <glib/gthread.h>
#include <glib/gi18n.h>

#include "pkg-paths.h"
#include "pkg-runtime.h"

static GOptionEntry entries[] = {
	{ NULL }
};

gint
main (gint argc,
      gchar *argv[])
{
	GOptionContext *context  = NULL;
	GError         *error    = NULL;
	const gchar    *data_dir = NULL;

	/* initialize i18n */
	data_dir = PACKAGE_LOCALE_DIR;
	bindtextdomain (GETTEXT_PACKAGE, data_dir);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* parse command line arguments */
	context = g_option_context_new ("- Perfkit user interface");
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_printerr ("%s\n", error->message);
		return EXIT_FAILURE;
	}

	/* initialize libraries */
	g_thread_init (NULL);
	g_type_init ();

	/* initialize runtime and block on main loop */
	pkg_runtime_initialize ();
	pkg_runtime_run ();
	pkg_runtime_shutdown ();

	return EXIT_SUCCESS;
}

