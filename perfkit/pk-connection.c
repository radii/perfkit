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

/**
 * SECTION:pk-connection
 * @title: PkConnection
 * @short_description: Perfkit connection protocol
 *
 * #PkConnection is an abstraction on the RPC interaction from a client to
 * a Perfkit daemon.  This allows for multiple connection implementations
 * which can be used based on the needs of the client.
 *
 * Currently, there is a DBUS #PkConnection implementation.  It can be used
 * by providing the URI "dbus://" to pk_connection_new_for_uri().
 */

G_DEFINE_TYPE (PkConnection, pk_connection, G_TYPE_OBJECT)

struct _PkConnectionPrivate
{
	PkChannels *channels;
};

/**
 * pk_connection_new_for_uri:
 * @uri: a connection uri
 *
 * Creates a new #PkConnection using the protocol required based on the
 * @uri provided.  A @uri of "dbus://" will use the DBUS protocol
 * implementation.
 *
 * Return value: A #PkConnection or %NULL if the URI is invalid.
 *
 * Side effects: None
 */
PkConnection*
pk_connection_new_for_uri (const gchar *uri)
{
	PkConnection *connection = NULL;

	g_return_val_if_fail (uri != NULL, NULL);

	if (g_str_has_prefix (uri, "dbus://"))
		connection = pk_connection_dbus_new ();
	else
		g_warning (_("Unknown connection URI: %s"), uri);

	return connection;
}

/**
 * pk_connection_connect_async:
 * @connection: A #PkConnection
 * @callback: A #GAsyncReadyCallback
 * @user_data: User data for @callback
 *
 * Asynchronously connects to the Perfkit daemon.  See pk_connection_connect().
 *
 * Side effects: Alters the state of the #PkConnection based on the connection
 *   success.
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
 * @connection: A #PkConnection
 * @result: The #GAsyncResult supplied to the async callback
 * @error: A location for a #GError or %NULL
 *
 * Completes an asynchronous request to connect to the Perfkit daemon.
 * This should be called from the callback provided to
 * pk_connection_connect_async().
 *
 * Return value: %TRUE on success.  @error is set on failure.
 *
 * Side effects: None
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
 * @connection: A #PkConnection
 * @callback: A #GAsyncReadyCallback
 * @user_data: User data for @callback
 *
 * Asynchronously disconnects from the Perfkit daemon.
 *
 * Side effects: Alters the state of the connection to disconnected.
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
 * @connection: A #PkConnection
 * @result: A #GAsyncResult
 *
 * Completes an asynchronous request to pk_connection_disconnect_async().
 * This method should be called from the #GAsyncReadyCallback provided to
 * pk_connection_disconnect_async().
 *
 * Side effects: None
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
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL
 *
 * Synchronously connects to the Perfkit daemon.
 *
 * Return value: %TRUE on success.  Upon failure @error is set.
 *
 * Side effects: The state of the connection is set to connected if the
 *   connection was successful.
 */
gboolean
pk_connection_connect (PkConnection  *connection,
                       GError       **error)
{
	return PK_CONNECTION_GET_CLASS (connection)->connect (connection, error);
}

/**
 * pk_connection_disconnect:
 * @connection: A #PkConnection
 *
 * Synchronously disconnects @connection from the Perfkit daemon.
 *
 * Side effects: The state of the connection is set to disconnected.
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
 * @connection: A #PkConnection
 *
 * Retrieves the proxy for the Perfkit channels service.  The channels
 * service provides access to retrieving, adding, and removing Perfkit
 * channels.
 *
 * Return value: A #PkChannels instance.
 *
 * Side effects: None
 */
PkChannels*
pk_connection_get_channels (PkConnection *connection)
{
	return connection->priv->channels;
}

/**************************************************************************
 *                          Private RPC Wrappers                          *
 **************************************************************************/

gboolean
pk_connection_channels_add (PkConnection  *connection,
                            gint          *channel_id,
                            GError       **error)
{
	g_return_val_if_fail (PK_IS_CONNECTION (connection), FALSE);
	return PK_CONNECTION_GET_CLASS (connection)->
		channels_add (connection, channel_id, error);
}

gboolean
pk_connection_channels_find_all (PkConnection  *connection,
                                 gint         **channel_ids,
                                 gint          *n_channels)
{
	g_return_val_if_fail (PK_IS_CONNECTION (connection), FALSE);
	return PK_CONNECTION_GET_CLASS (connection)->
		channels_find_all (connection, channel_ids, n_channels);
}

gchar*
pk_connection_channel_get_target (PkConnection *connection,
                                  gint          channel_id)
{
	g_return_val_if_fail (PK_IS_CONNECTION (connection), NULL);
	return PK_CONNECTION_GET_CLASS (connection)->
		channel_get_target (connection, channel_id);
}

gchar**
pk_connection_channel_get_args (PkConnection *connection,
                                gint          channel_id)
{
	g_return_val_if_fail (PK_IS_CONNECTION (connection), NULL);
	return PK_CONNECTION_GET_CLASS (connection)->
		channel_get_args (connection, channel_id);
}

gchar*
pk_connection_channel_get_dir (PkConnection *connection,
                               gint          channel_id)
{
	g_return_val_if_fail (PK_IS_CONNECTION (connection), NULL);
	return PK_CONNECTION_GET_CLASS (connection)->
		channel_get_dir (connection, channel_id);
}

gchar**
pk_connection_channel_get_env (PkConnection *connection,
                               gint          channel_id)
{
	g_return_val_if_fail (PK_IS_CONNECTION (connection), NULL);
	return PK_CONNECTION_GET_CLASS (connection)->
		channel_get_env (connection, channel_id);
}

GPid
pk_connection_channel_get_pid (PkConnection *connection,
                               gint          channel_id)
{
	g_return_val_if_fail (PK_IS_CONNECTION (connection), 0);
	return PK_CONNECTION_GET_CLASS (connection)->
		channel_get_pid (connection, channel_id);
}

PkChannelState
pk_connection_channel_get_state (PkConnection *connection,
                                 gint          channel_id)
{
	g_return_val_if_fail (PK_IS_CONNECTION (connection), -1);
	return PK_CONNECTION_GET_CLASS (connection)->
		channel_get_state (connection, channel_id);
}

/**************************************************************************
 *                         GObject Class Methods                          *
 **************************************************************************/

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
