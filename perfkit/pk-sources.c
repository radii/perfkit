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

#include "pk-source.h"
#include "pk-source-priv.h"
#include "pk-source-info.h"
#include "pk-source-info-priv.h"
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
 * pk_sources_get:
 * @sources: A #PkSources
 * @source_id: the source id
 *
 * Retrieves a proxy for a remote Perfkit source.
 *
 * Return value: the newly created instance of #PkSource.  The instance
 *   should be freed with g_object_unref().
 *
 * Side effects: None
 */
PkSource*
pk_sources_get (PkSources *sources,
                gint       source_id)
{
	g_return_val_if_fail (PK_IS_SOURCES (sources), NULL);
	return pk_source_new (sources->priv->connection, source_id);
}

/**
 * pk_sources_get_source_types:
 * @sources: A #PkSources
 *
 * Retrieves a list of #PkSourceInfo<!-- -->'s containing information
 * about the available data source types.
 *
 * Return value:
 *       A newly created #GList containing a list of #PkSourceInfo.
 *       The list should be freed using g_list_free().  Each of the
 *       #PkSourceInfo should be freed with g_object_unref().
 *
 * Side effects:
 *       None.
 */
GList*
pk_sources_get_source_types (PkSources *sources)
{
	GError *error = NULL;
	gchar **uids = NULL;
	GList *list = NULL;
	PkSourceInfo *info;
	gint i;

	g_return_val_if_fail (PK_IS_SOURCES (sources), NULL);

	if (!pk_connection_sources_get_source_types (sources->priv->connection,
	                                             &uids, &error)) {
	    g_warning ("%s: Error retrieving source types: %s",
	               G_STRFUNC, error->message);
	    g_error_free (error);
	    return NULL;
	}

	for (i = 0; uids [i]; i++) {
		info = pk_source_info_new (sources->priv->connection, uids [i]);
		list = g_list_prepend (list, info);
	}

	g_strfreev (uids);

	return list;
}
