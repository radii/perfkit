/* pka-spawn-info.h
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

#if !defined (__PERFKIT_AGENT_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit-agent/perfkit-agent.h> can be included directly."
#endif

#ifndef __PKA_SPAWN_INFO_H__
#define __PKA_SPAWN_INFO_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PKA_TYPE_SPAWN_INFO (pka_spawn_info_get_type())

/**
 * PkaSpawnInfo:
 *
 * #PkaSpawnInfo contains information about how a channel should be started.
 * When a channel is created, using pka_channel_new(), this structure is
 * passed in.  The settings are immutable within the channel going forward.
 *
 * The pid may be set to signify that the data sources within the channel
 * should attach to an existing process.  Otherwise, target should be set
 * to the path of an executable to launch and pid will be set upon launch.
 */
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
} PkaSpawnInfo;

GType         pka_spawn_info_get_type (void) G_GNUC_CONST;
PkaSpawnInfo* pka_spawn_info_copy     (PkaSpawnInfo *spawn_info);
void          pka_spawn_info_free     (PkaSpawnInfo *spawn_info);

G_END_DECLS

#endif /* __PKA_SPAWN_INFO_H__ */
