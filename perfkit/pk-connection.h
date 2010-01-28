/* pk-connection.h
 *
 * Copyright (C) 2010 Christian Hergert
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

#if !defined (__PERFKIT_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit/perfkit.h> can be included directly."
#endif

#ifndef __PK_CONNECTION_H__
#define __PK_CONNECTION_H__

#include <glib-object.h>

#include "pk-manager.h"

G_BEGIN_DECLS

#define PK_TYPE_CONNECTION            (pk_connection_get_type())
#define PK_CONNECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_CONNECTION, PkConnection))
#define PK_CONNECTION_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_CONNECTION, PkConnection const))
#define PK_CONNECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PK_TYPE_CONNECTION, PkConnectionClass))
#define PK_IS_CONNECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PK_TYPE_CONNECTION))
#define PK_IS_CONNECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PK_TYPE_CONNECTION))
#define PK_CONNECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PK_TYPE_CONNECTION, PkConnectionClass))

typedef struct _PkConnection        PkConnection;
typedef struct _PkConnectionClass   PkConnectionClass;
typedef struct _PkConnectionPrivate PkConnectionPrivate;

struct _PkConnection
{
	GObject parent;

	/*< private >*/
	PkConnectionPrivate *priv;
};

struct _PkConnectionClass
{
	GObjectClass parent_class;

    /*
     * Connection Operations.
     */

	gboolean       (*connect)                     (PkConnection     *connection,
	                                               GError          **error);

	void           (*disconnect)                  (PkConnection     *connection);

	gboolean       (*is_connected)                (PkConnection     *connection);

    /*
     * Channel Operations.
     */

    gboolean       (*channel_add_source)          (PkConnection     *connection,
	                                               gint              channel_id,
	                                               const gchar      *source_type,
	                                               gint             *source_id,
	                                               GError          **error);

	gboolean       (*channel_get_target)          (PkConnection     *connection,
	                                               gint              channel_id,
	                                               gchar           **target,
	                                               GError          **error);

	gboolean       (*channel_get_working_dir)     (PkConnection     *connection,
	                                               gint              channel_id,
	                                               gchar           **target,
	                                               GError          **error);

	gboolean       (*channel_get_args)            (PkConnection     *connection,
	                                               gint              channel_id,
	                                               gchar          ***args,
	                                               GError          **error);

	gboolean       (*channel_get_env)             (PkConnection     *connection,
	                                               gint              channel_id,
	                                               gchar          ***env,
	                                               GError          **error);

	gboolean       (*channel_get_state)           (PkConnection     *connection,
	                                               gint              channel_id,
                                                   PkChannelState   *state,
                                                   GError          **error);

	gboolean       (*channel_get_pid)             (PkConnection    *connection,
	                                               gint             channel_id,
	                                               GPid            *pid,
	                                               GError         **error);

	gboolean       (*channel_start)               (PkConnection    *connection,
	                                               gint             channel_id,
	                                               GError         **error);

	gboolean       (*channel_stop)                (PkConnection    *connection,
	                                               gint             channel_id,
	                                               gboolean         killpid,
	                                               GError         **error);

	gboolean       (*channel_pause)               (PkConnection    *connection,
	                                               gint             channel_id,
	                                               GError         **error);

	gboolean       (*channel_unpause)             (PkConnection    *connection,
	                                               gint             channel_id,
	                                               GError         **error);

	/*
	 * Encoder Operations.
	 */

	/*
	 * EncoderInfo Operations.
	 */

	/*
	 * Manager Operations.
	 */

	gboolean       (*manager_create_channel)      (PkConnection    *connection,
	                                               PkSpawnInfo     *spawn_info,
	                                               gint            *channel_id,
	                                               GError         **error);

	gboolean       (*manager_get_channels)        (PkConnection    *connection,
	                                               gint           **channels,
	                                               gint            *n_channels,
	                                               GError         **error);

	gboolean       (*manager_ping)                (PkConnection    *connection,
	                                               GTimeVal        *tv,
	                                               GError         **error);

	gboolean       (*manager_get_version)         (PkConnection    *connection,
	                                               gchar          **version,
	                                               GError         **error);

	gboolean       (*manager_create_subscription) (PkConnection    *connection,
	                                               PkChannel       *channel,
	                                               gsize            buffer_size,
	                                               gulong           buffer_timeout,
	                                               PkEncoderInfo   *encoder_info,
	                                               GError         **error);

	gboolean       (*manager_get_source_infos)    (PkConnection    *connection,
	                                               gchar         ***encoder_infos,
	                                               GError         **error);

	gboolean       (*manager_remove_channel)      (PkConnection    *connection,
	                                               gint             channel_id,
	                                               GError         **error);

	/*
	 * Source Operations.
	 */

	/*
	 * SourceInfo Operations.
	 */
};

GType         pk_connection_get_type     (void) G_GNUC_CONST;
PkConnection* pk_connection_new_from_uri (const gchar *uri);
gboolean      pk_connection_connect      (PkConnection  *connection,
                                          GError       **error);
void          pk_connection_disconnect   (PkConnection  *connection);
gboolean      pk_connection_is_connected (PkConnection  *connection);
PkManager*    pk_connection_get_manager  (PkConnection  *connection);
const gchar*  pk_connection_get_uri      (PkConnection  *connection);

G_END_DECLS

#endif /* __PK_CONNECTION_H__ */
