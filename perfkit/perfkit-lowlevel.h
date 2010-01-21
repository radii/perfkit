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

gboolean pk_connection_channel_get_target      (PkConnection   *connection,
                                                gint            channel_id,
                                                gchar         **target,
                                                GError        **error);
gboolean pk_connection_channel_get_working_dir (PkConnection   *connection,
                                                gint            channel_id,
                                                gchar         **target,
                                                GError        **error);
gboolean pk_connection_channel_get_args        (PkConnection   *connection,
                                                gint            channel_id,
                                                gchar        ***args,
                                                GError        **error);
gboolean pk_connection_channel_get_env         (PkConnection   *connection,
                                                gint            channel_id,
                                                gchar        ***env,
                                                GError        **error);
gboolean pk_connection_channel_get_pid         (PkConnection   *connection,
                                                gint            channel_id,
                                                GPid           *pid,
                                                GError        **error);
gboolean pk_connection_channel_start           (PkConnection   *connection,
                                                gint            channel_id,
                                                GError        **error);
gboolean pk_connection_channel_stop            (PkConnection   *connection,
                                                gint            channel_id,
                                                gboolean        killpid,
                                                GError        **error);
gboolean pk_connection_channel_pause           (PkConnection   *connection,
                                                gint            channel_id,
                                                GError        **error);
gboolean pk_connection_channel_unpause         (PkConnection   *connection,
                                                gint            channel_id,
                                                GError        **error);
gboolean pk_connection_manager_create_channel  (PkConnection   *connection,
                                                PkSpawnInfo    *spawn_info,
                                                gint           *channel_id,
                                                GError        **error);

G_END_DECLS

#endif /* __PERFKIT_LOWLEVEL_H__ */
