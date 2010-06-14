/* pka-listener-lowlevel.h
 *
 * Copyright 2010 Christian Hergert <chris@dronelabs.com>
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

#ifndef __PKA_LISTENER_LOWLEVEL_H__
#define __PKA_LISTENER_LOWLEVEL_H__

#include "pka-listener.h"

G_BEGIN_DECLS

void          pka_listener_channel_add_source_async           (PkaListener           *listener,
                                                               gint                   channel,
                                                               gint                   source,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_add_source_finish          (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_channel_get_args_async             (PkaListener           *listener,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_get_args_finish            (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gchar               ***args,
                                                               GError               **error);
void          pka_listener_channel_get_created_at_async       (PkaListener           *listener,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_get_created_at_finish      (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GTimeVal              *tv,
                                                               GError               **error);
void          pka_listener_channel_get_env_async              (PkaListener           *listener,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_get_env_finish             (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gchar               ***env,
                                                               GError               **error);
void          pka_listener_channel_get_exit_status_async      (PkaListener           *listener,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_get_exit_status_finish     (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gint                  *exit_status,
                                                               GError               **error);
void          pka_listener_channel_get_kill_pid_async         (PkaListener           *listener,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_get_kill_pid_finish        (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gboolean              *kill_pid,
                                                               GError               **error);
void          pka_listener_channel_get_pid_async              (PkaListener           *listener,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_get_pid_finish             (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gint                  *pid,
                                                               GError               **error);
void          pka_listener_channel_get_pid_set_async          (PkaListener           *listener,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_get_pid_set_finish         (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gboolean              *pid_set,
                                                               GError               **error);
void          pka_listener_channel_get_sources_async          (PkaListener           *listener,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_get_sources_finish         (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gint                 **sources,
                                                               gsize                 *sources_len,
                                                               GError               **error);
void          pka_listener_channel_get_state_async            (PkaListener           *listener,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_get_state_finish           (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gint                  *state,
                                                               GError               **error);
void          pka_listener_channel_get_target_async           (PkaListener           *listener,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_get_target_finish          (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gchar                **target,
                                                               GError               **error);
void          pka_listener_channel_get_working_dir_async      (PkaListener           *listener,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_get_working_dir_finish     (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gchar                **working_dir,
                                                               GError               **error);
void          pka_listener_channel_mute_async                 (PkaListener           *listener,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_mute_finish                (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_channel_set_args_async             (PkaListener           *listener,
                                                               gint                   channel,
                                                               gchar                **args,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_set_args_finish            (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_channel_set_env_async              (PkaListener           *listener,
                                                               gint                   channel,
                                                               gchar                **env,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_set_env_finish             (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_channel_set_kill_pid_async         (PkaListener           *listener,
                                                               gint                   channel,
                                                               gboolean               kill_pid,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_set_kill_pid_finish        (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_channel_set_pid_async              (PkaListener           *listener,
                                                               gint                   channel,
                                                               gint                   pid,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_set_pid_finish             (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_channel_set_target_async           (PkaListener           *listener,
                                                               gint                   channel,
                                                               const gchar           *target,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_set_target_finish          (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_channel_set_working_dir_async      (PkaListener           *listener,
                                                               gint                   channel,
                                                               const gchar           *working_dir,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_set_working_dir_finish     (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_channel_start_async                (PkaListener           *listener,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_start_finish               (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_channel_stop_async                 (PkaListener           *listener,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_stop_finish                (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_channel_unmute_async               (PkaListener           *listener,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_unmute_finish              (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_encoder_get_plugin_async           (PkaListener           *listener,
                                                               gint                   encoder,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_encoder_get_plugin_finish          (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gchar                **plugin,
                                                               GError               **error);
void          pka_listener_manager_add_channel_async          (PkaListener           *listener,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_manager_add_channel_finish         (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gint                  *channel,
                                                               GError               **error);
void          pka_listener_manager_add_source_async           (PkaListener           *listener,
                                                               const gchar           *plugin,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_manager_add_source_finish          (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gint                  *source,
                                                               GError               **error);
void          pka_listener_manager_add_subscription_async     (PkaListener           *listener,
                                                               gsize                  buffer_size,
                                                               gsize                  timeout,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_manager_add_subscription_finish    (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gint                  *subscription,
                                                               GError               **error);
void          pka_listener_manager_get_channels_async         (PkaListener           *listener,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_manager_get_channels_finish        (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gint                 **channels,
                                                               gsize                 *channels_len,
                                                               GError               **error);
void          pka_listener_manager_get_hostname_async         (PkaListener           *listener,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_manager_get_hostname_finish        (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gchar                **hostname,
                                                               GError               **error);
void          pka_listener_manager_get_plugins_async          (PkaListener           *listener,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_manager_get_plugins_finish         (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gchar               ***plugins,
                                                               GError               **error);
void          pka_listener_manager_get_sources_async          (PkaListener           *listener,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_manager_get_sources_finish         (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gint                 **sources,
                                                               gsize                 *sources_len,
                                                               GError               **error);
void          pka_listener_manager_get_subscriptions_async    (PkaListener           *listener,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_manager_get_subscriptions_finish   (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gint                 **subscriptions,
                                                               gsize                 *subscriptions_len,
                                                               GError               **error);
void          pka_listener_manager_get_version_async          (PkaListener           *listener,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_manager_get_version_finish         (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gchar                **version,
                                                               GError               **error);
void          pka_listener_manager_ping_async                 (PkaListener           *listener,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_manager_ping_finish                (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GTimeVal              *tv,
                                                               GError               **error);
void          pka_listener_manager_remove_channel_async       (PkaListener           *listener,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_manager_remove_channel_finish      (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gboolean              *removed,
                                                               GError               **error);
void          pka_listener_manager_remove_source_async        (PkaListener           *listener,
                                                               gint                   source,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_manager_remove_source_finish       (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_manager_remove_subscription_async  (PkaListener           *listener,
                                                               gint                   subscription,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_manager_remove_subscription_finish (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gboolean              *removed,
                                                               GError               **error);
void          pka_listener_plugin_get_copyright_async         (PkaListener           *listener,
                                                               const gchar           *plugin,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_plugin_get_copyright_finish        (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gchar                **copyright,
                                                               GError               **error);
void          pka_listener_plugin_get_description_async       (PkaListener           *listener,
                                                               const gchar           *plugin,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_plugin_get_description_finish      (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gchar                **description,
                                                               GError               **error);
void          pka_listener_plugin_get_name_async              (PkaListener           *listener,
                                                               const gchar           *plugin,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_plugin_get_name_finish             (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gchar                **name,
                                                               GError               **error);
void          pka_listener_plugin_get_plugin_type_async       (PkaListener           *listener,
                                                               const gchar           *plugin,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_plugin_get_plugin_type_finish      (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gint                  *type,
                                                               GError               **error);
void          pka_listener_plugin_get_version_async           (PkaListener           *listener,
                                                               const gchar           *plugin,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_plugin_get_version_finish          (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gchar                **version,
                                                               GError               **error);
void          pka_listener_source_get_plugin_async            (PkaListener           *listener,
                                                               gint                   source,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_source_get_plugin_finish           (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gchar                **plugin,
                                                               GError               **error);
void          pka_listener_subscription_add_channel_async     (PkaListener           *listener,
                                                               gint                   subscription,
                                                               gint                   channel,
                                                               gboolean               monitor,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_subscription_add_channel_finish    (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_subscription_add_source_async      (PkaListener           *listener,
                                                               gint                   subscription,
                                                               gint                   source,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_subscription_add_source_finish     (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_subscription_get_buffer_async      (PkaListener           *listener,
                                                               gint                   subscription,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_subscription_get_buffer_finish     (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gint                  *timeout,
                                                               gint                  *size,
                                                               GError               **error);
void          pka_listener_subscription_get_created_at_async  (PkaListener           *listener,
                                                               gint                   subscription,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_subscription_get_created_at_finish (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GTimeVal              *tv,
                                                               GError               **error);
void          pka_listener_subscription_get_sources_async     (PkaListener           *listener,
                                                               gint                   subscription,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_subscription_get_sources_finish    (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gint                 **sources,
                                                               gsize                 *sources_len,
                                                               GError               **error);
void          pka_listener_subscription_mute_async            (PkaListener           *listener,
                                                               gint                   subscription,
                                                               gboolean               drain,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_subscription_mute_finish           (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_subscription_remove_channel_async  (PkaListener           *listener,
                                                               gint                   subscription,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_subscription_remove_channel_finish (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_subscription_remove_source_async   (PkaListener           *listener,
                                                               gint                   subscription,
                                                               gint                   source,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_subscription_remove_source_finish  (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_subscription_set_buffer_async      (PkaListener           *listener,
                                                               gint                   subscription,
                                                               gint                   timeout,
                                                               gint                   size,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_subscription_set_buffer_finish     (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_subscription_set_encoder_async     (PkaListener           *listener,
                                                               gint                   subscription,
                                                               gint                   encoder,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_subscription_set_encoder_finish    (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_subscription_unmute_async          (PkaListener           *listener,
                                                               gint                   subscription,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_subscription_unmute_finish         (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);

G_END_DECLS

#endif /* __PKA_LISTENER_LOWLEVEL_H__ */