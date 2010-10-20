/* ppg-prefs.c
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

#include <glib/gi18n.h>

#include "ppg-prefs.h"

#define PROJECT_SCHEMA      "org.perfkit.profiler.project"
#define PROJECT_DEFAULT_DIR "default-dir"

static GSettings *project_settings = NULL;

gboolean
ppg_prefs_init (gint    *argc,
                gchar ***argv)
{
    project_settings = g_settings_new(PROJECT_SCHEMA);

	return TRUE;
}

GSettings*
ppg_prefs_get_project_settings (void)
{
	return project_settings;
}

gchar *
ppg_prefs_get_project_default_dir (void)
{
    return g_settings_get_string(project_settings, PROJECT_DEFAULT_DIR);
}

void
ppg_prefs_set_project_default_dir (const gchar *default_dir)
{
    g_settings_set_string(project_settings, PROJECT_DEFAULT_DIR, default_dir);
}
