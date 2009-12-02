/* pk-connection.c
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
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

#include <glib/gi18n.h>

#include "pk-channels.h"
#include "pk-channels-priv.h"
#include "pk-connection.h"
#include "pk-connection-dbus.h"

G_DEFINE_TYPE (PkConnection, pk_connection, G_TYPE_OBJECT)

struct _PkConnectionPrivate
{
	PkChannels *channels;
};

static void
pk_connection_finalize (GObject *object)
{
	G_OBJECT_CLASS (pk_connection_parent_class)->finalize (object);
}

static void
pk_connection_class_init (PkConnectionClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_connection_finalize;
	g_type_class_add_private (object_class, sizeof (PkConnectionPrivate));
}

static void
pk_connection_init (PkConnection *connection)
{
	connection->priv = G_TYPE_INSTANCE_GET_PRIVATE (connection,
	                                                PK_TYPE_CONNECTION,
	                                                PkConnectionPrivate);

	connection->priv->channels = pk_channels_new (connection);
	g_object_add_weak_pointer (G_OBJECT (connection->priv->channels),
	                           (gpointer*)&connection->priv->channels);
}

/**
 * pk_connection_new_for_uri:
 * @uri: 
 *
 * 
 *
 * Return value: 
 */
PkConnection*
pk_connection_new_for_uri (const gchar *uri)
{
	PkConnection *connection = NULL;

	g_return_val_if_fail (uri != NULL, NULL);

	if (g_str_has_prefix (uri, "dbus://")) {
		connection = pk_connection_dbus_new ();
	}
	else {
		g_warning (_("Unknown connection URI: %s"), uri);
	}

	return connection;
}

/**
 * pk_connection_connect_async:
 * @connection: 
 * @callback: 
 * @user_data: 
 *
 * 
 */
void
pk_connection_connect_async (PkConnection        *connection,
                             GAsyncReadyCallback  callback,
                             gpointer             user_data)
{
	PK_CONNECTION_GET_CLASS (connection)->
		connect_async (connection, callback, user_data);
}

/**
 * pk_connection_connect_finish:
 * @connection:
 * @callback: 
 * @error: 
 *
 *
 *
 * Return value: 
 */
gboolean
pk_connection_connect_finish (PkConnection  *connection,
                              GAsyncResult  *result,
                              GError       **error)
{
	return PK_CONNECTION_GET_CLASS (connection)->
		connect_finish (connection, result, error);
}

/**
 * pk_connection_disconnect_async:
 * @connection: 
 * @callback: 
 * @user_data: 
 *
 * 
 */
void
pk_connection_disconnect_async (PkConnection        *connection,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data)
{
	PK_CONNECTION_GET_CLASS (connection)->
		disconnect_async (connection, callback, user_data);
}

/**
 * pk_connection_disconnect_finish:
 * @connection:
 * @callback: 
 * @error: 
 *
 *
 *
 * Return value: 
 */
void
pk_connection_disconnect_finish (PkConnection  *connection,
                                 GAsyncResult  *result)
{
	PK_CONNECTION_GET_CLASS (connection)->
		disconnect_finish (connection, result);
}

/**
 * pk_connection_connect:
 * @connection:
 * @error: 
 *
 * 
 *
 * Return value: 
 */
gboolean
pk_connection_connect (PkConnection  *connection,
                       GError       **error)
{
	return PK_CONNECTION_GET_CLASS (connection)->connect (connection, error);
}

/**
 * pk_connection_disconnect:
 * @connection: 
 *
 * 
 */
void
pk_connection_disconnect (PkConnection *connection)
{
	PK_CONNECTION_GET_CLASS (connection)->disconnect (connection);
}

GQuark
pk_connection_error_quark (void)
{
	return g_quark_from_static_string ("pk-connection-error-quark");
}

/**
 * pk_connection_get_channels:
 * @connection: 
 *
 * 
 *
 * Return value: 
 */
PkChannels*
pk_connection_get_channels (PkConnection *connection)
{
	return connection->priv->channels;
}
