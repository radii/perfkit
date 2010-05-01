/* pk-subscription.c
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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
#include "pk-util.h"
#include "pk-connection-lowlevel.h"

G_DEFINE_TYPE(PkSubscription, pk_subscription, G_TYPE_INITIALLY_UNOWNED)

struct _PkSubscriptionPrivate
{
	PkConnection *connection;
	gint id;
};

/**
 * pk_subscription_finalize:
 * @object: A #PkSubscription.
 *
 * Finalizes an object after its reference count has reached zero.  Allocated
 * memory is released.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_subscription_finalize (GObject *object)
{
	G_OBJECT_CLASS(pk_subscription_parent_class)->finalize(object);
}

/**
 * pk_subscription_class_init:
 * @klass: A #PkSubscriptionClass.
 *
 * Initializes the class vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_subscription_class_init (PkSubscriptionClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pk_subscription_finalize;
	g_type_class_add_private(object_class, sizeof(PkSubscriptionPrivate));
}

/**
 * pk_subscription_init:
 * @subscription: A #PkSubscription.
 *
 * Initializes a new instance of #PkSubscription.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_subscription_init (PkSubscription *subscription)
{
	subscription->priv = G_TYPE_INSTANCE_GET_PRIVATE(subscription,
	                                                 PK_TYPE_SUBSCRIPTION,
	                                                 PkSubscriptionPrivate);
}

/**
 * pk_subscription_new:
 *
 * Creates a new instance of #PkSubscription.
 *
 * Returns: the newly created instance of #PkSubscription.
 * Side effects: None.
 */
PkSubscription*
pk_subscription_new_for_connection (PkConnection *connection) /* IN */
{
	return g_object_new(PK_TYPE_SUBSCRIPTION,
	                    "connection", connection,
	                    NULL);
}

/**
 * pk_subscription_enable_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "subscription_Subscription" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_subscription_enable_cb (GObject      *object,    /* IN */
                           GAsyncResult *result,    /* IN */
                           gpointer      user_data) /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(object));
	g_return_if_fail(G_IS_SIMPLE_ASYNC_RESULT(user_data));

	g_simple_async_result_set_op_res_gpointer(user_data,
	                                          g_object_ref(result),
	                                          (GDestroyNotify)g_object_unref);
	g_simple_async_result_complete(user_data);
}

/**
 * pk_subscription_enable_async:
 * @subscription: A #PkSubscription.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "subscription_enable" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_subscription_enable_async (PkSubscription      *subscription, /* IN */
                              GCancellable        *cancellable,  /* IN */
                              GAsyncReadyCallback  callback,     /* IN */
                              gpointer             user_data)    /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_SUBSCRIPTION(subscription));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(subscription, callback, user_data,
                    pk_subscription_enable_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, subscription->priv->connection);
	pk_connection_subscription_enable_async(subscription->priv->connection,
	                                        subscription->priv->id,
	                                        cancellable,
	                                        pk_subscription_enable_cb,
	                                        res);
}

/**
 * pk_subscription_enable_finish:
 * @subscription: A #PkSubscription.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "subscription_enable" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_subscription_enable_finish (PkSubscription  *subscription, /* IN */
                               GAsyncResult    *result,       /* IN */
                               GError         **error)        /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, subscription,
	                                    pk_subscription_enable_async),
	                     FALSE);

	ENTRY;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_subscription_enable_finish(subscription->priv->connection,
	                                               res,
	                                               error);
	RETURN(ret);
}

/**
 * pk_subscription_disable_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "subscription_Subscription" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_subscription_disable_cb (GObject      *object,    /* IN */
                            GAsyncResult *result,    /* IN */
                            gpointer      user_data) /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(object));
	g_return_if_fail(G_IS_SIMPLE_ASYNC_RESULT(user_data));

	g_simple_async_result_set_op_res_gpointer(user_data,
	                                          g_object_ref(result),
	                                          (GDestroyNotify)g_object_unref);
	g_simple_async_result_complete(user_data);
}

/**
 * pk_subscription_disable_async:
 * @subscription: A #PkSubscription.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "subscription_disable" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_subscription_disable_async (PkSubscription      *subscription, /* IN */
                               GCancellable        *cancellable,  /* IN */
                               GAsyncReadyCallback  callback,     /* IN */
                               gpointer             user_data)    /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_SUBSCRIPTION(subscription));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(subscription, callback, user_data,
                    pk_subscription_disable_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, subscription->priv->connection);
	pk_connection_subscription_disable_async(subscription->priv->connection,
	                                         subscription->priv->id,
	                                         cancellable,
	                                         pk_subscription_disable_cb,
	                                         res);
}

/**
 * pk_subscription_disable_finish:
 * @subscription: A #PkSubscription.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "subscription_disable" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_subscription_disable_finish (PkSubscription  *subscription, /* IN */
                                GAsyncResult    *result,       /* IN */
                                GError         **error)        /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, subscription,
	                                    pk_subscription_disable_async),
	                     FALSE);

	ENTRY;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_subscription_disable_finish(subscription->priv->connection,
	                                                res,
	                                                error);
	RETURN(ret);
}

/**
 * pk_subscription_set_handlers_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "subscription_Subscription" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_subscription_set_handlers_cb (GObject      *object,    /* IN */
                                 GAsyncResult *result,    /* IN */
                                 gpointer      user_data) /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(object));
	g_return_if_fail(G_IS_SIMPLE_ASYNC_RESULT(user_data));

	g_simple_async_result_set_op_res_gpointer(user_data,
	                                          g_object_ref(result),
	                                          (GDestroyNotify)g_object_unref);
	g_simple_async_result_complete(user_data);
}

/**
 * pk_subscription_set_handlers_async:
 * @subscription: A #PkSubscription.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "subscription_set_handlers" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_subscription_set_handlers_async (PkSubscription      *subscription, /* IN */
                                    GFunc                manifest,     /* IN */
                                    GFunc                sample,       /* IN */
                                    GCancellable        *cancellable,  /* IN */
                                    GAsyncReadyCallback  callback,     /* IN */
                                    gpointer             user_data)    /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_SUBSCRIPTION(subscription));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(subscription, callback, user_data,
                    pk_subscription_set_handlers_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, subscription->priv->connection);
	pk_connection_subscription_set_handlers_async(subscription->priv->connection,
	                                              subscription->priv->id,
	                                              manifest,
	                                              sample,
	                                              cancellable,
	                                              pk_subscription_set_handlers_cb,
	                                              res);
}

/**
 * pk_subscription_set_handlers_finish:
 * @subscription: A #PkSubscription.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "subscription_set_handlers" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_subscription_set_handlers_finish (PkSubscription  *subscription, /* IN */
                                     GAsyncResult    *result,       /* IN */
                                     GError         **error)        /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, subscription,
	                                    pk_subscription_set_handlers_async),
	                     FALSE);

	ENTRY;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_subscription_set_handlers_finish(subscription->priv->connection,
	                                                     res,
	                                                     error);
	RETURN(ret);
}

/**
 * pk_subscription_get_encoder_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "subscription_Subscription" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_subscription_get_encoder_cb (GObject      *object,    /* IN */
                                GAsyncResult *result,    /* IN */
                                gpointer      user_data) /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(object));
	g_return_if_fail(G_IS_SIMPLE_ASYNC_RESULT(user_data));

	g_simple_async_result_set_op_res_gpointer(user_data,
	                                          g_object_ref(result),
	                                          (GDestroyNotify)g_object_unref);
	g_simple_async_result_complete(user_data);
}

/**
 * pk_subscription_get_encoder_async:
 * @subscription: A #PkSubscription.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "subscription_get_encoder" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_subscription_get_encoder_async (PkSubscription      *subscription, /* IN */
                                   GCancellable        *cancellable,  /* IN */
                                   GAsyncReadyCallback  callback,     /* IN */
                                   gpointer             user_data)    /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_SUBSCRIPTION(subscription));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(subscription, callback, user_data,
                    pk_subscription_get_encoder_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, subscription->priv->connection);
	pk_connection_subscription_get_encoder_async(subscription->priv->connection,
	                                             subscription->priv->id,
	                                             cancellable,
	                                             pk_subscription_get_encoder_cb,
	                                             res);
}

/**
 * pk_subscription_get_encoder_finish:
 * @subscription: A #PkSubscription.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "subscription_get_encoder" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_subscription_get_encoder_finish (PkSubscription  *subscription, /* IN */
                                    GAsyncResult    *result,       /* IN */
                                    gint            *encoder,      /* OUT */
                                    GError         **error)        /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, subscription,
	                                    pk_subscription_get_encoder_async),
	                     FALSE);

	ENTRY;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_subscription_get_encoder_finish(subscription->priv->connection,
	                                                    res,
	                                                    encoder,
	                                                    error);
	RETURN(ret);
}

