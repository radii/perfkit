/* pk-source.c
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

#include "pk-source.h"
#include "pk-connection-lowlevel.h"
#include "pk-log.h"

/**
 * SECTION:pk-source
 * @title: PkSource
 * @short_description: 
 *
 * Section overview.
 */

G_DEFINE_TYPE(PkSource, pk_source, G_TYPE_OBJECT)

struct _PkSourcePrivate
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
 * pk_source_get_id:
 * @source: A #PkSource.
 *
 * Retrieves the identifier for the source.
 *
 * Returns: A gint.
 * Side effects: None.
 */
gint
pk_source_get_id (PkSource *source) /* IN */
{
	g_return_val_if_fail(PK_IS_SOURCE(source), -1);
	return source->priv->id;
}

/**
 * pk_source_get_connection:
 * @source: A #PkSource.
 *
 * Retrieves the connection for the source.
 *
 * Returns: A PkConnection.
 * Side effects: None.
 */
PkConnection*
pk_source_get_connection (PkSource *source) /* IN */
{
	g_return_val_if_fail(PK_IS_SOURCE(source), NULL);
	return source->priv->connection;
}

/**
 * pk_source_get_plugin:
 * @source: A #PkSource.
 * @plugin: A location for the plugin.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the plugin for which the source originates.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_source_get_plugin (PkSource  *source, /* IN */
                      PkPlugin **plugin, /* OUT */
                      GError   **error)  /* OUT */
{
	PkSourcePrivate *priv;
	gboolean ret;
	gchar* plugin_id = NULL;

	g_return_val_if_fail(PK_IS_SOURCE(source), FALSE);

	ENTRY;
	priv = source->priv;
	if (!(ret = pk_connection_source_get_plugin(
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
pk_source_get_plugin_cb (GObject      *object,    /* IN */
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
 * pk_source_get_plugin_async:
 * @source: A #PkSource.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Retrieves the plugin for which the source originates.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_source_get_plugin_async (PkSource            *source,      /* IN */
                            GCancellable        *cancellable, /* IN */
                            GAsyncReadyCallback  callback,    /* IN */
                            gpointer             user_data)   /* IN */
{
	PkSourcePrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_SOURCE(source));

	ENTRY;
	priv = source->priv;
	result = g_simple_async_result_new(
			G_OBJECT(source),
			callback,
			user_data,
			pk_source_get_plugin_async);
	pk_connection_source_get_plugin_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_source_get_plugin_cb,
			result);
	EXIT;
}


/**
 * pk_source_get_plugin_finish:
 * @source: A #PkSource.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the plugin for which the source originates.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_source_get_plugin_finish (PkSource      *source, /* IN */
                             GAsyncResult  *result, /* IN */
                             PkPlugin     **plugin, /* OUT */
                             GError       **error)  /* OUT */
{
	PkSourcePrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;
	gchar* plugin_id = NULL;

	g_return_val_if_fail(PK_IS_SOURCE(source), FALSE);

	ENTRY;
	priv = source->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_source_get_plugin_finish(
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
pk_source_finalize (GObject *object) /* IN */
{
	G_OBJECT_CLASS(pk_source_parent_class)->finalize(object);
}

static void
pk_source_get_property (GObject    *object,  /* IN */
                        guint       prop_id, /* IN */
                        GValue     *value,   /* IN */
                        GParamSpec *pspec)   /* IN */
{
	switch (prop_id) {
	case PROP_ID:
		g_value_set_int(value,
		                pk_source_get_id(PK_SOURCE(object)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_source_set_property (GObject      *object,  /* IN */
                        guint         prop_id, /* IN */
                        const GValue *value,   /* IN */
                        GParamSpec   *pspec)   /* IN */
{
	switch (prop_id) {
	case PROP_ID:
		PK_SOURCE(object)->priv->id = g_value_get_int(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_source_class_init (PkSourceClass *klass) /* IN */
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pk_source_finalize;
	object_class->get_property = pk_source_get_property;
	object_class->set_property = pk_source_set_property;
	g_type_class_add_private(object_class, sizeof(PkSourcePrivate));

	/**
	 * PkSource:id:
	 *
	 * The source identifier.
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
	 * PkSource:connection:
	 *
	 * The source connection.
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
pk_source_init (PkSource *source) /* IN */
{
	source->priv = G_TYPE_INSTANCE_GET_PRIVATE(source, PK_TYPE_SOURCE, PkSourcePrivate);
}
