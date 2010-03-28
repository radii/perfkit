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
	PkConnection  *conn;
	gint           id;
	gchar         *target;
	gchar         *working_dir;
	gchar        **args;
	gchar        **env;
	GPid           pid;
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
	PkChannelState state = 0;
	GError *error = NULL;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), 0);

	priv = channel->priv;

	if (!pk_connection_channel_get_state(priv->conn, priv->id, &state, &error)) {
		g_printerr("%s\n", error->message);
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
	PkChannelPrivate *priv;

	g_return_val_if_fail (PK_IS_CHANNEL (channel), NULL);

	priv = channel->priv;

	if (!priv->target) {
		if (!pk_connection_channel_get_target(priv->conn,
											  priv->id,
											  &priv->target,
											  NULL)) {
			return NULL;
		}
	}

	return priv->target;
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
	PkChannelPrivate *priv;

	g_return_val_if_fail (PK_IS_CHANNEL (channel), NULL);

	priv = channel->priv;

	if (!priv->working_dir) {
		if (!pk_connection_channel_get_working_dir(priv->conn,
		                                           priv->id,
		                                           &priv->working_dir,
		                                           NULL)) {
		    return NULL;
		}
	}

	return priv->working_dir;
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
	PkChannelPrivate *priv;

	g_return_val_if_fail (PK_IS_CHANNEL (channel), NULL);

	priv = channel->priv;

	if (!priv->args) {
		if (!pk_connection_channel_get_args(priv->conn,
		                                    priv->id,
		                                    &priv->args,
		                                    NULL)) {
		    return NULL;
		}
	}

	return priv->args;
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
	PkChannelPrivate *priv;

	g_return_val_if_fail (PK_IS_CHANNEL (channel), NULL);

	priv = channel->priv;

	if (!priv->env) {
		if (!pk_connection_channel_get_env(priv->conn,
		                                   priv->id,
		                                   &priv->env,
		                                   NULL)) {
		    return NULL;
		}
	}

	return priv->env;
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
	PkChannelPrivate *priv;

	g_return_val_if_fail (PK_IS_CHANNEL (channel), 0);

	priv = channel->priv;

	if (!priv->pid) {
		if (!pk_connection_channel_get_pid(priv->conn,
		                                   priv->id,
		                                   &priv->pid,
		                                   NULL)) {
		    return 0;
		}
	}

	return priv->pid;
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

PkSource*
pk_channel_add_source (PkChannel    *channel,
                       PkSourceInfo *source_info)
{
	GError *error = NULL;
	gint source_id = 0;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), NULL);
	g_return_val_if_fail(PK_IS_SOURCE_INFO(source_info), NULL);

	if (!pk_connection_channel_add_source(channel->priv->conn,
	                                      channel->priv->id,
	                                      pk_source_info_get_uid(source_info),
	                                      &source_id,
	                                      &error)) {
	    g_error_free(error);
	    return NULL;
	}

	return g_object_new(PK_TYPE_CHANNEL,
	                    "connection", channel->priv->conn,
	                    "id", source_id,
	                    NULL);
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
pk_channel_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
	PkChannelPrivate *priv = ((PkChannel *)object)->priv;

	switch (prop_id) {
	case PROP_ID:
		priv->id = g_value_get_int(value);
		break;
	case PROP_CONN:
		priv->conn = g_value_dup_object(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_channel_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
	PkChannelPrivate *priv = ((PkChannel *)object)->priv;

	switch (prop_id) {
	case PROP_ID:
		g_value_set_int(value, priv->id);
		break;
	case PROP_CONN:
		g_value_set_object(value, priv->conn);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_channel_class_init (PkChannelClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_channel_finalize;
	object_class->dispose = pk_channel_dispose;
	object_class->set_property = pk_channel_set_property;
	object_class->get_property = pk_channel_get_property;
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

	g_object_class_install_property(object_class,
	                                PROP_CONN,
	                                g_param_spec_object("connection",
	                                                    "connection",
	                                                    "Connection",
	                                                    PK_TYPE_CONNECTION,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
	                                PROP_ID,
	                                g_param_spec_int("id",
	                                                 "id",
	                                                 "Id",
	                                                 0,
	                                                 G_MAXINT,
	                                                 0,
	                                                 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
pk_channel_init (PkChannel *channel)
{
	channel->priv = G_TYPE_INSTANCE_GET_PRIVATE(channel,
	                                            PK_TYPE_CHANNEL,
	                                            PkChannelPrivate);
}
