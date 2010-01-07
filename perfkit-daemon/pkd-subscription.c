/* pkd-subscription.c
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

#include <string.h>

#include "pkd-pipeline.h"
#include "pkd-subscription.h"

/**
 * SECTION:pkd-subscription
 * @title: PkdSubscription
 * @short_description: Subscriptions to data streams
 *
 * #PkdSubscription encapsulates the work required to transfer data
 * streams between the daemon and clients.  A subscription consists
 * of a #PkdChannel to be observed and various buffering settings.
 *
 * The subscription receives events about data format changes and
 * the data itself from the #PkdChannel it is subscribed to.  Information
 * about the data stream is delivered as a #PkdManifest.  The samples
 * are received as #PkdSample<!-- -->'s.  Whenever the format of the sample
 * stream is changed, a new #PkdManifest is delivered prior to the new
 * samples being delivered.
 *
 * The subscriptions internal #PkdEncoder converts the #PkdManifest<!-- -->'s
 * and #PkdSample<!-- -->'s into buffers which can be transported to the
 * client.  Various encoders can provide extra features like compression,
 * encryption, or similar.
 */

#define SWAP_MANIFEST(s,m) G_STMT_START {  \
    if ((s)->manifest)                     \
        pkd_manifest_unref((s)->manifest); \
    (s)->manifest = pkd_manifest_ref((m)); \
} G_STMT_END

/*
 * Internal callbacks to notify channels of subscription.
 */
extern void pkd_channel_add_subscription    (PkdChannel      *channel,
                                             PkdSubscription *sub);
extern void pkd_channel_remove_subscription (PkdChannel      *channel,
                                             PkdSubscription *sub);

struct _PkdSubscription
{
	guint            sub_id;
	volatile gint    ref_count;

	PkdChannel      *channel;        /* Our producing channel */
	PkdEncoder      *encoder;        /* Sample/Manifest encoder */
	PkdManifestFunc  manifest_func;  /* Manifest callback */
	gpointer         manifest_data;  /* Manifest calback data */
	PkdSampleFunc    sample_func;    /* Sample callback */
	gpointer         sample_data;    /* Sample callback data */
	gsize            bufsize;        /* Total buffer size */
	glong            timeout;        /* Buffering timeout in Milliseconds */

	GMutex          *mutex;          /* Synchronization mutex */
	GQueue          *queue;          /* Queue for delivering samples */
	gboolean         disabled;       /* Subscription is disabled (default) */
	gsize            buflen;         /* Current buffer length */
	PkdManifest     *manifest;       /* Our current manifest */
};

static guint subscription_seq = 0;

static void
pkd_subscription_destroy (PkdSubscription *sub)
{
	pkd_channel_remove_subscription(sub->channel, sub);

	g_mutex_free(sub->mutex);
	g_queue_free(sub->queue);
	g_object_unref(sub->channel);
	if (sub->encoder) {
		g_object_unref(sub->encoder);
	}

	memset(sub, 0, sizeof(*sub));
}

/**
 * pkd_subscription_new:
 * @encoder: A #PkdEncoder or %NULL.
 * @buffer_max: The max buffer size before data is flushed, or 0 for
 *    no size based buffer.
 * @buffer_timeout: The number of milliseconds to buffer before the data
 *    is flushed.  0 for no time based buffer.
 * @manifest_func: Callback when source manifests change.
 * @manifest_data: Data for @manifest_func.
 * @sample_func: Callback for incoming samples.
 * @sample_data: Data for @sample_func.
 *
 * Creates a new instance of #PkdSubscription.
 *
 * Returns: The newly created #PkdSubscription.  The structure should be freed
 *   with pkd_subscription_unref().
 *
 * Side effects: None.
 */
PkdSubscription*
pkd_subscription_new (PkdChannel      *channel,
                      PkdEncoderInfo  *encoder_info,
                      gsize            buffer_max,
                      glong            buffer_timeout,
                      PkdManifestFunc  manifest_func,
                      gpointer         manifest_data,
                      PkdSampleFunc    sample_func,
                      gpointer         sample_data)
{
	PkdSubscription *sub;
	PkdEncoder *encoder = NULL;

	g_return_val_if_fail(PKD_IS_CHANNEL(channel), NULL);
	g_return_val_if_fail(!encoder_info||PKD_IS_ENCODER_INFO(encoder_info), NULL);
	g_return_val_if_fail(sample_func != NULL, NULL);
	g_return_val_if_fail(manifest_func != NULL, NULL);

	/*
	 * Create an instance of the desired encoder if necessary.
	 */
	if (encoder_info) {
		encoder = pkd_encoder_info_create(encoder_info);
		pkd_pipeline_add_encoder(encoder);
	}

	/*
	 * Setup the subscription values.
	 */
	sub = g_slice_new0(PkdSubscription);
	sub->ref_count = 1;
	sub->sub_id = g_atomic_int_exchange_and_add((gint *)&subscription_seq, 1);
	sub->mutex = g_mutex_new();
	sub->queue = g_queue_new();
	sub->disabled = TRUE;
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
	pkd_channel_add_subscription(channel, sub);

	return sub;
}

/**
 * pkd_subscription_ref:
 * @subscription: A #PkdSubscription
 *
 * Atomically increments the reference count of @subscription by one.
 *
 * Returns: the @subscription pointer.
 *
 * Side effects: None.
 */
PkdSubscription*
pkd_subscription_ref (PkdSubscription *subscription)
{
	g_return_val_if_fail(subscription != NULL, NULL);
	g_return_val_if_fail(subscription->ref_count > 0, NULL);

	g_atomic_int_inc(&subscription->ref_count);

	return subscription;
}

/**
 * pkd_subscription_unref:
 * @subscription: A #PkdSubscription
 *
 * Atomically decrements the reference count of @subscription by one.  When
 * the reference count reaches zero, the resources are released and the
 * structure is freed.
 *
 * Side effects: Resources released and structure freed when ref count
 *   reaches zero.
 */
void
pkd_subscription_unref (PkdSubscription *subscription)
{
	g_return_if_fail(subscription != NULL);
	g_return_if_fail(subscription->ref_count > 0);

	if (g_atomic_int_dec_and_test(&subscription->ref_count)) {
		pkd_subscription_destroy(subscription);
	}
}

/**
 * pkd_subscription_get_id:
 * @subscription: A #PkdSubscription
 *
 * Retrieves the unique subscription id.
 *
 * Returns: The subscription id.
 *
 * Side effects: None.
 */
guint
pkd_subscription_get_id (PkdSubscription *subscription)
{
	return subscription->sub_id;
}

/**
 * pkd_subscription_needs_flush_locked:
 * @sub: A #PkdSubscription.
 *
 * Determines if the subscrtipion needs to be flushed immediately.
 *
 * Returns: %TRUE if the subscription needs to be flushed.
 *
 * Side effects: None.
 */
static inline gboolean
pkd_subscription_needs_flush_locked (PkdSubscription *sub)
{
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
 * pkd_subscription_flush_locked:
 * @sub: A #PkdSubscription
 *
 * Flushes the queue.  Encodes the samples and ships them off to the buffer
 * callback.
 *
 * The caller should own the subscription lock.
 */
static void
pkd_subscription_flush_locked (PkdSubscription *sub)
{
	PkdEncoder *encoder = sub->encoder;
	gchar *buf = NULL;
	gsize len = 0, n_samples = 0;
	PkdSample **samples;
	gint i;

	/*
	 * Get the samples from the queue.
	 */
	n_samples = g_queue_get_length(sub->queue);
	samples = g_malloc(sizeof(PkdSample*) * n_samples);
	for (i = 0; i < n_samples; i++) {
		samples[i] = g_queue_pop_head(sub->queue);
	}

	/*
	 * Encode the sample buffer.
	 */
	if (!pkd_encoder_encode_samples(encoder, sub->manifest, samples,
	                                n_samples, &buf, &len))
	{
	    g_error("%s: An error has occurred while encoding a buffer.  The "
	            "encoder type is: %s.",
	            G_STRLOC,
	            encoder ? g_type_name(G_TYPE_FROM_INSTANCE(encoder)) : "NONE");
	}

	/*
	 * Ship the buffer off to the listener.
	 */
	sub->sample_func(buf, len, sub->sample_data);

	/*
	 * We are done with the buffer.
	 */
	g_free(buf);
	sub->buflen = 0;
}

/**
 * pkd_subscription_deliver_sample:
 * @subscription: A #PkdSubscription
 * @sample: A #PkdSample
 *
 * Delivers a sample to the subscription.  If the subscription if configured
 * to buffer, the data will be stored until the condition is reached.
 */
void
pkd_subscription_deliver_sample (PkdSubscription *subscription,
                                 PkdSample       *sample)
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

	if (subscription->disabled) {
		/*
		 * We are disabled so we can silently drop the sample.
		 */
		goto unlock;
	}

	pkd_sample_get_data(sample, &buf, &buflen);
	g_assert(buflen);

	/*
	 * We are enabled.
	 *
	 *   1) Push the item onto the queue.
	 *   2) Update the new buffered size.
	 *   2) Flush the buffer if needed.
	 *
	 */
	g_queue_push_tail(subscription->queue, pkd_sample_ref(sample));
	subscription->buflen += buflen;
	if (pkd_subscription_needs_flush_locked(subscription)) {
		pkd_subscription_flush_locked(subscription);
	}

unlock:
	g_mutex_unlock(subscription->mutex);
}

/**
 * pkd_subscription_deliver_manifest_locked:
 * @subscription: A #PkdSubscription
 * @manifest: A #PkdManifest
 *
 * Delivers a manifest to the subscription.  If the subscription if configured
 * to buffer, the data will be stored until the condition is reached.
 *
 * The mutex should be held when calling this method.
 */
static void
pkd_subscription_deliver_manifest_locked (PkdSubscription *subscription,
                                          PkdManifest     *manifest)
{
	PkdEncoder *encoder;
	gchar *buf = NULL;
	gsize buflen = 0;

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
	 * 1) We are disabled.  No samples in queue.  No flushing required.  Simply
	 *    store the manifest for when we are enabled and it will get flushed.
	 * 2) We are enabled.  Samples may be in queue.  Samples must be flushed
	 *    first.  Then we can store, then send our manifest.
	 *
	 */

	if (subscription->disabled) {
		/*
		 * State 1:
		 *
		 * The queue should be empty.  Release the previous manifest and
		 * store the new one for when we are enabled and flushing occurs.
		 */
		g_assert_cmpint(g_queue_get_length(subscription->queue), ==, 0);
		SWAP_MANIFEST(subscription, manifest);
		return;
	}

	/*
	 * State 2:
	 *
	 * Flush queue if neccesary.  Store, then send manifest.
	 */
	if (g_queue_get_length(subscription->queue)) {
		pkd_subscription_flush_locked(subscription);
	}

	SWAP_MANIFEST(subscription, manifest);

	/*
	 * Encode the manifest stream into a buffer for delivery to a listener.
	 */
	encoder = subscription->encoder;
	if (!pkd_encoder_encode_manifest(encoder, manifest, &buf, &buflen)) {
		/*
		 * Fatal error: cannot encode the data stream.
		 */
	    g_error("%s: An error has occurred while encoding a buffer.  The "
	            "encoder type is: %s.",
	            G_STRLOC,
	            encoder ? g_type_name(G_TYPE_FROM_INSTANCE(encoder)) : "NONE");
	}

	/*
	 * Call the listener back via their configured callback.
	 */
	subscription->manifest_func(buf, buflen, subscription->manifest_data);

	/*
	 * We are finished with the buffer.
	 */
	g_free(buf);
}

/**
 * pkd_subscription_deliver_manifest:
 * @subscription: A #PkdSubscription
 * @manifest: A #PkdManifest
 *
 * Delivers a manifest to the subscription.  If the subscription if configured
 * to buffer, the data will be stored until the condition is reached.
 */
void
pkd_subscription_deliver_manifest (PkdSubscription *subscription,
                                   PkdManifest     *manifest)
{
	g_return_if_fail(subscription != NULL);
	g_return_if_fail(manifest != NULL);

	g_mutex_lock(subscription->mutex);
	pkd_subscription_deliver_manifest_locked(subscription, manifest);
	g_mutex_unlock(subscription->mutex);
}

/**
 * pkd_subscription_enable:
 * @subscription: A #PkdSubscription
 *
 * Enables a subscription to begin delivering manifest updates and samples.
 * Subscriptions are started in a disabled state so this should be called
 * when the client is ready to receive the information stream.
 *
 * Side effects:
 *   The current manifest is delivered.
 *   Samples are allowed to be delivered.
 */
void
pkd_subscription_enable (PkdSubscription *subscription)
{
	g_return_if_fail(subscription != NULL);

	g_mutex_lock(subscription->mutex);

	/*
	 * Mark the subscription as enabled.
	 */
	subscription->disabled = FALSE;

	/*
	 * Send our current manifest to the client.
	 */
	if (subscription->manifest) {
		pkd_subscription_deliver_manifest_locked(subscription,
		                                         subscription->manifest);
	}

	g_mutex_unlock(subscription->mutex);
}

/**
 * pkd_subscription_disable:
 * @subscription: A #PkdSubscription.
 * @drain: If the current buffer should be drained before disabling.
 *
 * Disables the client receiving the subscription stream from receiving future
 * manifest and sample events until pkd_subscription_enable() is called.  If
 * drain is set, the current buffer will be flushed to the client.
 *
 * Side effects:
 *   Future samples and manifest updates will be dropped.
 */
void
pkd_subscription_disable (PkdSubscription *subscription,
                          gboolean         drain)
{
	g_return_if_fail(subscription != NULL);

	g_mutex_lock(subscription->mutex);

	/*
	 * Mark the subscription as disabled.
	 */
	subscription->disabled = TRUE;

	/*
	 * Drain buffered samples if needed.
	 */
	if (drain) {
		pkd_subscription_flush_locked(subscription);
	}

	g_mutex_unlock(subscription->mutex);
}
