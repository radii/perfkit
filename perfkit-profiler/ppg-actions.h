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

#ifndef __PPG_ACTIONS_H__
#define __PPG_ACTIONS_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef GtkAction* (*PpgActionFunc) (GtkWidget *widget);

void ppg_actions_init                    (void);
void ppg_actions_register                (GType                 widget_type,
                                          GType                 action_type);
void ppg_actions_register_func           (GType                 widget_type,
                                          PpgActionFunc         factory);
void ppg_actions_register_entries        (GType                 widget_type,
                                          GtkActionEntry       *entries,
                                          guint                 n_entries);
void ppg_actions_register_toggle_entries (GType                 widget_type,
                                          GtkToggleActionEntry *entries,
                                          guint                 n_entries);
void ppg_actions_load                    (GtkWidget            *widget,
                                          GtkAccelGroup        *accel_group,
                                          GtkActionGroup       *action_group);

G_END_DECLS

#endif /* __PPG_ACTIONS_H__ */
