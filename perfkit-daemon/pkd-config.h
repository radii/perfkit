/* pkd-config.h
 *
 * Copyright (C) 2009 Christian Hergert
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

#if !defined (__PERFKIT_DAEMON_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit-daemon/perfkit-daemon.h> can be included directly."
#endif

#ifndef __PKD_CONFIG_H__
#define __PKD_CONFIG_H__

#include <glib.h>

G_BEGIN_DECLS

void     pkd_config_init         (const gchar *filename);
gchar*   pkd_config_get_string   (const gchar *group,
                                  const gchar *key,
                                  const gchar *default_);
gboolean pkd_config_get_boolean  (const gchar *group,
                                  const gchar *key,
                                  gboolean     default_);
gint     pkd_config_get_integer  (const gchar *group,
                                  const gchar *key,
                                  gint         default_);

G_END_DECLS

#endif /* __PKD_CONFIG_H__ */
