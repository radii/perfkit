/* pk-connection-dbus.c
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

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <glib/gi18n.h>

#include "pk-channel-dbus.h"
#include "pk-channels-dbus.h"
#include "pk-connection-dbus.h"
#include "pk-sample.h"
#include "pk-sample-priv.h"
#include "pk-source-dbus.h"
#include "pk-sources-dbus.h"

G_DEFINE_TYPE (PkConnectionDBus, pk_connection_dbus, PK_TYPE_CONNECTION)

struct _PkConnectionDBusPrivate
{
	GStaticRWLock    rw_lock;
	DBusGConnection *dbus;          /* Connection to shared DBUS */
	DBusGProxy      *channels;      /* Proxy to Channels service */
	DBusGProxy      *sources;       /* Proxy to Sources service */
	DBusServer      *server;        /* Private DBUS server for receiving
	                                 * samples from the daemon.
	                                 */
	GHashTable      *subscriptions; /* Active subscriptions */
};

typedef struct
{
	gint             subscription_id;
	PkSampleCallback callback;
	gpointer         user_data;
} Subscription;

static gint subscription_seq = 0;

static gboolean
pk_util_parse_int (const gchar *str,
                   gint        *v_int)
{
	gchar *ptr;
	gint   val;

	g_return_val_if_fail (str != NULL, FALSE);
	g_return_val_if_fail (v_int != NULL, FALSE);

	*v_int = 0;
	errno = 0;

	val = strtol (str, &ptr, 0);

	if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0))
		return FALSE;

	if (str == ptr)
		return FALSE;

	*v_int = val;

	return TRUE;
}

static DBusGProxy*
pk_channel_proxy_new (PkConnection *connection,
                      gint          channel_id)
{
	DBusGProxy      *proxy;
	DBusGConnection *conn;
	gchar           *path;

	conn = PK_CONNECTION_DBUS (connection)->priv->dbus;
	path = g_strdup_printf ("/com/dronelabs/Perfkit/Channels/%d",
	                        channel_id);
	proxy = dbus_g_proxy_new_for_name (conn,
	                                   "com.dronelabs.Perfkit",
	                                   path,
	                                   "com.dronelabs.Perfkit.Channel");
	g_free (path);

	return proxy;
}

static DBusGProxy*
pk_source_proxy_new (PkConnection *connection,
                     gint          source_id)
{
	DBusGProxy      *proxy;
	DBusGConnection *conn;
	gchar           *path;

	conn = PK_CONNECTION_DBUS (connection)->priv->dbus;
	path = g_strdup_printf ("/com/dronelabs/Perfkit/Sources/%d",
	                        source_id);
	proxy = dbus_g_proxy_new_for_name (conn,
	                                   "com.dronelabs.Perfkit", path,
	                                   "com.dronelabs.Perfkit.Source");
	g_free (path);

	return proxy;
}

static gboolean
pk_connection_is_connected_locked (PkConnection *connection)
{
	return (PK_CONNECTION_DBUS (connection)->priv->dbus != NULL);
}

#define HAS_INTERFACE(m,i)   (g_strcmp0(i, dbus_message_get_interface(m)) == 0)
#define IS_MEMBER_NAMED(m,i) (g_strcmp0(i, dbus_message_get_member(m)) == 0)
#define IS_SIGNATURE(m,i)    (g_strcmp0(i, dbus_message_get_signature(m)) == 0)

enum
{
	MESSAGE_UNKNOWN,
	MESSAGE_DELIVER,
};

static gint
get_message_type (DBusMessage *message)
{
	if (IS_MEMBER_NAMED (message, "Deliver"))
		return MESSAGE_DELIVER;
	return MESSAGE_UNKNOWN;
}

static DBusHandlerResult
handle_deliver (DBusConnection   *connection,
                DBusMessage      *message,
                PkConnectionDBus *self)
{
	PkConnectionDBusPrivate  *priv;
	DBusMessageIter           iter,
	                          array;
	const gchar              *data   = NULL;
	gint                      len    = 0,
	                          sub_id = 0;
	PkSample                 *sample;
	gchar                   **decomp = NULL;
	Subscription             *sub;

	if (!IS_SIGNATURE (message, "ay") ||
	    (!dbus_message_get_path_decomposed (message, &decomp)) ||
	    (g_strv_length (decomp) != 2) ||
	    (g_strcmp0 (decomp [0], "Subscriptions") != 0) ||
	    (!pk_util_parse_int (decomp [1], &sub_id)))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	priv = self->priv;

	dbus_message_iter_init (message, &iter);
	dbus_message_iter_recurse (&iter, &array);
	dbus_message_iter_get_fixed_array (&array, &data, &len);

	/* create and deliver the sample */
	sample = pk_sample_new_from_data (data, len);
	g_static_rw_lock_reader_lock (&priv->rw_lock);
	if (NULL != (sub = g_hash_table_lookup (priv->subscriptions, &sub_id)))
		sub->callback (sample, sub->user_data);
	g_static_rw_lock_reader_unlock (&priv->rw_lock);
	pk_sample_unref (sample);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
subscription_message_func (DBusConnection *connection,
                           DBusMessage    *message,
                           gpointer        user_data)
{
	if (!HAS_INTERFACE (message, "com.dronelabs.Perfkit.Subscription"))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	switch (get_message_type (message)) {
	case MESSAGE_DELIVER:
		return handle_deliver (connection, message, user_data);
	default:
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}
}

static DBusObjectPathVTable subscription_vtable = {
	NULL,                      /* Unregister Function */
	subscription_message_func, /* Message Function    */
};

static void
pk_connection_dbus_accept_func (DBusServer     *server,
                                DBusConnection *conn,
                                gpointer        user_data)
{
	/* TODO:
	 *   Store the connection somewhere.
	 *   Provide a way to disconnect/close the connection
	 *   Authentication (maybe a unique cookie)?
	 */

	dbus_connection_ref (conn);
	dbus_connection_setup_with_g_main (conn, NULL);

	/*
	 * Register a handler for anything in the /Subscriptions handler.
	 * This allows us to have a single handler for incoming messages
	 * including samples and out-of-band data.
	 */

	dbus_connection_register_fallback (conn,
	                                   "/Subscriptions",
	                                   &subscription_vtable,
	                                   user_data);
}

static gboolean
pk_connection_dbus_real_is_connected (PkConnection *connection)
{
	g_return_val_if_fail (PK_IS_CONNECTION_DBUS (connection), FALSE);
	return (PK_CONNECTION_DBUS (connection)->priv->dbus != NULL);
}

static gboolean
pk_connection_dbus_real_connect (PkConnection  *connection,
                                 GError       **error)
{
	PkConnectionDBusPrivate *priv;
	gboolean                 success = FALSE;
	gchar                   *path;
	DBusError                path_error;

	g_return_val_if_fail (PK_IS_CONNECTION_DBUS (connection), FALSE);

	priv = PK_CONNECTION_DBUS (connection)->priv;

	g_static_rw_lock_writer_lock (&priv->rw_lock);

	if (pk_connection_is_connected_locked (connection)) {
		g_set_error (error, PK_CONNECTION_ERROR,
		             PK_CONNECTION_ERROR_INVALID,
		             _("The connection is already connected"));
	}
	else {
		/* TODO: Support multiple bus types */
		if (NULL != (priv->dbus = dbus_g_bus_get (DBUS_BUS_SESSION, error))) {
			priv->channels = dbus_g_proxy_new_for_name (priv->dbus,
			                                            "com.dronelabs.Perfkit",
			                                            "/com/dronelabs/Perfkit/Channels",
			                                            "com.dronelabs.Perfkit.Channels");
			priv->sources = dbus_g_proxy_new_for_name (priv->dbus,
			                                           "com.dronelabs.Perfkit",
			                                           "/com/dronelabs/Perfkit/Sources",
			                                           "com.dronelabs.Perfkit.Sources");

			success = TRUE;

			/* TODO: We should create a new server before each incoming
			 *       subscription and then remove the file.
			 */

			/*
			 * Setup private DBUS channel for receiving streaming samples
			 * from perfkit-daemon instances.
			 */

			path = g_strdup_printf ("unix:path=/tmp/perfkit-%d.socket", getpid ());
			dbus_error_init (&path_error);

			if (!(priv->server = dbus_server_listen (path, &path_error))) {
				g_warning ("Cannot listen on %s: %s: %s",
				           path, path_error.name, path_error.message);
				g_warning ("Receiving data samples will not be available");
				dbus_error_free (&path_error);
			}
			else {
				dbus_server_set_auth_mechanisms (priv->server, NULL);
				dbus_server_set_new_connection_function (priv->server,
				                                         pk_connection_dbus_accept_func,
				                                         g_object_ref (connection),
				                                         (DBusFreeFunction)g_object_unref);
				dbus_server_setup_with_g_main (priv->server, NULL);
			}

			g_free (path);
		}
		else {
			g_set_error (error, PK_CONNECTION_ERROR,
			             PK_CONNECTION_ERROR_INVALID,
			             _("Could not connect to DBUS"));
		}
	}

	g_static_rw_lock_writer_unlock (&priv->rw_lock);

	return success;
}

static void
pk_connection_dbus_real_connect_async (PkConnection        *connection,
                                       GAsyncReadyCallback  callback,
                                       gpointer             user_data)
{
	GSimpleAsyncResult *result;

	g_return_if_fail (PK_IS_CONNECTION_DBUS (connection));
	g_return_if_fail (callback != NULL);

	result = g_simple_async_result_new (G_OBJECT (connection),
	                                    callback,
	                                    user_data,
	                                    pk_connection_dbus_real_connect_async);
	g_simple_async_result_complete (result);
	g_object_unref (result);
}

static gboolean
pk_connection_dbus_real_connect_finish (PkConnection  *connection,
                                        GAsyncResult  *result,
                                        GError       **error)
{
	g_return_val_if_fail (PK_IS_CONNECTION_DBUS (connection), FALSE);
	g_return_val_if_fail (g_simple_async_result_is_valid (result,
	                                                      G_OBJECT (connection),
	                                                      pk_connection_dbus_real_connect_async),
	                      FALSE);

	return pk_connection_dbus_real_connect (connection, error);
}

static void
pk_connection_dbus_real_disconnect (PkConnection *connection)
{
	PkConnectionDBusPrivate *priv;

	g_return_if_fail (PK_IS_CONNECTION_DBUS (connection));

	priv = PK_CONNECTION_DBUS (connection)->priv;

	g_static_rw_lock_writer_lock (&priv->rw_lock);

	if (pk_connection_is_connected_locked (connection)) {
		dbus_g_connection_unref (priv->dbus);
		priv->dbus = NULL;

		if (priv->channels)
			g_object_unref (priv->channels);
		priv->channels = NULL;

		if (priv->sources)
			g_object_unref (priv->sources);
		priv->sources = NULL;
	}

	g_static_rw_lock_writer_unlock (&priv->rw_lock);
}

static void
pk_connection_dbus_real_disconnect_async (PkConnection        *connection,
                                          GAsyncReadyCallback  callback,
                                          gpointer             user_data)
{
	GSimpleAsyncResult *result;

	g_return_if_fail (PK_IS_CONNECTION_DBUS (connection));
	g_return_if_fail (callback != NULL);

	result = g_simple_async_result_new (G_OBJECT (connection),
	                                    callback,
	                                    user_data,
	                                    pk_connection_dbus_real_disconnect_async);
	g_simple_async_result_complete (result);
	g_object_unref (result);
}

static void
pk_connection_dbus_real_disconnect_finish (PkConnection *connection,
                                           GAsyncResult *result)
{
	g_return_if_fail (PK_IS_CONNECTION_DBUS (connection));
	g_return_if_fail (g_simple_async_result_is_valid (result,
	                                                  G_OBJECT (connection),
	                                                  pk_connection_dbus_real_disconnect_async));

	pk_connection_dbus_real_disconnect (connection);
}

static gboolean
pk_connection_dbus_real_channels_add (PkConnection  *connection,
                                      gint          *channel_id,
                                      GError       **error)
{
	PkConnectionDBusPrivate *priv;
	gchar                   *path   = NULL,
	                        *tmp;
	gboolean                 result = FALSE;

	g_return_val_if_fail (PK_IS_CONNECTION_DBUS (connection), FALSE);
	g_return_val_if_fail (channel_id != NULL, FALSE);

	priv = PK_CONNECTION_DBUS (connection)->priv;

	if (!com_dronelabs_Perfkit_Channels_add (priv->channels, &path, error))
		return FALSE;

	if (g_str_has_prefix (path, "/com/dronelabs/Perfkit/Channels/")) {
		tmp = g_strrstr (path, "/");
		if (tmp)
			tmp++;
		errno = 0;
		*channel_id = strtol (tmp, NULL, 0);
		result = (errno == 0);
	}

	g_free (path);

	if (!result) {
		g_set_error (error, PK_CONNECTION_ERROR,
		             PK_CONNECTION_ERROR_INVALID,
		             _("Invalid object path returned from DBUS"));
	}

	return result;
}

static gboolean
pk_connection_dbus_real_channels_find_all (PkConnection  *connection,
                                           gint         **channel_ids,
                                           gint          *n_channels)
{
	PkConnectionDBusPrivate  *priv;
	GPtrArray                *paths = NULL;
	gboolean                  result;
	gint                      i,
	                          id;
	const gchar              *str;

	g_return_val_if_fail (channel_ids != NULL, FALSE);
	g_return_val_if_fail (n_channels != NULL, FALSE);

	priv = PK_CONNECTION_DBUS (connection)->priv;

	if (!pk_connection_is_connected_locked (connection)) {
		g_warning (_("Connection not connected.  Please connect first."));
		return FALSE;
	}

	result = com_dronelabs_Perfkit_Channels_find_all (priv->channels, &paths, NULL);
	*n_channels = 0;

	if (result) {
		*channel_ids = g_malloc0 (sizeof (gint) * paths->len);
		for (i = 0; i < paths->len; i++) {
			str = g_strrstr (g_ptr_array_index (paths, i), "/");
			if (str) {
				str++;
				errno = 0;
				id = strtol (str, NULL, 0);
				if (errno == 0)
					(*channel_ids) [(*n_channels)++] = id;
			}
		}
	}

	if (paths)
		g_ptr_array_unref (paths);

	return result;
}

static gchar*
pk_connection_dbus_real_channel_get_target (PkConnection *connection,
                                            gint          channel_id)
{
	PkConnectionDBusPrivate *priv;
	DBusGProxy              *proxy;
	gchar                   *target = NULL;

	g_return_val_if_fail (PK_IS_CONNECTION_DBUS (connection), NULL);

	priv = PK_CONNECTION_DBUS (connection)->priv;

	proxy = pk_channel_proxy_new (connection, channel_id);
	com_dronelabs_Perfkit_Channel_get_target (proxy, &target, NULL);
	g_object_unref (proxy);

	return target;
}

static void
pk_connection_dbus_real_channel_set_target (PkConnection *connection,
                                            gint          channel_id,
                                            const gchar  *target)
{
	PkConnectionDBusPrivate *priv;
	DBusGProxy              *proxy;

	g_return_if_fail (PK_IS_CONNECTION_DBUS (connection));

	priv = PK_CONNECTION_DBUS (connection)->priv;

	proxy = pk_channel_proxy_new (connection, channel_id);
	com_dronelabs_Perfkit_Channel_set_target (proxy, target, NULL);
	g_object_unref (proxy);
}

static gchar**
pk_connection_dbus_real_channel_get_args (PkConnection *connection,
                                          gint          channel_id)
{
	PkConnectionDBusPrivate  *priv;
	DBusGProxy               *proxy;
	gchar                   **args = NULL;

	g_return_val_if_fail (PK_IS_CONNECTION_DBUS (connection), NULL);

	priv = PK_CONNECTION_DBUS (connection)->priv;

	proxy = pk_channel_proxy_new (connection, channel_id);
	com_dronelabs_Perfkit_Channel_get_args (proxy, &args, NULL);
	g_object_unref (proxy);

	return args;
}

static void
pk_connection_dbus_real_channel_set_args (PkConnection  *connection,
                                          gint           channel_id,
                                          gchar        **args)
{
	PkConnectionDBusPrivate *priv;
	DBusGProxy              *proxy;

	g_return_if_fail (PK_IS_CONNECTION_DBUS (connection));

	priv = PK_CONNECTION_DBUS (connection)->priv;

	proxy = pk_channel_proxy_new (connection, channel_id);
	com_dronelabs_Perfkit_Channel_set_args (proxy, (const gchar**)args, NULL);
	g_object_unref (proxy);
}

static gchar*
pk_connection_dbus_real_channel_get_dir (PkConnection *connection,
                                         gint          channel_id)
{
	PkConnectionDBusPrivate *priv;
	DBusGProxy              *proxy;
	gchar                   *dir = NULL;

	g_return_val_if_fail (PK_IS_CONNECTION_DBUS (connection), NULL);

	priv = PK_CONNECTION_DBUS (connection)->priv;

	proxy = pk_channel_proxy_new (connection, channel_id);
	com_dronelabs_Perfkit_Channel_get_dir (proxy, &dir, NULL);
	g_object_unref (proxy);

	return dir;
}

static void
pk_connection_dbus_real_channel_set_dir (PkConnection *connection,
                                         gint          channel_id,
                                         const gchar  *dir)
{
	PkConnectionDBusPrivate *priv;
	DBusGProxy              *proxy;

	g_return_if_fail (PK_IS_CONNECTION_DBUS (connection));

	priv = PK_CONNECTION_DBUS (connection)->priv;

	proxy = pk_channel_proxy_new (connection, channel_id);
	com_dronelabs_Perfkit_Channel_set_dir (proxy, dir, NULL);
	g_object_unref (proxy);
}

static gchar**
pk_connection_dbus_real_channel_get_env (PkConnection *connection,
                                         gint          channel_id)
{
	PkConnectionDBusPrivate  *priv;
	DBusGProxy               *proxy;
	gchar                   **env = NULL;

	g_return_val_if_fail (PK_IS_CONNECTION_DBUS (connection), NULL);

	priv = PK_CONNECTION_DBUS (connection)->priv;

	proxy = pk_channel_proxy_new (connection, channel_id);
	com_dronelabs_Perfkit_Channel_get_env (proxy, &env, NULL);
	g_object_unref (proxy);

	return env;
}

static void
pk_connection_dbus_real_channel_set_env (PkConnection  *connection,
                                         gint           channel_id,
                                         gchar        **env)
{
	PkConnectionDBusPrivate *priv;
	DBusGProxy              *proxy;

	g_return_if_fail (PK_IS_CONNECTION_DBUS (connection));

	priv = PK_CONNECTION_DBUS (connection)->priv;

	proxy = pk_channel_proxy_new (connection, channel_id);
	com_dronelabs_Perfkit_Channel_set_env (proxy, (const gchar**)env, NULL);
	g_object_unref (proxy);
}

static GPid
pk_connection_dbus_real_channel_get_pid (PkConnection *connection,
                                         gint          channel_id)
{
	PkConnectionDBusPrivate *priv;
	DBusGProxy              *proxy;
	GPid                     pid = 0;

	g_return_val_if_fail (PK_IS_CONNECTION_DBUS (connection), 0);

	priv = PK_CONNECTION_DBUS (connection)->priv;

	proxy = pk_channel_proxy_new (connection, channel_id);
	com_dronelabs_Perfkit_Channel_get_pid (proxy, (guint*)&pid, NULL);
	g_object_unref (proxy);

	return pid;
}

static void
pk_connection_dbus_real_channel_set_pid (PkConnection *connection,
                                         gint          channel_id,
                                         GPid          pid)
{
	PkConnectionDBusPrivate *priv;
	DBusGProxy              *proxy;

	g_return_if_fail (PK_IS_CONNECTION_DBUS (connection));

	priv = PK_CONNECTION_DBUS (connection)->priv;

	proxy = pk_channel_proxy_new (connection, channel_id);
	com_dronelabs_Perfkit_Channel_set_pid (proxy, pid, NULL);
	g_object_unref (proxy);
}

static PkChannelState
pk_connection_dbus_real_channel_get_state (PkConnection *connection,
                                           gint          channel_id)
{
	PkConnectionDBusPrivate *priv;
	DBusGProxy              *proxy;
	guint                    state = 0;

	g_return_val_if_fail (PK_IS_CONNECTION_DBUS (connection), 0);

	priv = PK_CONNECTION_DBUS (connection)->priv;

	proxy = pk_channel_proxy_new (connection, channel_id);
	com_dronelabs_Perfkit_Channel_get_state (proxy, (guint*)&state, NULL);
	g_object_unref (proxy);

	return state;
}

static gboolean
pk_connection_dbus_real_channel_start (PkConnection  *connection,
                                       gint           channel_id,
                                       GError       **error)
{
	PkConnectionDBusPrivate *priv;
	DBusGProxy              *proxy;
	gboolean                 result;

	g_return_val_if_fail (PK_IS_CONNECTION_DBUS (connection), 0);

	priv = PK_CONNECTION_DBUS (connection)->priv;

	proxy = pk_channel_proxy_new (connection, channel_id);
	result = com_dronelabs_Perfkit_Channel_start (proxy, error);
	g_object_unref (proxy);

	return result;
}

static gboolean
pk_connection_dbus_real_channel_stop (PkConnection  *connection,
                                      gint           channel_id,
                                      GError       **error)
{
	PkConnectionDBusPrivate *priv;
	DBusGProxy              *proxy;
	gboolean                 result;

	g_return_val_if_fail (PK_IS_CONNECTION_DBUS (connection), 0);

	priv = PK_CONNECTION_DBUS (connection)->priv;

	proxy = pk_channel_proxy_new (connection, channel_id);
	result = com_dronelabs_Perfkit_Channel_stop (proxy, error);
	g_object_unref (proxy);

	return result;
}

static gboolean
pk_connection_dbus_real_channel_pause (PkConnection  *connection,
                                       gint           channel_id,
                                       GError       **error)
{
	PkConnectionDBusPrivate *priv;
	DBusGProxy              *proxy;
	gboolean                 result;

	g_return_val_if_fail (PK_IS_CONNECTION_DBUS (connection), 0);

	priv = PK_CONNECTION_DBUS (connection)->priv;

	proxy = pk_channel_proxy_new (connection, channel_id);
	result = com_dronelabs_Perfkit_Channel_pause (proxy, error);
	g_object_unref (proxy);

	return result;
}

static gboolean
pk_connection_dbus_real_channel_unpause (PkConnection  *connection,
                                         gint           channel_id,
                                         GError       **error)
{
	PkConnectionDBusPrivate *priv;
	DBusGProxy              *proxy;
	gboolean                 result;

	g_return_val_if_fail (PK_IS_CONNECTION_DBUS (connection), 0);

	priv = PK_CONNECTION_DBUS (connection)->priv;

	proxy = pk_channel_proxy_new (connection, channel_id);
	result = com_dronelabs_Perfkit_Channel_unpause (proxy, error);
	g_object_unref (proxy);

	return result;
}

static void
pk_connection_dbus_real_channel_subscribe (PkConnection     *connection,
                                           gint              channel_id,
                                           PkSampleCallback  callback,
                                           gpointer          user_data)
{
	PkConnectionDBusPrivate *priv;
	DBusGProxy              *proxy;
	gchar                   *path;
	Subscription            *sub;

	g_return_if_fail (PK_IS_CONNECTION_DBUS (connection));

	priv = PK_CONNECTION_DBUS (connection)->priv;

	/*
	 * Subscriptions work by creating a new service on our private DBUS
	 * server.  We then send the socket address and path on this Bus to
	 * the perfkit-daemon.  The daemon then connects back to our Bus and
	 * sends samples directly to our service.
	 */

	/* create subscription */
	sub = g_slice_new (Subscription);
	sub->subscription_id = g_atomic_int_exchange_and_add (&subscription_seq, 1);
	sub->callback = callback;
	sub->user_data = user_data;

	/* save subscription for lookups */
	g_static_rw_lock_writer_lock (&priv->rw_lock);
	g_hash_table_insert (priv->subscriptions, &sub->subscription_id, sub);
	g_static_rw_lock_writer_unlock (&priv->rw_lock);

	/* TODO: Prepare socket for this subscription so we can remove it
	 *       after they have connected back.
	 */

	/* notify the daemon of the subscription */
	path = g_strdup_printf ("/Subscriptions/%d", sub->subscription_id);
	proxy = pk_channel_proxy_new (connection, channel_id);
	com_dronelabs_Perfkit_Channel_subscribe (proxy,
	                                         dbus_server_get_address (priv->server),
	                                         path, NULL);
	g_object_unref (proxy);
	g_free (path);
}

static gchar**
pk_connection_dbus_real_sources_get_types (PkConnection *connection)
{
	PkConnectionDBusPrivate  *priv;
	gchar                   **types = NULL;

	g_return_val_if_fail (PK_IS_CONNECTION_DBUS (connection), 0);

	priv = PK_CONNECTION_DBUS (connection)->priv;

	com_dronelabs_Perfkit_Sources_get_types (priv->sources, &types, NULL);

	return types;
}

static gboolean
pk_connection_dbus_real_sources_add (PkConnection  *connection,
                                     const gchar   *type,
                                     gint          *source_id,
                                     GError       **error)
{
	PkConnectionDBusPrivate  *priv;
	gchar                    *path   = NULL,
	                         *tmp;
	gboolean                  result = FALSE;

	g_return_val_if_fail (PK_IS_CONNECTION_DBUS (connection), FALSE);
	g_return_val_if_fail (source_id != NULL, FALSE);

	priv = PK_CONNECTION_DBUS (connection)->priv;

	*source_id = 0;

	if (!com_dronelabs_Perfkit_Sources_add (priv->sources, type,
	                                        &path, error)) {
	    return FALSE;
	}

	tmp = g_strrstr (path, "/");
	if (tmp) {
		tmp++;
		if (!(result = pk_util_parse_int (tmp, source_id))) {
			g_set_error (error, PK_CONNECTION_ERROR,
			             PK_CONNECTION_ERROR_INVALID,
			             "An invalid path was returned");
		}
	}
	g_free (path);

	return result;
}

static void
pk_connection_dbus_real_source_set_channel (PkConnection *connection,
                                            gint          source_id,
                                            gint          channel_id)
{
	PkConnectionDBusPrivate  *priv;
	gchar                    *path;
	DBusGProxy               *proxy;

	g_return_if_fail (PK_IS_CONNECTION_DBUS (connection));

	priv = PK_CONNECTION_DBUS (connection)->priv;

	proxy = pk_source_proxy_new (connection, source_id);
	path = g_strdup_printf ("/com/dronelabs/Perfkit/Channels/%d",
	                        channel_id);

	com_dronelabs_Perfkit_Source_set_channel (proxy, path, NULL);

	g_free (path);
	g_object_unref (proxy);
}

static void
subscription_free (gpointer data)
{
	g_slice_free (Subscription, data);
}

static void
pk_connection_dbus_finalize (GObject *object)
{
	G_OBJECT_CLASS (pk_connection_dbus_parent_class)->finalize (object);
}

static void
pk_connection_dbus_class_init (PkConnectionDBusClass *klass)
{
	GObjectClass      *object_class;
	PkConnectionClass *conn_class;

	object_class           = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_connection_dbus_finalize;
	g_type_class_add_private (object_class, sizeof (PkConnectionDBusPrivate));

	conn_class                     = PK_CONNECTION_CLASS (klass);
	conn_class->connect            = pk_connection_dbus_real_connect;
	conn_class->connect_finish     = pk_connection_dbus_real_connect_finish;
	conn_class->connect_finish     = pk_connection_dbus_real_connect_finish;
	conn_class->disconnect         = pk_connection_dbus_real_disconnect;
	conn_class->disconnect_async   = pk_connection_dbus_real_disconnect_async;
	conn_class->disconnect_finish  = pk_connection_dbus_real_disconnect_finish;
	conn_class->is_connected       = pk_connection_dbus_real_is_connected;
	conn_class->channels_add       = pk_connection_dbus_real_channels_add;
	conn_class->channels_find_all  = pk_connection_dbus_real_channels_find_all;
	conn_class->channel_get_target = pk_connection_dbus_real_channel_get_target;
	conn_class->channel_set_target = pk_connection_dbus_real_channel_set_target;
	conn_class->channel_get_args   = pk_connection_dbus_real_channel_get_args;
	conn_class->channel_set_args   = pk_connection_dbus_real_channel_set_args;
	conn_class->channel_get_dir    = pk_connection_dbus_real_channel_get_dir;
	conn_class->channel_set_dir    = pk_connection_dbus_real_channel_set_dir;
	conn_class->channel_get_env    = pk_connection_dbus_real_channel_get_env;
	conn_class->channel_set_env    = pk_connection_dbus_real_channel_set_env;
	conn_class->channel_get_pid    = pk_connection_dbus_real_channel_get_pid;
	conn_class->channel_set_pid    = pk_connection_dbus_real_channel_set_pid;
	conn_class->channel_get_state  = pk_connection_dbus_real_channel_get_state;
	conn_class->channel_start      = pk_connection_dbus_real_channel_start;
	conn_class->channel_stop       = pk_connection_dbus_real_channel_stop;
	conn_class->channel_pause      = pk_connection_dbus_real_channel_pause;
	conn_class->channel_unpause    = pk_connection_dbus_real_channel_unpause;
	conn_class->channel_subscribe  = pk_connection_dbus_real_channel_subscribe;
	conn_class->sources_get_types  = pk_connection_dbus_real_sources_get_types;
	conn_class->sources_add        = pk_connection_dbus_real_sources_add;
	conn_class->source_set_channel = pk_connection_dbus_real_source_set_channel;
}

static void
pk_connection_dbus_init (PkConnectionDBus *connection)
{
	connection->priv = G_TYPE_INSTANCE_GET_PRIVATE (connection,
	                                                PK_TYPE_CONNECTION_DBUS,
	                                                PkConnectionDBusPrivate);

	g_static_rw_lock_init (&connection->priv->rw_lock);

	connection->priv->subscriptions =
		g_hash_table_new_full (g_int_hash, g_int_equal, NULL,
		                       subscription_free);
}

PkConnection*
pk_connection_dbus_new (void)
{
	return g_object_new (PK_TYPE_CONNECTION_DBUS, NULL);
}
