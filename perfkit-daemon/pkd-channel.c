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

#include <sys/types.h>
#include <signal.h>

#include <glib.h>
#include <glib/gi18n.h>

#include "pkd-channel.h"
#include "pkd-channel-priv.h"
#include "pkd-log.h"
#include "pkd-runtime.h"
#include "pkd-sample.h"
#include "pkd-source.h"

/**
 * SECTION:pkd-channel
 * @title: PkdChannel
 * @short_description: Perfkit data channels
 *
 * #PkdChannel is a funnel for multiple #PkdSource<!-- -->'s.  It allows for
 * retrieving aggregated samples from sources.
 */

static gboolean do_start (PkdChannel *channel, GError **error);
static void     do_stop  (PkdChannel *channel);

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

struct _PkdChannelPrivate
{
	GStaticRWLock   rw_lock;
	gint            id;        /* Unique ID of the channel */
	gint            state;     /* Current State of the channel */
	gchar          *dir;       /* Working directory for child process */
	gchar         **args;      /* GStrv of arguments for target */
	GPid            pid;       /* PID to attach to or PID of child process */
	gchar          *target;    /* Path to executable to spawn if needed */
	gchar         **env;       /* GStrv of KEY=VALUE environment vars */
	gboolean        spawned;   /* If we are responsible for the child process */
	GPtrArray      *sources;   /* Array of PkdSource's */
	GPtrArray      *subs;      /* Array of Subscriptions */
};

typedef struct
{
	gint id;
	PkdChannelSubscriptionFunc func;
	gpointer user_data;
} Subscription;

static gint channel_seq = 0;
static gint sub_seq = 0;

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
 *
 * Side effects: None
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
 *
 * Side effects: Sets the directory property of the channel.
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
 *
 * Side effects: None
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
 *
 * Side effects: Alters the arguments of the #PkChannel.
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
 *
 * Side effects: None
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
 *
 * Side effects: Alters the pid of the channel.  No side effects will occur
 *   if the process was spawned.
 */
void
pkd_channel_set_pid (PkdChannel *channel,
                     GPid        pid)
{
	PkdChannelPrivate *priv;

	g_return_if_fail (PKD_IS_CHANNEL (channel));

	priv = channel->priv;

	g_static_rw_lock_writer_lock (&priv->rw_lock);
	if (!priv->spawned)
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
 *
 * Side effects: None
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
 *
 * Side effects: Alters the target of the #PkChannel.  No side effects will
 *   occur if the process has already been spawned.
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
	if (!priv->spawned)
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
 *
 * Side effects: None
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
 *
 * Side effects: Alters the environment of #PkChannel.  No side effects will
 *   occur if the target has been spawned.
 */
void
pkd_channel_set_env (PkdChannel* channel,
                     const gchar ** env)
{
	PkdChannelPrivate  *priv;
	gchar             **env_copy = NULL;

	g_return_if_fail (PKD_IS_CHANNEL (channel));

	priv = channel->priv;

	g_static_rw_lock_writer_lock (&priv->rw_lock);
	if (!priv->spawned) {
		env_copy = priv->env;
		priv->env = g_strdupv ((gchar**)env);
	}
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
 *
 * Side effects: None
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
 *
 * Side effects: Alters the state machine of the channel if the channel is in
 *   the READY state.
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
	case PKD_CHANNEL_READY: {
		g_message (_("Starting channel %d"), priv->id);
		priv->state = PKD_CHANNEL_STARTED;
		if (!(result = do_start (channel, error))) {
			priv->state = PKD_CHANNEL_STOPPED;
			result = FALSE;
		}
		break;
	}
	default:
		g_set_error (error, PKD_CHANNEL_ERROR, PKD_CHANNEL_ERROR_INVALID,
		             _("Channel must be in ready or paused state to start"));
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
 * Stops the #PkdChannel<!-- -->'s recording process.  If there was an error,
 * then %FALSE is returned and @error is set.
 *
 * Return value: %TRUE on success
 *
 * Side effects: Alters the state machine of the channel if the channel is in
 *   the READY, STARTED, or PAUSED state.
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
	case PKD_CHANNEL_READY:
	case PKD_CHANNEL_STARTED:
	case PKD_CHANNEL_PAUSED: {
		priv->state = PKD_CHANNEL_STOPPED;
		g_message (_("Stopping channel %d"), priv->id);
		do_stop (channel);
		result = TRUE;
		break;
	}
	default:
		g_set_error (error, PKD_CHANNEL_ERROR, PKD_CHANNEL_ERROR_INVALID,
		             _("Channel must be in started or paused state to stop"));
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
 *
 * Side effects: Alters the state machine of the channel if the channel is in
 *   the STARTED state.
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
	case PKD_CHANNEL_STARTED: {
		g_message (_("Pausing channel %d"), priv->id);
		priv->state = PKD_CHANNEL_PAUSED;
		/* TODO: Pause Execution Hooks */
		result = TRUE;
		break;
	}
	default:
		g_set_error (error, PKD_CHANNEL_ERROR, PKD_CHANNEL_ERROR_INVALID,
		             _("The channel must be started before pausing"));
		result = FALSE;
	}

	g_static_rw_lock_writer_unlock (&priv->rw_lock);

	return result;
}

/**
 * pkd_channel_unpause:
 * @channel: A #PkdChannel
 * @error: A location for a #GError or %NULL
 *
 * Unpauses the #PkdChannel<!-- -->'s recording process.  If the channel is not
 * currently paused, then %FALSE is returned and @error is set.
 *
 * Once unpaused, recording samples will continue to be emitted on their
 * configured frequency.
 *
 * See pkd_channel_pause() to pause a #PkdChannel.
 *
 * Return value: %TRUE on success
 *
 * Side effects: ALters the state machine of the channel if the channel is in
 *   the PAUSED state.
 */
gboolean
pkd_channel_unpause (PkdChannel  *channel,
                     GError     **error)
{
	PkdChannelPrivate *priv;
	gboolean           result = FALSE;

	g_return_val_if_fail (PKD_IS_CHANNEL (channel), FALSE);

	priv = channel->priv;

	g_static_rw_lock_writer_lock (&priv->rw_lock);

	switch (priv->state) {
	case PKD_CHANNEL_PAUSED: {
		g_message ("Unpausing channel %d", priv->id);
		priv->state = PKD_CHANNEL_STARTED;
		/* TODO: Unpause Execution Hooks */
		result = TRUE;
		break;
	}
	default:
		g_set_error (error, PKD_CHANNEL_ERROR, PKD_CHANNEL_ERROR_INVALID,
		             "The channel must be paused before unpausing");
		result = FALSE;
	}

	g_static_rw_lock_writer_unlock (&priv->rw_lock);

	return result;
}

/**
 * pkd_channel_get_state:
 * @channel: A #PkdChannel
 *
 * Retrieves the current state of the #PkdChannel as a #PkdChannelState
 * enumeration.
 *
 * Return value: the current state of the channel
 *
 * Side effects: None
 */
PkdChannelState
pkd_channel_get_state (PkdChannel *channel)
{
	g_return_val_if_fail (PKD_IS_CHANNEL (channel), -1);
	return g_atomic_int_get (&channel->priv->state);
}

GQuark
pkd_channel_error_quark (void)
{
	return g_quark_from_static_string ("pkd-channel-error-quark");
}

/*
 * Internal method used by PkdSource to notify the channel it
 * will be delivering samples to it.
 */
void
pkd_channel_add_source (PkdChannel *channel,
                        PkdSource  *source)
{
	PkdChannelPrivate *priv;

	g_return_if_fail (PKD_IS_CHANNEL (channel));
	g_return_if_fail (PKD_IS_SOURCE (source));

	priv = channel->priv;

	g_static_rw_lock_writer_lock (&priv->rw_lock);
	g_ptr_array_add (priv->sources, g_object_ref (source));
	g_static_rw_lock_writer_unlock (&priv->rw_lock);
}

static gboolean
do_spawn (PkdChannel  *channel,
          GError     **error)
{
	PkdChannelPrivate  *priv;
	gboolean            result;
	gchar             **argv,
	                   *args;
	gint                argc,
	                    i;

	priv = channel->priv;

	/* command target + args + NULL seminole */
	argc = g_strv_length (priv->args) + 2;
	argv = g_malloc0 (sizeof (gchar*) * argc);

	argv [0] = g_strdup (priv->target);
	for (i = 1; i < argc - 1; i++)
		argv [i] = g_strdup (priv->args [i - 1]);

	/* log the creation of the process */
	args = g_strjoinv (" ", argv);
	g_message ("Spawning process \"%s %s\"", argv [0], args);
	g_free (args);

	/* spawn the process. in the future we may want to add stdin/stdout
	 * redirection so that it may be proxied over DBUS.
	 */
	result = g_spawn_async (priv->dir,
	                        argv,
	                        priv->env,
	                        G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
	                        NULL,
	                        NULL,
	                        &priv->pid,
	                        error);

	g_strfreev (argv);

	return result;
}

static void
source_start_func (PkdSource  *source,
                   gpointer    user_data)
{
	GError *error = NULL;

	if (!pkd_source_start (source, &error)) {
		/* TODO: Handle failed data source */
		g_printerr ("%s\n", error->message);
		g_error_free (error);
		error = NULL;
	}
}

static gboolean
do_start (PkdChannel  *channel,
          GError     **error)
{
	PkdChannelPrivate *priv;
	PkdSource         *spawn = NULL;
	gboolean           spawned = TRUE;
	PkdSource         *source;
	gint               i;

	priv = channel->priv;

	if (!priv->pid) {
		for (i = 0; i < priv->sources->len; i++) {
			source = g_ptr_array_index (priv->sources, i);
			if (pkd_source_needs_spawn (source)) {
				spawn = source;
				break;
			}
		}

		if (spawn)
			spawned = pkd_source_spawn (spawn, error);
		else
			spawned = do_spawn (channel, error);

		priv->spawned = TRUE;
	}

	g_ptr_array_foreach (priv->sources, (GFunc)source_start_func, NULL);

	return spawned;
}

static void
do_stop (PkdChannel *channel)
{
	PkdChannelPrivate *priv;

	g_return_if_fail (PKD_IS_CHANNEL (channel));

	priv = channel->priv;

	/* Kill the process that we created if it was spawned by the channel.
	 * If the process was started by a channel, it is it's responsibility to
	 * stop the process when it is stopped.
	 */

	g_ptr_array_foreach (priv->sources, (GFunc)pkd_source_stop, NULL);

	if (priv->pid && priv->spawned)
		kill (priv->pid, SIGTERM);
}

/**
 * pkd_channel_deliver:
 * @channel: A #PkdChannel
 * @source: A #PkdSource
 * @sample: A #PkdSample
 *
 * Delivers a newly created #PkdSample from @source to @channel.  The
 * #PkdChannel instance will then forward the sample to listening
 * subscriptions.
 */
void
pkd_channel_deliver (PkdChannel *channel,
                     PkdSource  *source,
                     PkdSample  *sample)
{
	PkdChannelPrivate *priv;
	Subscription      *sub;
	gint               i;

	/* TODO:
	 *   - How should we handle delivery errors?
	 *   - Should we send samples asynchronously with no reply?
	 *   - Should we make a copy of the subscription array in the
	 *     critical section (ref'ing each) and then execute callbacks
	 *     outside the section.
	 */

	priv = channel->priv;

	pkd_sample_ref (sample);

	g_static_rw_lock_reader_lock (&priv->rw_lock);

	for (i = 0; i < priv->subs->len; i++) {
		sub = g_ptr_array_index (priv->subs, i);
		sub->func (channel, sample, sub->user_data);
	}

	g_static_rw_lock_reader_unlock (&priv->rw_lock);

	pkd_sample_unref (sample);
}

static void
subscription_free (Subscription *sub)
{
	g_return_if_fail (sub != NULL);

	memset (sub, 0, sizeof (Subscription));
	g_slice_free (Subscription, sub);
}

static Subscription*
subscription_new (PkdChannelSubscriptionFunc func,
                  gpointer                   user_data)
{
	Subscription *sub;

	g_return_val_if_fail (func != NULL, NULL);

	sub = g_slice_new0 (Subscription);
	sub->id = g_atomic_int_exchange_and_add (&sub_seq, 1);
	sub->func = func;
	sub->user_data = user_data;

	return sub;
}

/**
 * pkd_channel_subscribe:
 * @channel: A #PkdChannel
 * @func: A #PkdChannelSubscriptionFunc
 * @user_data: data for @func
 *
 * Subscribes to sample callbacks for the channel.
 *
 * Return value:
 *       A unique id for the subscription.
 *
 * Side effects:
 *       None.
 */
gint
pkd_channel_subscribe (PkdChannel                 *channel,
                       PkdChannelSubscriptionFunc  func,
                       gpointer                    user_data)
{
	PkdChannelPrivate *priv;
	Subscription *sub;

	g_return_val_if_fail (PKD_IS_CHANNEL (channel), -1);
	g_return_val_if_fail (func != NULL, -1);

	priv = channel->priv;

	sub = subscription_new (func, user_data);
	g_assert (sub);

	g_static_rw_lock_writer_lock (&priv->rw_lock);
	g_ptr_array_add (priv->subs, sub);
	g_static_rw_lock_writer_unlock (&priv->rw_lock);

	return sub->id;
}

/**************************************************************************
 *                        Private GObject Methods                         *
 **************************************************************************/

static void
pkd_channel_finalize (GObject *object)
{
	PkdChannelPrivate *priv;

	priv = PKD_CHANNEL (object)->priv;

	g_free (priv->dir);
	g_free (priv->target);
	g_strfreev (priv->args);
	g_strfreev (priv->env);
	g_ptr_array_unref (priv->sources);
	g_ptr_array_unref (priv->subs);

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

	object_class               = G_OBJECT_CLASS (klass);
	object_class->set_property = pkd_channel_set_property;
	object_class->get_property = pkd_channel_get_property;
	object_class->finalize     = pkd_channel_finalize;
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
}

static void
pkd_channel_init (PkdChannel *channel)
{
	channel->priv = G_TYPE_INSTANCE_GET_PRIVATE ((channel),
	                                             PKD_TYPE_CHANNEL,
	                                             PkdChannelPrivate);

	channel->priv->state = PKD_CHANNEL_READY;
	channel->priv->args = g_malloc0 (sizeof (gchar*));
	channel->priv->id = g_atomic_int_exchange_and_add (&channel_seq, 1);
	channel->priv->dir = g_strdup ("/");
	channel->priv->sources = g_ptr_array_sized_new (16);
	g_ptr_array_set_free_func (channel->priv->sources,
	                           (GDestroyNotify)g_object_unref);
	channel->priv->subs = g_ptr_array_new ();
	g_ptr_array_set_free_func (channel->priv->subs,
	                           (GDestroyNotify)subscription_free);
}
