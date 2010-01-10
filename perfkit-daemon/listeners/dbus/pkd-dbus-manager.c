/* pkd-dbus-manager.c
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

#include "pkd-dbus.h"
#include "pkd-dbus-manager.h"

static gboolean pkd_dbus_manager_get_channels (PkdDBusManager   *manager,
                                               gchar          ***paths,
                                               GError          **error);
static gboolean pkd_dbus_manager_ping         (PkdDBusManager   *manager,
                                               const gchar     **time_,
                                               GError          **path);

#include "pkd-dbus-manager-dbus.h"

G_DEFINE_TYPE (PkdDBusManager, pkd_dbus_manager, G_TYPE_OBJECT)

typedef struct {
	DBusGConnection *conn;
	PkdDBusManager  *manager;
	DBusGProxy      *proxy;
} SubInfo;

gboolean
pkd_dbus_manager_create_channel (PkdDBusManager  *manager,
                                 GPid             pid,
                                 const gchar     *target,
                                 gchar          **args,
                                 gchar          **env,
                                 const gchar     *working_dir,
                                 gchar          **channel,
                                 GError         **error)
{
	PkdChannel   *real_channel;
	PkdSpawnInfo  spawn;

	memset(&spawn, 0, sizeof(PkdSpawnInfo));

	spawn.pid = pid;
	spawn.target = (gchar *)target;
	spawn.args = args;
	spawn.env = env;
	spawn.working_dir = (gchar *)working_dir;

	real_channel = pkd_channel_new(&spawn);
	if (!real_channel) {
		/* TODO: Set error */
		return FALSE;
	}

	pkd_pipeline_add_channel(real_channel);
	*channel = g_strdup_printf("/com/dronelabs/Perfkit/Channels/%d",
	                           pkd_channel_get_id(real_channel));

	return TRUE;
}

static void
manifest_cb (gchar    *buffer,
             gsize     buffer_size,
             gpointer  data)
{
	SubInfo *info = data;
	GByteArray *ar;

	ar = g_byte_array_sized_new(buffer_size);
	g_byte_array_append(ar, (guint8 *)buffer, buffer_size);

	dbus_g_proxy_call_no_reply(info->proxy, "Manifest",
	                           dbus_g_type_get_collection("GByteArray", G_TYPE_CHAR),
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
	                           dbus_g_type_get_collection("GByteArray", G_TYPE_CHAR),
	                           ar,
	                           G_TYPE_INVALID,
	                           G_TYPE_INVALID);
}

gboolean
pkd_dbus_manager_create_subscription (PkdDBusManager  *manager,
                                      const gchar     *delivery_address,
                                      const gchar     *delivery_path,
                                      const gchar     *channel,
                                      guint            buffer_size,
                                      guint            buffer_timeout,
                                      const gchar     *encoder_info,
                                      gchar          **subscription,
                                      GError         **error)
{
	PkdChannel *real_channel;
	PkdEncoderInfo *real_encoder_info;
	PkdSubscription *sub;
	SubInfo *data = NULL;
	DBusGConnection *conn;

	g_message("%s", G_STRLOC);

	g_return_val_if_fail(PKD_DBUS_IS_MANAGER(manager), FALSE);
	g_return_val_if_fail(delivery_address != NULL, FALSE);
	g_return_val_if_fail(delivery_path != NULL, FALSE);
	g_return_val_if_fail(channel != NULL, FALSE);

	conn = pkd_dbus_get_connection();
	real_channel = (PkdChannel *)dbus_g_connection_lookup_g_object(conn, channel);
	if (!real_channel) {
		g_warning("Error locating channel: %s", channel);
		/* TODO: Set error message */
		return FALSE;
	}

	g_message("Registing subscription for %s.", channel);

	/* Okay if this is NULL */
	real_encoder_info = (PkdEncoderInfo *)dbus_g_connection_lookup_g_object(conn, encoder_info);

	data = g_slice_new0(SubInfo);
	data->conn = dbus_g_connection_open(delivery_address, NULL);
	if (!data->conn) {
		g_warning("Error opening connection to DBus address.");
		goto error;
	}
	data->manager = manager;
	data->proxy = dbus_g_proxy_new_for_peer(data->conn, delivery_path,
	                                        "com.dronelabs.Perfkit.Subscription");

	/* subscribe to the channel */
	sub = pkd_subscription_new(real_channel,
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

	/* notify the pipeline of the subscription */
	pkd_pipeline_add_subscription(sub);

	*subscription = g_strdup_printf("/com/dronelabs/Perfkit/Subscriptions/%d",
	                                pkd_subscription_get_id(sub));

	return TRUE;

error:
	g_set_error(error, PKD_DBUS_MANAGER_ERROR,
	            PKD_DBUS_MANAGER_ERROR_SUBSCRIPTION,
	            _("Could not register subscription."));
	g_message("Error registering new subscription via DBus.");
	if (data->proxy);
		g_object_unref(data->proxy);
	if (data->conn)
		dbus_g_connection_unref(data->conn);
	g_slice_free(SubInfo, data);
	return FALSE;
}

GQuark
pkd_dbus_manager_error_quark (void)
{
	return g_quark_from_static_string("pkd-dbus-manager-error-quark");
}

gboolean
pkd_dbus_manager_get_processes (PkdDBusManager  *manager,
                                GPtrArray      **processes,
                                GError         **error)
{
	return FALSE;
}

static gboolean
pkd_dbus_manager_get_channels (PkdDBusManager   *manager,
                               gchar          ***paths,
                               GError          **error)
{
	GList *channels, *iter;
	gchar **cpaths;
	gint i;

	channels = pkd_pipeline_get_channels();
	cpaths = g_malloc0(sizeof(gchar*) * (g_list_length(channels) + 1));

	for (iter = channels, i = 0; iter; iter = iter->next, i++) {
		cpaths[i] = g_strdup_printf("/com/dronelabs/Perfkit/Channels/%d",
		                            pkd_channel_get_id(iter->data));
	}

	g_list_foreach(channels, (GFunc)g_object_unref, NULL);
	g_list_free(channels);
	*paths = cpaths;

	return TRUE;
}

static gboolean
pkd_dbus_manager_ping (PkdDBusManager  *manager,
                       const gchar    **time_,
                       GError         **error)
{
	GTimeVal tv;

	g_get_current_time(&tv);
	*time_ = g_time_val_to_iso8601(&tv);

	return TRUE;
}

static void
pkd_dbus_manager_finalize (GObject *object)
{
	PkdDBusManagerPrivate *priv;

	g_return_if_fail (PKD_DBUS_IS_MANAGER (object));

	priv = PKD_DBUS_MANAGER (object)->priv;

	G_OBJECT_CLASS (pkd_dbus_manager_parent_class)->finalize (object);
}

static void
pkd_dbus_manager_class_init (PkdDBusManagerClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkd_dbus_manager_finalize;

	dbus_g_object_type_install_info(PKD_DBUS_TYPE_MANAGER,
	                                &dbus_glib_pkd_dbus_manager_object_info);
}

static void
pkd_dbus_manager_init (PkdDBusManager *manager)
{
	dbus_g_connection_register_g_object(pkd_dbus_get_connection(),
	                                    "/com/dronelabs/Perfkit/Manager",
	                                    G_OBJECT(manager));
}
