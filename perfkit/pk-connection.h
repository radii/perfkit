/* pk-connection.h
 *
 * Copyright 2010 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
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
 * PkConnectionState:
 * @PK_CONNECTION_DISCONNECTED: 
 * @PK_CONNECTION_CONNECTED: 
 *
 * The #PkConnection state enumeration.
 */
typedef enum
{
	PK_CONNECTION_DISCONNECTED = 0,
	PK_CONNECTION_CONNECTED    = 1,
	PK_CONNECTION_FAILED       = 2,
} PkConnectionState;

/**
 * PkConnectionError:
 * @PK_CONNECTION_ERROR_NOT_IMPLEMENTED: 
 *
 * The #PkConnection error enumeration.
 */
typedef enum
{
	PK_CONNECTION_ERROR_NOT_IMPLEMENTED,
} PkConnectionError;

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

	void          (*channel_add_source_async)           (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     gint                   source,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_add_source_finish)          (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*channel_get_args_async)             (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_get_args_finish)            (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gchar               ***args,
	                                                     GError               **error);
	void          (*channel_get_env_async)              (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_get_env_finish)             (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gchar               ***env,
	                                                     GError               **error);
	void          (*channel_get_exit_status_async)      (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_get_exit_status_finish)     (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gint                  *exit_status,
	                                                     GError               **error);
	void          (*channel_get_kill_pid_async)         (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_get_kill_pid_finish)        (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gboolean              *kill_pid,
	                                                     GError               **error);
	void          (*channel_get_pid_async)              (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_get_pid_finish)             (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gint                  *pid,
	                                                     GError               **error);
	void          (*channel_get_pid_set_async)          (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_get_pid_set_finish)         (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gboolean              *pid_set,
	                                                     GError               **error);
	void          (*channel_get_sources_async)          (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_get_sources_finish)         (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gint                 **sources,
	                                                     gsize                 *sources_len,
	                                                     GError               **error);
	void          (*channel_get_state_async)            (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_get_state_finish)           (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gint                  *state,
	                                                     GError               **error);
	void          (*channel_get_target_async)           (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_get_target_finish)          (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gchar                **target,
	                                                     GError               **error);
	void          (*channel_get_working_dir_async)      (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_get_working_dir_finish)     (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gchar                **working_dir,
	                                                     GError               **error);
	void          (*channel_mute_async)                 (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_mute_finish)                (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*channel_set_args_async)             (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     gchar                **args,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_set_args_finish)            (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*channel_set_env_async)              (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     gchar                **env,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_set_env_finish)             (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*channel_set_kill_pid_async)         (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     gboolean               kill_pid,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_set_kill_pid_finish)        (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*channel_set_pid_async)              (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     gint                   pid,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_set_pid_finish)             (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*channel_set_target_async)           (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     const gchar           *target,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_set_target_finish)          (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*channel_set_working_dir_async)      (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     const gchar           *working_dir,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_set_working_dir_finish)     (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*channel_start_async)                (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_start_finish)               (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*channel_stop_async)                 (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_stop_finish)                (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*channel_unmute_async)               (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*channel_unmute_finish)              (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*connect_async)                      (PkConnection          *connection,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*connect_finish)                     (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*disconnect_async)                   (PkConnection          *connection,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*disconnect_finish)                  (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*encoder_get_plugin_async)           (PkConnection          *connection,
	                                                     gint                   encoder,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*encoder_get_plugin_finish)          (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gchar                **pluign,
	                                                     GError               **error);
	void          (*manager_add_channel_async)          (PkConnection          *connection,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*manager_add_channel_finish)         (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gint                  *channel,
	                                                     GError               **error);
	void          (*manager_add_source_async)           (PkConnection          *connection,
	                                                     const gchar           *plugin,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*manager_add_source_finish)          (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gint                  *source,
	                                                     GError               **error);
	void          (*manager_add_subscription_async)     (PkConnection          *connection,
	                                                     gsize                  buffer_size,
	                                                     gsize                  timeout,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*manager_add_subscription_finish)    (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gint                  *subscription,
	                                                     GError               **error);
	void          (*manager_get_channels_async)         (PkConnection          *connection,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*manager_get_channels_finish)        (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gint                 **channels,
	                                                     gsize                 *channels_len,
	                                                     GError               **error);
	void          (*manager_get_plugins_async)          (PkConnection          *connection,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*manager_get_plugins_finish)         (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gchar               ***plugins,
	                                                     GError               **error);
	void          (*manager_get_sources_async)          (PkConnection          *connection,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*manager_get_sources_finish)         (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gint                 **sources,
	                                                     gsize                 *sources_len,
	                                                     GError               **error);
	void          (*manager_get_version_async)          (PkConnection          *connection,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*manager_get_version_finish)         (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gchar                **version,
	                                                     GError               **error);
	void          (*manager_ping_async)                 (PkConnection          *connection,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*manager_ping_finish)                (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GTimeVal              *tv,
	                                                     GError               **error);
	void          (*manager_remove_channel_async)       (PkConnection          *connection,
	                                                     gint                   channel,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*manager_remove_channel_finish)      (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gboolean              *removed,
	                                                     GError               **error);
	void          (*manager_remove_source_async)        (PkConnection          *connection,
	                                                     gint                   source,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*manager_remove_source_finish)       (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*manager_remove_subscription_async)  (PkConnection          *connection,
	                                                     gint                   subscription,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*manager_remove_subscription_finish) (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gboolean              *removed,
	                                                     GError               **error);
	void          (*plugin_get_copyright_async)         (PkConnection          *connection,
	                                                     const gchar           *plugin,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*plugin_get_copyright_finish)        (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gchar                **copyright,
	                                                     GError               **error);
	void          (*plugin_get_description_async)       (PkConnection          *connection,
	                                                     const gchar           *plugin,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*plugin_get_description_finish)      (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gchar                **description,
	                                                     GError               **error);
	void          (*plugin_get_name_async)              (PkConnection          *connection,
	                                                     const gchar           *plugin,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*plugin_get_name_finish)             (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gchar                **name,
	                                                     GError               **error);
	void          (*plugin_get_plugin_type_async)       (PkConnection          *connection,
	                                                     const gchar           *plugin,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*plugin_get_plugin_type_finish)      (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gint                  *type,
	                                                     GError               **error);
	void          (*plugin_get_version_async)           (PkConnection          *connection,
	                                                     const gchar           *plugin,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*plugin_get_version_finish)          (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gchar                **version,
	                                                     GError               **error);
	void          (*source_get_plugin_async)            (PkConnection          *connection,
	                                                     gint                   source,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*source_get_plugin_finish)           (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     gchar                **plugin,
	                                                     GError               **error);
	void          (*subscription_add_channel_async)     (PkConnection          *connection,
	                                                     gint                   subscription,
	                                                     gint                   channel,
	                                                     gboolean               monitor,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*subscription_add_channel_finish)    (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*subscription_add_source_async)      (PkConnection          *connection,
	                                                     gint                   subscription,
	                                                     gint                   source,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*subscription_add_source_finish)     (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*subscription_mute_async)            (PkConnection          *connection,
	                                                     gint                   subscription,
	                                                     gboolean               drain,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*subscription_mute_finish)           (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*subscription_remove_channel_async)  (PkConnection          *connection,
	                                                     gint                   subscription,
	                                                     gint                   channel,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*subscription_remove_channel_finish) (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*subscription_remove_source_async)   (PkConnection          *connection,
	                                                     gint                   subscription,
	                                                     gint                   source,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*subscription_remove_source_finish)  (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*subscription_set_buffer_async)      (PkConnection          *connection,
	                                                     gint                   subscription,
	                                                     gint                   timeout,
	                                                     gint                   size,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*subscription_set_buffer_finish)     (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*subscription_set_encoder_async)     (PkConnection          *connection,
	                                                     gint                   subscription,
	                                                     gint                   encoder,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*subscription_set_encoder_finish)    (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
	void          (*subscription_unmute_async)          (PkConnection          *connection,
	                                                     gint                   subscription,
	                                                     GCancellable          *cancellable,
	                                                     GAsyncReadyCallback    callback,
	                                                     gpointer               user_data);
	gboolean      (*subscription_unmute_finish)         (PkConnection          *connection,
	                                                     GAsyncResult          *result,
	                                                     GError               **error);
};

gboolean      pk_connection_connect                           (PkConnection          *connection,
                                                               GError               **error);
void          pk_connection_connect_async                     (PkConnection          *connection,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_connect_finish                    (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_disconnect                        (PkConnection          *connection,
                                                               GError               **error);
void          pk_connection_disconnect_async                  (PkConnection          *connection,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_disconnect_finish                 (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pk_connection_emit_state_changed                (PkConnection          *connection,
                                                               PkConnectionState      state);
GQuark        pk_connection_error_quark                       (void);
GType         pk_connection_get_type                          (void) G_GNUC_CONST;
const gchar*  pk_connection_get_uri                           (PkConnection          *connection);
guint         pk_connection_hash                              (PkConnection          *connection);
gboolean      pk_connection_is_connected                      (PkConnection          *connection);
PkConnection* pk_connection_new_from_uri                      (const gchar           *uri);

G_END_DECLS

#endif /* __PK_CONNECTION_H__ */