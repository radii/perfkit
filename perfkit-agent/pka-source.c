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
 * @short_description: 
 *
 * 
 */

G_DEFINE_ABSTRACT_TYPE (PkaSource, pka_source, G_TYPE_OBJECT)

extern void pka_sample_set_source_id   (PkaSample   *sample,
                                        gint         source_id);
extern void pka_manifest_set_source_id (PkaManifest *manifest,
                                        gint         source_id);

struct _PkaSourcePrivate
{
	GStaticRWLock  rw_lock;

	gint           id;
	PkaPlugin     *plugin;
	PkaManifest   *manifest;
	GPtrArray     *subscriptions;

	// XXX: DECPRECATED
	GMutex        *mutex;
	PkaChannel    *channel;
};

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
	g_mutex_lock(priv->mutex);
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
	g_mutex_unlock(priv->mutex);
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
		if (priv->manifest == manifest) {
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

	/*
	 * TODO: It would be a good idea if we can stop the source from doing
	 *   any significant work when it knows that nobody is listening and the
	 *   data would simply be dropped.
	 */

	ENTRY;
	priv = source->priv;
	g_static_rw_lock_writer_lock(&priv->rw_lock);
	if (g_ptr_array_remove_fast(priv->subscriptions, subscription)) {
		g_object_unref(subscription);
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
	g_return_if_fail(PKA_IS_SOURCE(source));
	g_return_if_fail(spawn_info != NULL);

	ENTRY;
	if (PKA_SOURCE_GET_CLASS(source)->notify_started) {
		PKA_SOURCE_GET_CLASS(source)->notify_started(source, spawn_info);
	}
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
	g_return_if_fail(PKA_IS_SOURCE(source));

	ENTRY;
	if (PKA_SOURCE_GET_CLASS(source)->notify_stopped) {
		PKA_SOURCE_GET_CLASS(source)->notify_stopped(source);
	}
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
	if (PKA_SOURCE_GET_CLASS(source)->notify_muted) {
		PKA_SOURCE_GET_CLASS(source)->notify_muted(source);
	}
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
	if (PKA_SOURCE_GET_CLASS(source)->notify_unmuted) {
		PKA_SOURCE_GET_CLASS(source)->notify_unmuted(source);
	}
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
	source->priv->mutex = g_mutex_new();
	g_static_rw_lock_init(&source->priv->rw_lock);
	/*
	 * TODO:  We should consider doing a bit array for the list of which
	 *   subscribers are listening to the source.  This would allow us to
	 *   have a single array (of say 64 subscribers) and simply dispatch
	 *   to the subscribers whose bit-field is set in a 64-bit bitmap.
	 */
	source->priv->subscriptions = g_ptr_array_new();
}
