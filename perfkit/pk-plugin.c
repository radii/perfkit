/* pk-plugin.c
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

#include "pk-connection-lowlevel.h"
#include "pk-plugin.h"
#include "pk-util.h"
#include "string.h"

G_DEFINE_TYPE(PkPlugin, pk_plugin, G_TYPE_INITIALLY_UNOWNED)

struct _PkPluginPrivate
{
	PkConnection *connection;
	gchar* id;
};

/**
 * pk_plugin_finalize:
 * @object: A #PkPlugin.
 *
 * Finalizes an object after its reference count has reached zero.  Allocated
 * memory is released.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_plugin_finalize (GObject *object)
{
	G_OBJECT_CLASS(pk_plugin_parent_class)->finalize(object);
}

/**
 * pk_plugin_class_init:
 * @klass: A #PkPluginClass.
 *
 * Initializes the class vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_plugin_class_init (PkPluginClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pk_plugin_finalize;
	g_type_class_add_private(object_class, sizeof(PkPluginPrivate));
}

/**
 * pk_plugin_init:
 * @plugin: A #PkPlugin.
 *
 * Initializes a new instance of #PkPlugin.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_plugin_init (PkPlugin *plugin)
{
	plugin->priv = G_TYPE_INSTANCE_GET_PRIVATE(plugin,
	                                           PK_TYPE_PLUGIN,
	                                           PkPluginPrivate);
}

/**
 * pk_plugin_new:
 *
 * Creates a new instance of #PkPlugin.
 *
 * Returns: the newly created instance of #PkPlugin.
 * Side effects: None.
 */
PkPlugin*
pk_plugin_new_for_connection (PkConnection *connection) /* IN */
{
	return g_object_new(PK_TYPE_PLUGIN,
	                    "connection", connection,
	                    NULL);
}

/**
 * pk_plugin_get_name_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "plugin_Plugin" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_plugin_get_name_cb (GObject      *object,    /* IN */
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
 * pk_plugin_get_name_async:
 * @plugin: A #PkPlugin.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "plugin_get_name" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_plugin_get_name_async (PkPlugin            *plugin,      /* IN */
                          GCancellable        *cancellable, /* IN */
                          GAsyncReadyCallback  callback,    /* IN */
                          gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_PLUGIN(plugin));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(plugin, callback, user_data,
                    pk_plugin_get_name_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, plugin->priv->connection);
	pk_connection_plugin_get_name_async(plugin->priv->connection,
	                                    plugin->priv->id,
	                                    cancellable,
	                                    pk_plugin_get_name_cb,
	                                    res);
}

/**
 * pk_plugin_get_name_finish:
 * @plugin: A #PkPlugin.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "plugin_get_name" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_plugin_get_name_finish (PkPlugin      *plugin, /* IN */
                           GAsyncResult  *result, /* IN */
                           gchar        **name,   /* OUT */
                           GError       **error)  /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_PLUGIN(plugin), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, plugin,
	                                    pk_plugin_get_name_async),
	                     FALSE);

	ENTRY;
	*name = NULL;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_plugin_get_name_finish(plugin->priv->connection,
	                                           res,
	                                           name,
	                                           error);
	RETURN(ret);
}

/**
 * pk_plugin_get_description_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "plugin_Plugin" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_plugin_get_description_cb (GObject      *object,    /* IN */
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
 * pk_plugin_get_description_async:
 * @plugin: A #PkPlugin.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "plugin_get_description" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_plugin_get_description_async (PkPlugin            *plugin,      /* IN */
                                 GCancellable        *cancellable, /* IN */
                                 GAsyncReadyCallback  callback,    /* IN */
                                 gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_PLUGIN(plugin));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(plugin, callback, user_data,
                    pk_plugin_get_description_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, plugin->priv->connection);
	pk_connection_plugin_get_description_async(plugin->priv->connection,
	                                           plugin->priv->id,
	                                           cancellable,
	                                           pk_plugin_get_description_cb,
	                                           res);
}

/**
 * pk_plugin_get_description_finish:
 * @plugin: A #PkPlugin.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "plugin_get_description" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_plugin_get_description_finish (PkPlugin      *plugin,      /* IN */
                                  GAsyncResult  *result,      /* IN */
                                  gchar        **description, /* OUT */
                                  GError       **error)       /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_PLUGIN(plugin), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, plugin,
	                                    pk_plugin_get_description_async),
	                     FALSE);

	ENTRY;
	*description = NULL;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_plugin_get_description_finish(plugin->priv->connection,
	                                                  res,
	                                                  description,
	                                                  error);
	RETURN(ret);
}

/**
 * pk_plugin_get_version_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "plugin_Plugin" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_plugin_get_version_cb (GObject      *object,    /* IN */
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
 * pk_plugin_get_version_async:
 * @plugin: A #PkPlugin.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "plugin_get_version" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_plugin_get_version_async (PkPlugin            *plugin,      /* IN */
                             GCancellable        *cancellable, /* IN */
                             GAsyncReadyCallback  callback,    /* IN */
                             gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_PLUGIN(plugin));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(plugin, callback, user_data,
                    pk_plugin_get_version_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, plugin->priv->connection);
	pk_connection_plugin_get_version_async(plugin->priv->connection,
	                                       plugin->priv->id,
	                                       cancellable,
	                                       pk_plugin_get_version_cb,
	                                       res);
}

/**
 * pk_plugin_get_version_finish:
 * @plugin: A #PkPlugin.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "plugin_get_version" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_plugin_get_version_finish (PkPlugin      *plugin,  /* IN */
                              GAsyncResult  *result,  /* IN */
                              gchar        **version, /* OUT */
                              GError       **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_PLUGIN(plugin), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, plugin,
	                                    pk_plugin_get_version_async),
	                     FALSE);

	ENTRY;
	*version = NULL;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_plugin_get_version_finish(plugin->priv->connection,
	                                              res,
	                                              version,
	                                              error);
	RETURN(ret);
}

/**
 * pk_plugin_get_plugin_type_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "plugin_Plugin" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_plugin_get_plugin_type_cb (GObject      *object,    /* IN */
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
 * pk_plugin_get_plugin_type_async:
 * @plugin: A #PkPlugin.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "plugin_get_plugin_type" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_plugin_get_plugin_type_async (PkPlugin            *plugin,      /* IN */
                                 GCancellable        *cancellable, /* IN */
                                 GAsyncReadyCallback  callback,    /* IN */
                                 gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_PLUGIN(plugin));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);

	res = ASYNC_NEW(plugin, callback, user_data,
                    pk_plugin_get_plugin_type_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, plugin->priv->connection);
	pk_connection_plugin_get_plugin_type_async(plugin->priv->connection,
	                                           plugin->priv->id,
	                                           cancellable,
	                                           pk_plugin_get_plugin_type_cb,
	                                           res);
}

/**
 * pk_plugin_get_plugin_type_finish:
 * @plugin: A #PkPlugin.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "plugin_get_plugin_type" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_plugin_get_plugin_type_finish (PkPlugin      *plugin, /* IN */
                                  GAsyncResult  *result, /* IN */
                                  gint          *type,   /* OUT */
                                  GError       **error)  /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_PLUGIN(plugin), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, plugin,
	                                    pk_plugin_get_plugin_type_async),
	                     FALSE);

	ENTRY;
	*type = 0;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_plugin_get_plugin_type_finish(plugin->priv->connection,
	                                                  res,
	                                                  type,
	                                                  error);
	RETURN(ret);
}

