/* ppg-path.c
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

#include "ppg-path.h"

G_CONST_RETURN gchar*
ppg_path_get_data_dir (void)
{
	static gsize initialized = FALSE;
	static gchar *data_dir = NULL;
	
	if (g_once_init_enter(&initialized)) {
		if (g_getenv("PERFKIT_PROFILER_DATA_PATH")) {
			data_dir = (gchar *)g_getenv("PERFKIT_PROFILER_DATA_PATH");
		} else {
			data_dir = (gchar *)PACKAGE_DATA_DIR;
		}
		g_once_init_leave(&initialized, TRUE);
	}

	return data_dir;
}

/**
 * ppg_path_build:
 * @first_part: First part of the path.
 *
 * Builds a path to a file using the proper data prefix.
 *
 * Returns: A newly allocated string to be freed with g_free().
 * Side effects: None.
 */
gchar*
ppg_path_build (const gchar *first_part, /* IN */
                ...)
{
	va_list args;
	GPtrArray *ptr;
	const gchar *path;
	gchar *ret;

	ptr = g_ptr_array_new();
	path = first_part;

	/*
	 * Add base paths.
	 */
	g_ptr_array_add(ptr, (gpointer)ppg_path_get_data_dir());

	/*
	 * Extract va args.
	 */
	va_start(args, first_part);
	do {
		g_ptr_array_add(ptr, (gpointer)path);
		path = va_arg(args, const gchar*);
	} while (path != NULL);
	va_end(args);

	/*
	 * Null sentinal.
	 */
	g_ptr_array_add(ptr, NULL);

	/*
	 * Build filename.
	 */
	ret = g_build_filenamev((gchar **)ptr->pdata);
	g_ptr_array_free(ptr, TRUE);
	return ret;
}
