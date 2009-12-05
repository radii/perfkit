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
 * pk_channel_set_target:
 * @channel: A #PkChannel
 * @target: A path to an executable on the remote host
 *
 * Sets the "target" property.  The target is the executable that should
 * be spawned when the channel starts recording.  A value of %NULL
 * indicates that no process need be spawned.
 *
 * Side effects: the target for the channel is changed if the channel
 *   has not yet been started.
 */
void
pk_channel_set_target (PkChannel   *channel,
                       const gchar *target)
{
	g_return_if_fail (PK_IS_CHANNEL (channel));
	pk_connection_channel_set_target (channel->priv->connection,
	                                  channel->priv->channel_id,
	                                  target);
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
 * pk_channel_set_args:
 * @channel: A #PkChannel
 * @args: a #GStrv containing arguments for the target executable
 *
 * Sets the "args" property.  The args are passed to the child process
 * when it is spawned.
 *
 * Side effects: Sets the args property of the channel.
 */
void
pk_channel_set_args (PkChannel  *channel,
                     gchar     **args)
{
	g_return_if_fail (PK_IS_CHANNEL (channel));
	pk_connection_channel_set_args (channel->priv->connection,
	                                channel->priv->channel_id,
	                                args);
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
 * pk_channel_set_dir:
 * @channel: A #PkChannel
 * @dir: the new working directory
 *
 * Sets the "dir" property.  The dir is the working directory for the
 * spawned process on the remote host.
 *
 * Side effects: the dir for the channel is changed if the channel
 *   has not yet been started.
 */
void
pk_channel_set_dir (PkChannel   *channel,
                    const gchar *dir)
{
	g_return_if_fail (PK_IS_CHANNEL (channel));
	pk_connection_channel_set_dir (channel->priv->connection,
	                               channel->priv->channel_id,
	                               dir);
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
 * pk_channel_set_env:
 * @channel: A #PkChannel
 * @env: a #GStrv containining KEY=VALUE environment variables
 *
 * Sets the "env" property.  The env is a %NULL-terminated list of strings
 * containing KEY=VALUE pairs.
 *
 * Side effects: Sets the env property of the channel.
 */
void
pk_channel_set_env (PkChannel  *channel,
                    gchar     **env)
{
	g_return_if_fail (PK_IS_CHANNEL (channel));
	pk_connection_channel_set_env (channel->priv->connection,
	                               channel->priv->channel_id,
	                               env);
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
 * pk_channel_set_pid:
 * @channel: A #PkChannel
 * @pid: a process identifier
 *
 * Sets the "pid" property.  The pid can be used two different ways.
 * Once a process has been spawned, it will contain the spawned process
 * identifier.  However, if no procses is spawned, it can be used to
 * attach to an existing process.  Therefore, it must be set before the
 * channel is started.
 *
 * Side effects: the pid property is changed.
 */
void
pk_channel_set_pid (PkChannel *channel,
                    GPid       pid)
{
	g_return_if_fail (PK_IS_CHANNEL (channel));
	pk_connection_channel_set_pid (channel->priv->connection,
	                               channel->priv->channel_id,
	                               pid);
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

/**
 * pk_channel_start:
 * @channel: A #PkChannel
 * @error: A location for a #GError or %NULL
 *
 * 
 *
 * Return value: %TRUE on success. @error is set on failure.
 *
 * Side effects: The channels state machine is altered.
 */
gboolean
pk_channel_start (PkChannel  *channel,
                  GError    **error)
{
	g_return_val_if_fail (PK_IS_CHANNEL (channel), FALSE);
	return pk_connection_channel_start (channel->priv->connection,
	                                    channel->priv->channel_id,
	                                    error);
}

/**
 * pk_channel_stop:
 * @channel: A #PkChannel
 * @error: A location for a #GError or %NULL
 *
 * 
 *
 * Return value: %TRUE on success. @error is set on failure.
 *
 * Side effects: The channels state machine is altered.
 */
gboolean
pk_channel_stop (PkChannel  *channel,
                 GError    **error)
{
	g_return_val_if_fail (PK_IS_CHANNEL (channel), FALSE);
	return pk_connection_channel_stop (channel->priv->connection,
	                                   channel->priv->channel_id,
	                                   error);
}

/**
 * pk_channel_pause:
 * @channel: A #PkChannel
 * @error: A location for a #GError or %NULL
 *
 * 
 *
 * Return value: %TRUE on success. @error is set on failure.
 *
 * Side effects: The channels state machine is altered.
 */
gboolean
pk_channel_pause (PkChannel  *channel,
                  GError    **error)
{
	g_return_val_if_fail (PK_IS_CHANNEL (channel), FALSE);
	return pk_connection_channel_pause (channel->priv->connection,
	                                    channel->priv->channel_id,
	                                    error);
}

/**
 * pk_channel_unpause:
 * @channel: A #PkChannel
 * @error: A location for a #GError or %NULL
 *
 * 
 *
 * Return value: %TRUE on success. @error is set on failure.
 *
 * Side effects: The channels state machine is altered.
 */
gboolean
pk_channel_unpause (PkChannel  *channel,
                    GError    **error)
{
	g_return_val_if_fail (PK_IS_CHANNEL (channel), FALSE);
	return pk_connection_channel_unpause (channel->priv->connection,
	                                      channel->priv->channel_id,
	                                      error);
}
