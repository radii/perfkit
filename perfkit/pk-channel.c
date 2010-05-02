/* pk-channel.c
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

#include "pk-channel.h"
#include "pk-connection-lowlevel.h"
#include "pk-source.h"
#include "pk-util.h"
#include "string.h"

G_DEFINE_TYPE(PkChannel, pk_channel, G_TYPE_INITIALLY_UNOWNED)

struct _PkChannelPrivate
{
	PkConnection *connection;
	gint id;
};

/**
 * pk_channel_finalize:
 * @object: A #PkChannel.
 *
 * Finalizes an object after its reference count has reached zero.  Allocated
 * memory is released.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_channel_finalize (GObject *object)
{
	G_OBJECT_CLASS(pk_channel_parent_class)->finalize(object);
}

/**
 * pk_channel_class_init:
 * @klass: A #PkChannelClass.
 *
 * Initializes the class vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_channel_class_init (PkChannelClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pk_channel_finalize;
	g_type_class_add_private(object_class, sizeof(PkChannelPrivate));
}

/**
 * pk_channel_init:
 * @channel: A #PkChannel.
 *
 * Initializes a new instance of #PkChannel.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_channel_init (PkChannel *channel)
{
	channel->priv = G_TYPE_INSTANCE_GET_PRIVATE(channel,
	                                            PK_TYPE_CHANNEL,
	                                            PkChannelPrivate);
}

/**
 * pk_channel_new:
 *
 * Creates a new instance of #PkChannel.
 *
 * Returns: the newly created instance of #PkChannel.
 * Side effects: None.
 */
PkChannel*
pk_channel_new_for_connection (PkConnection *connection) /* IN */
{
	return g_object_new(PK_TYPE_CHANNEL,
	                    "connection", connection,
	                    NULL);
}

/**
 * pk_channel_get_args_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "channel_Channel" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_channel_get_args_cb (GObject      *object,    /* IN */
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
 * pk_channel_get_args_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "channel_get_args" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_get_args_async (PkChannel           *channel,     /* IN */
                           GCancellable        *cancellable, /* IN */
                           GAsyncReadyCallback  callback,    /* IN */
                           gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_CHANNEL(channel));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(channel, callback, user_data,
                    pk_channel_get_args_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, channel->priv->connection);
	pk_connection_channel_get_args_async(channel->priv->connection,
	                                     channel->priv->id,
	                                     cancellable,
	                                     pk_channel_get_args_cb,
	                                     res);
}

/**
 * pk_channel_get_args_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "channel_get_args" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_channel_get_args_finish (PkChannel      *channel, /* IN */
                            GAsyncResult   *result,  /* IN */
                            gchar        ***args,    /* OUT */
                            GError        **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, channel,
	                                    pk_channel_get_args_async),
	                     FALSE);

	ENTRY;
	*args = NULL;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_channel_get_args_finish(channel->priv->connection,
	                                            res,
	                                            args,
	                                            error);
	RETURN(ret);
}

/**
 * pk_channel_get_env_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "channel_Channel" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_channel_get_env_cb (GObject      *object,    /* IN */
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
 * pk_channel_get_env_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "channel_get_env" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_get_env_async (PkChannel           *channel,     /* IN */
                          GCancellable        *cancellable, /* IN */
                          GAsyncReadyCallback  callback,    /* IN */
                          gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_CHANNEL(channel));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(channel, callback, user_data,
                    pk_channel_get_env_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, channel->priv->connection);
	pk_connection_channel_get_env_async(channel->priv->connection,
	                                    channel->priv->id,
	                                    cancellable,
	                                    pk_channel_get_env_cb,
	                                    res);
}

/**
 * pk_channel_get_env_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "channel_get_env" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_channel_get_env_finish (PkChannel      *channel, /* IN */
                           GAsyncResult   *result,  /* IN */
                           gchar        ***env,     /* OUT */
                           GError        **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, channel,
	                                    pk_channel_get_env_async),
	                     FALSE);

	ENTRY;
	*env = NULL;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_channel_get_env_finish(channel->priv->connection,
	                                           res,
	                                           env,
	                                           error);
	RETURN(ret);
}

/**
 * pk_channel_get_pid_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "channel_Channel" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_channel_get_pid_cb (GObject      *object,    /* IN */
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
 * pk_channel_get_pid_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "channel_get_pid" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_get_pid_async (PkChannel           *channel,     /* IN */
                          GCancellable        *cancellable, /* IN */
                          GAsyncReadyCallback  callback,    /* IN */
                          gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_CHANNEL(channel));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(channel, callback, user_data,
                    pk_channel_get_pid_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, channel->priv->connection);
	pk_connection_channel_get_pid_async(channel->priv->connection,
	                                    channel->priv->id,
	                                    cancellable,
	                                    pk_channel_get_pid_cb,
	                                    res);
}

/**
 * pk_channel_get_pid_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "channel_get_pid" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_channel_get_pid_finish (PkChannel     *channel, /* IN */
                           GAsyncResult  *result,  /* IN */
                           GPid          *pid,     /* OUT */
                           GError       **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, channel,
	                                    pk_channel_get_pid_async),
	                     FALSE);

	ENTRY;
	*pid = 0;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_channel_get_pid_finish(channel->priv->connection,
	                                           res,
	                                           pid,
	                                           error);
	RETURN(ret);
}

/**
 * pk_channel_get_state_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "channel_Channel" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_channel_get_state_cb (GObject      *object,    /* IN */
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
 * pk_channel_get_state_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "channel_get_state" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_get_state_async (PkChannel           *channel,     /* IN */
                            GCancellable        *cancellable, /* IN */
                            GAsyncReadyCallback  callback,    /* IN */
                            gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_CHANNEL(channel));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(channel, callback, user_data,
                    pk_channel_get_state_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, channel->priv->connection);
	pk_connection_channel_get_state_async(channel->priv->connection,
	                                      channel->priv->id,
	                                      cancellable,
	                                      pk_channel_get_state_cb,
	                                      res);
}

/**
 * pk_channel_get_state_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "channel_get_state" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_channel_get_state_finish (PkChannel     *channel, /* IN */
                             GAsyncResult  *result,  /* IN */
                             gint          *state,   /* OUT */
                             GError       **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, channel,
	                                    pk_channel_get_state_async),
	                     FALSE);

	ENTRY;
	*state = 0;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_channel_get_state_finish(channel->priv->connection,
	                                             res,
	                                             state,
	                                             error);
	RETURN(ret);
}

/**
 * pk_channel_get_target_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "channel_Channel" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_channel_get_target_cb (GObject      *object,    /* IN */
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
 * pk_channel_get_target_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "channel_get_target" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_get_target_async (PkChannel           *channel,     /* IN */
                             GCancellable        *cancellable, /* IN */
                             GAsyncReadyCallback  callback,    /* IN */
                             gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_CHANNEL(channel));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(channel, callback, user_data,
                    pk_channel_get_target_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, channel->priv->connection);
	pk_connection_channel_get_target_async(channel->priv->connection,
	                                       channel->priv->id,
	                                       cancellable,
	                                       pk_channel_get_target_cb,
	                                       res);
}

/**
 * pk_channel_get_target_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "channel_get_target" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_channel_get_target_finish (PkChannel     *channel, /* IN */
                              GAsyncResult  *result,  /* IN */
                              gchar        **target,  /* OUT */
                              GError       **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, channel,
	                                    pk_channel_get_target_async),
	                     FALSE);

	ENTRY;
	*target = NULL;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_channel_get_target_finish(channel->priv->connection,
	                                              res,
	                                              target,
	                                              error);
	RETURN(ret);
}

/**
 * pk_channel_get_working_dir_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "channel_Channel" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_channel_get_working_dir_cb (GObject      *object,    /* IN */
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
 * pk_channel_get_working_dir_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "channel_get_working_dir" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_get_working_dir_async (PkChannel           *channel,     /* IN */
                                  GCancellable        *cancellable, /* IN */
                                  GAsyncReadyCallback  callback,    /* IN */
                                  gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_CHANNEL(channel));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(channel, callback, user_data,
                    pk_channel_get_working_dir_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, channel->priv->connection);
	pk_connection_channel_get_working_dir_async(channel->priv->connection,
	                                            channel->priv->id,
	                                            cancellable,
	                                            pk_channel_get_working_dir_cb,
	                                            res);
}

/**
 * pk_channel_get_working_dir_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "channel_get_working_dir" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_channel_get_working_dir_finish (PkChannel     *channel,     /* IN */
                                   GAsyncResult  *result,      /* IN */
                                   gchar        **working_dir, /* OUT */
                                   GError       **error)       /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, channel,
	                                    pk_channel_get_working_dir_async),
	                     FALSE);

	ENTRY;
	*working_dir = NULL;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_channel_get_working_dir_finish(channel->priv->connection,
	                                                   res,
	                                                   working_dir,
	                                                   error);
	RETURN(ret);
}

/**
 * pk_channel_get_sources_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "channel_Channel" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_channel_get_sources_cb (GObject      *object,    /* IN */
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
 * pk_channel_get_sources_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "channel_get_sources" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_get_sources_async (PkChannel           *channel,     /* IN */
                              GCancellable        *cancellable, /* IN */
                              GAsyncReadyCallback  callback,    /* IN */
                              gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_CHANNEL(channel));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(channel, callback, user_data,
                    pk_channel_get_sources_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, channel->priv->connection);
	pk_connection_channel_get_sources_async(channel->priv->connection,
	                                        channel->priv->id,
	                                        cancellable,
	                                        pk_channel_get_sources_cb,
	                                        res);
}

/**
 * pk_channel_get_sources_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "channel_get_sources" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_channel_get_sources_finish (PkChannel     *channel, /* IN */
                               GAsyncResult  *result,  /* IN */
                               GList        **sources, /* OUT */
                               GError       **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;
	gint *p_sources = NULL;
	gsize p_sources_len = 0;
	gint i;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, channel,
	                                    pk_channel_get_sources_async),
	                     FALSE);
	g_return_val_if_fail(sources != NULL, FALSE);

	ENTRY;
	*sources = NULL;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_channel_get_sources_finish(channel->priv->connection,
	                                               res,
	                                               &p_sources,
	                                               &p_sources_len,
	                                               error);
	if (ret) {
		for (i = 0; p_sources[i]; i++) {
			(*sources) = g_list_append((*sources),
			                           g_object_new(PK_TYPE_SOURCE,
			                                        "connection", channel->priv->connection,
			                                        "id", p_sources[i],
			                                        NULL));
		}
		g_free(p_sources);
	}
	RETURN(ret);
}

/**
 * pk_channel_add_source_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "channel_Channel" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_channel_add_source_cb (GObject      *object,    /* IN */
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
 * pk_channel_add_source_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "channel_add_source" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_add_source_async (PkChannel           *channel,     /* IN */
                             const gchar         *plugin,      /* IN */
                             GCancellable        *cancellable, /* IN */
                             GAsyncReadyCallback  callback,    /* IN */
                             gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_CHANNEL(channel));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);
	g_return_if_fail(plugin != NULL);

	res = ASYNC_NEW(channel, callback, user_data,
                    pk_channel_add_source_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, channel->priv->connection);
	pk_connection_channel_add_source_async(channel->priv->connection,
	                                       channel->priv->id,
	                                       plugin,
	                                       cancellable,
	                                       pk_channel_add_source_cb,
	                                       res);
}

/**
 * pk_channel_add_source_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "channel_add_source" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_channel_add_source_finish (PkChannel     *channel, /* IN */
                              GAsyncResult  *result,  /* IN */
                              gint          *source,  /* OUT */
                              GError       **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, channel,
	                                    pk_channel_add_source_async),
	                     FALSE);

	ENTRY;
	*source = 0;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_channel_add_source_finish(channel->priv->connection,
	                                              res,
	                                              source,
	                                              error);
	RETURN(ret);
}

/**
 * pk_channel_remove_source_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "channel_Channel" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_channel_remove_source_cb (GObject      *object,    /* IN */
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
 * pk_channel_remove_source_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "channel_remove_source" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_remove_source_async (PkChannel           *channel,     /* IN */
                                gint                 source,      /* IN */
                                GCancellable        *cancellable, /* IN */
                                GAsyncReadyCallback  callback,    /* IN */
                                gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_CHANNEL(channel));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(channel, callback, user_data,
                    pk_channel_remove_source_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, channel->priv->connection);
	pk_connection_channel_remove_source_async(channel->priv->connection,
	                                          channel->priv->id,
	                                          source,
	                                          cancellable,
	                                          pk_channel_remove_source_cb,
	                                          res);
}

/**
 * pk_channel_remove_source_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "channel_remove_source" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_channel_remove_source_finish (PkChannel     *channel, /* IN */
                                 GAsyncResult  *result,  /* IN */
                                 GError       **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, channel,
	                                    pk_channel_remove_source_async),
	                     FALSE);

	ENTRY;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_channel_remove_source_finish(channel->priv->connection,
	                                                 res,
	                                                 error);
	RETURN(ret);
}

/**
 * pk_channel_start_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "channel_Channel" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_channel_start_cb (GObject      *object,    /* IN */
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
 * pk_channel_start_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "channel_start" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_start_async (PkChannel           *channel,     /* IN */
                        GCancellable        *cancellable, /* IN */
                        GAsyncReadyCallback  callback,    /* IN */
                        gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_CHANNEL(channel));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(channel, callback, user_data,
                    pk_channel_start_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, channel->priv->connection);
	pk_connection_channel_start_async(channel->priv->connection,
	                                  channel->priv->id,
	                                  cancellable,
	                                  pk_channel_start_cb,
	                                  res);
}

/**
 * pk_channel_start_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "channel_start" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_channel_start_finish (PkChannel     *channel, /* IN */
                         GAsyncResult  *result,  /* IN */
                         GError       **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, channel,
	                                    pk_channel_start_async),
	                     FALSE);

	ENTRY;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_channel_start_finish(channel->priv->connection,
	                                         res,
	                                         error);
	RETURN(ret);
}

/**
 * pk_channel_stop_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "channel_Channel" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_channel_stop_cb (GObject      *object,    /* IN */
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
 * pk_channel_stop_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "channel_stop" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_stop_async (PkChannel           *channel,     /* IN */
                       gboolean             killpid,     /* IN */
                       GCancellable        *cancellable, /* IN */
                       GAsyncReadyCallback  callback,    /* IN */
                       gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_CHANNEL(channel));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(channel, callback, user_data,
                    pk_channel_stop_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, channel->priv->connection);
	pk_connection_channel_stop_async(channel->priv->connection,
	                                 channel->priv->id,
	                                 killpid,
	                                 cancellable,
	                                 pk_channel_stop_cb,
	                                 res);
}

/**
 * pk_channel_stop_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "channel_stop" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_channel_stop_finish (PkChannel     *channel, /* IN */
                        GAsyncResult  *result,  /* IN */
                        GError       **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, channel,
	                                    pk_channel_stop_async),
	                     FALSE);

	ENTRY;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_channel_stop_finish(channel->priv->connection,
	                                        res,
	                                        error);
	RETURN(ret);
}

/**
 * pk_channel_pause_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "channel_Channel" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_channel_pause_cb (GObject      *object,    /* IN */
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
 * pk_channel_pause_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "channel_pause" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_pause_async (PkChannel           *channel,     /* IN */
                        GCancellable        *cancellable, /* IN */
                        GAsyncReadyCallback  callback,    /* IN */
                        gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_CHANNEL(channel));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(channel, callback, user_data,
                    pk_channel_pause_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, channel->priv->connection);
	pk_connection_channel_pause_async(channel->priv->connection,
	                                  channel->priv->id,
	                                  cancellable,
	                                  pk_channel_pause_cb,
	                                  res);
}

/**
 * pk_channel_pause_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "channel_pause" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_channel_pause_finish (PkChannel     *channel, /* IN */
                         GAsyncResult  *result,  /* IN */
                         GError       **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, channel,
	                                    pk_channel_pause_async),
	                     FALSE);

	ENTRY;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_channel_pause_finish(channel->priv->connection,
	                                         res,
	                                         error);
	RETURN(ret);
}

/**
 * pk_channel_unpause_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "channel_Channel" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_channel_unpause_cb (GObject      *object,    /* IN */
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
 * pk_channel_unpause_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "channel_unpause" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_unpause_async (PkChannel           *channel,     /* IN */
                          GCancellable        *cancellable, /* IN */
                          GAsyncReadyCallback  callback,    /* IN */
                          gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_CHANNEL(channel));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(channel, callback, user_data,
                    pk_channel_unpause_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, channel->priv->connection);
	pk_connection_channel_unpause_async(channel->priv->connection,
	                                    channel->priv->id,
	                                    cancellable,
	                                    pk_channel_unpause_cb,
	                                    res);
}

/**
 * pk_channel_unpause_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "channel_unpause" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_channel_unpause_finish (PkChannel     *channel, /* IN */
                           GAsyncResult  *result,  /* IN */
                           GError       **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, channel,
	                                    pk_channel_unpause_async),
	                     FALSE);

	ENTRY;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_channel_unpause_finish(channel->priv->connection,
	                                           res,
	                                           error);
	RETURN(ret);
}

