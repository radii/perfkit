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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <egg-time.h>

#include "pkd-memory.h"

G_DEFINE_TYPE (PkdMemory, pkd_memory, PKD_TYPE_SOURCE)

struct _PkdMemoryPrivate
{
	PkdManifest *manifest;
	glong        span;
	GMutex      *mutex;
	GCond       *cond;
	GPid         pid;
	gboolean     running;
};

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

static gpointer
thread_func (gpointer data)
{
	PkdMemory *memory = data;
	PkdMemoryPrivate *priv;
	PkdManifest *m;
	GPid pid;
	GTimeVal tv;
	glong span;
	gboolean done = FALSE;
	proc_pid_statm st;
	PkdSample *s;

	g_assert(memory);

	priv = memory->priv;
	pid = priv->pid;
	span = priv->span;

	/*
	 * Create our manifest.
	 */
	m = pkd_manifest_sized_new(5);
	pkd_manifest_append(m, "size", G_TYPE_UINT);
	pkd_manifest_append(m, "resident", G_TYPE_UINT);
	pkd_manifest_append(m, "share", G_TYPE_UINT);
	pkd_manifest_append(m, "text", G_TYPE_UINT);
	pkd_manifest_append(m, "data", G_TYPE_UINT);

	/*
	 * Deliver the manifest to the subscription.
	 */
	pkd_source_deliver_manifest(data, m);

	do {
		/*
		 * Generate the absolute timeout for our condition before we sample
		 * so there is less sampling drift.
		 */
		g_get_current_time(&tv);
		g_time_val_add(&tv, span);

		/*
		 * Parse /proc/pid/statm for memory statistics.
		 */
		if (proc_pid_statm_read(&st, pid)) {
			/*
			 * Generate sample.
			 */
			s = pkd_sample_new();
			pkd_sample_append_uint(s, 1, st.size);
			pkd_sample_append_uint(s, 2, st.resident);
			pkd_sample_append_uint(s, 3, st.share);
			pkd_sample_append_uint(s, 4, st.text);
			pkd_sample_append_uint(s, 5, st.data);

			/*
			 * Deliver sample to subscription.
			 */
			pkd_source_deliver_sample(data, s);

			/*
			 * Done with the sample.
			 */
			pkd_sample_unref(s);
		}

		g_mutex_lock(priv->mutex);

		/*
		 * Check to see if we are done sampling.
		 */
		if ((done = !priv->running))
			goto unlock;

		/*
		 * Block until our timeout has passed or we have stopped.
		 */
		g_cond_timed_wait(priv->cond, priv->mutex, &tv);

		/*
		 * Check if we finished sampling while sleeping.
		 */
		done = !priv->running;

unlock:
		g_mutex_unlock(priv->mutex);
	} while (!done);

	/*
	 * Finished with our manifest.
	 */
	pkd_manifest_unref(m);

	return NULL;
}

static void
pkd_memory_notify_stopped (PkdSource *source)
{
	PkdMemoryPrivate *priv;

	priv = PKD_MEMORY(source)->priv;

	g_mutex_lock(priv->mutex);

	/*
	 * Notify the thread we have completed.
	 */
	priv->running = FALSE;
	g_cond_signal(priv->cond);

	g_mutex_unlock(priv->mutex);
}

static void
pkd_memory_notify_start (PkdSource    *source,
                         PkdSpawnInfo *spawn_info)
{
	PkdMemoryPrivate *priv;

	priv = PKD_MEMORY(source)->priv;
	priv->pid = spawn_info->pid;
	priv->running = TRUE;

	g_message("Starting memory monitor thread for source %d.",
	          pkd_source_get_id(source));
	g_thread_create(thread_func, source, FALSE, NULL);
}

static void
pkd_memory_finalize (GObject *object)
{
	PkdMemoryPrivate *priv;

	g_return_if_fail (PKD_IS_MEMORY (object));

	priv = PKD_MEMORY (object)->priv;

	if (priv->running) {
		pkd_memory_notify_stopped(PKD_SOURCE(object));
	}

	g_mutex_free(priv->mutex);
	g_cond_free(priv->cond);

	G_OBJECT_CLASS(pkd_memory_parent_class)->finalize(object);
}

static void
pkd_memory_class_init (PkdMemoryClass *klass)
{
	GObjectClass *object_class;
	PkdSourceClass *source_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pkd_memory_finalize;
	g_type_class_add_private(object_class, sizeof(PkdMemoryPrivate));

	source_class = PKD_SOURCE_CLASS(klass);
	source_class->notify_started = pkd_memory_notify_start;
	source_class->notify_stopped = pkd_memory_notify_stopped;
}

static void
pkd_memory_init (PkdMemory *memory)
{
	memory->priv = G_TYPE_INSTANCE_GET_PRIVATE(memory,
	                                           PKD_TYPE_MEMORY,
	                                           PkdMemoryPrivate);
	memory->priv->span = G_USEC_PER_SEC;
	memory->priv->mutex = g_mutex_new();
	memory->priv->cond = g_cond_new();
}

PkdSource*
pkd_memory_new (void)
{
	return g_object_new(PKD_TYPE_MEMORY, NULL);
}

const PkdStaticSourceInfo pkd_source_plugin = {
	.uid         = "Memory",
	.name        = "Memory Data Source",
	.description = "This source provides information memory usage of a target "
	               "process or the entire system.",
	.version     = "0.1.0",
	.factory     = pkd_memory_new,
};
