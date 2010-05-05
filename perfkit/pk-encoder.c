/* pk-encoder.c
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
#include "pk-encoder.h"
#include "pk-util.h"
#include "string.h"

G_DEFINE_TYPE(PkEncoder, pk_encoder, G_TYPE_INITIALLY_UNOWNED)

struct _PkEncoderPrivate
{
	PkConnection *connection;
	gint id;
};

/**
 * pk_encoder_finalize:
 * @object: A #PkEncoder.
 *
 * Finalizes an object after its reference count has reached zero.  Allocated
 * memory is released.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_encoder_finalize (GObject *object)
{
	G_OBJECT_CLASS(pk_encoder_parent_class)->finalize(object);
}

/**
 * pk_encoder_class_init:
 * @klass: A #PkEncoderClass.
 *
 * Initializes the class vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_encoder_class_init (PkEncoderClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pk_encoder_finalize;
	g_type_class_add_private(object_class, sizeof(PkEncoderPrivate));
}

/**
 * pk_encoder_init:
 * @encoder: A #PkEncoder.
 *
 * Initializes a new instance of #PkEncoder.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_encoder_init (PkEncoder *encoder)
{
	encoder->priv = G_TYPE_INSTANCE_GET_PRIVATE(encoder,
	                                            PK_TYPE_ENCODER,
	                                            PkEncoderPrivate);
}

/**
 * pk_encoder_new:
 *
 * Creates a new instance of #PkEncoder.
 *
 * Returns: the newly created instance of #PkEncoder.
 * Side effects: None.
 */
PkEncoder*
pk_encoder_new_for_connection (PkConnection *connection) /* IN */
{
	return g_object_new(PK_TYPE_ENCODER,
	                    "connection", connection,
	                    NULL);
}

/**
 * pk_encoder_set_property_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "encoder_Encoder" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_encoder_set_property_cb (GObject      *object,    /* IN */
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
 * pk_encoder_set_property_async:
 * @encoder: A #PkEncoder.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "encoder_set_property" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_encoder_set_property_async (PkEncoder           *encoder,     /* IN */
                               const gchar         *name,        /* IN */
                               const GValue        *value,       /* IN */
                               GCancellable        *cancellable, /* IN */
                               GAsyncReadyCallback  callback,    /* IN */
                               gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_ENCODER(encoder));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);
	g_return_if_fail(name != NULL);

	res = ASYNC_NEW(encoder, callback, user_data,
                    pk_encoder_set_property_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, encoder->priv->connection);
	pk_connection_encoder_set_property_async(encoder->priv->connection,
	                                         encoder->priv->id,
	                                         name,
	                                         value,
	                                         cancellable,
	                                         pk_encoder_set_property_cb,
	                                         res);
}

/**
 * pk_encoder_set_property_finish:
 * @encoder: A #PkEncoder.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "encoder_set_property" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_encoder_set_property_finish (PkEncoder     *encoder, /* IN */
                                GAsyncResult  *result,  /* IN */
                                GError       **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_ENCODER(encoder), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, encoder,
	                                    pk_encoder_set_property_async),
	                     FALSE);

	ENTRY;
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_encoder_set_property_finish(encoder->priv->connection,
	                                                res,
	                                                error);
	RETURN(ret);
}

/**
 * pk_encoder_get_property_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback for #PkConnection<!-- -->'s "encoder_Encoder" RPC.
 * Dispatches the real #GAsyncResult for the operation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_encoder_get_property_cb (GObject      *object,    /* IN */
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
 * pk_encoder_get_property_async:
 * @encoder: A #PkEncoder.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncCallback to call when the result is ready.
 * @user_data: user data for @callback.
 *
 * Asynchronously executes the "encoder_get_property" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_encoder_get_property_async (PkEncoder           *encoder,     /* IN */
                               const gchar         *name,        /* IN */
                               GCancellable        *cancellable, /* IN */
                               GAsyncReadyCallback  callback,    /* IN */
                               gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *res;

	g_return_if_fail(PK_IS_ENCODER(encoder));
	g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
	g_return_if_fail(callback != NULL);
	g_return_if_fail(name != NULL);

	res = ASYNC_NEW(encoder, callback, user_data,
                    pk_encoder_get_property_async);
	ASYNC_ERROR_IF_NOT_CONNECTED(res, encoder->priv->connection);
	pk_connection_encoder_get_property_async(encoder->priv->connection,
	                                         encoder->priv->id,
	                                         name,
	                                         cancellable,
	                                         pk_encoder_get_property_cb,
	                                         res);
}

/**
 * pk_encoder_get_property_finish:
 * @encoder: A #PkEncoder.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request for the "encoder_get_property" RPC.
 *
 * Returns: %TRUE if RPC was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_encoder_get_property_finish (PkEncoder     *encoder, /* IN */
                                GAsyncResult  *result,  /* IN */
                                GValue        *value,   /* OUT */
                                GError       **error)   /* OUT */
{
	GAsyncResult *res;
	gboolean ret;

	g_return_val_if_fail(PK_IS_ENCODER(encoder), FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(ASYNC_IS_VALID(result, encoder,
	                                    pk_encoder_get_property_async),
	                     FALSE);

	ENTRY;
	memset(value, 0, sizeof(*value));
	res = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(result));
	ret = pk_connection_encoder_get_property_finish(encoder->priv->connection,
	                                                res,
	                                                value,
	                                                error);
	RETURN(ret);
}

