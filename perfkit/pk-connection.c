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
	PkManager  *manager;
	gchar      *uri;
	GMutex     *lock;
	GHashTable *subs;
};

typedef struct
{
	gint            sub_id;
	PkManifest     *manifest;
	PkManifestFunc  manifest_func;
	PkSampleFunc    sample_func;
	gpointer        user_data;
} Sub;

enum
{
	PROP_0,
	PROP_URI,
};

static GType
get_connection_type (const gchar *uri)
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
	 * Get the connection type "type://".
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
	 * Determine the connection path.
	 */
	if (g_getenv("PK_CONNECTIONS_DIR") != NULL) {
		path = g_module_build_path(g_getenv("PK_CONNECTIONS_DIR"),
		                           type_name);
	} else {
		path = g_module_build_path(PACKAGE_LIB_DIR
		                           G_DIR_SEPARATOR_S
		                           "perfkit"
		                           G_DIR_SEPARATOR_S
		                           "connections",
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
	 * Lookup the "pk_connection_plugin" symbol.
	 */
	g_module_symbol(module, "pk_connection_plugin", (gpointer *)&plugin);
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
 * Creates a new instance of #PkConnection using @uri.  If the connection
 * specified cannot be provided, %NULL is returned.
 *
 * Returns: the newly created #PkConnection or %NULL.
 */
PkConnection*
pk_connection_new_from_uri (const gchar *uri)
{
	PkConnection *conn;
	GType conn_type;

	/*
	 * Get the connection type from the uri.
	 */
	conn_type = get_connection_type(uri);
	if (!g_type_is_a(conn_type, PK_TYPE_CONNECTION)) {
		g_warning("Connection plugin did not return a PkConnection.");
		return NULL;
	}

	/*
	 * Instantiate the connection.
	 */
	conn = g_object_new(conn_type, "uri", uri, NULL);
	if (!conn) {
		g_warning("Could not create new instance of %s.",
		          g_type_name(conn_type));
		return NULL;
	}

	return conn;
}

gboolean
pk_connection_is_connected (PkConnection *connection)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);
	return PK_CONNECTION_GET_CLASS(connection)->is_connected(connection);
}

gboolean
pk_connection_channel_get_state (PkConnection    *connection,
                                 gint             channel_id,
                                 PkChannelState  *state,
                                 GError         **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->channel_get_state(
			connection, channel_id, state, error);
}

gboolean
pk_connection_manager_get_channels (PkConnection  *connection,
                                    gint         **channels,
                                    gint          *n_channels,
                                    GError       **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->manager_get_channels(
			connection, channels, n_channels, error);
}

const gchar*
pk_connection_get_uri (PkConnection *connection)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), NULL);
	return connection->priv->uri;
}

PkManager*
pk_connection_get_manager (PkConnection *connection)
{
	PkConnectionPrivate *priv;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), NULL);

	priv = connection->priv;

	g_mutex_lock(priv->lock);
	if (!priv->manager) {
		priv->manager = g_object_new(PK_TYPE_MANAGER,
		                             "connection", connection,
		                             NULL);
	}
	g_mutex_unlock(priv->lock);

	return priv->manager;
}

gboolean
pk_connection_connect (PkConnection  *connection,
                       GError       **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->connect(connection, error);
}

void
pk_connection_disconnect (PkConnection *connection)
{
	g_return_if_fail(PK_IS_CONNECTION(connection));

	PK_CONNECTION_GET_CLASS(connection)->disconnect(connection);
}

gboolean
pk_connection_channel_get_target (PkConnection   *connection,
                                  gint            channel_id,
                                  gchar         **target,
                                  GError        **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->
		channel_get_target(connection, channel_id, target, error);
}

gboolean
pk_connection_channel_get_working_dir (PkConnection   *connection,
                                       gint            channel_id,
                                       gchar         **working_dir,
                                       GError        **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->
		channel_get_working_dir(connection, channel_id, working_dir, error);
}

gboolean
pk_connection_channel_get_args (PkConnection   *connection,
                                gint            channel_id,
                                gchar        ***args,
                                GError        **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->
		channel_get_args(connection, channel_id, args, error);
}

gboolean
pk_connection_channel_get_env (PkConnection   *connection,
                               gint            channel_id,
                               gchar        ***env,
                               GError        **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->
		channel_get_env(connection, channel_id, env, error);
}

gboolean
pk_connection_channel_get_pid (PkConnection   *connection,
                               gint            channel_id,
                               GPid           *pid,
                               GError        **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->
		channel_get_pid(connection, channel_id, pid, error);
}

gboolean
pk_connection_manager_create_channel (PkConnection   *connection,
                                      PkSpawnInfo    *spawn_info,
                                      gint           *channel_id,
                                      GError        **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->
		manager_create_channel(connection, spawn_info, channel_id, error);
}

gboolean
pk_connection_manager_create_subscription (PkConnection  *connection,
                                           gint           channel_id,
                                           gsize          buffer_size,
                                           gulong         buffer_timeout,
                                           const gchar   *encoder_info_uid,
                                           gint          *subscription_id,
                                           GError       **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);
	g_return_val_if_fail(channel_id >= 0, FALSE);
	g_return_val_if_fail(subscription_id != NULL, FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->
		manager_create_subscription(connection, channel_id,
				buffer_size, buffer_timeout, encoder_info_uid,
				subscription_id, error);
}

gboolean
pk_connection_manager_remove_channel (PkConnection  *connection,
                                      gint           channel_id,
                                      GError       **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->
		manager_remove_channel(connection, channel_id, error);
}

gboolean
pk_connection_channel_start (PkConnection  *connection,
                             gint           channel_id,
                             GError       **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->
		channel_start(connection, channel_id, error);
}

gboolean
pk_connection_channel_stop (PkConnection  *connection,
                            gint           channel_id,
                            gboolean       killpid,
                            GError       **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->
		channel_stop(connection, channel_id, killpid, error);
}

gboolean
pk_connection_channel_pause (PkConnection  *connection,
                             gint           channel_id,
                             GError       **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->
		channel_pause(connection, channel_id, error);
}

gboolean
pk_connection_channel_unpause (PkConnection  *connection,
                               gint           channel_id,
                               GError       **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->
		channel_unpause(connection, channel_id, error);
}

gboolean
pk_connection_manager_ping (PkConnection  *connection,
                            GTimeVal      *tv,
                            GError       **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);
	g_return_val_if_fail(tv != NULL, FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->
		manager_ping(connection, tv, error);
}

gboolean
pk_connection_manager_get_version (PkConnection  *connection,
                                   gchar        **version,
                                   GError       **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);
	g_return_val_if_fail(version != NULL, FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->
		manager_get_version(connection, version, error);
}

gboolean
pk_connection_manager_get_source_plugins (PkConnection   *connection,
                                          gchar        ***plugins,
                                          GError        **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);
	g_return_val_if_fail(plugins != NULL, FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->
		manager_get_source_plugins(connection, plugins, error);
}

gboolean
pk_connection_channel_add_source (PkConnection  *connection,
                                  gint           channel_id,
                                  const gchar   *source_type,
                                  gint          *source_id,
                                  GError       **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);
	g_return_val_if_fail(channel_id >= 0, FALSE);
	g_return_val_if_fail(source_type != NULL, FALSE);
	g_return_val_if_fail(source_id != NULL, FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->
		channel_add_source(connection, channel_id, source_type,
				source_id, error);
}

gboolean
pk_connection_channel_remove_source (PkConnection  *connection,
                                     gint           channel_id,
                                     gint           source_id,
                                     GError       **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);
	g_return_val_if_fail(channel_id >= 0, FALSE);
	g_return_val_if_fail(source_id >= 0, FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->
		channel_remove_source(connection, channel_id, source_id, error);
}

/**
 * pk_connection_subscription_get_manifest:
 * @connection: A #PkConnection.
 * @subscription_id: The subscription identifier.
 *
 * Retrieves the current manifest for the subscription.  The #PkManifest
 * has it's reference count incremented and should be freed by the caller
 * using pk_manifest_unref().
 *
 * Returns:
 *    A #PkManifest if successful; otherwise %NULL.
 *
 * Side effects:
 *    None.
 */
PkManifest*
pk_connection_subscription_get_manifest (PkConnection *connection,
                                         gint          subscription_id)
{
	PkConnectionPrivate *priv;
	PkManifest *manifest = NULL;
	Sub *sub;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), NULL);

	priv = connection->priv;

	g_mutex_lock(priv->lock);
	if (NULL != (sub = g_hash_table_lookup(priv->subs, &subscription_id))) {
		if (sub->manifest) {
			manifest = pk_manifest_ref(sub->manifest);
		}
	}
	g_mutex_unlock(priv->lock);

	return manifest;
}

gboolean
pk_connection_subscription_set_handlers (PkConnection   *connection,
                                         gint            subscription_id,
                                         PkManifestFunc  manifest_func,
                                         PkSampleFunc    sample_func,
                                         gpointer        user_data)
{
	PkConnectionPrivate *priv;
	Sub *sub;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);
	g_return_val_if_fail(subscription_id >= 0, FALSE);
	g_return_val_if_fail(manifest_func != NULL, FALSE);
	g_return_val_if_fail(sample_func != NULL, FALSE);

	priv = connection->priv;

	sub = g_slice_new0(Sub);
	sub->sub_id = subscription_id;
	sub->manifest_func = manifest_func;
	sub->sample_func = sample_func;
	sub->user_data = user_data;

	g_mutex_lock(priv->lock);
	g_hash_table_insert(priv->subs, &sub->sub_id, sub);
	g_mutex_unlock(priv->lock);

	return TRUE;
}

static gboolean
decode_manifest (const guint8 *data,
                 gsize         length,
                 Sub          *sub)
{
	PkManifest *manifest, *tmp;

	manifest = pk_manifest_new_from_data(data, length);
	if (!manifest) {
		return FALSE;
	}

	tmp = sub->manifest;
	sub->manifest = manifest;

	if (tmp) {
		pk_manifest_unref(tmp);
	}

	return TRUE;
}

static gboolean
decode_samples (const guint8   *data,
                gsize           length,
                Sub            *sub,
                PkSample     ***samples,
                gsize          *n_samples)
{
	PkSample *sample = NULL;
	gsize n_bytes = 0, offset = 0;
	GPtrArray *ar;

	g_return_val_if_fail(data != NULL, FALSE);
	g_return_val_if_fail(length > 0, FALSE);
	g_return_val_if_fail(samples != NULL, FALSE);
	g_return_val_if_fail(n_samples != NULL, FALSE);

	ar = g_ptr_array_new();
	*n_samples = 0;

	while (offset < length) {
		sample = pk_sample_new_from_data(sub->manifest,
		                                 data,
		                                 length - offset,
		                                 &n_bytes);
		offset += n_bytes;
		data += n_bytes;
		if (sample) {
			g_ptr_array_add(ar, sample);
			(*n_samples)++;
		}
	}

	*samples = (PkSample **)ar->pdata;
	g_ptr_array_free(ar, FALSE);

	return TRUE;
}

void
pk_connection_subscription_deliver_manifest (PkConnection *connection,
                                             gint          subscription_id,
                                             const guint8 *data,
                                             gsize         length)
{
	PkConnectionPrivate *priv;
	Sub *sub;

	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(data != NULL);
	g_return_if_fail(length > 0);

	priv = connection->priv;

	/* Locking Strategy? */

	if (NULL != (sub = g_hash_table_lookup(priv->subs, &subscription_id))) {
		if (decode_manifest(data, length, sub)) {
			sub->manifest_func(sub->manifest, sub->user_data);
		}
	}
}

void
pk_connection_subscription_deliver_sample (PkConnection *connection,
                                           gint          subscription_id,
                                           const guint8 *data,
                                           gsize         length)
{
	PkConnectionPrivate *priv;
	PkSample **samples = NULL;
	gsize n_samples = 0;
	Sub *sub;
	gint i;

	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(data != NULL);
	g_return_if_fail(length > 0);

	priv = connection->priv;

	/* Locking Strategy? */

	if (NULL != (sub = g_hash_table_lookup(priv->subs, &subscription_id))) {
		if (decode_samples(data, length, sub, &samples, &n_samples)) {
			for (i = 0; i < n_samples; i++) {
				sub->sample_func(samples[i], sub->user_data);
				pk_sample_unref(samples[i]);
			}
			g_free(samples);
		}
	}
}

gboolean
pk_connection_subscription_enable (PkConnection  *connection,
                                   gint           subscription_id,
                                   GError       **error)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);
	g_return_val_if_fail(subscription_id >= 0, FALSE);

	return PK_CONNECTION_GET_CLASS(connection)->
		subscription_enable(connection, subscription_id, error);
}

static void
pk_connection_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
	switch (prop_id) {
	case PROP_URI:
		g_value_set_string(value, pk_connection_get_uri((gpointer)object));
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
	case PROP_URI:
		PK_CONNECTION(object)->priv->uri = g_value_dup_string(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_connection_finalize (GObject *object)
{
	PkConnectionPrivate *priv = PK_CONNECTION(object)->priv;

	g_free(priv->uri);

	G_OBJECT_CLASS(pk_connection_parent_class)->finalize(object);
}

static void
pk_connection_dispose (GObject *object)
{
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
	 * PkConnection:uri:
	 *
	 * The "uri" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_URI,
	                                g_param_spec_string("uri",
	                                                    "uri",
	                                                    "uri",
	                                                    NULL,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
sub_free (gpointer data)
{
	Sub *sub = data;

	if (sub->manifest) {
		pk_manifest_unref(sub->manifest);
	}

	g_slice_free(Sub, data);
}

static void
pk_connection_init (PkConnection *connection)
{
	PkConnectionPrivate *priv;

	connection->priv = G_TYPE_INSTANCE_GET_PRIVATE(connection,
	                                               PK_TYPE_CONNECTION,
	                                               PkConnectionPrivate);
	priv = connection->priv;

	priv->lock = g_mutex_new();
	priv->subs = g_hash_table_new_full(g_int_hash, g_int_equal,
	                                   NULL, sub_free);
}
