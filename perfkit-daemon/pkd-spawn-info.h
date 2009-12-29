/* pkd-spawn-info.h
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

#ifndef __PKD_SPAWN_INFO_H__
#define __PKD_SPAWN_INFO_H__

#include <glib.h>

typedef struct
{
	GPid    pid;
	gchar  *target;
	gchar **args;
	gchar **env;
	gchar  *working_dir;
	gint    standard_input;
	gint    standard_output;
	gint    standard_error;
} PkdSpawnInfo;

#endif /* __PKD_SPAWN_INFO_H__ */
