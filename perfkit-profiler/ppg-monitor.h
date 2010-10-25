/* ppg-monitor.h
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

#ifndef __PPG_MONITOR_H__
#define __PPG_MONITOR_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

void       ppg_monitor_init    (void);
GtkWidget* ppg_monitor_cpu_new (void);
GtkWidget* ppg_monitor_mem_new (void);
GtkWidget* ppg_monitor_net_new (void);

G_END_DECLS

#endif /* __PPG_MONITOR_H__ */
