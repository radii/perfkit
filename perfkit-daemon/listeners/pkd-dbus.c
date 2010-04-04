/* pkd-dbus.c
 *
 * Copyright (C) 2009 Christian Hergert
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

#include <gmodule.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <perfkit-daemon/perfkit-daemon.h>

#include "pkd-dbus.h"
#include "pkd-dbus-glue.h"
#include "pkd-dbus-manager.h"
#include "pkd-channel-glue.h"
#include "pkd-channel-dbus.h"
#include "pkd-encoder-info-dbus.h"
#include "pkd-source-info-dbus.h"
#include "pkd-source-dbus.h"

G_DEFINE_TYPE (PkdDBus, pkd_dbus, PKD_TYPE_LISTENER)

static DBusGConnection *dbus_conn = NULL;

struct _PkdDBusPrivate
{
	DBusGConnection *conn;
	PkdDBusManager  *manager;
};

DBusGConnection*
pkd_dbus_get_connection (void)
{
	return dbus_conn;
}

static gboolean
pkd_dbus_start(PkdListener  *listener,
               GError      **ret_error)
{
	PkdDBusPrivate *priv;
	GError *error = NULL;
	GList *list, *iter;
	gchar *path;

	g_return_val_if_fail(PKD_IS_DBUS(listener), FALSE);

	priv = PKD_DBUS(listener)->priv;

	/*
	 * Connect to the DBus Daemon.
	 */
	if (!(priv->conn = dbus_g_bus_get(DBUS_BUS_SESSION, &error))) {
		g_warning("%s: Error connecting to DBus: %s", G_STRLOC, error->message);
		if (ret_error) *ret_error = error;
		else g_error_free(error);
		return FALSE;
	}

	/*
	 * Store a pointer to the connection for dbus helpers to retrieve.
	 */
	dbus_conn = priv->conn;

	/*
	 * Request our known bus name.
	 */
	if (dbus_bus_request_name(dbus_g_connection_get_connection(priv->conn),
	                          "com.dronelabs.Perfkit",
	                          DBUS_NAME_FLAG_DO_NOT_QUEUE,
	                          NULL) == DBUS_REQUEST_NAME_REPLY_EXISTS)
	{
		g_warning("Existing instance of perfkit-daemon was found on the DBus."
		          " The DBus listener will not be activated.");
		return TRUE;
	}

	/*
	 * Register DBus object information for known types.
	 */
	dbus_g_object_type_install_info(PKD_TYPE_CHANNEL,
	                                &dbus_glib_pkd_channel_object_info);
	dbus_g_object_type_install_info(PKD_TYPE_ENCODER_INFO,
	                                &dbus_glib_pkd_encoder_info_object_info);
	dbus_g_object_type_install_info(PKD_TYPE_SOURCE_INFO,
	                                &dbus_glib_pkd_source_info_object_info);
	dbus_g_object_type_install_info(PKD_TYPE_SOURCE,
	                                &dbus_glib_pkd_source_object_info);


	/*
	 * Register objects on the DBus.
	 */
	g_message("Registering DBus Services.");

	/*
	 * Export available encoder plugins.
	 */
	list = pkd_pipeline_get_encoder_plugins();
	for (iter = list; iter; iter = iter->next) {
		path = g_strdup_printf("/com/dronelabs/Perfkit/Plugins/Encoders/%s",
		                       pkd_encoder_info_get_uid(iter->data));
		dbus_g_connection_register_g_object(dbus_conn, path, iter->data);
		g_free(path);
	}
	g_list_foreach(list, (GFunc)g_object_unref, NULL);
	g_list_free(list);

	/*
	 * Export available source plugins.
	 */
	list = pkd_pipeline_get_source_plugins();
	for (iter = list; iter; iter = iter->next) {
		path = g_strdup_printf("/com/dronelabs/Perfkit/Plugins/Sources/%s",
		                       pkd_source_info_get_uid(iter->data));
		dbus_g_connection_register_g_object(dbus_conn, path, iter->data);
		g_free(path);
	}
	g_list_foreach(list, (GFunc)g_object_unref, NULL);
	g_list_free(list);

	/*
	 * Manager Servce (/com/dronelabs/Perfkit/Manager)
	 */
	priv->manager = g_object_new(PKD_DBUS_TYPE_MANAGER, NULL);
	dbus_g_connection_register_g_object(dbus_conn,
	                                    "/com/dronelabs/Perfkit/Manager",
	                                    G_OBJECT(priv->manager));

	g_message("DBus listener started.");

	return TRUE;
}

static void
pkd_dbus_stop(PkdListener *listener)
{
	PkdDBusPrivate *priv;
	GList *list;
#ifdef HAVE_DBUS_0_82
	GList *iter;
#endif

	priv = PKD_DBUS(listener)->priv;

	if (!priv->manager) {
		goto finish;
	}

	/*
	 * Remove source plugins from DBUS.
	 */
	list = pkd_pipeline_get_source_plugins();
#ifdef HAVE_DBUS_0_82
	g_message("Removing source plugins from DBus.");
	for (iter = list; iter; iter = iter->next) {
		dbus_g_connection_unregister_g_object(dbus_conn, iter->data);
	}
#endif
	g_list_foreach(list, (GFunc)g_object_unref, NULL);
	g_list_free(list);

	/*
	 * Remove encoder plugins from DBUS.
	 */
	list = pkd_pipeline_get_encoder_plugins();
#ifdef HAVE_DBUS_0_82
	g_message("Removing encoder plugins from DBus.");
	for (iter = list; iter; iter = iter->next) {
		dbus_g_connection_unregister_g_object(dbus_conn, iter->data);
	}
#endif
	g_list_foreach(list, (GFunc)g_object_unref, NULL);
	g_list_free(list);

	/*
	 * Cleanup our manager object.
	 */
#ifdef HAVE_DBUS_0_82
	dbus_g_connection_unregister_g_object(dbus_conn, G_OBJECT(priv->manager));
#endif
	g_object_unref(priv->manager);
	priv->manager = NULL;

finish:

	/*
	 * Close our connection to DBUS.
	 */
	if (priv->conn) {
		dbus_g_connection_unref(priv->conn);
		priv->conn = NULL;
		dbus_conn = NULL;
	}

	g_message("DBus listener stopped.");
}

static void
pkd_dbus_channel_added (PkdListener *listener,
                        PkdChannel  *channel)
{
	gchar *path;

	path = g_strdup_printf("/com/dronelabs/Perfkit/Channels/%d",
	                       pkd_channel_get_id(channel));
	dbus_g_connection_register_g_object(dbus_conn, path, G_OBJECT(channel));
	g_free(path);
}

static void
pkd_dbus_source_added (PkdListener *listener,
                       PkdSource   *source)
{
	gchar *path;

	path = g_strdup_printf("/com/dronelabs/Perfkit/Sources/%d",
	                       pkd_source_get_id(source));
	dbus_g_connection_register_g_object(dbus_conn, path, G_OBJECT(source));
	g_free(path);
}

static void
pkd_dbus_finalize(GObject *object)
{
	G_OBJECT_CLASS(pkd_dbus_parent_class)->finalize(object);
}

static void
pkd_dbus_class_init(PkdDBusClass *klass)
{
	GObjectClass *object_class;
	PkdListenerClass *listener_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pkd_dbus_finalize;
	g_type_class_add_private(object_class, sizeof(PkdDBusPrivate));

	listener_class = PKD_LISTENER_CLASS(klass);
	listener_class->start = pkd_dbus_start;
	listener_class->stop = pkd_dbus_stop;
	listener_class->channel_added = pkd_dbus_channel_added;
	listener_class->source_added = pkd_dbus_source_added;
}

static void
pkd_dbus_init(PkdDBus *dbus)
{
	dbus->priv = G_TYPE_INSTANCE_GET_PRIVATE(dbus, PKD_TYPE_DBUS, PkdDBusPrivate);
}

G_MODULE_EXPORT void
pkd_listener_register(void)
{
	PkdListener *listener;

	/*
	 * Check to see if the listener is disabled.
	 */
	if (pkd_config_get_boolean("listener.dbus", "disabled", FALSE)) {
		g_message("DBus listener disabled by config.");
		return;
	}

	/*
	 * Create new instance of the DBus listener and add it to the pipeline.
	 */
	listener = g_object_new (PKD_TYPE_DBUS, NULL);
	pkd_pipeline_add_listener(listener);
	g_object_unref(listener);
}
