/* pk-channel.c
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

#include "pk-channel.h"
#include "pk-connection.h"
#include "pk-connection-priv.h"

/**
 * SECTION:pk-channel
 * @title: PkChannel
 * @short_description: Perfkit channel proxy
 *
 * The #PkChannel is a proxy for a remote channel within a Perfkit
 * daemon.  It provides methods to manage the channel regardless of the
 * protocol used to talk to the Perfkit daemon.
 *
 * You can get access to #PkChannel instances through the #PkChannels
 * class.
 */

G_DEFINE_TYPE (PkChannel, pk_channel, G_TYPE_OBJECT)

struct _PkChannelPrivate
{
	PkConnection *connection;
	gint          channel_id;
};

static void
pk_channel_finalize (GObject *object)
{
	G_OBJECT_CLASS (pk_channel_parent_class)->finalize (object);
}

static void
pk_channel_class_init (PkChannelClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_channel_finalize;
	g_type_class_add_private (object_class, sizeof (PkChannelPrivate));
}

static void
pk_channel_init (PkChannel *channel)
{
	channel->priv = G_TYPE_INSTANCE_GET_PRIVATE (channel,
	                                             PK_TYPE_CHANNEL,
	                                             PkChannelPrivate);
}

PkChannel*
pk_channel_new (PkConnection *connection,
                gint          channel_id)
{
	PkChannel *channel;

	channel = g_object_new (PK_TYPE_CHANNEL, NULL);
	channel->priv->channel_id = channel_id;
	channel->priv->connection = g_object_ref (connection);

	return channel;
}

/**
 * pk_channel_get_id:
 * @channel: A #PkChannel
 *
 * Retrieves the identifier for the #PkChannel.
 *
 * Return value: the channel identifier as an integer
 *
 * Side effects: None
 */
gint
pk_channel_get_id (PkChannel *channel)
{
	g_return_val_if_fail (PK_IS_CHANNEL (channel), -1);
	return channel->priv->channel_id;
}

/**
 * pk_channel_get_target:
 * @channel: A #PkChannel
 *
 * Retrieves the target executable for the channel.
 *
 * Return value: the channel's target which should be freed with g_free().
 *
 * Side effects: None
 */
gchar*
pk_channel_get_target (PkChannel *channel)
{
	g_return_val_if_fail (PK_IS_CHANNEL (channel), NULL);
	return pk_connection_channel_get_target (channel->priv->connection,
	                                         channel->priv->channel_id);
}

/**
 * pk_channel_get_args:
 * @channel: A #PkChannel
 *
 * Retrieves the arguments used for the target channel.
 *
 * Return value: a %NULL terminated #GStrv containing the arguments used for
 *   the channel.  They should be freed using g_strfreev().
 *
 * Side effects: None
 */
gchar**
pk_channel_get_args (PkChannel *channel)
{
	g_return_val_if_fail (PK_IS_CHANNEL (channel), NULL);
	return pk_connection_channel_get_args (channel->priv->connection,
	                                       channel->priv->channel_id);
}

/**
 * pk_channel_get_dir:
 * @channel: A #PkChannel
 *
 * Retrieves the working directory of the channel.
 *
 * Return value: the working directory as a string.  The string should
 *   be freed with g_free().
 *
 * Side effects: None
 */
gchar*
pk_channel_get_dir (PkChannel *channel)
{
	g_return_val_if_fail (PK_IS_CHANNEL (channel), NULL);
	return pk_connection_channel_get_dir (channel->priv->connection,
	                                      channel->priv->channel_id);
}

/**
 * pk_channel_get_env:
 * @channel: A #PkChannel
 *
 * Retrieves the environment variables of the #PkChannel.
 *
 * Return value: A #GStrv containing the environment variables in the format
 *   KEY=VALUE.  It should be freed with g_strfreev().
 *
 * Side effects: None
 */
gchar**
pk_channel_get_env (PkChannel *channel)
{
	g_return_val_if_fail (PK_IS_CHANNEL (channel), NULL);
	return pk_connection_channel_get_env (channel->priv->connection,
	                                      channel->priv->channel_id);
}

/**
 * pk_channel_get_pid:
 * @channel: A #PkChannel
 *
 * Retrieves the process id of the channel's target process.
 *
 * Return value: the #GPid of the target process or 0 if no process
 *   has been spawned.
 *
 * Side effects: None
 */
GPid
pk_channel_get_pid (PkChannel *channel)
{
	g_return_val_if_fail (PK_IS_CHANNEL (channel), 0);
	return pk_connection_channel_get_pid (channel->priv->connection,
	                                      channel->priv->channel_id);
}

/**
 * pk_channel_get_state:
 * @channel: A #PkChannel
 *
 * Retrieves the state of the #PkChannel.
 *
 * Return value: the #PkChannelState of the channel.
 *
 * Side effects: None
 */
PkChannelState
pk_channel_get_state (PkChannel *channel)
{
	g_return_val_if_fail (PK_IS_CHANNEL (channel), -1);
	return pk_connection_channel_get_state (channel->priv->connection,
	                                        channel->priv->channel_id);
}
