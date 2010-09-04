/* ppg-actions.h
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

#if !defined (__PERFKIT_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit-profiler/perfkit-profiler.h> can be included directly."
#endif

#ifndef __PPG_ACTIONS_H__
#define __PPG_ACTIONS_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

void ppg_actions_load                (GtkWidget      *widget,
                                      GtkActionGroup *action_group);
void ppg_actions_load_from_ui_string (GtkWidget      *widget,
                                      GtkActionGroup *action_group,
                                      const gchar    *data,
                                      gssize          data_len,
                                      const gchar    *first_path,
                                      ...);
void ppg_actions_register            (GType           widget_type,
                                      GType           action_type);
void ppg_actions_unregsiter          (GType           widget_type,
                                      GType           action_type);

G_END_DECLS

#endif /* __PPG_ACTIONS_H__ */
