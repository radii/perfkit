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

#define G_OBJECT_UNREF(o) (o ? (g_object_unref(o), NULL) : NULL)
#define G_CLOSURE_UNREF(o) (o ? (g_closure_unref(o), NULL) : NULL)

#define G_MSEC_PER_SEC (1000)

#define STORE_MANIFEST(s, m)                                                \
    G_STMT_START {                                                          \
        gint *_k;                                                           \
        _k = g_new(gint, 1);                                                \
        *_k = pka_manifest_get_source_id((m));                              \
        TRACE(Subscription, "Storing manifest for source %d.", *_k);        \
        g_tree_insert((s)->manifests, _k, pka_manifest_ref((m)));           \
    } G_STMT_END

#define NOTIFY_MANIFEST_HANDLER(s, m)                                       \
    G_STMT_START {                                                          \
        GValue params[3] = { { 0 } };                                       \
        if (G_LIKELY((s)->manifest_cb)) {                                   \
            g_value_init(&params[0], PKA_TYPE_SUBSCRIPTION);                \
            g_value_init(&params[1], G_TYPE_POINTER);                       \
            g_value_init(&params[2], G_TYPE_ULONG);                         \
            g_value_set_boxed(&params[0], subscription);                    \
            g_value_set_pointer(&params[1], buf);                           \
            g_value_set_ulong(&params[2], buflen);                          \
            g_closure_invoke(subscription->manifest_cb, NULL,               \
                             3, &params[0], NULL);                          \
            g_value_unset(&params[0]);                                      \
            g_value_unset(&params[1]);                                      \
            g_value_unset(&params[2]);                                      \
        }                                                                   \
    } G_STMT_END

#define NOTIFY_SAMPLE_HANDLER(s, b, l)                                      \
    G_STMT_START {                                                          \
        if (G_LIKELY(sub->sample_cb)) {                                     \
            g_value_init(&params[0], PKA_TYPE_SUBSCRIPTION);                \
            g_value_init(&params[1], G_TYPE_POINTER);                       \
            g_value_init(&params[2], G_TYPE_ULONG);                         \
            g_value_set_boxed(&params[0], sub);                             \
            g_value_set_pointer(&params[1], buf);                           \
            g_value_set_ulong(&params[2], buflen);                          \
            g_closure_invoke(sub->sample_cb, NULL, 3, &params[0], NULL);    \
            g_value_unset(&params[0]);                                      \
            g_value_unset(&params[1]);                                      \
            g_value_unset(&params[2]);                                      \
        }                                                                   \
    } G_STMT_END

/**
 * SECTION:pka-subscription
 * @title: PkaSubscription
 * @short_description: Subscriptions to data streams
 *
 * #PkaSubscription encapsulates the work required to transfer data
 * streams between the agent and clients.  A subscription consists
 * of channels, sources.  In the future, this may support fields within
 * sources directly.
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

extern void pka_channel_add_subscription    (PkaChannel      *channel,
                                             PkaSubscription *sub);
extern void pka_channel_remove_subscription (PkaChannel      *channel,
                                             PkaSubscription *sub);
static void pka_subscription_flush_locked   (PkaSubscription *sub);

struct _PkaSubscription
{
	volatile gint    ref_count;
	guint            id;             /* Monotonic subscription id */
	GMutex          *mutex;          /* Synchronization mutex */
	PkaEncoder      *encoder;        /* Sample/Manifest encoder */
	GClosure        *manifest_cb;    /* Manifest callback closure */
	GClosure        *sample_cb;      /* Sample callback closure */
	gsize            maxbuf;         /* Maxiumum buffer size */
	glong            timeout;        /* Buffering timeout in milliseconds */
	gboolean         muted;          /* Subscription is muted (default) */
	GTree           *channels;       /* Subscriptions channels */
	GTree           *manifests;      /* Manifests by source id */
	GQueue          *queue;          /* Queue of pending samples */
	gsize            buflen;         /* Current buffer length */

	//PkaChannel      *channel;        /* Our producing channel */
	//PkaManifest     *manifest;       /* Our current manifest */
	//gboolean         finished;       /* Subscription has completed */
};

/**
 * pka_subscription_destroy:
 * @subscription: A #PkaSubscription.
 *
 * Destroy calback when the subscriptions reference count has reached zero.
 * The subscriptions resources are released.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_subscription_destroy (PkaSubscription *subscription) /* IN */
{
	PkaSample *sample;

	while (!g_queue_is_empty(subscription->queue)) {
		sample = g_queue_pop_head(subscription->queue);
		pka_sample_unref(sample);
	}

	g_mutex_free(subscription->mutex);
	G_OBJECT_UNREF(subscription->encoder);
	G_CLOSURE_UNREF(subscription->manifest_cb);
	G_CLOSURE_UNREF(subscription->sample_cb);
	g_tree_destroy(subscription->channels);
	g_queue_free(subscription->queue);
}

/**
 * pka_subscription_get_type:
 *
 * Retrieves the #GType of the subscription.
 *
 * Returns: A #GType.
 * Side effects: None.
 */
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

static gint
g_int_compare (gint *a, /* IN */
               gint *b) /* IN */
{
	return (*a) - (*b);
}

/**
 * pka_subscription_new:
 *
 * Creates a new instance of #PkaSubscription.  Subscriptions are used to
 * deliver manifests and samples to a client.
 *
 * Returns: The newly created #PkaSubscription.  The subscription should be
 *   freed using pka_subscription_unref().
 * Side effects: None.
 */
PkaSubscription*
pka_subscription_new (void)
{
	static gint id_seq = 0;
	PkaSubscription *sub;

	ENTRY;
	sub = g_slice_new0(PkaSubscription);
	sub->ref_count = 1;
	sub->id = g_atomic_int_exchange_and_add(&id_seq, 1);
	sub->mutex = g_mutex_new();
	sub->muted = TRUE;
#if 0
	sub->channels = g_tree_new_full((GCompareDataFunc)g_int_compare,
	                                NULL, g_free,
	                                (GDestroyNotify)g_object_unref);
#endif
	sub->manifests = g_tree_new_full((GCompareDataFunc)g_int_compare,
	                                 NULL, g_free,
	                                 (GDestroyNotify)pka_manifest_unref);
	sub->queue = g_queue_new();
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
pka_subscription_ref (PkaSubscription *subscription) /* IN */
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
pka_subscription_unref (PkaSubscription *subscription) /* IN */
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
pka_subscription_get_id (PkaSubscription *subscription) /* IN */
{
	g_return_val_if_fail(subscription != NULL, 0);
	return subscription->id;
}

/**
 * pka_subscription_needs_flush_locked:
 * @subscription: A #PkaSubscription.
 *
 * Determines if the subscrtipion needs to be flushed immediately.
 *
 * Returns: %TRUE if the subscription needs to be flushed.
 * Side effects: None.
 */
static inline gboolean
pka_subscription_needs_flush_locked (PkaSubscription *subscription) /* IN */
{
    ENTRY;

	/*
	 * If only a buffer timeout is specified, then we always return FALSE.
	 */
	if (subscription->timeout && !subscription->maxbuf) {
		RETURN(FALSE);
	}

	/*
	 * If we are not buffering, we need to flush immediately.
	 */
	if (!subscription->maxbuf) {
		RETURN(TRUE);
	}

	/*
	 * If there is nothing in the queue, prevent costly flush path.
	 */
	if (!g_queue_get_length(subscription->queue)) {
		RETURN(FALSE);
	}

	RETURN(subscription->buflen >= subscription->maxbuf);
}

static gint
sort_samples_by_source (gconstpointer a,    /* IN */
                        gconstpointer b,    /* IN */
                        gpointer      data) /* IN */
{
	return pka_sample_get_source_id((PkaSample *)a) - 
	       pka_sample_get_source_id((PkaSample *)b);
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
pka_subscription_flush_locked (PkaSubscription *sub) /* IN */
{
	GValue params[3] = { { 0 } };
	PkaManifest *manifest = NULL;
	PkaSample **samples;
	PkaEncoder *encoder;
	guint8 *buf = NULL;
	gsize buflen = 0;
	gsize n_samples = 0;
	gint source_id = 0;
	gint begin = 0;
	gint i;

	g_return_if_fail(sub != NULL);

	/*
	 * We have a queue of samples that are waiting to be shipped off to the
	 * client.  The encoder needs the manifest to know how to intelligently
	 * encode the run of samples.
	 *
	 * To make this easier, we first sort the queue by source id so that
	 * every time we reach a sample with a new source id, we can encode
	 * the run up to that item.
	 *
	 * After encoding the run, we notify the handler with the encoded bytes.
	 */

	ENTRY;

	if (G_UNLIKELY(!sub->sample_cb || g_queue_is_empty(sub->queue))) {
		sub->buflen = 0;
		while (!g_queue_is_empty(sub->queue)) {
			pka_sample_unref(g_queue_pop_head(sub->queue));
		}
		EXIT;
	}

	encoder = sub->encoder;
	sub->buflen = 0;

	/*
	 * Sort the samples by source id so we can encode the runs.
	 */
	g_queue_sort(sub->queue, sort_samples_by_source, NULL);
	n_samples = g_queue_get_length(sub->queue);
	g_assert_cmpint(n_samples, >, 0);
	samples = g_new(PkaSample*, n_samples);

	/*
	 * Add samples to the run and encode them.  Notify the subscriber after
	 * they have been encoded.
	 */
	for (i = 0; i < n_samples; i++) {
		samples[i] = g_queue_pop_head(sub->queue);
		source_id = pka_sample_get_source_id(samples[i]);
		if (!manifest) {
			TRACE(Subscription, "Looking up manifest for source %d.", source_id);
			manifest = g_tree_lookup(sub->manifests, &source_id);
			if (!manifest) {
				/*
				 * We won a race and there is no manifest for the sample yet.
				 * We will simply jump over it and continue.
				 */
				TRACE(Subscription, "Dropping sample due to missing manifest "
				                    "from source %d.", source_id);
				begin = i;
				continue;
			}
		}

		/*
		 * Encode the run of samples if we are at the end of array or the
		 * next sample is not from the same source.
		 */
		if (((i + 1) == n_samples) ||
		    (source_id != pka_sample_get_source_id(g_queue_peek_head(sub->queue))))
		{
			TRACE(Subscription, "Encoding run of samples[%d:%d]", begin, i);
			if (!pka_encoder_encode_samples(encoder, manifest, &samples[begin],
			                                i - begin + 1, &buf, &buflen)) {
				/*
				 * XXX: This is currently a fatal scenario.  We should find a way to
				 *  handle this intelligently at some point.
				 */
				ERROR(Encoder, "An error has occurred while encoding a buffer.  The "
				               "encoder type is: %s.",
				      encoder ? g_type_name(G_TYPE_FROM_INSTANCE(encoder)) : "NONE");
			}
			DUMP_BYTES(Samples, buf, buflen);
			NOTIFY_SAMPLE_HANDLER(sub, buf, buflen);
			begin = i + 1;
			g_free(buf);
			buf = NULL;
			buflen = 0;
		}
	}

	/*
	 * Cleanup after ourselves.
	 */
	for (i = 0; i < n_samples; i++) {
		pka_sample_unref(samples[i]);
	}
	g_free(samples);
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

	ENTRY;

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
	if (!subscription->sample_cb) {
		GOTO(drop_no_handler);
	}
	if (subscription->muted) {
		GOTO(drop_while_muted);
	}
	pka_sample_get_data(sample, &buf, &buflen);
	if (!buflen) {
		GOTO(drop_no_data);
	}
	g_queue_push_tail(subscription->queue, pka_sample_ref(sample));
	subscription->buflen += buflen;
	if (pka_subscription_needs_flush_locked(subscription)) {
		pka_subscription_flush_locked(subscription);
	}
  drop_no_handler:
  drop_while_muted:
  drop_no_data:
	g_mutex_unlock(subscription->mutex);
	EXIT;
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
	guint8 *buf = NULL;
	gsize buflen = 0;

	/*
	 * Whenever a new manifest comes in we flush the current buffer.  This
	 * may not be necessary, as we should be assigning a monotonic id to
	 * both the manifest and the sample.  Also, as we support multiple
	 * manifests (for each source, etc), we should consider only flushing
	 * the relevant samples.
	 *
	 * We can enter this in either one of two states:
	 *
	 * 1) We are muted.  No samples in queue.  No flushing required.  Simply
	 *    store the manifest for when we are unmuted and it will get flushed.
	 * 2) We are unmuted.  Samples may be in queue.  Samples must be flushed
	 *    first.  Then we can store, send our manifest.
	 */

	ENTRY;
	if (subscription->muted) {
		/*
		 * State 1:
		 *
		 * The queue should be empty.  Release the previous manifest and
		 * store the new one for when we are unmuted and flushing occurs.
		 */
		g_assert_cmpint(g_queue_get_length(subscription->queue), ==, 0);
		STORE_MANIFEST(subscription, manifest);
		EXIT;
	}

	/*
	 * State 2:
	 *
	 * Store the manifest, flush the queue if necessary.
	 */
	if (g_queue_get_length(subscription->queue)) {
		pka_subscription_flush_locked(subscription);
	}
	STORE_MANIFEST(subscription, manifest);
	encoder = subscription->encoder;
	if (!pka_encoder_encode_manifest(encoder, manifest, &buf, &buflen)) {
		/*
		 * XXX: Currently a fatal error: cannot encode the data stream.
		 *   This should simply kill the subscription in case of failure.
		 */
		ERROR(Encoder, "An error has occurred while encoding a buffer.  The "
		               "encoder type is: %s.",
		      encoder ? g_type_name(G_TYPE_FROM_INSTANCE(encoder)) : "NONE");
	}
	if (!buflen) {
		GOTO(no_manifest_data);
	}
	DUMP_BYTES(Manifest, buf, buflen);
	NOTIFY_MANIFEST_HANDLER(subscription, manifest);

  no_manifest_data:
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
pka_subscription_deliver_manifest (PkaSubscription *subscription, /* IN */
                                   PkaManifest     *manifest)     /* IN */
{
	g_return_if_fail(subscription != NULL);
	g_return_if_fail(manifest != NULL);

	ENTRY;
	g_mutex_lock(subscription->mutex);
	pka_subscription_deliver_manifest_locked(subscription, manifest);
	g_mutex_unlock(subscription->mutex);
	EXIT;
}

/**
 * pka_subscription_deliver_manifest_foreach:
 * @key: A pointer to the key which is a #gint.
 * @manifest: The manifest instance.
 * @subscription: The #PkaSubscription.
 *
 * A foreach traversal function for a #GTree to deliver the manifests to the
 * subscriber.
 *
 * Returns: %FALSE.
 * Side effects: None.
 */
static gboolean
pka_subscription_deliver_manifest_foreach (gint            *key,          /* IN */
                                           PkaManifest     *manifest,     /* IN */
                                           PkaSubscription *subscription) /* IN */
{
	ENTRY;
	TRACE(Subscription, "Found manifest for source %d.",
	      pka_manifest_get_source_id((manifest)));
	pka_subscription_deliver_manifest_locked(subscription, manifest);
	RETURN(FALSE);
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
pka_subscription_unmute (PkaSubscription *subscription) /* IN */
{
	g_return_if_fail(subscription != NULL);

	ENTRY;
	g_mutex_lock(subscription->mutex);
	subscription->muted = FALSE;
	g_tree_foreach(subscription->manifests,
	               (GTraverseFunc)pka_subscription_deliver_manifest_foreach,
	               subscription);
	g_mutex_unlock(subscription->mutex);
	EXIT;
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
pka_subscription_mute (PkaSubscription *subscription, /* IN */
                       gboolean         drain)        /* IN */
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
