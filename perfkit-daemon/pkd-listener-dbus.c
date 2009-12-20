/* pkd-listener-dbus.c
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdlib.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "pkd-config.h"
#include "pkd-channel.h"
#include "pkd-channel-glue.h"
#include "pkd-channel-dbus.h"
#include "pkd-channel-priv.h"
#include "pkd-channels.h"
#include "pkd-channels-glue.h"
#include "pkd-channels-dbus.h"
#include "pkd-listener-dbus.h"
#include "pkd-runtime.h"
#include "pkd-source.h"
#include "pkd-source-glue.h"
#include "pkd-source-dbus.h"
#include "pkd-sources.h"
#include "pkd-sources-glue.h"
#include "pkd-sources-dbus.h"
#include "pkd-source-info.h"
#include "pkd-source-info-glue.h"
#include "pkd-source-info-dbus.h"

static void pkd_listener_dbus_init_listener (PkdListenerIface *iface);

G_DEFINE_TYPE_EXTENDED (PkdListenerDBus, pkd_listener_dbus, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (PKD_TYPE_LISTENER,
                                               pkd_listener_dbus_init_listener));

/**
 * SECTION:pkd-listener_dbus
 * @title: PkdListenerDBus
 * @short_description: 
 *
 * 
 */

static DBusGConnection *conn = NULL;

/*
 *-----------------------------------------------------------------------------
 *
 * Public Methods
 *
 *-----------------------------------------------------------------------------
 */

/**
 * pkd_listener_dbus_register:
 *
 * Checks to see if configuration specifies the DBus listener should be
 * running.  If so, the #PkdListenerDBus will be instantiated and registered
 * with the runtime.
 *
 * Side effects:
 *       None.
 */
void
pkd_listener_dbus_register (void)
{
	PkdListener *listener = NULL;
	gboolean disabled;

	g_return_if_fail (listener == NULL);

	/*
	 * Make sure the DBus listener is not disabled.
	 */

	disabled = pkd_config_get_boolean_default ("dbus", "disable", FALSE);
	if (disabled) {
		g_message ("DBUS listener is diabled. Not starting.");
		return;
	}

	listener = g_object_new (PKD_TYPE_LISTENER_DBUS, NULL);
	pkd_runtime_add_listener (listener);
	g_message ("%s: DBUS listener created.", G_STRFUNC);
}

/*
 *-----------------------------------------------------------------------------
 *
 * Listener Methods
 *
 *-----------------------------------------------------------------------------
 */

static void
channel_added_cb (PkdChannels     *channels,
                  PkdChannel      *channel,
                  PkdListenerDBus *dbus)
{
	gchar *path;

	path = g_strdup_printf ("/com/dronelabs/Perfkit/Channels/%d",
	                        pkd_channel_get_id (channel));
	dbus_g_connection_register_g_object (conn, path, G_OBJECT (channel));
	g_free (path);
}

static void
source_added_cb (PkdSources      *sources,
                 PkdSource       *source,
                 PkdListenerDBus *dbus)
{
	gchar *path;

	path = g_strdup_printf ("/com/dronelabs/Perkit/Sources/%d",
	                        pkd_source_get_id (source));
	dbus_g_connection_register_g_object (conn, path, G_OBJECT (source));
	g_free (path);
}

static void
source_info_added_cb (PkdSources      *sources,
                      PkdSourceInfo   *source_info,
                      PkdListenerDBus *dbus)
{
	gchar *path;

	path = g_strdup_printf ("/com/dronelabs/Perkit/SourceInfo/%s",
	                        pkd_source_info_get_uid (source_info));
	dbus_g_connection_register_g_object (conn, path, G_OBJECT (source_info));
	g_free (path);
}

static gboolean
pkd_listener_dbus_real_listen (PkdListener  *listener,
                               GError      **error)
{
	PkdListenerDBusPrivate *priv;
	gboolean sysbus;
	DBusBusType bus_type;
	GError *local_error = NULL;
	PkdService *channels, *sources;
	GList *list, *iter;
	gchar *path;

	g_return_val_if_fail (PKD_IS_LISTENER_DBUS (listener), FALSE);

	priv = PKD_LISTENER_DBUS (listener)->priv;

	g_assert (!conn);

	sysbus = pkd_config_get_boolean_default ("dbus", "system", FALSE);
	bus_type = sysbus ? DBUS_BUS_SYSTEM : DBUS_BUS_SESSION;

	/*
	 * Connect to the proper DBus.
	 */

	if (!(conn = dbus_g_bus_get (bus_type, &local_error))) {
		g_warning ("%s: Error connecting to DBus: %s",
		           G_STRFUNC, local_error->message);
		if (error) {
			*error = local_error;
		}
		return FALSE;
	}

	/*
	 * Register DBus types for required Objects.
	 */
	dbus_g_object_type_install_info (PKD_TYPE_CHANNEL, &dbus_glib_pkd_channel_object_info);
	dbus_g_object_type_install_info (PKD_TYPE_CHANNELS, &dbus_glib_pkd_channels_object_info);
	dbus_g_object_type_install_info (PKD_TYPE_SOURCE, &dbus_glib_pkd_source_object_info);
	dbus_g_object_type_install_info (PKD_TYPE_SOURCES, &dbus_glib_pkd_sources_object_info);
	dbus_g_object_type_install_info (PKD_TYPE_SOURCE_INFO, &dbus_glib_pkd_source_info_object_info);

	/*
	 * Try to request our BusName.  If another exists, there is already an
	 * instance of perfkit-daemon running.  In that case, we should probably
	 * exit.
	 */
	if (dbus_bus_request_name (dbus_g_connection_get_connection (conn),
	                           "com.dronelabs.Perfkit",
	                           DBUS_NAME_FLAG_DO_NOT_QUEUE,
	                           NULL) == DBUS_REQUEST_NAME_REPLY_EXISTS) {
		g_warning ("Error requesting BusName com.dronelabs.Perfkit. "
		           "Perhaps there is another instance of perfkit-daemon");
		goto error;
	}

	/*
	 * Register signals for adding new items to the DBus.
	 */

	sources = pkd_runtime_get_service ("Sources");
	channels = pkd_runtime_get_service ("Channels");
	g_assert (sources);
	g_assert (channels);

	g_signal_connect (channels,
	                  "channel-added",
	                  G_CALLBACK (channel_added_cb),
	                  listener);
	g_signal_connect (sources,
	                  "source-added",
	                  G_CALLBACK (source_added_cb),
	                  listener);
	g_signal_connect (sources,
	                  "source-info-added",
	                  G_CALLBACK (source_info_added_cb),
	                  listener);

	/*
	 * Register each of the currently registered source types.
	 */

	list = pkd_sources_get_registered (PKD_SOURCES (sources));

	for (iter = list; iter; iter = iter->next) {
		path = g_strdup_printf ("/com/dronelabs/Perfkit/SourceInfo/%s",
		                        pkd_source_info_get_uid (iter->data));
		
		dbus_g_connection_register_g_object (conn, path, G_OBJECT (iter->data));
		g_free (path);
		g_object_unref (iter->data);
	}

	list = NULL;
	path = NULL;
	g_list_free (list);

	/*
	 * Register services on the DBus.
	 */

	dbus_g_connection_register_g_object (conn,
	                                     "/com/dronelabs/Perfkit/Sources",
	                                     G_OBJECT (sources));
	dbus_g_connection_register_g_object (conn,
	                                     "/com/dronelabs/Perfkit/Channels",
	                                     G_OBJECT (channels));

	return TRUE;

error:
	dbus_g_connection_unref (conn);
	return FALSE;
}

/*
 *-----------------------------------------------------------------------------
 *
 * DBus Handler Methods
 *
 *-----------------------------------------------------------------------------
 */

static gboolean
pkd_channel_get_target_dbus (PkdChannel  *channel,
                             gchar      **target,
                             GError     **error)
{
	g_return_val_if_fail (PKD_IS_CHANNEL (channel), FALSE);
	g_return_val_if_fail (target != NULL, FALSE);
	*target = pkd_channel_get_target (channel);
	return TRUE;
}

static gboolean
pkd_channel_set_target_dbus (PkdChannel  *channel,
                             gchar       *target,
                             GError     **error)
{
	g_return_val_if_fail (PKD_IS_CHANNEL (channel), FALSE);
	g_return_val_if_fail (target != NULL, FALSE);
	pkd_channel_set_target (channel, target);
	return TRUE;
}

static gboolean
pkd_channel_get_args_dbus (PkdChannel   *channel,
                           gchar      ***args,
                           GError      **error)
{
	g_return_val_if_fail (PKD_IS_CHANNEL (channel), FALSE);
	g_return_val_if_fail (args != NULL, FALSE);
	*args = pkd_channel_get_args (channel);
	return TRUE;
}

static gboolean
pkd_channel_set_args_dbus (PkdChannel  *channel,
                           gchar      **args,
                           GError     **error)
{
	g_return_val_if_fail (PKD_IS_CHANNEL (channel), FALSE);
	g_return_val_if_fail (args != NULL, FALSE);
	pkd_channel_set_args (channel, (const gchar**)args);
	return TRUE;
}

static gboolean
pkd_channel_get_pid_dbus (PkdChannel  *channel,
                          guint       *pid,
                          GError     **error)
{
	g_return_val_if_fail (PKD_IS_CHANNEL (channel), FALSE);
	g_return_val_if_fail (pid != NULL, FALSE);
	*pid = (guint)pkd_channel_get_pid (channel);
	return TRUE;
}

static gboolean
pkd_channel_set_pid_dbus (PkdChannel  *channel,
                          guint        pid,
                          GError     **error)
{
	g_return_val_if_fail (PKD_IS_CHANNEL (channel), FALSE);
	pkd_channel_set_pid (channel, (GPid)pid);
	return TRUE;
}

static gboolean
pkd_channel_get_dir_dbus (PkdChannel  *channel,
                          gchar      **dir,
                          GError     **error)
{
	g_return_val_if_fail (PKD_IS_CHANNEL (channel), FALSE);
	g_return_val_if_fail (dir != NULL, FALSE);
	*dir = pkd_channel_get_dir (channel);
	return TRUE;
}

static gboolean
pkd_channel_set_dir_dbus (PkdChannel  *channel,
                          gchar       *dir,
                          GError     **error)
{
	g_return_val_if_fail (PKD_IS_CHANNEL (channel), FALSE);
	g_return_val_if_fail (dir != NULL, FALSE);
	pkd_channel_set_dir (channel, dir);
	return TRUE;
}

static gboolean
pkd_channel_get_env_dbus (PkdChannel   *channel,
                          gchar      ***env,
                          GError      **error)
{
	g_return_val_if_fail (PKD_IS_CHANNEL (channel), FALSE);
	g_return_val_if_fail (env != NULL, FALSE);
	*env = pkd_channel_get_env (channel);
	return TRUE;
}

static gboolean
pkd_channel_set_env_dbus (PkdChannel  *channel,
                          gchar      **env,
                          GError     **error)
{
	g_return_val_if_fail (PKD_IS_CHANNEL (channel), FALSE);
	g_return_val_if_fail (env != NULL, FALSE);
	pkd_channel_set_env (channel, (const gchar**)env);
	return TRUE;
}

static gboolean
pkd_channel_get_state_dbus (PkdChannel  *channel,
                            guint       *v_uint,
                            GError     **error)
{
	g_return_val_if_fail (PKD_IS_CHANNEL (channel), FALSE);
	g_return_val_if_fail (v_uint != NULL, FALSE);
	*v_uint = pkd_channel_get_state (channel);
	return TRUE;
}

typedef struct {
	DBusGConnection *conn;
	DBusGProxy      *proxy;
	gint             sub_id;
} DBusSubscription;

static void
sample_cb (PkdChannel *channel,
           PkdSample  *sample,
           gpointer    user_data)
{
	DBusSubscription *sub = (DBusSubscription *)channel;

	g_assert (sample);
	g_assert (sub);

	dbus_g_proxy_call_no_reply (sub->proxy, "Deliver",
	                            dbus_g_type_get_collection ("GArray", G_TYPE_UCHAR),
	                            pkd_sample_get_array (sample),
	                            G_TYPE_INVALID,
	                            G_TYPE_INVALID);
}

static gboolean
pkd_channel_subscribe_dbus (PkdChannel   *channel,
                            const gchar  *address,
                            const gchar  *path,
                            GError      **error)
{
	PkdChannelPrivate *priv;
	DBusGConnection   *sub_conn;
	DBusGProxy        *proxy;
	DBusSubscription  *sub;

	g_return_val_if_fail (PKD_IS_CHANNEL (channel), FALSE);
	g_return_val_if_fail (address != NULL, FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	priv = channel->priv;

	/* connect to peers DBUS */
	if (!(sub_conn = dbus_g_connection_open (address, error)))
		return FALSE;

	/* get a proxy to their subscription listner */
	if (!(proxy = dbus_g_proxy_new_for_peer (
			sub_conn, path, "com.dronelabs.Perfkit.Subscription"))) {
		g_set_error (error, PKD_CHANNEL_ERROR, PKD_CHANNEL_ERROR_INVALID,
		             "Could not connect to subscription path");
		dbus_g_connection_unref (sub_conn);
		return FALSE;
	}

	/* store subscription for later */
	sub = g_slice_new0 (DBusSubscription);
	sub->conn = sub_conn;
	sub->proxy = proxy;
	sub->sub_id = pkd_channel_subscribe (channel, sample_cb, sub);

	return TRUE;
}

static gboolean
pkd_channels_add_dbus (PkdChannels  *channels,
                       gchar       **path,
                       GError      **error)
{
	PkdChannel *channel;

	g_return_val_if_fail (PKD_IS_CHANNELS (channels), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	channel = pkd_channels_add (channels);
	*path = g_strdup_printf (DBUS_PKD_CHANNELS_PREFIX "%d",
	                         pkd_channel_get_id (channel));

	return TRUE;
}

static gboolean
pkd_channels_remove_dbus (PkdChannels  *channels,
                          gchar        *path,
                          GError      **error)
{
	PkdChannelsPrivate *priv;
	PkdChannel         *channel;
	gint                id;

	g_return_val_if_fail (PKD_IS_CHANNELS (channels), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);
	g_return_val_if_fail (g_str_has_prefix (path, DBUS_PKD_CHANNELS_PREFIX), FALSE);

	priv = channels->priv;
	errno = 0;

	id = strtoll (path + strlen (DBUS_PKD_CHANNELS_PREFIX), NULL, 10);

	if (errno != 0) {
		g_set_error (error, PKD_CHANNELS_ERROR,
		             PKD_CHANNELS_ERROR_INVALID_CHANNEL,
		             "The requested path is invalid");
		return FALSE;
	}

	/*
	 * Retrieve the channel by id.  If we get a valid channel back, we own
	 * a reference.  First we remove it, then lose our reference (which could
	 * possibly free the object).
	 */
	channel = pkd_channels_get (channels, id);
	pkd_channels_remove (channels, channel);
	g_object_unref (channel);

	return TRUE;
}

static gboolean
pkd_channels_find_all_dbus (PkdChannels  *channels,
                            GPtrArray   **paths,
                            GError      **error)
{
	GList *list, *tmp;

	g_return_val_if_fail (PKD_IS_CHANNELS (channels), FALSE);
	g_return_val_if_fail (paths != NULL && *paths == NULL, FALSE);

	list = pkd_channels_find_all (channels);
	*paths = g_ptr_array_sized_new (g_list_length (list));

	for (tmp = list; tmp; tmp = tmp->next) {
		g_ptr_array_add ((*paths),
		                 g_strdup_printf (DBUS_PKD_CHANNELS_PREFIX "%d",
		                                  pkd_channel_get_id (tmp->data)));
	}

	g_list_foreach (list, (GFunc)g_object_unref, NULL);
	g_list_free (list);

	return TRUE;
}

static gboolean
pkd_source_get_channel_dbus (PkdSource  *source,
                             gchar     **path,
                             GError    **error)
{
	PkdSourcePrivate *priv;
	PkdChannel *channel;

	g_return_val_if_fail (path != NULL, FALSE);

	priv = source->priv;

	channel = pkd_source_get_channel (source);

	if (!channel)
		*path = g_strdup ("");
	else
		*path = g_strdup_printf ("/com/dronelabs/Perfkit/Channels/%d",
		                         pkd_channel_get_id (channel));

	if (channel)
		g_object_unref (channel);

	return TRUE;
}

static gboolean
pkd_source_set_channel_dbus (PkdSource    *source,
                             const gchar  *path,
                             GError      **error)
{
	PkdChannel *channel;

	g_return_val_if_fail (path != NULL, FALSE);

	channel = PKD_CHANNEL (dbus_g_connection_lookup_g_object (conn, path));
	if (!channel)
		return FALSE;

	pkd_source_set_channel (source, channel);

	return TRUE;
}

static gboolean
pkd_source_info_get_uid_dbus (PkdSourceInfo  *source_info,
                              gchar         **uid,
                              GError        **error)
{
	gchar *tmp;

	g_return_val_if_fail (PKD_IS_SOURCE_INFO (source_info), FALSE);
	g_return_val_if_fail (uid != NULL, FALSE);

	tmp = (gchar *)pkd_source_info_get_uid (source_info);
	g_assert (tmp);
	*uid = g_strdup (tmp);

	return TRUE;
}

static gboolean
pkd_source_info_create_dbus (PkdSourceInfo  *source_info,
                             gchar         **path,
                             GError        **error)
{
	PkdSource *source;

	g_return_val_if_fail (PKD_IS_SOURCE_INFO (source_info), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	source = pkd_source_info_create (source_info);
	if (!source)
		return FALSE; // TODO: Add error message

	*path = g_strdup_printf ("/com/dronelabs/Perfkit/Sources/%d",
	                         pkd_source_get_id (source));
	// TODO: Unref source? Is another reference stored?

	return TRUE;
}

static gboolean
pkd_sources_get_source_types_dbus (PkdSources   *sources,
                                   gchar      ***uids,
                                   GError      **error)
{
	GList *list, *iter;
	gchar **c;
	gint i;

	list = pkd_sources_get_registered (sources);
	c = g_malloc0 ((sizeof (gchar*)) * (g_list_length (list) + 1));

	for (iter = list, i = 0; iter; iter = iter->next, i++) {
		c[i] = g_strdup_printf ("/com/dronelabs/Perfkit/SourceInfo/%s",
		                        pkd_source_info_get_uid (iter->data));
	}

	*uids = c;

	return TRUE;
}

static gboolean
pkd_source_info_get_name_dbus (PkdSourceInfo  *info,
                               gchar         **name,
                               GError        **error)
{
	g_return_val_if_fail (PKD_IS_SOURCE_INFO (info), FALSE);
	g_return_val_if_fail (name != NULL, FALSE);

	(*name) = g_strdup (pkd_source_info_get_name (info));
	return TRUE;
}


/*
 *-----------------------------------------------------------------------------
 *
 * Class Methods
 *
 *-----------------------------------------------------------------------------
 */

static void
pkd_listener_dbus_finalize (GObject *object)
{
	if (conn) {
		dbus_g_connection_unref (conn);
	}

	G_OBJECT_CLASS (pkd_listener_dbus_parent_class)->finalize (object);
}

static void
pkd_listener_dbus_class_init (PkdListenerDBusClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkd_listener_dbus_finalize;
}

static void
pkd_listener_dbus_init (PkdListenerDBus *listener_dbus)
{
}

static void
pkd_listener_dbus_init_listener (PkdListenerIface *iface)
{
	iface->listen = pkd_listener_dbus_real_listen;
}
