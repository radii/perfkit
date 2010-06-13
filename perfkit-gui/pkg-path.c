/* pkg-path.c
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdarg.h>

#include "pkg-log.h"
#include "pkg-path.h"

static const gchar*
pkg_path_get_data_dir (void)
{
	static gchar *data_dir = NULL;
	gchar *path;

	if (g_once_init_enter((gsize *)&data_dir)) {
		if (g_getenv("PERFKIT_GUI_DATA_PATH")) {
			path = g_strdup(g_getenv("PERFKIT_GUI_DATA_PATH"));
		} else {
			path = g_build_filename(PACKAGE_DATA_DIR, "perfkit", NULL);
		}
		g_once_init_leave((gsize *)&data_dir, (gsize)path);
	}
	return data_dir;
}

gchar*
pkg_path_for_data (const gchar *first_path, ...)
{
	GPtrArray *strv;
	va_list args;
	gchar *arg;

	g_return_val_if_fail(first_path != NULL, NULL);

	ENTRY;
	strv = g_ptr_array_new();
	g_ptr_array_add(strv, (gpointer)pkg_path_get_data_dir());
	g_ptr_array_add(strv, (gpointer)first_path);
	va_start(args, first_path);
	while ((arg = va_arg(args, gchar*))) {
		g_ptr_array_add(strv, (gpointer)arg);
	}
	va_end(args);
	g_ptr_array_add(strv, NULL);
	arg = g_build_filenamev((gchar **)strv->pdata);
	g_ptr_array_unref(strv);
	RETURN(arg);
}
