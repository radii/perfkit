/* pk-subscription.c
 *
 * Copyright 2010 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "pk-subscription.h"
#include "pk-connection-lowlevel.h"
#include "pk-log.h"

/**
 * SECTION:pk-subscription
 * @title: PkSubscription
 * @short_description: 
 *
 * Section overview.
 */

G_DEFINE_TYPE(PkSubscription, pk_subscription, G_TYPE_OBJECT)

struct _PkSubscriptionPrivate
{
	PkConnection *connection;
	gint id;
};

enum
{
	PROP_0,
	PROP_ID,
	PROP_CONNECTION,
};

/**
 * pk_subscription_get_id:
 * @subscription: A #PkSubscription.
 *
 * Retrieves the identifier for the subscription.
 *
 * Returns: A gint.
 * Side effects: None.
 */
gint
pk_subscription_get_id (PkSubscription *subscription) /* IN */
{
	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), -1);
	return subscription->priv->id;
}

/**
 * pk_subscription_get_connection:
 * @subscription: A #PkSubscription.
 *
 * Retrieves the connection for the subscription.
 *
 * Returns: A PkConnection.
 * Side effects: None.
 */
PkConnection*
pk_subscription_get_connection (PkSubscription *subscription) /* IN */
{
	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), NULL);
	return subscription->priv->connection;
}

/**
 * pk_subscription_add_channel:
 * @subscription: A #PkSubscription.
 * @channel: A gint.
 * @monitor: A gboolean.
 * @error: A location for a #GError, or %NULL.
 *
 * Adds all sources of @channel to the list of sources for which manifest
 * and samples are delivered to the subscriber.
 * 
 * If @monitor is TRUE, then sources added to @channel will automatically
 * be added to the subscription.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_subscription_add_channel (PkSubscription  *subscription, /* IN */
                             PkChannel       *channel,      /* IN */
                             gboolean         monitor,      /* IN */
                             GError         **error)        /* OUT */
{
	PkSubscriptionPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);

	ENTRY;
	priv = subscription->priv;
	if (!(ret = pk_connection_subscription_add_channel(
			priv->connection,
			priv->id,
			pk_channel_get_id(channel),
			monitor,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_subscription_add_channel_cb (GObject      *object,    /* IN */
                                GAsyncResult *result,    /* IN */
                                gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_subscription_add_channel_async:
 * @subscription: A #PkSubscription.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Adds all sources of @channel to the list of sources for which manifest
 * and samples are delivered to the subscriber.
 * 
 * If @monitor is TRUE, then sources added to @channel will automatically
 * be added to the subscription.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_subscription_add_channel_async (PkSubscription      *subscription, /* IN */
                                   PkChannel           *channel,      /* IN */
                                   gboolean             monitor,      /* IN */
                                   GCancellable        *cancellable,  /* IN */
                                   GAsyncReadyCallback  callback,     /* IN */
                                   gpointer             user_data)    /* IN */
{
	PkSubscriptionPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_SUBSCRIPTION(subscription));

	ENTRY;
	priv = subscription->priv;
	result = g_simple_async_result_new(
			G_OBJECT(subscription),
			callback,
			user_data,
			pk_subscription_add_channel_async);
	pk_connection_subscription_add_channel_async(
			priv->connection,
			priv->id,
			pk_channel_get_id(channel),
			monitor,
			cancellable,
			pk_subscription_add_channel_cb,
			result);
	EXIT;
}


/**
 * pk_subscription_add_channel_finish:
 * @subscription: A #PkSubscription.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Adds all sources of @channel to the list of sources for which manifest
 * and samples are delivered to the subscriber.
 * 
 * If @monitor is TRUE, then sources added to @channel will automatically
 * be added to the subscription.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_subscription_add_channel_finish (PkSubscription  *subscription, /* IN */
                                    GAsyncResult    *result,       /* IN */
                                    GError         **error)        /* OUT */
{
	PkSubscriptionPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);

	ENTRY;
	priv = subscription->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_subscription_add_channel_finish(
			priv->connection,
			real_result,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_subscription_add_source:
 * @subscription: A #PkSubscription.
 * @source: A gint.
 * @error: A location for a #GError, or %NULL.
 *
 * Adds @source to the list of sources for which manifest and samples are
 * delivered to the subscriber.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_subscription_add_source (PkSubscription  *subscription, /* IN */
                            PkSource        *source,       /* IN */
                            GError         **error)        /* OUT */
{
	PkSubscriptionPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);

	ENTRY;
	priv = subscription->priv;
	if (!(ret = pk_connection_subscription_add_source(
			priv->connection,
			priv->id,
			pk_source_get_id(source),
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_subscription_add_source_cb (GObject      *object,    /* IN */
                               GAsyncResult *result,    /* IN */
                               gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_subscription_add_source_async:
 * @subscription: A #PkSubscription.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Adds @source to the list of sources for which manifest and samples are
 * delivered to the subscriber.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_subscription_add_source_async (PkSubscription      *subscription, /* IN */
                                  PkSource            *source,       /* IN */
                                  GCancellable        *cancellable,  /* IN */
                                  GAsyncReadyCallback  callback,     /* IN */
                                  gpointer             user_data)    /* IN */
{
	PkSubscriptionPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_SUBSCRIPTION(subscription));

	ENTRY;
	priv = subscription->priv;
	result = g_simple_async_result_new(
			G_OBJECT(subscription),
			callback,
			user_data,
			pk_subscription_add_source_async);
	pk_connection_subscription_add_source_async(
			priv->connection,
			priv->id,
			pk_source_get_id(source),
			cancellable,
			pk_subscription_add_source_cb,
			result);
	EXIT;
}


/**
 * pk_subscription_add_source_finish:
 * @subscription: A #PkSubscription.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Adds @source to the list of sources for which manifest and samples are
 * delivered to the subscriber.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_subscription_add_source_finish (PkSubscription  *subscription, /* IN */
                                   GAsyncResult    *result,       /* IN */
                                   GError         **error)        /* OUT */
{
	PkSubscriptionPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);

	ENTRY;
	priv = subscription->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_subscription_add_source_finish(
			priv->connection,
			real_result,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_subscription_mute:
 * @subscription: A #PkSubscription.
 * @drain: A gboolean.
 * @error: A location for a #GError, or %NULL.
 *
 * Prevents the subscription from further manifest or sample delivery.  If
 * @drain is set, the current buffer will be flushed.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_subscription_mute (PkSubscription  *subscription, /* IN */
                      gboolean         drain,        /* IN */
                      GError         **error)        /* OUT */
{
	PkSubscriptionPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);

	ENTRY;
	priv = subscription->priv;
	if (!(ret = pk_connection_subscription_mute(
			priv->connection,
			priv->id,
			drain,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_subscription_mute_cb (GObject      *object,    /* IN */
                         GAsyncResult *result,    /* IN */
                         gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_subscription_mute_async:
 * @subscription: A #PkSubscription.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Prevents the subscription from further manifest or sample delivery.  If
 * @drain is set, the current buffer will be flushed.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_subscription_mute_async (PkSubscription      *subscription, /* IN */
                            gboolean             drain,        /* IN */
                            GCancellable        *cancellable,  /* IN */
                            GAsyncReadyCallback  callback,     /* IN */
                            gpointer             user_data)    /* IN */
{
	PkSubscriptionPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_SUBSCRIPTION(subscription));

	ENTRY;
	priv = subscription->priv;
	result = g_simple_async_result_new(
			G_OBJECT(subscription),
			callback,
			user_data,
			pk_subscription_mute_async);
	pk_connection_subscription_mute_async(
			priv->connection,
			priv->id,
			drain,
			cancellable,
			pk_subscription_mute_cb,
			result);
	EXIT;
}


/**
 * pk_subscription_mute_finish:
 * @subscription: A #PkSubscription.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Prevents the subscription from further manifest or sample delivery.  If
 * @drain is set, the current buffer will be flushed.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_subscription_mute_finish (PkSubscription  *subscription, /* IN */
                             GAsyncResult    *result,       /* IN */
                             GError         **error)        /* OUT */
{
	PkSubscriptionPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);

	ENTRY;
	priv = subscription->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_subscription_mute_finish(
			priv->connection,
			real_result,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_subscription_remove_channel:
 * @subscription: A #PkSubscription.
 * @channel: A gint.
 * @error: A location for a #GError, or %NULL.
 *
 * Removes @channel and all of its sources from the subscription.  This
 * prevents further manifest and sample delivery to the subscriber.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_subscription_remove_channel (PkSubscription  *subscription, /* IN */
                                PkChannel       *channel,      /* IN */
                                GError         **error)        /* OUT */
{
	PkSubscriptionPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);

	ENTRY;
	priv = subscription->priv;
	if (!(ret = pk_connection_subscription_remove_channel(
			priv->connection,
			priv->id,
			pk_channel_get_id(channel),
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_subscription_remove_channel_cb (GObject      *object,    /* IN */
                                   GAsyncResult *result,    /* IN */
                                   gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_subscription_remove_channel_async:
 * @subscription: A #PkSubscription.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Removes @channel and all of its sources from the subscription.  This
 * prevents further manifest and sample delivery to the subscriber.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_subscription_remove_channel_async (PkSubscription      *subscription, /* IN */
                                      PkChannel           *channel,      /* IN */
                                      GCancellable        *cancellable,  /* IN */
                                      GAsyncReadyCallback  callback,     /* IN */
                                      gpointer             user_data)    /* IN */
{
	PkSubscriptionPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_SUBSCRIPTION(subscription));

	ENTRY;
	priv = subscription->priv;
	result = g_simple_async_result_new(
			G_OBJECT(subscription),
			callback,
			user_data,
			pk_subscription_remove_channel_async);
	pk_connection_subscription_remove_channel_async(
			priv->connection,
			priv->id,
			pk_channel_get_id(channel),
			cancellable,
			pk_subscription_remove_channel_cb,
			result);
	EXIT;
}


/**
 * pk_subscription_remove_channel_finish:
 * @subscription: A #PkSubscription.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Removes @channel and all of its sources from the subscription.  This
 * prevents further manifest and sample delivery to the subscriber.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_subscription_remove_channel_finish (PkSubscription  *subscription, /* IN */
                                       GAsyncResult    *result,       /* IN */
                                       GError         **error)        /* OUT */
{
	PkSubscriptionPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);

	ENTRY;
	priv = subscription->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_subscription_remove_channel_finish(
			priv->connection,
			real_result,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_subscription_remove_source:
 * @subscription: A #PkSubscription.
 * @source: A gint.
 * @error: A location for a #GError, or %NULL.
 *
 * Removes @source from the subscription.  This prevents further manifest
 * and sample delivery to the subscriber.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_subscription_remove_source (PkSubscription  *subscription, /* IN */
                               PkSource        *source,       /* IN */
                               GError         **error)        /* OUT */
{
	PkSubscriptionPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);

	ENTRY;
	priv = subscription->priv;
	if (!(ret = pk_connection_subscription_remove_source(
			priv->connection,
			priv->id,
			pk_source_get_id(source),
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_subscription_remove_source_cb (GObject      *object,    /* IN */
                                  GAsyncResult *result,    /* IN */
                                  gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_subscription_remove_source_async:
 * @subscription: A #PkSubscription.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Removes @source from the subscription.  This prevents further manifest
 * and sample delivery to the subscriber.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_subscription_remove_source_async (PkSubscription      *subscription, /* IN */
                                     PkSource            *source,       /* IN */
                                     GCancellable        *cancellable,  /* IN */
                                     GAsyncReadyCallback  callback,     /* IN */
                                     gpointer             user_data)    /* IN */
{
	PkSubscriptionPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_SUBSCRIPTION(subscription));

	ENTRY;
	priv = subscription->priv;
	result = g_simple_async_result_new(
			G_OBJECT(subscription),
			callback,
			user_data,
			pk_subscription_remove_source_async);
	pk_connection_subscription_remove_source_async(
			priv->connection,
			priv->id,
			pk_source_get_id(source),
			cancellable,
			pk_subscription_remove_source_cb,
			result);
	EXIT;
}


/**
 * pk_subscription_remove_source_finish:
 * @subscription: A #PkSubscription.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Removes @source from the subscription.  This prevents further manifest
 * and sample delivery to the subscriber.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_subscription_remove_source_finish (PkSubscription  *subscription, /* IN */
                                      GAsyncResult    *result,       /* IN */
                                      GError         **error)        /* OUT */
{
	PkSubscriptionPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);

	ENTRY;
	priv = subscription->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_subscription_remove_source_finish(
			priv->connection,
			real_result,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_subscription_set_buffer:
 * @subscription: A #PkSubscription.
 * @timeout: A gint.
 * @size: A gint.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the buffering timeout and maximum buffer size for the subscription.
 * If @timeout milliseconds pass or @size bytes are consummed buffering,
 * the data will be delivered to the subscriber.
 * 
 * Set @timeout and @size to 0 to disable buffering.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_subscription_set_buffer (PkSubscription  *subscription, /* IN */
                            gint             timeout,      /* IN */
                            gint             size,         /* IN */
                            GError         **error)        /* OUT */
{
	PkSubscriptionPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);

	ENTRY;
	priv = subscription->priv;
	if (!(ret = pk_connection_subscription_set_buffer(
			priv->connection,
			priv->id,
			timeout,
			size,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_subscription_set_buffer_cb (GObject      *object,    /* IN */
                               GAsyncResult *result,    /* IN */
                               gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_subscription_set_buffer_async:
 * @subscription: A #PkSubscription.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Sets the buffering timeout and maximum buffer size for the subscription.
 * If @timeout milliseconds pass or @size bytes are consummed buffering,
 * the data will be delivered to the subscriber.
 * 
 * Set @timeout and @size to 0 to disable buffering.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_subscription_set_buffer_async (PkSubscription      *subscription, /* IN */
                                  gint                 timeout,      /* IN */
                                  gint                 size,         /* IN */
                                  GCancellable        *cancellable,  /* IN */
                                  GAsyncReadyCallback  callback,     /* IN */
                                  gpointer             user_data)    /* IN */
{
	PkSubscriptionPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_SUBSCRIPTION(subscription));

	ENTRY;
	priv = subscription->priv;
	result = g_simple_async_result_new(
			G_OBJECT(subscription),
			callback,
			user_data,
			pk_subscription_set_buffer_async);
	pk_connection_subscription_set_buffer_async(
			priv->connection,
			priv->id,
			timeout,
			size,
			cancellable,
			pk_subscription_set_buffer_cb,
			result);
	EXIT;
}


/**
 * pk_subscription_set_buffer_finish:
 * @subscription: A #PkSubscription.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the buffering timeout and maximum buffer size for the subscription.
 * If @timeout milliseconds pass or @size bytes are consummed buffering,
 * the data will be delivered to the subscriber.
 * 
 * Set @timeout and @size to 0 to disable buffering.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_subscription_set_buffer_finish (PkSubscription  *subscription, /* IN */
                                   GAsyncResult    *result,       /* IN */
                                   GError         **error)        /* OUT */
{
	PkSubscriptionPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);

	ENTRY;
	priv = subscription->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_subscription_set_buffer_finish(
			priv->connection,
			real_result,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_subscription_unmute:
 * @subscription: A #PkSubscription.
 * @error: A location for a #GError, or %NULL.
 *
 * Enables the subscription for manifest and sample delivery.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_subscription_unmute (PkSubscription  *subscription, /* IN */
                        GError         **error)        /* OUT */
{
	PkSubscriptionPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);

	ENTRY;
	priv = subscription->priv;
	if (!(ret = pk_connection_subscription_unmute(
			priv->connection,
			priv->id,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_subscription_unmute_cb (GObject      *object,    /* IN */
                           GAsyncResult *result,    /* IN */
                           gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_subscription_unmute_async:
 * @subscription: A #PkSubscription.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Enables the subscription for manifest and sample delivery.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_subscription_unmute_async (PkSubscription      *subscription, /* IN */
                              GCancellable        *cancellable,  /* IN */
                              GAsyncReadyCallback  callback,     /* IN */
                              gpointer             user_data)    /* IN */
{
	PkSubscriptionPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_SUBSCRIPTION(subscription));

	ENTRY;
	priv = subscription->priv;
	result = g_simple_async_result_new(
			G_OBJECT(subscription),
			callback,
			user_data,
			pk_subscription_unmute_async);
	pk_connection_subscription_unmute_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_subscription_unmute_cb,
			result);
	EXIT;
}


/**
 * pk_subscription_unmute_finish:
 * @subscription: A #PkSubscription.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Enables the subscription for manifest and sample delivery.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_subscription_unmute_finish (PkSubscription  *subscription, /* IN */
                               GAsyncResult    *result,       /* IN */
                               GError         **error)        /* OUT */
{
	PkSubscriptionPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);

	ENTRY;
	priv = subscription->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_subscription_unmute_finish(
			priv->connection,
			real_result,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_subscription_finalize (GObject *object) /* IN */
{
	G_OBJECT_CLASS(pk_subscription_parent_class)->finalize(object);
}

static void
pk_subscription_get_property (GObject    *object,  /* IN */
                        guint       prop_id, /* IN */
                        GValue     *value,   /* IN */
                        GParamSpec *pspec)   /* IN */
{
	switch (prop_id) {
	case PROP_ID:
		g_value_set_int(value,
		                pk_subscription_get_id(PK_SUBSCRIPTION(object)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_subscription_set_property (GObject      *object,  /* IN */
                        guint         prop_id, /* IN */
                        const GValue *value,   /* IN */
                        GParamSpec   *pspec)   /* IN */
{
	switch (prop_id) {
	case PROP_ID:
		PK_SUBSCRIPTION(object)->priv->id = g_value_get_int(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_subscription_class_init (PkSubscriptionClass *klass) /* IN */
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pk_subscription_finalize;
	object_class->get_property = pk_subscription_get_property;
	object_class->set_property = pk_subscription_set_property;
	g_type_class_add_private(object_class, sizeof(PkSubscriptionPrivate));

	/**
	 * PkSubscription:id:
	 *
	 * The subscription identifier.
	 */
	g_object_class_install_property(object_class,
	                                PROP_ID,
	                                g_param_spec_int("id",
	                                                 "Id",
	                                                 "Id",
	                                                 G_MININT,
	                                                 G_MAXINT,
	                                                 G_MININT,
	                                                 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	/**
	 * PkSubscription:connection:
	 *
	 * The subscription connection.
	 */
	g_object_class_install_property(object_class,
	                                PROP_CONNECTION,
	                                g_param_spec_object("connection",
	                                                    "Connection",
	                                                    "Connection",
	                                                    PK_TYPE_CONNECTION,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
pk_subscription_init (PkSubscription *subscription) /* IN */
{
	subscription->priv = G_TYPE_INSTANCE_GET_PRIVATE(subscription, PK_TYPE_SUBSCRIPTION, PkSubscriptionPrivate);
}
