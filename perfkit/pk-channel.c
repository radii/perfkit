/* pk-channel.c
 *
 * Copyright (C) 2010 Christian Hergert
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "perfkit-lowlevel.h"
#include "pk-channel.h"
#include "pk-connection.h"

G_DEFINE_TYPE(PkChannel, pk_channel, G_TYPE_OBJECT)

/**
 * SECTION:pk-channel
 * @title: PkChannel
 * @short_description: 
 *
 * 
 */

struct _PkChannelPrivate
{
	PkConnection *conn;
	gint id;
};

enum
{
	PROP_0,
	PROP_CONN,
	PROP_ID,
	PROP_ARGS,
	PROP_TARGET,
	PROP_ENV,
	PROP_PID,
	PROP_WORKING_DIR,
};

gint
pk_channel_get_id (PkChannel *channel)
{
	g_return_val_if_fail(PK_IS_CHANNEL(channel), -1);
	return channel->priv->id;
}

PkChannelState
pk_channel_get_state (PkChannel *channel)
{
	PkChannelPrivate *priv;
	PkConnectionClass *klass;
	PkChannelState state;
	GError *error = NULL;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), 0);

	priv = channel->priv;
	klass = PK_CONNECTION_GET_CLASS(priv->conn);

	if (0 == (state = klass->channel_get_state(priv->conn, priv->id, &error))) {
		g_warning("%s", error->message);
		g_error_free(error);
		return 0;
	}

	return state;
}

/**
 * pk_channel_get_target:
 * @channel: A #PkChannel
 *
 * Retrieves the "target" property.
 *
 * Return value: a string containing the "target" property or %NULL.
 *   The value should not be modified or freed.
 *
 * Side effects: None.
 */
G_CONST_RETURN gchar*
pk_channel_get_target (PkChannel *channel)
{
	gchar *target = NULL;

	g_return_val_if_fail (PK_IS_CHANNEL (channel), NULL);

	/* leaks */
	pk_connection_channel_get_target(channel->priv->conn,
	                                 channel->priv->id,
	                                 &target,
	                                 NULL);

	return target;
}

/**
 * pk_channel_get_working_dir:
 * @channel: A #PkChannel
 *
 * Retrieves the "working-dir" property.
 *
 * Return value: a string containing the "working-dir" property or %NULL.
 *   The value should not be modified or freed.
 *
 * Side effects: None.
 */
G_CONST_RETURN gchar*
pk_channel_get_working_dir (PkChannel *channel)
{
	gchar *dir = NULL;

	g_return_val_if_fail (PK_IS_CHANNEL (channel), NULL);

	/* leaks */
	pk_connection_channel_get_working_dir(channel->priv->conn,
	                                      channel->priv->id,
	                                      &dir,
	                                      NULL);

	return dir;
}

/**
 * pk_channel_get_args:
 * @channel: A #PkChannel
 *
 * Retrieves the "args" property.
 *
 * Return value: a string array containing the "args" property or %NULL.
 *   The value should not be modified or freed.
 *
 * Side effects: None.
 */
gchar**
pk_channel_get_args (PkChannel *channel)
{
	gchar **args = NULL;

	g_return_val_if_fail (PK_IS_CHANNEL (channel), NULL);

	/* leaks */
	pk_connection_channel_get_args(channel->priv->conn,
	                               channel->priv->id,
	                               &args,
	                               NULL);

	return args;
}

/**
 * pk_channel_get_env:
 * @channel: A #PkChannel
 *
 * Retrieves the "env" property.
 *
 * Return value: a string array containing the "env" property or %NULL.
 *   The value should not be modified or freed.
 *
 * Side effects: None.
 */
gchar**
pk_channel_get_env (PkChannel *channel)
{
	gchar **env = NULL;

	g_return_val_if_fail (PK_IS_CHANNEL (channel), NULL);

	/* leaks */
	pk_connection_channel_get_env(channel->priv->conn,
	                              channel->priv->id,
	                              &env,
	                              NULL);

	return env;
}

/**
 * pk_channel_get_pid:
 * @channel: A #PkChannel
 *
 * Retrieves the "pid" property.
 *
 * Return value: The process identifier or 0.
 *
 * Side effects: None.
 */
GPid
pk_channel_get_pid (PkChannel *channel)
{
	GPid pid = 0;

	g_return_val_if_fail (PK_IS_CHANNEL (channel), 0);

	pk_connection_channel_get_pid(channel->priv->conn,
	                              channel->priv->id,
	                              &pid,
	                              NULL);

	return pid;
}

gboolean
pk_channel_start (PkChannel  *channel,
                  GError    **error)
{
	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	return pk_connection_channel_start(channel->priv->conn,
	                                   channel->priv->id,
	                                   error);
}

gboolean
pk_channel_stop (PkChannel  *channel,
                 gboolean    killpid,
                 GError    **error)
{
	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	return pk_connection_channel_stop(channel->priv->conn,
	                                  channel->priv->id,
	                                  killpid,
	                                  error);
}

gboolean
pk_channel_pause (PkChannel  *channel,
                  GError    **error)
{
	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	return pk_connection_channel_pause (channel->priv->conn,
	                                    channel->priv->id,
	                                    error);
}

gboolean
pk_channel_unpause (PkChannel  *channel,
                    GError    **error)
{
	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	return pk_connection_channel_unpause (channel->priv->conn,
	                                      channel->priv->id,
	                                      error);
}

static void
pk_channel_finalize (GObject *object)
{
	G_OBJECT_CLASS(pk_channel_parent_class)->finalize(object);
}

static void
pk_channel_dispose (GObject *object)
{
	G_OBJECT_CLASS(pk_channel_parent_class)->dispose(object);
}

static void
pk_channel_class_init (PkChannelClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_channel_finalize;
	object_class->dispose = pk_channel_dispose;
	g_type_class_add_private (object_class, sizeof (PkChannelPrivate));

	/**
	 * PkChannel:target:
	 *
	 * The "target" property.
	 */
	g_object_class_install_property (object_class,
	                                 PROP_TARGET,
	                                 g_param_spec_string ("target",
	                                                      "target",
	                                                      "The target property",
	                                                      NULL,
	                                                      G_PARAM_READABLE));
}

static void
pk_channel_init (PkChannel *channel)
{
	channel->priv = G_TYPE_INSTANCE_GET_PRIVATE(channel,
	                                            PK_TYPE_CHANNEL,
	                                            PkChannelPrivate);
}
