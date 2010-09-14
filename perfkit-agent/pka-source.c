/* pka-source.c
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
#include "pka-log.h"
#include "pka-source.h"
#include "pka-subscription.h"

/**
 * SECTION:pka-source
 * @title: PkaSource
 * @short_description: Base class for data source implementations.
 *
 * #PkaSource provides the core logic needed for implementing
 * data sources within the Perfkit Agent.  It handles proper signalling
 * of events and subscription management.
 *
 * #PkaSource is an abstract #GType, implemented by #PkaSourceSimple
 * for a convenient way to implement data sources.
 */

G_DEFINE_ABSTRACT_TYPE (PkaSource, pka_source, G_TYPE_OBJECT)

/*
 * Locking Overview:
 *
 *   PkaSource must dance upon a tight-rope so that the locking model is
 *   correct.  This is required since it is generally tough to know exactly
 *   how a datasource is going to react to our various events.
 *
 *   Signalling of Started, Stopped, Muted, and Unmuted signals happens
 *   from the main loop to simplify locking models.  It also helps give
 *   some sort of guarantee of thread to the source implementations.
 */

extern void pka_sample_set_source_id   (PkaSample   *sample,
                                        gint         source_id);
extern void pka_manifest_set_source_id (PkaManifest *manifest,
                                        gint         source_id);

struct _PkaSourcePrivate
{
	GStaticRWLock  rw_lock;

	gint           id;
	gboolean       running;
	PkaPlugin     *plugin;
	PkaManifest   *manifest;
	GPtrArray     *subscriptions;
	PkaChannel    *channel;
};

enum
{
	STARTED,
	MUTED,
	UNMUTED,
	STOPPED,
	RESET,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/**
 * pka_source_emit_started:
 * @user_data: A pointer array containing a #PkaSource and #PkaSpawnInfo.
 *
 * #GSourceFunc style callback that emits the "started" signal.
 *
 * Returns: %FALSE.
 * Side effects: None.
 */
static gboolean
pka_source_emit_started (gpointer user_data) /* IN */
{
	PkaSource *source;
	PkaSpawnInfo *spawn_info;
	gpointer *state = user_data;

	g_return_val_if_fail(user_data != NULL, FALSE);

	ENTRY;
	source = state[0];
	spawn_info = state[1];
	g_signal_emit(source, signals[STARTED], 0, spawn_info);
	g_object_unref(source);
	pka_spawn_info_free(spawn_info);
	g_free(state);
	RETURN(FALSE);
}

/**
 * pka_source_queue_started:
 * @source: A #PkaSource.
 * @spawn_info: A #PkaSpawnInfo.
 *
 * Queues the emission of the "started" signal to the main thread.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_source_queue_started (PkaSource    *source,     /* IN */
                          PkaSpawnInfo *spawn_info) /* IN */
{
	PkaSourcePrivate *priv;
	gpointer *state;

	g_return_if_fail(PKA_IS_SOURCE(source));
	g_return_if_fail(spawn_info != NULL);

	ENTRY;
	priv = source->priv;
	state = g_new0(gpointer, 2);
	state[0] = g_object_ref(source);
	state[1] = pka_spawn_info_copy(spawn_info);
	g_timeout_add(0, pka_source_emit_started, state);
	EXIT;
}

/**
 * pka_source_emit_stopped:
 * @source: A #PkaSource.
 *
 * #GSourceFunc style function that emits the "stopped" signal.
 *
 * Returns: %FALSE.
 * Side effects: None.
 */
static gboolean
pka_source_emit_stopped (PkaSource *source) /* IN */
{
	g_return_val_if_fail(PKA_IS_SOURCE(source), FALSE);

	ENTRY;
	g_signal_emit(source, signals[STOPPED], 0);
	g_object_unref(source);
	RETURN(FALSE);
}

/**
 * pka_source_queue_stopped:
 * @source: A #PkaSource.
 *
 * Queues the emission of the "stopped" signal to the main thread.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_source_queue_stopped (PkaSource *source) /* IN */
{
	g_return_if_fail(PKA_IS_SOURCE(source));

	ENTRY;
	g_timeout_add(0, (GSourceFunc)pka_source_emit_stopped,
	              g_object_ref(source));
	EXIT;
}

/**
 * pka_source_emit_muted:
 * @source: A #PkaSource.
 *
 * #GSourceFunc style function that emits the "muted" signal.
 *
 * Returns: %FALSE.
 * Side effects: None.
 */
static gboolean
pka_source_emit_muted (PkaSource *source) /* IN */
{
	g_return_val_if_fail(PKA_IS_SOURCE(source), FALSE);

	ENTRY;
	g_signal_emit(source, signals[MUTED], 0);
	g_object_unref(source);
	RETURN(FALSE);
}

/**
 * pka_source_queue_muted:
 * @source: A #PkaSource.
 *
 * Queues the emission of the "muted" signal to the main thread.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_source_queue_muted (PkaSource *source) /* IN */
{
	g_return_if_fail(PKA_IS_SOURCE(source));

	ENTRY;
	g_timeout_add(0, (GSourceFunc)pka_source_emit_muted,
	              g_object_ref(source));
	EXIT;
}

/**
 * pka_source_emit_unmuted:
 * @source: A #PkaSource.
 *
 * #GSourceFunc style function that emits the "unmuted" signal.
 *
 * Returns: %FALSE.
 * Side effects: None.
 */
static gboolean
pka_source_emit_unmuted (PkaSource *source) /* IN */
{
	g_return_val_if_fail(PKA_IS_SOURCE(source), FALSE);

	ENTRY;
	g_signal_emit(source, signals[UNMUTED], 0);
	g_object_unref(source);
	RETURN(FALSE);
}

/**
 * pka_source_queue_unmuted:
 * @source: A #PkaSource.
 *
 * Queues the emission of the "unmuted" signal to the main thread.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_source_queue_unmuted (PkaSource *source) /* IN */
{
	g_return_if_fail(PKA_IS_SOURCE(source));

	ENTRY;
	g_timeout_add(0, (GSourceFunc)pka_source_emit_unmuted,
	              g_object_ref(source));
	EXIT;
}

/**
 * pka_source_emit_reset:
 * @user_data: A #PkaSource.
 *
 * #GSourceFunc style callback that emits the "reset" signal.
 *
 * Returns: %FALSE.
 * Side effects: None.
 */
static gboolean
pka_source_emit_reset (gpointer user_data) /* IN */
{
	PkaSource *source = user_data;

	g_return_val_if_fail(PKA_IS_SOURCE(source), FALSE);

	ENTRY;
	g_signal_emit(source, signals[RESET], 0);
	g_object_unref(source);
	RETURN(FALSE);
}

/**
 * pka_source_queue_reset:
 * @source: A #PkaSource.
 *
 * Queues the emission of the "reset" signal to the main thread.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_source_queue_reset (PkaSource *source) /* IN */
{
	g_return_if_fail(PKA_IS_SOURCE(source));

	ENTRY;
	g_timeout_add(0, pka_source_emit_reset, g_object_ref(source));
	EXIT;
}

/**
 * pka_source_set_channel:
 * @source: A #PkaSource
 * @channel: A #PkaChannel
 *
 * Internal method used by channels to attach themselves as the destination
 * for a sources samples and manifest.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_source_set_channel (PkaSource  *source,  /* IN */
                        PkaChannel *channel) /* IN */
{
	PkaSourcePrivate *priv;
	gboolean ret = FALSE;

	g_return_val_if_fail(PKA_IS_SOURCE(source), FALSE);
	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = source->priv;
	g_static_rw_lock_writer_lock(&priv->rw_lock);
	if (priv->channel) {
		GOTO(failed);
	}
	DEBUG(Source, "Source %d accepting channel %d as master.",
	      priv->id, pka_channel_get_id(channel));
	priv->channel = channel;
	g_object_add_weak_pointer(G_OBJECT(priv->channel),
	                          (gpointer *)&priv->channel);
	ret = TRUE;
  failed:
  	g_static_rw_lock_writer_unlock(&priv->rw_lock);
	RETURN(ret);
}

/**
 * pka_source_deliver_sample:
 * @source: a #PkaSource.
 * @sample: a #PkaSample.
 *
 * Delivers @sample to all observing subscriptions.
 *
 * This function implies that it becomes the new owner to @sample.  Therefore,
 * it does not increase the reference count of @sample.  If the caller intends
 * to use the sample after calling this method, it should call pka_sample_ref()
 * before-hand.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_source_deliver_sample (PkaSource *source, /* IN */
                           PkaSample *sample) /* IN */
{
	PkaSourcePrivate *priv;
	PkaSubscription *subscription;
	gint i;

	g_return_if_fail(PKA_IS_SOURCE(source));
	g_return_if_fail(sample != NULL);

	ENTRY;
	priv = source->priv;
	pka_sample_set_source_id(sample, priv->id);
	/*
	 * Notify subscribers of the incoming sample.
	 * Reader lock required to ensure subscriptions integrity.
	 */
	g_static_rw_lock_reader_lock(&priv->rw_lock);
	for (i = 0; i < priv->subscriptions->len; i++) {
		subscription = g_ptr_array_index(priv->subscriptions, i);
		pka_subscription_deliver_sample(subscription, source,
		                                priv->manifest, sample);
	}
	g_static_rw_lock_reader_unlock(&priv->rw_lock);
	EXIT;
}

/**
 * pka_source_deliver_manifest:
 * @source: A #PkaSource.
 * @manifest: A #PkaManifest.
 *
 * Delivers a manifest to all observing subscribers.
 *
 * This function expects that it is to become the new owner of @manifest.
 * Therefore, it does not increase the reference count of @manifest.  If
 * the caller intends to use the manifest after calling this method, it
 * should increase its reference count using pka_manifest_ref() before-hand.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_source_deliver_manifest (PkaSource   *source,   /* IN */
                             PkaManifest *manifest) /* IN */
{
	PkaSourcePrivate *priv;
	PkaSubscription *subscription;
	gint i;

	g_return_if_fail(PKA_IS_SOURCE(source));
	g_return_if_fail(manifest != NULL);

	ENTRY;
	priv = source->priv;
	pka_manifest_set_source_id(manifest, priv->id);
	/*
	 * Update our cached copy of the manifest.
	 * Requires write lock.
	 */
	g_static_rw_lock_writer_lock(&priv->rw_lock);
	if (priv->manifest) {
		pka_manifest_unref(priv->manifest);
	}
	priv->manifest = pka_manifest_ref(manifest);
	g_static_rw_lock_writer_unlock(&priv->rw_lock);
	/*
	 * Notify all of our subscribers of the new manifest.
	 * Requires read lock to ensure subscription integrity.  I wish that
	 * we could downgrade a write lock to a read lock, but that is not
	 * currently possible with GStaticRWLock.
	 */
	g_static_rw_lock_reader_lock(&priv->rw_lock);
	for (i = 0; i < priv->subscriptions->len; i++) {
		subscription = g_ptr_array_index(priv->subscriptions, i);
		pka_subscription_deliver_manifest(subscription, source, manifest);
	}
	g_static_rw_lock_reader_unlock(&priv->rw_lock);
	EXIT;
}

/**
 * pka_source_get_id:
 * @source: A #PkaSource.
 *
 * Retrieves the unique identifier for @source.
 *
 * Returns: the source identifier.
 * Side effects: None.
 */
gint
pka_source_get_id (PkaSource *source) /* IN */
{
	g_return_val_if_fail(PKA_IS_SOURCE(source), -1);
	return source->priv->id;
}

/**
 * pka_source_add_subscription:
 * @source: A #PkaSource.
 * @subscription: A #PkaSubscription.
 *
 * Internal method to add @subscription to the list of subscriptions
 * observing the source.  All future manifest and sample updates will
 * be dispatched to @source.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_source_add_subscription (PkaSource       *source,       /* IN */
                             PkaSubscription *subscription) /* IN */
{
	PkaSourcePrivate *priv;
	PkaManifest *manifest = NULL;

	g_return_if_fail(PKA_IS_SOURCE(source));
	g_return_if_fail(subscription != NULL);

	ENTRY;
	priv = source->priv;
	g_static_rw_lock_writer_lock(&priv->rw_lock);
	if (priv->manifest) {
		manifest = pka_manifest_ref(priv->manifest);
	}
	g_ptr_array_add(priv->subscriptions, pka_subscription_ref(subscription));
	/*
	 * If this is the first subscription and we are running, then the source
	 * is in a automatic-muted state.  We will notify it to unmute.
	 */
	if (priv->running && priv->subscriptions->len == 1) {
		pka_source_queue_unmuted(source);
	}
	g_static_rw_lock_writer_unlock(&priv->rw_lock);
	/*
	 * Ensure the current manifest is delivered.  We do this outside of the
	 * write lock to make sure that other work can happen concurrently.  It
	 * also guarantees that a new manifest cannot be delivered while we are
	 * notifying the subscription of this one since manifest delivery requires
	 * a write lock.
	 */
	if (manifest) {
		g_static_rw_lock_reader_lock(&priv->rw_lock);
		/*
		 * Check to see if manifest still matches (with memory barrier).
		 */
		if (g_atomic_pointer_get(&priv->manifest) == manifest) {
			pka_subscription_deliver_manifest(subscription, source, manifest);
		}
		pka_manifest_unref(manifest);
		g_static_rw_lock_reader_unlock(&priv->rw_lock);
	}
	EXIT;
}

/**
 * pka_source_remove_subscription:
 * @source: A #PkaSource.
 * @subscription: A #PkaSubscription.
 *
 * Internal method to remove a subscription from the list of observers of
 * @source.  The subscription will not receive future manifest and sample
 * updates from @source.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_source_remove_subscription (PkaSource       *source,       /* IN */
                                PkaSubscription *subscription) /* IN */
{
	PkaSourcePrivate *priv;

	g_return_if_fail(PKA_IS_SOURCE(source));
	g_return_if_fail(subscription != NULL);

	ENTRY;
	priv = source->priv;
	g_static_rw_lock_writer_lock(&priv->rw_lock);
	if (g_ptr_array_remove_fast(priv->subscriptions, subscription)) {
		pka_subscription_unref(subscription);
	}
	/*
	 * If we are still running and there are no more subscriptions active,
	 * then we will do our best to reduce system overhead and mute the souce.
	 */
	if (priv->running && !priv->subscriptions->len) {
		pka_source_queue_muted(source);
	}
	g_static_rw_lock_writer_unlock(&priv->rw_lock);
	EXIT;
}

/**
 * pka_source_notify_started:
 * @source: A #PkaSource.
 * @spawn_info: A #PkaSpawnInfo.
 *
 * Notifies @source that the channel has started and it should too.
 * @spawn_info is provided for the source to easily attach to
 * the process if neccessary.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_source_notify_started (PkaSource    *source,     /* IN */
                           PkaSpawnInfo *spawn_info) /* IN */
{
	PkaSourcePrivate *priv;

	g_return_if_fail(PKA_IS_SOURCE(source));
	g_return_if_fail(spawn_info != NULL);

	/*
	 * To reduce the amount of overhead that the perfkit-agent will cause
	 * when there are no subscriptions present, we will tell the source to
	 * mute if there are no active subscriptions.
	 */

	ENTRY;
	priv = source->priv;
	g_static_rw_lock_writer_lock(&priv->rw_lock);
	priv->running = TRUE;
	pka_source_queue_started(source, spawn_info);
	if (!priv->subscriptions->len) {
		pka_source_queue_muted(source);
	}
	g_static_rw_lock_writer_unlock(&priv->rw_lock);
	EXIT;
}

/**
 * pka_source_notify_stopped:
 * @source: A #PkaSource.
 *
 * Notifies @source that the channel has stopped and it should too.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_source_notify_stopped (PkaSource *source) /* IN */
{
	PkaSourcePrivate *priv;

	g_return_if_fail(PKA_IS_SOURCE(source));

	ENTRY;
	priv = source->priv;
	g_static_rw_lock_writer_lock(&priv->rw_lock);
	if (priv->running) {
		priv->running = FALSE;
		pka_source_queue_stopped(source);
	}
	g_static_rw_lock_writer_unlock(&priv->rw_lock);
	EXIT;
}

/**
 * pka_source_notify_muted:
 * @source: A #PkaSource.
 *
 * Notifies @source that the channel has muted and it should attempt, if
 * possible, to do as little work as possible (since we are muted and nobody
 * is listening).
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_source_notify_muted (PkaSource *source) /* IN */
{
	g_return_if_fail(PKA_IS_SOURCE(source));

	ENTRY;
	pka_source_queue_muted(source);
	EXIT;
}

/**
 * pka_source_notify_unmuted:
 * @source: A #PkaSource.
 *
 * Notifies @source when a previously muted channel has been unmuted and the
 * source should attempt to spin itself back up and continue sampling.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_source_notify_unmuted (PkaSource *source) /* IN */
{
	g_return_if_fail(PKA_IS_SOURCE(source));

	ENTRY;
	pka_source_queue_unmuted(source);
	EXIT;
}

/**
 * pka_source_notify_reset:
 * @source: (in): A #PkaSource.
 *
 * Notifies @source that it should reset itself for another run.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_source_notify_reset (PkaSource *source) /* IN */
{
	g_return_if_fail(PKA_IS_SOURCE(source));

	ENTRY;
	pka_source_queue_reset(source);
	EXIT;
}

/**
 * pka_source_get_plugin:
 * @source: A #PkaSource.
 *
 * Retrieves the #PkaPlugin that was used to create @source.
 *
 * Returns: A #PkaPlugin.
 * Side effects: None.
 */
PkaPlugin*
pka_source_get_plugin (PkaSource *source) /* IN */
{
	g_return_val_if_fail(PKA_IS_SOURCE(source), NULL);
	return source->priv->plugin;
}

/**
 * pka_source_set_plugin:
 * @soure: A #PkaSource.
 *
 * Internal method to set the plugin that created @source.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_source_set_plugin (PkaSource *source, /* IN */
                       PkaPlugin *plugin) /* IN */
{
	g_return_if_fail(PKA_IS_SOURCE(source));
	g_return_if_fail(source->priv->plugin == NULL);

	ENTRY;
	source->priv->plugin = g_object_ref(plugin);
	EXIT;
}

/**
 * pka_source_modify_spawn_info:
 * @source: A #PkaSource.
 *
 * Allows a source to modify the spawn information before the process is
 * spawned.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_source_modify_spawn_info (PkaSource     *source,     /* IN */
                              PkaSpawnInfo  *spawn_info, /* IN */
                              GError       **error)      /* OUT */
{
	gboolean ret = TRUE;

	g_return_val_if_fail(PKA_IS_SOURCE(source), FALSE);
	g_return_val_if_fail(spawn_info != NULL, FALSE);

	ENTRY;
	if (PKA_SOURCE_GET_CLASS(source)->modify_spawn_info) {
		ret = PKA_SOURCE_GET_CLASS(source)->
			modify_spawn_info(source, spawn_info, error);
	}
	RETURN(ret);
}

/**
 * pka_source_get_manifest:
 * @source: A #PkaSource.
 *
 * Retrieves the current manifest for @source.  If no manifest has been
 * delivered, this will return %NULL.
 *
 * Returns: A #PkaManifest which should be freed with pka_manifest_unref(),
 *   or %NULL if no manifest was delivered.
 * Side effects: None.
 */
PkaManifest*
pka_source_get_manifest (PkaSource *source) /* IN */
{
	PkaSourcePrivate *priv;
	PkaManifest *ret = NULL;

	g_return_val_if_fail(PKA_IS_SOURCE(source), NULL);

	ENTRY;
	priv = source->priv;
	g_static_rw_lock_reader_lock(&priv->rw_lock);
	if (priv->manifest) {
		ret = pka_manifest_ref(priv->manifest);
	}
	g_static_rw_lock_reader_unlock(&priv->rw_lock);
	RETURN(ret);
}

/**
 * pka_source_finalize:
 * @source: A #PkaSource.
 *
 * Finalizer for #PkaSource to free resources allocated to @source.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_source_finalize (GObject *object) /* IN */
{
	PkaSourcePrivate *priv = PKA_SOURCE(object)->priv;

	ENTRY;
	if (priv->plugin) {
		g_object_unref(priv->plugin);
	}
	if (priv->channel) {
		g_object_remove_weak_pointer(G_OBJECT(priv->channel),
		                             (gpointer *)&priv->channel);
	}
	G_OBJECT_CLASS(pka_source_parent_class)->finalize(object);
	EXIT;
}

/**
 * pka_source_dispose:
 * @object: A #PkaSource.
 *
 * Dispose callback for #PkaSource.  This method is meant to drop all
 * references to other objects so that dependent objects may also potentially
 * reach a reference count of zero.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_source_dispose (GObject *object) /* IN */
{
	ENTRY;
	G_OBJECT_CLASS(pka_source_parent_class)->dispose(object);
	EXIT;
}

/**
 * pka_source_class_init:
 * @klass: A #PkaSourceClass.
 *
 * Static constructor for #PkaSourceClass.  Initializes class vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_source_class_init (PkaSourceClass *klass) /* IN */
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pka_source_finalize;
	object_class->dispose = pka_source_dispose;
	g_type_class_add_private(object_class, sizeof(PkaSourcePrivate));

	/**
	 * PkaSource::started:
	 * @source: A #PkaSource.
	 * @spawn_info: A #PkaSpawnInfo.
	 *
	 * The "started" event is emitted when the source is started.  @spawn_info
	 * contains the information used to start the source.
	 *
	 * Returns: None.
	 * Side effects: None.
	 */
	signals[STARTED] = g_signal_new("started",
	                                PKA_TYPE_SOURCE,
	                                G_SIGNAL_RUN_FIRST,
	                                G_STRUCT_OFFSET(PkaSourceClass, started),
	                                NULL, NULL,
	                                g_cclosure_marshal_VOID__BOXED,
	                                G_TYPE_NONE,
	                                1,
	                                PKA_TYPE_SPAWN_INFO);

	/**
	 * PkaSource::stopped:
	 * @source: A #PkaSource.
	 *
	 * The "stopped" signal is emitted when the source is stopped.
	 *
	 * Returns: None.
	 * Side effects: None.
	 */
	signals[STOPPED] = g_signal_new("stopped",
	                                PKA_TYPE_SOURCE,
	                                G_SIGNAL_RUN_FIRST,
	                                G_STRUCT_OFFSET(PkaSourceClass, stopped),
	                                NULL, NULL,
	                                g_cclosure_marshal_VOID__VOID,
	                                G_TYPE_NONE,
	                                0);

	/**
	 * PkaSource::muted:
	 * @source: A #PkaSource.
	 *
	 * The "muted" signal is emitted when the source is muted.  Samples will
	 * be dropped while the source is muted.
	 *
	 * Returns: None.
	 * Side effects: None.
	 */
	signals[MUTED] = g_signal_new("muted",
	                              PKA_TYPE_SOURCE,
	                              G_SIGNAL_RUN_FIRST,
	                              G_STRUCT_OFFSET(PkaSourceClass, muted),
	                              NULL, NULL,
	                              g_cclosure_marshal_VOID__VOID,
	                              G_TYPE_NONE,
	                              0);

	/**
	 * PkaSource::unmuted:
	 * @source: A #PkaSource.
	 *
	 * The "unmuted" signal is emitted when the source is unmuted.  Samples
	 * will no longer be dropped and delivered to the subscribers.
	 *
	 * Returns: None.
	 * Side effects: None.
	 */
	signals[UNMUTED] = g_signal_new("unmuted",
	                                PKA_TYPE_SOURCE,
	                                G_SIGNAL_RUN_FIRST,
	                                G_STRUCT_OFFSET(PkaSourceClass, unmuted),
	                                NULL, NULL,
	                                g_cclosure_marshal_VOID__VOID,
	                                G_TYPE_NONE,
	                                0);

	signals[RESET] = g_signal_new("reset",
	                              PKA_TYPE_SOURCE,
	                              G_SIGNAL_RUN_FIRST,
	                              G_STRUCT_OFFSET(PkaSourceClass, reset),
	                              NULL,
	                              NULL,
	                              g_cclosure_marshal_VOID__VOID,
	                              G_TYPE_NONE,
	                              0);
}

/**
 * pka_source_init:
 * @source: A #PkaSource.
 *
 * Instance initializer for #PkaSource.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_source_init (PkaSource *source) /* IN */
{
	static gint id_seq = 0;

	source->priv = G_TYPE_INSTANCE_GET_PRIVATE(source,
	                                           PKA_TYPE_SOURCE,
	                                           PkaSourcePrivate);
	source->priv->id = g_atomic_int_exchange_and_add(&id_seq, 1);
	g_static_rw_lock_init(&source->priv->rw_lock);
	/*
	 * TODO:  We should consider doing a bit array for the list of which
	 *   subscribers are listening to the source.  This would allow us to
	 *   have a single array (of say 64 subscribers) and simply dispatch
	 *   to the subscribers whose bit-field is set in a 64-bit bitmap.
	 */
	source->priv->subscriptions = g_ptr_array_new();
}
