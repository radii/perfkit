/* pk-channel.h
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

#ifndef __PK_CHANNEL_H__
#define __PK_CHANNEL_H__

#include "pk-connection.h"
#include "pk-source.h"

G_BEGIN_DECLS

#define PK_TYPE_CHANNEL            (pk_channel_get_type())
#define PK_CHANNEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_CHANNEL, PkChannel))
#define PK_CHANNEL_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_CHANNEL, PkChannel const))
#define PK_CHANNEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PK_TYPE_CHANNEL, PkChannelClass))
#define PK_IS_CHANNEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PK_TYPE_CHANNEL))
#define PK_IS_CHANNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PK_TYPE_CHANNEL))
#define PK_CHANNEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PK_TYPE_CHANNEL, PkChannelClass))

typedef struct _PkChannel        PkChannel;
typedef struct _PkChannelClass   PkChannelClass;
typedef struct _PkChannelPrivate PkChannelPrivate;

struct _PkChannel
{
	GObject parent;

	/*< private >*/
	PkChannelPrivate *priv;
};

struct _PkChannelClass
{
	GObjectClass parent_class;
};

GType         pk_channel_get_type                (void) G_GNUC_CONST;
gint          pk_channel_get_id                  (PkChannel             *channel) G_GNUC_PURE;
PkConnection* pk_channel_get_connection          (PkChannel             *channel) G_GNUC_PURE;
gboolean      pk_channel_add_source              (PkChannel             *channel,
                                                  PkSource              *source,
                                                  GError               **error);
void          pk_channel_add_source_async        (PkChannel             *channel,
                                                  PkSource              *source,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_add_source_finish       (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  GError               **error);
gboolean      pk_channel_get_args                (PkChannel             *channel,
                                                  gchar               ***args,
                                                  GError               **error);
void          pk_channel_get_args_async          (PkChannel             *channel,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_get_args_finish         (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  gchar               ***args,
                                                  GError               **error);
gboolean      pk_channel_get_env                 (PkChannel             *channel,
                                                  gchar               ***env,
                                                  GError               **error);
void          pk_channel_get_env_async           (PkChannel             *channel,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_get_env_finish          (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  gchar               ***env,
                                                  GError               **error);
gboolean      pk_channel_get_exit_status         (PkChannel             *channel,
                                                  gint                  *exit_status,
                                                  GError               **error);
void          pk_channel_get_exit_status_async   (PkChannel             *channel,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_get_exit_status_finish  (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  gint                  *exit_status,
                                                  GError               **error);
gboolean      pk_channel_get_kill_pid            (PkChannel             *channel,
                                                  gboolean              *kill_pid,
                                                  GError               **error);
void          pk_channel_get_kill_pid_async      (PkChannel             *channel,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_get_kill_pid_finish     (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  gboolean              *kill_pid,
                                                  GError               **error);
gboolean      pk_channel_get_pid                 (PkChannel             *channel,
                                                  gint                  *pid,
                                                  GError               **error);
void          pk_channel_get_pid_async           (PkChannel             *channel,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_get_pid_finish          (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  gint                  *pid,
                                                  GError               **error);
gboolean      pk_channel_get_pid_set             (PkChannel             *channel,
                                                  gboolean              *pid_set,
                                                  GError               **error);
void          pk_channel_get_pid_set_async       (PkChannel             *channel,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_get_pid_set_finish      (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  gboolean              *pid_set,
                                                  GError               **error);
gboolean      pk_channel_get_sources             (PkChannel             *channel,
                                                  GList                **sources,
                                                  GError               **error);
void          pk_channel_get_sources_async       (PkChannel             *channel,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_get_sources_finish      (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  GList                **sources,
                                                  GError               **error);
gboolean      pk_channel_get_state               (PkChannel             *channel,
                                                  gint                  *state,
                                                  GError               **error);
void          pk_channel_get_state_async         (PkChannel             *channel,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_get_state_finish        (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  gint                  *state,
                                                  GError               **error);
gboolean      pk_channel_get_target              (PkChannel             *channel,
                                                  gchar                **target,
                                                  GError               **error);
void          pk_channel_get_target_async        (PkChannel             *channel,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_get_target_finish       (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  gchar                **target,
                                                  GError               **error);
gboolean      pk_channel_get_working_dir         (PkChannel             *channel,
                                                  gchar                **working_dir,
                                                  GError               **error);
void          pk_channel_get_working_dir_async   (PkChannel             *channel,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_get_working_dir_finish  (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  gchar                **working_dir,
                                                  GError               **error);
gboolean      pk_channel_mute                    (PkChannel             *channel,
                                                  GError               **error);
void          pk_channel_mute_async              (PkChannel             *channel,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_mute_finish             (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  GError               **error);
gboolean      pk_channel_set_args                (PkChannel             *channel,
                                                  gchar                **args,
                                                  GError               **error);
void          pk_channel_set_args_async          (PkChannel             *channel,
                                                  gchar                **args,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_set_args_finish         (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  GError               **error);
gboolean      pk_channel_set_env                 (PkChannel             *channel,
                                                  gchar                **env,
                                                  GError               **error);
void          pk_channel_set_env_async           (PkChannel             *channel,
                                                  gchar                **env,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_set_env_finish          (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  GError               **error);
gboolean      pk_channel_set_kill_pid            (PkChannel             *channel,
                                                  gboolean               kill_pid,
                                                  GError               **error);
void          pk_channel_set_kill_pid_async      (PkChannel             *channel,
                                                  gboolean               kill_pid,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_set_kill_pid_finish     (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  GError               **error);
gboolean      pk_channel_set_pid                 (PkChannel             *channel,
                                                  gint                   pid,
                                                  GError               **error);
void          pk_channel_set_pid_async           (PkChannel             *channel,
                                                  gint                   pid,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_set_pid_finish          (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  GError               **error);
gboolean      pk_channel_set_target              (PkChannel             *channel,
                                                  const gchar           *target,
                                                  GError               **error);
void          pk_channel_set_target_async        (PkChannel             *channel,
                                                  const gchar           *target,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_set_target_finish       (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  GError               **error);
gboolean      pk_channel_set_working_dir         (PkChannel             *channel,
                                                  const gchar           *working_dir,
                                                  GError               **error);
void          pk_channel_set_working_dir_async   (PkChannel             *channel,
                                                  const gchar           *working_dir,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_set_working_dir_finish  (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  GError               **error);
gboolean      pk_channel_start                   (PkChannel             *channel,
                                                  GError               **error);
void          pk_channel_start_async             (PkChannel             *channel,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_start_finish            (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  GError               **error);
gboolean      pk_channel_stop                    (PkChannel             *channel,
                                                  GError               **error);
void          pk_channel_stop_async              (PkChannel             *channel,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_stop_finish             (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  GError               **error);
gboolean      pk_channel_unmute                  (PkChannel             *channel,
                                                  GError               **error);
void          pk_channel_unmute_async            (PkChannel             *channel,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_channel_unmute_finish           (PkChannel             *channel,
                                                  GAsyncResult          *result,
                                                  GError               **error);

G_END_DECLS

#endif /* __PK_CHANNEL_H__ */
