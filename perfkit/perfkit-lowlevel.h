/* perfkit-lowlevel.h
 *
 * Copyright (C) 2010 Christian Hergert
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

#ifndef __PERFKIT_LOWLEVEL_H__
#define __PERFKIT_LOWLEVEL_H__

#include <perfkit/perfkit.h>

G_BEGIN_DECLS

gboolean
pk_connection_channel_add_source (PkConnection  *connection,
                                  gint           channel_id,
                                  const gchar   *source_type,
                                  gint          *source_id,
                                  GError       **error);

gboolean
pk_connection_channel_get_target (PkConnection   *connection,
                                  gint            channel_id,
                                  gchar         **target,
                                  GError        **error);

gboolean
pk_connection_channel_get_state (PkConnection    *connection,
                                 gint             channel_id,
                                 PkChannelState  *state,
                                 GError         **error);

gboolean
pk_connection_channel_get_working_dir (PkConnection   *connection,
                                       gint            channel_id,
                                       gchar         **target,
                                       GError        **error);

gboolean
pk_connection_channel_get_args (PkConnection   *connection,
                                gint            channel_id,
                                gchar        ***args,
                                GError        **error);

gboolean
pk_connection_channel_get_env (PkConnection   *connection,
                               gint            channel_id,
                               gchar        ***env,
                               GError        **error);

gboolean
pk_connection_channel_get_pid (PkConnection   *connection,
                               gint            channel_id,
                               GPid           *pid,
                               GError        **error);

gboolean
pk_connection_channel_start (PkConnection   *connection,
                             gint            channel_id,
                             GError        **error);

gboolean
pk_connection_channel_stop (PkConnection   *connection,
                            gint            channel_id,
                            gboolean        killpid,
                            GError        **error);

gboolean
pk_connection_channel_pause (PkConnection   *connection,
                             gint            channel_id,
                             GError        **error);

gboolean
pk_connection_channel_unpause (PkConnection   *connection,
                               gint            channel_id,
                               GError        **error);

gboolean
pk_connection_manager_create_channel (PkConnection   *connection,
                                      PkSpawnInfo    *spawn_info,
                                      gint           *channel_id,
                                      GError        **error);

gboolean
pk_connection_manager_get_channels (PkConnection  *connection,
                                    gint         **channels,
                                    gint          *n_channels,
                                    GError       **error);

gboolean
pk_connection_manager_ping (PkConnection    *connection,
                            GTimeVal        *tv,
                            GError         **error);

gboolean
pk_connection_manager_get_version (PkConnection  *connection,
                                   gchar        **version,
                                   GError       **error);

gboolean
pk_connection_manager_create_subscription (PkConnection    *connection,
                                           gint             channel_id,
                                           gsize            buffer_size,
                                           gulong           buffer_timeout,
                                           const gchar     *encoder_info_uid,
                                           gint            *subscription_id,
                                           GError         **error);

gboolean
pk_connection_manager_get_source_plugins (PkConnection   *connection,
                                          gchar        ***source_infos,
                                          GError        **error);

gboolean
pk_connection_manager_remove_channel (PkConnection  *connection,
                                      gint           channel_id,
                                      GError       **error);

gboolean
pk_connection_channel_remove_source (PkConnection  *connection,
                                     gint           channel_id,
                                     gint           source_id,
                                     GError       **error);

gboolean
pk_connection_subscription_set_handlers (PkConnection   *connection,
                                         gint            subscription_id,
                                         PkManifestFunc  manifest_func,
                                         PkSampleFunc    sample_func,
                                         gpointer        user_data);

void
pk_connection_subscription_deliver_sample (PkConnection *connection,
                                           gint          subscription_id,
                                           const guint8 *data,
                                           gsize         length);

void
pk_connection_subscription_deliver_manifest (PkConnection *connection,
                                             gint          subscription_id,
                                             const guint8 *data,
                                             gsize         length);

gboolean
pk_connection_subscription_enable (PkConnection  *connection,
                                   gint           subscription_id,
                                   GError       **error);

gboolean
pk_connection_subscription_disable (PkConnection  *connection,
                                    gint           subscription_id,
                                    GError       **error);

G_END_DECLS

#endif /* __PERFKIT_LOWLEVEL_H__ */
