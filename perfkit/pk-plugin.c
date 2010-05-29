/* pk-plugin.c
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

#include "pk-plugin.h"
#include "pk-connection-lowlevel.h"
#include "pk-log.h"

/**
 * SECTION:pk-plugin
 * @title: PkPlugin
 * @short_description: 
 *
 * Section overview.
 */

G_DEFINE_TYPE(PkPlugin, pk_plugin, G_TYPE_OBJECT)

struct _PkPluginPrivate
{
	PkConnection *connection;
	gchar *id;
};

enum
{
	PROP_0,
	PROP_ID,
	PROP_CONNECTION,
};

/**
 * pk_plugin_get_id:
 * @plugin: A #PkPlugin.
 *
 * Retrieves the identifier for the plugin.
 *
 * Returns: A const gchar*.
 * Side effects: None.
 */
const gchar*
pk_plugin_get_id (PkPlugin *plugin) /* IN */
{
	g_return_val_if_fail(PK_IS_PLUGIN(plugin), NULL);
	return plugin->priv->id;
}

/**
 * pk_plugin_get_connection:
 * @plugin: A #PkPlugin.
 *
 * Retrieves the connection for the plugin.
 *
 * Returns: A PkConnection.
 * Side effects: None.
 */
PkConnection*
pk_plugin_get_connection (PkPlugin *plugin) /* IN */
{
	g_return_val_if_fail(PK_IS_PLUGIN(plugin), NULL);
	return plugin->priv->connection;
}

/**
 * pk_plugin_get_copyright:
 * @plugin: A #PkPlugin.
 * @copyright: A location for the copyright.
 * @error: A location for a #GError, or %NULL.
 *
 * The plugin copyright.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_plugin_get_copyright (PkPlugin  *plugin,    /* IN */
                         gchar    **copyright, /* OUT */
                         GError   **error)     /* OUT */
{
	PkPluginPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_PLUGIN(plugin), FALSE);

	ENTRY;
	priv = plugin->priv;
	if (!(ret = pk_connection_plugin_get_copyright(
			priv->connection,
			priv->id,
			copyright,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_plugin_get_copyright_cb (GObject      *object,    /* IN */
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
 * pk_plugin_get_copyright_async:
 * @plugin: A #PkPlugin.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * The plugin copyright.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_plugin_get_copyright_async (PkPlugin            *plugin,      /* IN */
                               GCancellable        *cancellable, /* IN */
                               GAsyncReadyCallback  callback,    /* IN */
                               gpointer             user_data)   /* IN */
{
	PkPluginPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_PLUGIN(plugin));

	ENTRY;
	priv = plugin->priv;
	result = g_simple_async_result_new(
			G_OBJECT(plugin),
			callback,
			user_data,
			pk_plugin_get_copyright_async);
	pk_connection_plugin_get_copyright_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_plugin_get_copyright_cb,
			result);
	EXIT;
}


/**
 * pk_plugin_get_copyright_finish:
 * @plugin: A #PkPlugin.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * The plugin copyright.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_plugin_get_copyright_finish (PkPlugin      *plugin,    /* IN */
                                GAsyncResult  *result,    /* IN */
                                gchar        **copyright, /* OUT */
                                GError       **error)     /* OUT */
{
	PkPluginPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_PLUGIN(plugin), FALSE);

	ENTRY;
	priv = plugin->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_plugin_get_copyright_finish(
			priv->connection,
			real_result,
			copyright,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_plugin_get_description:
 * @plugin: A #PkPlugin.
 * @description: A location for the description.
 * @error: A location for a #GError, or %NULL.
 *
 * The plugin description.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_plugin_get_description (PkPlugin  *plugin,      /* IN */
                           gchar    **description, /* OUT */
                           GError   **error)       /* OUT */
{
	PkPluginPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_PLUGIN(plugin), FALSE);

	ENTRY;
	priv = plugin->priv;
	if (!(ret = pk_connection_plugin_get_description(
			priv->connection,
			priv->id,
			description,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_plugin_get_description_cb (GObject      *object,    /* IN */
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
 * pk_plugin_get_description_async:
 * @plugin: A #PkPlugin.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * The plugin description.
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
	PkPluginPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_PLUGIN(plugin));

	ENTRY;
	priv = plugin->priv;
	result = g_simple_async_result_new(
			G_OBJECT(plugin),
			callback,
			user_data,
			pk_plugin_get_description_async);
	pk_connection_plugin_get_description_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_plugin_get_description_cb,
			result);
	EXIT;
}


/**
 * pk_plugin_get_description_finish:
 * @plugin: A #PkPlugin.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * The plugin description.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_plugin_get_description_finish (PkPlugin      *plugin,      /* IN */
                                  GAsyncResult  *result,      /* IN */
                                  gchar        **description, /* OUT */
                                  GError       **error)       /* OUT */
{
	PkPluginPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_PLUGIN(plugin), FALSE);

	ENTRY;
	priv = plugin->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_plugin_get_description_finish(
			priv->connection,
			real_result,
			description,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_plugin_get_name:
 * @plugin: A #PkPlugin.
 * @name: A location for the name.
 * @error: A location for a #GError, or %NULL.
 *
 * The plugin name.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_plugin_get_name (PkPlugin  *plugin, /* IN */
                    gchar    **name,   /* OUT */
                    GError   **error)  /* OUT */
{
	PkPluginPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_PLUGIN(plugin), FALSE);

	ENTRY;
	priv = plugin->priv;
	if (!(ret = pk_connection_plugin_get_name(
			priv->connection,
			priv->id,
			name,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_plugin_get_name_cb (GObject      *object,    /* IN */
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
 * pk_plugin_get_name_async:
 * @plugin: A #PkPlugin.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * The plugin name.
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
	PkPluginPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_PLUGIN(plugin));

	ENTRY;
	priv = plugin->priv;
	result = g_simple_async_result_new(
			G_OBJECT(plugin),
			callback,
			user_data,
			pk_plugin_get_name_async);
	pk_connection_plugin_get_name_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_plugin_get_name_cb,
			result);
	EXIT;
}


/**
 * pk_plugin_get_name_finish:
 * @plugin: A #PkPlugin.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * The plugin name.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_plugin_get_name_finish (PkPlugin      *plugin, /* IN */
                           GAsyncResult  *result, /* IN */
                           gchar        **name,   /* OUT */
                           GError       **error)  /* OUT */
{
	PkPluginPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_PLUGIN(plugin), FALSE);

	ENTRY;
	priv = plugin->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_plugin_get_name_finish(
			priv->connection,
			real_result,
			name,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_plugin_get_plugin_type:
 * @plugin: A #PkPlugin.
 * @type: A location for the type.
 * @error: A location for a #GError, or %NULL.
 *
 * The plugin type.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_plugin_get_plugin_type (PkPlugin  *plugin, /* IN */
                           gint      *type,   /* OUT */
                           GError   **error)  /* OUT */
{
	PkPluginPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_PLUGIN(plugin), FALSE);

	ENTRY;
	priv = plugin->priv;
	if (!(ret = pk_connection_plugin_get_plugin_type(
			priv->connection,
			priv->id,
			type,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_plugin_get_plugin_type_cb (GObject      *object,    /* IN */
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
 * pk_plugin_get_plugin_type_async:
 * @plugin: A #PkPlugin.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * The plugin type.
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
	PkPluginPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_PLUGIN(plugin));

	ENTRY;
	priv = plugin->priv;
	result = g_simple_async_result_new(
			G_OBJECT(plugin),
			callback,
			user_data,
			pk_plugin_get_plugin_type_async);
	pk_connection_plugin_get_plugin_type_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_plugin_get_plugin_type_cb,
			result);
	EXIT;
}


/**
 * pk_plugin_get_plugin_type_finish:
 * @plugin: A #PkPlugin.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * The plugin type.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_plugin_get_plugin_type_finish (PkPlugin      *plugin, /* IN */
                                  GAsyncResult  *result, /* IN */
                                  gint          *type,   /* OUT */
                                  GError       **error)  /* OUT */
{
	PkPluginPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_PLUGIN(plugin), FALSE);

	ENTRY;
	priv = plugin->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_plugin_get_plugin_type_finish(
			priv->connection,
			real_result,
			type,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_plugin_get_version:
 * @plugin: A #PkPlugin.
 * @version: A location for the version.
 * @error: A location for a #GError, or %NULL.
 *
 * The plugin version.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_plugin_get_version (PkPlugin  *plugin,  /* IN */
                       gchar    **version, /* OUT */
                       GError   **error)   /* OUT */
{
	PkPluginPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_PLUGIN(plugin), FALSE);

	ENTRY;
	priv = plugin->priv;
	if (!(ret = pk_connection_plugin_get_version(
			priv->connection,
			priv->id,
			version,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_plugin_get_version_cb (GObject      *object,    /* IN */
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
 * pk_plugin_get_version_async:
 * @plugin: A #PkPlugin.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * The plugin version.
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
	PkPluginPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_PLUGIN(plugin));

	ENTRY;
	priv = plugin->priv;
	result = g_simple_async_result_new(
			G_OBJECT(plugin),
			callback,
			user_data,
			pk_plugin_get_version_async);
	pk_connection_plugin_get_version_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_plugin_get_version_cb,
			result);
	EXIT;
}


/**
 * pk_plugin_get_version_finish:
 * @plugin: A #PkPlugin.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * The plugin version.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_plugin_get_version_finish (PkPlugin      *plugin,  /* IN */
                              GAsyncResult  *result,  /* IN */
                              gchar        **version, /* OUT */
                              GError       **error)   /* OUT */
{
	PkPluginPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_PLUGIN(plugin), FALSE);

	ENTRY;
	priv = plugin->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_plugin_get_version_finish(
			priv->connection,
			real_result,
			version,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_plugin_finalize (GObject *object) /* IN */
{
	G_OBJECT_CLASS(pk_plugin_parent_class)->finalize(object);
}

static void
pk_plugin_get_property (GObject    *object,  /* IN */
                        guint       prop_id, /* IN */
                        GValue     *value,   /* IN */
                        GParamSpec *pspec)   /* IN */
{
	switch (prop_id) {
	case PROP_ID:
		g_value_set_string(value,
		                   pk_plugin_get_id(PK_PLUGIN(object)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_plugin_set_property (GObject      *object,  /* IN */
                        guint         prop_id, /* IN */
                        const GValue *value,   /* IN */
                        GParamSpec   *pspec)   /* IN */
{
	switch (prop_id) {
	case PROP_ID:
		PK_PLUGIN(object)->priv->id = g_value_dup_string(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_plugin_class_init (PkPluginClass *klass) /* IN */
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pk_plugin_finalize;
	object_class->get_property = pk_plugin_get_property;
	object_class->set_property = pk_plugin_set_property;
	g_type_class_add_private(object_class, sizeof(PkPluginPrivate));

	/**
	 * PkPlugin:id:
	 *
	 * The plugin identifier.
	 */
	g_object_class_install_property(object_class,
	                                PROP_ID,
	                                g_param_spec_string("id",
	                                                    "Id",
	                                                    "Id",
	                                                    NULL,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	/**
	 * PkPlugin:connection:
	 *
	 * The plugin connection.
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
pk_plugin_init (PkPlugin *plugin) /* IN */
{
	plugin->priv = G_TYPE_INSTANCE_GET_PRIVATE(plugin, PK_TYPE_PLUGIN, PkPluginPrivate);
}
