/* pka-memory.c
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
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

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <perfkit-agent/perfkit-agent.h>

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "Memory"

typedef struct
{
	PkaManifest *manifest;
	GPid pid;
	gint size;
	gint resident;
	gint share;
	gint text;
	gint lib;
	gint data;
	gint dt;
} Memory;

/*
 * Read /proc/pid/statm and store to memory state.
 */
static inline gboolean
memory_read (Memory *state)
{
	gint fd;
	gchar path[64];
	gchar buffer[64];
 
 	ENTRY;
	memset(path, 0, sizeof(path));
	snprintf(path, sizeof(path), "/proc/%d/statm", state->pid);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		RETURN(FALSE);
	}
	if (read(fd, buffer, sizeof(buffer)) < 1) {
		close(fd);
		RETURN(FALSE);
	}
	sscanf(buffer,
	       "%d %d %d %d %d %d %d",
	       &state->size,
	       &state->resident,
	       &state->share,
	       &state->text,
	       &state->lib,
	       &state->data,
	       &state->dt);
	close(fd);
	RETURN(TRUE);
}

/*
 * Handle a sample callback from the PkaSourceSimple.
 */
static void
memory_sample (PkaSourceSimple *source,
               gpointer         user_data)
{
	Memory *state = user_data;
	PkaSample *s;

	ENTRY;

	/*
	 * Create and deliver our manifest if it has not yet been done.
	 */
	if (G_UNLIKELY(!state->manifest)) {
		TRACE(Memory, "Initializing manifest");
		state->manifest = pka_manifest_sized_new(5);
		pka_manifest_append(state->manifest, "size", G_TYPE_UINT);
		pka_manifest_append(state->manifest, "resident", G_TYPE_UINT);
		pka_manifest_append(state->manifest, "share", G_TYPE_UINT);
		pka_manifest_append(state->manifest, "text", G_TYPE_UINT);
		pka_manifest_append(state->manifest, "data", G_TYPE_UINT);
		pka_source_deliver_manifest(PKA_SOURCE(source), state->manifest);
	}

	/*
	 * Retrieve the next sample.
	 */
	if (memory_read(state)) {
		/*
		 * Create our data sample.
		 */
		s = pka_sample_new();
		pka_sample_append_uint(s, 1, state->size);
		pka_sample_append_uint(s, 2, state->resident);
		pka_sample_append_uint(s, 3, state->share);
		pka_sample_append_uint(s, 4, state->text);
		pka_sample_append_uint(s, 5, state->data);

		/*
		 * Deliver the sample.
		 */
		pka_source_deliver_sample(PKA_SOURCE(source), s);

		/*
		 * We no longer need the sample.
		 */
		pka_sample_unref(s);
	}

	EXIT;
}

/*
 * Handle a spawn event from the PkaSourceSimple.
 */
static void
memory_spawn (PkaSourceSimple *source,
              PkaSpawnInfo    *spawn_info,
              gpointer         user_data)
{
	ENTRY;
	((Memory *)user_data)->pid = spawn_info->pid;
	EXIT;
}

/*
 * Free the memory state when source is destroyed.
 */
static void
memory_free (gpointer data)
{
	Memory *state = data;

	g_return_if_fail(state != NULL);

	ENTRY;
	if (state->manifest) {
		pka_manifest_unref(state->manifest);
	}
	g_slice_free(Memory, data);
	EXIT;
}

/*
 * Create a new PkaSourceSimple for memory sampling.
 */
GObject*
memory_new (GError **error)
{
	Memory *memory;

	ENTRY;
	memory = g_slice_new0(Memory);
	RETURN(G_OBJECT(pka_source_simple_new_full(memory_sample,
	                                           memory_spawn,
	                                           memory,
	                                           memory_free)));
}

const PkaPluginInfo pka_plugin_info = {
	.id          = "Memory",
	.name        = "Memory usage",
	.description = "System and process memory usage.",
	.version     = "0.1.1",
	.copyright   = "Copyright 2010 Christian Hergert",
	.factory     = memory_new,
	.plugin_type = PKA_PLUGIN_SOURCE,
};
