/* pk-channels.c
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

#include "pk-channel-priv.h"
#include "pk-channels.h"
#include "pk-connection.h"
#include "pk-connection-priv.h"

G_DEFINE_TYPE (PkChannels, pk_channels, G_TYPE_OBJECT)

struct _PkChannelsPrivate
{
	PkConnection *connection;
};

static void
pk_channels_finalize (GObject *object)
{
	G_OBJECT_CLASS (pk_channels_parent_class)->finalize (object);
}

static void
pk_channels_class_init (PkChannelsClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_channels_finalize;
	g_type_class_add_private (object_class, sizeof (PkChannelsPrivate));
}

static void
pk_channels_init (PkChannels *channels)
{
	channels->priv = G_TYPE_INSTANCE_GET_PRIVATE (channels,
	                                              PK_TYPE_CHANNELS,
	                                              PkChannelsPrivate);
}

PkChannels*
pk_channels_new (PkConnection *connection)
{
	PkChannels *channels;

	channels = g_object_new (PK_TYPE_CHANNELS, NULL);
	channels->priv->connection = connection;

	return channels;
}

/**
 * pk_channels_find_all:
 * @channels: 
 *
 * 
 *
 * Return value:
 */
GList*
pk_channels_find_all (PkChannels *channels)
{
	PkChannelsPrivate *priv;
	gint              *channel_ids,
	                   n_channels = 0,
	                   i;
	GList             *list = NULL;
	PkChannel         *channel;

	g_return_val_if_fail (PK_IS_CHANNELS (channels), NULL);

	priv = channels->priv;

	if (!pk_connection_channels_find_all (priv->connection,
	                                      &channel_ids,
	                                      &n_channels,
	                                      NULL))
	    return NULL;

	for (i = 0; i < n_channels; i++) {
		channel = pk_channels_get (channels, channel_ids [i]);
		list = g_list_prepend (list, channel);
	}

	list = g_list_reverse (list);
	g_free (channel_ids);

	return list;
}

/**
 * pk_channels_get:
 * @channels: 
 * @channel_id: 
 *
 * 
 *
 * Return value: 
 */
PkChannel*
pk_channels_get (PkChannels *channels,
                 gint        channel_id)
{
	g_return_val_if_fail (PK_IS_CHANNELS (channels), NULL);
	return pk_channel_new (channels->priv->connection, channel_id);
}
