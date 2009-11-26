/* memory.c
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
#include <config.h>
#endif

#include <perfkit-daemon/pkd-source.h>

#include "plugin.h"

#define MEMORY_TYPE_SOURCE				(memory_source_get_type ())
#define MEMORY_SOURCE(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), MEMORY_TYPE_SOURCE, MemorySource))
#define MEMORY_SOURCE_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MEMORY_TYPE_SOURCE, MemorySource const))
#define MEMORY_SOURCE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), MEMORY_TYPE_SOURCE, MemorySourceClass))
#define MEMORY_IS_SOURCE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEMORY_TYPE_SOURCE))
#define MEMORY_IS_SOURCE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MEMORY_TYPE_SOURCE))
#define MEMORY_SOURCE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), MEMORY_TYPE_SOURCE, MemorySourceClass))

typedef struct _MemorySource		MemorySource;
typedef struct _MemorySourceClass	MemorySourceClass;
typedef struct _MemorySourcePrivate	MemorySourcePrivate;

struct _MemorySource
{
	PkdSource parent;
	
	MemorySourcePrivate *priv;
};

struct _MemorySourceClass
{
	PkdSourceClass parent_class;
};

G_DEFINE_TYPE (MemorySource, memory_source, PKD_TYPE_SOURCE)

struct _MemorySourcePrivate
{
	GPid pid;
};

static void
memory_source_finalize (GObject *object)
{
	G_OBJECT_CLASS (memory_source_parent_class)->finalize (object);
}

static gboolean
memory_source_start (PkdSource  *source,
                     GError    **error)
{
	g_debug ("Start Sampling");
	return TRUE;
}

static void
memory_source_class_init (MemorySourceClass *klass)
{
	GObjectClass   *object_class;
	PkdSourceClass *source_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = memory_source_finalize;
	g_type_class_add_private (object_class, sizeof (MemorySourcePrivate));

	source_class = PKD_SOURCE_CLASS (klass);
	source_class->start = memory_source_start;
}

static void
memory_source_init (MemorySource *source)
{
	source->priv = G_TYPE_INSTANCE_GET_PRIVATE(source,
	                                           MEMORY_TYPE_SOURCE,
	                                           MemorySourcePrivate);
}

SOURCE_PLUGIN (Memory, memory, MEMORY, MEMORY_TYPE_SOURCE)
