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

#include "pkd-memory.h"

G_DEFINE_TYPE (PkdMemory, pkd_memory, PKD_TYPE_SOURCE)

/**
 * SECTION:pkd-memory
 * @title: PkdMemory
 * @short_description: 
 *
 * 
 */

const PkdStaticSourceInfo pkd_source_plugin = {
	.uid         = "Memory",
	.name        = "Memory Data Source",
	.description = "This source provides information memory usage of a target "
	               "process or the entire system.",
	.version     = "0.1.0",
	.factory     = pkd_memory_new,
};

struct _PkdMemoryPrivate
{
	gpointer dummy;
};

/**
 * pkd_memory_new:
 *
 * Creates a new instance of #PkdMemory.
 *
 * Return value: the newly created #PkdMemory instance.
 */
PkdSource*
pkd_memory_new (void)
{
	return g_object_new (PKD_TYPE_MEMORY, NULL);
}

static void
pkd_memory_finalize (GObject *object)
{
	PkdMemoryPrivate *priv;

	g_return_if_fail (PKD_IS_MEMORY (object));

	priv = PKD_MEMORY (object)->priv;

	G_OBJECT_CLASS(pkd_memory_parent_class)->finalize(object);
}

static void
pkd_memory_class_init (PkdMemoryClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pkd_memory_finalize;
	g_type_class_add_private(object_class, sizeof(PkdMemoryPrivate));
}

static void
pkd_memory_init (PkdMemory *memory)
{
	memory->priv = G_TYPE_INSTANCE_GET_PRIVATE(memory,
	                                           PKD_TYPE_MEMORY,
	                                           PkdMemoryPrivate);
}
