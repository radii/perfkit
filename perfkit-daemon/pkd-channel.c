/* pkd-channel.c
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
#include "config.h"
#endif

#include "pkd-channel.h"
#include "pkd-channel-dbus.h"
#include "pkd-runtime.h"

/**
 * SECTION:pkd-channel
 * @title: PkdChannel
 * @short_description: Perfkit data channels
 *
 * #PkdChannel represents a way to get data samples from a system.  It can
 * contain multiple #PkdSource<!-- -->'s.
 */

G_DEFINE_TYPE (PkdChannel, pkd_channel, G_TYPE_OBJECT)

enum
{
	PROP_0,
	PROP_TARGET,
	PROP_PID,
	PROP_ENV,
	PROP_ARGS,
	PROP_DIR,
};

enum
{
	STATE_READY,    /* Channel has not been started, and can still be
	                 * configured.
					 */
	STATE_STARTED,  /* Once started, this channel will create aggregate samples
	                 * according to the recording frequency settings..
					 */
	STATE_STOPPED,  /* Once stopped, this channel is completed and no further
	                 * processing can continue.
					 */
	STATE_PAUSED,   /* Channel was started, then paused.  This is the only
	                 * non-initial state that can progress to STATE_STARTED.
					 */
	STATE_LAST
};

struct _PkdChannelPrivate
{
	GStaticRWLock   rw_lock;
	gint            id;
	gint            state;
	gchar          *dir;
	gchar         **args;
	GPid            pid;
	gchar          *target;
	gchar         **env;
};

static gint channel_seq = 0;

static void
pkd_channel_finalize (GObject *object)
{
	G_OBJECT_CLASS (pkd_channel_parent_class)->finalize (object);
}

static void
pkd_channel_set_property (GObject      *object,
                          guint         prop_id,
						  const GValue *value,
						  GParamSpec   *pspec)
{
	switch (prop_id) {
	case PROP_TARGET:
		pkd_channel_set_target (PKD_CHANNEL (object),
		                        g_value_get_string (value));
		break;
	case PROP_PID:
		pkd_channel_set_pid (PKD_CHANNEL (object),
		                     (GPid)g_value_get_uint (value));
		break;
	case PROP_ENV:
		pkd_channel_set_env (PKD_CHANNEL (object),
		                     g_value_get_boxed (value));
		break;
	case PROP_ARGS:
		pkd_channel_set_args (PKD_CHANNEL (object),
		                      g_value_get_boxed (value));
		break;
	case PROP_DIR:
		pkd_channel_set_dir (PKD_CHANNEL (object),
		                     g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pkd_channel_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
	switch (prop_id) {
	case PROP_TARGET:
		g_value_take_string (value, pkd_channel_get_target (PKD_CHANNEL (object)));
		break;
	case PROP_PID:
		g_value_set_uint (value, pkd_channel_get_pid (PKD_CHANNEL (object)));
		break;
	case PROP_ENV:
		g_value_set_boxed (value, pkd_channel_get_env (PKD_CHANNEL (object)));
		break;
	case PROP_ARGS:
		g_value_set_boxed (value, pkd_channel_get_args (PKD_CHANNEL (object)));
		break;
	case PROP_DIR:
		g_value_take_string (value, pkd_channel_get_dir (PKD_CHANNEL (object)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pkd_channel_class_init (PkdChannelClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->set_property = pkd_channel_set_property;
	object_class->get_property = pkd_channel_get_property;
	object_class->finalize = pkd_channel_finalize;
	g_type_class_add_private (object_class, sizeof (PkdChannelPrivate));

	/**
	 * PkdChannel:target:
	 *
	 * The "target" property.
	 */
	g_object_class_install_property (object_class,
	                                 PROP_TARGET,
	                                 g_param_spec_string ("target",
	                                                      "target",
	                                                      "Target executable",
	                                                      NULL,
	                                                      G_PARAM_READWRITE));

	/**
	 * PkdChannel:dir:
	 *
	 * The "dir" property.
	 */
	g_object_class_install_property (object_class,
	                                 PROP_DIR,
	                                 g_param_spec_string ("dir",
	                                                      "dir",
	                                                      "Target working directory",
	                                                      NULL,
	                                                      G_PARAM_READWRITE));

	/**
	 * PkdChannel:pid:
	 *
	 * The "pid" property.
	 */
	g_object_class_install_property (object_class,
	                                 PROP_PID,
	                                 g_param_spec_uint ("pid",
	                                                    "pid",
	                                                    "Target process id",
	                                                    0,
	                                                    G_MAXINT,
	                                                    0,
	                                                    G_PARAM_READWRITE));

	/**
	 * PkdChannel:env:
	 *
	 * The "env" property.
	 */
	g_object_class_install_property (object_class,
	                                 PROP_ENV,
	                                 g_param_spec_boxed ("env",
	                                                     "env",
	                                                     "Environment variables",
	                                                     G_TYPE_STRV,
	                                                     G_PARAM_READWRITE));

	/**
	 * PkdChannel:args:
	 *
	 * The "args" property.
	 */
	g_object_class_install_property (object_class,
	                                 PROP_ARGS,
									 g_param_spec_boxed ("args",
	                                                     "args",
	                                                     "Target arguments",
	                                                     G_TYPE_STRV,
	                                                     G_PARAM_READWRITE));

	dbus_g_object_type_install_info (PKD_TYPE_CHANNEL, &dbus_glib_pkd_channel_object_info);
}

static void
pkd_channel_init (PkdChannel *channel)
{
	gchar *path;

	channel->priv = G_TYPE_INSTANCE_GET_PRIVATE ((channel),
	                                             PKD_TYPE_CHANNEL,
	                                             PkdChannelPrivate);

	channel->priv->state = STATE_READY;

	/* generate unique identifier */
	channel->priv->id = g_atomic_int_exchange_and_add (&channel_seq, 1);

	/* register the channel on the DBUS */
	path = g_strdup_printf ("/com/dronelabs/Perfkit/Channels/%d",
	                        channel->priv->id);
	dbus_g_connection_register_g_object (pkd_runtime_get_connection (),
	                                     path, G_OBJECT (channel));
	g_free (path);
}

/**
 * pkd_channel_get_dir:
 * @channel: A #PkdChannel
 *
 * Retreives the "dir" property.
 *
 * Note that this class is thread safe so you are returned a copy of the string
 * which you are responsible for freeing when you are done with g_free().
 *
 * Return value: a string which should be freed with g_free().
 */
gchar *
pkd_channel_get_dir (PkdChannel *channel)
{
	PkdChannelPrivate *priv;
	gchar             *dir_copy;

	g_return_val_if_fail (PKD_IS_CHANNEL (channel), NULL);

	priv = channel->priv;

	g_static_rw_lock_reader_lock (&priv->rw_lock);
	dir_copy = g_strdup (priv->dir);
	g_static_rw_lock_reader_unlock (&priv->rw_lock);

	return dir_copy;
}

/**
 * pkd_channel_set_dir:
 * @channel: A #PkdChannel
 * @dir: A string or %NULL
 *
 * Sets the "dir" property.
 */
void
pkd_channel_set_dir (PkdChannel  *channel,
                     const gchar *dir)
{
	PkdChannelPrivate *priv;
	gchar *dir_copy;

	g_return_if_fail (PKD_IS_CHANNEL (channel));

	priv = channel->priv;

	dir_copy = priv->dir;
	priv->dir = g_strdup (dir);
	g_free (dir_copy);

	g_object_notify (G_OBJECT (channel), "dir");
}

/**
 * pkd_channel_get_args:
 * @channel: A #PkdChannel
 *
 * Retreives the "args" property.
 *
 * Note that this class is thread safe so you are returned a copy of the string
 * array which you are responsible for freeing when you are done with
 * g_strfreev().
 *
 * Return value: a string array which should be freed with g_strfreev().
 */
gchar **
pkd_channel_get_args (PkdChannel *channel)
{
	PkdChannelPrivate  *priv;
	gchar             **args_copy;

	g_return_val_if_fail (PKD_IS_CHANNEL (channel), NULL);

	priv = channel->priv;

	g_static_rw_lock_reader_lock (&priv->rw_lock);
	args_copy = g_strdupv (priv->args);
	g_static_rw_lock_reader_unlock (&priv->rw_lock);

	return args_copy;
}

/**
 * pkd_channel_set_args:
 * @channel: A #PkdChannel
 * @args: A string array or %NULL
 *
 * Sets the "args" property.
 */
void
pkd_channel_set_args (PkdChannel   *channel,
                      const gchar **args)
{
	PkdChannelPrivate  *priv;
	gchar             **args_copy;

	g_return_if_fail (PKD_IS_CHANNEL (channel));

	priv = channel->priv;

	g_static_rw_lock_writer_lock (&priv->rw_lock);
	args_copy = priv->args;
	priv->args = g_strdupv ((gchar**)args);
	g_static_rw_lock_writer_unlock (&priv->rw_lock);
	g_strfreev (args_copy);

	g_object_notify (G_OBJECT (channel), "args");
}

/**
 * pkd_channel_get_pid:
 * @channel: A #PkdChannel
 *
 * Retreives the "pid" property.
 *
 * Return value: a GPid
 */
GPid
pkd_channel_get_pid (PkdChannel *channel)
{
	g_return_val_if_fail (PKD_IS_CHANNEL (channel), 0);
	return channel->priv->pid;
}

/**
 * pkd_channel_set_pid:
 * @channel: A #PkdChannel
 * @pid: A #GPid
 *
 * Sets the "pid" property.
 */
void
pkd_channel_set_pid (PkdChannel *channel,
                     GPid        pid)
{
	PkdChannelPrivate *priv;

	g_return_if_fail (PKD_IS_CHANNEL (channel));

	priv = channel->priv;

	g_static_rw_lock_writer_lock (&priv->rw_lock);
	priv->pid = pid;
	g_static_rw_lock_writer_unlock (&priv->rw_lock);

	g_object_notify (G_OBJECT (channel), "pid");
}



/**
 * pkd_channel_get_target:
 * @channel: A #PkdChannel
 *
 * Retreives the "target" property.
 *
 * Note that this class is thread safe so you are returned a copy of the string
 * which you are responsible for freeing when you are done with g_free().
 *
 * Return value: a string which should be freed with g_free().
 */
gchar *
pkd_channel_get_target (PkdChannel* channel)
{
	PkdChannelPrivate *priv;
	gchar             *target_copy;

	g_return_val_if_fail (PKD_IS_CHANNEL (channel), NULL);

	priv = channel->priv;

	g_static_rw_lock_reader_lock (&priv->rw_lock);
	target_copy = g_strdup (priv->target);
	g_static_rw_lock_reader_unlock (&priv->rw_lock);

	return target_copy;
}

/**
 * pkd_channel_set_target:
 * @channel: A #PkdChannel
 * @target: A string or %NULL
 *
 * Sets the "target" property.
 */
void
pkd_channel_set_target (PkdChannel  *channel,
                        const gchar *target)
{
	PkdChannelPrivate *priv;
	gchar             *target_copy;

	g_return_if_fail (PKD_IS_CHANNEL (channel));

	priv = channel->priv;

	target_copy = priv->target;
	priv->target = g_strdup (target);
	g_free (target_copy);

	g_object_notify (G_OBJECT (channel), "target");
}

/**
 * pkd_channel_get_env:
 * @channel: A #PkdChannel
 *
 * Retreives the "env" property.
 *
 * Note that this class is thread safe so you are returned a copy of the string
 * array which you are responsible for freeing when you are done with
 * g_strfreev().
 *
 * Return value: a string array which should be freed with g_strfreev().
 */
gchar **
pkd_channel_get_env (PkdChannel* channel)
{
	PkdChannelPrivate *priv;
	gchar **env_copy;

	g_return_val_if_fail (PKD_IS_CHANNEL (channel), NULL);

	priv = channel->priv;

	g_static_rw_lock_reader_lock (&priv->rw_lock);
	env_copy = g_strdupv (priv->env);
	g_static_rw_lock_reader_unlock (&priv->rw_lock);

	return env_copy;
}

/**
 * pkd_channel_set_env:
 * @channel: A #PkdChannel
 * @env: A string array or %NULL
 *
 * Sets the "env" property.
 */
void
pkd_channel_set_env (PkdChannel* channel,
                     const gchar ** env)
{
	PkdChannelPrivate *priv;
	gchar **env_copy;

	g_return_if_fail (PKD_IS_CHANNEL (channel));

	priv = channel->priv;

	g_static_rw_lock_writer_lock (&priv->rw_lock);
	env_copy = priv->env;
	priv->env = g_strdupv ((gchar**)env);
	g_static_rw_lock_writer_unlock (&priv->rw_lock);
	g_strfreev (env_copy);

	g_object_notify (G_OBJECT (channel), "env");
}

/**
 * pkd_channel_get_id:
 * @channel: A #PkdChannel
 *
 * Retrieves the unique identifier of the #PkdChannel.  The id is only unique
 * to the running instance of the program.  Id's will be reused at next startup.
 *
 * Return value: the channel's identifier
 */
gint
pkd_channel_get_id (PkdChannel *channel)
{
	g_return_val_if_fail (PKD_IS_CHANNEL (channel), 0);
	return channel->priv->id;
}

/**
 * pkd_channel_start:
 * @channel: A #PkdChannel
 * @error: A location for a #GError or %NULL
 *
 * Attempts to start a #PkdChannel<!-- -->'s recording process.  If the channel
 * cannot be started, %FALSE is returned and @error is set.
 *
 * Return value: %TRUE on success
 */
gboolean
pkd_channel_start (PkdChannel  *channel,
                   GError     **error)
{
	PkdChannelPrivate *priv;
	gboolean           result = FALSE;

	g_return_val_if_fail (PKD_IS_CHANNEL (channel), FALSE);

	priv = channel->priv;

	g_static_rw_lock_writer_lock (&priv->rw_lock);

	switch (priv->state) {
	case STATE_READY:
	case STATE_PAUSED: {
		priv->state = STATE_STARTED;
		/* TODO: Process Execution Hooks */
		result = TRUE;
		break;
	}
	default:
		g_set_error (error, PKD_CHANNEL_ERROR, PKD_CHANNEL_ERROR_INVALID,
		             "Channel must be in ready or paused state to start");
		result = FALSE;
		break;
	}

	g_static_rw_lock_writer_unlock (&priv->rw_lock);

	return result;
}

/**
 * pkd_channel_stop:
 * @channel: A #PkdChannel
 * @error: A location for a #GError or %NULL
 *
 * Stops the #PkdChannel<!-- -->'s recording process.  If there was an error, then
 * %FALSE is returned and @error is set.
 *
 * Return value: %TRUE on success
 */
gboolean
pkd_channel_stop (PkdChannel  *channel,
                  GError     **error)
{
	PkdChannelPrivate *priv;
	gboolean           result = FALSE;

	g_return_val_if_fail (PKD_IS_CHANNEL (channel), FALSE);

	priv = channel->priv;

	g_static_rw_lock_writer_lock (&priv->rw_lock);

	switch (priv->state) {
	case STATE_STARTED:
	case STATE_PAUSED: {
		priv->state = STATE_STOPPED;
		/* TODO: Stop Execution Hooks */
		result = TRUE;
		break;
	}
	default:
		g_set_error (error, PKD_CHANNEL_ERROR, PKD_CHANNEL_ERROR_INVALID,
		             "Channel must be in started or paused state to stop");
		result = FALSE;
		break;
	}

	g_static_rw_lock_writer_unlock (&priv->rw_lock);

	return result;
}

/**
 * pkd_channel_pause:
 * @channel: A #PkdChannel
 * @error: A location for a #GError or %NULL
 *
 * Pauses the #PkdChannel<!-- -->'s recording process.  If the channel is not
 * currently started, then %FALSE is returned and @error is set.
 *
 * Once paused, no more samples will be emitted until after the channel has
 * been started again with pkd_channel_unpause().
 *
 * Return value: %TRUE on success
 */
gboolean
pkd_channel_pause (PkdChannel  *channel,
                   GError     **error)
{
	PkdChannelPrivate *priv;
	gboolean           result = FALSE;

	g_return_val_if_fail (PKD_IS_CHANNEL (channel), FALSE);

	priv = channel->priv;

	g_static_rw_lock_writer_lock (&priv->rw_lock);

	switch (priv->state) {
	case STATE_STARTED: {
		priv->state = STATE_PAUSED;
		/* TODO: Pause Execution Hooks */
		result = TRUE;
		break;
	}
	default:
		g_set_error (error, PKD_CHANNEL_ERROR, PKD_CHANNEL_ERROR_INVALID,
		             "The channel must be started before pausing");
		result = FALSE;
	}

	g_static_rw_lock_writer_unlock (&priv->rw_lock);

	return result;
}

GQuark
pkd_channel_error_quark (void)
{
	return g_quark_from_static_string ("pkd-channel-error-quark");
}
