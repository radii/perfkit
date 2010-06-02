/* pka-subscription.c
 *
 * Copyright (C) 2009-2010 Christian Hergert <chris@dronelabs.com>
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

#include <string.h>

#include "pka-marshal.h"
#include "pka-subscription.h"
#include "pka-log.h"

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "Subscription"

#define G_MSEC_PER_SEC (1000)

/**
 * SECTION:pka-subscription
 * @title: PkaSubscription
 * @short_description: Subscriptions to data streams
 *
 * #PkaSubscription encapsulates the work required to transfer data
 * streams between the agent and clients.  A subscription consists
 * of a #PkaChannel to be observed and various buffering settings.
 *
 * The subscription receives events about data format changes and
 * the data itself from the #PkaChannel it is subscribed to.  Information
 * about the data stream is delivered as a #PkaManifest.  The samples
 * are received as #PkaSample<!-- -->'s.  Whenever the format of the sample
 * stream is changed, a new #PkaManifest is delivered prior to the new
 * samples being delivered.
 *
 * The subscriptions internal #PkaEncoder converts the #PkaManifest<!-- -->'s
 * and #PkaSample<!-- -->'s into buffers which can be transported to the
 * client.  Various encoders can provide extra features like compression,
 * encryption, or similar.
 */

#define SWAP_MANIFEST(s,m) G_STMT_START {  \
    if ((s)->manifest)                     \
        pka_manifest_unref((s)->manifest); \
    (s)->manifest = pka_manifest_ref((m)); \
} G_STMT_END

extern void pka_channel_add_subscription    (PkaChannel      *channel,
                                             PkaSubscription *sub);
extern void pka_channel_remove_subscription (PkaChannel      *channel,
                                             PkaSubscription *sub);
static void pka_subscription_flush_locked   (PkaSubscription *sub);

struct _PkaSubscription
{
	guint            id;
	volatile gint    ref_count;

	PkaChannel      *channel;        /* Our producing channel */
	PkaEncoder      *encoder;        /* Sample/Manifest encoder */
	GClosure        *manifest_cb;    /* Manifest callback */
	GClosure        *sample_cb;      /* Sample callback */
	gsize            bufsize;        /* Total buffer size */
	glong            timeout;        /* Buffering timeout in Milliseconds */

	GMutex          *mutex;          /* Synchronization mutex */
	GQueue          *queue;          /* Queue for delivering samples */
	gboolean         muted;          /* Subscription is muted (default) */
	gsize            buflen;         /* Current buffer length */
	PkaManifest     *manifest;       /* Our current manifest */
	gboolean         finished;       /* Subscription has completed */
};

#if 0
static guint subscription_seq = 0;
#endif

static void
pka_subscription_destroy (PkaSubscription *sub)
{
	if (sub->channel) {
		pka_channel_remove_subscription(sub->channel, sub);
		g_object_unref(sub->channel);
	}
	if (sub->encoder) {
		g_object_unref(sub->encoder);
	}
	g_queue_free(sub->queue);
	g_mutex_free(sub->mutex);

	memset(sub, 0, sizeof(*sub));
}

GType
pka_subscription_get_type (void)
{
	static gsize initialized = FALSE;
	static GType type_id = 0;

	if (g_once_init_enter(&initialized)) {
		type_id = g_boxed_type_register_static("PkaSubscription",
		                                       (GBoxedCopyFunc)pka_subscription_ref,
		                                       (GBoxedFreeFunc)pka_subscription_unref);
		g_once_init_leave(&initialized, TRUE);
	}

	return type_id;
}

#if 0
static gboolean
pka_subscription_timeout_cb (PkaSubscription *sub)
{
	/*
	 * If we finished, stop the func and drop our reference.
	 */
	if (sub->finished) {
		pka_subscription_unref(sub);
		return FALSE;
	}

	/*
	 * Flush the subscriptions buffers.
	 */
	g_mutex_lock(sub->mutex);
	pka_subscription_flush_locked(sub);
	g_mutex_unlock(sub->mutex);

	return TRUE;
}
#endif

/**
 * pka_subscription_new:
 * @encoder: A #PkaEncoder or %NULL.
 * @buffer_max: The max buffer size before data is flushed, or 0 for
 *    no size based buffer.
 * @buffer_timeout: The number of milliseconds to buffer before the data
 *    is flushed.  0 for no time based buffer.
 * @manifest_func: Callback when source manifests change.
 * @manifest_data: Data for @manifest_func.
 * @sample_func: Callback for incoming samples.
 * @sample_data: Data for @sample_func.
 *
 * Creates a new instance of #PkaSubscription.
 *
 * Returns: The newly created #PkaSubscription.  The structure should be freed
 *   with pka_subscription_unref().
 *
 * Side effects: None.
 */
#if 0
PkaSubscription*
pka_subscription_new (PkaChannel      *channel,
                      PkaEncoderInfo  *encoder_info,
                      gsize            buffer_max,
                      gulong           buffer_timeout,
                      PkaManifestFunc  manifest_func,
                      gpointer         manifest_data,
                      PkaSampleFunc    sample_func,
                      gpointer         sample_data)
{
	PkaSubscription *sub;
	PkaEncoder *encoder = NULL;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), NULL);
	g_return_val_if_fail(!encoder_info||PKA_IS_ENCODER_INFO(encoder_info), NULL);
	g_return_val_if_fail(sample_func != NULL, NULL);
	g_return_val_if_fail(manifest_func != NULL, NULL);

	/*
	 * Create an instance of the desired encoder if necessary.
	 */
	if (encoder_info) {
		encoder = pka_encoder_info_create(encoder_info);
		//pka_pipeline_add_encoder(encoder);
	}

	/*
	 * Setup the subscription values.
	 */
	sub = g_slice_new0(PkaSubscription);
	sub->ref_count = 1;
	sub->id = g_atomic_int_exchange_and_add((gint *)&subscription_seq, 1);
	sub->mutex = g_mutex_new();
	sub->queue = g_queue_new();
	sub->muted = TRUE;
	sub->channel = g_object_ref(channel);
	sub->encoder = encoder;
	sub->bufsize = buffer_max;
	sub->buflen = 0;
	sub->timeout = buffer_timeout;
	sub->manifest_func = manifest_func;
	sub->manifest_data = manifest_data;
	sub->sample_func = sample_func;
	sub->sample_data = sample_data;

	/*
	 * Notify the channel of the subscription.
	 */
	pka_channel_add_subscription(channel, sub);

	/*
	 * Setup flush timeout if needed.  We try to reduce the CPU wake-up
	 * overhead if the timeout is requested on a whole second.
	 */
	if (buffer_timeout > 0) {
		if ((buffer_timeout % G_MSEC_PER_SEC) == 0) {
			g_timeout_add_seconds(buffer_timeout / G_MSEC_PER_SEC,
			                      (GSourceFunc)pka_subscription_timeout_cb,
			                      pka_subscription_ref(sub));
		} else {
			g_timeout_add(buffer_timeout,
			              (GSourceFunc)pka_subscription_timeout_cb,
			              pka_subscription_ref(sub));
		}
	}

	return sub;
}
#endif

PkaSubscription*
pka_subscription_new (void)
{
	static gint id_seq = 0;
	PkaSubscription *sub;

	ENTRY;
	sub = g_slice_new0(PkaSubscription);
	sub->id = g_atomic_int_exchange_and_add(&id_seq, 1);
	sub->muted = TRUE;
	sub->queue = g_queue_new();
	sub->mutex = g_mutex_new();
	sub->ref_count = 1;
	RETURN(sub);
}

/**
 * pka_subscription_ref:
 * @subscription: A #PkaSubscription
 *
 * Atomically increments the reference count of @subscription by one.
 *
 * Returns: the @subscription pointer.
 *
 * Side effects: None.
 */
PkaSubscription*
pka_subscription_ref (PkaSubscription *subscription)
{
	g_return_val_if_fail(subscription != NULL, NULL);
	g_return_val_if_fail(subscription->ref_count > 0, NULL);

	g_atomic_int_inc(&subscription->ref_count);

	return subscription;
}

/**
 * pka_subscription_unref:
 * @subscription: A #PkaSubscription
 *
 * Atomically decrements the reference count of @subscription by one.  When
 * the reference count reaches zero, the resources are released and the
 * structure is freed.
 *
 * Side effects: Resources released and structure freed when ref count
 *   reaches zero.
 */
void
pka_subscription_unref (PkaSubscription *subscription)
{
	g_return_if_fail(subscription != NULL);
	g_return_if_fail(subscription->ref_count > 0);

	if (g_atomic_int_dec_and_test(&subscription->ref_count)) {
		pka_subscription_destroy(subscription);
	}
}

/**
 * pka_subscription_get_id:
 * @subscription: A #PkaSubscription
 *
 * Retrieves the unique subscription id.
 *
 * Returns: The subscription id.
 *
 * Side effects: None.
 */
guint
pka_subscription_get_id (PkaSubscription *subscription)
{
	g_return_val_if_fail(subscription != NULL, 0);
	return subscription->id;
}

/**
 * pka_subscription_needs_flush_locked:
 * @sub: A #PkaSubscription.
 *
 * Determines if the subscrtipion needs to be flushed immediately.
 *
 * Returns: %TRUE if the subscription needs to be flushed.
 *
 * Side effects: None.
 */
static inline gboolean
pka_subscription_needs_flush_locked (PkaSubscription *sub)
{
	/*
	 * We don't need to flush if we have a time-based buffer and no size
	 * based buffer.
	 */
	if (sub->timeout && !sub->bufsize) {
		return FALSE;
	}

	/*
	 * If there is nothing in the queue, prevent costly flush path.
	 */
	if (!g_queue_get_length(sub->queue)) {
		return FALSE;
	}

	/*
	 * If we are not buffering, we need to flush immediately.
	 */
	if (!sub->bufsize) {
		return TRUE;
	}

	return (sub->buflen >= sub->bufsize);
}

/**
 * pka_subscription_flush_locked:
 * @sub: A #PkaSubscription
 *
 * Flushes the queue.  Encodes the samples and ships them off to the buffer
 * callback.
 *
 * The caller should own the subscription lock.
 */
static void
pka_subscription_flush_locked (PkaSubscription *sub)
{
	PkaEncoder *encoder = sub->encoder;
	GValue params[3] = { { 0 } };
	guint8 *buf = NULL;
	gsize len = 0;
	gsize n_samples = 0;
	PkaSample **samples;
	gint i;

	ENTRY;

	/*
	 * Get the samples from the queue.
	 */
	n_samples = g_queue_get_length(sub->queue);
	samples = g_malloc(sizeof(PkaSample*) * n_samples);
	for (i = 0; i < n_samples; i++) {
		samples[i] = g_queue_pop_head(sub->queue);
	}
	sub->buflen = 0;

	/*
	 * Encode the buffer.
	 */
	if (!pka_encoder_encode_samples(encoder, sub->manifest, samples,
	                                n_samples, &buf, &len)) {
		ERROR(Encoder, "An error has occurred while encoding a buffer.  The "
	                   "encoder type is: %s.",
		      encoder ? g_type_name(G_TYPE_FROM_INSTANCE(encoder)) : "NONE");
	}

	DUMP_BYTES(Sample, buf, len);

	/*
	 * Notify the handler via their configured callback.
	 */
	if (G_LIKELY(sub->sample_cb)) {
		g_value_init(&params[0], PKA_TYPE_SUBSCRIPTION);
		g_value_init(&params[1], G_TYPE_POINTER);
		g_value_init(&params[2], G_TYPE_ULONG);
		g_value_set_boxed(&params[0], sub);
		g_value_set_pointer(&params[1], buf);
		g_value_set_ulong(&params[2], len);
		g_closure_invoke(sub->sample_cb, NULL, 3, &params[0], NULL);
		g_value_unset(&params[0]);
		g_value_unset(&params[1]);
		g_value_unset(&params[2]);
	}

	g_free(buf);
	EXIT;
}

/**
 * pka_subscription_deliver_sample:
 * @subscription: A #PkaSubscription
 * @sample: A #PkaSample
 *
 * Delivers a sample to the subscription.  If the subscription if configured
 * to buffer, the data will be stored until the condition is reached.
 */
void
pka_subscription_deliver_sample (PkaSubscription *subscription,
                                 PkaSample       *sample)
{
	const guint8 *buf = NULL;
	gsize buflen = 0;

	g_return_if_fail(subscription != NULL);
	g_return_if_fail(sample != NULL);

	/*
	 * NOTES:
	 *
	 *   This is going to be a critical path as the system grows.  For the
	 *   time being, we will just protect the thing with a mutex.
	 *   However, we should look at doing liburcu or something else in the
	 *   future.
	 *
	 *   Upon further investigation, liburcu probably isn't what we would
	 *   want since it is for read heavy situations (like a routing table).
	 *   What we need is a lightweight queue.  Where multiple writers can
	 *   deliver fast.  Flushing will typically be done by a single consumer
	 *   at a time.  Or at minimum we can use the mutex for consumers.
	 *
	 */

	g_mutex_lock(subscription->mutex);

	if (subscription->muted) {
		/*
		 * We are muted so we can silently drop the sample.
		 */
		goto unlock;
	}

	pka_sample_get_data(sample, &buf, &buflen);
	g_assert(buflen);

	/*
	 * We are unmuted.
	 *
	 *   1) Push the item onto the queue.
	 *   2) Update the new buffered size.
	 *   2) Flush the buffer if needed.
	 *
	 */
	g_queue_push_tail(subscription->queue, pka_sample_ref(sample));
	subscription->buflen += buflen;
	if (pka_subscription_needs_flush_locked(subscription)) {
		pka_subscription_flush_locked(subscription);
	}

unlock:
	g_mutex_unlock(subscription->mutex);
}

/**
 * pka_subscription_deliver_manifest_locked:
 * @subscription: A #PkaSubscription
 * @manifest: A #PkaManifest
 *
 * Delivers a manifest to the subscription.  If the subscription if configured
 * to buffer, the data will be stored until the condition is reached.
 *
 * The mutex should be held when calling this method.
 */
static void
pka_subscription_deliver_manifest_locked (PkaSubscription *subscription,
                                          PkaManifest     *manifest)
{
	PkaEncoder *encoder;
	GValue params[3] = { { 0 } };
	guint8 *buf = NULL;
	gsize buflen = 0;

	ENTRY;

	/*
	 * NOTES:
	 *
	 *   Whenever a new manifest comes in we flush the current buffer.  This
	 *   may not be necessary, as we should be assigning a monotonic id to
	 *   both the manifest and the sample.
	 *
	 */

	/*
	 * We can enter this in either one of two states:
	 *
	 * 1) We are muted.  No samples in queue.  No flushing required.  Simply
	 *    store the manifest for when we are unmuted and it will get flushed.
	 * 2) We are unmuted.  Samples may be in queue.  Samples must be flushed
	 *    first.  Then we can store, then send our manifest.
	 *
	 */

	if (subscription->muted) {
		/*
		 * State 1:
		 *
		 * The queue should be empty.  Release the previous manifest and
		 * store the new one for when we are unmuted and flushing occurs.
		 */
		g_assert_cmpint(g_queue_get_length(subscription->queue), ==, 0);
		/*
		 * TODO: This needs to handle multiple sources.
		 */
		SWAP_MANIFEST(subscription, manifest);
		EXIT;
	}

	/*
	 * State 2:
	 *
	 * Flush queue if neccesary.  Store, then send manifest.
	 */
	if (g_queue_get_length(subscription->queue)) {
		pka_subscription_flush_locked(subscription);
	}

	SWAP_MANIFEST(subscription, manifest);

	/*
	 * Encode the manifest stream into a buffer for delivery to a listener.
	 */
	encoder = subscription->encoder;
	if (!pka_encoder_encode_manifest(encoder, manifest, &buf, &buflen)) {
		/*
		 * XXX: Fatal error: cannot encode the data stream.
		 *
		 *   This should simply kill the subscription in case of failure.
		 *
		 */
	    ERROR(Encoder, "An error has occurred while encoding a buffer.  The "
		               "encoder type is: %s.",
		      encoder ? g_type_name(G_TYPE_FROM_INSTANCE(encoder)) : "NONE");
	}

	DUMP_BYTES(Manifest, buf, buflen);

	/*
	 * Call the handler via their configured callback.
	 */
	if (G_LIKELY(subscription->manifest_cb)) {
		g_value_init(&params[0], PKA_TYPE_SUBSCRIPTION);
		g_value_init(&params[1], G_TYPE_POINTER);
		g_value_init(&params[2], G_TYPE_ULONG);
		g_value_set_boxed(&params[0], subscription);
		g_value_set_pointer(&params[1], buf);
		g_value_set_ulong(&params[2], buflen);
		g_closure_invoke(subscription->manifest_cb, NULL, 3, &params[0], NULL);
		g_value_unset(&params[0]);
		g_value_unset(&params[1]);
		g_value_unset(&params[2]);
	}

	g_free(buf);
	EXIT;
}

/**
 * pka_subscription_deliver_manifest:
 * @subscription: A #PkaSubscription
 * @manifest: A #PkaManifest
 *
 * Delivers a manifest to the subscription.  If the subscription if configured
 * to buffer, the data will be stored until the condition is reached.
 */
void
pka_subscription_deliver_manifest (PkaSubscription *subscription,
                                   PkaManifest     *manifest)
{
	g_return_if_fail(subscription != NULL);
	g_return_if_fail(manifest != NULL);

	g_mutex_lock(subscription->mutex);
	pka_subscription_deliver_manifest_locked(subscription, manifest);
	g_mutex_unlock(subscription->mutex);
}

/**
 * pka_subscription_unmute:
 * @subscription: A #PkaSubscription
 *
 * Enables a subscription to begin delivering manifest updates and samples.
 * Subscriptions are started in a muted state so this should be called
 * when the client is ready to receive the information stream.
 *
 * Side effects:
 *   The current manifest is delivered.
 *   Samples are allowed to be delivered.
 */
void
pka_subscription_unmute (PkaSubscription *subscription)
{
	g_return_if_fail(subscription != NULL);

	g_mutex_lock(subscription->mutex);

	/*
	 * Mark the subscription as unmuted.
	 */
	subscription->muted = FALSE;

	/*
	 * Send our current manifest to the client.
	 */
	if (subscription->manifest) {
		pka_subscription_deliver_manifest_locked(subscription,
		                                         subscription->manifest);
	}

	g_mutex_unlock(subscription->mutex);
}

/**
 * pka_subscription_mute:
 * @subscription: A #PkaSubscription.
 * @drain: If the current buffer should be drained before disabling.
 *
 * Disables the client receiving the subscription stream from receiving future
 * manifest and sample events until pka_subscription_unmute() is called.  If
 * drain is set, the current buffer will be flushed to the client.
 *
 * Side effects:
 *   Future samples and manifest updates will be dropped.
 */
void
pka_subscription_mute (PkaSubscription *subscription,
                       gboolean         drain)
{
	g_return_if_fail(subscription != NULL);

	g_mutex_lock(subscription->mutex);
	subscription->muted = TRUE;
	if (drain) {
		pka_subscription_flush_locked(subscription);
	}
	g_mutex_unlock(subscription->mutex);
}

/**
 * pka_subscription_set_handlers:
 * @subscription: A #PkaSubscription.
 *
 * Sets the manifest and sample callback functions for the subscription.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_subscription_set_handlers (PkaSubscription *subscription,     /* IN */
                               PkaManifestFunc  manifest_func,    /* IN */
                               gpointer         manifest_data,    /* IN */
                               GDestroyNotify   manifest_destroy, /* IN */
                               PkaSampleFunc    sample_func,      /* IN */
                               gpointer         sample_data,      /* IN */
                               GDestroyNotify   sample_destroy)   /* IN */
{
	g_return_if_fail(subscription != NULL);
	g_return_if_fail(manifest_func != NULL);
	g_return_if_fail(sample_func != NULL);

	ENTRY;
	g_mutex_lock(subscription->mutex);
	if (subscription->manifest_cb) {
		g_closure_unref(subscription->manifest_cb);
	}
	if (subscription->sample_cb) {
		g_closure_unref(subscription->sample_cb);
	}
	subscription->manifest_cb = g_cclosure_new(G_CALLBACK(manifest_func),
	                                           manifest_data,
	                                           (GClosureNotify)manifest_destroy);
	subscription->sample_cb = g_cclosure_new(G_CALLBACK(sample_func),
	                                         sample_data,
	                                         (GClosureNotify)sample_destroy);
	g_closure_set_marshal(subscription->manifest_cb,
	                      pka_marshal_VOID__POINTER_ULONG);
	g_closure_set_marshal(subscription->sample_cb,
	                      pka_marshal_VOID__POINTER_ULONG);
	g_mutex_unlock(subscription->mutex);
	EXIT;
}
