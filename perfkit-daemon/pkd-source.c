/* pkd-source.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pkd-source.h"

/**
 * SECTION:pkd-source
 * @title: PkdSource
 * @short_description: Data collection sources
 *
 * #PkdSource represents a data collection source.  Sources are responsible
 * for creating samples which contain their collected data.
 */

G_DEFINE_ABSTRACT_TYPE (PkdSource, pkd_source, G_TYPE_OBJECT)

struct _PkdSourcePrivate
{
	gpointer dummy;
};

static gboolean
noop_needs_spawn (PkdSource *source)
{
	return FALSE;
}

static void
pkd_source_finalize (GObject *object)
{
	G_OBJECT_CLASS (pkd_source_parent_class)->finalize (object);
}

static void
pkd_source_class_init (PkdSourceClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkd_source_finalize;
	g_type_class_add_private (object_class, sizeof (PkdSourcePrivate));

	klass->needs_spawn = noop_needs_spawn;
}

static void
pkd_source_init (PkdSource *source)
{
	source->priv = G_TYPE_INSTANCE_GET_PRIVATE ((source),
	                                            PKD_TYPE_SOURCE,
	                                            PkdSourcePrivate);
}

/**
 * pkd_source_new:
 *
 * Creates a new instance of #PkdSource.
 *
 * Return value: the newly created #PkdSource instance.
 */
PkdSource*
pkd_source_new (void)
{
	return g_object_new (PKD_TYPE_SOURCE, NULL);
}

/**
 * pkd_source_needs_spawn:
 * @source: A #PkdSource
 *
 * Checks to see if the source needs to spawn the child process to function
 * correctly.
 *
 * Return value: TRUE if the source is required to spawn the child process
 */
gboolean
pkd_source_needs_spawn (PkdSource *source)
{
	g_return_val_if_fail (PKD_IS_SOURCE (source), FALSE);
	PKD_SOURCE_GET_CLASS (source)->needs_spawn (source);
}

/**
 * pkd_source_spawn:
 * @source: A #PkdSource
 *
 * Spawns the target executable if needed.
 *
 * Return value: %TRUE if the child is spawned.
 */
gboolean
pkd_source_spawn (PkdSource  *source,
                  GError    **error)
{
	g_return_val_if_fail (PKD_IS_SOURCE (source), FALSE);
	/* TODO */
	return FALSE;
}

gboolean
pkd_source_start (PkdSource  *source,
                  GError    **error)
{
	g_return_val_if_fail (PKD_IS_SOURCE (source), FALSE);
	/* TODO */
	return TRUE;
}
