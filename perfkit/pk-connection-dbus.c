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

#include "errno.h"
#include "stdlib.h"

#include <dbus/dbus-glib.h>
#include <glib/gi18n.h>

#include "pk-channel-dbus.h"
#include "pk-channels-dbus.h"
#include "pk-connection-dbus.h"

G_DEFINE_TYPE (PkConnectionDBus, pk_connection_dbus, PK_TYPE_CONNECTION)

static void     pk_connection_dbus_real_connect_async     (PkConnection         *connection,
                                                           GAsyncReadyCallback   callback,
                                                           gpointer              user_data);
static gboolean pk_connection_dbus_real_connect_finish    (PkConnection         *connection,
                                                           GAsyncResult         *result,
                                                           GError              **error);
static gboolean pk_connection_dbus_real_connect           (PkConnection         *connection,
                                                           GError              **error);
static void     pk_connection_dbus_real_disconnect        (PkConnection         *connection);
static void     pk_connection_dbus_real_disconnect_async  (PkConnection         *connection,
                                                           GAsyncReadyCallback   callback,
                                                           gpointer              user_data);
static void     pk_connection_dbus_real_disconnect_finish (PkConnection         *connection,
                                                           GAsyncResult         *result);
static gboolean pk_connection_dbus_real_channels_find_all (PkConnection         *connection,
                                                           gint                **channel_ids,
                                                           gint                 *n_channels);
static gchar*   pk_connection_dbus_real_channel_get_target(PkConnection         *connection,
                                                           gint                  channel_id);
static gchar**  pk_connection_dbus_real_channel_get_args  (PkConnection         *connection,
                                                           gint                  channel_id);
static gchar*   pk_connection_dbus_real_channel_get_dir   (PkConnection         *connection,
                                                           gint                  channel_id);
static gchar**  pk_connection_dbus_real_channel_get_env   (PkConnection         *connection,
                                                           gint                  channel_id);
static GPid     pk_connection_dbus_real_channel_get_pid   (PkConnection         *connection,
                                                           gint                  channel_id);
static PkChannelState
                pk_connection_dbus_real_channel_get_state (PkConnection         *connection,
                                                           gint                  channel_id);

struct _PkConnectionDBusPrivate
{
	GStaticRWLock    rw_lock;
	DBusGConnection *dbus;
	DBusGProxy      *channels;
	DBusGProxy      *sources;
};

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

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_connection_dbus_finalize;
	g_type_class_add_private (object_class, sizeof (PkConnectionDBusPrivate));

	conn_class = PK_CONNECTION_CLASS (klass);
	conn_class->connect = pk_connection_dbus_real_connect;
	conn_class->connect_finish = pk_connection_dbus_real_connect_finish;
	conn_class->connect_finish = pk_connection_dbus_real_connect_finish;
	conn_class->disconnect = pk_connection_dbus_real_disconnect;
	conn_class->disconnect_async = pk_connection_dbus_real_disconnect_async;
	conn_class->disconnect_finish = pk_connection_dbus_real_disconnect_finish;
	conn_class->channels_find_all = pk_connection_dbus_real_channels_find_all;
	conn_class->channel_get_target = pk_connection_dbus_real_channel_get_target;
	conn_class->channel_get_args = pk_connection_dbus_real_channel_get_args;
	conn_class->channel_get_dir = pk_connection_dbus_real_channel_get_dir;
	conn_class->channel_get_env = pk_connection_dbus_real_channel_get_env;
	conn_class->channel_get_pid = pk_connection_dbus_real_channel_get_pid;
	conn_class->channel_get_state = pk_connection_dbus_real_channel_get_state;
}

static void
pk_connection_dbus_init (PkConnectionDBus *connection)
{
	connection->priv = G_TYPE_INSTANCE_GET_PRIVATE (connection,
	                                                PK_TYPE_CONNECTION_DBUS,
	                                                PkConnectionDBusPrivate);
	g_static_rw_lock_init (&connection->priv->rw_lock);
}

PkConnection*
pk_connection_dbus_new (void)
{
	return g_object_new (PK_TYPE_CONNECTION_DBUS, NULL);
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
                      

static gboolean
pk_connection_dbus_real_connect (PkConnection  *connection,
                                 GError       **error)
{
	PkConnectionDBusPrivate *priv;
	gboolean                 success;

	g_return_val_if_fail (PK_IS_CONNECTION_DBUS (connection), FALSE);

	priv = PK_CONNECTION_DBUS (connection)->priv;

	g_static_rw_lock_writer_lock (&priv->rw_lock);

	if (priv->dbus) {
		g_set_error (error, PK_CONNECTION_ERROR,
		             PK_CONNECTION_ERROR_INVALID,
		             _("The connection is already connected"));
		success = FALSE;
	}
	else {
		/* TODO: Support multiple bus types */
		priv->dbus = dbus_g_bus_get (DBUS_BUS_SESSION, error);
		success = (priv->dbus != NULL);
	}

	if (success) {
		priv->channels = dbus_g_proxy_new_for_name (priv->dbus,
		                                            "com.dronelabs.Perfkit",
		                                            "/com/dronelabs/Perfkit/Channels",
		                                            "com.dronelabs.Perfkit.Channels");
		priv->sources = dbus_g_proxy_new_for_name (priv->dbus,
		                                           "com.dronelabs.Perfkit",
		                                           "/com/dronelabs/Perfkit/Sources",
		                                           "com.dronelabs.Perfkit.Sources");
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
	if (priv->dbus) {
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
pk_connection_dbus_real_disconnect_async  (PkConnection         *connection,
                                           GAsyncReadyCallback   callback,
                                           gpointer              user_data)
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
pk_connection_dbus_real_channels_find_all (PkConnection  *connection,
                                           gint         **channel_ids,
                                           gint          *n_channels)
{
	PkConnectionDBusPrivate  *priv;
	GPtrArray                *paths = NULL;
	gboolean                  result;
	gint                      i, id;
	gchar                    *str;

	g_return_val_if_fail (channel_ids != NULL, FALSE);
	g_return_val_if_fail (n_channels != NULL, FALSE);

	priv = PK_CONNECTION_DBUS (connection)->priv;

	if (!priv->channels) {
		g_warning (_("Connection not open.  Open connection first."));
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