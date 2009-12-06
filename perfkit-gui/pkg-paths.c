/* pkg-paths.c
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
		const gchar *root = g_getenv ("PKG_DATA_PATH");
		if (!root)
			root = PACKAGE_DATA_DIR;
		data_dir = g_build_filename (root, "perfkit-gui", NULL);
	}

	return data_dir;
}

const gchar *
pkg_dirs_get_locale_dir (void)
{
	static gchar *locale_dir = NULL;

	if (!G_UNLIKELY (locale_dir))
		locale_dir = g_build_filename (PACKAGE_DATA_DIR, "locale", NULL);

	return locale_dir;
}
