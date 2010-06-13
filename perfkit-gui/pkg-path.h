/* pkg-path.h
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

#if !defined (__PERFKIT_GUI_INSIDE__) && !defined (PERFKIT_GUI_COMPILATION)
#error "Only <perfkit-gui/perfkit-gui.h> can be included directly."
#endif

#ifndef __PKG_PATH_H__
#define __PKG_PATH_H__

#include <glib.h>

G_BEGIN_DECLS

gchar* pkg_path_for_data (const gchar *first_path, ...) G_GNUC_NULL_TERMINATED;

G_END_DECLS

#endif /* __PKG_PATH_H__ */
