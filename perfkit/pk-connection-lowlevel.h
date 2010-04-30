/* pk-connection-lowlevel.h
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

#ifndef __PK_CONNECTION_LOWLEVEL_H__
#define __PK_CONNECTION_LOWLEVEL_H__

#include "pk-connection.h"
#include "pk-connection-lowlevel.h"

G_BEGIN_DECLS

/*
 * The "plugin_get_name" RPC.
 */
void     pk_connection_plugin_get_name_async              (PkConnection          *connection,
                                                           gchar                 *plugin,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_plugin_get_name_finish             (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           gchar                **name,
                                                           GError               **error);
gboolean pk_connection_plugin_get_name                    (PkConnection          *connection,
                                                           gchar                 *plugin,
                                                           gchar                **name,
                                                           GError               **error);

/*
 * The "plugin_get_description" RPC.
 */
void     pk_connection_plugin_get_description_async       (PkConnection          *connection,
                                                           gchar                 *plugin,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_plugin_get_description_finish      (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           gchar                **description,
                                                           GError               **error);
gboolean pk_connection_plugin_get_description             (PkConnection          *connection,
                                                           gchar                 *plugin,
                                                           gchar                **description,
                                                           GError               **error);

/*
 * The "plugin_get_version" RPC.
 */
void     pk_connection_plugin_get_version_async           (PkConnection          *connection,
                                                           gchar                 *plugin,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_plugin_get_version_finish          (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           gchar                **version,
                                                           GError               **error);
gboolean pk_connection_plugin_get_version                 (PkConnection          *connection,
                                                           gchar                 *plugin,
                                                           gchar                **version,
                                                           GError               **error);

/*
 * The "plugin_get_plugin_type" RPC.
 */
void     pk_connection_plugin_get_plugin_type_async       (PkConnection          *connection,
                                                           gchar                 *plugin,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_plugin_get_plugin_type_finish      (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           gint                  *type,
                                                           GError               **error);
gboolean pk_connection_plugin_get_plugin_type             (PkConnection          *connection,
                                                           gchar                 *plugin,
                                                           gint                  *type,
                                                           GError               **error);

/*
 * The "encoder_set_property" RPC.
 */
void     pk_connection_encoder_set_property_async         (PkConnection          *connection,
                                                           gint                   encoder,
                                                           const gchar           *name,
                                                           const GValue          *value,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_encoder_set_property_finish        (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           GError               **error);
gboolean pk_connection_encoder_set_property               (PkConnection          *connection,
                                                           gint                   encoder,
                                                           const gchar           *name,
                                                           const GValue          *value,
                                                           GError               **error);

/*
 * The "encoder_get_property" RPC.
 */
void     pk_connection_encoder_get_property_async         (PkConnection          *connection,
                                                           gint                   encoder,
                                                           const gchar           *name,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_encoder_get_property_finish        (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           GValue                *value,
                                                           GError               **error);
gboolean pk_connection_encoder_get_property               (PkConnection          *connection,
                                                           gint                   encoder,
                                                           const gchar           *name,
                                                           GValue                *value,
                                                           GError               **error);

/*
 * The "source_set_property" RPC.
 */
void     pk_connection_source_set_property_async          (PkConnection          *connection,
                                                           gint                   source,
                                                           const gchar           *name,
                                                           const GValue          *value,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_source_set_property_finish         (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           GError               **error);
gboolean pk_connection_source_set_property                (PkConnection          *connection,
                                                           gint                   source,
                                                           const gchar           *name,
                                                           const GValue          *value,
                                                           GError               **error);

/*
 * The "source_get_property" RPC.
 */
void     pk_connection_source_get_property_async          (PkConnection          *connection,
                                                           gint                   source,
                                                           const gchar           *name,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_source_get_property_finish         (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           GValue                *value,
                                                           GError               **error);
gboolean pk_connection_source_get_property                (PkConnection          *connection,
                                                           gint                   source,
                                                           const gchar           *name,
                                                           GValue                *value,
                                                           GError               **error);

/*
 * The "source_get_plugin" RPC.
 */
void     pk_connection_source_get_plugin_async            (PkConnection          *connection,
                                                           gint                   source,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_source_get_plugin_finish           (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           gchar                **plugin,
                                                           GError               **error);
gboolean pk_connection_source_get_plugin                  (PkConnection          *connection,
                                                           gint                   source,
                                                           gchar                **plugin,
                                                           GError               **error);

/*
 * The "manager_get_channels" RPC.
 */
void     pk_connection_manager_get_channels_async         (PkConnection          *connection,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_manager_get_channels_finish        (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           gint                 **channels,
                                                           gsize                 *channels_len,
                                                           GError               **error);
gboolean pk_connection_manager_get_channels               (PkConnection          *connection,
                                                           gint                 **channels,
                                                           gsize                 *channels_len,
                                                           GError               **error);

/*
 * The "manager_get_source_plugins" RPC.
 */
void     pk_connection_manager_get_source_plugins_async   (PkConnection          *connection,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_manager_get_source_plugins_finish  (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           gchar               ***plugins,
                                                           GError               **error);
gboolean pk_connection_manager_get_source_plugins         (PkConnection          *connection,
                                                           gchar               ***plugins,
                                                           GError               **error);

/*
 * The "manager_get_version" RPC.
 */
void     pk_connection_manager_get_version_async          (PkConnection          *connection,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_manager_get_version_finish         (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           gchar                **version,
                                                           GError               **error);
gboolean pk_connection_manager_get_version                (PkConnection          *connection,
                                                           gchar                **version,
                                                           GError               **error);

/*
 * The "manager_ping" RPC.
 */
void     pk_connection_manager_ping_async                 (PkConnection          *connection,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_manager_ping_finish                (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           GTimeVal              *tv,
                                                           GError               **error);
gboolean pk_connection_manager_ping                       (PkConnection          *connection,
                                                           GTimeVal              *tv,
                                                           GError               **error);

/*
 * The "manager_add_channel" RPC.
 */
void     pk_connection_manager_add_channel_async          (PkConnection          *connection,
                                                           const PkSpawnInfo     *spawn_info,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_manager_add_channel_finish         (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           gint                  *channel,
                                                           GError               **error);
gboolean pk_connection_manager_add_channel                (PkConnection          *connection,
                                                           const PkSpawnInfo     *spawn_info,
                                                           gint                  *channel,
                                                           GError               **error);

/*
 * The "manager_remove_channel" RPC.
 */
void     pk_connection_manager_remove_channel_async       (PkConnection          *connection,
                                                           gint                   channel,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_manager_remove_channel_finish      (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           GError               **error);
gboolean pk_connection_manager_remove_channel             (PkConnection          *connection,
                                                           gint                   channel,
                                                           GError               **error);

/*
 * The "manager_add_subscription" RPC.
 */
void     pk_connection_manager_add_subscription_async     (PkConnection          *connection,
                                                           gint                   channel,
                                                           gsize                  buffer_size,
                                                           gulong                 buffer_timeout,
                                                           const gchar           *encoder,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_manager_add_subscription_finish    (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           gint                  *subscription,
                                                           GError               **error);
gboolean pk_connection_manager_add_subscription           (PkConnection          *connection,
                                                           gint                   channel,
                                                           gsize                  buffer_size,
                                                           gulong                 buffer_timeout,
                                                           const gchar           *encoder,
                                                           gint                  *subscription,
                                                           GError               **error);

/*
 * The "manager_remove_subscription" RPC.
 */
void     pk_connection_manager_remove_subscription_async  (PkConnection          *connection,
                                                           gint                   subscription,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_manager_remove_subscription_finish (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           GError               **error);
gboolean pk_connection_manager_remove_subscription        (PkConnection          *connection,
                                                           gint                   subscription,
                                                           GError               **error);

/*
 * The "channel_get_args" RPC.
 */
void     pk_connection_channel_get_args_async             (PkConnection          *connection,
                                                           gint                   channel,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_channel_get_args_finish            (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           gchar               ***args,
                                                           GError               **error);
gboolean pk_connection_channel_get_args                   (PkConnection          *connection,
                                                           gint                   channel,
                                                           gchar               ***args,
                                                           GError               **error);

/*
 * The "channel_get_env" RPC.
 */
void     pk_connection_channel_get_env_async              (PkConnection          *connection,
                                                           gint                   channel,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_channel_get_env_finish             (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           gchar               ***env,
                                                           GError               **error);
gboolean pk_connection_channel_get_env                    (PkConnection          *connection,
                                                           gint                   channel,
                                                           gchar               ***env,
                                                           GError               **error);

/*
 * The "channel_get_pid" RPC.
 */
void     pk_connection_channel_get_pid_async              (PkConnection          *connection,
                                                           gint                   channel,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_channel_get_pid_finish             (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           GPid                  *pid,
                                                           GError               **error);
gboolean pk_connection_channel_get_pid                    (PkConnection          *connection,
                                                           gint                   channel,
                                                           GPid                  *pid,
                                                           GError               **error);

/*
 * The "channel_get_state" RPC.
 */
void     pk_connection_channel_get_state_async            (PkConnection          *connection,
                                                           gint                   channel,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_channel_get_state_finish           (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           gint                  *state,
                                                           GError               **error);
gboolean pk_connection_channel_get_state                  (PkConnection          *connection,
                                                           gint                   channel,
                                                           gint                  *state,
                                                           GError               **error);

/*
 * The "channel_get_target" RPC.
 */
void     pk_connection_channel_get_target_async           (PkConnection          *connection,
                                                           gint                   channel,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_channel_get_target_finish          (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           gchar                **target,
                                                           GError               **error);
gboolean pk_connection_channel_get_target                 (PkConnection          *connection,
                                                           gint                   channel,
                                                           gchar                **target,
                                                           GError               **error);

/*
 * The "channel_get_working_dir" RPC.
 */
void     pk_connection_channel_get_working_dir_async      (PkConnection          *connection,
                                                           gint                   channel,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_channel_get_working_dir_finish     (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           gchar                **working_dir,
                                                           GError               **error);
gboolean pk_connection_channel_get_working_dir            (PkConnection          *connection,
                                                           gint                   channel,
                                                           gchar                **working_dir,
                                                           GError               **error);

/*
 * The "channel_add_source" RPC.
 */
void     pk_connection_channel_add_source_async           (PkConnection          *connection,
                                                           gint                   channel,
                                                           const gchar           *plugin,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_channel_add_source_finish          (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           gint                  *source,
                                                           GError               **error);
gboolean pk_connection_channel_add_source                 (PkConnection          *connection,
                                                           gint                   channel,
                                                           const gchar           *plugin,
                                                           gint                  *source,
                                                           GError               **error);

/*
 * The "channel_remove_source" RPC.
 */
void     pk_connection_channel_remove_source_async        (PkConnection          *connection,
                                                           gint                   channel,
                                                           gint                   source,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_channel_remove_source_finish       (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           GError               **error);
gboolean pk_connection_channel_remove_source              (PkConnection          *connection,
                                                           gint                   channel,
                                                           gint                   source,
                                                           GError               **error);

/*
 * The "channel_start" RPC.
 */
void     pk_connection_channel_start_async                (PkConnection          *connection,
                                                           gint                   channel,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_channel_start_finish               (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           GError               **error);
gboolean pk_connection_channel_start                      (PkConnection          *connection,
                                                           gint                   channel,
                                                           GError               **error);

/*
 * The "channel_stop" RPC.
 */
void     pk_connection_channel_stop_async                 (PkConnection          *connection,
                                                           gint                   channel,
                                                           gboolean               killpid,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_channel_stop_finish                (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           GError               **error);
gboolean pk_connection_channel_stop                       (PkConnection          *connection,
                                                           gint                   channel,
                                                           gboolean               killpid,
                                                           GError               **error);

/*
 * The "channel_pause" RPC.
 */
void     pk_connection_channel_pause_async                (PkConnection          *connection,
                                                           gint                   channel,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_channel_pause_finish               (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           GError               **error);
gboolean pk_connection_channel_pause                      (PkConnection          *connection,
                                                           gint                   channel,
                                                           GError               **error);

/*
 * The "channel_unpause" RPC.
 */
void     pk_connection_channel_unpause_async              (PkConnection          *connection,
                                                           gint                   channel,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_channel_unpause_finish             (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           GError               **error);
gboolean pk_connection_channel_unpause                    (PkConnection          *connection,
                                                           gint                   channel,
                                                           GError               **error);

/*
 * The "subscription_enable" RPC.
 */
void     pk_connection_subscription_enable_async          (PkConnection          *connection,
                                                           gint                   subscription,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_subscription_enable_finish         (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           GError               **error);
gboolean pk_connection_subscription_enable                (PkConnection          *connection,
                                                           gint                   subscription,
                                                           GError               **error);

/*
 * The "subscription_disable" RPC.
 */
void     pk_connection_subscription_disable_async         (PkConnection          *connection,
                                                           gint                   subscription,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_subscription_disable_finish        (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           GError               **error);
gboolean pk_connection_subscription_disable               (PkConnection          *connection,
                                                           gint                   subscription,
                                                           GError               **error);

/*
 * The "subscription_set_handlers" RPC.
 */
void     pk_connection_subscription_set_handlers_async    (PkConnection          *connection,
                                                           gint                   subscription,
                                                           GFunc                  manifest,
                                                           GFunc                  sample,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_subscription_set_handlers_finish   (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           GError               **error);
gboolean pk_connection_subscription_set_handlers          (PkConnection          *connection,
                                                           gint                   subscription,
                                                           GFunc                  manifest,
                                                           GFunc                  sample,
                                                           GError               **error);

/*
 * The "subscription_get_encoder" RPC.
 */
void     pk_connection_subscription_get_encoder_async     (PkConnection          *connection,
                                                           gint                   subscription,
                                                           GCancellable          *cancellable,
                                                           GAsyncReadyCallback    callback,
                                                           gpointer               user_data);
gboolean pk_connection_subscription_get_encoder_finish    (PkConnection          *connection,
                                                           GAsyncResult          *result,
                                                           gint                  *encoder,
                                                           GError               **error);
gboolean pk_connection_subscription_get_encoder           (PkConnection          *connection,
                                                           gint                   subscription,
                                                           gint                  *encoder,
                                                           GError               **error);

G_END_DECLS

#endif /* __PK_CONNECTION_LOWLEVEL_H__ */
