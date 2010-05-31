/* pk-connection-lowlevel.h
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

#ifndef __PK_CONNECTION_LOWLEVEL_H__
#define __PK_CONNECTION_LOWLEVEL_H__

#include "pk-connection.h"

G_BEGIN_DECLS

gboolean      pk_connection_channel_add_source                (PkConnection          *connection,
                                                               gint                   channel,
                                                               gint                   source,
                                                               GError               **error);
void          pk_connection_channel_add_source_async          (PkConnection          *connection,
                                                               gint                   channel,
                                                               gint                   source,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_add_source_finish         (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_channel_get_args                  (PkConnection          *connection,
                                                               gint                   channel,
                                                               gchar               ***args,
                                                               GError               **error);
void          pk_connection_channel_get_args_async            (PkConnection          *connection,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_get_args_finish           (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gchar               ***args,
                                                               GError               **error);
gboolean      pk_connection_channel_get_env                   (PkConnection          *connection,
                                                               gint                   channel,
                                                               gchar               ***env,
                                                               GError               **error);
void          pk_connection_channel_get_env_async             (PkConnection          *connection,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_get_env_finish            (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gchar               ***env,
                                                               GError               **error);
gboolean      pk_connection_channel_get_exit_status           (PkConnection          *connection,
                                                               gint                   channel,
                                                               gint                  *exit_status,
                                                               GError               **error);
void          pk_connection_channel_get_exit_status_async     (PkConnection          *connection,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_get_exit_status_finish    (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gint                  *exit_status,
                                                               GError               **error);
gboolean      pk_connection_channel_get_kill_pid              (PkConnection          *connection,
                                                               gint                   channel,
                                                               gboolean              *kill_pid,
                                                               GError               **error);
void          pk_connection_channel_get_kill_pid_async        (PkConnection          *connection,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_get_kill_pid_finish       (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gboolean              *kill_pid,
                                                               GError               **error);
gboolean      pk_connection_channel_get_pid                   (PkConnection          *connection,
                                                               gint                   channel,
                                                               gint                  *pid,
                                                               GError               **error);
void          pk_connection_channel_get_pid_async             (PkConnection          *connection,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_get_pid_finish            (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gint                  *pid,
                                                               GError               **error);
gboolean      pk_connection_channel_get_pid_set               (PkConnection          *connection,
                                                               gint                   channel,
                                                               gboolean              *pid_set,
                                                               GError               **error);
void          pk_connection_channel_get_pid_set_async         (PkConnection          *connection,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_get_pid_set_finish        (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gboolean              *pid_set,
                                                               GError               **error);
gboolean      pk_connection_channel_get_sources               (PkConnection          *connection,
                                                               gint                   channel,
                                                               gint                 **sources,
                                                               gsize                 *sources_len,
                                                               GError               **error);
void          pk_connection_channel_get_sources_async         (PkConnection          *connection,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_get_sources_finish        (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gint                 **sources,
                                                               gsize                 *sources_len,
                                                               GError               **error);
gboolean      pk_connection_channel_get_state                 (PkConnection          *connection,
                                                               gint                   channel,
                                                               gint                  *state,
                                                               GError               **error);
void          pk_connection_channel_get_state_async           (PkConnection          *connection,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_get_state_finish          (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gint                  *state,
                                                               GError               **error);
gboolean      pk_connection_channel_get_target                (PkConnection          *connection,
                                                               gint                   channel,
                                                               gchar                **target,
                                                               GError               **error);
void          pk_connection_channel_get_target_async          (PkConnection          *connection,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_get_target_finish         (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gchar                **target,
                                                               GError               **error);
gboolean      pk_connection_channel_get_working_dir           (PkConnection          *connection,
                                                               gint                   channel,
                                                               gchar                **working_dir,
                                                               GError               **error);
void          pk_connection_channel_get_working_dir_async     (PkConnection          *connection,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_get_working_dir_finish    (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gchar                **working_dir,
                                                               GError               **error);
gboolean      pk_connection_channel_mute                      (PkConnection          *connection,
                                                               gint                   channel,
                                                               GError               **error);
void          pk_connection_channel_mute_async                (PkConnection          *connection,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_mute_finish               (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_channel_set_args                  (PkConnection          *connection,
                                                               gint                   channel,
                                                               gchar                **args,
                                                               GError               **error);
void          pk_connection_channel_set_args_async            (PkConnection          *connection,
                                                               gint                   channel,
                                                               gchar                **args,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_set_args_finish           (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_channel_set_env                   (PkConnection          *connection,
                                                               gint                   channel,
                                                               gchar                **env,
                                                               GError               **error);
void          pk_connection_channel_set_env_async             (PkConnection          *connection,
                                                               gint                   channel,
                                                               gchar                **env,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_set_env_finish            (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_channel_set_kill_pid              (PkConnection          *connection,
                                                               gint                   channel,
                                                               gboolean               kill_pid,
                                                               GError               **error);
void          pk_connection_channel_set_kill_pid_async        (PkConnection          *connection,
                                                               gint                   channel,
                                                               gboolean               kill_pid,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_set_kill_pid_finish       (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_channel_set_pid                   (PkConnection          *connection,
                                                               gint                   channel,
                                                               gint                   pid,
                                                               GError               **error);
void          pk_connection_channel_set_pid_async             (PkConnection          *connection,
                                                               gint                   channel,
                                                               gint                   pid,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_set_pid_finish            (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_channel_set_target                (PkConnection          *connection,
                                                               gint                   channel,
                                                               const gchar           *target,
                                                               GError               **error);
void          pk_connection_channel_set_target_async          (PkConnection          *connection,
                                                               gint                   channel,
                                                               const gchar           *target,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_set_target_finish         (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_channel_set_working_dir           (PkConnection          *connection,
                                                               gint                   channel,
                                                               const gchar           *working_dir,
                                                               GError               **error);
void          pk_connection_channel_set_working_dir_async     (PkConnection          *connection,
                                                               gint                   channel,
                                                               const gchar           *working_dir,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_set_working_dir_finish    (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_channel_start                     (PkConnection          *connection,
                                                               gint                   channel,
                                                               GError               **error);
void          pk_connection_channel_start_async               (PkConnection          *connection,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_start_finish              (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_channel_stop                      (PkConnection          *connection,
                                                               gint                   channel,
                                                               GError               **error);
void          pk_connection_channel_stop_async                (PkConnection          *connection,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_stop_finish               (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_channel_unmute                    (PkConnection          *connection,
                                                               gint                   channel,
                                                               GError               **error);
void          pk_connection_channel_unmute_async              (PkConnection          *connection,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_unmute_finish             (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_encoder_get_plugin                (PkConnection          *connection,
                                                               gint                   encoder,
                                                               gchar                **plugin,
                                                               GError               **error);
void          pk_connection_encoder_get_plugin_async          (PkConnection          *connection,
                                                               gint                   encoder,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_encoder_get_plugin_finish         (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gchar                **plugin,
                                                               GError               **error);
gboolean      pk_connection_manager_add_channel               (PkConnection          *connection,
                                                               gint                  *channel,
                                                               GError               **error);
void          pk_connection_manager_add_channel_async         (PkConnection          *connection,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_manager_add_channel_finish        (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gint                  *channel,
                                                               GError               **error);
gboolean      pk_connection_manager_add_source                (PkConnection          *connection,
                                                               const gchar           *plugin,
                                                               gint                  *source,
                                                               GError               **error);
void          pk_connection_manager_add_source_async          (PkConnection          *connection,
                                                               const gchar           *plugin,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_manager_add_source_finish         (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gint                  *source,
                                                               GError               **error);
gboolean      pk_connection_manager_add_subscription          (PkConnection          *connection,
                                                               gsize                  buffer_size,
                                                               gsize                  timeout,
                                                               gint                  *subscription,
                                                               GError               **error);
void          pk_connection_manager_add_subscription_async    (PkConnection          *connection,
                                                               gsize                  buffer_size,
                                                               gsize                  timeout,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_manager_add_subscription_finish   (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gint                  *subscription,
                                                               GError               **error);
gboolean      pk_connection_manager_get_channels              (PkConnection          *connection,
                                                               gint                 **channels,
                                                               gsize                 *channels_len,
                                                               GError               **error);
void          pk_connection_manager_get_channels_async        (PkConnection          *connection,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_manager_get_channels_finish       (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gint                 **channels,
                                                               gsize                 *channels_len,
                                                               GError               **error);
gboolean      pk_connection_manager_get_plugins               (PkConnection          *connection,
                                                               gchar               ***plugins,
                                                               GError               **error);
void          pk_connection_manager_get_plugins_async         (PkConnection          *connection,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_manager_get_plugins_finish        (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gchar               ***plugins,
                                                               GError               **error);
gboolean      pk_connection_manager_get_sources               (PkConnection          *connection,
                                                               gint                 **sources,
                                                               gsize                 *sources_len,
                                                               GError               **error);
void          pk_connection_manager_get_sources_async         (PkConnection          *connection,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_manager_get_sources_finish        (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gint                 **sources,
                                                               gsize                 *sources_len,
                                                               GError               **error);
gboolean      pk_connection_manager_get_version               (PkConnection          *connection,
                                                               gchar                **version,
                                                               GError               **error);
void          pk_connection_manager_get_version_async         (PkConnection          *connection,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_manager_get_version_finish        (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gchar                **version,
                                                               GError               **error);
gboolean      pk_connection_manager_ping                      (PkConnection          *connection,
                                                               GTimeVal              *tv,
                                                               GError               **error);
void          pk_connection_manager_ping_async                (PkConnection          *connection,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_manager_ping_finish               (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GTimeVal              *tv,
                                                               GError               **error);
gboolean      pk_connection_manager_remove_channel            (PkConnection          *connection,
                                                               gint                   channel,
                                                               gboolean              *removed,
                                                               GError               **error);
void          pk_connection_manager_remove_channel_async      (PkConnection          *connection,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_manager_remove_channel_finish     (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gboolean              *removed,
                                                               GError               **error);
gboolean      pk_connection_manager_remove_source             (PkConnection          *connection,
                                                               gint                   source,
                                                               GError               **error);
void          pk_connection_manager_remove_source_async       (PkConnection          *connection,
                                                               gint                   source,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_manager_remove_source_finish      (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_manager_remove_subscription       (PkConnection          *connection,
                                                               gint                   subscription,
                                                               gboolean              *removed,
                                                               GError               **error);
void          pk_connection_manager_remove_subscription_async (PkConnection          *connection,
                                                               gint                   subscription,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_manager_remove_subscription_finish (PkConnection          *connection,
                                                                GAsyncResult          *result,
                                                                gboolean              *removed,
                                                                GError               **error);
gboolean      pk_connection_plugin_get_copyright              (PkConnection          *connection,
                                                               const gchar           *plugin,
                                                               gchar                **copyright,
                                                               GError               **error);
void          pk_connection_plugin_get_copyright_async        (PkConnection          *connection,
                                                               const gchar           *plugin,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_plugin_get_copyright_finish       (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gchar                **copyright,
                                                               GError               **error);
gboolean      pk_connection_plugin_get_description            (PkConnection          *connection,
                                                               const gchar           *plugin,
                                                               gchar                **description,
                                                               GError               **error);
void          pk_connection_plugin_get_description_async      (PkConnection          *connection,
                                                               const gchar           *plugin,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_plugin_get_description_finish     (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gchar                **description,
                                                               GError               **error);
gboolean      pk_connection_plugin_get_name                   (PkConnection          *connection,
                                                               const gchar           *plugin,
                                                               gchar                **name,
                                                               GError               **error);
void          pk_connection_plugin_get_name_async             (PkConnection          *connection,
                                                               const gchar           *plugin,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_plugin_get_name_finish            (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gchar                **name,
                                                               GError               **error);
gboolean      pk_connection_plugin_get_plugin_type            (PkConnection          *connection,
                                                               const gchar           *plugin,
                                                               gint                  *type,
                                                               GError               **error);
void          pk_connection_plugin_get_plugin_type_async      (PkConnection          *connection,
                                                               const gchar           *plugin,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_plugin_get_plugin_type_finish     (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gint                  *type,
                                                               GError               **error);
gboolean      pk_connection_plugin_get_version                (PkConnection          *connection,
                                                               const gchar           *plugin,
                                                               gchar                **version,
                                                               GError               **error);
void          pk_connection_plugin_get_version_async          (PkConnection          *connection,
                                                               const gchar           *plugin,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_plugin_get_version_finish         (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gchar                **version,
                                                               GError               **error);
gboolean      pk_connection_source_get_plugin                 (PkConnection          *connection,
                                                               gint                   source,
                                                               gchar                **plugin,
                                                               GError               **error);
void          pk_connection_source_get_plugin_async           (PkConnection          *connection,
                                                               gint                   source,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_source_get_plugin_finish          (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gchar                **plugin,
                                                               GError               **error);
gboolean      pk_connection_subscription_add_channel          (PkConnection          *connection,
                                                               gint                   subscription,
                                                               gint                   channel,
                                                               gboolean               monitor,
                                                               GError               **error);
void          pk_connection_subscription_add_channel_async    (PkConnection          *connection,
                                                               gint                   subscription,
                                                               gint                   channel,
                                                               gboolean               monitor,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_subscription_add_channel_finish   (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_subscription_add_source           (PkConnection          *connection,
                                                               gint                   subscription,
                                                               gint                   source,
                                                               GError               **error);
void          pk_connection_subscription_add_source_async     (PkConnection          *connection,
                                                               gint                   subscription,
                                                               gint                   source,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_subscription_add_source_finish    (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_subscription_mute                 (PkConnection          *connection,
                                                               gint                   subscription,
                                                               gboolean               drain,
                                                               GError               **error);
void          pk_connection_subscription_mute_async           (PkConnection          *connection,
                                                               gint                   subscription,
                                                               gboolean               drain,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_subscription_mute_finish          (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_subscription_remove_channel       (PkConnection          *connection,
                                                               gint                   subscription,
                                                               gint                   channel,
                                                               GError               **error);
void          pk_connection_subscription_remove_channel_async (PkConnection          *connection,
                                                               gint                   subscription,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_subscription_remove_channel_finish (PkConnection          *connection,
                                                                GAsyncResult          *result,
                                                                GError               **error);
gboolean      pk_connection_subscription_remove_source        (PkConnection          *connection,
                                                               gint                   subscription,
                                                               gint                   source,
                                                               GError               **error);
void          pk_connection_subscription_remove_source_async  (PkConnection          *connection,
                                                               gint                   subscription,
                                                               gint                   source,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_subscription_remove_source_finish (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_subscription_set_buffer           (PkConnection          *connection,
                                                               gint                   subscription,
                                                               gint                   timeout,
                                                               gint                   size,
                                                               GError               **error);
void          pk_connection_subscription_set_buffer_async     (PkConnection          *connection,
                                                               gint                   subscription,
                                                               gint                   timeout,
                                                               gint                   size,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_subscription_set_buffer_finish    (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_subscription_set_encoder          (PkConnection          *connection,
                                                               gint                   subscription,
                                                               gint                   encoder,
                                                               GError               **error);
void          pk_connection_subscription_set_encoder_async    (PkConnection          *connection,
                                                               gint                   subscription,
                                                               gint                   encoder,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_subscription_set_encoder_finish   (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pk_connection_subscription_set_handlers_async   (PkConnection          *connection,
                                                               gint                   subscription,
                                                               PkManifestFunc         manifest_func,
                                                               gpointer               manifest_data,
                                                               GDestroyNotify         manifest_destroy,
                                                               PkSampleFunc           sample_func,
                                                               gpointer               sample_data,
                                                               GDestroyNotify         sample_destroy,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_subscription_set_handlers_finish  (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_subscription_unmute               (PkConnection          *connection,
                                                               gint                   subscription,
                                                               GError               **error);
void          pk_connection_subscription_unmute_async         (PkConnection          *connection,
                                                               gint                   subscription,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_subscription_unmute_finish        (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);

G_END_DECLS

#endif /* __PK_CONNECTION_LOWLEVEL_H__ */
