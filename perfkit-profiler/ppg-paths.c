/* ppg-paths.c
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

#include "ppg-paths.h"

static gchar *data_dir = NULL;
static gchar *icon_dir = NULL;

void
ppg_paths_init (void)
{
	if (g_getenv("PPG_DATA_DIR")) {
		data_dir = g_strdup(g_getenv("PPG_DATA_DIR"));
	} else {
		data_dir = g_build_filename(PACKAGE_DATA_DIR, "perfkit", NULL);
	}

	icon_dir = g_build_filename(data_dir, "icons", NULL);
}

const gchar *
ppg_paths_get_icon_dir (void)
{
	return icon_dir;
}
