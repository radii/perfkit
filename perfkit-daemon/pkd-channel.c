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

/**
 * SECTION:pkd-channel
 * @title: PkdChannel
 * @short_description: Perfkit data channels
 *
 * #PkdChannel represents a way to get data samples from a system.  It can
 * contain multiple #PkdSource<!-- -->'s.
 */

G_DEFINE_TYPE (PkdChannel, pkd_channel, G_TYPE_OBJECT)

struct _PkdChannelPrivate
{
	GStaticRWLock   rw_lock;
	gchar          *dir;
	gchar         **args;
	GPid            pid;
	gchar          *target;
	gchar         **env;
};

static void
pkd_channel_finalize (GObject *object)
{
	G_OBJECT_CLASS (pkd_channel_parent_class)->finalize (object);
}

static void
pkd_channel_class_init (PkdChannelClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkd_channel_finalize;
	g_type_class_add_private (object_class, sizeof (PkdChannelPrivate));
}

static void
pkd_channel_init (PkdChannel *channel)
{
	channel->priv = G_TYPE_INSTANCE_GET_PRIVATE ((channel),
	                                             PKD_TYPE_CHANNEL,
	                                             PkdChannelPrivate);
}

/**
 * pkd_channel_new:
 *
 * Creates a new instance of #PkdChannel.
 *
 * Return value: the newly created #PkdChannel instance.
 */
PkdChannel*
pkd_channel_new (void)
{
	return g_object_new (PKD_TYPE_CHANNEL, NULL);
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

