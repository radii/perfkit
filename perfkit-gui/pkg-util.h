/* pkg-util.h
 *
 * Copyright (C) 2007 David Zeuthen
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

#ifndef __PKG_UTIL_H__
#define __PKG_UTIL_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

GtkWidget* pkg_util_dialog_warning (GtkWidget   *parent,
                                    const gchar *title,
                                    const gchar *primary,
                                    const gchar *secondary,
                                    gboolean     use_expander);
void       pkg_util_get_mix_color  (GtkWidget    *widget,
                                    GtkStateType  state,
                                    gchar        *color_buf,
                                    gsize         color_buf_size);

G_END_DECLS

#endif /* __PKG_UTIL_H__ */
