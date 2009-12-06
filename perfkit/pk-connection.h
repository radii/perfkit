/* pk-connection.h
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
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

#ifndef __PK_CONNECTION_H__
#define __PK_CONNECTION_H__

#include <gio/gio.h>

#include "pk-channel.h"
#include "pk-channels.h"
#include "pk-sources.h"
#include "pk-sample.h"

G_BEGIN_DECLS

#define PK_TYPE_CONNECTION            (pk_connection_get_type ())
#define PK_CONNECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_CONNECTION, PkConnection))
#define PK_CONNECTION_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_CONNECTION, PkConnection const))
#define PK_CONNECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PK_TYPE_CONNECTION, PkConnectionClass))
#define PK_IS_CONNECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PK_TYPE_CONNECTION))
#define PK_IS_CONNECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PK_TYPE_CONNECTION))
#define PK_CONNECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PK_TYPE_CONNECTION, PkConnectionClass))
#define PK_CONNECTION_ERROR           (pk_connection_error_quark())

/**
 * PkConnectionError:
 * @PK_CONNECTION_ERROR_INVALID: 
 *
 * The #PkConnectionError enumeration.
 */
typedef enum
{
	PK_CONNECTION_ERROR_INVALID,
} PkConnectionError;

typedef struct _PkConnection        PkConnection;
typedef struct _PkConnectionClass   PkConnectionClass;
typedef struct _PkConnectionPrivate PkConnectionPrivate;

/**
 * PkSampleCallback:
 * @sample: A #PkSample
 * @user_data: user data supplied to subscription
 *
 * A callback for a sample received from a perfkit-daemon.
 */
typedef void (*PkSampleCallback) (PkSample *sample, gpointer user_data);

struct _PkConnection
{
	GObject parent;

	/*< private >*/
	PkConnectionPrivate *priv;
};

struct _PkConnectionClass
{
	GObjectClass parent_class;

	gboolean          (*connect)                  (PkConnection         *connection,
	                                               GError              **error);

	/*
	 * Daemon Connection Implementation
	 */
	void              (*connect_async)            (PkConnection         *connection,
	                                               GAsyncReadyCallback   callback,
	                                               gpointer              user_data);
	gboolean          (*connect_finish)           (PkConnection         *connection,
	                                               GAsyncResult         *result,
	                                               GError              **error);

	void              (*disconnect)               (PkConnection         *connection);
	void              (*disconnect_async)         (PkConnection         *connection,
	                                               GAsyncReadyCallback   callback,
	                                               gpointer              user_data);
	void              (*disconnect_finish)        (PkConnection         *connection,
	                                               GAsyncResult         *result);

	/*
	 * Channels Service RPCs
	 */
	gboolean          (*channels_add)             (PkConnection         *connection,
	                                               gint                 *channel_id,
	                                               GError              **error);
	gboolean          (*channels_find_all)        (PkConnection         *connection,
	                                               gint                **channel_ids,
	                                               gint                 *n_channels);
	void              (*channels_find_all_async)  (PkConnection         *connection,
	                                               GAsyncReadyCallback   callback,
	                                               gpointer              user_data);
	gboolean          (*channels_find_all_finish) (PkConnection         *connection,
	                                               GAsyncResult         *result,
	                                               gint                **channel_ids,
	                                               gint                 *n_channels,
	                                               GError              **error);

	/*
	 * Channel RPCs
	 */
	gchar*            (*channel_get_target)       (PkConnection         *connection,
	                                               gint                  channel_id);
	void              (*channel_set_target)       (PkConnection         *connection,
	                                               gint                  channel_id,
	                                               const gchar          *target);
	gchar**           (*channel_get_args)         (PkConnection         *connection,
	                                               gint                  channel_id);
	void              (*channel_set_args)         (PkConnection         *connection,
	                                               gint                  channel_id,
	                                               gchar               **args);
	gchar*            (*channel_get_dir)          (PkConnection         *connection,
	                                               gint                  channel_id);
	void              (*channel_set_dir)          (PkConnection         *connection,
	                                               gint                  channel_id,
	                                               const gchar          *dir);
	gchar**           (*channel_get_env)          (PkConnection         *connection,
	                                               gint                  channel_id);
	void              (*channel_set_env)          (PkConnection         *connection,
	                                               gint                  channel_id,
	                                               gchar               **env);
	GPid              (*channel_get_pid)          (PkConnection         *connection,
	                                               gint                  channel_id);
	void              (*channel_set_pid)          (PkConnection         *connection,
	                                               gint                  channel_id,
	                                               GPid                  pid);
	PkChannelState    (*channel_get_state)        (PkConnection         *connection,
	                                               gint                  channel_id);
	gboolean          (*channel_start)            (PkConnection         *connection,
	                                               gint                  channel_id,
	                                               GError              **error);
	gboolean          (*channel_stop)             (PkConnection         *connection,
	                                               gint                  channel_id,
	                                               GError              **error);
	gboolean          (*channel_pause)            (PkConnection         *connection,
	                                               gint                  channel_id,
	                                               GError              **error);
	gboolean          (*channel_unpause)          (PkConnection         *connection,
	                                               gint                  channel_id,
	                                               GError              **error);
	void              (*channel_subscribe)        (PkConnection         *connection,
	                                               gint                  channel_id,
	                                               PkSampleCallback      callback,
	                                               gpointer              user_data);

	/*
	 * Sources Service RPCs
	 */

	gchar**           (*sources_get_types)        (PkConnection         *connection);
	gboolean          (*sources_add)              (PkConnection         *connection,
	                                               const gchar          *type,
	                                               gint                 *source_id,
	                                               GError              **error);

	/*
	 * Source RPCs
	 */
	void              (*source_set_channel)       (PkConnection         *connection,
	                                               gint                  source_id,
	                                               gint                  channel_id);

	/*
	 * Future Expansion
	 */
	gpointer          reserved [64];
};

GType         pk_connection_get_type           (void) G_GNUC_CONST;
GQuark        pk_connection_error_quark        (void) G_GNUC_CONST;
PkConnection* pk_connection_new_for_uri        (const gchar          *uri);

/*
 * Daemon Connection
 */
gboolean      pk_connection_connect            (PkConnection         *connection,
                                                GError              **error);
void          pk_connection_connect_async      (PkConnection         *connection,
                                                GAsyncReadyCallback   callback,
                                                gpointer              user_data);
gboolean      pk_connection_connect_finish     (PkConnection         *connection,
                                                GAsyncResult         *result,
                                                GError              **error);
void          pk_connection_disconnect         (PkConnection         *connection);
void          pk_connection_disconnect_async   (PkConnection         *connection,
                                                GAsyncReadyCallback   callback,
                                                gpointer              user_data);
void          pk_connection_disconnect_finish  (PkConnection         *connection,
                                                GAsyncResult         *result);

PkChannels*   pk_connection_get_channels       (PkConnection         *connection);
PkSources*    pk_connection_get_sources        (PkConnection         *connection);

G_END_DECLS

#endif /* __PK_CONNECTION_H__ */
