/* pka-dbus.c
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
#include <perfkit-agent/perfkit-agent.h>

#include "pka-dbus.h"
#include "pka-dbus-glue.h"
#include "pka-dbus-manager.h"
#include "pka-channel-glue.h"
#include "pka-channel-dbus.h"
#include "pka-encoder-info-dbus.h"
#include "pka-source-info-dbus.h"
#include "pka-source-glue.h"
#include "pka-source-dbus.h"

G_DEFINE_TYPE (PkaDBus, pka_dbus, PKA_TYPE_LISTENER)

static DBusGConnection *dbus_conn = NULL;

struct _PkaDBusPrivate
{
	DBusGConnection *conn;
	PkaDBusManager  *manager;
};

DBusGConnection*
pka_dbus_get_connection (void)
{
	return dbus_conn;
}

static gboolean
pka_dbus_start(PkaListener  *listener,
               GError      **ret_error)
{
	PkaDBusPrivate *priv;
	GError *error = NULL;
	GList *list, *iter;
	gchar *path;

	g_return_val_if_fail(PKA_IS_DBUS(listener), FALSE);

	priv = PKA_DBUS(listener)->priv;

	/*
	 * Connect to the DBus Agent.
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
		g_warning("Existing instance of perfkit-agent was found on the DBus."
		          " The DBus listener will not be activated.");
		return TRUE;
	}

	/*
	 * Register DBus object information for known types.
	 */
	dbus_g_object_type_install_info(PKA_TYPE_CHANNEL,
	                                &dbus_glib_pka_channel_object_info);
	dbus_g_object_type_install_info(PKA_TYPE_ENCODER_INFO,
	                                &dbus_glib_pka_encoder_info_object_info);
	dbus_g_object_type_install_info(PKA_TYPE_SOURCE_INFO,
	                                &dbus_glib_pka_source_info_object_info);
	dbus_g_object_type_install_info(PKA_TYPE_SOURCE,
	                                &dbus_glib_pka_source_object_info);


	/*
	 * Register objects on the DBus.
	 */
	g_message("Registering DBus Services.");

	/*
	 * Export available encoder plugins.
	 */
	list = pka_pipeline_get_encoder_plugins();
	for (iter = list; iter; iter = iter->next) {
		path = g_strdup_printf("/com/dronelabs/Perfkit/Plugins/Encoders/%s",
		                       pka_encoder_info_get_uid(iter->data));
		dbus_g_connection_register_g_object(dbus_conn, path, iter->data);
		g_free(path);
	}
	g_list_foreach(list, (GFunc)g_object_unref, NULL);
	g_list_free(list);

	/*
	 * Export available source plugins.
	 */
	list = pka_pipeline_get_source_plugins();
	for (iter = list; iter; iter = iter->next) {
		path = g_strdup_printf("/com/dronelabs/Perfkit/Plugins/Sources/%s",
		                       pka_source_info_get_uid(iter->data));
		dbus_g_connection_register_g_object(dbus_conn, path, iter->data);
		g_free(path);
	}
	g_list_foreach(list, (GFunc)g_object_unref, NULL);
	g_list_free(list);

	/*
	 * Manager Servce (/com/dronelabs/Perfkit/Manager)
	 */
	priv->manager = g_object_new(PKA_DBUS_TYPE_MANAGER, NULL);
	dbus_g_connection_register_g_object(dbus_conn,
	                                    "/com/dronelabs/Perfkit/Manager",
	                                    G_OBJECT(priv->manager));

	g_message("DBus listener started.");

	return TRUE;
}

static void
pka_dbus_stop(PkaListener *listener)
{
	PkaDBusPrivate *priv;
	GList *list;
#ifdef HAVE_DBUS_0_82
	GList *iter;
#endif

	priv = PKA_DBUS(listener)->priv;

	if (!priv->manager) {
		goto finish;
	}

	/*
	 * Remove source plugins from DBUS.
	 */
	list = pka_pipeline_get_source_plugins();
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
	list = pka_pipeline_get_encoder_plugins();
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
pka_dbus_channel_added (PkaListener *listener,
                        PkaChannel  *channel)
{
	gchar *path;

	path = g_strdup_printf("/com/dronelabs/Perfkit/Channels/%d",
	                       pka_channel_get_id(channel));
	dbus_g_connection_register_g_object(dbus_conn, path, G_OBJECT(channel));
	g_free(path);
}

static void
pka_dbus_source_added (PkaListener *listener,
                       PkaSource   *source)
{
	gchar *path;

	path = g_strdup_printf("/com/dronelabs/Perfkit/Sources/%d",
	                       pka_source_get_id(source));
	dbus_g_connection_register_g_object(dbus_conn, path, G_OBJECT(source));
	g_free(path);
}

static void
pka_dbus_finalize(GObject *object)
{
	G_OBJECT_CLASS(pka_dbus_parent_class)->finalize(object);
}

static void
pka_dbus_class_init(PkaDBusClass *klass)
{
	GObjectClass *object_class;
	PkaListenerClass *listener_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pka_dbus_finalize;
	g_type_class_add_private(object_class, sizeof(PkaDBusPrivate));

	listener_class = PKA_LISTENER_CLASS(klass);
	listener_class->start = pka_dbus_start;
	listener_class->stop = pka_dbus_stop;
	listener_class->channel_added = pka_dbus_channel_added;
	listener_class->source_added = pka_dbus_source_added;
}

static void
pka_dbus_init(PkaDBus *dbus)
{
	dbus->priv = G_TYPE_INSTANCE_GET_PRIVATE(dbus, PKA_TYPE_DBUS, PkaDBusPrivate);
}

G_MODULE_EXPORT void
pka_listener_register(void)
{
	PkaListener *listener;

	/*
	 * Check to see if the listener is disabled.
	 */
	if (pka_config_get_boolean("listener.dbus", "disabled", FALSE)) {
		g_message("DBus listener disabled by config.");
		return;
	}

	/*
	 * Create new instance of the DBus listener and add it to the pipeline.
	 */
	listener = g_object_new (PKA_TYPE_DBUS, NULL);
	pka_pipeline_add_listener(listener);
	g_object_unref(listener);
}
