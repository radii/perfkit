/* pka-dbus-manager.c
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

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <string.h>

#include "pka-dbus.h"
#include "pka-dbus-manager.h"
#include "pka-dbus-subscription.h"

static gboolean pka_dbus_manager_get_channels (PkaDBusManager   *manager,
                                               gchar          ***paths,
                                               GError          **error);
static gboolean pka_dbus_manager_ping         (PkaDBusManager   *manager,
                                               const gchar     **time_,
                                               GError          **path);

#include "pka-dbus-manager-dbus.h"

#define CHANNEL_PATH       "/org/perfkit/Agent/Channels/%d"
#define SOURCE_PLUGIN_PATH "/org/perfkit/Agent/Plugins/Sources/%s"
#define SUBSCRIPTION_PATH  "/org/perfkit/Agent/Subscriptions/%d"

G_DEFINE_TYPE (PkaDBusManager, pka_dbus_manager, G_TYPE_OBJECT)

typedef struct {
	DBusGConnection *conn;
	PkaDBusManager  *manager;
	DBusGProxy      *proxy;
} SubInfo;

gboolean
pka_dbus_manager_create_channel (PkaDBusManager  *manager,
                                 GPid             pid,
                                 const gchar     *target,
                                 gchar          **args,
                                 gchar          **env,
                                 const gchar     *working_dir,
                                 gchar          **channel,
                                 GError         **error)
{
	PkaChannel   *real_channel;
	PkaSpawnInfo  spawn;

	memset(&spawn, 0, sizeof(PkaSpawnInfo));

	spawn.pid = pid;
	spawn.target = (gchar *)target;
	spawn.args = args;
	spawn.env = env;
	spawn.working_dir = (gchar *)working_dir;

	real_channel = pka_channel_new(&spawn);
	if (!real_channel) {
		g_set_error(error, PKA_CHANNEL_ERROR,
		            PKA_CHANNEL_ERROR_UNKNOWN,
		            "An error occurred creating channel");
		return FALSE;
	}

	pka_pipeline_add_channel(real_channel);
	*channel = g_strdup_printf(CHANNEL_PATH,
	                           pka_channel_get_id(real_channel));

	return TRUE;
}

static void
manifest_cb (gchar    *buffer,
             gsize     buffer_size,
             gpointer  data)
{
	DBusConnection *conn;
	SubInfo *info = data;
	GByteArray *ar;

	/*
	 * Ensure that we are still connected.
	 */
	conn = dbus_g_connection_get_connection(info->conn);
	if (!dbus_connection_get_is_connected(conn)) {
		g_warning("NOT CONNECTED, NOT SENDING DATA.");
		return;
	}

	ar = g_byte_array_sized_new(buffer_size);
	g_byte_array_append(ar, (guint8 *)buffer, buffer_size);

	dbus_g_proxy_call_no_reply(info->proxy, "Manifest",
	                           dbus_g_type_get_collection("GArray", G_TYPE_CHAR),
	                           ar,
	                           G_TYPE_INVALID,
	                           G_TYPE_INVALID);
}

static void
sample_cb (gchar    *buffer,
           gsize     buffer_size,
           gpointer  data)
{
	SubInfo *info = data;
	GByteArray *ar;

	ar = g_byte_array_sized_new(buffer_size);
	g_byte_array_append(ar, (guint8 *)buffer, buffer_size);

	dbus_g_proxy_call_no_reply(info->proxy, "Sample",
	                           dbus_g_type_get_collection("GArray", G_TYPE_CHAR),
	                           ar,
	                           G_TYPE_INVALID,
	                           G_TYPE_INVALID);
}

static void
on_proxy_destroy (DBusGProxy      *proxy,
                  PkaSubscription *sub)
{
	pka_subscription_disable(sub, FALSE);
}

gboolean
pka_dbus_manager_create_subscription (PkaDBusManager  *manager,
                                      const gchar     *delivery_address,
                                      const gchar     *delivery_path,
                                      const gchar     *channel,
                                      guint            buffer_size,
                                      guint            buffer_timeout,
                                      const gchar     *encoder_info,
                                      gchar          **subscription,
                                      GError         **error)
{
	PkaChannel *real_channel;
	PkaEncoderInfo *real_encoder_info;
	PkaSubscription *sub;
	SubInfo *data = NULL;
	DBusGConnection *conn;

	g_return_val_if_fail(PKA_DBUS_IS_MANAGER(manager), FALSE);
	g_return_val_if_fail(delivery_address != NULL, FALSE);
	g_return_val_if_fail(delivery_path != NULL, FALSE);
	g_return_val_if_fail(channel != NULL, FALSE);

	g_message("DBus listener received subscription request.");

	conn = pka_dbus_get_connection();
	real_channel = (PkaChannel *)dbus_g_connection_lookup_g_object(conn, channel);
	if (!real_channel) {
		g_warning("Error locating channel: %s", channel);
		/* TODO: Set error message */
		return FALSE;
	}

	g_message("Registing subscription for %s.", channel);

	/* Okay if this is NULL */
	real_encoder_info = (PkaEncoderInfo *)dbus_g_connection_lookup_g_object(conn, encoder_info);

	data = g_slice_new0(SubInfo);
	data->conn = dbus_g_connection_open(delivery_address, NULL);
	if (!data->conn) {
		g_warning("Error opening connection to DBus address.");
		goto error;
	}
	data->manager = manager;
	data->proxy = dbus_g_proxy_new_for_peer(data->conn, delivery_path,
	                                        "org.perfkit.Agent.Subscription");

	/* subscribe to the channel */
	sub = pka_subscription_new(real_channel,
	                           real_encoder_info,
	                           buffer_size,
	                           buffer_timeout,
	                           manifest_cb,
	                           data,
	                           sample_cb,
	                           data);
	if (!sub) {
		/* TODO: Set error message */
		g_warning("Error creating subscription.");
		goto error;
	}

	g_signal_connect(data->proxy,
	                 "destroy",
	                 G_CALLBACK(on_proxy_destroy),
	                 sub);

	/* notify the pipeline of the subscription */
	pka_pipeline_add_subscription(sub);

	*subscription = g_strdup_printf(SUBSCRIPTION_PATH,
	                                pka_subscription_get_id(sub));

	dbus_g_connection_register_g_object(conn,
	                                    *subscription,
	                                    G_OBJECT(pka_dbus_subscription_new(sub)));

	return TRUE;

error:
	g_set_error(error, PKA_DBUS_MANAGER_ERROR,
	            PKA_DBUS_MANAGER_ERROR_SUBSCRIPTION,
	            _("Could not register subscription."));
	g_message("Error registering new subscription via DBus.");
	if (data->proxy)
		g_object_unref(data->proxy);
	if (data->conn)
		dbus_g_connection_unref(data->conn);
	g_slice_free(SubInfo, data);
	return FALSE;
}

gboolean
pka_dbus_manager_get_source_plugins (PkaDBusManager   *manager,
                                     gchar          ***paths,
                                     GError          **error)
{
	GList *list, *iter;
	gchar **p;
	gint i;

	g_return_val_if_fail(PKA_DBUS_IS_MANAGER(manager), FALSE);
	g_return_val_if_fail(paths != NULL, FALSE);

	list = pka_pipeline_get_source_plugins();

	if (!list) {
		*paths = g_malloc0(sizeof(gchar*));
		return TRUE;
	}

	p = g_malloc0((g_list_length(list) + 1) * sizeof(gchar*));
	for (iter = list, i = 0; iter; iter = iter->next, i++) {
		p[i] = g_strdup_printf(SOURCE_PLUGIN_PATH,
		                       pka_source_info_get_uid(iter->data));
	}

	p[i] = NULL;
	*paths = p;

	g_list_foreach(list, (GFunc)g_object_unref, NULL);
	g_list_free(list);

	return TRUE;
}

gboolean
pka_dbus_manager_get_version (PkaDBusManager  *manager,
                              gchar          **version,
                              GError         **error)
{
	*version = g_strdup(PKA_VERSION_S);
	return TRUE;
}

GQuark
pka_dbus_manager_error_quark (void)
{
	return g_quark_from_static_string("pka-dbus-manager-error-quark");
}

gboolean
pka_dbus_manager_get_processes (PkaDBusManager  *manager,
                                GPtrArray      **processes,
                                GError         **error)
{
	return FALSE;
}

static gboolean
pka_dbus_manager_get_channels (PkaDBusManager   *manager,
                               gchar          ***paths,
                               GError          **error)
{
	GList *channels, *iter;
	gchar **cpaths;
	gint i;

	g_return_val_if_fail(PKA_DBUS_IS_MANAGER(manager), FALSE);
	g_return_val_if_fail(paths != NULL, FALSE);

	channels = pka_pipeline_get_channels();
	cpaths = g_malloc0(sizeof(gchar*) * (g_list_length(channels) + 1));

	for (iter = channels, i = 0; iter; iter = iter->next, i++) {
		cpaths[i] = g_strdup_printf(CHANNEL_PATH,
		                            pka_channel_get_id(iter->data));
	}

	g_list_foreach(channels, (GFunc)g_object_unref, NULL);
	g_list_free(channels);
	*paths = cpaths;

	return TRUE;
}


gboolean
pka_dbus_manager_remove_channel (PkaDBusManager  *manager,
                                 const gchar     *path,
                                 GError         **error)
{
	PkaDBusManagerPrivate *priv;
	PkaChannel *channel;

	g_return_val_if_fail(PKA_DBUS_IS_MANAGER(manager), FALSE);
	g_return_val_if_fail(path != NULL, FALSE);

	priv = manager->priv;

	channel = (PkaChannel *)dbus_g_connection_lookup_g_object(pka_dbus_get_connection(), path);
	if (channel) {
		pka_pipeline_remove_channel(channel);
	}

	return TRUE;
}

static gboolean
pka_dbus_manager_ping (PkaDBusManager  *manager,
                       const gchar    **time_,
                       GError         **error)
{
	GTimeVal tv;

	g_get_current_time(&tv);
	*time_ = g_time_val_to_iso8601(&tv);

	return TRUE;
}

static void
pka_dbus_manager_finalize (GObject *object)
{
	PkaDBusManagerPrivate *priv;

	g_return_if_fail (PKA_DBUS_IS_MANAGER (object));

	priv = PKA_DBUS_MANAGER (object)->priv;

	G_OBJECT_CLASS (pka_dbus_manager_parent_class)->finalize (object);
}

static void
pka_dbus_manager_class_init (PkaDBusManagerClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pka_dbus_manager_finalize;

	dbus_g_object_type_install_info(PKA_DBUS_TYPE_MANAGER,
	                                &dbus_glib_pka_dbus_manager_object_info);
}

static void
pka_dbus_manager_init (PkaDBusManager *manager)
{
}
