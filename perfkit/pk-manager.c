/* pk-manager.c
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
#include "pk-manager.h"
#include "pk-plugin.h"
#include "pk-util.h"
#include "string.h"

G_DEFINE_TYPE(PkManager, pk_manager, G_TYPE_INITIALLY_UNOWNED)

struct _PkManagerPrivate
{
	PkConnection *connection;
};

/**
 * pk_manager_finalize:
 * @object: A #PkManager.
 *
 * Finalizes an object after its reference count has reached zero.  Allocated
 * memory is released.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_manager_finalize (GObject *object)
{
	G_OBJECT_CLASS(pk_manager_parent_class)->finalize(object);
}

/**
 * pk_manager_class_init:
 * @klass: A #PkManagerClass.
 *
 * Initializes the class vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_manager_class_init (PkManagerClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pk_manager_finalize;
	g_type_class_add_private(object_class, sizeof(PkManagerPrivate));
}

/**
 * pk_manager_init:
 * @manager: A #PkManager.
 *
 * Initializes a new instance of #PkManager.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_manager_init (PkManager *manager)
{
	manager->priv = G_TYPE_INSTANCE_GET_PRIVATE(manager,
	                                            PK_TYPE_MANAGER,
	                                            PkManagerPrivate);
}

/**
 * pk_manager_new:
 *
 * Creates a new instance of #PkManager.
 *
 * Returns: the newly created instance of #PkManager.
 * Side effects: None.
 */
PkManager*
pk_manager_new_for_connection (PkConnection *connection) /* IN */
{
	return g_object_new(PK_TYPE_MANAGER,
	                    "connection", connection,
	                    NULL);
}

/**
 * pk_manager_get_channels_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "manager_Manager" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_manager_get_channels_cb (GObject      *object,    /* IN */
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
 * pk_manager_get_channels_async:
 * @manager: A #PkManager.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "manager_get_channels" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_manager_get_channels_async (PkManager           *manager,     /* IN */
                               GCancellable        *cancellable, /* IN */
                               GAsyncReadyCallback  callback,    /* IN */
                               gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_MANAGER(manager));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(manager, callback, user_data,
                    pk_manager_get_channels_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, manager->priv->connection);
	pk_connection_manager_get_channels_async(manager->priv->connection,
	                                         cancellable,
	                                         pk_manager_get_channels_cb,
	                                         res);
}

/**
 * pk_manager_get_channels_finish:
 * @manager: A #PkManager.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "manager_get_channels" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_manager_get_channels_finish (PkManager     *manager,  /* IN */
                                GAsyncResult  *result,   /* IN */
                                GList        **channels, /* OUT */
                                GError       **error)    /* OUT */
{
	GAsyncResult *res;
	gboolean ret;
	gint *p_channels = NULL;
	gsize p_channels_len = 0;
	gint i;

	g_return_val_if_fail(PK_IS_MANAGER(manager), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, manager,
	                                    pk_manager_get_channels_async),
	                     FALSE);
	g_return_val_if_fail(channels != NULL, FALSE);

	ENTRY;
	*channels = NULL;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_manager_get_channels_finish(manager->priv->connection,
	                                                res,
	                                                &p_channels,
	                                                &p_channels_len,
	                                                error);
	if (ret) {
		for (i = 0; p_channels[i]; i++) {
			(*channels) = g_list_append((*channels),
			                            g_object_new(PK_TYPE_CHANNEL,
			                                         "connection", manager->priv->connection,
			                                         "id", p_channels[i],
			                                         NULL));
		}
		g_free(p_channels);
	}
	RETURN(ret);
}

/**
 * pk_manager_get_source_plugins_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "manager_Manager" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_manager_get_source_plugins_cb (GObject      *object,    /* IN */
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
 * pk_manager_get_source_plugins_async:
 * @manager: A #PkManager.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "manager_get_source_plugins" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_manager_get_source_plugins_async (PkManager           *manager,     /* IN */
                                     GCancellable        *cancellable, /* IN */
                                     GAsyncReadyCallback  callback,    /* IN */
                                     gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_MANAGER(manager));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(manager, callback, user_data,
                    pk_manager_get_source_plugins_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, manager->priv->connection);
	pk_connection_manager_get_source_plugins_async(manager->priv->connection,
	                                               cancellable,
	                                               pk_manager_get_source_plugins_cb,
	                                               res);
}

/**
 * pk_manager_get_source_plugins_finish:
 * @manager: A #PkManager.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "manager_get_source_plugins" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_manager_get_source_plugins_finish (PkManager     *manager, /* IN */
                                      GAsyncResult  *result,  /* IN */
                                      GList        **plugins, /* OUT */
                                      GError       **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;
	gchar **p_plugins = NULL;
	gint i;

	g_return_val_if_fail(PK_IS_MANAGER(manager), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, manager,
	                                    pk_manager_get_source_plugins_async),
	                     FALSE);
	g_return_val_if_fail(plugins != NULL, FALSE);

	ENTRY;
	*plugins = NULL;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_manager_get_source_plugins_finish(manager->priv->connection,
	                                                      res,
	                                                      &p_plugins,
	                                                      error);
	if (ret) {
		for (i = 0; p_plugins[i]; i++) {
			(*plugins) = g_list_append((*plugins),
			                           g_object_new(PK_TYPE_PLUGIN,
			                                        "connection", manager->priv->connection,
			                                        "id", p_plugins[i],
			                                        NULL));
		}
		g_free(p_plugins);
	}
	RETURN(ret);
}

/**
 * pk_manager_get_version_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "manager_Manager" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_manager_get_version_cb (GObject      *object,    /* IN */
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
 * pk_manager_get_version_async:
 * @manager: A #PkManager.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "manager_get_version" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_manager_get_version_async (PkManager           *manager,     /* IN */
                              GCancellable        *cancellable, /* IN */
                              GAsyncReadyCallback  callback,    /* IN */
                              gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_MANAGER(manager));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(manager, callback, user_data,
                    pk_manager_get_version_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, manager->priv->connection);
	pk_connection_manager_get_version_async(manager->priv->connection,
	                                        cancellable,
	                                        pk_manager_get_version_cb,
	                                        res);
}

/**
 * pk_manager_get_version_finish:
 * @manager: A #PkManager.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "manager_get_version" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_manager_get_version_finish (PkManager     *manager, /* IN */
                               GAsyncResult  *result,  /* IN */
                               gchar        **version, /* OUT */
                               GError       **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_MANAGER(manager), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, manager,
	                                    pk_manager_get_version_async),
	                     FALSE);

	ENTRY;
	*version = NULL;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_manager_get_version_finish(manager->priv->connection,
	                                               res,
	                                               version,
	                                               error);
	RETURN(ret);
}

/**
 * pk_manager_ping_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "manager_Manager" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_manager_ping_cb (GObject      *object,    /* IN */
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
 * pk_manager_ping_async:
 * @manager: A #PkManager.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "manager_ping" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_manager_ping_async (PkManager           *manager,     /* IN */
                       GCancellable        *cancellable, /* IN */
                       GAsyncReadyCallback  callback,    /* IN */
                       gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_MANAGER(manager));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(manager, callback, user_data,
                    pk_manager_ping_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, manager->priv->connection);
	pk_connection_manager_ping_async(manager->priv->connection,
	                                 cancellable,
	                                 pk_manager_ping_cb,
	                                 res);
}

/**
 * pk_manager_ping_finish:
 * @manager: A #PkManager.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "manager_ping" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_manager_ping_finish (PkManager     *manager, /* IN */
                        GAsyncResult  *result,  /* IN */
                        GTimeVal      *tv,      /* OUT */
                        GError       **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_MANAGER(manager), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, manager,
	                                    pk_manager_ping_async),
	                     FALSE);

	ENTRY;
	memset(tv, 0, sizeof(*tv));
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_manager_ping_finish(manager->priv->connection,
	                                        res,
	                                        tv,
	                                        error);
	RETURN(ret);
}

/**
 * pk_manager_add_channel_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "manager_Manager" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_manager_add_channel_cb (GObject      *object,    /* IN */
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
 * pk_manager_add_channel_async:
 * @manager: A #PkManager.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "manager_add_channel" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_manager_add_channel_async (PkManager           *manager,     /* IN */
                              const PkSpawnInfo   *spawn_info,  /* IN */
                              GCancellable        *cancellable, /* IN */
                              GAsyncReadyCallback  callback,    /* IN */
                              gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_MANAGER(manager));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(manager, callback, user_data,
                    pk_manager_add_channel_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, manager->priv->connection);
	pk_connection_manager_add_channel_async(manager->priv->connection,
	                                        spawn_info,
	                                        cancellable,
	                                        pk_manager_add_channel_cb,
	                                        res);
}

/**
 * pk_manager_add_channel_finish:
 * @manager: A #PkManager.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "manager_add_channel" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_manager_add_channel_finish (PkManager     *manager, /* IN */
                               GAsyncResult  *result,  /* IN */
                               gint          *channel, /* OUT */
                               GError       **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_MANAGER(manager), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, manager,
	                                    pk_manager_add_channel_async),
	                     FALSE);

	ENTRY;
	*channel = 0;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_manager_add_channel_finish(manager->priv->connection,
	                                               res,
	                                               channel,
	                                               error);
	RETURN(ret);
}

/**
 * pk_manager_remove_channel_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "manager_Manager" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_manager_remove_channel_cb (GObject      *object,    /* IN */
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
 * pk_manager_remove_channel_async:
 * @manager: A #PkManager.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "manager_remove_channel" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_manager_remove_channel_async (PkManager           *manager,     /* IN */
                                 gint                 channel,     /* IN */
                                 GCancellable        *cancellable, /* IN */
                                 GAsyncReadyCallback  callback,    /* IN */
                                 gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_MANAGER(manager));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(manager, callback, user_data,
                    pk_manager_remove_channel_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, manager->priv->connection);
	pk_connection_manager_remove_channel_async(manager->priv->connection,
	                                           channel,
	                                           cancellable,
	                                           pk_manager_remove_channel_cb,
	                                           res);
}

/**
 * pk_manager_remove_channel_finish:
 * @manager: A #PkManager.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "manager_remove_channel" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_manager_remove_channel_finish (PkManager     *manager, /* IN */
                                  GAsyncResult  *result,  /* IN */
                                  GError       **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_MANAGER(manager), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, manager,
	                                    pk_manager_remove_channel_async),
	                     FALSE);

	ENTRY;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_manager_remove_channel_finish(manager->priv->connection,
	                                                  res,
	                                                  error);
	RETURN(ret);
}

/**
 * pk_manager_add_subscription_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "manager_Manager" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_manager_add_subscription_cb (GObject      *object,    /* IN */
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
 * pk_manager_add_subscription_async:
 * @manager: A #PkManager.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "manager_add_subscription" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_manager_add_subscription_async (PkManager           *manager,        /* IN */
                                   gint                 channel,        /* IN */
                                   gsize                buffer_size,    /* IN */
                                   gulong               buffer_timeout, /* IN */
                                   const gchar         *encoder,        /* IN */
                                   GCancellable        *cancellable,    /* IN */
                                   GAsyncReadyCallback  callback,       /* IN */
                                   gpointer             user_data)      /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_MANAGER(manager));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);
	g_return_if_fail(encoder != NULL);

	res = ASYNC_NEW(manager, callback, user_data,
                    pk_manager_add_subscription_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, manager->priv->connection);
	pk_connection_manager_add_subscription_async(manager->priv->connection,
	                                             channel,
	                                             buffer_size,
	                                             buffer_timeout,
	                                             encoder,
	                                             cancellable,
	                                             pk_manager_add_subscription_cb,
	                                             res);
}

/**
 * pk_manager_add_subscription_finish:
 * @manager: A #PkManager.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "manager_add_subscription" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_manager_add_subscription_finish (PkManager     *manager,      /* IN */
                                    GAsyncResult  *result,       /* IN */
                                    gint          *subscription, /* OUT */
                                    GError       **error)        /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_MANAGER(manager), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, manager,
	                                    pk_manager_add_subscription_async),
	                     FALSE);

	ENTRY;
	*subscription = 0;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_manager_add_subscription_finish(manager->priv->connection,
	                                                    res,
	                                                    subscription,
	                                                    error);
	RETURN(ret);
}

/**
 * pk_manager_remove_subscription_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "manager_Manager" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_manager_remove_subscription_cb (GObject      *object,    /* IN */
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
 * pk_manager_remove_subscription_async:
 * @manager: A #PkManager.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "manager_remove_subscription" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_manager_remove_subscription_async (PkManager           *manager,      /* IN */
                                      gint                 subscription, /* IN */
                                      GCancellable        *cancellable,  /* IN */
                                      GAsyncReadyCallback  callback,     /* IN */
                                      gpointer             user_data)    /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_MANAGER(manager));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(manager, callback, user_data,
                    pk_manager_remove_subscription_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, manager->priv->connection);
	pk_connection_manager_remove_subscription_async(manager->priv->connection,
	                                                subscription,
	                                                cancellable,
	                                                pk_manager_remove_subscription_cb,
	                                                res);
}

/**
 * pk_manager_remove_subscription_finish:
 * @manager: A #PkManager.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "manager_remove_subscription" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_manager_remove_subscription_finish (PkManager     *manager, /* IN */
                                       GAsyncResult  *result,  /* IN */
                                       GError       **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_MANAGER(manager), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, manager,
	                                    pk_manager_remove_subscription_async),
	                     FALSE);

	ENTRY;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_manager_remove_subscription_finish(manager->priv->connection,
	                                                       res,
	                                                       error);
	RETURN(ret);
}

