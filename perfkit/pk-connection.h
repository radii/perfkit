/* pk-connection.h
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
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

#ifndef __PK_CONNECTION_H__
#define __PK_CONNECTION_H__

#include <gio/gio.h>

#include "pk-spawn-info.h"

G_BEGIN_DECLS

#define PK_TYPE_CONNECTION            (pk_connection_get_type())
#define PK_CONNECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_CONNECTION, PkConnection))
#define PK_CONNECTION_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_CONNECTION, PkConnection const))
#define PK_CONNECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PK_TYPE_CONNECTION, PkConnectionClass))
#define PK_IS_CONNECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PK_TYPE_CONNECTION))
#define PK_IS_CONNECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PK_TYPE_CONNECTION))
#define PK_CONNECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PK_TYPE_CONNECTION, PkConnectionClass))
#define PK_CONNECTION_ERROR           (pk_connection_error_quark())

/**
 * PkConnectionError:
 * @PK_CONNECTION_ERROR_NOT_SUPPORTED: The operation is not supported.
 * @PK_CONNECTION_ERROR_STATE: The connection is in an invalid state.
 *
 * The #PkConnection error enumeration.
 */
typedef enum
{
	PK_CONNECTION_ERROR_NOT_SUPPORTED,
	PK_CONNECTION_ERROR_STATE,
} PkConnectionError;

/**
 * PkConnectionState:
 * @PK_CONNECTION_DISCONNECTED: The connection is not established.
 * @PK_CONNECTION_CONNECTED: The connection is established.
 * @PK_CONNECTION_FAILED: The connection had unrecoverable failures.
 *
 * The #PkConnection state enumeration.
 */
typedef enum
{
	PK_CONNECTION_DISCONNECTED,
	PK_CONNECTION_CONNECTED,
	PK_CONNECTION_FAILED,
} PkConnectionState;

typedef struct _PkConnection        PkConnection;
typedef struct _PkConnectionClass   PkConnectionClass;
typedef struct _PkConnectionPrivate PkConnectionPrivate;

struct _PkConnection
{
	GObject parent;

	/*< private >*/
	PkConnectionPrivate *priv;
};

struct _PkConnectionClass
{
	GObjectClass parent_class;

	/*
	 * Establish connection to daemon.
	 */
	void     (*connect_async)     (PkConnection         *connection,
	                               GCancellable         *cancellable,
	                               GAsyncReadyCallback   callback,
	                               gpointer              user_data);
	gboolean (*connect_finish)    (PkConnection         *connection,
	                               GAsyncResult         *result,
	                               GError              **error);

	/*
	 * Break active connection to daemon.
	 */
	void     (*disconnect_async)  (PkConnection         *connection,
	                               GCancellable         *cancellable,
	                               GAsyncReadyCallback   callback,
	                               gpointer              user_data);
	gboolean (*disconnect_finish) (PkConnection         *connection,
	                               GAsyncResult         *result,
	                               GError              **error);

	/*
	 * Connection RPCs.
	 */

	/**
	 * The "plugin_get_name" RPC.
	 */
	void     (*plugin_get_name_async)              (PkConnection          *connection,
	                                                gchar                 *plugin,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*plugin_get_name_finish)             (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                gchar                **name,
	                                                GError               **error);

	/**
	 * The "plugin_get_description" RPC.
	 */
	void     (*plugin_get_description_async)       (PkConnection          *connection,
	                                                gchar                 *plugin,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*plugin_get_description_finish)      (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                gchar                **description,
	                                                GError               **error);

	/**
	 * The "plugin_get_version" RPC.
	 */
	void     (*plugin_get_version_async)           (PkConnection          *connection,
	                                                gchar                 *plugin,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*plugin_get_version_finish)          (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                gchar                **version,
	                                                GError               **error);

	/**
	 * The "plugin_get_plugin_type" RPC.
	 */
	void     (*plugin_get_plugin_type_async)       (PkConnection          *connection,
	                                                gchar                 *plugin,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*plugin_get_plugin_type_finish)      (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                gint                  *type,
	                                                GError               **error);

	/**
	 * The "encoder_set_property" RPC.
	 */
	void     (*encoder_set_property_async)         (PkConnection          *connection,
	                                                gint                   encoder,
	                                                const gchar           *name,
	                                                const GValue          *value,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*encoder_set_property_finish)        (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                GError               **error);

	/**
	 * The "encoder_get_property" RPC.
	 */
	void     (*encoder_get_property_async)         (PkConnection          *connection,
	                                                gint                   encoder,
	                                                const gchar           *name,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*encoder_get_property_finish)        (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                GValue                *value,
	                                                GError               **error);

	/**
	 * The "source_set_property" RPC.
	 */
	void     (*source_set_property_async)          (PkConnection          *connection,
	                                                gint                   source,
	                                                const gchar           *name,
	                                                const GValue          *value,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*source_set_property_finish)         (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                GError               **error);

	/**
	 * The "source_get_property" RPC.
	 */
	void     (*source_get_property_async)          (PkConnection          *connection,
	                                                gint                   source,
	                                                const gchar           *name,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*source_get_property_finish)         (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                GValue                *value,
	                                                GError               **error);

	/**
	 * The "source_get_plugin" RPC.
	 */
	void     (*source_get_plugin_async)            (PkConnection          *connection,
	                                                gint                   source,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*source_get_plugin_finish)           (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                gchar                **plugin,
	                                                GError               **error);

	/**
	 * The "manager_get_channels" RPC.
	 */
	void     (*manager_get_channels_async)         (PkConnection          *connection,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*manager_get_channels_finish)        (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                gint                 **channels,
	                                                gsize                 *channels_len,
	                                                GError               **error);

	/**
	 * The "manager_get_source_plugins" RPC.
	 */
	void     (*manager_get_source_plugins_async)   (PkConnection          *connection,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*manager_get_source_plugins_finish)  (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                gchar               ***plugins,
	                                                GError               **error);

	/**
	 * The "manager_get_version" RPC.
	 */
	void     (*manager_get_version_async)          (PkConnection          *connection,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*manager_get_version_finish)         (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                gchar                **version,
	                                                GError               **error);

	/**
	 * The "manager_ping" RPC.
	 */
	void     (*manager_ping_async)                 (PkConnection          *connection,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*manager_ping_finish)                (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                GTimeVal              *tv,
	                                                GError               **error);

	/**
	 * The "manager_add_channel" RPC.
	 */
	void     (*manager_add_channel_async)          (PkConnection          *connection,
	                                                const PkSpawnInfo     *spawn_info,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*manager_add_channel_finish)         (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                gint                  *channel,
	                                                GError               **error);

	/**
	 * The "manager_remove_channel" RPC.
	 */
	void     (*manager_remove_channel_async)       (PkConnection          *connection,
	                                                gint                   channel,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*manager_remove_channel_finish)      (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                GError               **error);

	/**
	 * The "manager_add_subscription" RPC.
	 */
	void     (*manager_add_subscription_async)     (PkConnection          *connection,
	                                                gint                   channel,
	                                                gsize                  buffer_size,
	                                                gulong                 buffer_timeout,
	                                                const gchar           *encoder,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*manager_add_subscription_finish)    (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                gint                  *subscription,
	                                                GError               **error);

	/**
	 * The "manager_remove_subscription" RPC.
	 */
	void     (*manager_remove_subscription_async)  (PkConnection          *connection,
	                                                gint                   subscription,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*manager_remove_subscription_finish) (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                GError               **error);

	/**
	 * The "channel_get_args" RPC.
	 */
	void     (*channel_get_args_async)             (PkConnection          *connection,
	                                                gint                   channel,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*channel_get_args_finish)            (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                gchar               ***args,
	                                                GError               **error);

	/**
	 * The "channel_get_env" RPC.
	 */
	void     (*channel_get_env_async)              (PkConnection          *connection,
	                                                gint                   channel,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*channel_get_env_finish)             (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                gchar               ***env,
	                                                GError               **error);

	/**
	 * The "channel_get_pid" RPC.
	 */
	void     (*channel_get_pid_async)              (PkConnection          *connection,
	                                                gint                   channel,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*channel_get_pid_finish)             (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                GPid                  *pid,
	                                                GError               **error);

	/**
	 * The "channel_get_state" RPC.
	 */
	void     (*channel_get_state_async)            (PkConnection          *connection,
	                                                gint                   channel,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*channel_get_state_finish)           (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                gint                  *state,
	                                                GError               **error);

	/**
	 * The "channel_get_target" RPC.
	 */
	void     (*channel_get_target_async)           (PkConnection          *connection,
	                                                gint                   channel,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*channel_get_target_finish)          (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                gchar                **target,
	                                                GError               **error);

	/**
	 * The "channel_get_working_dir" RPC.
	 */
	void     (*channel_get_working_dir_async)      (PkConnection          *connection,
	                                                gint                   channel,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*channel_get_working_dir_finish)     (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                gchar                **working_dir,
	                                                GError               **error);

	/**
	 * The "channel_add_source" RPC.
	 */
	void     (*channel_add_source_async)           (PkConnection          *connection,
	                                                gint                   channel,
	                                                const gchar           *plugin,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*channel_add_source_finish)          (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                gint                  *source,
	                                                GError               **error);

	/**
	 * The "channel_remove_source" RPC.
	 */
	void     (*channel_remove_source_async)        (PkConnection          *connection,
	                                                gint                   channel,
	                                                gint                   source,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*channel_remove_source_finish)       (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                GError               **error);

	/**
	 * The "channel_start" RPC.
	 */
	void     (*channel_start_async)                (PkConnection          *connection,
	                                                gint                   channel,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*channel_start_finish)               (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                GError               **error);

	/**
	 * The "channel_stop" RPC.
	 */
	void     (*channel_stop_async)                 (PkConnection          *connection,
	                                                gint                   channel,
	                                                gboolean               killpid,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*channel_stop_finish)                (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                GError               **error);

	/**
	 * The "channel_pause" RPC.
	 */
	void     (*channel_pause_async)                (PkConnection          *connection,
	                                                gint                   channel,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*channel_pause_finish)               (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                GError               **error);

	/**
	 * The "channel_unpause" RPC.
	 */
	void     (*channel_unpause_async)              (PkConnection          *connection,
	                                                gint                   channel,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*channel_unpause_finish)             (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                GError               **error);

	/**
	 * The "subscription_enable" RPC.
	 */
	void     (*subscription_enable_async)          (PkConnection          *connection,
	                                                gint                   subscription,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*subscription_enable_finish)         (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                GError               **error);

	/**
	 * The "subscription_disable" RPC.
	 */
	void     (*subscription_disable_async)         (PkConnection          *connection,
	                                                gint                   subscription,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*subscription_disable_finish)        (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                GError               **error);

	/**
	 * The "subscription_set_handlers" RPC.
	 */
	void     (*subscription_set_handlers_async)    (PkConnection          *connection,
	                                                gint                   subscription,
	                                                GFunc                  manifest,
	                                                GFunc                  sample,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*subscription_set_handlers_finish)   (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                GError               **error);

	/**
	 * The "subscription_get_encoder" RPC.
	 */
	void     (*subscription_get_encoder_async)     (PkConnection          *connection,
	                                                gint                   subscription,
	                                                GCancellable          *cancellable,
	                                                GAsyncReadyCallback    callback,
	                                                gpointer               user_data);
	gboolean (*subscription_get_encoder_finish)    (PkConnection          *connection,
	                                                GAsyncResult          *result,
	                                                gint                  *encoder,
	                                                GError               **error);


	/*
	 * Padding for future ABI additions.
	 */
	gpointer reserved[190];

};

gboolean      pk_connection_connect            (PkConnection         *connection,
                                                GError              **error);
void          pk_connection_connect_async      (PkConnection         *connection,
                                                GCancellable         *cancellable,
                                                GAsyncReadyCallback   callback,
                                                gpointer              user_data);
gboolean      pk_connection_connect_finish     (PkConnection         *connection,
                                                GAsyncResult         *result,
                                                GError              **error);
gboolean      pk_connection_disconnect         (PkConnection         *connection,
                                                GError              **error);
void          pk_connection_disconnect_async   (PkConnection         *connection,
                                                GCancellable         *cancellable,
                                                GAsyncReadyCallback   callback,
                                                gpointer              user_data);
gboolean      pk_connection_disconnect_finish  (PkConnection         *connection,
                                                GAsyncResult         *result,
                                                GError              **error);
void          pk_connection_emit_state_changed (PkConnection         *connection,
                                                PkConnectionState     state);
GQuark        pk_connection_error_quark        (void) G_GNUC_CONST;
GType         pk_connection_get_type           (void) G_GNUC_CONST;
const gchar*  pk_connection_get_uri            (PkConnection         *connection) G_GNUC_PURE;
guint         pk_connection_hash               (PkConnection         *connection) G_GNUC_PURE;
gboolean      pk_connection_is_connected       (PkConnection         *connection);
PkConnection* pk_connection_new_from_uri       (const gchar          *uri);

G_END_DECLS

#endif /* __PK_CONNECTION_H__ */
