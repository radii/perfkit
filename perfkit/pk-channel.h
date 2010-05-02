/* pk-channel.h
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

#ifndef __PK_CHANNEL_H__
#define __PK_CHANNEL_H__

#include <gio/gio.h>

#include "pk-connection.h"

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
	GInitiallyUnowned parent;

	/*< private >*/
	PkChannelPrivate *priv;
};

struct _PkChannelClass
{
	GInitiallyUnownedClass parent_class;
};

GType      pk_channel_get_type               (void) G_GNUC_CONST;
PkChannel* pk_channel_new_for_connection     (PkConnection        *connection);
void       pk_channel_get_args_async         (PkChannel           *channel,
                                              GCancellable        *cancellable,
                                              GAsyncReadyCallback  callback,
                                              gpointer             user_data);
gboolean   pk_channel_get_args_finish        (PkChannel             *channel,
                                              GAsyncResult          *result,
                                              gchar               ***args,
                                              GError               **error);
void       pk_channel_get_env_async          (PkChannel           *channel,
                                              GCancellable        *cancellable,
                                              GAsyncReadyCallback  callback,
                                              gpointer             user_data);
gboolean   pk_channel_get_env_finish         (PkChannel             *channel,
                                              GAsyncResult          *result,
                                              gchar               ***env,
                                              GError               **error);
void       pk_channel_get_pid_async          (PkChannel           *channel,
                                              GCancellable        *cancellable,
                                              GAsyncReadyCallback  callback,
                                              gpointer             user_data);
gboolean   pk_channel_get_pid_finish         (PkChannel            *channel,
                                              GAsyncResult         *result,
                                              GPid                 *pid,
                                              GError              **error);
void       pk_channel_get_state_async        (PkChannel           *channel,
                                              GCancellable        *cancellable,
                                              GAsyncReadyCallback  callback,
                                              gpointer             user_data);
gboolean   pk_channel_get_state_finish       (PkChannel            *channel,
                                              GAsyncResult         *result,
                                              gint                 *state,
                                              GError              **error);
void       pk_channel_get_target_async       (PkChannel           *channel,
                                              GCancellable        *cancellable,
                                              GAsyncReadyCallback  callback,
                                              gpointer             user_data);
gboolean   pk_channel_get_target_finish      (PkChannel            *channel,
                                              GAsyncResult         *result,
                                              gchar               **target,
                                              GError              **error);
void       pk_channel_get_working_dir_async  (PkChannel           *channel,
                                              GCancellable        *cancellable,
                                              GAsyncReadyCallback  callback,
                                              gpointer             user_data);
gboolean   pk_channel_get_working_dir_finish (PkChannel            *channel,
                                              GAsyncResult         *result,
                                              gchar               **working_dir,
                                              GError              **error);
void       pk_channel_get_sources_async      (PkChannel           *channel,
                                              GCancellable        *cancellable,
                                              GAsyncReadyCallback  callback,
                                              gpointer             user_data);
gboolean   pk_channel_get_sources_finish     (PkChannel            *channel,
                                              GAsyncResult         *result,
                                              GList               **sources,
                                              GError              **error);
void       pk_channel_add_source_async       (PkChannel           *channel,
                                              const gchar         *plugin,
                                              GCancellable        *cancellable,
                                              GAsyncReadyCallback  callback,
                                              gpointer             user_data);
gboolean   pk_channel_add_source_finish      (PkChannel            *channel,
                                              GAsyncResult         *result,
                                              gint                 *source,
                                              GError              **error);
void       pk_channel_remove_source_async    (PkChannel           *channel,
                                              gint                 source,
                                              GCancellable        *cancellable,
                                              GAsyncReadyCallback  callback,
                                              gpointer             user_data);
gboolean   pk_channel_remove_source_finish   (PkChannel            *channel,
                                              GAsyncResult         *result,
                                              GError              **error);
void       pk_channel_start_async            (PkChannel           *channel,
                                              GCancellable        *cancellable,
                                              GAsyncReadyCallback  callback,
                                              gpointer             user_data);
gboolean   pk_channel_start_finish           (PkChannel            *channel,
                                              GAsyncResult         *result,
                                              GError              **error);
void       pk_channel_stop_async             (PkChannel           *channel,
                                              gboolean             killpid,
                                              GCancellable        *cancellable,
                                              GAsyncReadyCallback  callback,
                                              gpointer             user_data);
gboolean   pk_channel_stop_finish            (PkChannel            *channel,
                                              GAsyncResult         *result,
                                              GError              **error);
void       pk_channel_pause_async            (PkChannel           *channel,
                                              GCancellable        *cancellable,
                                              GAsyncReadyCallback  callback,
                                              gpointer             user_data);
gboolean   pk_channel_pause_finish           (PkChannel            *channel,
                                              GAsyncResult         *result,
                                              GError              **error);
void       pk_channel_unpause_async          (PkChannel           *channel,
                                              GCancellable        *cancellable,
                                              GAsyncReadyCallback  callback,
                                              gpointer             user_data);
gboolean   pk_channel_unpause_finish         (PkChannel            *channel,
                                              GAsyncResult         *result,
                                              GError              **error);

G_END_DECLS

#endif /* __PK_CHANNEL_H__ */
