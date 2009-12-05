/* pk-source.c
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

#include <dbus/dbus-glib.h>

#include "pk-source.h"
#include "pk-connection.h"
#include "pk-connection-priv.h"

/**
 * SECTION:pk-source
 * @title: PkSource
 * @short_description: Perfkit source proxy
 *
 * The #PkSource is a proxy for a remote source within a Perfkit
 * daemon.  It provides methods to manage the source regardless of the
 * protocol used to talk to the Perfkit daemon.
 *
 * You can get access to #PkSource instances through the #PkSources
 * class.
 */

G_DEFINE_TYPE (PkSource, pk_source, G_TYPE_OBJECT)

struct _PkSourcePrivate
{
	PkConnection *connection;
	gint          source_id;
};

static void
pk_source_finalize (GObject *object)
{
	G_OBJECT_CLASS (pk_source_parent_class)->finalize (object);
}

static void
pk_source_class_init (PkSourceClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_source_finalize;
	g_type_class_add_private (object_class, sizeof (PkSourcePrivate));
}

static void
pk_source_init (PkSource *source)
{
	source->priv = G_TYPE_INSTANCE_GET_PRIVATE (source,
	                                             PK_TYPE_SOURCE,
	                                             PkSourcePrivate);
}

PkSource*
pk_source_new (PkConnection *connection,
                gint          source_id)
{
	PkSource *source;

	source = g_object_new (PK_TYPE_SOURCE, NULL);
	source->priv->source_id = source_id;
	source->priv->connection = g_object_ref (connection);

	return source;
}

/**
 * pk_source_get_id:
 * @source: A #PkSource
 *
 * Retrieves the identifier for the #PkSource.
 *
 * Return value: the source identifier as an integer
 *
 * Side effects: None
 */
gint
pk_source_get_id (PkSource *source)
{
	g_return_val_if_fail (PK_IS_SOURCE (source), -1);
	return source->priv->source_id;
}

/**
 * pk_source_set_channel:
 * @source: A #PkSource
 * @channel: A #PkChannel
 *
 * Sets the channel in which the source delivers samples.
 *
 * Side effects: Sets the channel for the source.
 */
void
pk_source_set_channel (PkSource  *source,
                       PkChannel *channel)
{
	g_return_if_fail (PK_IS_SOURCE (source));
	g_return_if_fail (PK_IS_CHANNEL (channel));

	pk_connection_source_set_channel (source->priv->connection,
	                                  source->priv->source_id,
	                                  pk_channel_get_id (channel));
}
