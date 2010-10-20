/* ppg-prefs.h
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

#ifndef __PPG_PREFS_H__
#define __PPG_PREFS_H__

#include <gio/gio.h>

G_BEGIN_DECLS

gboolean   ppg_prefs_init                    (gint *argc, gchar ***argv);
GSettings* ppg_prefs_get_project_settings    (void);
gchar*     ppg_prefs_get_project_default_dir (void);
void       ppg_prefs_set_project_default_dir (const gchar *default_dir);

G_END_DECLS

#endif /* __PPG_PREFS_H__ */
