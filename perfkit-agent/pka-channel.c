/* pka-channel.c
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

#include "pka-channel.h"
#include "pka-source.h"
#include "pka-subscription.h"

/**
 * SECTION:pka-channel
 * @title: PkaChannel
 * @short_description: Data source aggregation and process management
 *
 * #PkaChannel encapsulates the logic for spawning a new inferior process or
 * attaching to an existing one.  #PkaSource<!-- -->'s are added to the
 * channel to provide instrumentation into the inferior.
 *
 * #PkaSource implementations will deliver data samples to the channel
 * directly.  The channel will in turn pass them on to any subscriptions that
 * are observing the channel.  This also occurs for #PkaManifest updates.
 */

G_DEFINE_TYPE (PkaChannel, pka_channel, G_TYPE_OBJECT)

/*
 * Internal methods used for management of samples and manifests.
 */
extern void pka_subscription_deliver_sample   (PkaSubscription *subscription,
                                               PkaSample       *sample);
extern void pka_subscription_deliver_manifest (PkaSubscription *subscription,
                                               PkaManifest     *manifest);
extern void pka_sample_set_source_id          (PkaSample       *sample,
                                               gint             source_id);
extern void pka_manifest_set_source_id        (PkaManifest     *manifest,
                                               gint             source_id);
extern void pka_source_notify_started         (PkaSource       *source,
                                               PkaSpawnInfo    *spawn_info);
extern void pka_source_notify_stopped         (PkaSource       *source);
extern void pka_source_notify_paused          (PkaSource       *source);
extern void pka_source_notify_unpaused        (PkaSource       *source);
extern void pka_source_set_channel            (PkaSource       *source,
                                               PkaChannel      *channel);

struct _PkaChannelPrivate
{
	guint         channel_id; /* Monotonic id for the channel. */
	PkaSpawnInfo  spawn_info; /* Needed information for spawning process. */
	gboolean      spawned;    /* Set when spawning so we only kill processes
	                           * that we have spawned.
	                           */

	GMutex       *mutex;
	guint         state;      /* Current channel state. */
	GPtrArray    *subs;       /* Array of subscriptions. */
	GPtrArray    *sources;    /* Array of data sources. */
	GTree        *indexed;    /* Pointer-to-index data source map. */
	GTree        *manifests;  /* Pointer-to-manifest map. */
};

static guint channel_seq = 0;

/**
 * pka_channel_get_id:
 * @channel: A #PkaChannel
 *
 * Retrieves the channel id.
 *
 * Returns: The channel id.
 *
 * Side effects: None.
 */
guint
pka_channel_get_id (PkaChannel *channel)
{
	g_return_val_if_fail(PKA_IS_CHANNEL(channel), 0);
	return channel->priv->channel_id;
}

/**
 * pka_channel_new:
 * @spawn_info: A #PkaSpawnInfo describing the new channel.
 *
 * Creates a new instance of #PkaChannel.  The settings for the channel are
 * taken from @spawn_info and are immutable after channel creation.
 *
 * Return value: the newly created #PkaChannel instance.
 *
 * Side effects: None.
 */
PkaChannel*
pka_channel_new (const PkaSpawnInfo *spawn_info)
{
	PkaChannelPrivate *priv;
	PkaChannel *channel;

	channel = g_object_new(PKA_TYPE_CHANNEL, NULL);
	priv = channel->priv;

	priv->spawn_info.pid = spawn_info->pid;
	priv->spawn_info.target = g_strdup(spawn_info->target);
	priv->spawn_info.args = g_strdupv(spawn_info->args);
	priv->spawn_info.env = g_strdupv(spawn_info->env);
	priv->spawn_info.working_dir = g_strdup(spawn_info->working_dir);

	return channel;
}

/**
 * pka_channel_get_target:
 * @channel: A #PkaChannel
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
pka_channel_get_target (PkaChannel *channel)
{
	g_return_val_if_fail(PKA_IS_CHANNEL(channel), NULL);
	return channel->priv->spawn_info.target;
}

/**
 * pka_channel_get_working_dir:
 * @channel: A #PkaChannel
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
pka_channel_get_working_dir (PkaChannel *channel)
{
	g_return_val_if_fail(PKA_IS_CHANNEL(channel), NULL);
	return channel->priv->spawn_info.working_dir;
}

/**
 * pka_channel_get_args:
 * @channel: A #PkaChannel
 *
 * Retrieves the "args" property.
 *
 * Returns: A string array containing the arguments for the process.  This
 *   value should not be modified or freed.
 *
 * Side effects: None.
 */
gchar**
pka_channel_get_args (PkaChannel *channel)
{
	g_return_val_if_fail(PKA_IS_CHANNEL(channel), NULL);
	return channel->priv->spawn_info.args;
}

/**
 * pka_channel_get_env:
 * @channel: A #PkaChannel
 *
 * Retrieves the "env" property.
 *
 * Returns: A string array containing the environment variables to set before
 *   the process has started.  The value should not be modified or freed.
 *
 * Side effects: None.
 */
gchar**
pka_channel_get_env (PkaChannel *channel)
{
	g_return_val_if_fail(PKA_IS_CHANNEL(channel), NULL);
	return channel->priv->spawn_info.env;
}

/**
 * pka_channel_get_pid:
 * @channel: A #PkaChannel
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
pka_channel_get_pid (PkaChannel *channel)
{
	g_return_val_if_fail(PKA_IS_CHANNEL(channel), 0);
	return channel->priv->spawn_info.pid;
}

static void
pka_channel_inferior_exited (GPid        pid,
                             gint        status,
                             PkaChannel *channel)
{
	g_return_if_fail(PKA_IS_CHANNEL(channel));

	/*
	 * TODO:
	 *
	 *    We should look at status and store it so that we can provide that
	 *    information to profiler user interfaces.
	 */

	pka_channel_stop(channel, FALSE, NULL);
	g_object_unref(channel);
}

/**
 * pka_channel_start:
 * @channel: A #PkaChannel
 * @error: A location for a #GError or %NULL
 *
 * Attempts to start the channel.  If the channel was successfully started
 * the attached #PkaSource<!-- -->'s will be notified to start creating
 * samples.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects:
 *   The channels state-machine is altered.
 *   Data sources are notified to start sending manifests and samples.
 */
gboolean
pka_channel_start (PkaChannel  *channel,
                   GError     **error)
{
	PkaChannelPrivate *priv;
	gboolean success = FALSE;
	GError *local_error = NULL;
	const gchar *working_dir;
	gchar **argv = NULL;
	gint len, i;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(channel->priv->state == PKA_CHANNEL_READY, FALSE);

	priv = channel->priv;

	g_mutex_lock(priv->mutex);

	/*
	 * Ensure we haven't yet been started.
	 */
	if (priv->state != PKA_CHANNEL_READY) {
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            _("Cannot start channel.  Channel already started."));
		goto unlock;
	}

	g_message("Starting channel %d.", pka_channel_get_id(channel));

	/*
	 * Spawn the inferior process if necessary.
	 */
	if (!priv->spawn_info.pid) {
		/*
		 * Build the spawned argv.  Copy in arguments if needed.
		 */
		if (priv->spawn_info.args) {
			len = g_strv_length(priv->spawn_info.args);
			argv = g_malloc0(sizeof(gchar*) * (len + 2));
			for (i = 0; i < len; i++) {
				argv[i + 1] = g_strdup(priv->spawn_info.args[i]);
			}
		} else {
			argv = g_malloc0(sizeof(gchar*) * 2);
		}

		/*
		 * Set the executable target.
		 */
		argv[0] = g_strdup(priv->spawn_info.target);

		/*
		 * Determine the working directory.
		 */
		working_dir = priv->spawn_info.working_dir;
		if (!working_dir || strlen(working_dir) == 0) {
			working_dir = g_get_tmp_dir();
		}

		/*
		 * Attempt to spawn the process.
		 */
		if (!g_spawn_async(working_dir,
		                   argv,
		                   priv->spawn_info.env,
		                   G_SPAWN_SEARCH_PATH |
		                   G_SPAWN_STDERR_TO_DEV_NULL |
		                   G_SPAWN_STDOUT_TO_DEV_NULL |
		                   G_SPAWN_DO_NOT_REAP_CHILD,
		                   NULL,
		                   NULL,
		                   &priv->spawn_info.pid,
		                   &local_error))
		{
			priv->state = PKA_CHANNEL_FAILED;
			g_warning(_("Error starting channel %d: %s"),
			          priv->channel_id, local_error->message);
			if (error) {
				*error = local_error;
			} else {
				g_error_free(local_error);
			}
			goto unlock;
		}

		/*
		 * Setup callback to reap child and stop the channel.
		 */
		g_child_watch_add(priv->spawn_info.pid,
		                  (GChildWatchFunc)pka_channel_inferior_exited,
		                  g_object_ref(channel));

		g_message("Channel %d started with process %u.",
		          pka_channel_get_id(channel),
		          priv->spawn_info.pid);
	}

	/*
	 * Tick the state machine to RUNNING.
	 */
	priv->spawned = TRUE;
	priv->state = PKA_CHANNEL_RUNNING;
	success = TRUE;

	/*
	 * Notify the included data channels of the inferior starting up.
	 */
	g_ptr_array_foreach(priv->sources,
	                    (GFunc)pka_source_notify_started,
	                    &priv->spawn_info);

unlock:
	g_mutex_unlock(priv->mutex);

	if (argv) {
		g_strfreev(argv);
	}

	return success;
}

/**
 * pka_channel_stop:
 * @channel: A #PkaChannel.
 * @killpid: If the inferior process should be terminated.
 * @error: A location for a #GError or %NULL.
 *
 * Attempts to stop the channel.  If successful, the attached
 * #PkaSource<!-- -->'s will be notified that they should stop sending
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
pka_channel_stop (PkaChannel  *channel,
                  gboolean     killpid,
                  GError     **error)
{
	PkaChannelPrivate *priv;
	gboolean result = TRUE;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);

	priv = channel->priv;

	g_mutex_lock(priv->mutex);

	switch (priv->state) {
	case PKA_CHANNEL_RUNNING:
	case PKA_CHANNEL_PAUSED:
		priv->state = PKA_CHANNEL_STOPPED;

		g_message("Stopping channel %d.", pka_channel_get_id(channel));

		/*
		 * Notify sources of channel stopping.
		 */
		g_ptr_array_foreach(priv->sources,
		                    (GFunc)pka_source_notify_stopped,
		                    NULL);

		/*
		 * Kill the process if needed.
		 */
		if (killpid && priv->spawned && priv->spawn_info.pid) {
			g_message("Killing process %d.", priv->spawn_info.pid);
			kill(priv->spawn_info.pid, SIGKILL);
		}

		break;
	case PKA_CHANNEL_READY:
		result = FALSE;
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            _("Channel has not yet been started."));
		/* Fall through */
	case PKA_CHANNEL_STOPPED:
	case PKA_CHANNEL_FAILED:
		goto unlock;
	default:
		g_assert_not_reached();
	}

unlock:
	g_mutex_unlock(priv->mutex);

	return result;
}

/**
 * pka_channel_pause:
 * @channel: A #PkaChannel.
 * @error: A location for a #GError, or %NULL.
 *
 * Attempts to pause the channel.  If successful, the attached
 * #PkaSource<!-- -->'s are notified to pause as well.  Any samples delivered
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
pka_channel_pause (PkaChannel  *channel,
                   GError     **error)
{
	PkaChannelPrivate *priv;
	gboolean result = TRUE;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);

	priv = channel->priv;

	g_mutex_lock(priv->mutex);

	switch (priv->state) {
	case PKA_CHANNEL_READY:
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            _("Cannot pause channel; not yet started."));
		result = FALSE;
		break;
	case PKA_CHANNEL_STOPPED:
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            _("Cannot pause channel; channel stopped."));
		result = FALSE;
		break;
	case PKA_CHANNEL_FAILED:
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            _("Cannot pause channel; channel failed to start."));
		result = FALSE;
		break;
	case PKA_CHANNEL_PAUSED:
		/* Already paused. */
		break;
	case PKA_CHANNEL_RUNNING:
		priv->state = PKA_CHANNEL_PAUSED;

		g_message("Pausing channel %d.", pka_channel_get_id(channel));

		/*
		 * Notify sources that we have paused.
		 */
		g_ptr_array_foreach(priv->sources,
		                    (GFunc)pka_source_notify_paused,
		                    NULL);
		break;
	default:
		g_assert_not_reached();
	}

	g_mutex_unlock(priv->mutex);

	return TRUE;
}

/**
 * pka_channel_traverse_manifests_cb:
 * @source: A #PkaSource
 * @manifest: A #PkaManifest
 * @channel: A #PkaChannel
 *
 * Internal method for use within a GTree foreach to notif all subscriptions
 * of the current manifest.
 *
 * Side effects: The manifest is broadcasted to the subscriptions.
 */
static gboolean
pka_channel_traverse_manifests_cb (PkaSource   *source,
                                   PkaManifest *manifest,
                                   PkaChannel  *channel)
{
	PkaChannelPrivate *priv = channel->priv;

	g_ptr_array_foreach(priv->subs,
	                    (GFunc)pka_subscription_deliver_manifest,
	                    manifest);

	return FALSE;
}

/**
 * pka_channel_unpause:
 * @channel: A #PkaChannel.
 * @error: A location for a #GError or %NULL.
 *
 * Attempts to unpause a #PkaChannel.  If successful; the
 * #PkaSource<!-- -->'s will be notified to unpause and continue sending
 * samples.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects:
 *   The state machine is altered.
 *   Data sources are notified to resume.
 */
gboolean
pka_channel_unpause (PkaChannel  *channel,
                     GError     **error)
{
	PkaChannelPrivate *priv;
	gboolean result = TRUE;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);

	priv = channel->priv;

	g_mutex_lock(priv->mutex);

	switch (priv->state) {
	case PKA_CHANNEL_PAUSED:
		priv->state = PKA_CHANNEL_RUNNING;

		g_message("Unpausing channel %d.", pka_channel_get_id(channel));

		/*
		 * Notify subscriptions of the current manifests.
		 */
		g_tree_foreach(priv->manifests,
		               (GTraverseFunc)pka_channel_traverse_manifests_cb,
		               channel);

		/*
		 * Notify sources to continue.
		 */
		g_ptr_array_foreach(priv->sources,
		                    (GFunc)pka_source_notify_unpaused,
		                    NULL);

		break;
	case PKA_CHANNEL_READY:
	case PKA_CHANNEL_RUNNING:
	case PKA_CHANNEL_STOPPED:
	case PKA_CHANNEL_FAILED:
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            _("Cannot unpause channel; channel not paused."));
		result = FALSE;
		break;
	default:
		g_assert_not_reached();
	}

	g_mutex_unlock(priv->mutex);

	return result;
}

/**
 * pka_channel_deliver_sample:
 * @channel: A #PkaChannel
 * @source: A #PkaSource
 * @sample: A #PkaSample
 *
 * Internal method used by #PkaSource to deliver its samples to our channel.
 *
 * Side effects: None.
 */
void
pka_channel_deliver_sample (PkaChannel *channel,
                            PkaSource  *source,
                            PkaSample  *sample)
{
	PkaChannelPrivate *priv;
	gint i, idx;

	g_return_if_fail(channel != NULL);
	g_return_if_fail(source != NULL);
	g_return_if_fail(sample != NULL);

	/*
	 * NOTES:
	 *
	 *   Can we look into dropping this into a Multi-Producer, Single Consumer
	 *   Queue that ships the samples over to the subscription every so often?
	 *   It would be nice not to screw with the sample thread by introducing
	 *   our mutexes.  It could cause collateral damage between sources during
	 *   contention.
	 *
	 */

	priv = channel->priv;

	g_mutex_lock(priv->mutex);

	/*
	 * Drop the sample if we are not currently recording samples.
	 */
	if (priv->state != PKA_CHANNEL_RUNNING) {
		goto unlock;
	}

	/*
	 * Pass the sample to all of the observing subscriptions.
	 */
	for (i = 0; i < priv->subs->len; i++) {
		idx = GPOINTER_TO_INT(g_tree_lookup(priv->indexed, source));
		pka_sample_set_source_id(sample, idx);
		pka_subscription_deliver_sample(priv->subs->pdata[i],
		                                sample);
	}

unlock:
	g_mutex_unlock(priv->mutex);
}

/**
 * pka_channel_deliver_manifest:
 * @channel: A #PkaChannel
 * @source: A #PkaSource
 * @manifest: A #PkaManifest
 *
 * Internal method used by #PkaSource to deliver its manifest updates to our
 * channel.
 *
 * Side effects: None.
 */
void
pka_channel_deliver_manifest (PkaChannel  *channel,
                              PkaSource   *source,
                              PkaManifest *manifest)
{
	PkaChannelPrivate *priv;
	gint i, idx;

	g_return_if_fail(channel != NULL);
	g_return_if_fail(source != NULL);
	g_return_if_fail(manifest != NULL);

	priv = channel->priv;

	g_mutex_lock(priv->mutex);

	/*
	 * NOTES:
	 *
	 *   We always store the most recent copy of the manifest for when
	 *   subscriptions are added.  That way we can notify them of the
	 *   current data manifest before we pass samples.
	 */

	/*
	 * Look up the source index and store it in the manifest.
	 */
	idx = GPOINTER_TO_INT(g_tree_lookup(priv->indexed, source));
	pka_manifest_set_source_id(manifest, idx);

	/*
	 * Store the manifest for future lookup.
	 */
	g_tree_insert(priv->manifests, source, pka_manifest_ref(manifest));

	/*
	 * We are finished for now unless we are active.
	 */
	if (priv->state != PKA_CHANNEL_RUNNING) {
		goto unlock;
	}

	/*
	 * Notify subscriptions of the manifest update.
	 */
	for (i = 0; i < priv->subs->len; i++) {
		pka_subscription_deliver_manifest(priv->subs->pdata[i],
		                                  manifest);
	}

unlock:
	g_mutex_unlock(priv->mutex);
}

static gboolean
deliver_manifest_to_sub (gpointer key,
                         gpointer value,
                         gpointer data)
{
	pka_subscription_deliver_manifest(data, value);
	return FALSE;
}

/**
 * pka_channel_add_subscription:
 * @channel: A #PkaChannel
 * @subscription: A #PkaSubscription
 *
 * Internal method for registering a subscription to a channel.
 *
 * Side effects: None.
 */
void
pka_channel_add_subscription (PkaChannel      *channel,
                              PkaSubscription *subscription)
{
	PkaChannelPrivate *priv;

	g_return_if_fail(PKA_IS_CHANNEL(channel));
	g_return_if_fail(subscription != NULL);

	priv = channel->priv;

	g_mutex_lock(priv->mutex);

	/*
	 * Store subscription reference.
	 */
	g_ptr_array_add(priv->subs, pka_subscription_ref(subscription));

	/*
	 * Notify subscription of current manifests.
	 */
	if ((priv->state & (PKA_CHANNEL_RUNNING | PKA_CHANNEL_PAUSED)) != 0) {
		g_tree_foreach(priv->manifests, deliver_manifest_to_sub, subscription);
	}

	g_mutex_unlock(priv->mutex);
}

/**
 * pka_channel_remove_subscription:
 * @channel: A #PkaChannel
 * @subscription: A #PkaSubscription
 *
 * Internal method for unregistering a subscription from a channel.
 *
 * Side effects: None.
 */
void
pka_channel_remove_subscription (PkaChannel      *channel,
                                 PkaSubscription *subscription)
{
	PkaChannelPrivate *priv;

	g_return_if_fail(PKA_IS_CHANNEL(channel));
	g_return_if_fail(subscription != NULL);

	priv = channel->priv;

	g_mutex_lock(priv->mutex);
	if (g_ptr_array_remove(priv->subs, subscription)) {
		pka_subscription_unref(subscription);
	}
	g_mutex_unlock(priv->mutex);
}

PkaChannelState
pka_channel_get_state (PkaChannel *channel)
{
	PkaChannelPrivate *priv;
	PkaChannelState state;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), 0);

	priv = channel->priv;

	g_mutex_lock(priv->mutex);
	state = priv->state;
	g_mutex_unlock(priv->mutex);

	return state;
}

gboolean
pka_channel_remove_source (PkaChannel  *channel,
                           gint         source_id,
                           GError     **error)
{
	return TRUE;
}

static void
pka_channel_finalize (GObject *object)
{
	PkaChannelPrivate *priv = PKA_CHANNEL(object)->priv;

	priv->spawn_info.pid = 0;
	g_free(priv->spawn_info.target);
	g_free(priv->spawn_info.working_dir);
	g_strfreev(priv->spawn_info.args);
	g_strfreev(priv->spawn_info.env);

	g_ptr_array_foreach(priv->sources, (GFunc)g_object_unref, NULL);
	g_ptr_array_foreach(priv->subs, (GFunc)pka_subscription_unref, NULL);

	g_ptr_array_free(priv->sources, TRUE);
	g_ptr_array_free(priv->subs, TRUE);
	g_mutex_free(priv->mutex);

	G_OBJECT_CLASS(pka_channel_parent_class)->finalize(object);
}

static void
pka_channel_class_init (PkaChannelClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pka_channel_finalize;
	g_type_class_add_private(object_class, sizeof(PkaChannelPrivate));
}

static gint
pointer_compare (gconstpointer a,
                 gconstpointer b)
{
	return (a == b) ? 0 : (a - b);
}

static void
pka_channel_init (PkaChannel *channel)
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
	                                            PKA_TYPE_CHANNEL,
	                                            PkaChannelPrivate);
	channel->priv->subs = g_ptr_array_new();
	channel->priv->sources = g_ptr_array_new();
	channel->priv->mutex = g_mutex_new();
	channel->priv->indexed = g_tree_new(pointer_compare);
	channel->priv->manifests = g_tree_new_full(
		(GCompareDataFunc)pointer_compare, NULL, NULL,
		(GDestroyNotify)pka_manifest_unref);
	channel->priv->channel_id = channel_id;
	channel->priv->state = PKA_CHANNEL_READY;
}

GQuark
pka_channel_error_quark (void)
{
	return g_quark_from_static_string("pka-channel-error-quark");
}
