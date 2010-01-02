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

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>

#include "pkd-channel.h"
#include "pkd-spawn-info.h"
#include "pkd-subscription.h"

/**
 * SECTION:pkd-channel
 * @title: PkdChannel
 * @short_description: Data source aggregation and process management
 *
 * #PkdChannel encapsulates the logic for spawning a new inferior process or
 * attaching to an existing one.  #PkdSource<!-- -->'s are added to the
 * channel to provide instrumentation into the inferior.
 *
 * #PkdSource implementations will deliver data samples to the channel
 * directly.  The channel will in turn pass them on to any subscriptions that
 * are observing the channel.  This also occurs for #PkdManifest updates.
 */

G_DEFINE_TYPE (PkdChannel, pkd_channel, G_TYPE_OBJECT)

/*
 * Internal methods used for management of samples and manifests.
 */
extern void pkd_subscription_deliver_sample   (PkdSubscription *subscription,
                                               PkdSample       *sample);
extern void pkd_subscription_deliver_manifest (PkdSubscription *subscription,
                                               PkdManifest     *manifest);
extern void pkd_sample_set_source_id          (PkdSample       *sample,
                                               gint             source_id);
extern void pkd_manifest_set_source_id        (PkdManifest     *manifest,
                                               gint             source_id);
extern void pkd_source_notify_started         (PkdSource       *source);
extern void pkd_source_notify_stopped         (PkdSource       *source);
extern void pkd_source_notify_paused          (PkdSource       *source);
extern void pkd_source_notify_unpaused        (PkdSource       *source);

enum
{
	STATE_0,
	STATE_READY,
	STATE_STARTED,
	STATE_PAUSED,
	STATE_STOPPED,
	STATE_FAILED,
};

struct _PkdChannelPrivate
{
	guint         channel_id; /* Monotonic id for the channel. */
	PkdSpawnInfo  spawn_info; /* Needed information for spawning process. */
	gboolean      spawned;    /* Set when spawning so we only kill processes
	                           * that we have spawned.
	                           */

	GMutex       *mutex;
	guint         state;      /* Current channel state. */
	GPtrArray    *subs;       /* Array of subscriptions. */
	GPtrArray    *sources;    /* Array of data sources. */
	GTree        *indexed;    /* Pointer-to-index data source map. */
};

static guint channel_seq = 0;

/**
 * pkd_channel_get_id:
 * @channel: A #PkdChannel
 *
 * Retrieves the channel id.
 *
 * Returns: The channel id.
 *
 * Side effects: None.
 */
guint
pkd_channel_get_id (PkdChannel *channel)
{
	g_return_val_if_fail(PKD_IS_CHANNEL(channel), 0);
	return channel->priv->channel_id;
}

/**
 * pkd_channel_new:
 * @spawn_info: A #PkdSpawnInfo describing the new channel.
 *
 * Creates a new instance of #PkdChannel.  The settings for the channel are
 * taken from @spawn_info and are immutable after channel creation.
 *
 * Return value: the newly created #PkdChannel instance.
 *
 * Side effects: None.
 */
PkdChannel*
pkd_channel_new (const PkdSpawnInfo *spawn_info)
{
	return g_object_new(PKD_TYPE_CHANNEL, NULL);
}

/**
 * pkd_channel_get_target:
 * @channel: A #PkdChannel
 *
 * Retrieves the "target" executable for the channel.  If the channel is to
 * connect to an existing process this is %NULL.
 *
 * Returns: A string containing the target.  The value should not be modified
 *   or freed.
 *
 * Side effects: None.
 */
const gchar*
pkd_channel_get_target (PkdChannel *channel)
{
	g_return_val_if_fail(PKD_IS_CHANNEL(channel), NULL);
	return channel->priv->spawn_info.target;
}

/**
 * pkd_channel_get_working_dir:
 * @channel: A #PkdChannel
 *
 * Retrieves the "working-dir" property for the channel.  This is the directory
 * the process should be spawned within.
 *
 * Returns: A string containing the working directory.  The value should not be
 *   modified or freed.
 *
 * Side effects: None.
 */
const gchar*
pkd_channel_get_working_dir (PkdChannel *channel)
{
	g_return_val_if_fail(PKD_IS_CHANNEL(channel), NULL);
	return channel->priv->spawn_info.working_dir;
}

/**
 * pkd_channel_get_args:
 * @channel: A #PkdChannel
 *
 * Retrieves the "args" property.
 *
 * Returns: A string array containing the arguments for the process.  This
 *   value should not be modified or freed.
 *
 * Side effects: None.
 */
gchar**
pkd_channel_get_args (PkdChannel *channel)
{
	g_return_val_if_fail(PKD_IS_CHANNEL(channel), NULL);
	return channel->priv->spawn_info.args;
}

/**
 * pkd_channel_get_env:
 * @channel: A #PkdChannel
 *
 * Retrieves the "env" property.
 *
 * Returns: A string array containing the environment variables to set before
 *   the process has started.  The value should not be modified or freed.
 *
 * Side effects: None.
 */
gchar**
pkd_channel_get_env (PkdChannel *channel)
{
	g_return_val_if_fail(PKD_IS_CHANNEL(channel), NULL);
	return channel->priv->spawn_info.env;
}

/**
 * pkd_channel_get_pid:
 * @channel: A #PkdChannel
 *
 * Retrieves the pid of the process.  If an existing process is to be monitored
 * this is the pid of that process.  If a new process was to be spawned, after
 * the process has been spawned this will contain its process id.
 *
 * Returns: The process id of the monitored proces.
 *
 * Side effects: None.
 */
GPid
pkd_channel_get_pid (PkdChannel *channel)
{
	g_return_val_if_fail(PKD_IS_CHANNEL(channel), 0);
	return channel->priv->spawn_info.pid;
}

/**
 * pkd_channel_add_source:
 * @channel: A #PkdChannel
 * @source_info: A #PkdSourceInfo
 *
 * Creats a new #PkdSource and adds it to the channel.  The source is configured
 * to deliver samples and manifest updates to the channel.
 *
 * If this is called after the process has started, no source will be added
 * and %NULL will be returned.  This may change in the future.
 *
 * Returns: A #PkdSource.  If the caller wants to keep a reference to the source
 *   it should call g_object_ref() to increment the reference count.
 *
 * Side effects: None.
 */
PkdSource*
pkd_channel_add_source (PkdChannel    *channel,
                        PkdSourceInfo *source_info)
{
	PkdChannelPrivate *priv;
	PkdSource *source = NULL;
	guint idx;

	g_return_val_if_fail(PKD_IS_CHANNEL(channel), NULL);
	g_return_val_if_fail(PKD_IS_SOURCE_INFO(source_info), NULL);

	priv = channel->priv;

	g_mutex_lock(priv->mutex);

	/*
	 * Sources can only be added before pkd_channel_start() has been called.
	 */
	if (priv->state != STATE_READY) {
		goto unlock;
	}

	/*
	 * Create the new #PkdSource using the factory callback.
	 */
	source = pkd_source_info_create(source_info);
	if (!source) {
		g_warning("%s: Error creating instance of %s from source plugin.",
		          G_STRLOC, pkd_source_info_get_uid(source_info));
		goto unlock;
	}

	/*
	 * Insert the newly created PkdSource into our GPtrArray for fast iteration
	 * and our GTree for O(log n) worst-case index id lookups.
	 */
	g_ptr_array_add(priv->sources, source);
	idx = priv->sources->len;
	g_tree_insert(priv->indexed, source, GINT_TO_POINTER(idx));

unlock:
	g_mutex_unlock(priv->mutex);

	return source;
}

/**
 * pkd_channel_start:
 * @channel: A #PkdChannel
 * @error: A location for a #GError or %NULL
 *
 * Attempts to start the channel.  If the channel was successfully started
 * the attached #PkdSource<!-- -->'s will be notified to start creating
 * samples.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects:
 *   The channels state-machine is altered.
 *   Data sources are notified to start sending manifests and samples.
 */
gboolean
pkd_channel_start (PkdChannel  *channel,
                   GError     **error)
{
	PkdChannelPrivate *priv;
	gboolean success = FALSE;
	GError *local_error = NULL;
	gchar **argv = NULL;
	gint len;

	g_return_val_if_fail(PKD_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(channel->priv->state == STATE_READY, FALSE);

	priv = channel->priv;

	g_mutex_lock(priv->mutex);

	/*
	 * Ensure we haven't yet been started.
	 */
	if (priv->state != STATE_READY) {
		g_set_error(error, PKD_CHANNEL_ERROR, PKD_CHANNEL_ERROR_STATE,
		            _("Cannot start channel.  Channel already started."));
		goto unlock;
	}

	/*
	 * Spawn the inferior process if necessary.
	 */
	if (!priv->spawn_info.pid) {
		len = g_strv_length(priv->spawn_info.args);
		argv = g_malloc0(sizeof(gchar*) * (len + 2));
		argv[0] = priv->spawn_info.target;
		memcpy(&argv[1], priv->spawn_info.args, (sizeof(gchar*) * len));

		if (!g_spawn_async(priv->spawn_info.working_dir,
		                   argv,
		                   priv->spawn_info.env,
		                   G_SPAWN_SEARCH_PATH,
		                   NULL,
		                   NULL,
		                   &priv->spawn_info.pid,
		                   &local_error))
		{
			priv->state = STATE_FAILED;
			g_warning(_("Error starting channel %d: %s"),
			          priv->channel_id, local_error->message);
			if (error) {
				*error = local_error;
			} else {
				g_error_free(local_error);
			}
			goto unlock;
		}
	}

	/*
	 * Tick the state machine to STARTED.
	 */
	priv->spawned = TRUE;
	priv->state = STATE_STARTED;
	success = TRUE;

	/*
	 * Notify the included data channels of the inferior starting up.
	 */
	g_ptr_array_foreach(priv->sources,
	                    (GFunc)pkd_source_notify_started,
	                    NULL);

unlock:
	g_mutex_unlock(priv->mutex);

	if (argv) {
		g_strfreev(argv);
	}

	return success;
}

/**
 * pkd_channel_stop:
 * @channel: A #PkdChannel.
 * @killpid: If the inferior process should be terminated.
 * @error: A location for a #GError or %NULL.
 *
 * Attempts to stop the channel.  If successful, the attached
 * #PkdSource<!-- -->'s will be notified that they should stop sending
 * samples.  After the sources have been notified if @killpid is set
 * the process will be terminated.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects:
 *   The state machine is altered.
 *   Data sources are notified to stop.
 *   The process is terminated if @killpid is set.
 */
gboolean
pkd_channel_stop (PkdChannel  *channel,
                  gboolean     killpid,
                  GError     **error)
{
	PkdChannelPrivate *priv;
	gboolean result = TRUE;

	g_return_val_if_fail(PKD_IS_CHANNEL(channel), FALSE);

	priv = channel->priv;

	g_mutex_lock(priv->mutex);

	switch (priv->state) {
	case STATE_STARTED:
	case STATE_PAUSED:
		priv->state = STATE_STOPPED;

		g_message("Stopping channel %d.", pkd_channel_get_id(channel));

		/*
		 * Notify sources of channel stopping.
		 */
		g_ptr_array_foreach(priv->sources,
		                    (GFunc)pkd_source_notify_stopped,
		                    NULL);

		/*
		 * Kill the process if needed.
		 */
		if (killpid && priv->spawned && priv->spawn_info.pid) {
			g_message("Killing process %d.", priv->spawn_info.pid);
			kill(priv->spawn_info.pid, SIGKILL);
		}

		break;
	case STATE_READY:
		result = FALSE;
		g_set_error(error, PKD_CHANNEL_ERROR, PKD_CHANNEL_ERROR_STATE,
		            _("Channel has not yet been started."));
		/* Fall through */
	case STATE_STOPPED:
	case STATE_FAILED:
		goto unlock;
	default:
		g_assert_not_reached();
	}

unlock:
	g_mutex_unlock(priv->mutex);

	return result;
}

/**
 * pkd_channel_pause:
 * @channel: A #PkdChannel.
 * @error: A location for a #GError, or %NULL.
 *
 * Attempts to pause the channel.  If successful, the attached
 * #PkdSource<!-- -->'s are notified to pause as well.  Any samples delivered
 * while paused are silently dropped.  Updated manifests, however, are stored
 * for future delivery when unpausing occurs.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects:
 *   The state machine is altered.
 *   Data sources are notified to pause.
 */
gboolean
pkd_channel_pause (PkdChannel  *channel,
                   GError     **error)
{
	return TRUE;
}

/**
 * pkd_channel_unpause:
 * @channel: A #PkdChannel.
 * @error: A location for a #GError or %NULL.
 *
 * Attempts to unpause a #PkdChannel.  If successful; the
 * #PkdSource<!-- -->'s will be notified to unpause and continue sending
 * samples.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects:
 *   The state machine is altered.
 *   Data sources are notified to resume.
 */
gboolean
pkd_channel_unpause (PkdChannel  *channel,
                     GError     **error)
{
	return TRUE;
}

/**
 * pkd_channel_deliver_sample:
 * @channel: A #PkdChannel
 * @source: A #PkdSource
 * @sample: A #PkdSample
 *
 * Internal method used by #PkdSource to deliver its samples to our channel.
 *
 * Side effects: None.
 */
void
pkd_channel_deliver_sample (PkdChannel *channel,
                            PkdSource  *source,
                            PkdSample  *sample)
{
	PkdChannelPrivate *priv;
	gint i, idx;

	g_return_if_fail(channel != NULL);
	g_return_if_fail(source != NULL);
	g_return_if_fail(sample != NULL);

	/*
	 * NOTES:
	 *
	 *   Can we look into dropping this into an Multi-Producer, Single Consumer
	 *   Queue that ships the samples over to the subscription ever so often?
	 *   it would be nice not to screw with the sample thread by introducing
	 *   our mutexes.  It could cause collateral damage between sources during
	 *   contention.
	 *
	 */

	priv = channel->priv;

	g_mutex_lock(priv->mutex);

	/*
	 * TODO:
	 *
	 *   If the channel itself is paused, we should do something similar
	 *   to what subscriptions are doing now.  Cache the most recent
	 *   manifest and drop all samples.
	 *
	 */

	for (i = 0; i < priv->subs->len; i++) {
		idx = GPOINTER_TO_INT(g_tree_lookup(priv->indexed, source));
		pkd_sample_set_source_id(sample, idx);
		pkd_subscription_deliver_sample(priv->subs->pdata[i],
		                                sample);
	}

	g_mutex_unlock(priv->mutex);
}

/**
 * pkd_channel_deliver_manifest:
 * @channel: A #PkdChannel
 * @source: A #PkdSource
 * @manifest: A #PkdManifest
 *
 * Internal method used by #PkdSource to deliver its manifest updates to our
 * channel.
 *
 * Side effects: None.
 */
void
pkd_channel_deliver_manifest (PkdChannel  *channel,
                              PkdSource   *source,
                              PkdManifest *manifest)
{
	PkdChannelPrivate *priv;
	gint i, idx;

	g_return_if_fail(channel != NULL);
	g_return_if_fail(source != NULL);
	g_return_if_fail(manifest != NULL);

	priv = channel->priv;

	g_mutex_lock(priv->mutex);

	for (i = 0; i < priv->subs->len; i++) {
		idx = GPOINTER_TO_INT(g_tree_lookup(priv->indexed, source));
		pkd_manifest_set_source_id(manifest, idx);
		pkd_subscription_deliver_manifest(priv->subs->pdata[i],
		                                  manifest);
	}

	g_mutex_unlock(priv->mutex);
}

/**
 * pkd_channel_add_subscription:
 * @channel: A #PkdChannel
 * @subscription: A #PkdSubscription
 *
 * Internal method for registering a subscription to a channel.
 *
 * Side effects: None.
 */
void
pkd_channel_add_subscription (PkdChannel      *channel,
                              PkdSubscription *subscription)
{
	PkdChannelPrivate *priv;

	g_return_if_fail(PKD_IS_CHANNEL(channel));
	g_return_if_fail(subscription != NULL);

	priv = channel->priv;

	g_mutex_lock(priv->mutex);
	g_ptr_array_add(priv->subs, pkd_subscription_ref(subscription));
	g_mutex_unlock(priv->mutex);
}

/**
 * pkd_channel_remove_subscription:
 * @channel: A #PkdChannel
 * @subscription: A #PkdSubscription
 *
 * Internal method for unregistering a subscription from a channel.
 *
 * Side effects: None.
 */
void
pkd_channel_remove_subscription (PkdChannel      *channel,
                                 PkdSubscription *subscription)
{
	PkdChannelPrivate *priv;

	g_return_if_fail(PKD_IS_CHANNEL(channel));
	g_return_if_fail(subscription != NULL);

	priv = channel->priv;

	g_mutex_lock(priv->mutex);
	if (g_ptr_array_remove(priv->subs, subscription)) {
		pkd_subscription_unref(subscription);
	}
	g_mutex_unlock(priv->mutex);
}

static void
pkd_channel_finalize (GObject *object)
{
	PkdChannelPrivate *priv = PKD_CHANNEL(object)->priv;

	priv->spawn_info.pid = 0;
	g_free(priv->spawn_info.target);
	g_free(priv->spawn_info.working_dir);
	g_strfreev(priv->spawn_info.args);
	g_strfreev(priv->spawn_info.env);

	g_ptr_array_foreach(priv->sources, (GFunc)g_object_unref, NULL);
	g_ptr_array_foreach(priv->subs, (GFunc)pkd_subscription_unref, NULL);

	G_OBJECT_CLASS(pkd_channel_parent_class)->finalize(object);
}

static void
pkd_channel_class_init (PkdChannelClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkd_channel_finalize;
	g_type_class_add_private(object_class, sizeof(PkdChannelPrivate));
}

static void
pkd_channel_init (PkdChannel *channel)
{
	guint channel_id;

	/*
	 * Get the next monotonic id in the sequence.
	 */
	channel_id = g_atomic_int_exchange_and_add((gint *)&channel_seq, 1);

	/*
	 * Setup private members.
	 */
	channel->priv = G_TYPE_INSTANCE_GET_PRIVATE(channel,
	                                            PKD_TYPE_CHANNEL,
	                                            PkdChannelPrivate);
	channel->priv->subs = g_ptr_array_new();
	channel->priv->sources = g_ptr_array_new();
	channel->priv->mutex = g_mutex_new();
	channel->priv->indexed = g_tree_new(g_direct_equal);
	channel->priv->channel_id = channel_id;
	channel->priv->state = STATE_READY;
}

GQuark
pkd_channel_error_quark (void)
{
	return g_quark_from_static_string("pkd-channel-error-quark");
}
