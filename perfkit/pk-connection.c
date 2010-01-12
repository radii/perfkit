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
#include "pk-protocol.h"

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
	PkProtocol *protocol;
};

enum
{
	PROP_0,
	PROP_PROTOCOL,
};

static GType
get_protocol_type (const gchar *uri)
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
	 * Get the protocol type "type://".
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
	 * Determine the protocol path.
	 */
	if (g_getenv("PK_PROTOCOLS_DIR") != NULL) {
		path = g_module_build_path(g_getenv("PK_PROTOCOLS_DIR"),
		                           type_name);
	} else {
		path = g_module_build_path(PACKAGE_LIB_DIR
		                           G_DIR_SEPARATOR_S
		                           "perfkit"
		                           G_DIR_SEPARATOR_S
		                           "protocols",
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
	 * Lookup the "pk_protocol_plugin" symbol.
	 */
	g_module_symbol(module, "pk_protocol_plugin", (gpointer *)&plugin);
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
 * Creates a new instance of #PkConnection using @uri.  If the protocol
 * specified cannot be provided, %NULL is returned.
 *
 * Returns: the newly created #PkConnection or %NULL.
 */
PkConnection*
pk_connection_new_from_uri(const gchar *uri)
{
	PkConnection *conn;
	PkProtocol *proto;
	GType proto_type;

	/*
	 * Get the protocol type from the uri.
	 */
	proto_type = get_protocol_type(uri);
	if (!g_type_is_a(proto_type, PK_TYPE_PROTOCOL)) {
		g_warning("Protocol plugin did not return a PkProtocol.");
		return NULL;
	}

	/*
	 * Instantiate the protocol.
	 */
	proto = g_object_new(proto_type, "uri", uri, NULL);
	if (!proto) {
		g_warning("Could not create new instance of %s.",
		          g_type_name(proto_type));
		return NULL;
	}

	/*
	 * Create an instance of the connection.
	 */
	conn = g_object_new(PK_TYPE_CONNECTION, "protocol", proto, NULL);
	if (!conn) {
		g_warning("Could not create new instance of PkConnection.");
		g_object_unref(proto);
	}

	return conn;
}

static void
pk_connection_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
	switch (prop_id) {
	case PROP_PROTOCOL:
		g_value_set_object(value, PK_CONNECTION(object)->priv->protocol);
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
	case PROP_PROTOCOL:
		PK_CONNECTION(object)->priv->protocol = g_value_dup_object(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_connection_finalize (GObject *object)
{
	G_OBJECT_CLASS(pk_connection_parent_class)->finalize(object);
}

static void
pk_connection_dispose (GObject *object)
{
	PkConnectionPrivate *priv = PK_CONNECTION(object)->priv;

	g_object_unref(priv->protocol);

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
	 * PkConnection:protocol:
	 *
	 * The "protocol" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_PROTOCOL,
	                                g_param_spec_object("protocol",
	                                                    "protocol",
	                                                    "protocol",
	                                                    PK_TYPE_PROTOCOL,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
pk_connection_init (PkConnection *connection)
{
	connection->priv = G_TYPE_INSTANCE_GET_PRIVATE(connection,
	                                               PK_TYPE_CONNECTION,
	                                               PkConnectionPrivate);
}
