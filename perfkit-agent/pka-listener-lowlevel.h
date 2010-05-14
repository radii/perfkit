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

void          pka_listener_channel_cork_async                 (PkaListener           *listener,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_cork_finish                (PkaListener           *listener,
                                                               GAsyncResult          *result,
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
                                                               gboolean               killpid,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_stop_finish                (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_channel_uncork_async               (PkaListener           *listener,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_channel_uncork_finish              (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);
void          pka_listener_encoder_get_plugin_async           (PkaListener           *listener,
                                                               gint                   encoder,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_encoder_get_plugin_finish          (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gchar                **pluign,
                                                               GError               **error);
void          pka_listener_manager_add_channel_async          (PkaListener           *listener,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_manager_add_channel_finish         (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gint                  *channel,
                                                               GError               **error);
void          pka_listener_manager_add_subscription_async     (PkaListener           *listener,
                                                               gsize                  buffer_size,
                                                               gsize                  timeout,
                                                               gint                   encoder,
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
void          pka_listener_manager_get_plugins_async          (PkaListener           *listener,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_manager_get_plugins_finish         (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gchar               ***plugins,
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
void          pka_listener_manager_remove_subscription_async  (PkaListener           *listener,
                                                               gint                   subscription,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_manager_remove_subscription_finish (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gboolean              *removed,
                                                               GError               **error);
void          pka_listener_plugin_create_encoder_async        (PkaListener           *listener,
                                                               const gchar           *plugin,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_plugin_create_encoder_finish       (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gint                  *encoder,
                                                               GError               **error);
void          pka_listener_plugin_create_source_async         (PkaListener           *listener,
                                                               const gchar           *plugin,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_plugin_create_source_finish        (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               gint                  *source,
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
void          pka_listener_subscription_cork_async            (PkaListener           *listener,
                                                               gint                   subscription,
                                                               gboolean               drain,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_subscription_cork_finish           (PkaListener           *listener,
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
void          pka_listener_subscription_uncork_async          (PkaListener           *listener,
                                                               gint                   subscription,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pka_listener_subscription_uncork_finish         (PkaListener           *listener,
                                                               GAsyncResult          *result,
                                                               GError               **error);

G_END_DECLS

#endif /* __PKA_LISTENER_LOWLEVEL_H__ */