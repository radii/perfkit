/* pk-sources.c
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "pk-sources.h"
#include "pk-connection.h"
#include "pk-connection-priv.h"

/**
 * SECTION:pk-sources
 * @title: PkSources
 * @short_description: Perfkit sources service
 *
 * #PkSources represents the sources service of a remove Perfkit daemon.
 * It can be used to add, remove, and retrieve #PkSource<!-- -->'s.
 *
 * A #PkSources instance can be retrieved by calling
 * pk_connection_get_sources() on a connected #PkConnection.
 */

G_DEFINE_TYPE (PkSources, pk_sources, G_TYPE_OBJECT)

struct _PkSourcesPrivate
{
	PkConnection *connection;
};

static void
pk_sources_finalize (GObject *object)
{
	G_OBJECT_CLASS (pk_sources_parent_class)->finalize (object);
}

static void
pk_sources_class_init (PkSourcesClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_sources_finalize;
	g_type_class_add_private (object_class, sizeof (PkSourcesPrivate));
}

static void
pk_sources_init (PkSources *sources)
{
	sources->priv = G_TYPE_INSTANCE_GET_PRIVATE (sources,
	                                              PK_TYPE_SOURCES,
	                                              PkSourcesPrivate);
}

PkSources*
pk_sources_new (PkConnection *connection)
{
	PkSources *sources;

	sources = g_object_new (PK_TYPE_SOURCES, NULL);
	sources->priv->connection = connection;

	return sources;
}

/**
 * pk_sources_get_types:
 * @sources: A #PkSources
 *
 * Retrieves the list data source types that are available on the
 * remote system.
 *
 * Return value: A #GStrv that contains the types of available data
 *   sources.  The value should be freed with g_strfreev().
 *
 * Side effects: None
 */
gchar**
pk_sources_get_types (PkSources *sources)
{
	g_return_val_if_fail (PK_IS_SOURCES (sources), NULL);
	return pk_connection_sources_get_types (sources->priv->connection);
}
