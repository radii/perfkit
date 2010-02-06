/* pkg-paths.h
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

#if !defined (__PERFKIT_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit-gui/perfkit-gui.h> can be included directly."
#endif

#ifndef __PERFKIT_PATHS_H__
#define __PERFKIT_PATHS_H__

#include <glib.h>

G_BEGIN_DECLS

const gchar* pkg_paths_get_data_dir    (void);
const gchar* pkg_paths_get_locale_dir  (void);
gchar*       pkg_paths_build_data_path (const gchar *first,
                                        ...) G_GNUC_NULL_TERMINATED;

G_END_DECLS

#endif /* __PERFKIT_PATHS_H__ */
