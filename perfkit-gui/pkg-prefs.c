/* pkg-prefs.c
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

#include <clutter/clutter.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "pkg-prefs.h"

static GOptionEntry entries[] = {
	{ NULL }
};

/**
 * pkg_prefs_init:
 * @argc: argument count
 * @argv: vector containing arguments
 * @error: location for a #GError or %NULL.
 *
 * Initializes the preferences subsystem using the arguments from the
 * command line.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 */
gboolean
pkg_prefs_init (gint     *argc,
                gchar  ***argv,
                GError  **error)
{
	GOptionContext *context;

	context = g_option_context_new(_("- A Perfkit Gui"));
	g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);
	g_option_context_add_group(context, gtk_get_option_group(TRUE));
	g_option_context_add_group(context, clutter_get_option_group());
	if (!g_option_context_parse(context, argc, argv, error)) {
		return FALSE;
	}

	return TRUE;
}
