/* pk-connection.c
 *
 * Copyright (C) 2010 Christian Hergert
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gmodule.h>
#include <string.h>

#include "pk-connection.h"

G_DEFINE_TYPE(PkConnection, pk_connection, G_TYPE_OBJECT)

/**
 * SECTION:pk-connection
 * @title: PkConnection
 * @short_description: 
 *
 * 
 */

struct _PkConnectionPrivate
{
	PkManager *manager;
	gchar *uri;
};

enum
{
	PROP_0,
	PROP_URI,
};

static GType
get_connection_type (const gchar *uri)
{
	GType (*plugin) (void);
	gboolean colon_found = FALSE;
	GType type = G_TYPE_INVALID;
	gchar type_name[16];
	GModule *module;
	gchar *path;
	gint i, m;

	g_return_val_if_fail(uri != NULL, G_TYPE_INVALID);

	/*
	 * Get the connection type "type://".
	 */
	memset(type_name, 0, sizeof(type_name));
	m = MIN(sizeof(type_name), strlen(uri));
	for (i = 0; i < m; i++) {
		if (uri[i] == ':') {
			colon_found = TRUE;
			break;
		}

		if (!g_ascii_isalnum(uri[i])) {
			break;
		}

		type_name[i] = uri[i];
	}

	/*
	 * If we didn't get to a :, we didn't get a valid type.
	 */
	if (!colon_found) {
		return G_TYPE_INVALID;
	}

	/*
	 * Determine the connection path.
	 */
	if (g_getenv("PK_CONNECTIONS_DIR") != NULL) {
		path = g_module_build_path(g_getenv("PK_CONNECTIONS_DIR"),
		                           type_name);
	} else {
		path = g_module_build_path(PACKAGE_LIB_DIR
		                           G_DIR_SEPARATOR_S
		                           "perfkit"
		                           G_DIR_SEPARATOR_S
		                           "connections",
		                           type_name);
	}

	/*
	 * Open the module.
	 */
	module = g_module_open(path, G_MODULE_BIND_LAZY);
	if (!module) {
		goto cleanup;
	}

	/*
	 * Lookup the "pk_connection_plugin" symbol.
	 */
	g_module_symbol(module, "pk_connection_plugin", (gpointer *)&plugin);
	if (!plugin) {
		goto cleanup;
	}

	/*
	 * Execute the plugin type func.
	 */
	type = plugin();

cleanup:
	g_free(path);
	return type;
}

/**
 * pk_connection_new_from_uri:
 * @uri: the uri of the agent
 *
 * Creates a new instance of #PkConnection using @uri.  If the connection
 * specified cannot be provided, %NULL is returned.
 *
 * Returns: the newly created #PkConnection or %NULL.
 */
PkConnection*
pk_connection_new_from_uri(const gchar *uri)
{
	PkConnection *conn;
	GType conn_type;

	/*
	 * Get the connection type from the uri.
	 */
	conn_type = get_connection_type(uri);
	if (!g_type_is_a(conn_type, PK_TYPE_CONNECTION)) {
		g_warning("Connection plugin did not return a PkConnection.");
		return NULL;
	}

	/*
	 * Instantiate the connection.
	 */
	conn = g_object_new(conn_type, "uri", uri, NULL);
	if (!conn) {
		g_warning("Could not create new instance of %s.",
		          g_type_name(conn_type));
		return NULL;
	}

	return conn;
}

const gchar*
pk_connection_get_uri (PkConnection *connection)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), NULL);
	return connection->priv->uri;
}

PkManager*
pk_connection_get_manager (PkConnection *connection)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), NULL);
	return connection->priv->manager;
}

gboolean
pk_connection_connect (PkConnection  *connection,
                       GError       **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->connect(connection, error);
}

gboolean
pk_connection_channel_get_target (PkConnection   *connection,
                                  gint            channel_id,
                                  gchar         **target,
                                  GError        **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->channel_get_target(connection,
	                                                               channel_id,
	                                                               target,
	                                                               error);
}

gboolean
pk_connection_channel_get_working_dir (PkConnection   *connection,
                                       gint            channel_id,
                                       gchar         **working_dir,
                                       GError        **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->channel_get_working_dir(connection,
	                                                                    channel_id,
	                                                                    working_dir,
	                                                                    error);
}

gboolean
pk_connection_channel_get_args (PkConnection   *connection,
                                gint            channel_id,
                                gchar        ***args,
                                GError        **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->channel_get_args(connection,
	                                                             channel_id,
	                                                             args,
	                                                             error);
}

gboolean
pk_connection_channel_get_env (PkConnection   *connection,
                               gint            channel_id,
                               gchar        ***env,
                               GError        **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->channel_get_env(connection,
	                                                            channel_id,
	                                                            env,
	                                                            error);
}

gboolean
pk_connection_channel_get_pid (PkConnection   *connection,
                               gint            channel_id,
                               GPid           *pid,
                               GError        **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->channel_get_pid(connection,
	                                                            channel_id,
	                                                            pid,
	                                                            error);
}

gboolean
pk_connection_manager_create_channel (PkConnection   *connection,
                                      PkSpawnInfo    *spawn_info,
                                      gint           *channel_id,
                                      GError        **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->manager_create_channel(
			connection, spawn_info, channel_id, error);
}

gboolean
pk_connection_channel_start (PkConnection  *connection,
                             gint           channel_id,
                             GError       **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->channel_start(
			connection, channel_id, error);
}

gboolean
pk_connection_channel_stop (PkConnection  *connection,
                            gint           channel_id,
                            gboolean       killpid,
                            GError       **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->channel_stop(
			connection, channel_id, killpid, error);
}

gboolean
pk_connection_channel_pause (PkConnection  *connection,
                             gint           channel_id,
                             GError       **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->channel_pause(
			connection, channel_id, error);
}

gboolean
pk_connection_channel_unpause (PkConnection  *connection,
                               gint           channel_id,
                               GError       **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->channel_unpause(
			connection, channel_id, error);
}

static void
pk_connection_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
	switch (prop_id) {
	case PROP_URI:
		g_value_set_string(value, pk_connection_get_uri((gpointer)object));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_connection_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
	switch (prop_id) {
	case PROP_URI:
		PK_CONNECTION(object)->priv->uri = g_value_dup_string(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_connection_finalize (GObject *object)
{
	PkConnectionPrivate *priv = PK_CONNECTION(object)->priv;

	g_free(priv->uri);

	G_OBJECT_CLASS(pk_connection_parent_class)->finalize(object);
}

static void
pk_connection_dispose (GObject *object)
{
	G_OBJECT_CLASS(pk_connection_parent_class)->dispose(object);
}

static void
pk_connection_class_init (PkConnectionClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_connection_finalize;
	object_class->dispose = pk_connection_dispose;
	object_class->set_property = pk_connection_set_property;
	object_class->get_property = pk_connection_get_property;
	g_type_class_add_private(object_class, sizeof(PkConnectionPrivate));

	/**
	 * PkConnection:uri:
	 *
	 * The "uri" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_URI,
	                                g_param_spec_string("uri",
	                                                    "uri",
	                                                    "uri",
	                                                    NULL,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
pk_connection_init (PkConnection *connection)
{
	connection->priv = G_TYPE_INSTANCE_GET_PRIVATE(connection,
	                                               PK_TYPE_CONNECTION,
	                                               PkConnectionPrivate);
	connection->priv->manager = g_object_new(PK_TYPE_MANAGER,
	                                         "connection", connection,
	                                         NULL);
}
