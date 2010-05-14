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

gboolean      pk_connection_channel_cork                      (PkConnection          *connection,
                                                               gint                   channel,
                                                               GError               **error);
void          pk_connection_channel_cork_async                (PkConnection          *connection,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_cork_finish               (PkConnection          *connection,
                                                               GAsyncResult          *result,
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
                                                               gboolean               killpid,
                                                               GError               **error);
void          pk_connection_channel_stop_async                (PkConnection          *connection,
                                                               gint                   channel,
                                                               gboolean               killpid,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_stop_finish               (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_channel_uncork                    (PkConnection          *connection,
                                                               gint                   channel,
                                                               GError               **error);
void          pk_connection_channel_uncork_async              (PkConnection          *connection,
                                                               gint                   channel,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_channel_uncork_finish             (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);
gboolean      pk_connection_encoder_get_plugin                (PkConnection          *connection,
                                                               gint                   encoder,
                                                               gchar                **pluign,
                                                               GError               **error);
void          pk_connection_encoder_get_plugin_async          (PkConnection          *connection,
                                                               gint                   encoder,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_encoder_get_plugin_finish         (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gchar                **pluign,
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
gboolean      pk_connection_manager_add_subscription          (PkConnection          *connection,
                                                               gsize                  buffer_size,
                                                               gsize                  timeout,
                                                               gint                   encoder,
                                                               gint                  *subscription,
                                                               GError               **error);
void          pk_connection_manager_add_subscription_async    (PkConnection          *connection,
                                                               gsize                  buffer_size,
                                                               gsize                  timeout,
                                                               gint                   encoder,
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
gboolean      pk_connection_plugin_create_encoder             (PkConnection          *connection,
                                                               const gchar           *plugin,
                                                               gint                  *encoder,
                                                               GError               **error);
void          pk_connection_plugin_create_encoder_async       (PkConnection          *connection,
                                                               const gchar           *plugin,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_plugin_create_encoder_finish      (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gint                  *encoder,
                                                               GError               **error);
gboolean      pk_connection_plugin_create_source              (PkConnection          *connection,
                                                               const gchar           *plugin,
                                                               gint                  *source,
                                                               GError               **error);
void          pk_connection_plugin_create_source_async        (PkConnection          *connection,
                                                               const gchar           *plugin,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_plugin_create_source_finish       (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               gint                  *source,
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
gboolean      pk_connection_subscription_cork                 (PkConnection          *connection,
                                                               gint                   subscription,
                                                               gboolean               drain,
                                                               GError               **error);
void          pk_connection_subscription_cork_async           (PkConnection          *connection,
                                                               gint                   subscription,
                                                               gboolean               drain,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_subscription_cork_finish          (PkConnection          *connection,
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
gboolean      pk_connection_subscription_uncork               (PkConnection          *connection,
                                                               gint                   subscription,
                                                               GError               **error);
void          pk_connection_subscription_uncork_async         (PkConnection          *connection,
                                                               gint                   subscription,
                                                               GCancellable          *cancellable,
                                                               GAsyncReadyCallback    callback,
                                                               gpointer               user_data);
gboolean      pk_connection_subscription_uncork_finish        (PkConnection          *connection,
                                                               GAsyncResult          *result,
                                                               GError               **error);

G_END_DECLS

#endif /* __PK_CONNECTION_LOWLEVEL_H__ */