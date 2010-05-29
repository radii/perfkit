/* pk-encoder.c
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

#include "pk-encoder.h"
#include "pk-connection-lowlevel.h"
#include "pk-log.h"

/**
 * SECTION:pk-encoder
 * @title: PkEncoder
 * @short_description: 
 *
 * Section overview.
 */

G_DEFINE_TYPE(PkEncoder, pk_encoder, G_TYPE_OBJECT)

struct _PkEncoderPrivate
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
 * pk_encoder_get_id:
 * @encoder: A #PkEncoder.
 *
 * Retrieves the identifier for the encoder.
 *
 * Returns: A gint.
 * Side effects: None.
 */
gint
pk_encoder_get_id (PkEncoder *encoder) /* IN */
{
	g_return_val_if_fail(PK_IS_ENCODER(encoder), -1);
	return encoder->priv->id;
}

/**
 * pk_encoder_get_connection:
 * @encoder: A #PkEncoder.
 *
 * Retrieves the connection for the encoder.
 *
 * Returns: A PkConnection.
 * Side effects: None.
 */
PkConnection*
pk_encoder_get_connection (PkEncoder *encoder) /* IN */
{
	g_return_val_if_fail(PK_IS_ENCODER(encoder), NULL);
	return encoder->priv->connection;
}

/**
 * pk_encoder_get_plugin:
 * @encoder: A #PkEncoder.
 * @plugin: A location for the plugin.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the plugin which created the encoder instance.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_encoder_get_plugin (PkEncoder  *encoder, /* IN */
                       PkPlugin  **plugin,  /* OUT */
                       GError    **error)   /* OUT */
{
	PkEncoderPrivate *priv;
	gboolean ret;
	gchar* plugin_id = NULL;

	g_return_val_if_fail(PK_IS_ENCODER(encoder), FALSE);

	ENTRY;
	priv = encoder->priv;
	if (!(ret = pk_connection_encoder_get_plugin(
			priv->connection,
			priv->id,
			&plugin_id,
			error))) {
		RETURN(FALSE);
	}
	*plugin = g_object_new(PK_TYPE_PLUGIN,
	                       "connection", priv->connection,
	                       "id", plugin_id,
	                       NULL);
	RETURN(ret);
}

static void
pk_encoder_get_plugin_cb (GObject      *object,    /* IN */
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
 * pk_encoder_get_plugin_async:
 * @encoder: A #PkEncoder.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Retrieves the plugin which created the encoder instance.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_encoder_get_plugin_async (PkEncoder           *encoder,     /* IN */
                             GCancellable        *cancellable, /* IN */
                             GAsyncReadyCallback  callback,    /* IN */
                             gpointer             user_data)   /* IN */
{
	PkEncoderPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_ENCODER(encoder));

	ENTRY;
	priv = encoder->priv;
	result = g_simple_async_result_new(
			G_OBJECT(encoder),
			callback,
			user_data,
			pk_encoder_get_plugin_async);
	pk_connection_encoder_get_plugin_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_encoder_get_plugin_cb,
			result);
	EXIT;
}


/**
 * pk_encoder_get_plugin_finish:
 * @encoder: A #PkEncoder.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the plugin which created the encoder instance.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_encoder_get_plugin_finish (PkEncoder     *encoder, /* IN */
                              GAsyncResult  *result,  /* IN */
                              PkPlugin     **plugin,  /* OUT */
                              GError       **error)   /* OUT */
{
	PkEncoderPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;
	gchar* plugin_id = NULL;

	g_return_val_if_fail(PK_IS_ENCODER(encoder), FALSE);

	ENTRY;
	priv = encoder->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_encoder_get_plugin_finish(
			priv->connection,
			real_result,
			&plugin_id,
			error))) {
		RETURN(FALSE);
	}
	*plugin = g_object_new(PK_TYPE_PLUGIN,
	                       "connection", priv->connection,
	                       "id", plugin_id,
	                       NULL);
	RETURN(ret);
}

static void
pk_encoder_finalize (GObject *object) /* IN */
{
	G_OBJECT_CLASS(pk_encoder_parent_class)->finalize(object);
}

static void
pk_encoder_get_property (GObject    *object,  /* IN */
                        guint       prop_id, /* IN */
                        GValue     *value,   /* IN */
                        GParamSpec *pspec)   /* IN */
{
	switch (prop_id) {
	case PROP_ID:
		g_value_set_int(value,
		                pk_encoder_get_id(PK_ENCODER(object)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_encoder_set_property (GObject      *object,  /* IN */
                        guint         prop_id, /* IN */
                        const GValue *value,   /* IN */
                        GParamSpec   *pspec)   /* IN */
{
	switch (prop_id) {
	case PROP_ID:
		PK_ENCODER(object)->priv->id = g_value_get_int(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_encoder_class_init (PkEncoderClass *klass) /* IN */
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pk_encoder_finalize;
	object_class->get_property = pk_encoder_get_property;
	object_class->set_property = pk_encoder_set_property;
	g_type_class_add_private(object_class, sizeof(PkEncoderPrivate));

	/**
	 * PkEncoder:id:
	 *
	 * The encoder identifier.
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
	 * PkEncoder:connection:
	 *
	 * The encoder connection.
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
pk_encoder_init (PkEncoder *encoder) /* IN */
{
	encoder->priv = G_TYPE_INSTANCE_GET_PRIVATE(encoder, PK_TYPE_ENCODER, PkEncoderPrivate);
}
