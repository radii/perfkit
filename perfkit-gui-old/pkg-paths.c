/* pkg-paths.c
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

#include "pkg-paths.h"

/**
 * pkg_paths_get_data_dir:
 *
 * Retrieves the directory containing data information for the application.
 *
 * Return value: a string containing the data directory.  This value should
 *   not be modified or freed.
 */
const gchar *
pkg_paths_get_data_dir (void)
{
	const gchar *data_dir = NULL;

	if (G_UNLIKELY (!data_dir)) {
		gchar *root = (gchar*)g_getenv ("PERFKIT_GUI_DATA_PATH");
		if (root)
			data_dir = g_strdup(root);
		else
			data_dir = g_build_filename (PACKAGE_DATA_DIR, "perfkit-gui", NULL);
	}

	return data_dir;
}

const gchar *
pkg_paths_get_locale_dir (void)
{
	static gchar *locale_dir = NULL;

	if (!G_UNLIKELY (locale_dir)) {
		locale_dir = g_build_filename (PACKAGE_DATA_DIR, "locale", NULL);
	}

	return locale_dir;
}


gchar*
pkg_paths_build_data_path (const gchar *first, ...)
{
	va_list args;
	gchar **argv;
	const gchar *part;
	gint c;

	g_return_val_if_fail(first != NULL, NULL);

	c = 1;
	argv = g_malloc0(sizeof(gchar*) * c);
	argv[0] = g_strdup(pkg_paths_get_data_dir());
	part = first;

	va_start(args, first);

	do {
		argv = g_realloc(argv, sizeof(gchar*) * (c + 2));
		argv[c++] = g_strdup(part);
		part = va_arg(args, gchar*);
	} while (part);

	va_end(args);

	argv[c] = NULL;

	return g_build_filenamev(argv);
}
