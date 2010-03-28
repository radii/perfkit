/* pkd-memory.c
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
#include <perfkit-daemon/perfkit-daemon.h>

typedef struct
{
	gint size;
	gint resident;
	gint share;
	gint text;
	gint lib;
	gint data;
	gint dt;
} proc_pid_statm;

typedef struct
{
	PkdManifest    *manifest;
	GPid            pid;
	proc_pid_statm  statm;
} MemoryState;

static inline gboolean
proc_pid_statm_read (proc_pid_statm *pstat,
                     GPid            pid)
{
	gint fd;
	gchar path[64];
	gchar buffer[64];
 
	memset (path, 0, sizeof (path));
	snprintf (path, sizeof (path), "/proc/%d/statm", pid);

	fd = open (path, O_RDONLY);
	if (fd < 0) {
		return FALSE;
	}

	if (read (fd, buffer, sizeof (buffer)) < 1) {
		close (fd);
		return FALSE;
	}
 
	sscanf (buffer,
	        "%d %d %d %d %d %d %d",
	        &pstat->size,
	        &pstat->resident,
	        &pstat->share,
	        &pstat->text,
	        &pstat->lib,
	        &pstat->data,
	        &pstat->dt);

	close (fd);

	return TRUE;
}

static void
pkd_memory_cb (PkdSourceSimple *source,
               gpointer         user_data)
{
	MemoryState *state = user_data;
	PkdSample *s;

	/*
	 * Create and deliver our manifest if it has not yet been done.
	 */
	if (G_UNLIKELY(!state->manifest)) {
		state->manifest = pkd_manifest_sized_new(5);
		pkd_manifest_append(state->manifest, "size", G_TYPE_UINT);
		pkd_manifest_append(state->manifest, "resident", G_TYPE_UINT);
		pkd_manifest_append(state->manifest, "share", G_TYPE_UINT);
		pkd_manifest_append(state->manifest, "text", G_TYPE_UINT);
		pkd_manifest_append(state->manifest, "data", G_TYPE_UINT);
		pkd_source_deliver_manifest(PKD_SOURCE(source), state->manifest);
	}

	/*
	 * Retrieve the next sample.
	 */
	if (proc_pid_statm_read(&state->statm, state->pid)) {
		/*
		 * Create our data sample.
		 */
		s = pkd_sample_new();
		pkd_sample_append_uint(s, 1, state->statm.size);
		pkd_sample_append_uint(s, 2, state->statm.resident);
		pkd_sample_append_uint(s, 3, state->statm.share);
		pkd_sample_append_uint(s, 4, state->statm.text);
		pkd_sample_append_uint(s, 5, state->statm.data);

		/*
		 * Deliver the sample.
		 */
		pkd_source_deliver_sample(PKD_SOURCE(source), s);

		/*
		 * We no longer need the sample.
		 */
		pkd_sample_unref(s);
	}
}

static void
pkd_memory_spawn_cb (PkdSourceSimple *source,
                     PkdSpawnInfo    *info,
                     gpointer         user_data)
{
	MemoryState *state = user_data;
	state->pid = info->pid;
}

static void
memory_state_free (gpointer data)
{
	g_slice_free(MemoryState, data);
}

PkdSource*
pkd_memory_new (void)
{
	MemoryState *state;

	state = g_slice_new0(MemoryState);
	return pkd_source_simple_new_full(pkd_memory_cb, state,
	                                  pkd_memory_spawn_cb, FALSE,
	                                  memory_state_free);
}

const PkdStaticSourceInfo pkd_source_plugin = {
	.uid         = "Memory",
	.name        = "Memory Data Source",
	.description = "This source provides information memory usage of a target "
	               "process or the entire system.",
	.version     = "0.1.0",
	.factory     = pkd_memory_new,
};
