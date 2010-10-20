/* ppg-runtime.h
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

#ifndef __PPG_RUNTIME_H__
#define __PPG_RUNTIME_H__

#include <glib.h>

G_BEGIN_DECLS

gboolean ppg_runtime_init      (gint   *argc,
                                gchar **argv[]);
void     ppg_runtime_quit      (void);
void     ppg_runtime_quit_fast (gint    code);
gint     ppg_runtime_run       (void);
void     ppg_runtime_shutdown  (void);
gboolean ppg_runtime_try_quit  (void);

G_END_DECLS

#endif /* __PPG_RUNTIME_H__ */
