/* pka-subscription.c
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
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

#include "pka-encoder.h"
#include "pka-marshal.h"
#include "pka-log.h"
#include "pka-subscription.h"

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "Subscription"

#define IS_AUTHORIZED(_context, _ioctl, _target) (TRUE)

struct _PkaSubscription
{
	volatile gint         ref_count;
	GStaticRWLock         rw_lock;

	gint                  id;
	PkaSubscriptionState  state;
	GTimeVal              created_at;
	GTree                *channels;
	GTree                *sources;
	GTree                *manifests;
	PkaEncoder           *encoder;
	GClosure             *manifest_closure;
	GClosure             *sample_closure;
};

extern void pka_source_add_subscription    (PkaSource       *source,
                                            PkaSubscription *subscription);
extern void pka_source_remove_subscription (PkaSource       *source,
                                            PkaSubscription *subscription);

/**
 * pka_subscription_destroy:
 * @subscription: A #PkaSubscription.
 *
 * Destroy handler when the reference count has reached zero.
 *
 * Returns: None.
 * Side effects: Armageddon.
 */
static void
pka_subscription_destroy (PkaSubscription *subscription) /* IN */
{
	g_return_if_fail(subscription != NULL);

	ENTRY;
	g_tree_unref(subscription->channels);
	g_tree_unref(subscription->sources);
	g_tree_unref(subscription->manifests);
	if (subscription->manifest_closure) {
		g_closure_unref(subscription->manifest_closure);
	}
	if (subscription->sample_closure) {
		g_closure_unref(subscription->sample_closure);
	}
	if (subscription->encoder) {
		g_object_unref(subscription->encoder);
	}
	EXIT;
}

/**
 * g_int_compare:
 * @a: A pointer to #gint.
 * @b: A pointer to #gint.
 *
 * qsort() style compare method for #GTree.
 *
 * Returns: qsort() return code.
 * Side effects: None.
 */
static gint
g_int_compare (gint *a, /* IN */
               gint *b) /* IN */
{
	return (*a - *b);
}

/**
 * pka_subscription_new:
 *
 * Creates a new instance of #PkaSubscription.
 *
 * Returns: the newly created instance.
 */
PkaSubscription*
pka_subscription_new (void)
{
	static gint id_seq = 0;
	PkaSubscription *subscription;

	#define INITIALIZE_TREE(_field, _free)                                   \
	    G_STMT_START {                                                       \
	   	     subscription->_field = g_tree_new_full(                         \
	   	         (GCompareDataFunc)g_int_compare,                            \
	   	         subscription,                                               \
	   	         (GDestroyNotify)g_free,                                     \
	   	         (GDestroyNotify)_free);                                     \
	   	 } G_STMT_END

	ENTRY;
	subscription = g_slice_new0(PkaSubscription);
	subscription->ref_count = 1;
	g_static_rw_lock_init(&subscription->rw_lock);
	subscription->id = g_atomic_int_exchange_and_add(&id_seq, 1);
	subscription->state = PKA_SUBSCRIPTION_MUTED;
	g_get_current_time(&subscription->created_at);
	INITIALIZE_TREE(channels, g_object_unref);
	INITIALIZE_TREE(sources, g_object_unref);
	INITIALIZE_TREE(manifests, pka_manifest_unref);
	RETURN(subscription);
}

/**
 * pka_subscription_set_encoder:
 * @subscription: A #PkaSubscription.
 *
 * Sets the encoder to be used for @subscription.  The manifest and sample
 * buffers will be encoded using this before notifying the handlers.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_subscription_set_encoder (PkaSubscription  *subscription, /* IN */
                              PkaContext       *context,      /* IN */
                              PkaEncoder       *encoder,      /* IN */
                              GError          **error)        /* OUT */
{
	gboolean ret = FALSE;

	g_return_val_if_fail(subscription != NULL, FALSE);
	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(!encoder || PKA_IS_ENCODER(encoder), FALSE);

	ENTRY;
	g_static_rw_lock_writer_lock(&subscription->rw_lock);
	if (subscription->encoder) {
		g_object_unref(subscription->encoder);
		subscription->encoder = NULL;
	}
	if (encoder) {
		subscription->encoder = g_object_ref(encoder);
	}
	g_static_rw_lock_writer_unlock(&subscription->rw_lock);
	RETURN(ret);
}

/**
 * pka_subscription_channel_source_added:
 * @channel: A #PkaChannel.
 * @source: A #PkaSource.
 * @subscription: A #PkaSubscription.
 *
 * Callback for when a source is removed from a channel.
 *
 * Returns: None.
 * Side effects: @source is removed from the subscription.
 */
static void
pka_subscription_channel_source_added (PkaChannel      *channel,      /* IN */
                                       PkaSource       *source,       /* IN */
                                       PkaSubscription *subscription) /* IN */
{
	GError *error = NULL;

	g_return_if_fail(PKA_IS_CHANNEL(channel));
	g_return_if_fail(PKA_IS_SOURCE(source));
	g_return_if_fail(subscription != NULL);

	ENTRY;
	if (!pka_subscription_add_source(subscription,
	                                 pka_context_default(),
	                                 source,
	                                 &error)) {
		WARNING(Subscription, "%s", error->message);
		g_error_free(error);
	}
	EXIT;
}

/**
 * pka_subscription_channel_source_removed:
 * @channel: A #PkaChannel.
 * @source: A #PkaSource.
 * @subscription: A #PkaSubscription.
 *
 * Callback for when a source is removed from a channel.
 *
 * Returns: None.
 * Side effects: @source is removed from @subscription.
 */
static void
pka_subscription_channel_source_removed (PkaChannel      *channel,      /* IN */
                                         PkaSource       *source,       /* IN */
                                         PkaSubscription *subscription) /* IN */
{
	GError *error = NULL;

	g_return_if_fail(PKA_IS_CHANNEL(channel));
	g_return_if_fail(PKA_IS_SOURCE(source));
	g_return_if_fail(subscription != NULL);

	ENTRY;
	if (!pka_subscription_remove_source(subscription,
	                                    pka_context_default(),
	                                    source,
	                                    &error)) {
		WARNING(Subscription, "%s", error->message);
		g_error_free(error);
	}
	EXIT;
}

/**
 * pka_subscription_add_channel:
 * @subscription: A #PkaSubscription.
 * @context: A #PkaContext.
 * @channel: A #PkaChannel.
 * @error: A location for a #GError, or %NULL.
 *
 * Adds @channel and all of its sources to @subscription.  Any new sources
 * added to @channel will be automatically added to @subscription.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_subscription_add_channel (PkaSubscription  *subscription, /* IN */
                              PkaContext       *context,      /* IN */
                              PkaChannel       *channel,      /* IN */
                              GError          **error)        /* OUT */
{
	gboolean ret = FALSE;
	GList *sources = NULL;
	GList *iter;
	gint *key;

	g_return_val_if_fail(subscription != NULL, FALSE);
	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);

	ENTRY;
	if (!IS_AUTHORIZED(context, MODIFY_SUBSCRIPTION, subscription)) {
		g_set_error(error, PKA_CONTEXT_ERROR,
		            PKA_CONTEXT_ERROR_NOT_AUTHORIZED,
		            "Not authorized to add channel.");
		GOTO(failed);
	}
	key = g_new(gint, 1);
	*key = pka_channel_get_id(channel);
	g_static_rw_lock_writer_lock(&subscription->rw_lock);
	g_tree_insert(subscription->channels, key, g_object_ref(channel));
	g_static_rw_lock_writer_unlock(&subscription->rw_lock);
	g_signal_connect(channel, "source-added",
	                 G_CALLBACK(pka_subscription_channel_source_added),
	                 subscription);
	g_signal_connect(channel, "source-removed",
	                 G_CALLBACK(pka_subscription_channel_source_removed),
	                 subscription);
	sources = pka_channel_get_sources(channel);
	for (iter = sources; iter; iter = iter->next) {
		if (!pka_subscription_add_source(subscription, context,
		                                 iter->data, NULL)) {
			WARNING(Subscription,
			        "Failed to add source %d to subscription %d.",
			        pka_source_get_id(iter->data),
			        subscription->id);
		}
	}
	g_list_foreach(sources, (GFunc)g_object_unref, NULL);
	g_list_free(sources);
	ret = TRUE;
  failed:
	RETURN(ret);
}

/**
 * pka_subscription_remove_channel:
 * @subscription: A #PkaSubscription.
 * @context: A #PkaContext.
 * @channel: A #PkaChannel.
 * @error: A location #GError, or %NULL.
 *
 * Removes a channel and all of its sources from being monitored by the
 * subscription.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_subscription_remove_channel (PkaSubscription  *subscription, /* IN */
                                 PkaContext       *context,      /* IN */
                                 PkaChannel       *channel,      /* IN */
                                 GError          **error)        /* OUT */
{
	gboolean ret = FALSE;
	GList *sources = NULL;
	GList *iter;
	gint key;

	g_return_val_if_fail(subscription != NULL, FALSE);
	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);

	ENTRY;
	if (!IS_AUTHORIZED(context, MODIFY_SUBSCRIPTION, subscription)) {
		g_set_error(error, PKA_CONTEXT_ERROR,
		            PKA_CONTEXT_ERROR_NOT_AUTHORIZED,
		            "Not authorized to remove channel %d from subscription %d.",
		            pka_channel_get_id(channel), subscription->id);
		GOTO(failed);
	}
	g_signal_handlers_disconnect_by_func(channel,
	                                     pka_subscription_channel_source_added,
	                                     subscription);
	g_signal_handlers_disconnect_by_func(channel,
	                                     pka_subscription_channel_source_removed,
	                                     subscription);
	sources = pka_channel_get_sources(channel);
	for (iter = sources; iter; iter = iter->next) {
		if (!pka_subscription_remove_source(subscription, context,
		                                    iter->data, NULL)) {
		    WARNING(Subscription,
		            "Could not remove source %d from subscription %d",
		            pka_source_get_id(iter->data), subscription->id);
		}
	}
	g_list_foreach(sources, (GFunc)g_object_unref, NULL);
	g_list_free(sources);
	key = pka_channel_get_id(channel);
	g_static_rw_lock_writer_lock(&subscription->rw_lock);
	g_tree_remove(subscription->channels, &key);
	g_static_rw_lock_writer_unlock(&subscription->rw_lock);
	ret = TRUE;
  failed:
	RETURN(ret);
}

/**
 * pka_subscription_add_source:
 * @subscription: A #PkaSubscription.
 * @context: A #PkaContext.
 * @source: A #PkaSource.
 * @error: A location for a #GError, or %NULL.
 *
 * Adds @source to the list of #PkaSource<!-- -->'s monitored by
 * @subscription.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_subscription_add_source (PkaSubscription  *subscription, /* IN */
                             PkaContext       *context,      /* IN */
                             PkaSource        *source,       /* IN */
                             GError          **error)        /* OUT */
{
	gboolean ret = FALSE;
	gint *key;

	g_return_val_if_fail(subscription != NULL, FALSE);
	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(PKA_IS_SOURCE(source), FALSE);

	ENTRY;
	if (!IS_AUTHORIZED(context, MODIFY_SUBSCRIPTION, subscription)) {
		g_set_error(error, PKA_CONTEXT_ERROR,
		            PKA_CONTEXT_ERROR_NOT_AUTHORIZED,
		            "Not authoraized to remove source %d from subscription %d.",
		            pka_source_get_id(source), subscription->id);
		GOTO(failed);
	}
	g_static_rw_lock_writer_lock(&subscription->rw_lock);
	key = g_new(gint, 1);
	*key = pka_source_get_id(source);
	g_tree_insert(subscription->sources, key, g_object_ref(source));
	g_static_rw_lock_writer_unlock(&subscription->rw_lock);
	/*
	 * The source will notify us of the current manifest if necessary.  Such
	 * an example would be if a source is already running and the source is
	 * added to the subscription.
	 *
	 * Therefore, this should be done outside of any subscription locks.
	 */
	pka_source_add_subscription(source, subscription);
	ret = TRUE;
  failed:
	RETURN(ret);
}

/**
 * pka_subscription_remove_source:
 * @subscription: A #PkaSubscription.
 * @context: A #PkaContext.
 * @source: A #PkaSource.
 * @error: A location for #GError, or %NULL.
 *
 * Removes @source from @subscription.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_subscription_remove_source (PkaSubscription  *subscription, /* IN */
                                PkaContext       *context,      /* IN */
                                PkaSource        *source,       /* IN */
                                GError          **error)        /* OUT */
{
	gboolean ret = FALSE;
	gint key;

	g_return_val_if_fail(subscription != NULL, FALSE);
	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(PKA_IS_SOURCE(source), FALSE);

	ENTRY;
	if (!IS_AUTHORIZED(context, MODIFY_SUBSCRIPTION, subscription)) {
		g_set_error(error, PKA_CONTEXT_ERROR,
		            PKA_CONTEXT_ERROR_NOT_AUTHORIZED,
		            "Not authoraized to remove source %d from subscription %d.",
		            pka_source_get_id(source), subscription->id);
		GOTO(failed);
	}
	key = pka_source_get_id(source);
	pka_source_remove_subscription(source, subscription);
	g_static_rw_lock_writer_lock(&subscription->rw_lock);
	g_tree_remove(subscription->sources, &key);
	g_static_rw_lock_writer_unlock(&subscription->rw_lock);
	ret = TRUE;
  failed:
	RETURN(ret);
}

/**
 * pka_subscription_set_state:
 * @subscription: A #PkaSubscription.
 *
 * Sets the current #PkaSubscriptionState of the subscription.
 *
 * Returns: None.
 * Side effects: None.
 */
gboolean
pka_subscription_set_state (PkaSubscription       *subscription, /* IN */
                            PkaContext            *context,      /* IN */
                            PkaSubscriptionState   state,        /* IN */
                            GError               **error)        /* OUT */
{
	gboolean ret = FALSE;

	g_return_val_if_fail(subscription != NULL, FALSE);
	g_return_val_if_fail(context != NULL, FALSE);

	ENTRY;
	if ((state != PKA_SUBSCRIPTION_MUTED) &&
	    (state != PKA_SUBSCRIPTION_UNMUTED)) {
		g_set_error(error, PKA_CONTEXT_ERROR,
		            PKA_CONTEXT_ERROR_NOT_AUTHORIZED,
		            "Invalid state [0x%08x].", state);
		GOTO(failed);
	}
	if (!IS_AUTHORIZED(context, MODIFY_SUBSCRIPTION, subscription)) {
		g_set_error(error, PKA_CONTEXT_ERROR,
		            PKA_CONTEXT_ERROR_NOT_AUTHORIZED,
		            "Cannot set state to 0x%08x.", (gint)state);
		GOTO(failed);
	}
	g_static_rw_lock_writer_lock(&subscription->rw_lock);
	if (subscription->state == state) {
		GOTO(finished);
	}
	subscription->state = state;
	switch (state) {
	CASE(PKA_SUBSCRIPTION_UNMUTED);
		/*
		 * TODO: Send the current manifests to the handler.
		 */
		BREAK;
	CASE(PKA_SUBSCRIPTION_MUTED);
		BREAK;
	default:
		g_warn_if_reached();
		BREAK;
	}
  finished:
	g_static_rw_lock_writer_unlock(&subscription->rw_lock);
	ret = TRUE;
  failed:
  	RETURN(ret);
}

/**
 * pka_subscription_mute:
 * @subscription: A #PkaSubscriptionState.
 *
 * Mutes the subscription, preventing future manifest and sample delivery
 * to the configured handlers.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_subscription_mute (PkaSubscription  *subscription, /* IN */
                       PkaContext       *context,      /* IN */
                       gboolean          drain,        /* IN */
                       GError          **error)        /* OUT */
{
	gboolean ret;

	ENTRY;
	/* FIXME: Add support for draining the buffer. */
	ret = pka_subscription_set_state(subscription, context,
	                                 PKA_SUBSCRIPTION_MUTED,
	                                 error);
	RETURN(ret);
}

/**
 * pka_subscription_unmute:
 * @subscription: A #PkaSubscription.
 *
 * Unmutes a muted subscription, allowing future manifest and sample
 * delivery to the configured handlers.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_subscription_unmute (PkaSubscription  *subscription, /* IN */
                         PkaContext       *context,      /* IN */
                         GError          **error)        /* OUT */
{
	gboolean ret;

	ENTRY;
	ret = pka_subscription_set_state(subscription, context,
	                                 PKA_SUBSCRIPTION_UNMUTED,
	                                 error);
	RETURN(ret);
}

/**
 * pka_subscription_deliver_manifest:
 * @subscription: A #PkaSubscription.
 * @source: A #PkaSource.
 * @manifest: A #PkaManifest.
 *
 * Delivers @manifest from @souce to the subscriptions handlers.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_subscription_deliver_manifest (PkaSubscription *subscription, /* IN */
                                   PkaSource       *source,       /* IN */
                                   PkaManifest     *manifest)     /* IN */
{
	GValue params[3] = { { 0 } };
	guint8 *buffer = NULL;
	gsize buffer_len = 0;

	g_return_if_fail(subscription != NULL);
	g_return_if_fail(manifest != NULL);
	g_return_if_fail(PKA_IS_SOURCE(source));

	ENTRY;
	g_static_rw_lock_reader_lock(&subscription->rw_lock);
	if (G_LIKELY(subscription->manifest_closure)) {
		if (!pka_encoder_encode_manifest(NULL, manifest, &buffer, &buffer_len)) {
			WARNING(Subscription, "Subscription %d failed to encode manifest.",
					subscription->id);
			GOTO(failed);
		}
		DUMP_BYTES(Manifest, buffer, buffer_len);
		/*
		 * XXX: It should be obvious that this marshalling isn't very fast.
		 *   But I've certainly done worse.  At least it handles things cleanly
		 *   with regard to using libffi.
		 */
		g_value_init(&params[0], PKA_TYPE_SUBSCRIPTION);
		g_value_init(&params[1], G_TYPE_POINTER);
		g_value_init(&params[2], G_TYPE_ULONG);
		g_value_set_boxed(&params[0], subscription);
		g_value_set_pointer(&params[1], buffer);
		g_value_set_ulong(&params[2], buffer_len);
		g_closure_invoke(subscription->manifest_closure, NULL,
		                 3, &params[0], NULL);
		g_value_unset(&params[0]);
		g_value_unset(&params[1]);
		g_value_unset(&params[2]);
		g_free(buffer);
	}
  failed:
	g_static_rw_lock_reader_unlock(&subscription->rw_lock);
	EXIT;
}

/**
 * pka_subscription_deliver_sample:
 * @subscription: A #PkaSubscription.
 *
 * Delivers @sample from @source to the @subscription.  @manifest should
 * be the current manifest for the source that has already been sent
 * to pka_subscription_deliver_manifest().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_subscription_deliver_sample (PkaSubscription *subscription, /* IN */
                                 PkaSource       *source,       /* IN */
                                 PkaManifest     *manifest,     /* IN */
                                 PkaSample       *sample)       /* IN */
{
	GValue params[3] = { { 0 } };
	guint8 *buffer = NULL;
	gsize buffer_len = 0;
	PkaSample *samples[1] = { sample };

	g_return_if_fail(subscription != NULL);
	g_return_if_fail(sample != NULL);
	g_return_if_fail(PKA_IS_SOURCE(source));

	ENTRY;
	g_static_rw_lock_reader_lock(&subscription->rw_lock);
	if (G_LIKELY(subscription->sample_closure)) {
		if (!pka_encoder_encode_samples(NULL, manifest, samples, 1,
		                                &buffer, &buffer_len)) {
			WARNING(Subscription, "Subscription %d failed to encode sample.",
			        subscription->id);
			GOTO(failed);
		}
		DUMP_BYTES(Sample, buffer, buffer_len);
		/*
		 * XXX: It should be obvious that this marshalling isn't very fast.
		 *   But I've certainly done worse.
		 */
		g_value_init(&params[0], PKA_TYPE_SUBSCRIPTION);
		g_value_init(&params[1], G_TYPE_POINTER);
		g_value_init(&params[2], G_TYPE_ULONG);
		g_value_set_boxed(&params[0], subscription);
		g_value_set_pointer(&params[1], buffer);
		g_value_set_ulong(&params[2], buffer_len);
		g_closure_invoke(subscription->sample_closure, NULL,
		                 3, &params[0], NULL);
		g_value_unset(&params[0]);
		g_value_unset(&params[1]);
		g_value_unset(&params[2]);
		g_free(buffer);
	}
  failed:
	g_static_rw_lock_reader_unlock(&subscription->rw_lock);
	EXIT;
}

/**
 * pka_subscription_set_handlers:
 * @subscription: A #PkaSubscription.
 * @context: A #PkaContext.
 * @manifest_func: A manifest callback function.
 * @manifest_data: Data for @manifest_func.
 * @manifest_destroy: A #GDestroyNotify to call when @manifest_func is no
 *    longer needed.
 * @sample_func: A sample callback function.
 * @sample_data: Data for @sample_func.
 * @sample_destroy: A #GDestroyNotify to call when @sample_func is no
 *    longer needed.
 *
 * Sets the manifest and sample callback methods for the subscription.
 * @manifest_func will be called when a manifest is received from a source
 * on the subscription.  @sample_func will be called when a sample is received
 * from the subscription.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_subscription_set_handlers (PkaSubscription  *subscription,     /* IN */
                               PkaContext       *context,          /* IN */
                               PkaManifestFunc   manifest_func,    /* IN */
                               gpointer          manifest_data,    /* IN */
                               GDestroyNotify    manifest_destroy, /* IN */
                               PkaSampleFunc     sample_func,      /* IN */
                               gpointer          sample_data,      /* IN */
                               GDestroyNotify    sample_destroy,   /* IN */
                               GError          **error)            /* IN */
{
	GClosure *manifest;
	GClosure *sample;

	g_return_if_fail(subscription != NULL);
	g_return_if_fail(context != NULL);

	ENTRY;
	/*
	 * Create the closures and set the marshaller.
	 */
	manifest = g_cclosure_new(G_CALLBACK(manifest_func),
	                          manifest_data,
	                          (GClosureNotify)manifest_destroy);
	sample = g_cclosure_new(G_CALLBACK(sample_func),
	                        sample_data,
	                        (GClosureNotify)sample_destroy);
	g_closure_set_marshal(manifest, pka_marshal_VOID__POINTER_ULONG);
	g_closure_set_marshal(sample, pka_marshal_VOID__POINTER_ULONG);
	/*
	 * Store the closures. Requires writer lock.
	 */
	g_static_rw_lock_writer_lock(&subscription->rw_lock);
	if (subscription->manifest_closure) {
		g_closure_unref(subscription->manifest_closure);
	}
	if (subscription->sample_closure) {
		g_closure_unref(subscription->sample_closure);
	}
	subscription->manifest_closure = manifest;
	subscription->sample_closure = sample;
	g_static_rw_lock_writer_unlock(&subscription->rw_lock);
	EXIT;
}

void
pka_subscription_get_created_at (PkaSubscription *subscription, /* IN */
                                 GTimeVal        *created_at)   /* OUT */
{
	g_return_if_fail(subscription != NULL);
	g_return_if_fail(created_at != NULL);

	ENTRY;
	*created_at = subscription->created_at;
	EXIT;
}

/**
 * pka_subscription_get_id:
 * @subscription: A #PkaSubscription.
 *
 * Retrieves monotonic identifier for @subscription.
 *
 * Returns: A #gint.
 * Side effects: None.
 */
gint
pka_subscription_get_id (PkaSubscription *subscription) /* IN */
{
	g_return_val_if_fail(subscription != NULL, -1);

	ENTRY;
	RETURN(subscription->id);
}

/**
 * PkaSubscription_ref:
 * @subscription: A #PkaSubscription.
 *
 * Atomically increments the reference count of @subscription by one.
 *
 * Returns: A reference to @subscription.
 * Side effects: None.
 */
PkaSubscription*
pka_subscription_ref (PkaSubscription *subscription) /* IN */
{
	g_return_val_if_fail(subscription != NULL, NULL);
	g_return_val_if_fail(subscription->ref_count > 0, NULL);

	ENTRY;
	g_atomic_int_inc(&subscription->ref_count);
	RETURN(subscription);
}

/**
 * pka_subscription_unref:
 * @subscription: A PkaSubscription.
 *
 * Atomically decrements the reference count of @subscription by one.  When the
 * reference count reaches zero, the structure will be destroyed and
 * freed.
 *
 * Returns: None.
 * Side effects: The structure will be freed when the reference count
 *   reaches zero.
 */
void
pka_subscription_unref (PkaSubscription *subscription) /* IN */
{
	g_return_if_fail(subscription != NULL);
	g_return_if_fail(subscription->ref_count > 0);

	ENTRY;
	if (g_atomic_int_dec_and_test(&subscription->ref_count)) {
		pka_subscription_destroy(subscription);
		g_slice_free(PkaSubscription, subscription);
	}
	EXIT;
}

/**
 * pka_subscription_get_type:
 *
 * Retrieves the GType for #PkaSubscription.
 *
 * Returns: A #GType.
 * Side effects: None.
 */
GType
pka_subscription_get_type (void)
{
	static gsize initialized = FALSE;
	static GType type_id = G_TYPE_INVALID;

	if (g_once_init_enter(&initialized)) {
		type_id = g_boxed_type_register_static(
				"PkaSubscription",
				(GBoxedCopyFunc)pka_subscription_ref,
				(GBoxedFreeFunc)pka_subscription_unref);
		g_once_init_leave(&initialized, TRUE);
	}
	return type_id;
}
