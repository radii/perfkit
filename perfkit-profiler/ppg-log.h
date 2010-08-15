/* ppg-log.h
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

#ifndef __PPG_LOG_H__
#define __PPG_LOG_H__

#include <glib.h>

G_BEGIN_DECLS

#define DEBUG(f,...)   g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, f, ## __VA_ARGS__)
#define ERROR(f,...)   g_log(G_LOG_DOMAIN, G_LOG_LEVEL_ERROR, f, ## __VA_ARGS__)
#define INFO(f,...)    g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, f, ## __VA_ARGS__)
#define WARNING(f,...) g_log(G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, f, ## __VA_ARGS__)

typedef void (*PpgLogFunc) (const gchar *message,
                            gpointer     user_data);

guint ppg_log_add_listener    (PpgLogFunc func,
                               gpointer   user_data);
void  ppg_log_remove_listener (guint      listener_id);

G_END_DECLS

#endif /* __PPG_LOG_H__ */
